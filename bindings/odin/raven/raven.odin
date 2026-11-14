// Raven for Odin.
//
// Generated from the C headers by tools/bindgen (Raven 0.2): do not edit.
//
// The C API as it is, under its own names. Enumerations are i32 with constants
// (R_BACKEND_VULKAN), structures have the layout of C and are zeroed when
// declared (`desc: RInitDesc`), texts are cstring: an Odin string literal
// passes as it is, other strings go through strings.clone_to_cstring.
// A callback is a `proc "c"`; to use the context of Odin inside (fmt,
// allocations), start it with `context = runtime.default_context()`.
//
//   package hello
//
//   import r "raven"
//
//   main :: proc() {
//       r.Rinit(r.R_BACKEND_VULKAN)
//       xml :: "<Window Title='Hi'><Button Text='Quit' OnClicked='quit'/></Window>"
//       r.Rwindow_create_from_xml(xml, len(xml))
//       r.Raction_bind("quit", proc "c" (sender: r.Rid, action: cstring, user_data: rawptr) {
//           r.Rquit(0)
//       }, nil)
//       for r.Rwait_frame(-1.0) != 0 {}
//       r.Rshutdown()
//   }
//
// It links the shared library: raven.lib (the import library of raven.dll) on
// Windows, libraven.so / libraven.dylib elsewhere, found where the linker
// looks: -extra-linker-flags:"/LIBPATH:<dir>" on Windows, "-L<dir>" elsewhere.
package raven

when ODIN_OS == .Windows {
	foreign import lib "system:raven.lib"
} else {
	foreign import lib "system:raven"
}

Rid :: u32
Ruint :: u32

RAVEN_VERSION_MAJOR :: 0
RAVEN_VERSION_MINOR :: 2
RAVEN_VERSION_PATCH :: 3
RAVEN_API_VERSION :: "0.2"
R_FALSE :: 0
R_TRUE :: 1
R_INVALID_ID : Rid : 0xFFFFFFFF

RPainter :: struct {}

RBackend :: i32
// Built-in Vulkan renderer.
R_BACKEND_VULKAN :: 1
// Renders nothing. Used for headless runs and unit tests.
R_BACKEND_NULL :: 2
// Backend supplied with Rbackend_register (see raven_backend.h).
R_BACKEND_CUSTOM :: 3
// Browser renderer (Emscripten, WebGL 2).
R_BACKEND_WEBGL :: 4

RError :: i32
R_NO_ERROR :: 0x0070
R_ERROR_INVALID_ID :: 0x0071
R_ERROR_INVALID_ENUM :: 0x0072
R_ERROR_INVALID_VALUE :: 0x0073
R_ERROR_INVALID_OPERATION :: 0x0074
R_ERROR_BACKEND :: 0x0075
R_ERROR_PLATFORM :: 0x0076
R_ERROR_OUT_OF_MEMORY :: 0x0077
R_ERROR_INVALID_FILE_FORMAT :: 0x0078
R_ERROR_FILE_NOT_FOUND :: 0x0079
R_ERROR_XML :: 0x007A
R_ERROR_WRONG_THREAD :: 0x007B

RSeverity :: i32
R_SEVERITY_WEAK_WARNING :: 0x0080
R_SEVERITY_WARNING :: 0x0081
R_SEVERITY_ERROR :: 0x0082
R_SEVERITY_FATAL :: 0x0083

RWindowFlags :: i32
R_WINDOW_DEFAULT :: 0
R_WINDOW_NO_RESIZE :: 1
R_WINDOW_HIDDEN :: 2
R_WINDOW_BORDERLESS :: 4
R_WINDOW_MAXIMIZED :: 8
// No title bar from the OS: the content covers the whole window and
// draws its own header, with a R_WIDGET_HEADER_BAR (raven_shell.h) or
// with any widget given to Rwidget_set_window_drag. The window is still
// moved, resized, snapped and maximized by the OS, as it does for its own
// title bars.
R_WINDOW_CUSTOM_HEADER :: 0x0010
// The window can be seen through: where the application draws nothing
// opaque, what is behind the window on the screen shows (the desktop,
// the other applications), not a color of the application. Glass then
// looks at the screen itself. What the OS does to what shows through is
// chosen with Rwindow_set_backdrop. See RBackdrop for what each window
// system can do.
R_WINDOW_TRANSPARENT :: 0x0020
// A splash screen: shown while the application starts. No frame (it
// implies R_WINDOW_BORDERLESS and R_WINDOW_NO_RESIZE, and cancels
// R_WINDOW_MAXIMIZED and R_WINDOW_CUSTOM_HEADER), centered on the screen
// before it is first shown and again when its size changes, above the
// other windows, out of the taskbar and of Alt+Tab (Windows), of the
// Window menu and of Cmd+` (macOS), typed _NET_WM_WINDOW_TYPE_SPLASH on
// X11. Wayland gives it no place of its own: it is a window without frame
// there. The application destroys it once its main window is ready
// (creating the main window first keeps Ris_running true).
R_WINDOW_SPLASH_SCREEN :: 0x0040

// Where a window in picture in picture goes (Rwindow_set_picture_in_picture_layout).
RPictureInPictureCorner :: i32
R_PIP_BOTTOM_RIGHT :: 0x0780
R_PIP_BOTTOM_LEFT :: 0x0781
R_PIP_TOP_RIGHT :: 0x0782
R_PIP_TOP_LEFT :: 0x0783

// What is seen behind a window created with R_WINDOW_TRANSPARENT.
//
//   Windows 11          every value
//   Windows 10          TRANSPARENT, BLUR, ACRYLIC (MICA gives ACRYLIC)
//   macOS               TRANSPARENT; other backdrops fall back to transparency
//   X11                 TRANSPARENT with any compositing manager; BLUR where
//                       the compositor blurs on request (KWin; picom and
//                       others with their own settings); ACRYLIC and MICA
//                       give BLUR
//   no compositor       nothing: the window stays opaque
RBackdrop :: i32
// Opaque: the window color, or the wallpaper of the theme.
R_BACKDROP_NONE :: 0x0400
// The screen behind the window, as it is.
R_BACKDROP_TRANSPARENT :: 0x0401
// The screen behind the window, blurred by the OS.
R_BACKDROP_BLUR :: 0x0402
// Blurred, tinted and grained by the OS (the acrylic of Windows).
R_BACKDROP_ACRYLIC :: 0x0403
// A soft tint taken from the wallpaper of the desktop (the mica of Windows 11).
R_BACKDROP_MICA :: 0x0404

RWindowSystem :: i32
R_WINDOW_SYSTEM_NONE :: 0x0090
R_WINDOW_SYSTEM_WIN32 :: 0x0091
R_WINDOW_SYSTEM_X11 :: 0x0092
R_WINDOW_SYSTEM_WAYLAND :: 0x0093
R_WINDOW_SYSTEM_COCOA :: 0x0094
R_WINDOW_SYSTEM_EMSCRIPTEN :: 0x0095
R_WINDOW_SYSTEM_ANDROID :: 0x0096
R_WINDOW_SYSTEM_UIKIT :: 0x0097

// What a text input holds: on mobile it picks the on-screen keyboard (letters, @, digits...).
RTextInputType :: i32
R_TEXT_INPUT_TYPE_TEXT :: 0x0B60
R_TEXT_INPUT_TYPE_EMAIL :: 0x0B61
R_TEXT_INPUT_TYPE_NUMBER :: 0x0B62
// Digits and a decimal separator.
R_TEXT_INPUT_TYPE_DECIMAL :: 0x0B63
R_TEXT_INPUT_TYPE_PHONE :: 0x0B64
R_TEXT_INPUT_TYPE_URL :: 0x0B65
R_TEXT_INPUT_TYPE_SEARCH :: 0x0B66

// The label and meaning of the return key of the on-screen keyboard. Enter does the same on every key.
RReturnKey :: i32
R_RETURN_KEY_DEFAULT :: 0x0B70
R_RETURN_KEY_DONE :: 0x0B71
R_RETURN_KEY_GO :: 0x0B72
R_RETURN_KEY_NEXT :: 0x0B73
R_RETURN_KEY_SEARCH :: 0x0B74
R_RETURN_KEY_SEND :: 0x0B75

// Edges and corners of a window, for Rwindow_begin_resize.
RWindowEdge :: i32
R_EDGE_LEFT :: 0x00D0
R_EDGE_RIGHT :: 0x00D1
R_EDGE_TOP :: 0x00D2
R_EDGE_BOTTOM :: 0x00D3
R_EDGE_TOP_LEFT :: 0x00D4
R_EDGE_TOP_RIGHT :: 0x00D5
R_EDGE_BOTTOM_LEFT :: 0x00D6
R_EDGE_BOTTOM_RIGHT :: 0x00D7

RMouseButton :: i32
R_MOUSE_LEFT :: 0
R_MOUSE_RIGHT :: 1
R_MOUSE_MIDDLE :: 2
R_MOUSE_X1 :: 3
R_MOUSE_X2 :: 4

RModifier :: i32
R_MOD_SHIFT :: 1
R_MOD_CTRL :: 2
R_MOD_ALT :: 4
R_MOD_SUPER :: 8

RCursor :: i32
R_CURSOR_ARROW :: 0x00B0
R_CURSOR_IBEAM :: 0x00B1
R_CURSOR_HAND :: 0x00B2
R_CURSOR_CROSSHAIR :: 0x00B3
R_CURSOR_RESIZE_H :: 0x00B4
R_CURSOR_RESIZE_V :: 0x00B5
R_CURSOR_RESIZE_NWSE :: 0x00B6
R_CURSOR_RESIZE_NESW :: 0x00B7
R_CURSOR_MOVE :: 0x00B8
R_CURSOR_NOT_ALLOWED :: 0x00B9
// No pointer at all (a game that hides it). Windows; elsewhere the arrow.
R_CURSOR_NONE :: 0x00BA

// Keyboard keys. Printable keys use their upper-case ASCII code.
RKey :: i32
R_KEY_UNKNOWN :: 0
R_KEY_SPACE :: 0x0020
R_KEY_APOSTROPHE :: 0x0027
R_KEY_COMMA :: 0x002C
R_KEY_MINUS :: 0x002D
R_KEY_PERIOD :: 0x002E
R_KEY_SLASH :: 0x002F
R_KEY_0 :: 0x0030
R_KEY_1 :: 0x0031
R_KEY_2 :: 0x0032
R_KEY_3 :: 0x0033
R_KEY_4 :: 0x0034
R_KEY_5 :: 0x0035
R_KEY_6 :: 0x0036
R_KEY_7 :: 0x0037
R_KEY_8 :: 0x0038
R_KEY_9 :: 0x0039
R_KEY_SEMICOLON :: 0x003B
R_KEY_EQUAL :: 0x003D
R_KEY_A :: 0x0041
R_KEY_B :: 0x0042
R_KEY_C :: 0x0043
R_KEY_D :: 0x0044
R_KEY_E :: 0x0045
R_KEY_F :: 0x0046
R_KEY_G :: 0x0047
R_KEY_H :: 0x0048
R_KEY_I :: 0x0049
R_KEY_J :: 0x004A
R_KEY_K :: 0x004B
R_KEY_L :: 0x004C
R_KEY_M :: 0x004D
R_KEY_N :: 0x004E
R_KEY_O :: 0x004F
R_KEY_P :: 0x0050
R_KEY_Q :: 0x0051
R_KEY_R :: 0x0052
R_KEY_S :: 0x0053
R_KEY_T :: 0x0054
R_KEY_U :: 0x0055
R_KEY_V :: 0x0056
R_KEY_W :: 0x0057
R_KEY_X :: 0x0058
R_KEY_Y :: 0x0059
R_KEY_Z :: 0x005A
R_KEY_LEFT_BRACKET :: 0x005B
R_KEY_BACKSLASH :: 0x005C
R_KEY_RIGHT_BRACKET :: 0x005D
R_KEY_GRAVE :: 0x0060
R_KEY_ESCAPE :: 0x0100
R_KEY_ENTER :: 0x0101
R_KEY_TAB :: 0x0102
R_KEY_BACKSPACE :: 0x0103
R_KEY_INSERT :: 0x0104
R_KEY_DELETE :: 0x0105
R_KEY_RIGHT :: 0x0106
R_KEY_LEFT :: 0x0107
R_KEY_DOWN :: 0x0108
R_KEY_UP :: 0x0109
R_KEY_PAGE_UP :: 0x010A
R_KEY_PAGE_DOWN :: 0x010B
R_KEY_HOME :: 0x010C
R_KEY_END :: 0x010D
R_KEY_CAPS_LOCK :: 0x010E
R_KEY_PRINT_SCREEN :: 0x010F
R_KEY_PAUSE :: 0x0110
R_KEY_F1 :: 0x0111
R_KEY_F2 :: 0x0112
R_KEY_F3 :: 0x0113
R_KEY_F4 :: 0x0114
R_KEY_F5 :: 0x0115
R_KEY_F6 :: 0x0116
R_KEY_F7 :: 0x0117
R_KEY_F8 :: 0x0118
R_KEY_F9 :: 0x0119
R_KEY_F10 :: 0x011A
R_KEY_F11 :: 0x011B
R_KEY_F12 :: 0x011C
R_KEY_KP_0 :: 0x011D
R_KEY_KP_1 :: 0x011E
R_KEY_KP_2 :: 0x011F
R_KEY_KP_3 :: 0x0120
R_KEY_KP_4 :: 0x0121
R_KEY_KP_5 :: 0x0122
R_KEY_KP_6 :: 0x0123
R_KEY_KP_7 :: 0x0124
R_KEY_KP_8 :: 0x0125
R_KEY_KP_9 :: 0x0126
R_KEY_KP_DECIMAL :: 0x0127
R_KEY_KP_DIVIDE :: 0x0128
R_KEY_KP_MULTIPLY :: 0x0129
R_KEY_KP_SUBTRACT :: 0x012A
R_KEY_KP_ADD :: 0x012B
R_KEY_KP_ENTER :: 0x012C
R_KEY_LEFT_SHIFT :: 0x012D
R_KEY_LEFT_CTRL :: 0x012E
R_KEY_LEFT_ALT :: 0x012F
R_KEY_LEFT_SUPER :: 0x0130
R_KEY_RIGHT_SHIFT :: 0x0131
R_KEY_RIGHT_CTRL :: 0x0132
R_KEY_RIGHT_ALT :: 0x0133
R_KEY_RIGHT_SUPER :: 0x0134
R_KEY_MENU :: 0x0135
R_KEY_COUNT :: 0x0136

REventType :: i32
R_EVENT_NONE :: 0x00C0
R_EVENT_MOUSE_MOVE :: 0x00C1
R_EVENT_MOUSE_DOWN :: 0x00C2
R_EVENT_MOUSE_UP :: 0x00C3
R_EVENT_MOUSE_WHEEL :: 0x00C4
R_EVENT_MOUSE_ENTER :: 0x00C5
R_EVENT_MOUSE_LEAVE :: 0x00C6
R_EVENT_KEY_DOWN :: 0x00C7
R_EVENT_KEY_UP :: 0x00C8
// A character was typed. `codepoint` holds the Unicode scalar value.
R_EVENT_TEXT :: 0x00C9
R_EVENT_FOCUS_GAINED :: 0x00CA
R_EVENT_FOCUS_LOST :: 0x00CB

// Moments of a drag coming from another application (files from the file manager, text...).
RDropPhase :: i32
// The pointer came over the target.
R_DROP_ENTER :: 0x00E0
// It moves over the target.
R_DROP_MOVE :: 0x00E1
// It left the target without dropping.
R_DROP_LEAVE :: 0x00E2
// The user dropped: `paths` and `text` hold what was dropped.
R_DROP_DROPPED :: 0x00E3

// What a finger does on a widget (Rwidget_set_touch_mode).
RTouchMode :: i32
// The finger scrolls what scrolls under it, and is a mouse elsewhere (default).
R_TOUCH_MODE_AUTO :: 0x0480
// A finger moving on the widget always scrolls what contains it, even over a control that drags.
R_TOUCH_MODE_SCROLL :: 0x0481
// The finger is a mouse on the widget: pressing presses at once, moving drags, nothing scrolls.
R_TOUCH_MODE_MOUSE :: 0x0482
// Fingers are ignored on the widget (only its touch callback sees them): a surface for the pen.
R_TOUCH_MODE_NONE :: 0x0483

RTouchPhase :: i32
// A finger came down.
R_TOUCH_DOWN :: 0x0490
R_TOUCH_MOVE :: 0x0491
// The finger was lifted.
R_TOUCH_UP :: 0x0492
// The system took the finger away (a gesture of its own, the window lost it): nothing must happen as for a lift.
R_TOUCH_CANCEL :: 0x0493

RPenPhase :: i32
// The tip touched the screen.
R_PEN_DOWN :: 0x04A0
// The pen moves while touching.
R_PEN_MOVE :: 0x04A1
// The tip left the screen.
R_PEN_UP :: 0x04A2
// The pen moves near the screen without touching it.
R_PEN_HOVER :: 0x04A3
// The pen went out of reach.
R_PEN_LEAVE :: 0x04A4

// Buttons on the side of a pen (RPenEvent::buttons).
RPenButton :: i32
R_PEN_BUTTON_BARREL :: 1
R_PEN_BUTTON_BARREL2 :: 2

RGesturePhase :: i32
// A second finger came down (a touch screen), or the system recognized a gesture (a trackpad).
R_GESTURE_BEGIN :: 0x04B0
R_GESTURE_UPDATE :: 0x04B1
// Fewer than two fingers remain, or the fingers left the trackpad (RGestureEvent::cancelled: the system took the gesture back).
R_GESTURE_END :: 0x04B2

// What the fingers of a gesture do (RGestureEvent::kind).
RGestureKind :: i32
// Two fingers that pinch, turn and move together: scale, rotation and pan.
R_GESTURE_PINCH :: 0x04C0
// Three or more fingers that move together on a trackpad: only the pan
// (scale stays 1). By default, a swipe across goes back or forward.
R_GESTURE_SWIPE :: 0x04C1

// Where the fingers of a gesture are (RGestureEvent::device).
RGestureDevice :: i32
// On the screen: the library recognized the gesture.
R_GESTURE_DEVICE_TOUCHSCREEN :: 0x04D0
// On a trackpad: the system recognized the gesture; the fingers are not on the window (x, y is the pointer).
R_GESTURE_DEVICE_TRACKPAD :: 0x04D1

// The looks. Each one comes dark and light.
//
// Raven's looks are the default; the ones of the platforms imitate them
// closely (their shapes, colors, sizes, states and motion), without using
// their widgets. The looks of the platforms are opaque, except macOS Glass
// (macOS 26), whose bars, menus and floating surfaces are glass as on the
// system; the glass materials themselves (Rtheme_set_material) belong to
// the glass looks of Raven.
RLook :: i32
// Raven, flat and dense, for tools. The look at start.
R_LOOK_RAVEN_DARK :: 0x00E0
R_LOOK_RAVEN_LIGHT :: 0x00E1
// Raven in glass: controls are capsules of translucent glass that refract
// and blur what is behind them, catch the light on their rim and react
// to the pointer with short animations. Windows get a soft colored
// background unless they have their own (Rwindow_set_background,
// Rwindow_set_clear_color).
R_LOOK_RAVEN_GLASS_DARK :: 0x00E2
R_LOOK_RAVEN_GLASS_LIGHT :: 0x00E3
// High contrast, for low vision: pure black and white, yellow (or blue)
// focus and selection, thick borders, larger text (raven_accessibility.h,
// Raccessibility_set_high_contrast).
R_LOOK_RAVEN_HIGH_CONTRAST_DARK :: 0x00E4
R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT :: 0x00E5
// Windows 11 (Fluent): its controls, its colors, the accent color the
// user chose (on Windows), the Segoe UI font when the system has it, and
// motion without bounce.
R_LOOK_WINDOWS_DARK :: 0x00E6
R_LOOK_WINDOWS_LIGHT :: 0x00E7
// macOS (Sequoia, Aqua): its controls, its colors, the accent color the
// user chose (on macOS), the San Francisco font when the system has it,
// no change under the pointer, and selections in the accent color.
R_LOOK_MACOS_DARK :: 0x00E8
R_LOOK_MACOS_LIGHT :: 0x00E9
// GNOME (libadwaita), the desktop of most Linux distributions: flat
// rounded controls, the accent and the font of the desktop (on Linux).
R_LOOK_GNOME_DARK :: 0x00EA
R_LOOK_GNOME_LIGHT :: 0x00EB
// The look of the platform the application runs on: Windows on Windows,
// macOS on macOS (macOS Glass from macOS 26 on), GNOME on Linux and the
// other systems.
R_LOOK_NATIVE_DARK :: 0x00EC
R_LOOK_NATIVE_LIGHT :: 0x00ED
// macOS 26 (Tahoe, Liquid Glass): the controls of macOS, larger and
// rounder, and glass where the system puts it, on what floats above the
// content: header bars and their buttons, menus, drop-down lists,
// tooltips, sheets; the knobs of sliders and switches turn into a lens
// of clear glass while they are held. The content itself (fields, lists,
// text) stays opaque. The accent color and the font of the system, on
// macOS.
R_LOOK_MACOS_GLASS_DARK :: 0x00EE
R_LOOK_MACOS_GLASS_LIGHT :: 0x00EF

// @deprecated The built-in themes of Raven 0.2, now looks: R_THEME_DARK is
// R_LOOK_RAVEN_DARK and so on, and Rtheme_apply is Rlook_set.
RTheme :: i32
// Flat, dense themes for tools.
R_THEME_DARK :: 0x00E0
R_THEME_LIGHT :: 0x00E1
// Glass themes: controls are capsules of translucent glass that refract
// and blur what is behind them, catch the light on their rim and react
// to the pointer with short animations. Windows get a soft colored
// background unless they have their own (Rwindow_set_background,
// Rwindow_set_clear_color).
R_THEME_GLASS_DARK :: 0x00E2
R_THEME_GLASS_LIGHT :: 0x00E3
// High contrast, for low vision: pure black and white, yellow (or blue)
// focus and selection, thick borders, larger text (raven_accessibility.h,
// Raccessibility_set_high_contrast).
R_THEME_HIGH_CONTRAST_DARK :: 0x00E4
R_THEME_HIGH_CONTRAST_LIGHT :: 0x00E5

// What surfaces are made of, from the clearest to the most opaque. A theme
// has one material for everything (Rtheme_set_material); any widget can be
// given its own (Rwidget_set_material, Material="..." in XML).
//
// The three glasses share the same anatomy (a tint over what is behind, a
// rim that bends it and catches the light, a shadow) and differ by how much
// they let through. Within one material, surfaces are also told apart by
// their height above the window: bars lie flat, panels float a little,
// controls a little more, menus and dialogs highest; each level has a
// brighter rim and a deeper shadow than the one below.
RMaterial :: i32
// Opaque: filled with the colors of the theme. The material of R_THEME_DARK and R_THEME_LIGHT.
R_MATERIAL_SOLID :: 0x02E0
// Clear glass: what is behind stays sharp enough to be recognized, strongly bent at the rim.
R_MATERIAL_GLASS :: 0x02E1
// Frosted glass: what is behind is blurred into soft colors. The material of the glass themes.
R_MATERIAL_FROSTED_GLASS :: 0x02E2
// Frosted glass colored by the accent color of the theme (or of the widget): for what must stand out.
R_MATERIAL_TINTED_GLASS :: 0x02E3
// Not a material: "the one of the theme". Removes the material given to a widget.
R_MATERIAL_DEFAULT :: 0x02E4

// Color roles. The comment lists the widgets reading each role.
RColorRole :: i32
// Window clear color.
R_COLOR_WINDOW :: 0x0100
// Border background.
R_COLOR_PANEL :: 0x0101
// Border outline, list and input outlines.
R_COLOR_BORDER :: 0x0102
// Text. Inherited by the children of the widget it is set on.
R_COLOR_TEXT :: 0x0103
R_COLOR_TEXT_DISABLED :: 0x0104
R_COLOR_TEXT_PLACEHOLDER :: 0x0105
// Primary buttons, checked boxes, slider fill, progress fill.
R_COLOR_ACCENT :: 0x0106
R_COLOR_ACCENT_HOVERED :: 0x0107
R_COLOR_ACCENT_PRESSED :: 0x0108
// Text and marks drawn on top of the accent color.
R_COLOR_ACCENT_TEXT :: 0x0109
// Button, combo box, check box background.
R_COLOR_CONTROL :: 0x010A
R_COLOR_CONTROL_HOVERED :: 0x010B
R_COLOR_CONTROL_PRESSED :: 0x010C
R_COLOR_CONTROL_BORDER :: 0x010D
// Text input, spin box, list and tree background.
R_COLOR_INPUT :: 0x010E
// Keyboard focus outline.
R_COLOR_FOCUS :: 0x010F
// Selected rows and selected text.
R_COLOR_SELECTION :: 0x0110
R_COLOR_ROW_HOVERED :: 0x0111
// Slider track, progress background, scrollbar track.
R_COLOR_TRACK :: 0x0112
// Scrollbar thumb, slider handle.
R_COLOR_HANDLE :: 0x0113
R_COLOR_HANDLE_HOVERED :: 0x0114
R_COLOR_SEPARATOR :: 0x0115
// Menu bar, tab bar, expander header.
R_COLOR_HEADER :: 0x0116
R_COLOR_HEADER_HOVERED :: 0x0117
// Active tab.
R_COLOR_TAB_ACTIVE :: 0x0118
// Menus, combo box lists.
R_COLOR_POPUP :: 0x0119
R_COLOR_POPUP_BORDER :: 0x011A
R_COLOR_SHADOW :: 0x011B
R_COLOR_TOOLTIP :: 0x011C
R_COLOR_TOOLTIP_TEXT :: 0x011D
// Track of a switch that is on.
R_COLOR_SWITCH :: 0x011E
// What went well, what needs attention, what is wrong or destroys something (markers, badges, classes).
R_COLOR_SUCCESS :: 0x011F
R_COLOR_WARNING :: 0x0120
R_COLOR_DANGER :: 0x0121
// Links of formatted text.
R_COLOR_LINK :: 0x0122
// Background of code in formatted text.
R_COLOR_CODE :: 0x0123

// Sizes, in logical units.
RMetric :: i32
R_METRIC_FONT_SIZE :: 0x0140
R_METRIC_CORNER_RADIUS :: 0x0141
R_METRIC_BORDER_WIDTH :: 0x0142
// Height of buttons, inputs, combo boxes, tabs, menu bars.
R_METRIC_CONTROL_HEIGHT :: 0x0143
// Horizontal / vertical padding inside controls.
R_METRIC_PADDING_X :: 0x0144
R_METRIC_PADDING_Y :: 0x0145
// Gap between an icon or a check mark and its text.
R_METRIC_SPACING :: 0x0146
R_METRIC_SCROLLBAR_SIZE :: 0x0147
// Height of list, tree and menu rows.
R_METRIC_ROW_HEIGHT :: 0x0148
// Indentation per tree level.
R_METRIC_INDENT :: 0x0149
R_METRIC_SPLITTER_SIZE :: 0x014A
R_METRIC_CHECK_SIZE :: 0x014B
R_METRIC_SLIDER_HANDLE_SIZE :: 0x014C
// Corner radius of menus, drop-down lists and tooltips.
R_METRIC_POPUP_RADIUS :: 0x014D
// The glass of the theme: each one multiplies what the material gives,
// 1 = unchanged. Frost (how much is blurred), refraction (how far the rim
// bends what is behind), saturation (of what is seen through), rim (the
// light it catches) and shadow (the one it drops).
R_METRIC_GLASS_FROST :: 0x014E
R_METRIC_GLASS_REFRACTION :: 0x014F
R_METRIC_GLASS_SATURATION :: 0x0150
R_METRIC_GLASS_RIM :: 0x0151
R_METRIC_GLASS_SHADOW :: 0x0152
// Radius of the blur of a BackgroundBlur that was not given one.
R_METRIC_BLUR_RADIUS :: 0x0153

// The styles of a font (Rfont_get_variant).
RFontStyle :: i32
R_FONT_REGULAR :: 0x0420
R_FONT_BOLD :: 0x0421
R_FONT_ITALIC :: 0x0422
R_FONT_BOLD_ITALIC :: 0x0423

RFilterType :: i32
R_FILTER_LINEAR :: 0x02C0
// No smoothing: keeps pixel art sharp.
R_FILTER_NEAREST :: 0x02C1
// Smooth, with mipmaps: an image shown much smaller than its size stays
// clean instead of shimmering, for a third more GPU memory. The default
// of the textures made from image files (Rtexture_create...). Backends
// without mipmaps treat it as R_FILTER_LINEAR.
R_FILTER_MIPMAP :: 0x02C2

RWidgetType :: i32
// Stacks children top to bottom. Slate: SVerticalBox.
R_WIDGET_VBOX :: 0x0200
// Stacks children left to right. Slate: SHorizontalBox.
R_WIDGET_HBOX :: 0x0201
// Rows and columns with spans and fill coefficients. Slate: SGridPanel.
R_WIDGET_GRID :: 0x0202
// Grid whose cells all have the same size. Slate: SUniformGridPanel.
R_WIDGET_UNIFORM_GRID :: 0x0203
// Draws children on top of each other. Slate: SOverlay.
R_WIDGET_OVERLAY :: 0x0204
// One child with a background, an outline and inner padding. Slate: SBorder.
R_WIDGET_BORDER :: 0x0205
// One child with size constraints or a fixed aspect ratio. Slate: SBox.
R_WIDGET_SIZE_BOX :: 0x0206
// Empty space. Slate: SSpacer.
R_WIDGET_SPACER :: 0x0207
// Scrollable stack of children. Slate: SScrollBox.
R_WIDGET_SCROLL_BOX :: 0x0208
// Children separated by draggable handles. Slate: SSplitter.
R_WIDGET_SPLITTER :: 0x0209
// Flows children and wraps to the next line. Slate: SWrapBox.
R_WIDGET_WRAP_BOX :: 0x020A
// Children placed with anchors and offsets. Slate: SConstraintCanvas.
R_WIDGET_CANVAS :: 0x020B
// Shows one child at a time. Slate: SWidgetSwitcher.
R_WIDGET_SWITCHER :: 0x020C
// Tab bar + one page per child.
R_WIDGET_TAB_VIEW :: 0x020D
// Collapsible header + one child. Slate: SExpandableArea.
R_WIDGET_EXPANDER :: 0x020E
R_WIDGET_LABEL :: 0x020F
R_WIDGET_BUTTON :: 0x0210
R_WIDGET_CHECKBOX :: 0x0211
R_WIDGET_SLIDER :: 0x0212
R_WIDGET_PROGRESSBAR :: 0x0213
R_WIDGET_IMAGE :: 0x0214
R_WIDGET_TEXT_INPUT :: 0x0215
R_WIDGET_SPIN_BOX :: 0x0216
R_WIDGET_COMBO_BOX :: 0x0217
R_WIDGET_SEPARATOR :: 0x0218
R_WIDGET_LIST_VIEW :: 0x0219
R_WIDGET_TREE_VIEW :: 0x021A
R_WIDGET_MENU_BAR :: 0x021B
R_WIDGET_MENU :: 0x021C
R_WIDGET_MENU_ITEM :: 0x021D
// Area showing content rendered by the application.
R_WIDGET_VIEWPORT :: 0x021E
// Widget painted and measured by application callbacks.
R_WIDGET_CUSTOM :: 0x021F
// Multi-line text editor.
R_WIDGET_TEXT_EDITOR :: 0x0220
// Title bar of a window with a custom header, holding any widget.
R_WIDGET_HEADER_BAR :: 0x0221
// Panels docked side by side or behind tabs, that the user rearranges.
R_WIDGET_DOCK_AREA :: 0x0222
// A capsule that unfolds into a panel, at the top of a window.
R_WIDGET_ISLAND :: 0x0223
// Explorer of the files of the computer.
R_WIDGET_FILE_BROWSER :: 0x0224
// One child over a blurred view of what is behind it.
R_WIDGET_BACKGROUND_BLUR :: 0x0225
// Chooses a color: hue, saturation, value, opacity, by hand or by numbers.
R_WIDGET_COLOR_PICKER :: 0x0226
// Tracks of clips and keys along a time ruler.
R_WIDGET_TIMELINE :: 0x0227
// Nodes linked by wires, on a canvas that pans and zooms.
R_WIDGET_NODE_GRAPH :: 0x0228
// Grid of tiles (an icon over a name), virtualized. Slate: STileView.
R_WIDGET_TILE_VIEW :: 0x0229
// Blocks that snap into each other, in the style of Scratch.
R_WIDGET_BLOCK_EDITOR :: 0x022A
// Rows of cells under columns that can be resized and sorted, virtualized.
R_WIDGET_TABLE :: 0x022B
// A date, chosen in a calendar that drops down.
R_WIDGET_DATE_PICKER :: 0x022C
// A time of the day: hours, minutes, optionally seconds.
R_WIDGET_TIME_PICKER :: 0x022D
// A short message that rises for a few seconds, with an optional action ("Undo").
R_WIDGET_TOAST :: 0x022E
// The path to the current place: links separated by chevrons.
R_WIDGET_BREADCRUMB :: 0x022F
// A score shown and chosen with stars.
R_WIDGET_RATING :: 0x0230
// Page numbers with previous and next.
R_WIDGET_PAGINATION :: 0x0231
// Curves of X / Y data: legend, line styles, markers, log axes, a second X and Y axis, pan and zoom.
R_WIDGET_PLOT :: 0x0232
// The pages of a PDF document, scrolled and zoomed (drawn by PDFium, loaded at run time).
R_WIDGET_PDF_VIEW :: 0x0233

RHAlign :: i32
R_HALIGN_FILL :: 0x0240
R_HALIGN_LEFT :: 0x0241
R_HALIGN_CENTER :: 0x0242
R_HALIGN_RIGHT :: 0x0243

RVAlign :: i32
R_VALIGN_FILL :: 0x0250
R_VALIGN_TOP :: 0x0251
R_VALIGN_CENTER :: 0x0252
R_VALIGN_BOTTOM :: 0x0253

RVisibility :: i32
// Drawn and laid out.
R_VISIBLE :: 0x0260
// Not drawn, but still takes its space in the layout.
R_HIDDEN :: 0x0261
// Not drawn and takes no space.
R_COLLAPSED :: 0x0262

// How a widget arrives. A transition animates what the widget and its
// children draw; the layout is not affected, the widget is in place from the
// first frame for its neighbours and for the mouse.
RTransition :: i32
R_TRANSITION_NONE :: 0x0300
// Becomes opaque.
R_TRANSITION_FADE :: 0x0301
// Grows from a smaller size and bounces a little past its own.
R_TRANSITION_POP :: 0x0302
// Shrinks from a larger size.
R_TRANSITION_ZOOM :: 0x0303
// Flies in from below / above / the right / the left, and settles with a bounce.
R_TRANSITION_FLY_UP :: 0x0304
R_TRANSITION_FLY_DOWN :: 0x0305
R_TRANSITION_FLY_LEFT :: 0x0306
R_TRANSITION_FLY_RIGHT :: 0x0307
// Falls from above, squashes on landing and wobbles.
R_TRANSITION_DROP :: 0x0308
// Short slide from the right / from the left, without bounce: pages that replace one another.
R_TRANSITION_SLIDE_LEFT :: 0x0309
R_TRANSITION_SLIDE_RIGHT :: 0x030A

ROrientation :: i32
R_HORIZONTAL :: 0x0270
R_VERTICAL :: 0x0271

// The looks of a R_WIDGET_BACKGROUND_BLUR. The first three are drawn by
// Raven from what the window shows behind the widget; the last one is the
// blur of the OS, which sees what is behind the window itself.
RBlurStyle :: i32
// What is behind, out of focus, and nothing else (a Gaussian blur).
R_BLUR_PLAIN :: 0x0440
// Frosted: blurred, its colors a little stronger, under a veil of the
// panel color of the theme, a fine grain and a hairline. The default.
R_BLUR_FROSTED :: 0x0441
// Glass: a slab that bends what is behind at its rim and catches the
// light, as the glass materials do (RMaterial); the radius says how
// frosted it is.
R_BLUR_GLASS :: 0x0442
// Native: the widget opens the window there, and what shows is what the
// OS puts behind the window (Rwindow_set_backdrop: the blurred desktop,
// acrylic, mica), under a light veil. It needs a window created with
// R_WINDOW_TRANSPARENT that the OS could make see-through; anywhere
// else it is drawn as R_BLUR_FROSTED. The radius is the one of the OS.
R_BLUR_NATIVE :: 0x0443

RDirection :: i32
// A widget: the direction of its parent (the default).
R_DIRECTION_INHERIT :: 0
R_DIRECTION_LTR :: 0x0720
R_DIRECTION_RTL :: 0x0721
// The windows: the direction of the language.
R_DIRECTION_AUTO :: 0x0722

RTextAlign :: i32
R_TEXT_ALIGN_LEFT :: 0x0280
R_TEXT_ALIGN_CENTER :: 0x0281
R_TEXT_ALIGN_RIGHT :: 0x0282

RToastKind :: i32
// A neutral message (the accent of the theme).
R_TOAST_INFO :: 0x04E0
R_TOAST_SUCCESS :: 0x04E1
R_TOAST_WARNING :: 0x04E2
R_TOAST_ERROR :: 0x04E3

// What a Toast does with a message that comes while another one is shown.
RToastQueue :: i32
// The new message takes the place of the one shown (the default).
R_TOAST_REPLACE :: 0x0500
// The messages are shown one after the other, each for its own time.
R_TOAST_SEQUENCE :: 0x0501
// Up to Rtoast_set_max_visible messages are shown at once, one above the other; the others wait.
R_TOAST_STACK :: 0x0502

RButtonStyle :: i32
// Filled with R_COLOR_CONTROL.
R_BUTTON_DEFAULT :: 0x0290
// Filled with R_COLOR_ACCENT.
R_BUTTON_PRIMARY :: 0x0291
// No background until hovered (toolbars).
R_BUTTON_FLAT :: 0x0292

RCheckboxStyle :: i32
R_CHECKBOX_CHECK :: 0x02A0
// Round mark. Checking one unchecks the other radio boxes of the same parent.
R_CHECKBOX_RADIO :: 0x02A1
// On / off switch.
R_CHECKBOX_SWITCH :: 0x02A2

RStretch :: i32
// Stretches the image to the widget, ignoring its aspect ratio.
R_STRETCH_FILL :: 0x02B0
// Largest size that fits entirely, keeping the aspect ratio.
R_STRETCH_FIT :: 0x02B1
// Smallest size that covers the widget, keeping the aspect ratio (cropped).
R_STRETCH_COVER :: 0x02B2
// Original size, centered.
R_STRETCH_NONE :: 0x02B3

// The axes of a Plot.
RPlotAxis :: i32
// The horizontal axis under the data.
R_PLOT_X :: 0x0520
// The vertical axis on the left.
R_PLOT_Y :: 0x0521
// A second horizontal axis, over the data: shown when a series or a drag line uses it.
R_PLOT_X2 :: 0x0522
// A second vertical axis, on the right: shown when a series or a drag line uses it.
R_PLOT_Y2 :: 0x0523

// How a PdfView sizes its pages.
RPdfFit :: i32
// The widest page fills the width of the view (default).
R_PDF_FIT_WIDTH :: 0x05C0
// The largest page fits whole in the view.
R_PDF_FIT_PAGE :: 0x05C1
// The zoom of Rpdf_view_set_zoom; zooming with the wheel or the keys switches to it.
R_PDF_FIT_NONE :: 0x05C2

// What a PdfView holds.
RPdfStatus :: i32
// No document.
R_PDF_EMPTY :: 0x05D0
// A document is shown.
R_PDF_LOADED :: 0x05D1
// The document needs a password, or the one given is wrong: load it again with one.
R_PDF_NEEDS_PASSWORD :: 0x05D2
// The file could not be read, or is not a PDF document.
R_PDF_ERROR :: 0x05D3
// The PDFium library was not found (see Rpdf_is_available).
R_PDF_UNAVAILABLE :: 0x05D4

// The fonts of a PDF document (Rpdf_document_set_font): those of Raven, then the ones added (2, 3...).
RPdfFont :: i32
// Roboto, the font of the interface of Raven.
R_PDF_FONT_SANS :: 0
// Roboto Mono.
R_PDF_FONT_MONO :: 1

// How a shape of a PDF document is painted: a bitmask.
RPdfPaint :: i32
// Filled with the fill color.
R_PDF_FILL :: 1
// Outlined with the stroke color and the line width.
R_PDF_STROKE :: 2

// A bitmask of styles of the text of a PDF document.
RPdfTextStyle :: i32
R_PDF_TEXT_BOLD :: 1
R_PDF_TEXT_ITALIC :: 2
R_PDF_TEXT_UNDERLINE :: 4

RPdfPrintFlags :: i32
// No dialog: printed at once on the printer named, else the default one.
R_PDF_PRINT_NO_DIALOG :: 1

// How an axis of a Plot spreads its values.
RPlotScale :: i32
R_PLOT_SCALE_LINEAR :: 0x0530
// Powers of ten at even distances; only values above zero are shown.
R_PLOT_SCALE_LOG :: 0x0531
// Symmetric logarithm: linear between -t and t (Rplot_set_axis_linear_threshold,
// 1 by default), logarithmic beyond on both sides: shows zero and the
// negative values a logarithm cannot.
R_PLOT_SCALE_SYMLOG :: 0x0532

// How the line of a series of a Plot is drawn.
RPlotLine :: i32
R_PLOT_LINE_SOLID :: 0x0540
R_PLOT_LINE_DASHED :: 0x0541
R_PLOT_LINE_DOTTED :: 0x0542
R_PLOT_LINE_DASH_DOT :: 0x0543
// No line: only the markers (a scatter plot).
R_PLOT_LINE_NONE :: 0x0544

// The mark drawn on each point of a series of a Plot.
RPlotMarker :: i32
R_PLOT_MARKER_NONE :: 0x0550
R_PLOT_MARKER_CIRCLE :: 0x0551
R_PLOT_MARKER_SQUARE :: 0x0552
R_PLOT_MARKER_DIAMOND :: 0x0553
R_PLOT_MARKER_TRIANGLE_UP :: 0x0554
R_PLOT_MARKER_TRIANGLE_DOWN :: 0x0555
R_PLOT_MARKER_CROSS :: 0x0556
R_PLOT_MARKER_PLUS :: 0x0557
R_PLOT_MARKER_ASTERISK :: 0x0558

// Where the legend of a Plot is.
RPlotLegend :: i32
R_PLOT_LEGEND_HIDDEN :: 0x0560
R_PLOT_LEGEND_TOP_LEFT :: 0x0561
R_PLOT_LEGEND_TOP :: 0x0562
R_PLOT_LEGEND_TOP_RIGHT :: 0x0563
R_PLOT_LEGEND_RIGHT :: 0x0564
R_PLOT_LEGEND_BOTTOM_RIGHT :: 0x0565
R_PLOT_LEGEND_BOTTOM :: 0x0566
R_PLOT_LEGEND_BOTTOM_LEFT :: 0x0567
R_PLOT_LEGEND_LEFT :: 0x0568
// To the right of the data, outside it.
R_PLOT_LEGEND_OUTSIDE :: 0x0569

// Shorthands of Rtext_editor_set_language for two languages.
RSyntax :: i32
R_SYNTAX_NONE :: 0x0340
R_SYNTAX_XML :: 0x0341
R_SYNTAX_C :: 0x0342

RLanguageFlags :: i32
// Tags and attributes instead of code: XML, HTML. The words and the comments of the description are not used.
R_LANGUAGE_MARKUP :: 1
// Selectors, properties and values: CSS.
R_LANGUAGE_STYLE_SHEET :: 2
// A block comment may hold another one (Rust).
R_LANGUAGE_NESTED_COMMENTS :: 4
// Between single quotes there is one character; any other single quote is not a text (C, Java, Rust).
R_LANGUAGE_CHAR_LITERALS :: 8
// A line that ends with ':' indents the next one (Python).
R_LANGUAGE_INDENT_ON_COLON :: 0x0010
// The words of the language are recognized whatever their case.
R_LANGUAGE_IGNORE_CASE :: 0x0020
// Markup that holds JavaScript in script elements and CSS in style elements (HTML).
R_LANGUAGE_EMBEDS :: 0x0040
// Formatted text written in Markdown: titles, strong and emphasized
// text, code, links, lists. The editor shows the text as it is written,
// marks included, with strong text in bold, emphasized text in italic
// and titles in bold: what will be formatted is seen while typing.
R_LANGUAGE_FORMATTED_TEXT :: 0x0080

// What a part of the text is, for the language of the editor (Rtext_editor_get_token).
RToken :: i32
R_TOKEN_TEXT :: 0x03A0
R_TOKEN_COMMENT :: 0x03A1
R_TOKEN_STRING :: 0x03A2
R_TOKEN_NUMBER :: 0x03A3
R_TOKEN_KEYWORD :: 0x03A4
// A type, or a name that is always there.
R_TOKEN_TYPE :: 0x03A5
// A name followed by an opening parenthesis.
R_TOKEN_FUNCTION :: 0x03A6
// The name of an element, in markup and in the selectors of a style sheet.
R_TOKEN_TAG :: 0x03A7
// The name of an attribute (markup), of a class or of a property (style sheets), of a key (JSON).
R_TOKEN_ATTRIBUTE :: 0x03A8
R_TOKEN_PUNCTUATION :: 0x03A9
// A line for the preprocessor, an annotation, a declaration of a document.
R_TOKEN_DIRECTIVE :: 0x03AA
// Formatted text (the Markdown language): a title, strong text, emphasized text, a link.
R_TOKEN_HEADING :: 0x03AB
R_TOKEN_STRONG :: 0x03AC
R_TOKEN_EMPHASIS :: 0x03AD
R_TOKEN_LINK :: 0x03AE

// What a marker says about the text it underlines.
RMarkerKind :: i32
R_MARKER_ERROR :: 0x0350
R_MARKER_WARNING :: 0x0351
R_MARKER_INFO :: 0x0352

// Buttons of a header bar.
RHeaderButtons :: i32
R_HEADER_NO_BUTTONS :: 0
R_HEADER_CLOSE :: 1
R_HEADER_MINIMIZE :: 2
R_HEADER_MAXIMIZE :: 4
R_HEADER_ALL_BUTTONS :: 7

// A progress bar on the icon of the application (Rwindow_set_taskbar_progress).
RTaskbarProgress :: i32
R_TASKBAR_PROGRESS_NONE :: 0x07A0
R_TASKBAR_PROGRESS_NORMAL :: 0x07A1
// Shown as paused (yellow on Windows).
R_TASKBAR_PROGRESS_PAUSED :: 0x07A2
// Shown as failed (red on Windows).
R_TASKBAR_PROGRESS_ERROR :: 0x07A3
// Something goes on, without knowing how far it is.
R_TASKBAR_PROGRESS_INDETERMINATE :: 0x07A4

// Where a panel goes in a dock area, relative to another panel or to the whole area.
RDockSide :: i32
R_DOCK_LEFT :: 0x0360
R_DOCK_RIGHT :: 0x0361
R_DOCK_TOP :: 0x0362
R_DOCK_BOTTOM :: 0x0363
// Behind the same tabs as the other panel.
R_DOCK_TAB :: 0x0364

RVisualPreset :: i32
// The look of the theme of the window: its platform, dark or light.
R_PRESET_AUTO :: 0x0510
// Cards with soft shadows, few lines (follows dark or light).
R_PRESET_MODERN :: 0x0511
// Headers in gradients, a grid of lines, thick links that glow; pins of the type "exec" are arrows.
R_PRESET_BLUEPRINT :: 0x0512
// Compact nodes, flat headers in muted colors, thin links.
R_PRESET_BLENDER :: 0x0513
// Almost black, outlines and links of light.
R_PRESET_NEON :: 0x0514

// What an item of a timeline is.
RTimelineItem :: i32
R_TIMELINE_NONE :: 0x0460
R_TIMELINE_CLIP :: 0x0461
R_TIMELINE_KEY :: 0x0462
R_TIMELINE_MARKER :: 0x0463

RBlockShape :: i32
// A step: snaps under another one, others snap under it.
R_BLOCK_STATEMENT :: 0x0700
// Starts a script ("when the game starts"): nothing goes above it.
R_BLOCK_HAT :: 0x0701
// Ends a script ("stop"): nothing goes under it.
R_BLOCK_CAP :: 0x0702
// Wraps stacks in its mouths ("repeat", "if ... else").
R_BLOCK_C :: 0x0703
// A rounded value that goes into a slot.
R_BLOCK_REPORTER :: 0x0704
// A hexagonal condition that goes into a %b slot (or a %r one).
R_BLOCK_BOOLEAN :: 0x0705

RBlockPlace :: i32
R_BLOCK_NEXT :: 0x0710
R_BLOCK_BODY :: 0x0711
R_BLOCK_INPUT :: 0x0712

// How much a notification of the system asks for attention (Rnotification_show).
RNotificationUrgency :: i32
// No sound; may only go to the list of notifications.
R_NOTIFICATION_LOW :: 0x04F0
R_NOTIFICATION_NORMAL :: 0x04F1
// Stays until the user dismisses it, where the system allows it.
R_NOTIFICATION_CRITICAL :: 0x04F2

RFileDialogFlags :: i32
// Several files can be chosen (Rfile_dialog_open): see Rfile_dialog_get_result.
R_FILE_DIALOG_MULTIPLE :: 1
// Never the dialog of the OS: the file browser of Raven, in a dialog of the parent window.
R_FILE_DIALOG_RAVEN :: 2
R_FILE_DIALOG_SHOW_HIDDEN :: 4

RFileBrowserMode :: i32
// Choose an existing file.
R_FILE_BROWSER_OPEN :: 0x0380
// Choose where to save: a name is typed; an existing file must be confirmed.
R_FILE_BROWSER_SAVE :: 0x0381
// Choose a folder.
R_FILE_BROWSER_FOLDER :: 0x0382

RMessageButtons :: i32
R_MESSAGE_OK :: 0x0390
R_MESSAGE_OK_CANCEL :: 0x0391
R_MESSAGE_YES_NO :: 0x0392
R_MESSAGE_YES_NO_CANCEL :: 0x0393

RMessageResult :: i32
// Dismissed (Escape), or the Cancel button.
R_MESSAGE_RESULT_CANCEL :: 0
R_MESSAGE_RESULT_OK :: 1
R_MESSAGE_RESULT_YES :: 2
R_MESSAGE_RESULT_NO :: 3

// How a spring moves. Animations in Raven are springs, as on iOS: a value is
// pulled toward its target and may run past it before settling, which is what
// makes a control feel alive rather than mechanical.
RMotion :: i32
// Fast, no bounce: hover, highlights.
R_MOTION_SNAPPY :: 0x0320
// Slower, no bounce: scrolling, fades, sizes.
R_MOTION_SMOOTH :: 0x0321
// Runs a little past the target and comes back: switches, popups, pressed buttons.
R_MOTION_BOUNCY :: 0x0322
// Swings several times around the target: things that fly in or get dropped.
R_MOTION_WOBBLY :: 0x0323

// The effects that come with Raven (Reffect_get).
REffect :: i32
// Soft lights that drift slowly: color, color2; params: speed, scale, intensity, -. Animated.
R_EFFECT_AURORA :: 0x0B40
// A band of light that sweeps across (loading placeholders): color = base, color2 = light; params: speed, width, angle, -. Animated.
R_EFFECT_SHIMMER :: 0x0B41
// A glowing edge whose colors run around the shape: color, color2; params: speed, width, glow, -. Animated.
R_EFFECT_NEON :: 0x0B42
// Rings that ripple what is behind, like water (reads the backdrop): color = tint; params: speed, strength, density, center x (0..1). Animated.
R_EFFECT_RIPPLE :: 0x0B43
// Bands of color that flow into each other: color, color2; params: speed, scale, contrast, -. Animated.
R_EFFECT_PLASMA :: 0x0B44
// A gradient with the fine grain of film: color (top left) to color2 (bottom right); params: grain, angle, -, -. Animated grain.
R_EFFECT_GRAIN :: 0x0B45
// A soft light that follows a point: color = light, color2 = background; params: x, y (0..1), radius (0..1), -.
R_EFFECT_SPOTLIGHT :: 0x0B46
// Four colors blended across the surface, slowly moving: color, color2 and their mix; params: speed, -, -, -. Animated.
R_EFFECT_MESH :: 0x0B47

// Flags of Rshader_create.
RShaderFlags :: i32
// The effect reads what is drawn behind it (raven_backdrop, raven_backdrop_blur).
R_SHADER_BACKDROP :: 1
// The effect changes with time: the window keeps drawing frames while it is shown.
R_SHADER_ANIMATED :: 2

// The kind of value an attribute takes.
RXmlValueType :: i32
R_XML_TEXT :: 0x03C0
// "true" or "false".
R_XML_BOOL :: 0x03C1
R_XML_NUMBER :: 0x03C2
// One of the words of `choices`.
R_XML_CHOICE :: 0x03C3
// "#RGB", "#RRGGBB", "#RRGGBBAA", or a name: white, black, red, transparent...
R_XML_COLOR :: 0x03C4
// The name of an action (Raction_bind).
R_XML_ACTION :: 0x03C5
// The path of a file.
R_XML_FILE :: 0x03C6
// Several numbers separated by commas: "8,4", "0,0,1,1".
R_XML_NUMBERS :: 0x03C7
// Texts separated by '|': "One|Two|Three".
R_XML_LIST :: 0x03C8

RColorFilter :: i32
R_COLOR_FILTER_NONE :: 0x0740
// Corrections: the colors that cannot be told apart are moved to colors that can.
R_COLOR_FILTER_PROTANOPIA :: 0x0741
R_COLOR_FILTER_DEUTERANOPIA :: 0x0742
R_COLOR_FILTER_TRITANOPIA :: 0x0743
R_COLOR_FILTER_GRAYSCALE :: 0x0744
R_COLOR_FILTER_INVERT :: 0x0745
// Simulations, to see an interface as the color blind see it.
R_COLOR_FILTER_SIMULATE_PROTANOPIA :: 0x0746
R_COLOR_FILTER_SIMULATE_DEUTERANOPIA :: 0x0747
R_COLOR_FILTER_SIMULATE_TRITANOPIA :: 0x0748
R_COLOR_FILTER_SIMULATE_ACHROMATOPSIA :: 0x0749

RAccessibleRole :: i32
// The role of the type of the widget (the default).
R_ROLE_AUTO :: 0
// Not exposed: its children are seen as the children of its parent.
R_ROLE_NONE :: 0x0760
R_ROLE_WINDOW :: 0x0761
R_ROLE_GROUP :: 0x0762
R_ROLE_TEXT :: 0x0763
R_ROLE_HEADING :: 0x0764
R_ROLE_LINK :: 0x0765
R_ROLE_BUTTON :: 0x0766
R_ROLE_TOGGLE_BUTTON :: 0x0767
R_ROLE_CHECKBOX :: 0x0768
R_ROLE_RADIO_BUTTON :: 0x0769
R_ROLE_SLIDER :: 0x076A
R_ROLE_PROGRESS_BAR :: 0x076B
R_ROLE_SPIN_BUTTON :: 0x076C
R_ROLE_TEXT_FIELD :: 0x076D
R_ROLE_DOCUMENT :: 0x076E
R_ROLE_COMBO_BOX :: 0x076F
R_ROLE_LIST :: 0x0770
R_ROLE_LIST_ITEM :: 0x0771
R_ROLE_TREE :: 0x0772
R_ROLE_TREE_ITEM :: 0x0773
R_ROLE_TABLE :: 0x0774
R_ROLE_MENU_BAR :: 0x0775
R_ROLE_MENU :: 0x0776
R_ROLE_MENU_ITEM :: 0x0777
R_ROLE_TAB_LIST :: 0x0778
R_ROLE_TAB :: 0x0779
R_ROLE_TAB_PANEL :: 0x077A
R_ROLE_TOOLBAR :: 0x077B
R_ROLE_STATUS_BAR :: 0x077C
R_ROLE_DIALOG :: 0x077D
R_ROLE_ALERT :: 0x077E
R_ROLE_IMAGE :: 0x077F
R_ROLE_CANVAS :: 0x0780
R_ROLE_SEPARATOR :: 0x0781
R_ROLE_SCROLL_BAR :: 0x0782
R_ROLE_PANE :: 0x0783

// Registers a function called once per frame, before the layout.
// The body of your loop does the same job; the callback is also called
// while the OS keeps the thread in a loop of its own (on Windows, while
// the user moves or resizes a window), when your loop is not running.
RFrameCallback :: #type proc "c" (delta_seconds: f64, user_data: rawptr)
RTaskCallback :: #type proc "c" (user_data: rawptr)
RParallelRangeCallback :: #type proc "c" (begin: u32, end: u32, user_data: rawptr)
RTimerCallback :: #type proc "c" (timer: Rid, user_data: rawptr)
// Actions connect widgets to application code by name. A widget carries an
// action name (set from XML with OnClicked="file.save", or from C with
// Rwidget_set_action); when its main event fires, the function bound to that
// name is called. Names are resolved when the event fires, so actions can be
// bound before or after the widgets are created.
RActionCallback :: #type proc "c" (sender: Rid, action: cstring, user_data: rawptr)
// Registers a function called whenever an error is raised.
// The function may be registered before Rinit. It is called on the
// thread that raised the error: errors of the renderer (and the messages
// of the Vulkan validation layers) come from the render thread.
RErrorCallback :: #type proc "c" (code: RError, severity: RSeverity, detail: cstring)
// Registers a function called after Raven recovered from the loss
// of the GPU device (a driver reset or update, a GPU removed, a long sleep).
//
// Raven then starts the backend again and remakes what it can by itself:
// the surfaces of the windows, the fonts, the shaders, the textures made
// from image files and the small textures (up to 256 x 256). The pixels of
// the larger textures made from pixels are lost: the textures keep their
// handle and size but are transparent until Rtexture_update fills them
// again, which is what this function is for. Textures that wrap objects of
// the backend (raven_vulkan.h) must be made again, and the Vulkan handles
// returned by Rvulkan_get_device... have changed.
//
// Called on the UI thread, during Rdraw_frame / Rwait_frame. Set it after
// Rinit; NULL removes it.
RDeviceResetCallback :: #type proc "c" (user_data: rawptr)
// The on-screen keyboard appeared, changed height or went away: `height` in logical units, 0 when hidden.
RKeyboardCallback :: #type proc "c" (window: Rid, visible: i32, height: f32, user_data: rawptr)
// Called for a drop target (a widget, or a window: `target` is its
// handle). Return R_TRUE to accept: while the pointer is over the target this
// shows the "can drop here" cursor and highlights the target; for
// R_DROP_DROPPED it tells the source that the data was taken.
RDropCallback :: #type proc "c" (target: Rid, event: ^RDropEvent, user_data: rawptr) -> i32
// Called when the user asks to close the window.
// Return R_TRUE to let the window close, R_FALSE to keep it open.
RWindowCloseCallback :: #type proc "c" (window: Rid, user_data: rawptr) -> i32
// Called after the window was resized. Sizes are in logical units.
RWindowResizeCallback :: #type proc "c" (window: Rid, width: i32, height: i32, user_data: rawptr)
// Sees every input event of the window (mouse, wheel, keys, text)
// before any widget, with `target` set to the widget under the pointer.
// Returning R_TRUE consumes the event: the widgets do not get it. The
// state read by Rwindow_is_key_down is updated either way.
RWindowEventHook :: #type proc "c" (window: Rid, event: ^REvent, user_data: rawptr) -> i32
// Return R_TRUE to take the finger: the library then neither clicks nor scrolls with it.
RTouchCallback :: #type proc "c" (widget: Rid, event: ^RTouchEvent, user_data: rawptr) -> i32
// Return R_TRUE (on R_PEN_DOWN) to take the stroke: the pen does not act as a mouse until it is lifted.
RPenCallback :: #type proc "c" (widget: Rid, event: ^RPenEvent, user_data: rawptr) -> i32
// Return R_TRUE (on R_GESTURE_BEGIN) to take the gesture: nothing zooms, pans or navigates by itself.
RGestureCallback :: #type proc "c" (widget: Rid, event: ^RGestureEvent, user_data: rawptr) -> i32
// Called once when the widget is gone: deleted by the application,
// with its parent or its window, or by Rshutdown. The place to release
// what belonged to it (a texture, a connection). The handle it receives
// names nothing any more: it is only there to tell widgets apart.
// Called on the UI thread, shortly after the deletion.
RWidgetDestroyCallback :: #type proc "c" (widget: Rid, user_data: rawptr)
// Receives the input events of a widget before the widget itself.
// Return R_TRUE to consume the event (the widget and its parents will
// not see it), R_FALSE to let it continue.
RWidgetEventCallback :: #type proc "c" (widget: Rid, event: ^REvent, user_data: rawptr) -> i32
RTabChangedCallback :: #type proc "c" (tab_view: Rid, index: i32, user_data: rawptr)
// The user closes a tab (its button, a middle click). Return R_TRUE
// to delete its page now, R_FALSE to keep it (the application may ask to
// save, then delete the page itself with Rwidget_destroy).
RTabCloseCallback :: #type proc "c" (tab_view: Rid, index: i32, content: Rid, user_data: rawptr) -> i32
RExpanderChangedCallback :: #type proc "c" (expander: Rid, expanded: i32, user_data: rawptr)
// Called when the user clicks a link of a Markdown label.
//
// Without a callback, a link written [Save](action:file.save) fires the
// action "file.save" (Raction_bind), and http, https and mailto links
// open in the browser or the mail program of the user (Ropen_url).
RLabelLinkCallback :: #type proc "c" (label: Rid, url: cstring, user_data: rawptr)
RButtonClickedCallback :: #type proc "c" (button: Rid, user_data: rawptr)
RCheckboxChangedCallback :: #type proc "c" (checkbox: Rid, checked: i32, user_data: rawptr)
RSliderChangedCallback :: #type proc "c" (slider: Rid, value: f32, user_data: rawptr)
RTextChangedCallback :: #type proc "c" (text_input: Rid, text: cstring, user_data: rawptr)
RSpinBoxChangedCallback :: #type proc "c" (spin_box: Rid, value: f64, user_data: rawptr)
RComboBoxChangedCallback :: #type proc "c" (combo_box: Rid, index: i32, user_data: rawptr)
// Virtual mode, for large or computed data: the list stores nothing and
// asks for the text of the rows it draws. The returned string must stay
// valid until the next call.
RListItemTextCallback :: #type proc "c" (list_view: Rid, index: i32, user_data: rawptr) -> cstring
// Colors the text of rows (a level, a verdict): the list asks for the color
// of each row it draws, stored or virtual. Set r, g, b, a (0..1) and return
// R_TRUE; R_FALSE (or alpha 0) keeps the color of the theme. The color is
// kept on a selected or hovered row. NULL removes the callback.
RListItemColorCallback :: #type proc "c" (list_view: Rid, index: i32, r: ^f32, g: ^f32, b: ^f32, a: ^f32, user_data: rawptr) -> i32
RListEventCallback :: #type proc "c" (list_view: Rid, index: i32, user_data: rawptr)
RTreeEventCallback :: #type proc "c" (tree_view: Rid, item: Rid, user_data: rawptr)
// Right click: `item` is the item under the pointer (selected
// first), or R_INVALID_ID in the empty space; x, y in the window, for Rmenu_open.
RTreeContextCallback :: #type proc "c" (tree_view: Rid, item: Rid, x: f32, y: f32, user_data: rawptr)
RMenuItemClickedCallback :: #type proc "c" (item: Rid, user_data: rawptr)
// Fills `item` for tile `index`. Called on the UI thread, only for the tiles in view.
RTileItemCallback :: #type proc "c" (tile_view: Rid, index: i32, item: ^RTileItem, user_data: rawptr)
// Selection change (`index`: the current tile, or -1) and activation (double click, Enter).
RTileEventCallback :: #type proc "c" (tile_view: Rid, index: i32, user_data: rawptr)
// Right click: the tile under the pointer (selected first), or -1 in the empty space; x, y in the window.
RTileContextCallback :: #type proc "c" (tile_view: Rid, index: i32, x: f32, y: f32, user_data: rawptr)
// The data of a drag, given by the application (paths of files...)
// instead of the indices: the dragged tiles are the selected ones
// (Rtile_view_get_selection). The text returned is copied; NULL keeps
// the indices.
RTileDragDataCallback :: #type proc "c" (tile_view: Rid, user_data: rawptr) -> cstring
// Virtual mode, for large or computed data: the table stores nothing and
// asks for the texts of the cells it draws. The returned string must stay
// valid until the next call.
RTableCellTextCallback :: #type proc "c" (table: Rid, row: i32, column: i32, user_data: rawptr) -> cstring
// Colors the text of cells (a verdict, a state): the table asks for the
// color of each cell it draws, stored or virtual (`row` is the index of the
// order shown). Set r, g, b, a (0..1) and return R_TRUE; R_FALSE (or alpha 0)
// keeps the color of the theme. The color is kept on a selected or hovered
// row. NULL removes the callback.
RTableCellColorCallback :: #type proc "c" (table: Rid, row: i32, column: i32, r: ^f32, g: ^f32, b: ^f32, a: ^f32, user_data: rawptr) -> i32
RTableEventCallback :: #type proc "c" (table: Rid, row: i32, user_data: rawptr)
// Called when the user clicks a header; `column` -1: not sorted.
RTableSortCallback :: #type proc "c" (table: Rid, column: i32, descending: i32, user_data: rawptr)
// Called when the user chooses a date. This also fires the action of the widget.
RDateChangedCallback :: #type proc "c" (date_picker: Rid, year: i32, month: i32, day: i32, user_data: rawptr)
// Called when the user changes the time. This also fires the action of the widget.
RTimeChangedCallback :: #type proc "c" (time_picker: Rid, hour: i32, minute: i32, second: i32, user_data: rawptr)
// Called by the button of a message, which then hides. This also fires the action of the widget.
RToastCallback :: #type proc "c" (toast: Rid, user_data: rawptr)
// Called when the user clicks an item. This also fires the action of the widget.
RBreadcrumbCallback :: #type proc "c" (breadcrumb: Rid, index: i32, user_data: rawptr)
// Called when the user changes the score. This also fires the action of the widget.
RRatingChangedCallback :: #type proc "c" (rating: Rid, value: f32, user_data: rawptr)
// Called when the user goes to another page. This also fires the action of the widget.
RPageChangedCallback :: #type proc "c" (pagination: Rid, page: i32, user_data: rawptr)
// Called when the user moves, zooms or fits the view (Rplot_get_axis_range tells where it is).
RPlotViewCallback :: #type proc "c" (plot: Rid, user_data: rawptr)
// Called while the user drags a drag line. This also fires the action of the widget.
RPlotDragLineCallback :: #type proc "c" (plot: Rid, line: i32, value: f64, user_data: rawptr)
// Called when the user hides or shows a series from the legend.
RPlotSeriesCallback :: #type proc "c" (plot: Rid, series: i32, visible: i32, user_data: rawptr)
// Called when the current page changes as the user scrolls. This also fires the action of the widget.
RPdfPageChangedCallback :: #type proc "c" (view: Rid, page: i32, user_data: rawptr)
// Called when the user zoomed (to zoom other editors the same, or to keep it for the next time).
RTextEditorZoomCallback :: #type proc "c" (editor: Rid, zoom: f32, user_data: rawptr)
// Called each time the editor gathers its proposals, with the
// start of the word being typed: add your own with
// Rtext_editor_add_completion (the fields of a structure, the answers of
// a language server...). They are listed before the others.
RCompletionCallback :: #type proc "c" (editor: Rid, prefix: cstring, user_data: rawptr)
// Called while the pointer moves over the text where no marker
// is, with the position under it: the text returned (NULL or "" for
// none) is shown as the tooltip, as a marker's message is (what a name
// stands for, the value of a variable). The text must stay valid until
// the next call.
RTextEditorHoverCallback :: #type proc "c" (editor: Rid, line: i32, column: i32, user_data: rawptr) -> cstring
// Called after each change made by the user (typing, pasting,
// undoing...) or by Rtext_editor_insert_text. The action of the widget
// (OnChanged in XML) fires at the same time.
RTextEditorChangedCallback :: #type proc "c" (editor: Rid, user_data: rawptr)
// Called when the tray icon is clicked (Rtray_set_clicked_callback).
RTrayClickedCallback :: #type proc "c" (tray: Rid, button: RMouseButton, user_data: rawptr)
// Called when the user closes a panel, before it is deleted.
// Return R_FALSE to keep the panel.
RDockCloseCallback :: #type proc "c" (area: Rid, panel: Rid, user_data: rawptr) -> i32
// Called after the user moved, resized, showed or closed a panel.
RDockChangedCallback :: #type proc "c" (area: Rid, user_data: rawptr)
// Called when the user unfolds or folds the island. The action of the widget (OnChanged) fires too.
RIslandChangedCallback :: #type proc "c" (island: Rid, expanded: i32, user_data: rawptr)
// Called each time the user changes the color: while dragging,
// and when a code or a number is entered. The action of the widget
// (OnChanged in XML) fires too.
RColorChangedCallback :: #type proc "c" (picker: Rid, r: f32, g: f32, b: f32, a: f32, user_data: rawptr)
// The playhead moved: scrubbed by the user, or running by itself.
RTimelineTimeCallback :: #type proc "c" (timeline: Rid, seconds: f64, user_data: rawptr)
// Something happened to an item; which thing depends on the function the callback was given to.
RTimelineItemCallback :: #type proc "c" (timeline: Rid, item: i32, user_data: rawptr)
// Asked when the user is about to link two pins that may be
// linked: return R_FALSE to refuse (a loop, a rule of the application).
RNodeGraphConnectCallback :: #type proc "c" (graph: Rid, output_pin: i32, input_pin: i32, user_data: rawptr) -> i32
// Something happened to a link or to a node; which thing depends on the function the callback was given to.
RNodeGraphItemCallback :: #type proc "c" (graph: Rid, item: i32, user_data: rawptr)
// Right click: `node` is the node under the pointer (selected first),
// or 0 on the empty canvas; x, y are in the widget (Rnode_graph_to_canvas).
// Without this callback the right button does nothing.
RNodeGraphContextCallback :: #type proc "c" (graph: Rid, node: i32, x: f32, y: f32, user_data: rawptr)
// Something happened at a point of the canvas: `item` depends on the
// function the callback was given to; x, y are in canvas units (as node positions).
RNodeGraphPointCallback :: #type proc "c" (graph: Rid, item: i32, x: f32, y: f32, user_data: rawptr)
// A block changed (0: several, or one was deleted), or a script was clicked (its top block).
RBlockEditorCallback :: #type proc "c" (editor: Rid, block: i32, user_data: rawptr)
// Right click: on a block (0 on the canvas), at x, y in the widget.
RBlockEditorContextCallback :: #type proc "c" (editor: Rid, block: i32, x: f32, y: f32, user_data: rawptr)
// Called when a dialog closes. `result` is the value given to Rdialog_close, 0 when dismissed.
RDialogClosedCallback :: #type proc "c" (content: Rid, result: i32, user_data: rawptr)
// Called when the user chooses: a double click or Enter on a file,
// the Open / Save button. `path` is the first chosen path; with several
// files, read them with Rfile_browser_get_selection. The action of the
// widget (OnActivated in XML) fires at the same time.
RFileBrowserCallback :: #type proc "c" (browser: Rid, path: cstring, user_data: rawptr)
RNotificationFallback :: #type proc "c" (title: cstring, body: cstring, urgency: RNotificationUrgency, user_data: rawptr)
RNotificationClickedCallback :: #type proc "c" (user_data: rawptr)
// Called on the render thread each time the window is rendered.
// State you bind (pipeline, descriptor sets, viewport, scissor) does not
// need to be restored. NULL removes the callback.
RViewportDrawCallback :: #type proc "c" (info: ^RViewportDrawInfo, user_data: rawptr)
// Called on the UI thread when the pixel size changes: resize your render target here.
RViewportResizeCallback :: #type proc "c" (viewport: Rid, width: i32, height: i32, user_data: rawptr)
RPaintCallback :: #type proc "c" (widget: Rid, painter: ^RPainter, width: f32, height: f32, user_data: rawptr)
RMeasureCallback :: #type proc "c" (widget: Rid, width: ^f32, height: ^f32, user_data: rawptr)
// Called each time a file was loaded again.
// path The file given when the interface was created.
// ok   R_FALSE when the file could not be used (the interface is unchanged).
//
// Widgets that had to be built again have new handles: this is the
// place to look them up again (Rwidget_find).
RXmlReloadCallback :: #type proc "c" (path: cstring, ok: i32, user_data: rawptr)
// Tags made by code. A library or an application that builds a widget of
// its own (a R_WIDGET_CUSTOM with its callbacks, or an assembly of
// widgets) can give it a tag: documents then write <MyWidget .../> as
// they write <Button/>.
//
// `create` makes the widget and returns it. `set_attribute` (may be
// NULL) receives the attributes of the element before Raven looks at
// them: it returns R_TRUE for those it understands, R_FALSE to leave
// them to the widget (Name, Width, Slot..., and what its type knows).
// Rwidget_set_attribute goes through it too.
RXmlTagCreateCallback :: #type proc "c" (tag: cstring, user_data: rawptr) -> Rid
RXmlTagAttributeCallback :: #type proc "c" (widget: Rid, name: cstring, value: cstring, user_data: rawptr) -> i32
RLocaleChangedCallback :: #type proc "c" (language: cstring, user_data: rawptr)
RAccessiblePressCallback :: #type proc "c" (widget: Rid, user_data: rawptr)

// Optional settings for Rinit_ex. A zero-initialized structure selects the
// default for every field, so new fields can be added without breaking code.
RInitDesc :: struct {
	// Rendering backend. 0 selects WebGL in browser builds, Vulkan otherwise.
	backend: RBackend,
	// Application name reported to the graphics driver. May be NULL.
	app_name: cstring,
	// Worker threads of the job system. 0 = one per spare CPU core, negative = none.
	worker_threads: i32,
	// R_TRUE renders on the UI thread instead of a dedicated render thread.
	disable_render_thread: i32,
	// R_TRUE presents without waiting for the vertical blank.
	disable_vsync: i32,
	// R_TRUE enables the backend debug layer (Vulkan validation) when installed.
	enable_validation: i32,
	// R_TRUE opens no OS window (null platform). Combine with R_BACKEND_NULL for tests.
	headless: i32,
	// Global UI scale multiplier applied on top of the monitor scale. 0 = 1.0.
	ui_scale: f32,
}

// Memory functions of the application, to replace the allocator of Raven. See Rset_allocator.
RAllocator :: struct {
	alloc: rawptr,
	realloc: rawptr,
	free: rawptr,
	user_data: rawptr,
}

// What the library holds. See Rmemory_get_stats.
RMemoryStats :: struct {
	// Bytes in the blocks currently allocated. 0 with the allocator of the application.
	used_bytes: uint,
	// Highest value of used_bytes since the start.
	peak_bytes: uint,
	// Bytes taken from the OS: used_bytes, plus what is free in the heap.
	reserved_bytes: uint,
	// Blocks currently allocated. 0 after Rshutdown: anything else is a leak of the library.
	allocation_count: uint,
	// Allocations made since the start.
	total_allocations: u64,
}

// Timings and counters of the last frame produced for a window.
RFrameStats :: struct {
	layout_ms: f64,
	paint_ms: f64,
	render_ms: f64,
	widget_count: u32,
	instance_count: u32,
	draw_call_count: u32,
	// Glass: times the frame was copied and blurred to be seen through (0 without glass).
	backdrop_count: u32,
	paint_job_count: u32,
	frame_index: u64,
}

// Distances from the edges of a window, in logical units.
RInsets :: struct {
	top: f32,
	right: f32,
	bottom: f32,
	left: f32,
}

// What the platform needs to know of the focused text input to set up the on-screen keyboard.
RTextInputInfo :: struct {
	type: RTextInputType,
	return_key: RReturnKey,
	// R_TRUE: the text is hidden (no suggestions, no autocorrection, no learning).
	password: i32,
	read_only: i32,
	// Most characters accepted; 0 for no limit.
	max_length: i32,
}

// Native handles of a window or of a native viewport.
//   Win32 : display = HINSTANCE, window = HWND
//   X11   : display = Display*,  window = Window (XID), visual = VisualID
//   Wayland: display = wl_display*, window = wl_surface* (Linux and BSD choose
//           Wayland or X11 when Rinit runs: check `system`)
//   Cocoa : display = NSWindow*, window = CAMetalLayer* for Vulkan presentation
//   Emscripten: window = opaque browser surface token (no OS window handle)
//   Android: window = ANativeWindow*
//   UIKit : display = UIWindow*, window = CAMetalLayer* for Vulkan presentation (iOS, iPadOS)
RNativeWindow :: struct {
	system: RWindowSystem,
	display: rawptr,
	window: uintptr,
	visual: uintptr,
	// R_TRUE when the window was created with R_WINDOW_TRANSPARENT and the
	// window system gave it an alpha channel: what is presented in it must
	// carry alpha (premultiplied), which a rendering backend asks for when
	// it creates its surface.
	transparent: i32,
}

// Input event delivered to a widget. Mouse and key events bubble from the
// target widget to its parents until one of them handles the event.
REvent :: struct {
	type: REventType,
	window: Rid,
	// Widget the event was first sent to.
	target: Rid,
	// Cursor position in the window, in logical units.
	x: f32,
	// Cursor position in the window, in logical units.
	y: f32,
	// Cursor position relative to the widget receiving the event.
	local_x: f32,
	// Cursor position relative to the widget receiving the event.
	local_y: f32,
	// Mouse movement since the previous event (R_EVENT_MOUSE_MOVE).
	delta_x: f32,
	// Mouse movement since the previous event (R_EVENT_MOUSE_MOVE).
	delta_y: f32,
	// Scroll amount in lines, positive = up / left (R_EVENT_MOUSE_WHEEL).
	wheel_x: f32,
	// Scroll amount in lines, positive = up / left (R_EVENT_MOUSE_WHEEL).
	wheel_y: f32,
	button: RMouseButton,
	// 1 = single click, 2 = double click, ... (R_EVENT_MOUSE_DOWN).
	clicks: i32,
	key: RKey,
	// Bitmask of RModifier.
	modifiers: u32,
	// R_TRUE when the key event comes from auto-repeat.
	repeat: i32,
	codepoint: u32,
	// R_EVENT_MOUSE_WHEEL: R_TRUE when the wheel comes from a trackpad (or a
	// mouse that scrolls smoothly). wheel_x, wheel_y are then small fractions of
	// a line, a line being 48 logical units the fingers moved: what follows the
	// fingers moves 48 units per line, and a canvas pans rather than zooms.
	wheel_precise: i32,
}

// What a drop target receives. The files and the text are only known when
// the drop happens (R_DROP_DROPPED), and are valid during the callback.
RDropEvent :: struct {
	phase: RDropPhase,
	window: Rid,
	// Pointer position in the window, and relative to the target widget.
	x: f32,
	// Pointer position in the window, and relative to the target widget.
	y: f32,
	local_x: f32,
	local_y: f32,
	// Dropped files and folders, as UTF-8 paths.
	paths: [^]cstring,
	path_count: i32,
	// Dropped text when it is not files, or NULL. For a drag inside the application: its data.
	text: cstring,
	// Drag inside the application (Rdrag_begin): its type, else NULL. Set in every phase.
	drag_type: cstring,
	// Widget the drag started from, or R_INVALID_ID.
	drag_source: Rid,
}

// How the library treats fingers and pens (Rtouch_set_settings, Rwindow_set_touch_settings).
RTouchSettings :: struct {
	// R_TRUE (default): one finger drives the widgets as the mouse does: a tap clicks, a drag drags.
	mouse_emulation: i32,
	// How far a finger moves (logical units) before a press becomes a drag or a scroll. Default 8.
	slop: f32,
	// Seconds a finger rests before the press is a right click (the context menu); 0 turns it off. Default 0.5.
	long_press: f32,
	// R_TRUE (default): flicked content keeps going and slows down.
	inertia: i32,
	// How fast it slows down, in logical units per second per second. Default 2400.
	deceleration: f32,
	// R_TRUE (default): two fingers pinch to zoom and pan.
	gestures: i32,
	// R_TRUE (default): the pen drives the widgets as the mouse does (its barrel button: the right button).
	pen_mouse_emulation: i32,
	// The pressure given to the application is the one of the pen to this power: 1 (default) as it is, below 1 softer, above 1 harder.
	pressure_curve: f32,
	// Pressure (0..1, after the curve) the tip needs before it presses. Default 0.
	pressure_threshold: f32,
	// R_TRUE (default): fingers are ignored while a pen is near the screen, and half a second after (the palm resting on it).
	palm_rejection: i32,
	// R_TRUE (default): the pinch and the rotation of a trackpad zoom what zooms, as two fingers on the screen do.
	trackpad_gestures: i32,
	// R_TRUE (default): three or more fingers swiped across a trackpad go back or forward (the side buttons of a mouse).
	swipe_navigation: i32,
	// How far (logical units) the fingers of a swipe go across before it navigates. Default 120.
	swipe_distance: f32,
}

RTouchEvent :: struct {
	phase: RTouchPhase,
	window: Rid,
	// Widget the finger came down on.
	target: Rid,
	// Number of the finger, the same from its down to its up.
	id: i32,
	// Position in the window, and relative to the widget receiving the event.
	x: f32,
	// Position in the window, and relative to the widget receiving the event.
	y: f32,
	local_x: f32,
	local_y: f32,
	// 0..1; 1 when the screen does not tell.
	pressure: f32,
	// Size of the contact, 0 when the screen does not tell.
	width: f32,
	// Size of the contact, 0 when the screen does not tell.
	height: f32,
	// Fingers on the window, this one included (0 after the last up).
	count: i32,
	// Bitmask of RModifier.
	modifiers: u32,
}

RPenEvent :: struct {
	phase: RPenPhase,
	window: Rid,
	// Widget under the pen; during a stroke, the one it started on.
	target: Rid,
	x: f32,
	y: f32,
	local_x: f32,
	local_y: f32,
	// 0..1, after the pressure curve; 0 while hovering.
	pressure: f32,
	// As the pen gives it, 0..1.
	raw_pressure: f32,
	// Tilt of the pen in degrees, -90..90, toward the right and toward the user; 0 when upright or unknown.
	tilt_x: f32,
	// Tilt of the pen in degrees, -90..90, toward the right and toward the user; 0 when upright or unknown.
	tilt_y: f32,
	// Rotation around its axis in degrees, 0..360 (when the pen tells).
	rotation: f32,
	// R_TRUE when the eraser end touches (or the eraser button is held).
	eraser: i32,
	// Bitmask of RPenButton.
	buttons: u32,
	modifiers: u32,
}

RGestureEvent :: struct {
	phase: RGesturePhase,
	window: Rid,
	// Widget under the center of the fingers when the gesture began.
	target: Rid,
	// Center of the fingers, in the window and relative to the widget receiving the event.
	x: f32,
	// Center of the fingers, in the window and relative to the widget receiving the event.
	y: f32,
	local_x: f32,
	local_y: f32,
	// Distance between the fingers relative to when the gesture began (2 = twice as far apart).
	scale: f32,
	// The same since the previous event: what a zoom multiplies by.
	scale_delta: f32,
	// Movement of the center since the previous event.
	pan_x: f32,
	// Movement of the center since the previous event.
	pan_y: f32,
	// Turn of the fingers since the gesture began, in degrees, clockwise.
	rotation: f32,
	// R_GESTURE_PINCH or R_GESTURE_SWIPE.
	kind: RGestureKind,
	// R_GESTURE_DEVICE_TOUCHSCREEN or R_GESTURE_DEVICE_TRACKPAD.
	device: RGestureDevice,
	// Fingers of the gesture (2 for a pinch, 3 or more for a swipe; 0 when the system does not tell).
	fingers: i32,
	// Movement of the center since the gesture began.
	total_x: f32,
	// Movement of the center since the gesture began.
	total_y: f32,
	// The same turn as `rotation`, since the previous event.
	rotation_delta: f32,
	// R_GESTURE_END: R_TRUE when the system took the gesture back (nothing must happen as for an end).
	cancelled: i32,
}

// How a PDF document is printed. Every field may be left to zero.
RPdfPrintDesc :: struct {
	// Window the print dialog belongs to (0 or R_INVALID_ID: none).
	window: Rid,
	// Name of the print job, shown in the queue of the printer. NULL: the title of the document or the name of the file.
	title: cstring,
	// Name of the printer, as the system knows it. NULL: the one chosen in the dialog, else the default printer.
	printer: cstring,
	// Copies to print (0: one); the dialog may change it.
	copies: i32,
	// Bitmask of RPdfPrintFlags.
	flags: u32,
	// Path of a file the job is written to instead of a printer. macOS: a
	// PDF made by the print system; Windows: what the driver of the printer
	// makes; elsewhere: the document itself.
	output: cstring,
	// Password of a document protected by one.
	password: cstring,
}

// What a tile shows, filled by the application when the tile is drawn.
RTileItem :: struct {
	// Name under the icon (valid until the callback returns).
	text: cstring,
	// Second line, dimmer, or NULL.
	detail: cstring,
	// Small text at the top right of the tile, or NULL.
	badge: cstring,
	// Icon texture (R_INVALID_ID: none), the region of it and its tint (white by default).
	icon: Rid,
	uv: [4]f32,
	tint: [4]f32,
	icon_radius: f32,
	// Color of the name; alpha 0 = the theme.
	text_color: [4]f32,
}

// What the editor knows of a language. Every field but the name may be NULL.
// Lists of words are separated by spaces.
RLanguageDesc :: struct {
	// "Rust". Compared without case.
	name: cstring,
	// Extensions of its files, separated by ';': "rs".
	extensions: cstring,
	// Words of the language itself: if, while, return...
	keywords: cstring,
	// Names of types and of what is always there: int, String, print...
	types: cstring,
	// Values with a name: true, false, null...
	constants: cstring,
	// What starts a comment that ends with its line: two slashes, a hash.
	line_comment: cstring,
	// What opens and closes a comment that may span lines.
	block_comment_start: cstring,
	block_comment_end: cstring,
	// Characters that open and close a text on one line: a double quote, a single quote.
	string_quotes: cstring,
	// Marks of the texts that span lines, separated by spaces: three double
	// quotes, a backquote. When what closes such a text is not what opens it,
	// write both around three dots: R"(...)" . Nothing is escaped in those.
	multiline_strings: cstring,
	// What starts a line for the preprocessor: "#".
	directive: cstring,
	// What starts an annotation: "@" (@Override), or "#[" up to its closing bracket.
	annotation: cstring,
	// Bitmask of RLanguageFlags.
	flags: u32,
	// Markup with R_LANGUAGE_EMBEDS: the language of what is between
	// <script> and </script> (the name of a language: "C", "Rust"...).
	// NULL: JavaScript, as in HTML.
	script_language: cstring,
}

// A task of the jump list of Windows (Rtaskbar_set_jump_list).
RJumpTask :: struct {
	// What the list shows.
	title: cstring,
	// The command line given to the application when it is chosen: it is started again with these arguments.
	arguments: cstring,
	// The tooltip; NULL: none.
	description: cstring,
}

// A kind of file in a file dialog.
RFileFilter :: struct {
	// Shown to the user: "Images".
	name: cstring,
	// Patterns separated by ';': "*.png;*.jpg". "*" matches everything.
	patterns: cstring,
}

// Every field may be left to zero.
RFileDialogDesc :: struct {
	// Window the dialog belongs to. R_INVALID_ID: none (a window is opened when Raven needs one).
	parent: Rid,
	// NULL: "Open", "Save as"...
	title: cstring,
	// Folder shown first. NULL: the one of the previous dialog, else the home of the user.
	directory: cstring,
	// Name proposed to the user (save dialogs).
	file_name: cstring,
	filters: ^RFileFilter,
	filter_count: i32,
	// Save dialogs: added to a name typed without extension ("xml").
	default_extension: cstring,
	// Bitmask of RFileDialogFlags.
	flags: u32,
}

// Passed to the draw callback of a viewport. Sizes are in pixels.
RViewportDrawInfo :: struct {
	backend: RBackend,
	viewport: Rid,
	window: Rid,
	// Area of the viewport in the window framebuffer.
	x: i32,
	// Area of the viewport in the window framebuffer.
	y: i32,
	// Area of the viewport in the window framebuffer.
	width: i32,
	// Area of the viewport in the window framebuffer.
	height: i32,
	// Visible part of that area. Already set as the scissor rectangle.
	clip_x: i32,
	// Visible part of that area. Already set as the scissor rectangle.
	clip_y: i32,
	// Visible part of that area. Already set as the scissor rectangle.
	clip_width: i32,
	// Visible part of that area. Already set as the scissor rectangle.
	clip_height: i32,
	framebuffer_width: i32,
	framebuffer_height: i32,
	frame_index: u64,
	// Vulkan: VkCommandBuffer being recorded, inside the render pass.
	command_buffer: rawptr,
	// Vulkan: VkRenderPass in use (one color attachment, no depth).
	render_pass: rawptr,
	// Vulkan: VkFormat of the color attachment.
	color_format: u32,
}

// State of one animated value. Zero-initialize it and keep it with the widget.
RSpring :: struct {
	value: f32,
	velocity: f32,
	started: i32,
}

// How an effect is drawn. Start from Rshader_look_init.
RShaderLook :: struct {
	// First and second color, 0..1, straight alpha.
	color: [4]f32,
	color2: [4]f32,
	// Four numbers the effect reads as fx.params (their meaning is the effect's).
	params: [4]f32,
	// Radius of the corners, in logical units.
	radius: f32,
	// Texture the effect reads with raven_texture(), or R_INVALID_ID.
	texture: Rid,
	// -1: animated when the shader is (R_SHADER_ANIMATED); R_TRUE / R_FALSE forces it.
	animate: i32,
	// R_TRUE (the default): the effect knows where the pointer is (fx.pointer, fx.hover, fx.press).
	pointer: i32,
	// How much the pointer is over it and how much it is pressed, 0..1, as
	// the effect reads them (fx.hover, fx.press). -1 (the default): 1 while
	// the pointer is in the rectangle, and while the left button is down
	// there; a Border eases them itself. The rectangle is clipped to the
	// visible painting area; overlaid siblings do not interrupt its hover.
	hover: f32,
	press: f32,
}

// Where and why a text could not be read (Rxml_document_parse).
RXmlError :: struct {
	// Both start at 1.
	line: i32,
	column: i32,
	message: [160]u8,
}

// One attribute of a tag (Rxml_get_attribute_info). The strings belong to the library.
RXmlAttributeInfo :: struct {
	name: cstring,
	type: RXmlValueType,
	// R_XML_CHOICE: the words, separated by '|'. NULL otherwise.
	choices: cstring,
	// What it belongs to: the tag itself ("Button"), "Widget" for what every widget has, "Slot" for the place in the parent.
	group: cstring,
	// One line saying what it does.
	description: cstring,
}

// What the system asks for.
RAccessibilitySettings :: struct {
	// Size of the texts asked by the user (1 = normal).
	text_scale: f32,
	high_contrast: i32,
	reduce_motion: i32,
	// R_TRUE when a screen reader is running.
	screen_reader: i32,
}

RHostPlatformDesc :: struct {
	// The pointer over `window` should look like `cursor`. May be NULL.
	set_cursor: rawptr,
	// Copies UTF-8 text to the clipboard of the application. NULL: the clipboard stays in the process.
	clipboard_set: rawptr,
	// The clipboard text, valid until the next call, or NULL. NULL: the clipboard stays in the process.
	clipboard_get: rawptr,
	// Raven has work to do (a timer, a posted task, a redraw): produce a frame soon. Thread-safe. May be NULL.
	wake: rawptr,
	user_data: rawptr,
}

@(default_calling_convention = "c")
foreign lib {
	// Starts the library with default settings on the given backend.
	// The calling thread becomes the UI thread.
	// Returns: R_TRUE on success. On failure, see Rerror_get_last.
	Rinit :: proc(backend: RBackend) -> i32 ---
	// Starts the library with explicit settings.
	// desc Settings, or NULL for the defaults.
	Rinit_ex :: proc(desc: ^RInitDesc) -> i32 ---
	// Destroys every window, widget and resource, stops the worker and
	// render threads and releases the backend.
	Rshutdown :: proc() ---
	Ris_initialized :: proc() -> i32 ---
	// Returns: The library version, e.g. "0.1.0".
	Rget_version :: proc() -> cstring ---
	// Replaces the allocator of Raven with the functions of the
	// application (an engine allocator, a tracker...). NULL selects the
	// allocator of Raven again. Must be called before Rinit, and before any
	// other function that allocates. The functions are called from several
	// threads; alloc must return memory aligned for any type (16 bytes).
	Rset_allocator :: proc(allocator: ^RAllocator) ---
	// Returns: An allocator that forwards to malloc, realloc and free of the C
	// runtime: Rset_allocator(Rallocator_get_system()) is how memory tools
	// that follow malloc (Valgrind, heap profilers) see the blocks of Raven.
	// The environment variable RAVEN_ALLOCATOR=system does the same without
	// recompiling, and it is the default in a build with AddressSanitizer.
	Rallocator_get_system :: proc() -> ^RAllocator ---
	// Memory from the allocator of Raven (or from the one given to
	// Rset_allocator), for the application: custom widgets, pixel buffers.
	// Aligned on 16 bytes. Never returns NULL: running out of memory is a
	// fatal error. Thread-safe.
	Rmemory_alloc :: proc(size: uint) -> rawptr ---
	// Resizes a block, keeping its content. A NULL block is allocated. Thread-safe.
	Rmemory_realloc :: proc(ptr: rawptr, size: uint) -> rawptr ---
	// Frees a block. NULL is ignored. Thread-safe.
	Rmemory_free :: proc(ptr: rawptr) ---
	// What the library holds right now. Thread-safe.
	Rmemory_get_stats :: proc(stats: ^RMemoryStats) ---
	// Walks the whole heap of Raven and verifies its structure.
	// Returns: R_FALSE when it is corrupted (a buffer overrun wrote over a block
	// header). Slow: for tests and for hunting a bug. Thread-safe.
	Rmemory_check :: proc() -> i32 ---
	// Returns to the OS the regions of the heap that hold nothing. Thread-safe.
	Rmemory_trim :: proc() ---
	// Produces one frame: processes the pending OS events without
	// waiting, runs the posted tasks and the timers, then lays out, paints and
	// submits every window that changed. A window in which nothing changed is
	// not drawn again, so the call is cheap when the UI is idle.
	// Returns: R_FALSE once Rquit was called or the last window is closed.
	Rdraw_frame :: proc() -> i32 ---
	// Same as Rdraw_frame, but first sleeps until there is something to
	// do: an OS event, a posted task, a timer, an animation, a pending redraw.
	// timeout_seconds Longest sleep. Negative sleeps without limit, 0
	// does not sleep.
	Rwait_frame :: proc(timeout_seconds: f64) -> i32 ---
	// Makes Rdraw_frame and Rwait_frame return R_FALSE. Thread-safe.
	Rquit :: proc(exit_code: i32) ---
	// Returns: The code given to Rquit, 0 when the last window was closed.
	Rget_exit_code :: proc() -> i32 ---
	// Returns: R_FALSE once Rquit was called or every window is closed.
	Ris_running :: proc() -> i32 ---
	// Processes the pending OS events without producing a frame.
	// Rdraw_frame does it already: only useful to keep the windows responsive
	// during a long operation.
	Rpoll_events :: proc() ---
	// Sleeps until an OS event, a posted task, a timer or a pending
	// redraw, and processes the OS events. Rwait_frame does it already.
	// timeout_seconds Maximum wait. Negative waits without limit.
	Rwait_events :: proc(timeout_seconds: f64) ---
	Rset_frame_callback :: proc(callback: RFrameCallback, user_data: rawptr) ---
	// Returns: Seconds between the last two frames.
	Rget_delta_time :: proc() -> f64 ---
	// Returns: Seconds elapsed since Rinit, from a monotonic clock. Thread-safe.
	Rget_time :: proc() -> f64 ---
	// Scale of the whole interface, on top of the one of the monitor
	// (RInitDesc.ui_scale at start): every window is laid out again. 0.25 .. 4.
	Rset_ui_scale :: proc(scale: f32) ---
	// R_TRUE: every frame waits for the vertical synchronization of the
	// screen (no tearing, no wasted frames); R_FALSE: as fast as possible
	// (RInitDesc.disable_vsync at start). Takes effect at the next frame.
	Rset_vsync :: proc(enabled: i32) ---
	Rget_vsync :: proc() -> i32 ---
	Rget_ui_scale :: proc() -> f32 ---
	// R_FALSE makes every change immediate: for users who ask for
	// reduced motion (Raccessibility_set_reduce_motion), and for tests that
	// read the state right after an input.
	Rset_animations_enabled :: proc(enabled: i32) ---
	Rget_animations_enabled :: proc() -> i32 ---
	// Multiplies the speed of every animation. 1 is the default, 0.25 a slow motion.
	Rset_animation_speed :: proc(speed: f32) ---
	Rget_animation_speed :: proc() -> f32 ---
	// When positive, each frame advances the animations by exactly this
	// many seconds instead of the time really elapsed: frames rendered one by
	// one for a video or a screenshot, deterministic tests. 0 follows the clock.
	Rset_animation_time_step :: proc(seconds: f64) ---
	// Queues a function to run on the UI thread during the next frame
	// and wakes Rwait_frame. Thread-safe: this is how worker threads touch the UI.
	Rtask_post :: proc(task: RTaskCallback, user_data: rawptr) ---
	// Runs a function on a worker thread of the job system. Thread-safe.
	Rtask_run_async :: proc(task: RTaskCallback, user_data: rawptr) ---
	// Splits [0, count) into ranges of at most `grain` items and runs them
	// on the worker threads and the calling thread. Returns when all are done.
	Rparallel_for :: proc(count: u32, grain: u32, function: RParallelRangeCallback, user_data: rawptr) ---
	// Wakes Rwait_frame and Rwait_events from any thread.
	Rwake_up :: proc() ---
	Rget_worker_thread_count :: proc() -> i32 ---
	Ris_ui_thread :: proc() -> i32 ---
	// Calls a function on the UI thread after `interval_seconds`.
	// repeat R_TRUE to fire repeatedly, R_FALSE to fire once and delete itself.
	Rtimer_create :: proc(interval_seconds: f64, repeat: i32, callback: RTimerCallback, user_data: rawptr) -> Rid ---
	Rtimer_destroy :: proc(timer: Rid) ---
	Rtimer_is_valid :: proc(timer: Rid) -> i32 ---
	Raction_bind :: proc(name: cstring, callback: RActionCallback, user_data: rawptr) ---
	Raction_unbind :: proc(name: cstring) ---
	// Fires an action as a widget would: the function bound to the
	// name is called, or the default handler. What a tool bar button, a
	// script or a test uses to do what a menu item does.
	// sender Passed to the function; R_INVALID_ID when there is none.
	Raction_fire :: proc(name: cstring, sender: Rid) ---
	// Receives every action that has no bound function and is not one of the built-in actions (e.g. a script bridge).
	Raction_set_default_handler :: proc(callback: RActionCallback, user_data: rawptr) ---
	// The actions Raven knows by itself: an interface written in XML
	// uses them with no C at all (OnClicked="app.quit").
	//
	//   app.quit                          quits (Rquit(0))
	//   window.close, window.minimize,    the window of the widget
	//   window.maximize, window.restore   (maximize goes back when it already is)
	//   window.picture_in_picture         in and out of picture in picture
	//   island.expand, island.fold,       the island of that window
	//   island.toggle
	//   theme.dark, theme.light,          the look of Raven, dark or light
	//   theme.toggle
	//   theme:NAME                        Rtheme_use(NAME): a theme loaded or built in
	//   language:CODE                     Rlocale_set_language(CODE)
	//   open:ADDRESS                      Ropen_url (https:, mailto:, a file)
	//   copy:TEXT                         the text to the clipboard
	//   show:NAME, hide:NAME, toggle:NAME the widget named NAME in the window (hidden: Collapsed)
	//   enable:NAME, disable:NAME
	//   focus:NAME
	//   page:NAME=2, page:NAME=+1         what a Switcher, a TabView, a ComboBox or a ListView
	//                                     shows; +1 and -1 step, and go round
	//
	// A function bound to one of these names (Raction_bind) replaces it; the
	// default handler receives the others only.
	// enabled R_FALSE turns them all off: a design tool that shows
	// interfaces without running them. On by default.
	Raction_set_builtins :: proc(enabled: i32) ---
	// Returns: R_TRUE when the name is one of the built-in actions (Raction_set_builtins), whether they are on or not.
	Raction_is_builtin :: proc(name: cstring) -> i32 ---
	// Opens an address with the program the user has for it: a web
	// page in the browser, a mailto: in the mail program, a file in its
	// application. Returns at once.
	// Returns: R_FALSE when the OS could not be asked (nothing tells whether
	// a program then really opened it).
	Ropen_url :: proc(url: cstring) -> i32 ---
	Rclipboard_set_text :: proc(text: cstring) ---
	// Copies one content in two formats: `html` for the programs that
	// paste formatting (word processors, chat, mail) and `text` for the
	// others (editors, terminals). Each program takes the format it prefers.
	// Where the window system has no clipboard formats (Win32 and macOS for
	// now), or `html` is NULL, it is Rclipboard_set_text(text).
	Rclipboard_set_html :: proc(text: cstring, html: cstring) ---
	// Returns: The clipboard text, or "" when empty. Valid until the next call.
	Rclipboard_get_text :: proc() -> cstring ---
	// Retrieves and clears the last recorded error.
	// detail   Optional output: human-readable description.
	// severity Optional output: severity of the error.
	// Returns: R_NO_ERROR when nothing was recorded.
	Rerror_get_last :: proc(detail: ^cstring, severity: ^RSeverity) -> RError ---
	Rerror_to_string :: proc(error: RError) -> cstring ---
	Rseverity_to_string :: proc(severity: RSeverity) -> cstring ---
	Rerror_set_callback :: proc(callback: RErrorCallback) ---
	Rset_device_reset_callback :: proc(callback: RDeviceResetCallback, user_data: rawptr) ---
	Rwindow_get_frame_stats :: proc(window: Rid, stats: ^RFrameStats) ---
	// Opens a top-level window.
	// width,height Client size in logical units.
	// flags        Bitmask of RWindowFlags.
	Rwindow_create :: proc(title: cstring, width: i32, height: i32, flags: u32) -> Rid ---
	// Closes a window and deletes its content widgets.
	Rwindow_destroy :: proc(window: Rid) ---
	Rwindow_is_valid :: proc(window: Rid) -> i32 ---
	// Sets the root widget of the window. The window takes ownership of
	// the widget; the previous content is deleted.
	Rwindow_set_content :: proc(window: Rid, widget: Rid) ---
	Rwindow_get_content :: proc(window: Rid) -> Rid ---
	Rwindow_set_title :: proc(window: Rid, title: cstring) ---
	Rwindow_get_title :: proc(window: Rid) -> cstring ---
	Rwindow_set_size :: proc(window: Rid, width: i32, height: i32) ---
	Rwindow_get_size :: proc(window: Rid, width: ^i32, height: ^i32) ---
	// The part of the window the system covers (status bar, notch,
	// home indicator, navigation bar), in logical units. The content of the
	// window is laid out inside it already. All 0 on desktop.
	Rwindow_get_safe_area :: proc(window: Rid, insets: ^RInsets) ---
	// The part of the window the on-screen keyboard covers (only
	// `bottom` is set), in logical units; 0 when it is hidden or there is
	// none (desktop). The content is laid out above it already.
	Rwindow_get_keyboard_area :: proc(window: Rid, insets: ^RInsets) ---
	// Called when the on-screen keyboard appears, changes height or goes away (mobile). NULL removes it.
	Rwindow_set_keyboard_callback :: proc(window: Rid, callback: RKeyboardCallback, user_data: rawptr) ---
	// No widget of the window has the keyboard focus any more: on mobile, the on-screen keyboard goes away.
	Rwindow_clear_focus :: proc(window: Rid) ---
	Rwindow_set_min_size :: proc(window: Rid, width: i32, height: i32) ---
	Rwindow_set_position :: proc(window: Rid, x: i32, y: i32) ---
	Rwindow_get_position :: proc(window: Rid, x: ^i32, y: ^i32) ---
	// Size of the drawable area in pixels.
	Rwindow_get_framebuffer_size :: proc(window: Rid, width: ^i32, height: ^i32) ---
	// Returns: Pixels per logical unit (monitor scale x global UI scale).
	Rwindow_get_scale :: proc(window: Rid) -> f32 ---
	Rwindow_show :: proc(window: Rid) ---
	Rwindow_hide :: proc(window: Rid) ---
	// Shows the window in front of the others with the keyboard, and
	// makes the application the active one. Rwindow_show alone does not take
	// the keyboard from another application (a prompt that appears while the
	// user types elsewhere); this is for a window the user just asked for (a
	// card opened from a tray icon). Where the system has no such request,
	// the same as Rwindow_show.
	Rwindow_focus :: proc(window: Rid) ---
	Rwindow_maximize :: proc(window: Rid) ---
	Rwindow_minimize :: proc(window: Rid) ---
	Rwindow_restore :: proc(window: Rid) ---
	Rwindow_is_maximized :: proc(window: Rid) -> i32 ---
	Rwindow_is_minimized :: proc(window: Rid) -> i32 ---
	// Returns: R_TRUE while the window is the one the user is working in.
	Rwindow_is_active :: proc(window: Rid) -> i32 ---
	// Asks to close the window, as the close button of its title bar
	// does: the close callback is asked first and may refuse.
	Rwindow_close :: proc(window: Rid) ---
	// Custom headers. Call these from a mouse-down handler, while the left
	// button is held: the window follows the pointer until the button is
	// released (the widget does not receive the release). A
	// R_WIDGET_HEADER_BAR and the widgets given to Rwidget_set_window_drag do
	// it by themselves.
	Rwindow_begin_move :: proc(window: Rid) ---
	Rwindow_begin_resize :: proc(window: Rid, edge: RWindowEdge) ---
	// Icon of the window (title bar, task bar), from tightly packed
	// RGBA8 pixels; NULL goes back to the default: on Windows the first icon
	// of the executable, when it has one. Windows and X11.
	Rwindow_set_icon :: proc(window: Rid, rgba: [^]u8, width: i32, height: i32) ---
	Rwindow_set_clear_color :: proc(window: Rid, r: f32, g: f32, b: f32, a: f32) ---
	// Image drawn behind the content, scaled to cover the window:
	// what the glass of a glass theme looks through. R_INVALID_ID removes it.
	Rwindow_set_background :: proc(window: Rid, texture: Rid) ---
	// Chooses what shows behind a window created with
	// R_WINDOW_TRANSPARENT (see RBackdrop). With anything but
	// R_BACKDROP_NONE the window is not cleared with its color and the
	// wallpaper of the theme is not drawn: only what the widgets draw hides
	// the screen. In XML: <Window Backdrop="Blur">.
	// Returns: R_TRUE when the window system does it; R_FALSE (the window
	// then stays as it was) without R_WINDOW_TRANSPARENT, or when it cannot.
	Rwindow_set_backdrop :: proc(window: Rid, backdrop: RBackdrop) -> i32 ---
	Rwindow_get_backdrop :: proc(window: Rid) -> RBackdrop ---
	// Gives the window a theme of its own, by name ("Dark",
	// "WindowsLight", or a theme of Rtheme_load): everything inside it (its
	// content, its island, its menus, lists and dialogs, its background) is
	// drawn with that theme, while the other windows keep the one of the
	// application (Rtheme_use). A widget given Rwidget_set_theme keeps its own.
	// In XML: <Window Theme="...">.
	// name NULL or "": the theme of the application again.
	// Returns: R_FALSE, after reporting an error, when no theme has that name.
	Rwindow_set_theme :: proc(window: Rid, name: cstring) -> i32 ---
	// Returns: Name of the theme the window is drawn with (the one of the application when it has none of its own).
	Rwindow_get_theme :: proc(window: Rid) -> cstring ---
	// Returns: R_TRUE when the window has a theme of its own (Rwindow_set_theme).
	Rwindow_has_own_theme :: proc(window: Rid) -> i32 ---
	// Opacity of the whole window, its frame included, from 0 to 1:
	// a window that fades, a tool that lets what is under it show.
	// Returns: R_FALSE when the window system cannot do it.
	Rwindow_set_opacity :: proc(window: Rid, opacity: f32) -> i32 ---
	Rwindow_get_opacity :: proc(window: Rid) -> f32 ---
	// Keeps the window above the other windows (splash screens, tools).
	// Returns: R_FALSE when the window system cannot do it.
	Rwindow_set_topmost :: proc(window: Rid, topmost: i32) -> i32 ---
	// Gives the window an owner (a tool window or an inspector of the
	// main window): it stays above its owner, has no button of its own in the
	// taskbar, is minimized and restored with it (where the window system
	// does it: Windows, X11), and is destroyed with it (everywhere: Raven
	// destroys the windows a window owns before the window). R_INVALID_ID
	// gives the window back its independence.
	// Returns: R_FALSE when the window system cannot keep the window with its
	// owner (it is still destroyed with it), or when the owner is not valid
	// or is owned by the window (nothing is changed then).
	Rwindow_set_owner :: proc(window: Rid, owner: Rid) -> i32 ---
	// Returns: The owner of the window (Rwindow_set_owner), or R_INVALID_ID.
	Rwindow_get_owner :: proc(window: Rid) -> Rid ---
	// Picture in picture: the window becomes a small floating window
	// that stays above the other windows, on every desktop (every space of
	// macOS, over the applications in full screen too), in a corner of the
	// work area of its monitor. It loses its title bar where the system
	// allows: the empty parts of its content move it, its edges resize it,
	// and its size keeps the proportions it was given. Leaving it gives back
	// the size, place, frame and state the window had.
	//
	// Raven does not add a button to leave it: the content shows one, with
	// the built-in action "window.picture_in_picture" (it goes in and out).
	// Returns: R_FALSE when the window system cannot keep a window above the
	// others (Wayland): the window is left as it is.
	Rwindow_set_picture_in_picture :: proc(window: Rid, enabled: i32) -> i32 ---
	Rwindow_is_picture_in_picture :: proc(window: Rid) -> i32 ---
	// Size (logical units) and corner of the window in picture in
	// picture, for the next time it goes in (now, when it is in). 0, 0 (the
	// default): 320 wide, with the proportions the content has when it goes
	// in. The proportions of width / height are kept while the user resizes.
	Rwindow_set_picture_in_picture_layout :: proc(window: Rid, width: i32, height: i32, corner: RPictureInPictureCorner) ---
	// Centers the window on the work area of its monitor (no task bar).
	Rwindow_center :: proc(window: Rid) ---
	// Shows (R_TRUE) or hides (R_FALSE) the inspector of the window: a
	// window beside it that lists its widgets as a tree, says what the
	// selected one is (type, name, classes, place and size in the window,
	// desired size, constraints, slot, visibility, state, style, action,
	// children) and what the last frame cost (layout, paint and render
	// times, widgets, instances, draw calls). The selected widget is
	// outlined in the window, with its type and size. "Pick" selects the
	// next widget clicked in the window (that click goes to the inspector
	// only). It follows the window as it changes; closing it hides it, and
	// it goes with the window. Nothing opens it but this function.
	Rwindow_set_inspector :: proc(window: Rid, shown: i32) ---
	Rwindow_is_inspector_shown :: proc(window: Rid) -> i32 ---
	// Returns: The window of the inspector of a window (to place it), or R_INVALID_ID when it is hidden.
	Rwindow_get_inspector :: proc(window: Rid) -> Rid ---
	// Returns: R_TRUE for a window created with R_WINDOW_SPLASH_SCREEN (IsSplashScreen in XML).
	Rwindow_is_splash_screen :: proc(window: Rid) -> i32 ---
	// Asks for a new frame even if no widget changed. Thread-safe.
	Rwindow_request_redraw :: proc(window: Rid) ---
	// R_TRUE redraws the window every frame (games, live viewports).
	Rwindow_set_continuous_redraw :: proc(window: Rid, enabled: i32) ---
	Rwindow_set_close_callback :: proc(window: Rid, callback: RWindowCloseCallback, user_data: rawptr) ---
	Rwindow_set_resize_callback :: proc(window: Rid, callback: RWindowResizeCallback, user_data: rawptr) ---
	// Takes what is dropped on the window from other applications,
	// where no widget takes it (Rwidget_set_drop_callback). NULL removes it.
	Rwindow_set_drop_callback :: proc(window: Rid, callback: RDropCallback, user_data: rawptr) ---
	// During R_DROP_ENTER / R_DROP_MOVE only: choose Raven's highlight
	// in target-local logical units (window-local for a window callback).
	// Clipped to the target; zero width or height hides it without refusing
	// the drop. Each callback starts with the default highlight again.
	// Dimensions must be finite and nonnegative; coordinates must be finite.
	Rdrop_set_highlight_rect :: proc(x: f32, y: f32, width: f32, height: f32) ---
	Rwindow_set_user_handle :: proc(window: Rid, handle: rawptr) ---
	Rwindow_get_user_handle :: proc(window: Rid) -> rawptr ---
	// Fills `native` with the OS handles of the window.
	Rwindow_get_native_handle :: proc(window: Rid, native: ^RNativeWindow) -> i32 ---
	// Reads back the last rendered frame as tightly packed RGBA8 pixels.
	// Blocks until the frame is rendered. Free the result with Rpixels_free.
	// The alpha is 255, except for a window that is seen through
	// (R_WINDOW_TRANSPARENT): its pixels are then premultiplied by their
	// alpha, as the window system receives them.
	// Returns: NULL when the backend cannot capture.
	Rwindow_capture :: proc(window: Rid, width: ^i32, height: ^i32) -> [^]u8 ---
	// Widget that has the keyboard focus in the window, or R_INVALID_ID.
	Rwindow_get_focused_widget :: proc(window: Rid) -> Rid ---
	// Deepest widget under the cursor, or R_INVALID_ID.
	Rwindow_get_hovered_widget :: proc(window: Rid) -> Rid ---
	Rwindow_get_mouse_position :: proc(window: Rid, x: ^f32, y: ^f32) ---
	// Inject input as if it came from the OS. Positions are in logical units.
	// Useful for automated tests, replays and when Raven is driven by a host
	// application.
	Rwindow_inject_mouse_move :: proc(window: Rid, x: f32, y: f32) ---
	Rwindow_inject_mouse_button :: proc(window: Rid, button: RMouseButton, pressed: i32) ---
	Rwindow_inject_mouse_wheel :: proc(window: Rid, wheel_x: f32, wheel_y: f32) ---
	Rwindow_inject_key :: proc(window: Rid, key: RKey, pressed: i32, modifiers: u32) ---
	Rwindow_inject_text :: proc(window: Rid, text: cstring) ---
	// A drag from another application moves over the window
	// (`inside` R_TRUE) or leaves it.
	// Returns: R_TRUE when a drop at this position would be accepted.
	Rwindow_inject_drag :: proc(window: Rid, x: f32, y: f32, inside: i32) -> i32 ---
	// Drops files and / or text at a position. Returns R_TRUE when a target took them.
	Rwindow_inject_drop :: proc(window: Rid, x: f32, y: f32, paths: [^]cstring, path_count: i32, text: cstring) -> i32 ---
	// R_TRUE while the key is held down in the window (cleared when the window loses the keyboard).
	Rwindow_is_key_down :: proc(window: Rid, key: RKey) -> i32 ---
	// R_TRUE while the mouse button is held down over (or captured by) the window.
	Rwindow_is_mouse_button_down :: proc(window: Rid, button: RMouseButton) -> i32 ---
	// Modifiers (RModifier) of the last input event.
	Rwindow_get_modifiers :: proc(window: Rid) -> u32 ---
	// R_TRUE while the pointer is over the client area.
	Rwindow_is_mouse_inside :: proc(window: Rid) -> i32 ---
	// R_TRUE when the keyboard focus is in a text input, spin box or text editor.
	Rwindow_wants_text_input :: proc(window: Rid) -> i32 ---
	// The kind of the focused text input; R_FALSE when there is none.
	Rwindow_get_text_input_info :: proc(window: Rid, info: ^RTextInputInfo) -> i32 ---
	// Its text (UTF-8), valid until it changes; NULL when there is none.
	Rwindow_get_text_input_text :: proc(window: Rid) -> cstring ---
	// Its selection: start == end is the caret. R_FALSE when there is none.
	Rwindow_get_text_input_selection :: proc(window: Rid, start: ^i32, end: ^i32) -> i32 ---
	// Selects bytes [start, end) of it (end before start puts the caret at end).
	Rwindow_set_text_input_selection :: proc(window: Rid, start: i32, end: i32) -> i32 ---
	// Replaces bytes [start, end) of it with `text` and puts the caret
	// after it, as typing would (the changed callback runs; MaxLength
	// applies). R_FALSE when there is none or it is read-only.
	Rwindow_replace_text_input :: proc(window: Rid, start: i32, end: i32, text: cstring) -> i32 ---
	// R_TRUE while a menu, list or dialog is open over the window.
	Rwindow_has_popup :: proc(window: Rid) -> i32 ---
	// Last painted caret in window pixels (x, y, width, height), including scrolling and scale.
	// Call after Rdraw_frame. Independent of the blink phase. Returns R_FALSE and zeros the
	// outputs when there is no visible editable caret, the window is inactive, or focus changed
	// since painting. Any output pointer may be NULL.
	Rwindow_get_text_input_rect :: proc(window: Rid, x: ^i32, y: ^i32, width: ^i32, height: ^i32) -> i32 ---
	// Returns: Number of open in-window popups (menus, combo lists, pickers), bottom to top.
	Rwindow_get_popup_count :: proc(window: Rid) -> i32 ---
	// Returns: Root widget of popup `index`, whose rect is in window coordinates; R_INVALID_ID out of range.
	Rwindow_get_popup :: proc(window: Rid, index: i32) -> Rid ---
	// R_FALSE: Tab no longer moves the keyboard focus (the application uses it as a shortcut). Default R_TRUE.
	Rwindow_set_tab_navigation :: proc(window: Rid, enabled: i32) ---
	Rwindow_set_event_hook :: proc(window: Rid, hook: RWindowEventHook, user_data: rawptr) ---
	// Starts dragging `data` from `source` (call it while a mouse button
	// is held, typically from a mouse-move event once the pointer has moved
	// a few units). Until the button is released, the widgets with a drop
	// callback (Rwidget_set_drop_callback) under the pointer receive
	// R_DROP_ENTER / MOVE / LEAVE and, on release, R_DROP_DROPPED, with
	// `drag_type` set and `text` = `data`. Returning R_FALSE from the callback
	// refuses the drop there. Escape cancels. `label` follows the pointer.
	// Returns: R_FALSE when the widget is not in a window.
	Rdrag_begin :: proc(source: Rid, type: cstring, data: cstring, label: cstring) -> i32 ---
	// R_TRUE while a drag started by Rdrag_begin is in progress in the window.
	Rdrag_is_active :: proc(window: Rid) -> i32 ---
	// Type of the drag in progress, or NULL.
	Rdrag_get_type :: proc(window: Rid) -> cstring ---
	// Data of the drag in progress, or NULL.
	Rdrag_get_data :: proc(window: Rid) -> cstring ---
	// Ends the drag in progress without dropping (the target receives R_DROP_LEAVE).
	Rdrag_cancel :: proc(window: Rid) ---
	Rwindow_register_shortcut :: proc(window: Rid, key: RKey, modifiers: u32, action: cstring) ---
	Rwindow_unregister_shortcut :: proc(window: Rid, key: RKey, modifiers: u32) ---
	// The settings every window follows, unless it has its own.
	Rtouch_get_settings :: proc(settings: ^RTouchSettings) ---
	// Changes them; NULL puts back the defaults.
	Rtouch_set_settings :: proc(settings: ^RTouchSettings) ---
	// The defaults, to start from.
	Rtouch_get_default_settings :: proc(settings: ^RTouchSettings) ---
	// Settings of one window; NULL: it follows Rtouch_set_settings again.
	Rwindow_set_touch_settings :: proc(window: Rid, settings: ^RTouchSettings) ---
	// The settings the window follows (its own, or those of the application).
	Rwindow_get_touch_settings :: proc(window: Rid, settings: ^RTouchSettings) ---
	// What a finger does on the widget and what it contains (default R_TOUCH_MODE_AUTO: as its parent says).
	Rwidget_set_touch_mode :: proc(widget: Rid, mode: RTouchMode) ---
	Rwidget_get_touch_mode :: proc(widget: Rid) -> RTouchMode ---
	// R_FALSE: two fingers (on the screen or a trackpad) neither zoom nor pan the widget (default R_TRUE).
	Rwidget_set_touch_gestures :: proc(widget: Rid, enabled: i32) ---
	Rwidget_get_touch_gestures :: proc(widget: Rid) -> i32 ---
	// R_FALSE: the pen is not a mouse on the widget and what it contains; only its pen callback sees it (default R_TRUE).
	Rwidget_set_pen_mouse :: proc(widget: Rid, enabled: i32) ---
	Rwidget_get_pen_mouse :: proc(widget: Rid) -> i32 ---
	Rwidget_set_touch_callback :: proc(widget: Rid, callback: RTouchCallback, user_data: rawptr) ---
	Rwidget_set_pen_callback :: proc(widget: Rid, callback: RPenCallback, user_data: rawptr) ---
	Rwidget_set_gesture_callback :: proc(widget: Rid, callback: RGestureCallback, user_data: rawptr) ---
	// Fingers on the window now.
	Rwindow_get_touch_count :: proc(window: Rid) -> i32 ---
	// R_TRUE while a pen is near the screen over the window.
	Rwindow_is_pen_in_range :: proc(window: Rid) -> i32 ---
	// R_TRUE when the pointer last used in the window was a finger (the interface may then be made larger).
	Rwindow_is_touch_active :: proc(window: Rid) -> i32 ---
	// A finger: R_TOUCH_DOWN, then R_TOUCH_MOVE..., then R_TOUCH_UP, with the same `id`.
	Rwindow_inject_touch :: proc(window: Rid, id: i32, phase: RTouchPhase, x: f32, y: f32) ---
	// A pen: `pressure` 0..1 as the pen would give it, `buttons` a bitmask of RPenButton.
	Rwindow_inject_pen :: proc(window: Rid, phase: RPenPhase, x: f32, y: f32, pressure: f32, tilt_x: f32, tilt_y: f32, buttons: u32, eraser: i32) ---
	// A gesture of a trackpad, as the system recognizes it: R_GESTURE_BEGIN,
	// R_GESTURE_UPDATE..., then R_GESTURE_END. (x, y) is the pointer, `scale_delta`
	// and `rotation_delta` (degrees, clockwise) what changed since the previous
	// event, `pan_x`, `pan_y` how far the fingers moved since then.
	Rwindow_inject_trackpad_gesture :: proc(window: Rid, kind: RGestureKind, phase: RGesturePhase, x: f32, y: f32, scale_delta: f32, rotation_delta: f32, pan_x: f32, pan_y: f32, fingers: i32) ---
	// Two fingers scrolling on a trackpad: (dx, dy) is how far they moved
	// (logical units), the content following them. Sent as a precise wheel
	// (REvent::wheel_precise) at the pointer.
	Rwindow_inject_trackpad_scroll :: proc(window: Rid, dx: f32, dy: f32) ---
	// The fingers that scrolled left the trackpad, on a system that
	// leaves the inertia to the application (Wayland): flicked content keeps
	// going (Touch.Inertia).
	Rwindow_inject_trackpad_scroll_end :: proc(window: Rid) ---
	// Advances the inertia of flicked content by `seconds` at once,
	// and fires a long press that is due, without waiting (tests).
	Rwindow_advance_touch :: proc(window: Rid, seconds: f64) ---
	// Changes the look of the application. The theme file in use, if
	// it names no look of its own, is laid again over the new one; colors
	// and metrics changed with Rtheme_set_... are lost; what was given to
	// widgets (Rwidget_set_color...) is kept.
	//
	//   Rlook_set(R_LOOK_NATIVE_LIGHT);   the look of the platform, light
	//
	// Theme files that name no look (no Look attribute) follow it too, so a
	// part of a window given one (Theme="paper.xml") changes with it.
	Rlook_set :: proc(look: RLook) ---
	// The look given to Rlook_set (R_LOOK_RAVEN_DARK at start).
	Rlook_get :: proc() -> RLook ---
	// @deprecated Rlook_set: a built-in theme is a look of Raven.
	Rtheme_apply :: proc(theme: RTheme) ---
	Rtheme_set_color :: proc(role: RColorRole, r: f32, g: f32, b: f32, a: f32) ---
	Rtheme_get_color :: proc(role: RColorRole, r: ^f32, g: ^f32, b: ^f32, a: ^f32) ---
	Rtheme_set_metric :: proc(metric: RMetric, value: f32) ---
	Rtheme_get_metric :: proc(metric: RMetric) -> f32 ---
	// Material of every widget that was not given one with
	// Rwidget_set_material. Rtheme_apply sets it: solid for the flat
	// themes, frosted glass for the glass themes.
	Rtheme_set_material :: proc(material: RMaterial) ---
	Rtheme_get_material :: proc() -> RMaterial ---
	// Font used by widgets that have no font of their own.
	Rtheme_set_font :: proc(font: Rid) ---
	Rtheme_get_font :: proc() -> Rid ---
	// Sets the accent color and everything that follows from it: its
	// hovered and pressed shades, the focus outline, the selection, the links.
	Rtheme_set_accent :: proc(r: f32, g: f32, b: f32) ---
	// Tells whether the theme is a dark one. It decides the tint of
	// the glass, the colors of the code and which wallpaper the glass themes
	// use. Rtheme_apply and the theme files set it.
	Rtheme_set_dark :: proc(dark: i32) ---
	// R_TRUE when the system is in dark mode, R_FALSE in light mode, -1 when the platform does not tell (desktop today).
	Rtheme_get_system_dark :: proc() -> i32 ---
	// Follows the dark or light mode of the system with the Raven look
	// (R_LOOK_RAVEN_DARK / R_LOOK_RAVEN_LIGHT), now and when it changes.
	// Off by default. Does nothing where the platform does not tell (desktop today).
	Rtheme_follow_system :: proc(enabled: i32) ---
	Rtheme_is_dark :: proc() -> i32 ---
	// Image drawn behind the content of the windows that have no
	// background of their own, or R_INVALID_ID for none (the window color).
	Rtheme_set_wallpaper :: proc(texture: Rid) ---
	Rtheme_get_wallpaper :: proc() -> Rid ---
	// Gives a color to the widgets of a type and / or of a class,
	// instead of the color of the theme.
	// widget      Tag of a type of widget ("Button"), or NULL for every type.
	// style_class A class (Rwidget_set_class, Class="..."), or NULL
	//                    for the widgets of the type whatever their classes.
	//
	// When several styles give the same value to a widget, the one with a
	// type and a class wins over the one with a class, which wins over the
	// one with a type; between equals, the last one set wins.
	Rtheme_set_style_color :: proc(widget: cstring, style_class: cstring, role: RColorRole, r: f32, g: f32, b: f32, a: f32) ---
	Rtheme_set_style_metric :: proc(widget: cstring, style_class: cstring, metric: RMetric, value: f32) ---
	// R_MATERIAL_DEFAULT removes the material of the style.
	Rtheme_set_style_material :: proc(widget: cstring, style_class: cstring, material: RMaterial) ---
	// Removes every style of the theme. Rtheme_apply and Rtheme_use do it too.
	Rtheme_clear_styles :: proc() ---
	// Reads a theme file and keeps the theme under its name (the Name
	// attribute, else the name of the file). It does not change the theme in
	// use unless a theme of that name is the one in use.
	//
	// The file is watched like the interface files (Rxml_set_live_reload):
	// saving it updates every widget that uses the theme.
	// Returns: The name of the theme, valid until Rshutdown, or NULL after
	// reporting an error.
	Rtheme_load :: proc(path: cstring) -> cstring ---
	// Same, from a text. `size` may be 0 for a null-terminated text.
	Rtheme_load_from_memory :: proc(xml: cstring, size: uint) -> cstring ---
	// Makes a named theme the theme of the application: a theme
	// given by Rtheme_load (laid over the look in use when it names no
	// look), or the name of a look ("WindowsLight", "RavenDark"; the old
	// "Dark", "Light", "GlassDark"... too), which is then Rlook_set.
	// Values changed with Rtheme_set_... are lost.
	// Returns: R_FALSE, after reporting an error, when no theme has that name.
	Rtheme_use :: proc(name: cstring) -> i32 ---
	// Name of the theme in use ("Dark" at start).
	Rtheme_get_name :: proc() -> cstring ---
	// Number of named themes, the four built-in ones included.
	Rtheme_get_count :: proc() -> i32 ---
	// Name of the index-th one, or NULL.
	Rtheme_get_name_at :: proc(index: i32) -> cstring ---
	// Writes the theme in use, with every change made since it was
	// applied, as a theme file.
	// name Name given to the theme in the file; NULL keeps the current one.
	Rtheme_save :: proc(path: cstring, name: cstring) -> i32 ---
	// Creates a font from the content of a .ttf / .otf file.
	// The buffer is copied. Glyphs are rasterized on demand, at the exact
	// pixel size needed by each window scale, into a shared atlas.
	// Returns: R_INVALID_ID on failure.
	Rfont_create :: proc(data: cstring, data_size: uint) -> Rid ---
	Rfont_create_from_file :: proc(path: cstring) -> Rid ---
	Rfont_destroy :: proc(font: Rid) ---
	Rfont_is_valid :: proc(font: Rid) -> i32 ---
	// Returns: The font embedded in the library (Roboto, Latin and Latin Extended-A).
	Rfont_get_default :: proc() -> Rid ---
	// Returns: The fixed-width font embedded in the library (Roboto Mono, same
	// characters), used by default by the text editors. In XML: Font="monospace".
	Rfont_get_monospace :: proc() -> Rid ---
	// Font used for the characters missing from `font` (symbols, icons,
	// other scripts). Fallbacks can be chained.
	Rfont_set_fallback :: proc(font: Rid, fallback: Rid) ---
	// Gives a font the font to use for one of its styles: the bold
	// file of the same family, its italic file. Formatted text (Markdown
	// labels, the Markdown of the text editor) uses them.
	//
	// A font that was given none still has the three styles: Raven draws
	// its glyphs thicker for bold and slanted for italic, keeping their
	// advances, so that text does not move when its style changes.
	// variant R_INVALID_ID goes back to the style Raven derives.
	Rfont_set_variant :: proc(font: Rid, style: RFontStyle, variant: Rid) ---
	// Returns: The font drawn for a style of `font`: the one given with
	// Rfont_set_variant, else the one Raven derives from it (made on first
	// use; it lives as long as `font` and cannot be deleted by itself).
	// Usable wherever a font is: Rlabel_set_font, Rdraw_text...
	Rfont_get_variant :: proc(font: Rid, style: RFontStyle) -> Rid ---
	// Measures a UTF-8 string, in logical units, at scale 1.
	// font R_INVALID_ID uses the theme font.
	// size Font size; 0 uses R_METRIC_FONT_SIZE.
	Rfont_measure_text :: proc(font: Rid, size: f32, text: cstring, width: ^f32, height: ^f32) ---
	// Creates a texture from the content of an image file (PNG, JPEG,
	// BMP, TGA, GIF, PSD, SVG). Decoding happens on the calling thread.
	// Returns: R_INVALID_ID on failure.
	Rtexture_create :: proc(data: cstring, data_size: uint) -> Rid ---
	// Same as Rtexture_create, decoded on a worker thread. The handle
	// is usable at once; widgets showing it draw nothing until it is ready.
	// The buffer is copied.
	Rtexture_create_async :: proc(data: cstring, data_size: uint) -> Rid ---
	// Draws an SVG image (an icon) into a texture, at `scale` pixels per
	// unit of the SVG. The widgets show it at the size the SVG gives, whatever
	// the scale (Rtexture_get_density).
	//
	// 0 or less: at the size it is shown. It is first drawn at a scale of 2;
	// once painted, it is drawn again (on a worker thread) with as many pixels
	// as the screen shows it with: the scale of the monitor of the window, an
	// Image that stretches it, a zoom. It follows when the window moves to a
	// screen of another density or the image grows, and comes down when it is
	// shown much smaller (sides of at most 4096 pixels). Its width and height
	// in pixels (Rtexture_get_size) change then; its size in logical units
	// does not. Rtexture_create, Rtexture_create_from_file and the asynchronous
	// ones read SVG files this way. Paths, fills, strokes and gradients are
	// drawn; text, filters and masks are not.
	Rtexture_create_from_svg :: proc(svg: cstring, svg_size: uint, scale: f32) -> Rid ---
	// Pixels of the texture per logical unit: the scale of an SVG, 1 otherwise.
	Rtexture_get_density :: proc(texture: Rid) -> f32 ---
	Rtexture_create_from_file :: proc(path: cstring) -> Rid ---
	// Reads and decodes the file on a worker thread.
	Rtexture_create_from_file_async :: proc(path: cstring) -> Rid ---
	// Creates a texture from tightly packed RGBA8 pixels (straight alpha). Its filter is R_FILTER_LINEAR.
	Rtexture_create_from_pixels :: proc(rgba: [^]u8, width: i32, height: i32) -> Rid ---
	// Replaces the pixels of a texture. The size may change. Use it to
	// show frames produced on the CPU or read back from another renderer.
	Rtexture_update :: proc(texture: Rid, rgba: [^]u8, width: i32, height: i32) ---
	// Decodes an image file held in memory (PNG, JPEG, BMP, TGA) into
	// tightly packed RGBA8 pixels, without making a texture: for pictures
	// that arrive one after the other and go to Rtexture_update. Thread-safe,
	// so that the decoding can be done away from the UI thread.
	// Returns: The pixels, to release with Rpixels_free; NULL when the data
	// is not an image Raven can read.
	Rimage_decode :: proc(data: rawptr, data_size: uint, width: ^i32, height: ^i32) -> [^]u8 ---
	// Releases pixels returned by Raven (Rimage_decode, Rwindow_capture). Thread-safe.
	Rpixels_free :: proc(pixels: [^]u8) ---
	// Returns: R_TRUE once an asynchronous texture is decoded (or failed).
	Rtexture_is_ready :: proc(texture: Rid) -> i32 ---
	// Blocks until an asynchronous texture is ready.
	Rtexture_wait :: proc(texture: Rid) ---
	Rtexture_destroy :: proc(texture: Rid) ---
	Rtexture_is_valid :: proc(texture: Rid) -> i32 ---
	Rtexture_get_size :: proc(texture: Rid, width: ^i32, height: ^i32) ---
	// How the texture is smoothed when it is scaled. The textures made
	// from image files start with R_FILTER_MIPMAP, the others with
	// R_FILTER_LINEAR.
	Rtexture_set_filter :: proc(texture: Rid, filter: RFilterType) ---
	// Creates a widget. It is not shown until it is added to a parent
	// that belongs to a window (Rwidget_add_child, Rwindow_set_content).
	Rwidget_create :: proc(type: RWidgetType) -> Rid ---
	// Deletes a widget and all its descendants.
	Rwidget_destroy :: proc(widget: Rid) ---
	Rwidget_is_valid :: proc(widget: Rid) -> i32 ---
	Rwidget_get_type :: proc(widget: Rid) -> RWidgetType ---
	// Appends a child to a panel. The parent takes ownership.
	// A widget that already has a parent is moved.
	// Returns: `child`, so calls can be nested:
	//         Rid ok = Rwidget_add_child(row, Rbutton_create("OK"));
	Rwidget_add_child :: proc(parent: Rid, child: Rid) -> Rid ---
	Rwidget_insert_child :: proc(parent: Rid, child: Rid, index: i32) -> Rid ---
	// Detaches a child without deleting it. The caller owns it again.
	Rwidget_remove_child :: proc(parent: Rid, child: Rid) ---
	// Deletes every child of a panel.
	Rwidget_clear_children :: proc(parent: Rid) ---
	Rwidget_get_parent :: proc(widget: Rid) -> Rid ---
	Rwidget_get_child_count :: proc(widget: Rid) -> i32 ---
	Rwidget_get_child :: proc(widget: Rid, index: i32) -> Rid ---
	// Returns: The window showing the widget, or R_INVALID_ID.
	Rwidget_get_window :: proc(widget: Rid) -> Rid ---
	Rwidget_set_name :: proc(widget: Rid, name: cstring) ---
	Rwidget_get_name :: proc(widget: Rid) -> cstring ---
	// Finds a widget by name.
	// scope A window or a widget; the search covers its whole subtree.
	// Returns: R_INVALID_ID when no widget has that name.
	Rwidget_find :: proc(scope: Rid, name: cstring) -> Rid ---
	// Space kept around the child, inside its slot. All panels.
	Rslot_set_padding :: proc(child: Rid, left: f32, top: f32, right: f32, bottom: f32) ---
	// Alignment of the child inside its slot. Default: fill. All panels.
	Rslot_set_halign :: proc(child: Rid, align: RHAlign) ---
	Rslot_set_valign :: proc(child: Rid, align: RVAlign) ---
	// The slot takes the desired size of the child along the panel
	// direction (Slate AutoWidth / AutoHeight). This is the default.
	// VBox, HBox.
	Rslot_set_auto :: proc(child: Rid) ---
	// The slot shares the space left by the auto slots, in proportion
	// to `coefficient` (Slate FillWidth / FillHeight). VBox, HBox.
	// In a Splitter, the coefficient is the relative size of the pane.
	Rslot_set_fill :: proc(child: Rid, coefficient: f32) ---
	// Upper bound of the slot along the panel direction. 0 = none. VBox, HBox.
	Rslot_set_max_size :: proc(child: Rid, max_size: f32) ---
	// Cell of the child. Grid, UniformGrid.
	Rslot_set_grid :: proc(child: Rid, column: i32, row: i32) ---
	Rslot_set_grid_span :: proc(child: Rid, column_span: i32, row_span: i32) ---
	// Canvas placement. Anchors are fractions of the canvas size (0..1).
	//   - anchors equal on an axis: the child is at anchor + position, with
	//     the slot size (or its desired size when the slot size is <= 0);
	//   - anchors different on an axis: the child stretches between them,
	//     position and size are then the near and far margins.
	// Alignment is the pivot of the child (0,0 = top-left, 0.5,0.5 = center).
	Rslot_set_anchors :: proc(child: Rid, min_x: f32, min_y: f32, max_x: f32, max_y: f32) ---
	Rslot_set_position :: proc(child: Rid, x: f32, y: f32) ---
	Rslot_set_size :: proc(child: Rid, width: f32, height: f32) ---
	Rslot_set_alignment :: proc(child: Rid, x: f32, y: f32) ---
	// Title of the tab showing the child. TabView.
	Rslot_set_title :: proc(child: Rid, title: cstring) ---
	Rwidget_set_visibility :: proc(widget: Rid, visibility: RVisibility) ---
	Rwidget_get_visibility :: proc(widget: Rid) -> RVisibility ---
	// A disabled widget ignores input and is drawn dimmed, with its descendants.
	Rwidget_set_enabled :: proc(widget: Rid, enabled: i32) ---
	Rwidget_is_enabled :: proc(widget: Rid) -> i32 ---
	// R_FALSE lets mouse events pass through the widget (not its children).
	Rwidget_set_hit_testable :: proc(widget: Rid, hit_testable: i32) ---
	// Opacity multiplier applied to the widget and its descendants (0..1).
	Rwidget_set_opacity :: proc(widget: Rid, opacity: f32) ---
	// Gives the widget a transition that plays each time it appears:
	// when its window is first drawn, when it is added to a window that is
	// shown, and when it becomes visible again (Rwidget_set_visibility, a
	// tab or a switcher showing it). In XML: Appear="FlyUp" AppearDelay="0.1".
	// delay_seconds Time before it starts. Give growing delays to the
	// items of a group to make them arrive one after the other.
	Rwidget_set_transition :: proc(widget: Rid, transition: RTransition, delay_seconds: f32) ---
	// Plays a transition once, now, whatever the widget was given with Rwidget_set_transition.
	Rwidget_play_transition :: proc(widget: Rid, transition: RTransition, delay_seconds: f32) ---
	// R_TRUE clips the descendants to the bounds of the widget.
	Rwidget_set_clipping :: proc(widget: Rid, clip: i32) ---
	// Text shown when the cursor rests on the widget. NULL removes it.
	Rwidget_set_tooltip :: proc(widget: Rid, text: cstring) ---
	// Returns: The tooltip text of the widget, "" when it has none.
	Rwidget_get_tooltip :: proc(widget: Rid) -> cstring ---
	Rwidget_set_cursor :: proc(widget: Rid, cursor: RCursor) ---
	// Size constraints, in logical units. They apply to the desired size the
	// widget reports to its parent. A negative value removes the constraint
	// on that axis.
	Rwidget_set_size :: proc(widget: Rid, width: f32, height: f32) ---
	Rwidget_set_min_size :: proc(widget: Rid, width: f32, height: f32) ---
	Rwidget_set_max_size :: proc(widget: Rid, width: f32, height: f32) ---
	// Bounds of the widget in its window, in logical units, after the last layout.
	Rwidget_get_rect :: proc(widget: Rid, x: ^f32, y: ^f32, width: ^f32, height: ^f32) ---
	Rwidget_get_desired_size :: proc(widget: Rid, width: ^f32, height: ^f32) ---
	Rwidget_is_hovered :: proc(widget: Rid) -> i32 ---
	Rwidget_set_focusable :: proc(widget: Rid, focusable: i32) ---
	// Gives the keyboard focus to the widget.
	Rwidget_focus :: proc(widget: Rid) ---
	Rwidget_is_focused :: proc(widget: Rid) -> i32 ---
	Rwidget_set_user_handle :: proc(widget: Rid, handle: rawptr) ---
	Rwidget_get_user_handle :: proc(widget: Rid) -> rawptr ---
	Rwidget_set_destroy_callback :: proc(widget: Rid, callback: RWidgetDestroyCallback, user_data: rawptr) ---
	Rwidget_set_event_callback :: proc(widget: Rid, callback: RWidgetEventCallback, user_data: rawptr) ---
	// Name of the action fired by the main event of the widget: button
	// click, value change, text commit, item activation (see Raction_bind).
	Rwidget_set_action :: proc(widget: Rid, action: cstring) ---
	Rwidget_get_action :: proc(widget: Rid) -> cstring ---
	// Makes the widget a target for what is dragged from other
	// applications: files from the file manager, text. It is highlighted
	// while a drop on it would be accepted. NULL removes the callback.
	Rwidget_set_drop_callback :: proc(widget: Rid, callback: RDropCallback, user_data: rawptr) ---
	// In a window created with R_WINDOW_CUSTOM_HEADER: dragging the
	// widget moves the window and a double click maximizes it, as a title bar
	// does. The buttons, fields and other controls inside it keep working.
	// In XML: WindowDrag="true".
	Rwidget_set_window_drag :: proc(widget: Rid, enabled: i32) ---
	// Deepest widget of a subtree under a point of its window, or R_INVALID_ID.
	Rwidget_hit_test :: proc(root: Rid, x: f32, y: f32) -> Rid ---
	// Deepest visible widget of a subtree under a point of its window,
	// whether it reacts to the pointer or not: a label, a panel, a disabled
	// button. What a design tool selects with a click.
	Rwidget_pick :: proc(root: Rid, x: f32, y: f32) -> Rid ---
	// Menu opened by a right click on the widget. The widget does not own it.
	Rwidget_set_context_menu :: proc(widget: Rid, menu: Rid) ---
	// Asks for the widget to be laid out and painted again.
	Rwidget_invalidate :: proc(widget: Rid) ---
	// Overrides a theme color for one widget. R_COLOR_TEXT is also
	// inherited by its descendants.
	Rwidget_set_color :: proc(widget: Rid, role: RColorRole, r: f32, g: f32, b: f32, a: f32) ---
	Rwidget_clear_color :: proc(widget: Rid, role: RColorRole) ---
	// The color the widget is drawn with for a role: its own, else
	// the one of the styles of its theme that apply to it, else the one of
	// its theme. What a custom widget should draw itself with.
	Rwidget_get_color :: proc(widget: Rid, role: RColorRole, r: ^f32, g: ^f32, b: ^f32, a: ^f32) ---
	// Gives one widget a material of its own, whatever the theme:
	// a solid button in a glass window, a panel of clear glass in a flat one.
	// A widget that draws no surface (a box, a label) gets one of this
	// material behind it: any widget can become a panel of glass.
	// In XML: Material="Glass | FrostedGlass | TintedGlass | Solid".
	// material R_MATERIAL_DEFAULT follows the theme again.
	Rwidget_set_material :: proc(widget: Rid, material: RMaterial) ---
	// Returns: The material the widget is drawn with: its own, or the one of the theme.
	Rwidget_get_material :: proc(widget: Rid) -> RMaterial ---
	// Overrides a theme metric for one widget.
	Rwidget_set_metric :: proc(widget: Rid, metric: RMetric, value: f32) ---
	Rwidget_clear_metric :: proc(widget: Rid, metric: RMetric) ---
	// Returns: The metric the widget uses: its own, else the one of a style, else the one of its theme.
	Rwidget_get_metric :: proc(widget: Rid, metric: RMetric) -> f32 ---
	// Classes of the widget, separated by spaces ("danger large"):
	// the styles of the theme made for these classes apply to it
	// (Rtheme_set_style_color, <Style Class="danger" .../> in a theme file).
	// In XML: Class="...". NULL or "" removes them.
	Rwidget_set_class :: proc(widget: Rid, classes: cstring) ---
	// Returns: The classes, "" when it has none. Valid until they change.
	Rwidget_get_class :: proc(widget: Rid) -> cstring ---
	Rwidget_add_class :: proc(widget: Rid, style_class: cstring) ---
	Rwidget_remove_class :: proc(widget: Rid, style_class: cstring) ---
	Rwidget_has_class :: proc(widget: Rid, style_class: cstring) -> i32 ---
	// Gives the widget and everything inside it a theme of their own,
	// by name ("Dark", "GlassLight", or a theme of Rtheme_load): a dark side
	// bar in a light window, a preview of another theme. Menus and lists
	// opened from inside follow. In XML: Theme="...".
	// name NULL or "" follows the parent (the application theme) again.
	// Returns: R_FALSE, after reporting an error, when no theme has that name.
	Rwidget_set_theme :: proc(widget: Rid, name: cstring) -> i32 ---
	// Returns: Name of the theme the widget is drawn with.
	Rwidget_get_theme :: proc(widget: Rid) -> cstring ---
	// Gives the widget and everything inside it a font, for the texts
	// that have none of their own (a label given Rlabel_set_font keeps its
	// own): a tool whose panels wear one font while the interface it shows
	// keeps the font of the application. Menus and lists opened from inside
	// follow.
	// font R_INVALID_ID: the font of the parent (in the end, the one of the theme, Rtheme_set_font) again.
	Rwidget_set_font :: proc(widget: Rid, font: Rid) ---
	// Returns: The font given to the widget itself, or R_INVALID_ID.
	Rwidget_get_font :: proc(widget: Rid) -> Rid ---
	// Gap inserted between neighbouring children.
	Rpanel_set_spacing :: proc(panel: Rid, spacing: f32) ---
	Rpanel_get_spacing :: proc(panel: Rid) -> f32 ---
	// Lets a column / row grow to take a share of the free space
	// (Slate FillColumn / FillRow). 0 sizes it to its content (default).
	Rgrid_set_column_fill :: proc(grid: Rid, column: i32, coefficient: f32) ---
	Rgrid_set_row_fill :: proc(grid: Rid, row: i32, coefficient: f32) ---
	// Places children automatically, left to right then top to bottom,
	// in the given number of columns. 0 uses the grid cell of each slot (default).
	Runiform_grid_set_columns :: proc(grid: Rid, columns: i32) ---
	Runiform_grid_set_min_cell_size :: proc(grid: Rid, width: f32, height: f32) ---
	// Space between the outline and the child.
	Rborder_set_padding :: proc(border: Rid, left: f32, top: f32, right: f32, bottom: f32) ---
	// Background color (R_COLOR_PANEL of this widget). Alpha 0 draws no background.
	Rborder_set_background :: proc(border: Rid, r: f32, g: f32, b: f32, a: f32) ---
	// Outline width and color. Width 0 draws no outline (default).
	Rborder_set_outline :: proc(border: Rid, width: f32, r: f32, g: f32, b: f32, a: f32) ---
	Rborder_set_corner_radius :: proc(border: Rid, top_left: f32, top_right: f32, bottom_right: f32, bottom_left: f32) ---
	// Soft drop shadow behind the border. Blur 0 disables it (default).
	Rborder_set_shadow :: proc(border: Rid, blur: f32, offset_x: f32, offset_y: f32) ---
	// What the border is made of. R_MATERIAL_GLASS turns it into a
	// panel of glass tinted by its background color (the alpha of the color
	// is the strength of the tint): floating tool bars, side bars, cards.
	Rborder_set_material :: proc(border: Rid, material: RMaterial) ---
	// Keeps the child at a fixed width / height ratio, as large as fits.
	// 0 disables it (default). The fixed, minimum and maximum sizes are set
	// with Rwidget_set_size, Rwidget_set_min_size and Rwidget_set_max_size.
	Rsize_box_set_aspect_ratio :: proc(size_box: Rid, ratio: f32) ---
	Rspacer_set_size :: proc(spacer: Rid, width: f32, height: f32) ---
	// Direction in which children are stacked and scrolled. Default: R_VERTICAL.
	Rscroll_box_set_orientation :: proc(scroll_box: Rid, orientation: ROrientation) ---
	// R_TRUE also scrolls across the stacking direction when the content is too large.
	Rscroll_box_set_scroll_both :: proc(scroll_box: Rid, enabled: i32) ---
	Rscroll_box_set_offset :: proc(scroll_box: Rid, x: f32, y: f32) ---
	Rscroll_box_get_offset :: proc(scroll_box: Rid, x: ^f32, y: ^f32) ---
	Rscroll_box_scroll_to_end :: proc(scroll_box: Rid) ---
	// Scrolls every ScrollBox above the widget so that the widget is visible.
	Rwidget_scroll_into_view :: proc(widget: Rid) ---
	// R_HORIZONTAL puts the panes side by side (default), R_VERTICAL stacks them.
	Rsplitter_set_orientation :: proc(splitter: Rid, orientation: ROrientation) ---
	Rsplitter_set_min_pane_size :: proc(splitter: Rid, size: f32) ---
	// Creates a panel that shows what is behind it out of focus,
	// with one child over it. In XML:
	//
	//   <BackgroundBlur Style="Frosted" Radius="24" CornerRadius="18" Padding="16">
	//       <Label>Over a blurred view of the window</Label>
	//   </BackgroundBlur>
	//
	// It works in any theme and with any material: it is not the glass of
	// the glass themes (which bends and reflects), but the looks of
	// RBlurStyle. Unlike the opacity of a widget, it does not let what is
	// behind show sharp.
	Rbackground_blur_create :: proc() -> Rid ---
	Rbackground_blur_set_style :: proc(blur: Rid, style: RBlurStyle) ---
	Rbackground_blur_get_style :: proc(blur: Rid) -> RBlurStyle ---
	// How far what is behind goes out of focus, in logical units:
	// 4 softens, 20 hides the details, 60 leaves only colors. A negative
	// value (the default) uses R_METRIC_BLUR_RADIUS of the theme.
	Rbackground_blur_set_radius :: proc(blur: Rid, radius: f32) ---
	Rbackground_blur_get_radius :: proc(blur: Rid) -> f32 ---
	// Color laid over the blur; its alpha is how much of it. Without
	// one, R_BLUR_PLAIN has none and the other styles take theirs from the
	// theme (R_COLOR_PANEL).
	Rbackground_blur_set_tint :: proc(blur: Rid, r: f32, g: f32, b: f32, a: f32) ---
	// Removes the tint given with Rbackground_blur_set_tint.
	Rbackground_blur_clear_tint :: proc(blur: Rid) ---
	// Saturation of what is seen through: 1 = as it is, 0 = gray, 1.5 = vivid. Negative = the one of the style.
	Rbackground_blur_set_saturation :: proc(blur: Rid, saturation: f32) ---
	// Fine grain over the blur, 0 = none .. 1. Negative = the one of the style.
	Rbackground_blur_set_grain :: proc(blur: Rid, grain: f32) ---
	Rbackground_blur_set_corner_radius :: proc(blur: Rid, top_left: f32, top_right: f32, bottom_right: f32, bottom_left: f32) ---
	// Space between the edge and the child.
	Rbackground_blur_set_padding :: proc(blur: Rid, left: f32, top: f32, right: f32, bottom: f32) ---
	// Soft shadow under the panel; blur 0 = none (default).
	Rbackground_blur_set_shadow :: proc(blur: Rid, shadow_blur: f32) ---
	Rswitcher_set_index :: proc(switcher: Rid, index: i32) ---
	Rswitcher_get_index :: proc(switcher: Rid) -> i32 ---
	// Adds a page. Same as Rwidget_add_child followed by Rslot_set_title.
	// Returns: The index of the new tab, or -1.
	Rtab_view_add_tab :: proc(tab_view: Rid, title: cstring, content: Rid) -> i32 ---
	Rtab_view_set_active :: proc(tab_view: Rid, index: i32) ---
	Rtab_view_get_active :: proc(tab_view: Rid) -> i32 ---
	Rtab_view_set_changed_callback :: proc(tab_view: Rid, callback: RTabChangedCallback, user_data: rawptr) ---
	// Documents mode: the major tabs of an application (the level,
	// then one tab per open asset, as in Unreal): tabs on the left with an
	// icon and a close button (a middle click closes too), shrinking when
	// they do not fit, no slide between pages. XML: Documents="true".
	Rtab_view_set_documents :: proc(tab_view: Rid, documents: i32) ---
	Rtab_view_get_tab_count :: proc(tab_view: Rid) -> i32 ---
	// The page of a tab, R_INVALID_ID if none.
	Rtab_view_get_tab :: proc(tab_view: Rid, index: i32) -> Rid ---
	// Index of the tab of a page, -1 if none.
	Rtab_view_find_tab :: proc(tab_view: Rid, content: Rid) -> i32 ---
	Rtab_view_set_tab_title :: proc(tab_view: Rid, content: Rid, title: cstring) ---
	// Icon before the title (documents). R_INVALID_ID removes it; tint: a mask in the text color.
	Rtab_view_set_tab_icon :: proc(tab_view: Rid, content: Rid, texture: Rid, u0: f32, v0: f32, u1: f32, v1: f32, tint: i32) ---
	// R_FALSE: no close button (documents ; the level tab). Tabs can be closed by default.
	Rtab_view_set_tab_closable :: proc(tab_view: Rid, content: Rid, closable: i32) ---
	// Shows the tab of a page (the changed callback is called).
	Rtab_view_show_tab :: proc(tab_view: Rid, content: Rid) ---
	Rtab_view_set_close_callback :: proc(tab_view: Rid, callback: RTabCloseCallback, user_data: rawptr) ---
	Rexpander_set_title :: proc(expander: Rid, title: cstring) ---
	Rexpander_get_title :: proc(expander: Rid) -> cstring ---
	Rexpander_set_expanded :: proc(expander: Rid, expanded: i32) ---
	Rexpander_is_expanded :: proc(expander: Rid) -> i32 ---
	Rexpander_set_changed_callback :: proc(expander: Rid, callback: RExpanderChangedCallback, user_data: rawptr) ---
	// Direction of every window: R_DIRECTION_AUTO (the default), _LTR or _RTL.
	Rset_layout_direction :: proc(direction: RDirection) ---
	// The direction the windows use now: R_DIRECTION_LTR or _RTL.
	Rget_layout_direction :: proc() -> RDirection ---
	// Direction of a part of a window: R_DIRECTION_INHERIT, _LTR or _RTL.
	Rwidget_set_direction :: proc(widget: Rid, direction: RDirection) ---
	// The direction the widget is laid out in: R_DIRECTION_LTR or _RTL.
	Rwidget_get_direction :: proc(widget: Rid) -> RDirection ---
	// Shortcut for Rwidget_create(R_WIDGET_LABEL) + Rlabel_set_text.
	Rlabel_create :: proc(text: cstring) -> Rid ---
	// Sets the text. '\n' starts a new line.
	Rlabel_set_text :: proc(label: Rid, text: cstring) ---
	Rlabel_get_text :: proc(label: Rid) -> cstring ---
	// R_INVALID_ID uses the theme font.
	Rlabel_set_font :: proc(label: Rid, font: Rid) ---
	// Font size in logical units. 0 uses R_METRIC_FONT_SIZE.
	Rlabel_set_text_size :: proc(label: Rid, size: f32) ---
	Rlabel_set_text_align :: proc(label: Rid, align: RTextAlign) ---
	// R_TRUE wraps the text on the width given by the parent.
	Rlabel_set_wrap :: proc(label: Rid, wrap: i32) ---
	// Text color (R_COLOR_TEXT of this widget).
	Rlabel_set_color :: proc(label: Rid, r: f32, g: f32, b: f32, a: f32) ---
	// R_TRUE reads the text as Markdown and shows it formatted
	// (Markdown="true" in XML):
	//
	//   # titles, **strong**, *emphasis*, `code`, ~~struck~~, [links](url),
	//   lists and lists in lists, - [x] task lists, > quotes, --- rules,
	//   | tables |, images alone on their line, and ``` blocks of code with
	//   the colors of their language (```c, ```python...: the languages of
	//   the text editor, raven_text_editor.h).
	//
	// The text wraps on the width of the label unless Rlabel_set_wrap said
	// otherwise; put the label in a scroll box for a long document. Strong
	// and emphasized text use the bold and italic of the font
	// (Rfont_set_variant), which Raven derives from the font itself when it
	// was given none. The colors come from the theme: R_COLOR_LINK,
	// R_COLOR_CODE, R_COLOR_SEPARATOR, R_COLOR_ACCENT.
	Rlabel_set_markdown :: proc(label: Rid, enabled: i32) ---
	Rlabel_get_markdown :: proc(label: Rid) -> i32 ---
	Rlabel_set_link_callback :: proc(label: Rid, callback: RLabelLinkCallback, user_data: rawptr) ---
	// The text of a Markdown label without its marks, as a reader sees it. Valid until the next call.
	Rlabel_get_plain_text :: proc(label: Rid) -> cstring ---
	// R_TRUE lets the user select the text of the label with the
	// mouse, as on a web page (Selectable="true" in XML): drag to select, a
	// double click selects a word, a triple click a paragraph, Shift + click
	// extends the selection; then Ctrl+C (Cmd+C) copies it and Ctrl+A selects
	// everything. A click elsewhere takes the selection away. Plain and
	// Markdown labels alike: a copy of a Markdown label is its text without
	// the marks, and its links are still clicked, not selected. The label
	// takes the mouse (what holds it no longer receives the clicks on it) and
	// the keyboard after a click; Tab passes it by. Default R_FALSE.
	Rlabel_set_selectable :: proc(label: Rid, selectable: i32) ---
	Rlabel_get_selectable :: proc(label: Rid) -> i32 ---
	// The text selected, as Ctrl+C copies it ("" when nothing is). Valid until the next call.
	Rlabel_get_selected_text :: proc(label: Rid) -> cstring ---
	// Selects the whole text of a selectable label.
	Rlabel_select_all :: proc(label: Rid) ---
	Rlabel_clear_selection :: proc(label: Rid) ---
	// Shortcut for Rwidget_create(R_WIDGET_BUTTON) + Rbutton_set_text.
	Rbutton_create :: proc(text: cstring) -> Rid ---
	// Sets the content of the button to a label. A button can instead
	// hold any single child widget (Rwidget_add_child), e.g. an HBox with an icon.
	Rbutton_set_text :: proc(button: Rid, text: cstring) ---
	Rbutton_set_style :: proc(button: Rid, style: RButtonStyle) ---
	Rbutton_set_clicked_callback :: proc(button: Rid, callback: RButtonClickedCallback, user_data: rawptr) ---
	Rcheckbox_create :: proc(text: cstring) -> Rid ---
	Rcheckbox_set_text :: proc(checkbox: Rid, text: cstring) ---
	Rcheckbox_get_text :: proc(checkbox: Rid) -> cstring ---
	Rcheckbox_set_checked :: proc(checkbox: Rid, checked: i32) ---
	Rcheckbox_is_checked :: proc(checkbox: Rid) -> i32 ---
	Rcheckbox_set_style :: proc(checkbox: Rid, style: RCheckboxStyle) ---
	Rcheckbox_set_changed_callback :: proc(checkbox: Rid, callback: RCheckboxChangedCallback, user_data: rawptr) ---
	Rslider_set_range :: proc(slider: Rid, minimum: f32, maximum: f32) ---
	Rslider_set_value :: proc(slider: Rid, value: f32) ---
	Rslider_get_value :: proc(slider: Rid) -> f32 ---
	// The range given with Rslider_set_range and the step (NULL for what is not wanted).
	Rslider_get_range :: proc(slider: Rid, minimum: ^f32, maximum: ^f32, step: ^f32) ---
	// Snaps the value to multiples of `step`. 0 = continuous (default).
	Rslider_set_step :: proc(slider: Rid, step: f32) ---
	Rslider_set_orientation :: proc(slider: Rid, orientation: ROrientation) ---
	Rslider_set_changed_callback :: proc(slider: Rid, callback: RSliderChangedCallback, user_data: rawptr) ---
	// Progress between 0 and 1.
	Rprogress_bar_set_value :: proc(progress_bar: Rid, value: f32) ---
	Rprogress_bar_get_value :: proc(progress_bar: Rid) -> f32 ---
	Rimage_set_texture :: proc(image: Rid, texture: Rid) ---
	Rimage_set_tint_color :: proc(image: Rid, r: f32, g: f32, b: f32, a: f32) ---
	// R_TRUE: the image is tinted with the text color around it (white icons that follow the theme). XML: TintText.
	Rimage_set_tint_text :: proc(image: Rid, enabled: i32) ---
	Rimage_set_stretch :: proc(image: Rid, stretch: RStretch) ---
	// Part of the texture to show, in UV coordinates. Default: 0, 0, 1, 1.
	Rimage_set_region :: proc(image: Rid, min_u: f32, min_v: f32, max_u: f32, max_v: f32) ---
	Rimage_set_corner_radius :: proc(image: Rid, radius: f32) ---
	Rtext_input_set_text :: proc(text_input: Rid, text: cstring) ---
	Rtext_input_get_text :: proc(text_input: Rid) -> cstring ---
	// Dimmed text shown while the input is empty.
	Rtext_input_set_placeholder :: proc(text_input: Rid, text: cstring) ---
	Rtext_input_set_password :: proc(text_input: Rid, password: i32) ---
	Rtext_input_is_password :: proc(text_input: Rid) -> i32 ---
	Rtext_input_is_read_only :: proc(text_input: Rid) -> i32 ---
	Rtext_input_set_read_only :: proc(text_input: Rid, read_only: i32) ---
	// Maximum number of characters. 0 = no limit (default).
	Rtext_input_set_max_length :: proc(text_input: Rid, max_length: i32) ---
	Rtext_input_select_all :: proc(text_input: Rid) ---
	// What it holds: picks the on-screen keyboard on mobile. Default: R_TEXT_INPUT_TYPE_TEXT.
	Rtext_input_set_input_type :: proc(text_input: Rid, type: RTextInputType) ---
	Rtext_input_get_input_type :: proc(text_input: Rid) -> RTextInputType ---
	// The return key of the on-screen keyboard (Done, Next, Search...). Default: R_RETURN_KEY_DEFAULT.
	Rtext_input_set_return_key :: proc(text_input: Rid, key: RReturnKey) ---
	Rtext_input_get_return_key :: proc(text_input: Rid) -> RReturnKey ---
	// Called on every edit.
	Rtext_input_set_changed_callback :: proc(text_input: Rid, callback: RTextChangedCallback, user_data: rawptr) ---
	// Called when Enter is pressed. This also fires the action of the widget.
	Rtext_input_set_committed_callback :: proc(text_input: Rid, callback: RTextChangedCallback, user_data: rawptr) ---
	Rspin_box_set_range :: proc(spin_box: Rid, minimum: f64, maximum: f64) ---
	Rspin_box_set_value :: proc(spin_box: Rid, value: f64) ---
	Rspin_box_get_value :: proc(spin_box: Rid) -> f64 ---
	// The range and the step (NULL for what is not wanted).
	Rspin_box_get_range :: proc(spin_box: Rid, minimum: ^f64, maximum: ^f64, step: ^f64) ---
	Rspin_box_set_step :: proc(spin_box: Rid, step: f64) ---
	// Digits shown after the decimal point. Default: 2.
	Rspin_box_set_decimals :: proc(spin_box: Rid, decimals: i32) ---
	// Step buttons in the looks of Raven too: a down button at the left
	// end and an up button at the right end, the value between them (XML
	// `Buttons="True"`). A click steps once; holding a button repeats its step.
	// The platform looks (Windows, GNOME, macOS) always draw their own buttons.
	// Default: R_FALSE (drag to scrub, arrows / wheel to step).
	Rspin_box_set_buttons :: proc(spin_box: Rid, buttons: i32) ---
	Rspin_box_get_buttons :: proc(spin_box: Rid) -> i32 ---
	Rspin_box_set_changed_callback :: proc(spin_box: Rid, callback: RSpinBoxChangedCallback, user_data: rawptr) ---
	// Returns: The index of the new item.
	Rcombo_box_add_item :: proc(combo_box: Rid, text: cstring) -> i32 ---
	Rcombo_box_clear_items :: proc(combo_box: Rid) ---
	Rcombo_box_get_item_count :: proc(combo_box: Rid) -> i32 ---
	Rcombo_box_get_item_text :: proc(combo_box: Rid, index: i32) -> cstring ---
	// Selects an item. -1 selects nothing.
	Rcombo_box_set_selected :: proc(combo_box: Rid, index: i32) ---
	Rcombo_box_get_selected :: proc(combo_box: Rid) -> i32 ---
	Rcombo_box_set_changed_callback :: proc(combo_box: Rid, callback: RComboBoxChangedCallback, user_data: rawptr) ---
	// R_HORIZONTAL draws a horizontal line (default).
	Rseparator_set_orientation :: proc(separator: Rid, orientation: ROrientation) ---
	// Returns: The index of the new item.
	Rlist_view_add_item :: proc(list_view: Rid, text: cstring) -> i32 ---
	Rlist_view_set_item_text :: proc(list_view: Rid, index: i32, text: cstring) ---
	Rlist_view_get_item_text :: proc(list_view: Rid, index: i32) -> cstring ---
	Rlist_view_remove_item :: proc(list_view: Rid, index: i32) ---
	Rlist_view_clear_items :: proc(list_view: Rid) ---
	Rlist_view_get_item_count :: proc(list_view: Rid) -> i32 ---
	Rlist_view_set_virtual_items :: proc(list_view: Rid, count: i32, callback: RListItemTextCallback, user_data: rawptr) ---
	Rlist_view_set_item_color_callback :: proc(list_view: Rid, callback: RListItemColorCallback, user_data: rawptr) ---
	// Selects a row. -1 selects nothing.
	Rlist_view_set_selected :: proc(list_view: Rid, index: i32) ---
	Rlist_view_get_selected :: proc(list_view: Rid) -> i32 ---
	Rlist_view_scroll_to_item :: proc(list_view: Rid, index: i32) ---
	Rlist_view_set_selection_changed_callback :: proc(list_view: Rid, callback: RListEventCallback, user_data: rawptr) ---
	// Called on double click or Enter. This also fires the action of the widget.
	Rlist_view_set_activated_callback :: proc(list_view: Rid, callback: RListEventCallback, user_data: rawptr) ---
	// Adds an item.
	// parent_item Item to add under, or R_INVALID_ID for a top-level item.
	// Returns: The handle of the new item.
	Rtree_view_add_item :: proc(tree_view: Rid, parent_item: Rid, text: cstring) -> Rid ---
	// Deletes an item and its descendants.
	Rtree_item_destroy :: proc(item: Rid) ---
	Rtree_view_clear_items :: proc(tree_view: Rid) ---
	Rtree_item_is_valid :: proc(item: Rid) -> i32 ---
	Rtree_item_set_text :: proc(item: Rid, text: cstring) ---
	Rtree_item_get_text :: proc(item: Rid) -> cstring ---
	Rtree_item_set_expanded :: proc(item: Rid, expanded: i32) ---
	Rtree_item_is_expanded :: proc(item: Rid) -> i32 ---
	Rtree_item_set_user_handle :: proc(item: Rid, handle: rawptr) ---
	Rtree_item_get_user_handle :: proc(item: Rid) -> rawptr ---
	// Top-level items of a tree.
	Rtree_view_get_item_count :: proc(tree_view: Rid) -> i32 ---
	Rtree_view_get_item :: proc(tree_view: Rid, index: i32) -> Rid ---
	// Returns: The parent item, or R_INVALID_ID for a top-level item.
	Rtree_item_get_parent :: proc(item: Rid) -> Rid ---
	Rtree_item_get_child_count :: proc(item: Rid) -> i32 ---
	Rtree_item_get_child :: proc(item: Rid, index: i32) -> Rid ---
	// Selects an item. R_INVALID_ID selects nothing.
	Rtree_view_set_selected_item :: proc(tree_view: Rid, item: Rid) ---
	Rtree_view_get_selected_item :: proc(tree_view: Rid) -> Rid ---
	Rtree_view_set_selection_changed_callback :: proc(tree_view: Rid, callback: RTreeEventCallback, user_data: rawptr) ---
	// Called on double click or Enter. This also fires the action of the widget.
	Rtree_view_set_activated_callback :: proc(tree_view: Rid, callback: RTreeEventCallback, user_data: rawptr) ---
	// Icon drawn before the text of the item (a region u0,v0,u1,v1 of the texture). R_INVALID_ID removes it.
	Rtree_item_set_icon :: proc(item: Rid, texture: Rid, u0: f32, v0: f32, u1: f32, v1: f32) ---
	// Color of the text of the item; alpha 0 goes back to the theme.
	Rtree_item_set_text_color :: proc(item: Rid, r: f32, g: f32, b: f32, a: f32) ---
	// Several items can be selected: Ctrl + click adds or removes one,
	// Shift + click selects a range. The selection callback receives the item
	// clicked last (the primary one, Rtree_view_get_selected_item).
	Rtree_view_set_multi_select :: proc(tree_view: Rid, enabled: i32) ---
	Rtree_item_set_selected :: proc(item: Rid, selected: i32) ---
	Rtree_item_is_selected :: proc(item: Rid) -> i32 ---
	Rtree_view_clear_selection :: proc(tree_view: Rid) ---
	// Selected items, top to bottom.
	Rtree_view_get_selection_count :: proc(tree_view: Rid) -> i32 ---
	Rtree_view_get_selection :: proc(tree_view: Rid, index: i32) -> Rid ---
	Rtree_view_scroll_to_item :: proc(tree_view: Rid, item: Rid) ---
	Rtree_view_set_context_callback :: proc(tree_view: Rid, callback: RTreeContextCallback, user_data: rawptr) ---
	// Items can be dragged: the selection is given to Rdrag_begin with
	// this type and, as data, the handles of the items in decimal separated
	// by spaces. NULL: no drag (default).
	Rtree_view_set_drag_type :: proc(tree_view: Rid, type: cstring) ---
	// Item at a point relative to the tree (drop callbacks give local_x, local_y), or R_INVALID_ID.
	Rtree_view_get_item_at :: proc(tree_view: Rid, local_x: f32, local_y: f32) -> Rid ---
	// Highlights where a drop would go: an item, or the whole tree (`whole_tree`). R_INVALID_ID, 0 removes it.
	Rtree_view_set_drop_highlight :: proc(tree_view: Rid, item: Rid, whole_tree: i32) ---
	// The menu bar of the system. On macOS, the first MenuBar of a
	// window is shown in the bar at the top of the screen while the window is
	// the active one, after the menu of the application (About, Hide, Quit);
	// it then takes no room in the window. Its shortcuts are those of the
	// system: R_MOD_SUPER is Command, R_MOD_CTRL is Control. Elsewhere it is
	// drawn in the window. On by default; R_FALSE keeps it in the window
	// everywhere. In XML: <MenuBar Native="false">.
	Rmenu_bar_set_native :: proc(menu_bar: Rid, native: i32) ---
	// Returns: R_TRUE while the menu bar is shown by the system instead of the window.
	Rmenu_bar_is_native :: proc(menu_bar: Rid) -> i32 ---
	// Creates a menu inside a menu bar or as a sub-menu of another menu.
	Rmenu_add_menu :: proc(menu_bar_or_menu: Rid, title: cstring) -> Rid ---
	Rmenu_set_title :: proc(menu: Rid, title: cstring) ---
	Rmenu_add_item :: proc(menu: Rid, text: cstring) -> Rid ---
	Rmenu_add_separator :: proc(menu: Rid) -> Rid ---
	Rmenu_item_set_text :: proc(item: Rid, text: cstring) ---
	Rmenu_item_get_text :: proc(item: Rid) -> cstring ---
	// Shows a shortcut next to the item and triggers the item when the
	// keys are pressed in the window of its menu bar.
	Rmenu_item_set_shortcut :: proc(item: Rid, key: RKey, modifiers: u32) ---
	// Display-only shortcut hint; copied, never binds a key. Overrides
	// the formatted shortcut without changing its binding. NULL restores the
	// formatted shortcut; an empty string hides it. XML: ShortcutText.
	Rmenu_item_set_shortcut_text :: proc(item: Rid, text: cstring) ---
	Rmenu_item_set_checkable :: proc(item: Rid, checkable: i32) ---
	Rmenu_item_set_checked :: proc(item: Rid, checked: i32) ---
	// Icon drawn before the text of a menu item (after the check mark column).
	// texture R_INVALID_ID removes it. u0, v0, u1, v1 : region of the texture.
	// tint    R_TRUE: the texture is a mask drawn in the text color (monochrome icons).
	Rmenu_item_set_icon :: proc(item: Rid, texture: Rid, u0: f32, v0: f32, u1: f32, v1: f32, tint: i32) ---
	Rmenu_item_is_checked :: proc(item: Rid) -> i32 ---
	Rmenu_item_set_clicked_callback :: proc(item: Rid, callback: RMenuItemClickedCallback, user_data: rawptr) ---
	// Opens a menu as a popup at a position of the window (logical units).
	Rmenu_open :: proc(menu: Rid, window: Rid, x: f32, y: f32) ---
	// Closes every open menu and popup of the window.
	Rwindow_close_popups :: proc(window: Rid) ---
	// Gives the tiles: `count` of them, described by `callback`. The selection is cleared.
	Rtile_view_set_items :: proc(tile_view: Rid, count: i32, callback: RTileItemCallback, user_data: rawptr) ---
	// Draws the tiles again (their description changed, not their number).
	Rtile_view_refresh :: proc(tile_view: Rid) ---
	Rtile_view_get_count :: proc(tile_view: Rid) -> i32 ---
	// Size of a tile (its name included) and of its icon. Default 112 x 112, icon 64.
	Rtile_view_set_tile_size :: proc(tile_view: Rid, width: f32, height: f32, icon_size: f32) ---
	Rtile_view_set_empty_text :: proc(tile_view: Rid, text: cstring) ---
	Rtile_view_set_selected :: proc(tile_view: Rid, index: i32, selected: i32) ---
	Rtile_view_is_selected :: proc(tile_view: Rid, index: i32) -> i32 ---
	Rtile_view_clear_selection :: proc(tile_view: Rid) ---
	Rtile_view_select_only :: proc(tile_view: Rid, index: i32) ---
	// The tile clicked last, or -1.
	Rtile_view_get_current :: proc(tile_view: Rid) -> i32 ---
	Rtile_view_get_selection_count :: proc(tile_view: Rid) -> i32 ---
	// Index of the nth selected tile, in order.
	Rtile_view_get_selection :: proc(tile_view: Rid, nth: i32) -> i32 ---
	Rtile_view_scroll_to :: proc(tile_view: Rid, index: i32) ---
	// Tile at a point relative to the widget (drop callbacks give local_x, local_y), or -1.
	Rtile_view_get_item_at :: proc(tile_view: Rid, local_x: f32, local_y: f32) -> i32 ---
	// Highlights the tile a drop would go to; -1 removes it.
	Rtile_view_set_drop_highlight :: proc(tile_view: Rid, index: i32) ---
	// Tiles can be dragged: the selected tiles are given to Rdrag_begin
	// with this type and, as data, their indices separated by spaces.
	Rtile_view_set_drag_type :: proc(tile_view: Rid, type: cstring) ---
	Rtile_view_set_drag_data_callback :: proc(tile_view: Rid, callback: RTileDragDataCallback, user_data: rawptr) ---
	Rtile_view_set_selection_changed_callback :: proc(tile_view: Rid, callback: RTileEventCallback, user_data: rawptr) ---
	Rtile_view_set_activated_callback :: proc(tile_view: Rid, callback: RTileEventCallback, user_data: rawptr) ---
	Rtile_view_set_context_callback :: proc(tile_view: Rid, callback: RTileContextCallback, user_data: rawptr) ---
	// Adds a column.
	// width Width in logical units; 0 or less gives the default (120).
	// Returns: The index of the column. The stored rows get an empty cell in it.
	Rtable_add_column :: proc(table: Rid, header: cstring, width: f32) -> i32 ---
	Rtable_get_column_count :: proc(table: Rid) -> i32 ---
	// Removes the columns and the stored rows.
	Rtable_clear_columns :: proc(table: Rid) ---
	Rtable_set_column_header :: proc(table: Rid, column: i32, header: cstring) ---
	Rtable_get_column_header :: proc(table: Rid, column: i32) -> cstring ---
	Rtable_set_column_width :: proc(table: Rid, column: i32, width: f32) ---
	// The width the column has on screen (a filling column takes more than its width).
	Rtable_get_column_width :: proc(table: Rid, column: i32) -> f32 ---
	// > 0: the column takes this share of the room the others leave; 0: its width only.
	Rtable_set_column_fill :: proc(table: Rid, column: i32, fill: f32) ---
	// R_FALSE: a click on the header does not sort (default R_TRUE).
	Rtable_set_column_sortable :: proc(table: Rid, column: i32, sortable: i32) ---
	// Where the texts sit in the cells: 0 left, 0.5 center, 1 right.
	Rtable_set_column_align :: proc(table: Rid, column: i32, align: f32) ---
	// Adds a row.
	// cells The texts of the cells, separated by '|' ("Ada|36|London"),
	//              or NULL for an empty row; Rtable_set_cell gives a text holding a '|'.
	// Returns: The index of the row.
	Rtable_add_row :: proc(table: Rid, cells: cstring) -> i32 ---
	Rtable_set_cell :: proc(table: Rid, row: i32, column: i32, text: cstring) ---
	Rtable_get_cell :: proc(table: Rid, row: i32, column: i32) -> cstring ---
	Rtable_remove_row :: proc(table: Rid, row: i32) ---
	Rtable_clear_rows :: proc(table: Rid) ---
	Rtable_get_row_count :: proc(table: Rid) -> i32 ---
	Rtable_set_virtual_rows :: proc(table: Rid, count: i32, callback: RTableCellTextCallback, user_data: rawptr) ---
	Rtable_set_cell_color_callback :: proc(table: Rid, callback: RTableCellColorCallback, user_data: rawptr) ---
	// Selects a row. -1 selects nothing.
	Rtable_set_selected :: proc(table: Rid, row: i32) ---
	Rtable_get_selected :: proc(table: Rid) -> i32 ---
	Rtable_scroll_to_row :: proc(table: Rid, row: i32) ---
	// Sorts by a column (stored rows are moved); -1 shows no sorted column.
	Rtable_sort :: proc(table: Rid, column: i32, descending: i32) ---
	Rtable_get_sort_column :: proc(table: Rid) -> i32 ---
	Rtable_is_sort_descending :: proc(table: Rid) -> i32 ---
	Rtable_set_header_visible :: proc(table: Rid, visible: i32) ---
	Rtable_set_grid_lines :: proc(table: Rid, visible: i32) ---
	// Every other row a little darker.
	Rtable_set_striped :: proc(table: Rid, striped: i32) ---
	Rtable_set_selection_changed_callback :: proc(table: Rid, callback: RTableEventCallback, user_data: rawptr) ---
	// Called on double click or Enter. This also fires the action of the widget.
	Rtable_set_activated_callback :: proc(table: Rid, callback: RTableEventCallback, user_data: rawptr) ---
	Rtable_set_sort_callback :: proc(table: Rid, callback: RTableSortCallback, user_data: rawptr) ---
	// Sets the date (today when the picker is created). An invalid date is refused.
	Rdate_picker_set_date :: proc(date_picker: Rid, year: i32, month: i32, day: i32) ---
	Rdate_picker_get_date :: proc(date_picker: Rid, year: ^i32, month: ^i32, day: ^i32) ---
	// The earliest and the latest dates that can be chosen; 0 for year leaves that side open.
	Rdate_picker_set_range :: proc(date_picker: Rid, min_year: i32, min_month: i32, min_day: i32, max_year: i32, max_month: i32, max_day: i32) ---
	// 0 = Sunday (default), 1 = Monday...
	Rdate_picker_set_first_day_of_week :: proc(date_picker: Rid, day: i32) ---
	// How the field writes the date: YYYY, MM, M, DD, D, MMMM (the name
	// of the month), MMM (its first three letters), dddd (the name of the day),
	// ddd; anything else is kept. Default "YYYY-MM-DD"; "D MMMM YYYY" gives "3 November 2026".
	Rdate_picker_set_format :: proc(date_picker: Rid, format: cstring) ---
	// The date as the field shows it (valid until the next call).
	Rdate_picker_get_text :: proc(date_picker: Rid) -> cstring ---
	// Opens or closes the calendar.
	Rdate_picker_set_open :: proc(date_picker: Rid, open: i32) ---
	Rdate_picker_is_open :: proc(date_picker: Rid) -> i32 ---
	Rdate_picker_set_changed_callback :: proc(date_picker: Rid, callback: RDateChangedCallback, user_data: rawptr) ---
	Rtime_picker_set_time :: proc(time_picker: Rid, hour: i32, minute: i32, second: i32) ---
	Rtime_picker_get_time :: proc(time_picker: Rid, hour: ^i32, minute: ^i32, second: ^i32) ---
	Rtime_picker_set_seconds_visible :: proc(time_picker: Rid, visible: i32) ---
	// Step of the minutes for the arrows and the wheel (default 1).
	Rtime_picker_set_minute_step :: proc(time_picker: Rid, step: i32) ---
	Rtime_picker_set_changed_callback :: proc(time_picker: Rid, callback: RTimeChangedCallback, user_data: rawptr) ---
	// Shows a message for `seconds` (0 or less: until Rtoast_hide), with the kind and button set now.
	Rtoast_show :: proc(toast: Rid, text: cstring, seconds: f64) ---
	// Same, with a kind of its own (the kind set by Rtoast_set_kind is left as it is).
	Rtoast_show_kind :: proc(toast: Rid, text: cstring, kind: RToastKind, seconds: f64) ---
	// Hides the newest message shown; the next one waiting comes.
	Rtoast_hide :: proc(toast: Rid) ---
	// Hides every message, and forgets those waiting.
	Rtoast_clear :: proc(toast: Rid) ---
	Rtoast_is_shown :: proc(toast: Rid) -> i32 ---
	// Returns: The messages shown and waiting.
	Rtoast_get_count :: proc(toast: Rid) -> i32 ---
	// Returns: The newest message shown (the last one, once hidden; the clicked one in the action callback).
	Rtoast_get_text :: proc(toast: Rid) -> cstring ---
	Rtoast_set_queue :: proc(toast: Rid, queue: RToastQueue) ---
	Rtoast_get_queue :: proc(toast: Rid) -> RToastQueue ---
	// How many messages a R_TOAST_STACK shows at once (default 3).
	Rtoast_set_max_visible :: proc(toast: Rid, count: i32) ---
	// The kind of the next messages (and of the message shown, with R_TOAST_REPLACE).
	Rtoast_set_kind :: proc(toast: Rid, kind: RToastKind) ---
	// A button at the end of the next messages ("Undo"); NULL or "" removes it.
	Rtoast_set_action_text :: proc(toast: Rid, text: cstring) ---
	Rtoast_set_action_callback :: proc(toast: Rid, callback: RToastCallback, user_data: rawptr) ---
	// Replaces the items: their texts separated by '|'.
	Rbreadcrumb_set_items :: proc(breadcrumb: Rid, items: cstring) ---
	// Returns: The index of the new item, at the end.
	Rbreadcrumb_add_item :: proc(breadcrumb: Rid, text: cstring) -> i32 ---
	// Keeps the first `count` items: Rbreadcrumb_truncate(b, index + 1) goes back to the item clicked.
	Rbreadcrumb_truncate :: proc(breadcrumb: Rid, count: i32) ---
	Rbreadcrumb_get_item_count :: proc(breadcrumb: Rid) -> i32 ---
	Rbreadcrumb_get_item_text :: proc(breadcrumb: Rid, index: i32) -> cstring ---
	Rbreadcrumb_set_clicked_callback :: proc(breadcrumb: Rid, callback: RBreadcrumbCallback, user_data: rawptr) ---
	// The score, from 0 to the number of stars.
	Rrating_set_value :: proc(rating: Rid, value: f32) ---
	Rrating_get_value :: proc(rating: Rid) -> f32 ---
	// Number of stars (default 5, at most 20).
	Rrating_set_max :: proc(rating: Rid, max_: i32) ---
	Rrating_get_max :: proc(rating: Rid) -> i32 ---
	// Half stars can be chosen (a whole star by default).
	Rrating_set_allow_half :: proc(rating: Rid, allow: i32) ---
	// Shown only, the user cannot change it.
	Rrating_set_read_only :: proc(rating: Rid, read_only: i32) ---
	// Size of a star (default 18).
	Rrating_set_star_size :: proc(rating: Rid, size: f32) ---
	Rrating_set_changed_callback :: proc(rating: Rid, callback: RRatingChangedCallback, user_data: rawptr) ---
	Rpagination_set_page_count :: proc(pagination: Rid, count: i32) ---
	Rpagination_get_page_count :: proc(pagination: Rid) -> i32 ---
	Rpagination_set_page :: proc(pagination: Rid, page: i32) ---
	Rpagination_get_page :: proc(pagination: Rid) -> i32 ---
	// Page numbers shown around the current one (default 1).
	Rpagination_set_siblings :: proc(pagination: Rid, siblings: i32) ---
	Rpagination_set_changed_callback :: proc(pagination: Rid, callback: RPageChangedCallback, user_data: rawptr) ---
	Rplot_set_title :: proc(plot: Rid, title: cstring) ---
	Rplot_get_title :: proc(plot: Rid) -> cstring ---
	// Adds a series (its name in the legend; NULL or "": not in the legend). Returns its index.
	Rplot_add_series :: proc(plot: Rid, name: cstring) -> i32 ---
	Rplot_remove_series :: proc(plot: Rid, series: i32) ---
	// Removes every series and every drag line.
	Rplot_clear :: proc(plot: Rid) ---
	Rplot_get_series_count :: proc(plot: Rid) -> i32 ---
	Rplot_set_series_name :: proc(plot: Rid, series: i32, name: cstring) ---
	Rplot_get_series_name :: proc(plot: Rid, series: i32) -> cstring ---
	// Replaces the points of a series with `count` points, copied.
	// x The X of the points, or NULL: 0, 1, 2...
	// y The Y of the points. NaN leaves a gap in the line.
	Rplot_set_series_data :: proc(plot: Rid, series: i32, x: [^]f64, y: [^]f64, count: i32) ---
	// Adds points after those of a series (x NULL: numbered after the last one).
	Rplot_append_series_data :: proc(plot: Rid, series: i32, x: [^]f64, y: [^]f64, count: i32) ---
	// Adds one point after those of a series.
	Rplot_add_point :: proc(plot: Rid, series: i32, x: f64, y: f64) ---
	Rplot_clear_series_data :: proc(plot: Rid, series: i32) ---
	Rplot_get_point_count :: proc(plot: Rid, series: i32) -> i32 ---
	// One point of a series. Returns R_FALSE when there is no such point.
	Rplot_get_point :: proc(plot: Rid, series: i32, index: i32, x: ^f64, y: ^f64) -> i32 ---
	// Keeps only the last `max` points of a series, for data that streams in (0: all of them, the default).
	Rplot_set_series_max_points :: proc(plot: Rid, series: i32, max_: i32) ---
	// The color of a series (by default the next one of a palette of ten).
	Rplot_set_series_color :: proc(plot: Rid, series: i32, r: f32, g: f32, b: f32, a: f32) ---
	// How the line is drawn and how wide (default solid, 1.75; width 0 keeps the width).
	Rplot_set_series_line :: proc(plot: Rid, series: i32, line: RPlotLine, width: f32) ---
	// The mark on each point and its radius (default none, 4; size 0 keeps the size).
	Rplot_set_series_marker :: proc(plot: Rid, series: i32, marker: RPlotMarker, size: f32) ---
	// Filled markers (the default) or only their outline.
	Rplot_set_series_marker_filled :: proc(plot: Rid, series: i32, filled: i32) ---
	// Draws the line in steps: along at the height of a point, then up or down to the next.
	Rplot_set_series_step :: proc(plot: Rid, series: i32, step: i32) ---
	// Shades the area between the line and zero, at this opacity (0: none, the default).
	Rplot_set_series_fill :: proc(plot: Rid, series: i32, opacity: f32) ---
	// The axes a series is drawn against: R_PLOT_X or R_PLOT_X2, R_PLOT_Y or R_PLOT_Y2.
	Rplot_set_series_axes :: proc(plot: Rid, series: i32, x_axis: RPlotAxis, y_axis: RPlotAxis) ---
	// Shows or hides a series (the user does it with a click on its name in the legend).
	Rplot_set_series_visible :: proc(plot: Rid, series: i32, visible: i32) ---
	Rplot_is_series_visible :: proc(plot: Rid, series: i32) -> i32 ---
	// The name written along an axis.
	Rplot_set_axis_label :: proc(plot: Rid, axis: RPlotAxis, label: cstring) ---
	Rplot_get_axis_label :: proc(plot: Rid, axis: RPlotAxis) -> cstring ---
	Rplot_set_axis_scale :: proc(plot: Rid, axis: RPlotAxis, scale: RPlotScale) ---
	Rplot_get_axis_scale :: proc(plot: Rid, axis: RPlotAxis) -> RPlotScale ---
	// R_PLOT_SCALE_SYMLOG: the axis is linear from -threshold to threshold (default 1).
	Rplot_set_axis_linear_threshold :: proc(plot: Rid, axis: RPlotAxis, threshold: f64) ---
	// Shows this range of an axis; the axis then stops following the data.
	Rplot_set_axis_range :: proc(plot: Rid, axis: RPlotAxis, min_: f64, max_: f64) ---
	// The range an axis shows now (after the user moved or zoomed it).
	Rplot_get_axis_range :: proc(plot: Rid, axis: RPlotAxis, min_: ^f64, max_: ^f64) ---
	// The axis is fitted to the data whenever it changes (the default, until the view is moved; a double click turns it on again).
	Rplot_set_axis_auto_fit :: proc(plot: Rid, axis: RPlotAxis, auto_fit: i32) ---
	Rplot_is_axis_auto_fit :: proc(plot: Rid, axis: RPlotAxis) -> i32 ---
	// How the labels of an axis are written: a printf format for one double ("%.1f s", "%g"), NULL: chosen from the ticks.
	Rplot_set_axis_format :: proc(plot: Rid, axis: RPlotAxis, format: cstring) ---
	// 1: shown, 0: hidden, -1: R_PLOT_X and R_PLOT_Y always, the second axes when something uses them (the default).
	Rplot_set_axis_visible :: proc(plot: Rid, axis: RPlotAxis, visible: i32) ---
	// Lines across the data at the ticks of an axis (on for R_PLOT_X and R_PLOT_Y).
	Rplot_set_axis_grid :: proc(plot: Rid, axis: RPlotAxis, grid: i32) ---
	// The values grow the other way (to the left, or down).
	Rplot_set_axis_inverted :: proc(plot: Rid, axis: RPlotAxis, inverted: i32) ---
	// The user cannot move or zoom this axis (the application still can).
	Rplot_set_axis_locked :: proc(plot: Rid, axis: RPlotAxis, locked: i32) ---
	// Fits every axis that is not locked to the data, and has them follow it again (what a double click does).
	Rplot_fit :: proc(plot: Rid) ---
	// Where the legend is (default R_PLOT_LEGEND_TOP_RIGHT).
	Rplot_set_legend :: proc(plot: Rid, legend: RPlotLegend) ---
	// Lines across the data through the pointer.
	Rplot_set_crosshair :: proc(plot: Rid, crosshair: i32) ---
	// The coordinates of the pointer in a corner of the data (on by default).
	Rplot_set_mouse_text :: proc(plot: Rid, shown: i32) ---
	// The values of the point under the pointer, written next to it (on by default).
	Rplot_set_tooltips :: proc(plot: Rid, shown: i32) ---
	// Where the pointer is, in the units of R_PLOT_X and R_PLOT_Y. Returns R_TRUE when it is over the data.
	Rplot_get_mouse_position :: proc(plot: Rid, x: ^f64, y: ^f64) -> i32 ---
	// Where a point of the data (R_PLOT_X, R_PLOT_Y) is in the widget, in logical units from its corner.
	Rplot_data_to_point :: proc(plot: Rid, x: f64, y: f64, point_x: ^f32, point_y: ^f32) ---
	// The data (R_PLOT_X, R_PLOT_Y) at a point of the widget, in logical units from its corner.
	Rplot_point_to_data :: proc(plot: Rid, point_x: f32, point_y: f32, x: ^f64, y: ^f64) ---
	// Adds a line across the data that the user drags: vertical at an X
	// (axis R_PLOT_X or R_PLOT_X2), horizontal at a Y (R_PLOT_Y or R_PLOT_Y2).
	// Its value is written on it, after `label` (may be NULL). Returns its index.
	Rplot_add_drag_line :: proc(plot: Rid, axis: RPlotAxis, value: f64, label: cstring) -> i32 ---
	Rplot_remove_drag_line :: proc(plot: Rid, line: i32) ---
	Rplot_get_drag_line_count :: proc(plot: Rid) -> i32 ---
	Rplot_set_drag_line_value :: proc(plot: Rid, line: i32, value: f64) ---
	Rplot_get_drag_line_value :: proc(plot: Rid, line: i32) -> f64 ---
	// The color of a drag line (by default the accent of the theme).
	Rplot_set_drag_line_color :: proc(plot: Rid, line: i32, r: f32, g: f32, b: f32, a: f32) ---
	Rplot_set_view_changed_callback :: proc(plot: Rid, callback: RPlotViewCallback, user_data: rawptr) ---
	Rplot_set_drag_line_callback :: proc(plot: Rid, callback: RPlotDragLineCallback, user_data: rawptr) ---
	Rplot_set_series_toggled_callback :: proc(plot: Rid, callback: RPlotSeriesCallback, user_data: rawptr) ---
	// R_TRUE when PDFium can be loaded (it is, the first time this is asked).
	Rpdf_is_available :: proc() -> i32 ---
	// Shows a PDF file. `password` may be NULL. Returns R_FALSE when it
	// cannot: Rpdf_view_get_status tells why (a password is needed, not a
	// PDF, no PDFium). The view then shows that instead of pages.
	Rpdf_view_load_file :: proc(view: Rid, path: cstring, password: cstring) -> i32 ---
	// Same as Rpdf_view_load_file, from bytes in memory (copied).
	Rpdf_view_load_memory :: proc(view: Rid, data: cstring, data_size: uint, password: cstring) -> i32 ---
	// Closes the document: the view is empty.
	Rpdf_view_close :: proc(view: Rid) ---
	Rpdf_view_get_status :: proc(view: Rid) -> RPdfStatus ---
	Rpdf_view_get_page_count :: proc(view: Rid) -> i32 ---
	// Size of a page in points (1/72 inch), as it is shown (its rotation applied).
	Rpdf_view_get_page_size :: proc(view: Rid, page: i32, width: ^f32, height: ^f32) ---
	// Scrolls to the top of a page; it becomes the current page.
	Rpdf_view_set_page :: proc(view: Rid, page: i32) ---
	// The current page: the one a third down the view.
	Rpdf_view_get_page :: proc(view: Rid) -> i32 ---
	// A zoom of its own (1: the printed size): the fit becomes R_PDF_FIT_NONE.
	Rpdf_view_set_zoom :: proc(view: Rid, zoom: f32) ---
	// The zoom the pages show at, the one a fit gives included.
	Rpdf_view_get_zoom :: proc(view: Rid) -> f32 ---
	Rpdf_view_set_fit :: proc(view: Rid, fit: RPdfFit) ---
	Rpdf_view_get_fit :: proc(view: Rid) -> RPdfFit ---
	// The title of the document (its information), "" when it has none.
	Rpdf_view_get_title :: proc(view: Rid) -> cstring ---
	// The text of a page (UTF-8), to search it or copy it. Valid until
	// the next call for this view.
	Rpdf_view_get_page_text :: proc(view: Rid, page: i32) -> cstring ---
	Rpdf_view_set_page_changed_callback :: proc(view: Rid, callback: RPdfPageChangedCallback, user_data: rawptr) ---
	// Prints the document shown (see Rpdf_print); the job is named after its title.
	Rpdf_view_print :: proc(view: Rid, desc: ^RPdfPrintDesc) -> i32 ---
	Rpdf_document_create :: proc() -> Rid ---
	Rpdf_document_delete :: proc(document: Rid) ---
	// The information of the document, shown by readers (the title in their window).
	Rpdf_document_set_title :: proc(document: Rid, title: cstring) ---
	Rpdf_document_set_author :: proc(document: Rid, author: cstring) ---
	Rpdf_document_set_subject :: proc(document: Rid, subject: cstring) ---
	// Adds a page of a size in points; drawing goes to it. Returns its index, -1 for an invalid size.
	Rpdf_document_add_page :: proc(document: Rid, width: f32, height: f32) -> i32 ---
	Rpdf_document_get_page_count :: proc(document: Rid) -> i32 ---
	Rpdf_document_set_fill_color :: proc(document: Rid, r: f32, g: f32, b: f32, a: f32) ---
	Rpdf_document_set_stroke_color :: proc(document: Rid, r: f32, g: f32, b: f32, a: f32) ---
	// Width of the lines and outlines, in points (default 1).
	Rpdf_document_set_line_width :: proc(document: Rid, width: f32) ---
	// A rectangle, its corners rounded by `radius`; `paint` is a bitmask of RPdfPaint.
	Rpdf_document_draw_rect :: proc(document: Rid, x: f32, y: f32, width: f32, height: f32, radius: f32, paint: u32) ---
	// The ellipse inside a rectangle.
	Rpdf_document_draw_ellipse :: proc(document: Rid, x: f32, y: f32, width: f32, height: f32, paint: u32) ---
	// A line in the stroke color.
	Rpdf_document_draw_line :: proc(document: Rid, x1: f32, y1: f32, x2: f32, y2: f32) ---
	// Adds a TrueType font (.ttf) to the document (the data is
	// copied, and embedded in the file). Returns its number for
	// Rpdf_document_set_font, -1 when it is not a TrueType font.
	Rpdf_document_add_font :: proc(document: Rid, data: cstring, data_size: uint) -> i32 ---
	Rpdf_document_add_font_from_file :: proc(document: Rid, path: cstring) -> i32 ---
	// The font of the text drawn next: an RPdfFont or a number of Rpdf_document_add_font, a size in points, a bitmask of RPdfTextStyle.
	Rpdf_document_set_font :: proc(document: Rid, font: i32, size: f32, style: u32) ---
	// Distance between two lines of text in the current font.
	Rpdf_document_get_line_height :: proc(document: Rid) -> f32 ---
	// Width of a text in the current font, in points (of its widest line).
	Rpdf_document_measure_text :: proc(document: Rid, text: cstring) -> f32 ---
	// Text in the fill color, the top of its first line at y ('\n'
	// starts a line below). Returns its width.
	Rpdf_document_draw_text :: proc(document: Rid, x: f32, y: f32, text: cstring) -> f32 ---
	// Text wrapped at the spaces to fit `width`, each line placed by
	// `align` (R_HALIGN_LEFT, R_HALIGN_CENTER or R_HALIGN_RIGHT). Returns the
	// height it took.
	Rpdf_document_draw_paragraph :: proc(document: Rid, x: f32, y: f32, width: f32, text: cstring, align: RHAlign) -> f32 ---
	// A picture of RGBA8 pixels stretched over a rectangle. A width or
	// height of 0 keeps the proportions of the picture; both 0: 96 pixels per inch.
	Rpdf_document_draw_image :: proc(document: Rid, x: f32, y: f32, width: f32, height: f32, rgba: [^]u8, pixels_width: i32, pixels_height: i32) ---
	// What a window shows (Rwindow_capture), as a picture: a report of
	// the screen, a chart. A width and height of 0: the size it has on the
	// screen. Returns R_FALSE when the window cannot be captured.
	Rpdf_document_draw_window :: proc(document: Rid, window: Rid, x: f32, y: f32, width: f32, height: f32) -> i32 ---
	// Writes the file. Returns R_FALSE when it cannot be written.
	Rpdf_document_save :: proc(document: Rid, path: cstring) -> i32 ---
	// The file in memory (followed by a '\0' not counted), to release with Rmemory_free; `size` receives its length.
	Rpdf_document_save_memory :: proc(document: Rid, size: ^uint) -> cstring ---
	// Prints a PDF document with the print system: on macOS the print
	// panel, on Windows the print dialog (the pages are drawn by PDFium, see
	// PDF VIEW), on Linux and the other systems CUPS (lp or lpr) after a
	// dialog of Raven that chooses the printer. It waits until the job is
	// sent. `desc` may be NULL. Returns R_FALSE when cancelled or when it
	// cannot print. Not in the browser.
	Rpdf_print :: proc(desc: ^RPdfPrintDesc, data: cstring, data_size: uint) -> i32 ---
	Rpdf_print_file :: proc(desc: ^RPdfPrintDesc, path: cstring) -> i32 ---
	// Prints a document written by the application.
	Rpdf_document_print :: proc(document: Rid, desc: ^RPdfPrintDesc) -> i32 ---
	// text Initial content, or NULL.
	Rtext_editor_create :: proc(text: cstring) -> Rid ---
	// Replaces the whole text. The history of undo is cleared; the changed callback is not called.
	Rtext_editor_set_text :: proc(editor: Rid, text: cstring) ---
	// Returns: The whole text, lines separated by '\n'. Valid until the next call or the next edit.
	Rtext_editor_get_text :: proc(editor: Rid) -> cstring ---
	// Returns: The number of bytes of Rtext_editor_get_text.
	Rtext_editor_get_length :: proc(editor: Rid) -> i32 ---
	Rtext_editor_get_line_count :: proc(editor: Rid) -> i32 ---
	// Returns: One line, without its line break. Valid until the next edit. NULL when `line` is out of range.
	Rtext_editor_get_line :: proc(editor: Rid, line: i32) -> cstring ---
	// Types a text at the cursor, replacing the selection, as the user
	// would: it can be undone and the changed callback is called.
	Rtext_editor_insert_text :: proc(editor: Rid, text: cstring) ---
	Rtext_editor_set_cursor :: proc(editor: Rid, line: i32, column: i32) ---
	Rtext_editor_get_cursor :: proc(editor: Rid, line: ^i32, column: ^i32) ---
	// Selects from one position to another; the cursor is on the second.
	Rtext_editor_select :: proc(editor: Rid, from_line: i32, from_column: i32, to_line: i32, to_column: i32) ---
	Rtext_editor_select_all :: proc(editor: Rid) ---
	// Start and end of the selection, in the order of the text. Returns R_FALSE when nothing is selected.
	Rtext_editor_get_selection :: proc(editor: Rid, from_line: ^i32, from_column: ^i32, to_line: ^i32, to_column: ^i32) -> i32 ---
	// Returns: The selected text, "" when there is none. Valid until the next call or the next edit.
	Rtext_editor_get_selected_text :: proc(editor: Rid) -> cstring ---
	// Scrolls so that a line is visible.
	Rtext_editor_scroll_to_line :: proc(editor: Rid, line: i32) ---
	Rtext_editor_undo :: proc(editor: Rid) ---
	Rtext_editor_redo :: proc(editor: Rid) ---
	Rtext_editor_can_undo :: proc(editor: Rid) -> i32 ---
	Rtext_editor_can_redo :: proc(editor: Rid) -> i32 ---
	// A read-only editor can still be scrolled, selected and copied.
	Rtext_editor_set_read_only :: proc(editor: Rid, read_only: i32) ---
	// Long lines continue on the next row instead of scrolling sideways. Off by default.
	Rtext_editor_set_word_wrap :: proc(editor: Rid, enabled: i32) ---
	// Shows the line numbers in a margin. Off by default.
	Rtext_editor_set_line_numbers :: proc(editor: Rid, enabled: i32) ---
	// Width of a tab, in spaces (4 by default).
	Rtext_editor_set_tab_size :: proc(editor: Rid, spaces: i32) ---
	// R_TRUE makes the Tab key insert spaces instead of a tab character. Off by default.
	Rtext_editor_set_tab_inserts_spaces :: proc(editor: Rid, enabled: i32) ---
	// The same as Rtext_editor_set_language with "XML", "C" or NULL.
	Rtext_editor_set_syntax :: proc(editor: Rid, syntax: RSyntax) ---
	// font R_INVALID_ID uses the font of the theme (or the fixed-width
	// font, with a language).
	// size 0 uses the font size of the theme.
	Rtext_editor_set_font :: proc(editor: Rid, font: Rid, size: f32) ---
	// Text shown, dimmed, while the editor is empty.
	Rtext_editor_set_placeholder :: proc(editor: Rid, text: cstring) ---
	// Zoom of the text: a factor of its font size, 1 = 100 % (0.5 .. 4).
	// The user zooms with Ctrl + wheel (around the pointer), Ctrl + '+',
	// Ctrl + '-' and Ctrl + 0 (back to 100 %), keypad included. Set by the
	// program, the top of the view stays and the zoom callback is not called.
	// XML: Zoom="1.25".
	Rtext_editor_set_zoom :: proc(editor: Rid, zoom: f32) ---
	Rtext_editor_get_zoom :: proc(editor: Rid) -> f32 ---
	// R_FALSE: Ctrl + wheel and the keys do not zoom (the wheel scrolls). On by default. XML: Zoomable="false".
	Rtext_editor_set_zoomable :: proc(editor: Rid, zoomable: i32) ---
	Rtext_editor_set_zoom_callback :: proc(editor: Rid, callback: RTextEditorZoomCallback, user_data: rawptr) ---
	// Declares a language, or replaces the one that has this name: to
	// add words to a built-in language, read it with Rlanguage_get, change
	// what you want and register it again. The strings are copied.
	Rlanguage_register :: proc(desc: ^RLanguageDesc) -> i32 ---
	// Reads the description of a language. The strings belong to the library. Returns R_FALSE when it is unknown.
	Rlanguage_get :: proc(name: cstring, desc: ^RLanguageDesc) -> i32 ---
	Rlanguage_get_count :: proc() -> i32 ---
	Rlanguage_get_name :: proc(index: i32) -> cstring ---
	// Returns: The name of the language of a file, from its extension, or NULL.
	Rlanguage_for_file :: proc(path: cstring) -> cstring ---
	// Gives the editor a language, by name ("C++", "Python"...), by
	// another usual name ("cpp", "js") or by the extension of its files
	// ("rs"). NULL or "" removes it. An editor with a language uses the
	// fixed-width font unless it was given a font.
	Rtext_editor_set_language :: proc(editor: Rid, name: cstring) ---
	// Returns: The name of the language of the editor, or "".
	Rtext_editor_get_language :: proc(editor: Rid) -> cstring ---
	// R_TRUE makes the editor an editor of Markdown (the "Markdown"
	// language; Markdown="true" in XML): the text stays as it is written,
	// and what it formats shows while typing, strong text in bold,
	// emphasized text in italic, titles in bold, code, links and marks in
	// their colors. A label with Rlabel_set_markdown shows the same text
	// formatted. R_FALSE removes the language.
	Rtext_editor_set_markdown :: proc(editor: Rid, enabled: i32) ---
	Rtext_editor_get_markdown :: proc(editor: Rid) -> i32 ---
	// What the language of the editor makes of the character at a
	// position: the colors come from it. R_TOKEN_TEXT without a language.
	Rtext_editor_get_token :: proc(editor: Rid, line: i32, column: i32) -> RToken ---
	// R_FALSE stops the editor from proposing words by itself; Ctrl+Space still does. On by default.
	Rtext_editor_set_completion :: proc(editor: Rid, enabled: i32) ---
	// Proposes words for what is before the cursor, as Ctrl+Space does.
	Rtext_editor_show_completions :: proc(editor: Rid) ---
	Rtext_editor_is_completing :: proc(editor: Rid) -> i32 ---
	Rtext_editor_set_completion_callback :: proc(editor: Rid, callback: RCompletionCallback, user_data: rawptr) ---
	// Adds a proposal. Only from a completion callback. Proposals that
	// do not start with the prefix are left out; the others keep the order
	// they were added in.
	// detail A few words shown beside it ("function", "int"), or NULL.
	Rtext_editor_add_completion :: proc(editor: Rid, text: cstring, detail: cstring) ---
	// Adds a proposal that writes another text than the one it shows
	// and is matched on: "Threshold", listed and filtered as such, that
	// writes "Threshold(" when chosen. Only from a completion callback.
	// insert What replaces the word being typed; NULL or "" is `text`.
	Rtext_editor_add_completion_insert :: proc(editor: Rid, text: cstring, detail: cstring, insert: cstring) ---
	// R_FALSE leaves the words of the language and the names of the
	// text out of the proposals: only the completion callback's are listed
	// (a language server that knows better). On by default.
	Rtext_editor_set_completion_words :: proc(editor: Rid, enabled: i32) ---
	// R_FALSE stops the editor from closing brackets and quotes and
	// from indenting after them. On by default; it only acts with a language.
	Rtext_editor_set_auto_pairs :: proc(editor: Rid, enabled: i32) ---
	// Underlines a part of a line and attaches a message to it, shown
	// when the pointer rests there: an error of a compiler, a warning.
	// Markers follow their line when text is added or removed above it, and
	// go away when their own line is edited.
	// to_column -1 for the end of the line.
	Rtext_editor_add_marker :: proc(editor: Rid, line: i32, from_column: i32, to_column: i32, kind: RMarkerKind, message: cstring) ---
	Rtext_editor_clear_markers :: proc(editor: Rid) ---
	Rtext_editor_get_marker_count :: proc(editor: Rid) -> i32 ---
	Rtext_editor_set_hover_callback :: proc(editor: Rid, callback: RTextEditorHoverCallback, user_data: rawptr) ---
	// Color of a kind of token in the theme in use, instead of the
	// one Raven picks for dark and for light themes. Theme files set them
	// with <Syntax Keyword="#FF7AB2" String="..."/>.
	Rtheme_set_token_color :: proc(token: RToken, r: f32, g: f32, b: f32, a: f32) ---
	Rtheme_get_token_color :: proc(token: RToken, r: ^f32, g: ^f32, b: ^f32, a: ^f32) ---
	// Back to the colors Raven picks.
	Rtheme_clear_token_colors :: proc() ---
	Rtext_editor_set_changed_callback :: proc(editor: Rid, callback: RTextEditorChangedCallback, user_data: rawptr) ---
	// title Text shown in the bar; NULL shows the title of the window.
	Rheader_bar_create :: proc(title: cstring) -> Rid ---
	// title NULL shows the title of the window.
	Rheader_bar_set_title :: proc(bar: Rid, title: cstring) ---
	// buttons Bitmask of RHeaderButtons. All by default; the maximize button only shows on a resizable window.
	Rheader_bar_set_buttons :: proc(bar: Rid, buttons: u32) ---
	// side R_HALIGN_RIGHT: minimize, maximize, close, as on Windows.
	// R_HALIGN_LEFT: three round colored buttons, as on macOS. Until it is
	// set, the side is the one of the system the program runs on (left on
	// macOS, right elsewhere), whatever the look.
	Rheader_bar_set_button_side :: proc(bar: Rid, side: RHAlign) ---
	// align R_HALIGN_CENTER (default) or R_HALIGN_LEFT.
	Rheader_bar_set_title_align :: proc(bar: Rid, align: RHAlign) ---
	// R_TRUE: the title sits in a dark tab hanging from the top edge of
	// the bar (slanted sides, round bottom corners). XML: TitleStyle="Notch".
	Rheader_bar_set_title_notch :: proc(bar: Rid, notch: i32) ---
	// R_FALSE removes the background of the bar: for a header that
	// floats above the content (in an Overlay) or sits inside a glass panel.
	Rheader_bar_set_background :: proc(bar: Rid, enabled: i32) ---
	Rdock_area_create :: proc() -> Rid ---
	// Adds a panel. The dock area takes ownership of the widget.
	// title    Text of its tab.
	// side     Where it goes.
	// relative A panel of the area to split or to share tabs with, or
	//                 R_INVALID_ID for the whole area (with R_DOCK_TAB: the
	//                 first group of tabs).
	// size     Part of the split space given to the new panel, 0..1
	//                 (0 = a half, or a quarter along the whole area).
	Rdock_area_add_panel :: proc(area: Rid, panel: Rid, title: cstring, side: RDockSide, relative: Rid, size: f32) ---
	// Moves a panel of the area somewhere else in it.
	Rdock_area_move_panel :: proc(area: Rid, panel: Rid, side: RDockSide, relative: Rid, size: f32) ---
	// Removes a panel and deletes it.
	Rdock_area_close_panel :: proc(area: Rid, panel: Rid) ---
	Rdock_area_set_panel_title :: proc(area: Rid, panel: Rid, title: cstring) ---
	// Icon drawn before the title of the tab of a panel (and on the chip while it is dragged).
	// texture Texture of the icon, R_INVALID_ID removes it.
	// u0,v0,u1,v1 Region of the texture (0, 0, 1, 1 : all of it).
	// tint    R_TRUE: the texture is a mask drawn in the text color of the tab (monochrome icons).
	Rdock_area_set_panel_icon :: proc(area: Rid, panel: Rid, texture: Rid, u0: f32, v0: f32, u1: f32, v1: f32, tint: i32) ---
	// R_FALSE removes the close button of the tab of a panel. Panels can be closed by default.
	Rdock_area_set_panel_closable :: proc(area: Rid, panel: Rid, closable: i32) ---
	// Takes a panel out of the arrangement without deleting it (a
	// window of an application closed from its menu). Its tab disappears
	// and its neighbours take its place; the area remembers where it was.
	Rdock_area_hide_panel :: proc(area: Rid, panel: Rid) ---
	// Puts a hidden panel back where it was, and brings its tab to the front (also for a panel that is not hidden).
	Rdock_area_open_panel :: proc(area: Rid, panel: Rid) ---
	// Returns: R_TRUE when the panel is in the arrangement (not hidden), shown or behind another tab.
	Rdock_area_is_panel_open :: proc(area: Rid, panel: Rid) -> i32 ---
	// R_TRUE: the close button of a tab hides its panel
	// (Rdock_area_hide_panel) instead of deleting it. XML: CloseHides="true".
	// The close callback is asked first either way.
	Rdock_area_set_close_hides :: proc(area: Rid, hides: i32) ---
	// Brings the tab of a panel to the front.
	Rdock_area_show_panel :: proc(area: Rid, panel: Rid) ---
	// Returns: R_TRUE when the panel is the one shown in its group of tabs.
	Rdock_area_is_panel_shown :: proc(area: Rid, panel: Rid) -> i32 ---
	// R_FALSE stops the user from moving the panels (the borders can still be dragged).
	Rdock_area_set_movable :: proc(area: Rid, movable: i32) ---
	// Describes how the panels are arranged, as a short text to keep
	// in a settings file. Panels are named by their widget name
	// (Rwidget_set_name): unnamed panels are not part of the description.
	// Returns: A string valid until the next call.
	Rdock_area_save_layout :: proc(area: Rid) -> cstring ---
	// Arranges the panels as a saved description says. Panels of the
	// area that the description does not name keep a place; names it has
	// that match no panel are ignored.
	// Returns: R_FALSE when the text is not a layout (nothing is changed).
	Rdock_area_load_layout :: proc(area: Rid, layout: cstring) -> i32 ---
	Rdock_area_set_close_callback :: proc(area: Rid, callback: RDockCloseCallback, user_data: rawptr) ---
	// Tearing: a tab dragged out of the area takes its panel to a
	// window of its own (a floating area of the same family), and a tab
	// dragged from such a window over the area docks it back where it is
	// dropped. A double click on the tab of a floating window, or closing
	// that window, puts its panels back where they were. Dragging the only
	// tab of a floating window moves the window. R_TRUE by default; XML:
	// Tearable="false". The functions of this area that take a panel also
	// work on its panels that are in floating windows.
	Rdock_area_set_tearable :: proc(area: Rid, tearable: i32) ---
	Rdock_area_is_tearable :: proc(area: Rid) -> i32 ---
	// Takes a panel to a window of its own, its top left corner at (x, y) on the screen. Returns the window.
	Rdock_area_float_panel :: proc(area: Rid, panel: Rid, x: i32, y: i32) -> Rid ---
	// A floating panel goes back where it was in the area.
	Rdock_area_dock_panel :: proc(area: Rid, panel: Rid) ---
	// The window a floating panel is in, R_INVALID_ID when it is docked in the area.
	Rdock_area_get_panel_window :: proc(area: Rid, panel: Rid) -> Rid ---
	Rdock_area_set_changed_callback :: proc(area: Rid, callback: RDockChangedCallback, user_data: rawptr) ---
	Risland_create :: proc() -> Rid ---
	// Unfolds or folds the island. The changed callback is not called.
	Risland_set_expanded :: proc(island: Rid, expanded: i32) ---
	Risland_is_expanded :: proc(island: Rid) -> i32 ---
	// Unfolds the island, and folds it back by itself after a while:
	// to tell something to the user. It stays open while the pointer is on it.
	Risland_expand_for :: proc(island: Rid, seconds: f64) ---
	// R_TRUE (default): the island unfolds under the pointer and folds back when it leaves.
	Risland_set_expand_on_hover :: proc(island: Rid, enabled: i32) ---
	// R_TRUE (default): a click on the island unfolds or folds it.
	Risland_set_expand_on_click :: proc(island: Rid, enabled: i32) ---
	// R_TRUE gives the island square upper corners: a notch that hangs
	// from the edge it touches, instead of a capsule that floats under it.
	Risland_set_attached :: proc(island: Rid, attached: i32) ---
	// R_TRUE: on a screen with a camera housing at its top (the notch
	// of a MacBook), the island of a window (Rwindow_set_island) lives in the
	// housing itself, in a window of its own anchored to the screen: it stays
	// there when the window is moved, and takes the mouse and the keyboard as
	// usual. Folded, it is the housing grown into two ears: the first Spacer
	// of its folded content becomes the room of the camera (what is before
	// it shows on the left, what is after on the right). Unfolded, its
	// content starts under the camera.
	//
	// It is in the housing while its window is the last one of the
	// application the user worked in, is shown, and is on that screen; it
	// goes back into its window otherwise, and on the window systems and
	// screens without a housing (where nothing changes). XML: Notch="true".
	Risland_set_notch :: proc(island: Rid, enabled: i32) ---
	Risland_get_notch :: proc(island: Rid) -> i32 ---
	// R_TRUE while the island is in the camera housing of the screen.
	Risland_is_in_notch :: proc(island: Rid) -> i32 ---
	Risland_set_changed_callback :: proc(island: Rid, callback: RIslandChangedCallback, user_data: rawptr) ---
	// Shows a widget (usually an island) floating at the top of a
	// window, in the middle, above its content. The window takes ownership;
	// the previous one is deleted. R_INVALID_ID removes it. The padding of
	// its slot (Rslot_set_padding) is its distance from the top edge.
	Rwindow_set_island :: proc(window: Rid, widget: Rid) ---
	Rwindow_get_island :: proc(window: Rid) -> Rid ---
	// A progress bar on the icon of the application: on the button of
	// the window in the taskbar (Windows), on the icon in the Dock (macOS),
	// on the launcher (Linux). There is one icon for the whole application in
	// the Dock and the launchers: the last window that set it is shown.
	// value From 0 to 1 (not used by NONE and INDETERMINATE).
	// Returns: R_FALSE where the system shows no progress.
	Rwindow_set_taskbar_progress :: proc(window: Rid, state: RTaskbarProgress, value: f32) -> i32 ---
	// Asks the user to look at the window when it is not the active
	// one: its taskbar button flashes (Windows), the icon bounces in the
	// Dock (macOS), the window is marked urgent (X11, the launchers of
	// Linux). `critical`: until the user comes to the window; otherwise a few
	// times. Nothing happens when the window is already the active one.
	Rwindow_request_attention :: proc(window: Rid, critical: i32) -> i32 ---
	// A short text on the icon of the application: a number of unread
	// messages, "!". macOS: the badge of the Dock. Windows: a small round
	// icon on the taskbar buttons of the windows, drawn from the text (one
	// to three characters fit). Linux: a number only (the text is read as
	// one; anything else shows nothing). NULL or "": no badge.
	Rtaskbar_set_badge :: proc(text: cstring) -> i32 ---
	// The menu of the icon of the application in the Dock (macOS),
	// above the entries of the system. A MENU without a parent: its
	// MenuItems fire as when chosen in a window (clicked callback, action),
	// and what changes in it shows the next time the menu opens.
	// R_INVALID_ID: none. R_FALSE where the system has no such menu.
	Rtaskbar_set_menu :: proc(menu: Rid) -> i32 ---
	// The tasks of the jump list of Windows (right click on the button
	// of the application in the taskbar, or on its tile in the Start menu).
	// Choosing one starts the application again with its arguments: the
	// application reads them as any other command line. count 0: none.
	// R_FALSE where there is no jump list.
	Rtaskbar_set_jump_list :: proc(tasks: ^RJumpTask, count: i32) -> i32 ---
	// Returns: R_TRUE when the system shows tray icons: always on Windows and
	// macOS; on Linux, when a host of StatusNotifierItem runs (the panel of
	// the desktop). An icon made where none runs yet appears when one starts.
	Rtray_is_supported :: proc() -> i32 ---
	// Puts an icon of the application beside the clock. While one
	// exists, the application goes on running when its last window closes
	// (Ris_running): Rquit (or the action app.quit) ends it.
	// Returns: R_INVALID_ID where the system has none.
	Rtray_create :: proc() -> Rid ---
	Rtray_destroy :: proc(tray: Rid) ---
	Rtray_is_valid :: proc(tray: Rid) -> i32 ---
	// The picture, from tightly packed RGBA8 pixels (straight alpha);
	// the system scales it (16 to 32 pixels square fit everywhere). Copied.
	Rtray_set_icon :: proc(tray: Rid, rgba: [^]u8, width: i32, height: i32) ---
	// The text shown when the pointer rests on the icon (and its title for the Linux panels).
	Rtray_set_tooltip :: proc(tray: Rid, text: cstring) ---
	// The menu the system shows for the icon: on a right click
	// (Windows, Linux), on a click (macOS). A MENU without a parent, whose
	// MenuItems fire as when chosen in a window; what changes in it shows
	// the next time it opens. R_INVALID_ID: none.
	Rtray_set_menu :: proc(tray: Rid, menu: Rid) ---
	// Called on a click: the left button (activate), the right one
	// when there is no menu, the middle one. On macOS an icon with a menu
	// opens it instead.
	Rtray_set_clicked_callback :: proc(tray: Rid, callback: RTrayClickedCallback, user_data: rawptr) ---
	// A short text shown beside the icon (macOS menu bar), drawn by the
	// system in its own font and colors; the item widens to fit it. NULL or
	// "": the icon alone. Systems that show no text there ignore it (the
	// tooltip is the place for it).
	Rtray_set_title :: proc(tray: Rid, text: cstring) ---
	// Where the icon is on the screen, in the units and coordinates of
	// Rwindow_set_position (a window can then open just under it).
	// Returns: R_FALSE when the system does not tell (the outputs are then 0).
	Rtray_get_rect :: proc(tray: Rid, x: ^i32, y: ^i32, width: ^i32, height: ^i32) -> i32 ---
	Rcolor_picker_create :: proc() -> Rid ---
	// Sets the color (0..1 each). The callback is not called.
	Rcolor_picker_set_color :: proc(picker: Rid, r: f32, g: f32, b: f32, a: f32) ---
	Rcolor_picker_get_color :: proc(picker: Rid, r: ^f32, g: ^f32, b: ^f32, a: ^f32) ---
	// The same color as hue (0..360), saturation and value (0..1).
	// The picker remembers the hue of a gray and the saturation of black,
	// which red, green and blue cannot tell.
	Rcolor_picker_set_hsv :: proc(picker: Rid, hue: f32, saturation: f32, value: f32) ---
	Rcolor_picker_get_hsv :: proc(picker: Rid, hue: ^f32, saturation: ^f32, value: ^f32) ---
	// Sets the color from text: "#RGB", "#RRGGBB", "#RRGGBBAA" (the
	// '#' may be left out) or one of the color names of the XML attributes.
	// A code without opacity leaves the opacity as it is.
	// Returns: R_FALSE when the text is not a color (the color is unchanged).
	Rcolor_picker_set_hex :: proc(picker: Rid, text: cstring) -> i32 ---
	// Returns: "#RRGGBB", or "#RRGGBBAA" when the opacity is shown and not full. Valid until the color changes.
	Rcolor_picker_get_hex :: proc(picker: Rid) -> cstring ---
	// R_TRUE (default) shows the opacity strip and the fourth number. R_FALSE: colors are opaque.
	Rcolor_picker_set_alpha_enabled :: proc(picker: Rid, enabled: i32) ---
	// The color shown next to the current one, to compare with what
	// was there before: clicking it comes back to it. Rcolor_picker_set_color
	// does not change it; a popup picker takes it when it opens.
	Rcolor_picker_set_previous_color :: proc(picker: Rid, r: f32, g: f32, b: f32, a: f32) ---
	// Adds a color to the row of swatches: one click takes it.
	Rcolor_picker_add_swatch :: proc(picker: Rid, r: f32, g: f32, b: f32, a: f32) ---
	Rcolor_picker_clear_swatches :: proc(picker: Rid) ---
	Rcolor_picker_get_swatch_count :: proc(picker: Rid) -> i32 ---
	// R_TRUE turns the widget into a swatch that opens the picker in a popup.
	Rcolor_picker_set_popup :: proc(picker: Rid, popup: i32) ---
	// R_TRUE hides the code and the numbers: only the square, the strips and the swatches.
	Rcolor_picker_set_compact :: proc(picker: Rid, compact: i32) ---
	Rcolor_picker_set_changed_callback :: proc(picker: Rid, callback: RColorChangedCallback, user_data: rawptr) ---
	// Red, green, blue (0..1) to hue (0..360), saturation, value (0..1).
	Rcolor_rgb_to_hsv :: proc(r: f32, g: f32, b: f32, hue: ^f32, saturation: ^f32, value: ^f32) ---
	Rcolor_hsv_to_rgb :: proc(hue: f32, saturation: f32, value: f32, r: ^f32, g: ^f32, b: ^f32) ---
	Rtimeline_create :: proc() -> Rid ---
	// Length of the sequence. The view and the playhead stay within 0 .. duration.
	Rtimeline_set_duration :: proc(timeline: Rid, seconds: f64) ---
	Rtimeline_get_duration :: proc(timeline: Rid) -> f64 ---
	// Moves the playhead. The time callback is not called.
	Rtimeline_set_time :: proc(timeline: Rid, seconds: f64) ---
	Rtimeline_get_time :: proc(timeline: Rid) -> f64 ---
	// Frames per second. With a frame rate the ruler counts in
	// frames ("2:12" is second 2, frame 12) and everything snaps to frames;
	// 0 (default) counts in seconds and snaps to the marks of the ruler.
	Rtimeline_set_frame_rate :: proc(timeline: Rid, frames_per_second: f64) ---
	Rtimeline_get_frame_rate :: proc(timeline: Rid) -> f64 ---
	// R_FALSE lets items be placed anywhere (holding Alt while dragging does it for one move).
	Rtimeline_set_snapping :: proc(timeline: Rid, enabled: i32) ---
	// The part of the sequence that is shown. Zooming is showing less.
	Rtimeline_set_view :: proc(timeline: Rid, start: f64, end: f64) ---
	Rtimeline_get_view :: proc(timeline: Rid, start: ^f64, end: ^f64) ---
	// Makes the playhead run by itself, at the speed of the clock,
	// calling the time callback at each frame; at the end it stops, or
	// starts again with Rtimeline_set_loop.
	Rtimeline_set_playing :: proc(timeline: Rid, playing: i32) ---
	Rtimeline_is_playing :: proc(timeline: Rid) -> i32 ---
	Rtimeline_set_loop :: proc(timeline: Rid, loop: i32) ---
	// Width of the column of the names of the tracks. 0 (default): fits the longest name.
	Rtimeline_set_header_width :: proc(timeline: Rid, width: f32) ---
	// Returns: The width of the column of the names, as laid out now.
	Rtimeline_get_header_width :: proc(timeline: Rid) -> f32 ---
	// Height of the ruler of time (and of the markers' flags band). 0 (default): automatic.
	Rtimeline_set_ruler_height :: proc(timeline: Rid, height: f32) ---
	Rtimeline_get_ruler_height :: proc(timeline: Rid) -> f32 ---
	// Height of the tracks that have none of their own (Rtimeline_set_track_height). 0 (default): automatic.
	Rtimeline_set_default_track_height :: proc(timeline: Rid, height: f32) ---
	Rtimeline_get_default_track_height :: proc(timeline: Rid) -> f32 ---
	// Returns: The track, or 0. Tracks are shown in the order they were added.
	Rtimeline_add_track :: proc(timeline: Rid, name: cstring) -> i32 ---
	// Removes a track and everything on it.
	Rtimeline_remove_track :: proc(timeline: Rid, track: i32) ---
	Rtimeline_get_track_count :: proc(timeline: Rid) -> i32 ---
	// Returns: The index-th track from the top, or 0.
	Rtimeline_get_track :: proc(timeline: Rid, index: i32) -> i32 ---
	Rtimeline_set_track_name :: proc(timeline: Rid, track: i32, name: cstring) ---
	Rtimeline_get_track_name :: proc(timeline: Rid, track: i32) -> cstring ---
	// Color of the clips and keys of the track that have none of their own.
	Rtimeline_set_track_color :: proc(timeline: Rid, track: i32, r: f32, g: f32, b: f32) ---
	// Height of the track in logical units; 0 = the default.
	Rtimeline_set_track_height :: proc(timeline: Rid, track: i32, height: f32) ---
	// A locked track shows its items but does not let the user change them.
	Rtimeline_set_track_locked :: proc(timeline: Rid, track: i32, locked: i32) ---
	// A muted track is drawn dimmed; what muting means is up to the application.
	Rtimeline_set_track_muted :: proc(timeline: Rid, track: i32, muted: i32) ---
	// A property of the look of a track: "ClipStyle" (Solid,
	// Gradient, Tint, Outline), "KeyShape" (Diamond, Circle, Square...),
	// "KeySize", "ClipRadius", "Background" (its lane), or a part and a
	// property ("Clip.Glow", "Key.BorderColor"). "" or "Default" gives back
	// the one of the timeline.
	// Returns: R_FALSE when the name or the value is not one.
	Rtimeline_set_track_style :: proc(timeline: Rid, track: i32, property: cstring, value: cstring) -> i32 ---
	Rtimeline_is_track_muted :: proc(timeline: Rid, track: i32) -> i32 ---
	// Returns: The clip, or 0 when the track does not exist.
	Rtimeline_add_clip :: proc(timeline: Rid, track: i32, start: f64, length: f64, label: cstring) -> i32 ---
	// Returns: The key, or 0.
	Rtimeline_add_key :: proc(timeline: Rid, track: i32, time: f64) -> i32 ---
	// A named moment of the whole sequence, shown on the ruler and across the tracks.
	Rtimeline_add_marker :: proc(timeline: Rid, time: f64, label: cstring) -> i32 ---
	Rtimeline_remove_item :: proc(timeline: Rid, item: i32) ---
	// Removes every track, clip, key and marker.
	Rtimeline_clear :: proc(timeline: Rid) ---
	Rtimeline_get_item_type :: proc(timeline: Rid, item: i32) -> RTimelineItem ---
	// Returns: The track of a clip or of a key; 0 for a marker.
	Rtimeline_get_item_track :: proc(timeline: Rid, item: i32) -> i32 ---
	Rtimeline_get_item_start :: proc(timeline: Rid, item: i32) -> f64 ---
	// Returns: The length of a clip; 0 for a key or a marker.
	Rtimeline_get_item_length :: proc(timeline: Rid, item: i32) -> f64 ---
	// Moves an item: to another track (0 = keep), another start, another length (clips; negative = keep).
	Rtimeline_set_item :: proc(timeline: Rid, item: i32, track: i32, start: f64, length: f64) ---
	Rtimeline_set_item_label :: proc(timeline: Rid, item: i32, label: cstring) ---
	Rtimeline_get_item_label :: proc(timeline: Rid, item: i32) -> cstring ---
	Rtimeline_set_item_color :: proc(timeline: Rid, item: i32, r: f32, g: f32, b: f32) ---
	// A property of the look of one clip, key or marker: "Style"
	// (Solid, Gradient, Tint, Outline: clips), "Shape" (keys), or any
	// property of its part ("Background", "BorderColor", "Glow",
	// "CornerRadius", "Foreground" for its label...).
	Rtimeline_set_item_style :: proc(timeline: Rid, item: i32, property: cstring, value: cstring) -> i32 ---
	// The look the timeline starts from (R_PRESET_AUTO by default; Preset="..." in XML).
	Rtimeline_set_preset :: proc(timeline: Rid, preset: RVisualPreset) ---
	Rtimeline_get_preset :: proc(timeline: Rid) -> RVisualPreset ---
	Rtimeline_get_item_count :: proc(timeline: Rid) -> i32 ---
	// Returns: The index-th item, in the order they were added, or 0.
	Rtimeline_get_item :: proc(timeline: Rid, index: i32) -> i32 ---
	// Selects an item, alone or (add) with the ones already selected.
	Rtimeline_select :: proc(timeline: Rid, item: i32, add: i32) ---
	Rtimeline_clear_selection :: proc(timeline: Rid) ---
	Rtimeline_is_selected :: proc(timeline: Rid, item: i32) -> i32 ---
	Rtimeline_get_selection_count :: proc(timeline: Rid) -> i32 ---
	Rtimeline_get_selected :: proc(timeline: Rid, index: i32) -> i32 ---
	Rtimeline_set_time_callback :: proc(timeline: Rid, callback: RTimelineTimeCallback, user_data: rawptr) ---
	// Called for each item the user moved, stretched or retimed, as
	// it happens. The action of the widget (OnChanged in XML) fires once
	// per gesture, when the button is released.
	Rtimeline_set_changed_callback :: proc(timeline: Rid, callback: RTimelineItemCallback, user_data: rawptr) ---
	// Called when the selection changed; `item` is the item just clicked, or 0.
	Rtimeline_set_selection_callback :: proc(timeline: Rid, callback: RTimelineItemCallback, user_data: rawptr) ---
	// Called when an item is double-clicked.
	Rtimeline_set_activated_callback :: proc(timeline: Rid, callback: RTimelineItemCallback, user_data: rawptr) ---
	// Called when the user deletes an item (the Delete key), before
	// it is removed: its values can still be read. Without this callback the
	// Delete key does nothing: the application decides what can be deleted.
	Rtimeline_set_removed_callback :: proc(timeline: Rid, callback: RTimelineItemCallback, user_data: rawptr) ---
	Rnode_graph_create :: proc() -> Rid ---
	// x,y Top-left corner of the node, in the units of the canvas.
	Rnode_graph_add_node :: proc(graph: Rid, title: cstring, x: f32, y: f32) -> i32 ---
	// Removes a node, its pins, their links, and the widget it holds.
	Rnode_graph_remove_node :: proc(graph: Rid, node: i32) ---
	Rnode_graph_clear :: proc(graph: Rid) ---
	Rnode_graph_get_node_count :: proc(graph: Rid) -> i32 ---
	Rnode_graph_get_node :: proc(graph: Rid, index: i32) -> i32 ---
	Rnode_graph_set_node_title :: proc(graph: Rid, node: i32, title: cstring) ---
	Rnode_graph_get_node_title :: proc(graph: Rid, node: i32) -> cstring ---
	Rnode_graph_set_node_position :: proc(graph: Rid, node: i32, x: f32, y: f32) ---
	Rnode_graph_get_node_position :: proc(graph: Rid, node: i32, x: ^f32, y: ^f32) ---
	// Size of a node on the canvas: Raven works it out from its title, its pins and its content.
	Rnode_graph_get_node_size :: proc(graph: Rid, node: i32, width: ^f32, height: ^f32) ---
	// Color of the header of the node; without one it takes the accent color of the theme.
	Rnode_graph_set_node_color :: proc(graph: Rid, node: i32, r: f32, g: f32, b: f32) ---
	// A property of the look of one node: "HeaderStyle" (Solid,
	// Gradient, Stripe, Tint, None), "HeaderColor", "TitleColor",
	// "Background", "BorderColor", "BorderWidth", "CornerRadius", "Glow",
	// "Icon" (an image file shown before its title), or a part and a property
	// ("Header.Gradient", "Node.ShadowSize"). "" or "Default" gives back the
	// one of the graph.
	// Returns: R_FALSE when the name or the value is not one.
	Rnode_graph_set_node_style :: proc(graph: Rid, node: i32, property: cstring, value: cstring) -> i32 ---
	// Puts a widget inside the node, under its pins: the graph owns
	// it from now on. R_INVALID_ID removes (and deletes) the one it had.
	Rnode_graph_set_node_content :: proc(graph: Rid, node: i32, widget: Rid) ---
	Rnode_graph_get_node_content :: proc(graph: Rid, node: i32) -> Rid ---
	// Name of the node, the `Name` of its XML element: what
	// <Link From="name.pin"> and Rnode_graph_find_node look for. Not shown.
	Rnode_graph_set_node_name :: proc(graph: Rid, node: i32, name: cstring) ---
	Rnode_graph_get_node_name :: proc(graph: Rid, node: i32) -> cstring ---
	// Returns: The node of that name, or 0.
	Rnode_graph_find_node :: proc(graph: Rid, name: cstring) -> i32 ---
	// Value the application attaches to a node.
	Rnode_graph_set_node_user_data :: proc(graph: Rid, node: i32, user_data: rawptr) ---
	Rnode_graph_get_node_user_data :: proc(graph: Rid, node: i32) -> rawptr ---
	// type Free text, or NULL for a pin that links to any type. Returns: The pin, or 0.
	Rnode_graph_add_input :: proc(graph: Rid, node: i32, label: cstring, type: cstring) -> i32 ---
	Rnode_graph_add_output :: proc(graph: Rid, node: i32, label: cstring, type: cstring) -> i32 ---
	Rnode_graph_remove_pin :: proc(graph: Rid, pin: i32) ---
	Rnode_graph_get_pin_node :: proc(graph: Rid, pin: i32) -> i32 ---
	Rnode_graph_is_pin_output :: proc(graph: Rid, pin: i32) -> i32 ---
	Rnode_graph_get_pin_label :: proc(graph: Rid, pin: i32) -> cstring ---
	Rnode_graph_get_pin_type :: proc(graph: Rid, pin: i32) -> cstring ---
	// Name of the pin for Rnode_graph_find_pin and for the links of XML; a pin without one answers to its label.
	Rnode_graph_set_pin_name :: proc(graph: Rid, pin: i32, name: cstring) ---
	// Returns: The pin of a node with that name (or label), or 0.
	Rnode_graph_find_pin :: proc(graph: Rid, node: i32, name: cstring) -> i32 ---
	// Where a pin is on the canvas (the middle of its dot).
	Rnode_graph_get_pin_position :: proc(graph: Rid, pin: i32, x: ^f32, y: ^f32) ---
	// Returns: The index-th input (or output) of a node, or 0.
	Rnode_graph_get_input :: proc(graph: Rid, node: i32, index: i32) -> i32 ---
	Rnode_graph_get_output :: proc(graph: Rid, node: i32, index: i32) -> i32 ---
	// Color of the pins of a type and of the links between them.
	Rnode_graph_set_type_color :: proc(graph: Rid, type: cstring, r: f32, g: f32, b: f32) ---
	// A property of the pins of a type and of their links: "Shape"
	// (Circle, Ring, Square, Diamond, Triangle, Arrow), "Size", "Glow",
	// "LinkShape" (Bezier, Straight, Step, Angled), "LinkWidth", "LinkGlow".
	Rnode_graph_set_type_style :: proc(graph: Rid, type: cstring, property: cstring, value: cstring) -> i32 ---
	// The same for one pin ("Shape", "Size", "Glow", "Color").
	Rnode_graph_set_pin_style :: proc(graph: Rid, pin: i32, property: cstring, value: cstring) -> i32 ---
	// Links an output to an input, as the user does by dragging.
	// The connect callback is not asked.
	// Returns: The link; 0 when the pins cannot be linked (two inputs, the
	// same node, different types).
	Rnode_graph_connect :: proc(graph: Rid, output_pin: i32, input_pin: i32) -> i32 ---
	Rnode_graph_disconnect :: proc(graph: Rid, link: i32) ---
	Rnode_graph_get_link_count :: proc(graph: Rid) -> i32 ---
	Rnode_graph_get_link :: proc(graph: Rid, index: i32) -> i32 ---
	Rnode_graph_get_link_output :: proc(graph: Rid, link: i32) -> i32 ---
	Rnode_graph_get_link_input :: proc(graph: Rid, link: i32) -> i32 ---
	// Returns: The link that arrives at an input pin, or 0.
	Rnode_graph_get_input_link :: proc(graph: Rid, input_pin: i32) -> i32 ---
	// What the middle of the widget shows: a point of the canvas, and how large (1 = one unit per logical unit).
	Rnode_graph_set_view :: proc(graph: Rid, center_x: f32, center_y: f32, zoom: f32) ---
	Rnode_graph_get_view :: proc(graph: Rid, center_x: ^f32, center_y: ^f32, zoom: ^f32) ---
	// Converts a point of the widget (from its corner, as in the events) to the canvas, and back, with the view as it is shown now.
	Rnode_graph_to_canvas :: proc(graph: Rid, x: f32, y: f32, canvas_x: ^f32, canvas_y: ^f32) ---
	Rnode_graph_from_canvas :: proc(graph: Rid, canvas_x: f32, canvas_y: f32, x: ^f32, y: ^f32) ---
	// Moves and zooms so that every node shows.
	Rnode_graph_frame_all :: proc(graph: Rid) ---
	// R_FALSE hides the grid of dots (shown by default).
	Rnode_graph_set_grid_visible :: proc(graph: Rid, visible: i32) ---
	// The pulses of light that run along the links, from output to
	// input. By default they run while the pointer is over the graph, and
	// the window rests the rest of the time. R_TRUE makes them run all the
	// time (the window then draws without pause); R_FALSE stops them.
	Rnode_graph_set_flow_visible :: proc(graph: Rid, visible: i32) ---
	// R_TRUE: the user can look but not move, link or delete.
	Rnode_graph_set_read_only :: proc(graph: Rid, read_only: i32) ---
	// The look the graph starts from (R_PRESET_AUTO by default; Preset="..." in XML).
	Rnode_graph_set_preset :: proc(graph: Rid, preset: RVisualPreset) ---
	Rnode_graph_get_preset :: proc(graph: Rid) -> RVisualPreset ---
	Rnode_graph_select_node :: proc(graph: Rid, node: i32, add: i32) ---
	Rnode_graph_clear_selection :: proc(graph: Rid) ---
	Rnode_graph_is_node_selected :: proc(graph: Rid, node: i32) -> i32 ---
	Rnode_graph_get_selection_count :: proc(graph: Rid) -> i32 ---
	Rnode_graph_get_selected_node :: proc(graph: Rid, index: i32) -> i32 ---
	Rnode_graph_set_connect_callback :: proc(graph: Rid, callback: RNodeGraphConnectCallback, user_data: rawptr) ---
	// Called after the user made a link, and before a link the user removed goes (its pins can still be read).
	Rnode_graph_set_linked_callback :: proc(graph: Rid, callback: RNodeGraphItemCallback, user_data: rawptr) ---
	Rnode_graph_set_unlinked_callback :: proc(graph: Rid, callback: RNodeGraphItemCallback, user_data: rawptr) ---
	// Called for each node the user moved, when the button is released.
	Rnode_graph_set_moved_callback :: proc(graph: Rid, callback: RNodeGraphItemCallback, user_data: rawptr) ---
	// Called when the selection changed; `item` is the node just clicked, or 0.
	Rnode_graph_set_selection_callback :: proc(graph: Rid, callback: RNodeGraphItemCallback, user_data: rawptr) ---
	// Called when the user deletes a node (the Delete key), before
	// it is removed. Without this callback the Delete key only removes
	// links: the application decides whether nodes can be deleted.
	Rnode_graph_set_removed_callback :: proc(graph: Rid, callback: RNodeGraphItemCallback, user_data: rawptr) ---
	// Called when the user double-clicks a node (to open what it stands for).
	Rnode_graph_set_activated_callback :: proc(graph: Rid, callback: RNodeGraphItemCallback, user_data: rawptr) ---
	Rnode_graph_set_context_callback :: proc(graph: Rid, callback: RNodeGraphContextCallback, user_data: rawptr) ---
	// Double click on the empty canvas (`item` 0): the application acts
	// there (a search to add a node...). Without this callback it frames all nodes.
	Rnode_graph_set_canvas_activated_callback :: proc(graph: Rid, callback: RNodeGraphPointCallback, user_data: rawptr) ---
	// A wire the user pulled from a pin was released on nothing: `item` is
	// that pin, x, y where it was dropped (to offer a node to connect it to). The
	// wire draws back as usual.
	Rnode_graph_set_wire_dropped_callback :: proc(graph: Rid, callback: RNodeGraphPointCallback, user_data: rawptr) ---
	// A pin was clicked, not dragged (`item` is the pin, x, y on the canvas):
	// the application may link in two clicks (Rnode_graph_set_pin_emphasis shows where
	// the second may go). With this callback such a click neither detaches the wire
	// of an input nor counts as a wire dropped on nothing.
	Rnode_graph_set_pin_clicked_callback :: proc(graph: Rid, callback: RNodeGraphPointCallback, user_data: rawptr) ---
	// The node under a point of the widget (x, y as in the events: Rnode_graph_to_canvas), or 0.
	Rnode_graph_get_node_at :: proc(graph: Rid, x: f32, y: f32) -> i32 ---
	// The pin near a point of the widget (or whose tag is under it: Rnode_graph_set_pin_tag), or 0 (a menu for a pin).
	Rnode_graph_get_pin_at :: proc(graph: Rid, x: f32, y: f32) -> i32 ---
	// The link under a point of the widget, or 0 (hidden links are never under it).
	Rnode_graph_get_link_at :: proc(graph: Rid, x: f32, y: f32) -> i32 ---
	// R_TRUE: this input keeps every link made to it (a state that several
	// transitions enter) instead of one. Pulling a wire from it starts a new one.
	Rnode_graph_set_pin_multiple :: proc(graph: Rid, pin: i32, multiple: i32) ---
	// R_TRUE: the node is running (live debug) : a soft ring breathes around it.
	Rnode_graph_set_node_active :: proc(graph: Rid, node: i32, active: i32) ---
	// Color of that ring (the success color of the theme by default).
	Rnode_graph_set_node_active_color :: proc(graph: Rid, node: i32, r: f32, g: f32, b: f32) ---
	// R_FALSE: the Delete key does not remove this node (the root of a tree, the output of a graph).
	Rnode_graph_set_node_deletable :: proc(graph: Rid, node: i32, deletable: i32) ---
	// Shows a node at once, without the animation a node added to a shown graph pops in with (an application that rebuilds its graph).
	Rnode_graph_set_node_shown :: proc(graph: Rid, node: i32) ---
	// The same for a link made with Rnode_graph_connect: drawn whole at once instead of growing from its output.
	Rnode_graph_set_link_shown :: proc(graph: Rid, link: i32) ---
	// An icon before the title: a texture of the application, shared by many nodes and not deleted with them (the "Icon" style loads one per node from a file). R_INVALID_ID removes it.
	Rnode_graph_set_node_icon :: proc(graph: Rid, node: i32, texture: Rid) ---
	// A short text in a pill on the top right corner of the node ("1", "!"...); NULL or "" removes it.
	Rnode_graph_set_node_badge :: proc(graph: Rid, node: i32, text: cstring) ---
	// A short text in a chip of the pin's color outside the node, beside the pin
	// (left of an input, right of an output): the name of what goes out or comes in
	// there, as a net label of a schematic. NULL or "" removes it.
	Rnode_graph_set_pin_tag :: proc(graph: Rid, pin: i32, text: cstring) ---
	// R_TRUE: the link stays (its pins are linked, the application keeps
	// it) but is not drawn and not picked: its two ends are shown otherwise (pin
	// tags naming it, as the labels of a schematic stand for a wire).
	Rnode_graph_set_link_hidden :: proc(graph: Rid, link: i32, hidden: i32) ---
	// How much a pin stands out, as while a wire is pulled: 1 shows it as a pin
	// to go to, -1 steps it back, 0 as usual (an application that links in two clicks).
	Rnode_graph_set_pin_emphasis :: proc(graph: Rid, pin: i32, emphasis: i32) ---
	// The pin whose tag (Rnode_graph_set_pin_tag) is under a point of the widget, or 0: the chip only, not the pin.
	Rnode_graph_get_tag_at :: proc(graph: Rid, x: f32, y: f32) -> i32 ---
	// R_TRUE: the pin's tag is drawn as under the pointer (a stronger fill, a
	// thicker edge) — every tag of one net lit while the pointer is on one of them.
	Rnode_graph_set_pin_tag_lit :: proc(graph: Rid, pin: i32, lit: i32) ---
	// Pin labels wider than `width` (logical units) are cut short with an
	// ellipsis and the nodes are laid out with them so (compact ports); 0, the
	// default, shows every label whole.
	Rnode_graph_set_pin_label_width :: proc(graph: Rid, width: f32) ---
	Rnode_graph_get_pin_label_width :: proc(graph: Rid) -> f32 ---
	// Color of the badge pill of one node (Rnode_graph_set_node_badge); its text turns dark or light to suit it.
	Rnode_graph_set_node_badge_color :: proc(graph: Rid, node: i32, r: f32, g: f32, b: f32) ---
	Rblock_editor_create :: proc() -> Rid ---
	// A kind of block, and its block in the palette. Returns its index, -1 on an error.
	Rblock_editor_add_kind :: proc(editor: Rid, name: cstring, label: cstring, shape: RBlockShape, category: cstring) -> i32 ---
	// Color of the blocks of a category (Raven picks one from its name otherwise).
	Rblock_editor_set_category_color :: proc(editor: Rid, category: cstring, r: f32, g: f32, b: f32) ---
	Rblock_editor_set_palette_visible :: proc(editor: Rid, visible: i32) ---
	// A new block, the top of a script of its own at x, y (canvas units). Returns it, 0 on an error.
	Rblock_editor_add_block :: proc(editor: Rid, kind: cstring, x: f32, y: f32) -> i32 ---
	// Removes a block and everything that hangs under it.
	Rblock_editor_remove_block :: proc(editor: Rid, block: i32) ---
	Rblock_editor_clear :: proc(editor: Rid) ---
	// Puts a block (with what is under it) at a place of another one. R_FALSE when it does not fit there.
	Rblock_editor_attach :: proc(editor: Rid, block: i32, target: i32, place: RBlockPlace, index: i32) -> i32 ---
	// Takes a block (with what is under it) out: a script of its own at x, y.
	Rblock_editor_detach :: proc(editor: Rid, block: i32, x: f32, y: f32) ---
	Rblock_editor_get_script_count :: proc(editor: Rid) -> i32 ---
	// The top block of a script.
	Rblock_editor_get_script :: proc(editor: Rid, index: i32) -> i32 ---
	Rblock_editor_get_kind :: proc(editor: Rid, block: i32) -> cstring ---
	// The block under it, 0 if none.
	Rblock_editor_get_next :: proc(editor: Rid, block: i32) -> i32 ---
	Rblock_editor_get_mouth_count :: proc(editor: Rid, block: i32) -> i32 ---
	// The first block in a mouth of a C block, 0 if empty.
	Rblock_editor_get_body :: proc(editor: Rid, block: i32, mouth: i32) -> i32 ---
	Rblock_editor_get_slot_count :: proc(editor: Rid, block: i32) -> i32 ---
	// The value block in a slot, 0 if the slot holds a typed value.
	Rblock_editor_get_input :: proc(editor: Rid, block: i32, slot: i32) -> i32 ---
	// The typed value of a slot.
	Rblock_editor_get_value :: proc(editor: Rid, block: i32, slot: i32) -> cstring ---
	Rblock_editor_set_value :: proc(editor: Rid, block: i32, slot: i32, value: cstring) ---
	// Where a block hangs: the block it hangs from (0: the top of a script).
	Rblock_editor_get_parent :: proc(editor: Rid, block: i32, place: ^RBlockPlace, index: ^i32) -> i32 ---
	Rblock_editor_get_position :: proc(editor: Rid, block: i32, x: ^f32, y: ^f32) ---
	// R_TRUE: the block is running: a halo breathes around it.
	Rblock_editor_set_block_glow :: proc(editor: Rid, block: i32, glow: i32) ---
	// Every script as text (valid until the next call).
	Rblock_editor_save :: proc(editor: Rid) -> cstring ---
	// Replaces the scripts with those of a text from Rblock_editor_save.
	Rblock_editor_load :: proc(editor: Rid, text: cstring) -> i32 ---
	Rblock_editor_set_changed_callback :: proc(editor: Rid, callback: RBlockEditorCallback, user_data: rawptr) ---
	Rblock_editor_set_clicked_callback :: proc(editor: Rid, callback: RBlockEditorCallback, user_data: rawptr) ---
	Rblock_editor_set_context_callback :: proc(editor: Rid, callback: RBlockEditorContextCallback, user_data: rawptr) ---
	// The choices of a %m slot of a kind, separated by '\n' ("" : the slot is typed in).
	Rblock_editor_set_slot_options :: proc(editor: Rid, kind: cstring, slot: i32, options: cstring) ---
	// R_FALSE: the kind is not in the palette (its blocks stay where they are).
	Rblock_editor_set_kind_visible :: proc(editor: Rid, kind: cstring, visible: i32) ---
	// A new label for a kind (a function that takes one more input...).
	// The blocks of that kind keep the values of the slots that are still there.
	Rblock_editor_set_kind_label :: proc(editor: Rid, kind: cstring, label: cstring) -> i32 ---
	// R_FALSE: the user cannot delete the block (the entry of a function). Not saved.
	Rblock_editor_set_block_deletable :: proc(editor: Rid, block: i32, deletable: i32) ---
	Rblock_editor_get_selection_count :: proc(editor: Rid) -> i32 ---
	Rblock_editor_get_selected :: proc(editor: Rid, index: i32) -> i32 ---
	// Selects a block (add: R_FALSE clears the selection first ; block 0 : only clears).
	Rblock_editor_select :: proc(editor: Rid, block: i32, add: i32) ---
	// Pan (canvas units at the top left) and zoom.
	Rblock_editor_get_view :: proc(editor: Rid, x: ^f32, y: ^f32, zoom: ^f32) ---
	Rblock_editor_set_view :: proc(editor: Rid, x: f32, y: f32, zoom: f32) ---
	// Shows a widget as a dialog of a window. The window takes
	// ownership of `content`: it is deleted when the dialog closes, so read
	// what you need from it before, or in `closed`.
	// The Escape key dismisses the dialog (result 0).
	Rdialog_open :: proc(window: Rid, content: Rid, closed: RDialogClosedCallback, user_data: rawptr) ---
	// Closes the dialog that contains a widget (the content itself or
	// anything inside it: the button that was clicked).
	// result Passed to the closed callback and returned by Rdialog_run.
	Rdialog_close :: proc(widget: Rid, result: i32) ---
	// Shows a dialog and waits until it closes. Returns its result.
	Rdialog_run :: proc(window: Rid, content: Rid) -> i32 ---
	// Shows a widget as a popup over the window, its top-left corner at
	// x, y (window coordinates; moved to stay inside the window): a search box,
	// a palette, a picker. Unlike a dialog it does not block the window: a click
	// outside it, or Escape, closes it. The application keeps the content (it is
	// not deleted on close) and can open it again. `closed` (may be NULL) is
	// called when it closes, with the result given to Rpopup_close (0 when dismissed).
	// The content draws its own background (a Border with a Background...).
	Rpopup_open :: proc(window: Rid, content: Rid, x: f32, y: f32, closed: RDialogClosedCallback, user_data: rawptr) ---
	// Closes the popup that contains a widget (the content or anything inside it).
	Rpopup_close :: proc(widget: Rid, result: i32) ---
	// Returns: R_TRUE while the widget is shown as a popup.
	Rpopup_is_open :: proc(content: Rid) -> i32 ---
	// Returns: R_TRUE while a dialog is shown in the window.
	Rwindow_has_dialog :: proc(window: Rid) -> i32 ---
	// Shows a message with buttons and waits for the answer.
	// window R_INVALID_ID: the first window of the application.
	// title  May be NULL.
	Rmessage_box :: proc(window: Rid, title: cstring, text: cstring, buttons: RMessageButtons) -> RMessageResult ---
	// Asks for a file to open.
	// desc May be NULL.
	// Returns: The path (UTF-8), valid until the next file dialog, or NULL when the user cancelled.
	Rfile_dialog_open :: proc(desc: ^RFileDialogDesc) -> cstring ---
	// Asks where to save a file. An existing file was confirmed by the user.
	Rfile_dialog_save :: proc(desc: ^RFileDialogDesc) -> cstring ---
	// Asks for a folder.
	Rfile_dialog_select_folder :: proc(desc: ^RFileDialogDesc) -> cstring ---
	// What the last file dialog returned: several paths with R_FILE_DIALOG_MULTIPLE.
	Rfile_dialog_get_result_count :: proc() -> i32 ---
	Rfile_dialog_get_result :: proc(index: i32) -> cstring ---
	// Registers a file held in memory under a path: every file Raven
	// reads (XML documents and their Include, Source and Font attributes,
	// themes, textures, fonts, Rfile_read) is looked up here first, so that
	// resources embedded in the executable need no file beside it. A relative
	// path names the file as it is written in the XML once joined to the
	// folder of the document that names it (Rxml_set_base_path for a
	// document given as text), e.g. "ui/editor.xml". Paths compare with '/'
	// and '\\' alike. Memory files are not watched for live reload. Call it
	// after Rinit; Rshutdown forgets every file.
	// copy R_TRUE: Raven keeps a copy; R_FALSE: `data` must outlive Raven (static data).
	Rfile_register_memory :: proc(path: cstring, data: rawptr, size: uint, copy: i32) -> i32 ---
	Rfile_unregister_memory :: proc(path: cstring) ---
	// R_TRUE when `path` is a file registered with Rfile_register_memory.
	Rfile_is_memory :: proc(path: cstring) -> i32 ---
	// Whole files, with UTF-8 paths on every platform.
	// Returns: Rfile_read: the content followed by a '\0', to release with
	// Rmemory_free; NULL when the file cannot be read. `size` may be NULL.
	Rfile_read :: proc(path: cstring, size: ^uint) -> cstring ---
	Rfile_write :: proc(path: cstring, data: rawptr, size: uint) -> i32 ---
	Rfile_browser_create :: proc() -> Rid ---
	Rfile_browser_set_mode :: proc(browser: Rid, mode: RFileBrowserMode) ---
	// Shows a folder. "~" is the home of the user. Returns R_FALSE when it cannot be read.
	Rfile_browser_set_directory :: proc(browser: Rid, path: cstring) -> i32 ---
	Rfile_browser_get_directory :: proc(browser: Rid) -> cstring ---
	// Reads the folder again.
	Rfile_browser_refresh :: proc(browser: Rid) ---
	// Kinds of files the user chooses from; the first one is active. The strings are copied.
	Rfile_browser_set_filters :: proc(browser: Rid, filters: ^RFileFilter, count: i32) ---
	Rfile_browser_set_show_hidden :: proc(browser: Rid, show: i32) ---
	// R_TRUE lets several files be selected (Ctrl and Shift with a click).
	Rfile_browser_set_multiple :: proc(browser: Rid, multiple: i32) ---
	// R_FALSE hides the name field, the filter list and the buttons:
	// what remains is an explorer to put in a side panel. Shown by default.
	Rfile_browser_set_controls_visible :: proc(browser: Rid, visible: i32) ---
	// R_FALSE hides the list of places on the left.
	Rfile_browser_set_places_visible :: proc(browser: Rid, visible: i32) ---
	// Name typed in the name field (save mode).
	Rfile_browser_set_file_name :: proc(browser: Rid, name: cstring) ---
	// Added to a name typed without extension, in save mode.
	Rfile_browser_set_default_extension :: proc(browser: Rid, extension: cstring) ---
	// Paths of the selected entries. The strings are valid until the selection or the folder changes.
	Rfile_browser_get_selection_count :: proc(browser: Rid) -> i32 ---
	Rfile_browser_get_selection :: proc(browser: Rid, index: i32) -> cstring ---
	// Selects an entry of the folder by its name.
	Rfile_browser_select :: proc(browser: Rid, name: cstring) ---
	Rfile_browser_set_accepted_callback :: proc(browser: Rid, callback: RFileBrowserCallback, user_data: rawptr) ---
	// Called by the Cancel button.
	Rfile_browser_set_cancelled_callback :: proc(browser: Rid, callback: RFileBrowserCallback, user_data: rawptr) ---
	// Called when the selection changes; `path` is the entry under the cursor, or NULL.
	Rfile_browser_set_selection_changed_callback :: proc(browser: Rid, callback: RFileBrowserCallback, user_data: rawptr) ---
	// Returns: R_TRUE when the system took the notification.
	Rnotification_show :: proc(title: cstring, body: cstring, urgency: RNotificationUrgency) -> i32 ---
	// R_FALSE when the platform has no notifications (Rnotification_show then always uses the fallback).
	Rnotification_is_supported :: proc() -> i32 ---
	// Called with the notifications the system could not show; NULL: none.
	Rnotification_set_fallback :: proc(callback: RNotificationFallback, user_data: rawptr) ---
	// Called when the user clicks a notification of Rnotification_show
	// (on the UI thread; the application may then bring its window to the
	// front: the system lets it). It is the last one shown that was clicked:
	// the application remembers what it was about. NULL: none.
	// Windows only for now (elsewhere it is never called).
	Rnotification_set_clicked_callback :: proc(callback: RNotificationClickedCallback, user_data: rawptr) ---
	// Shows a texture. R_INVALID_ID shows nothing.
	Rviewport_set_texture :: proc(viewport: Rid, texture: Rid) ---
	// How the texture is fitted in the widget. Default: R_STRETCH_FILL.
	Rviewport_set_stretch :: proc(viewport: Rid, stretch: RStretch) ---
	// R_TRUE flips the texture vertically (OpenGL framebuffers are bottom-up).
	Rviewport_set_flip_y :: proc(viewport: Rid, flip: i32) ---
	Rviewport_set_draw_callback :: proc(viewport: Rid, callback: RViewportDrawCallback, user_data: rawptr) ---
	// Creates (or destroys) the native child window of the viewport.
	// The viewport must already be in a window.
	// Returns: R_TRUE on success.
	Rviewport_set_native :: proc(viewport: Rid, enabled: i32) -> i32 ---
	// Fills `native` with the handles of the child window.
	Rviewport_get_native_handle :: proc(viewport: Rid, native: ^RNativeWindow) -> i32 ---
	// Size of the viewport in pixels after the last layout.
	Rviewport_get_pixel_size :: proc(viewport: Rid, width: ^i32, height: ^i32) ---
	Rviewport_set_resize_callback :: proc(viewport: Rid, callback: RViewportResizeCallback, user_data: rawptr) ---
	// R_TRUE redraws the window every frame while the viewport is
	// visible (game view). R_FALSE redraws only on Rwidget_invalidate (default).
	Rviewport_set_realtime :: proc(viewport: Rid, realtime: i32) ---
	Rcustom_set_paint_callback :: proc(widget: Rid, callback: RPaintCallback, user_data: rawptr) ---
	// Reports the desired size of the widget. Without it the widget asks for 0 x 0.
	Rcustom_set_measure_callback :: proc(widget: Rid, callback: RMeasureCallback, user_data: rawptr) ---
	// Filled rectangle with rounded corners (radius 0 = sharp).
	Rdraw_rect :: proc(painter: ^RPainter, x: f32, y: f32, width: f32, height: f32, radius: f32, r: f32, g: f32, b: f32, a: f32) ---
	// Outline of a rounded rectangle, drawn inside its bounds.
	Rdraw_rect_outline :: proc(painter: ^RPainter, x: f32, y: f32, width: f32, height: f32, radius: f32, thickness: f32, r: f32, g: f32, b: f32, a: f32) ---
	Rdraw_circle :: proc(painter: ^RPainter, center_x: f32, center_y: f32, radius: f32, r: f32, g: f32, b: f32, a: f32) ---
	// Anti-aliased line with round caps.
	Rdraw_line :: proc(painter: ^RPainter, x0: f32, y0: f32, x1: f32, y1: f32, thickness: f32, r: f32, g: f32, b: f32, a: f32) ---
	// Anti-aliased cubic Bezier curve from (x0, y0) to (x1, y1), pulled by two control points.
	Rdraw_curve :: proc(painter: ^RPainter, x0: f32, y0: f32, cx0: f32, cy0: f32, cx1: f32, cy1: f32, x1: f32, y1: f32, thickness: f32, r: f32, g: f32, b: f32, a: f32) ---
	Rdraw_triangle :: proc(painter: ^RPainter, x0: f32, y0: f32, x1: f32, y1: f32, x2: f32, y2: f32, r: f32, g: f32, b: f32, a: f32) ---
	// Texture stretched to a rectangle and multiplied by a tint color.
	Rdraw_image :: proc(painter: ^RPainter, texture: Rid, x: f32, y: f32, width: f32, height: f32, r: f32, g: f32, b: f32, a: f32) ---
	// Draws UTF-8 text; (x, y) is the top-left corner of the first line.
	// font R_INVALID_ID uses the theme font.
	// size 0 uses R_METRIC_FONT_SIZE.
	Rdraw_text :: proc(painter: ^RPainter, font: Rid, size: f32, x: f32, y: f32, text: cstring, r: f32, g: f32, b: f32, a: f32) ---
	// Rounded rectangle of glass. It shows what was drawn before it,
	// frosted and bent along its rim, under a tint: (r, g, b) is the color of
	// the tint and `a` its strength (0 = clear glass, 1 = opaque color).
	// Works with any theme. Needs a backend that implements glass (the Vulkan
	// backend does).
	Rdraw_glass :: proc(painter: ^RPainter, x: f32, y: f32, width: f32, height: f32, radius: f32, r: f32, g: f32, b: f32, a: f32) ---
	// Animates a value from the paint callback of a custom widget.
	// Moves the spring toward `target` by the time elapsed since the last
	// frame and returns its value. While it moves, the window keeps drawing
	// frames by itself. The first call puts the spring on its target.
	//
	//     static RSpring size;                                  (zeroed)
	//     float s = Rdraw_spring(painter, &size, hovered ? 1.0f : 0.0f, R_MOTION_BOUNCY);
	//     Rdraw_circle(painter, cx, cy, 20.0f + 8.0f * s, ...);
	Rdraw_spring :: proc(painter: ^RPainter, spring: ^RSpring, target: f32, motion: RMotion) -> f32 ---
	// Pushes a spring away from its target (units per second): it bounces back.
	Rspring_kick :: proc(spring: ^RSpring, velocity: f32) ---
	// Puts a spring at rest on a value, without animation.
	Rspring_set :: proc(spring: ^RSpring, value: f32) ---
	// Restricts the following draws to a rectangle, intersected with the current one.
	Rdraw_push_clip_rect :: proc(painter: ^RPainter, x: f32, y: f32, width: f32, height: f32) ---
	Rdraw_pop_clip_rect :: proc(painter: ^RPainter) ---
	// A shader from SPIR-V code (a fragment shader written with
	// raven_effect.glsl, as in a .spv file): `size` bytes, copied. R_INVALID_ID
	// when the code is not SPIR-V; a backend that refuses it reports an error
	// and the effect is drawn as a rectangle.
	Rshader_create :: proc(spirv: rawptr, size: uint, flags: u32) -> Rid ---
	// The same, from a .spv file (UTF-8 path, relative to the current folder).
	Rshader_create_from_file :: proc(path: cstring, flags: u32) -> Rid ---
	Rshader_destroy :: proc(shader: Rid) ---
	Rshader_is_valid :: proc(shader: Rid) -> i32 ---
	Rshader_get_flags :: proc(shader: Rid) -> u32 ---
	// A built-in effect, created the first time it is asked for.
	Reffect_get :: proc(effect: REffect) -> Rid ---
	// A built-in effect by its name ("aurora", "shimmer", "neon", "ripple", "plasma", "grain", "spotlight", "mesh"), or R_INVALID_ID.
	Reffect_find :: proc(name: cstring) -> Rid ---
	Reffect_get_name :: proc(effect: REffect) -> cstring ---
	// Its colors and parameters when nothing else is given (Rshader_look_init uses them).
	Reffect_get_defaults :: proc(effect: REffect, look: ^RShaderLook) ---
	// White to white, no parameter, no texture, square corners, animated when the shader is.
	Rshader_look_init :: proc(look: ^RShaderLook) ---
	// Fills a look with the defaults of the shader when it is a built-in effect (else Rshader_look_init).
	Rshader_look_defaults :: proc(shader: Rid, look: ^RShaderLook) ---
	// From a paint callback: draws the effect over a rectangle of the
	// widget (logical units, from its top left corner).
	Rdraw_shader :: proc(painter: ^RPainter, shader: Rid, x: f32, y: f32, width: f32, height: f32, look: ^RShaderLook) ---
	// The background of a border becomes an effect (its Background,
	// if any, is drawn under it; its child over it). R_INVALID_ID removes it.
	// XML: Effect="aurora" or Effect="file.spv", EffectColor, EffectColor2,
	// EffectParams="a,b,c,d", EffectAnimate, EffectBackdrop (for a file).
	Rborder_set_effect :: proc(border: Rid, shader: Rid, look: ^RShaderLook) ---
	Rborder_get_effect :: proc(border: Rid) -> Rid ---
	// Creates a window from a document whose root element is <Window>.
	// xml  UTF-8 text. It does not need to be null-terminated.
	// size Size of the text in bytes.
	// Returns: The window, or R_INVALID_ID. Parse errors are reported with
	//         their line and column through the error callback (R_ERROR_XML).
	Rwindow_create_from_xml :: proc(xml: cstring, size: uint) -> Rid ---
	Rwindow_create_from_xml_file :: proc(path: cstring) -> Rid ---
	// Creates a widget tree from a document whose root element is a
	// widget. The caller owns the returned widget until it is added to a parent.
	Rwidget_create_from_xml :: proc(xml: cstring, size: uint) -> Rid ---
	Rwidget_create_from_xml_file :: proc(path: cstring) -> Rid ---
	// A folder looked into after the one of the document (see above),
	// and the first one for the documents given from memory. NULL or "": none.
	Rxml_set_base_path :: proc(path: cstring) ---
	// Turns auto-loading on or off (see above). It is on by default;
	// the environment variable RAVEN_AUTO_LOAD=0 turns it off without
	// rebuilding. Turned off, the register functions are the only way in;
	// the places where paths are looked for stay the same.
	Rxml_set_auto_load :: proc(enabled: i32) ---
	Rxml_get_auto_load :: proc() -> i32 ---
	// Turns live reload on or off. It is on by default; the
	// environment variable RAVEN_LIVE_RELOAD=0 turns it off without
	// rebuilding. When off, files are never looked at again.
	Rxml_set_live_reload :: proc(enabled: i32) ---
	Rxml_get_live_reload :: proc() -> i32 ---
	Rxml_set_reload_callback :: proc(callback: RXmlReloadCallback, user_data: rawptr) ---
	// Looks at the files now, without waiting for the next look
	// (there are five per second), even when live reload is off.
	// Returns: The number of files found changed.
	Rxml_check_files :: proc() -> i32 ---
	// Reads the <Widget Name="..."> declarations of a document (its
	// root, or the children of a <Widgets> root) so that the tags they
	// declare can be used in the documents loaded afterwards.
	// Returns: The number of tags declared.
	Rxml_register_widgets :: proc(xml: cstring, size: uint) -> i32 ---
	Rxml_register_widgets_from_file :: proc(path: cstring) -> i32 ---
	Rxml_unregister_widget :: proc(name: cstring) ---
	Rxml_is_widget_registered :: proc(name: cstring) -> i32 ---
	// The declared tags, for tools that list them.
	Rxml_get_registered_widget_count :: proc() -> i32 ---
	Rxml_get_registered_widget_name :: proc(index: i32) -> cstring ---
	// Registers a tag (or replaces what it was registered with).
	// Returns: R_FALSE when the name is the one of a widget of Raven, or when too many tags are registered (64).
	Rxml_register_tag :: proc(tag: cstring, create: RXmlTagCreateCallback, set_attribute: RXmlTagAttributeCallback, user_data: rawptr) -> i32 ---
	Rxml_unregister_tag :: proc(tag: cstring) ---
	Rxml_is_tag_registered :: proc(tag: cstring) -> i32 ---
	// The path of a file named in a document: as the attributes
	// Source of Raven read it, looked for where files are looked for (see
	// Rxml_set_base_path): the folder of the document being loaded (while it
	// loads) and its subfolders, the base path, the current folder, PATH.
	// When no place holds it, the path it would have next to the document
	// (else in the base path). For the attributes of the tags made by code
	// that name files.
	// Returns: The path; valid until the next call.
	Rxml_resolve_path :: proc(path: cstring) -> cstring ---
	// Sets a property from text, with the names and value syntax of the
	// XML attributes. Handy for scripting and data-driven tools.
	// Returns: R_TRUE when the attribute exists for this widget and the value is valid.
	Rwidget_set_attribute :: proc(widget: Rid, name: cstring, value: cstring) -> i32 ---
	// A new document with one element.
	Rxml_document_create :: proc(root_tag: cstring) -> Rid ---
	// Reads a text into a document. Comments are kept.
	// error When not NULL, a mistake in the text is described there
	// and nothing is reported; when NULL it goes to the error callback.
	// Returns: The document, or R_INVALID_ID.
	Rxml_document_parse :: proc(xml: cstring, size: uint, error: ^RXmlError) -> Rid ---
	// Reads a file. The paths written in it are relative to its folder.
	Rxml_document_load :: proc(path: cstring) -> Rid ---
	// Releases a document and its nodes. What was built from it lives on.
	Rxml_document_destroy :: proc(document: Rid) ---
	Rxml_document_is_valid :: proc(document: Rid) -> i32 ---
	Rxml_document_get_root :: proc(document: Rid) -> Rid ---
	// Writes the document as text, one element per line, indented
	// with tabs. Rxml_node_get_line then says where each node was written.
	// Returns: The text, valid until the next call for this document or its destruction.
	Rxml_document_to_string :: proc(document: Rid) -> cstring ---
	Rxml_document_save :: proc(document: Rid, path: cstring) -> i32 ---
	// Builds the widgets the document describes, as
	// Rwidget_create_from_xml does. When the root is a <Window>, builds what
	// the window would show (R_INVALID_ID when it is empty).
	Rxml_document_create_widget :: proc(document: Rid) -> Rid ---
	// Builds the window the document describes, as Rwindow_create_from_xml does.
	Rxml_document_create_window :: proc(document: Rid) -> Rid ---
	Rxml_node_is_valid :: proc(node: Rid) -> i32 ---
	Rxml_node_get_document :: proc(node: Rid) -> Rid ---
	// Returns: The tag of an element, "" for a comment.
	Rxml_node_get_tag :: proc(node: Rid) -> cstring ---
	Rxml_node_set_tag :: proc(node: Rid, tag: cstring) ---
	Rxml_node_is_comment :: proc(node: Rid) -> i32 ---
	// The text inside an element (<Label>this</Label>), or the text of a comment.
	Rxml_node_get_text :: proc(node: Rid) -> cstring ---
	Rxml_node_set_text :: proc(node: Rid, text: cstring) ---
	Rxml_node_get_parent :: proc(node: Rid) -> Rid ---
	Rxml_node_get_child_count :: proc(node: Rid) -> i32 ---
	Rxml_node_get_child :: proc(node: Rid, index: i32) -> Rid ---
	// Returns: The place of a node among the children of its parent; 0 for the root.
	Rxml_node_get_index :: proc(node: Rid) -> i32 ---
	// index Where among the children; -1 for the end.
	// Returns: The new node; R_INVALID_ID (R_ERROR_XML) when it would be nested
	//         deeper than a document can be: 256 levels under the root, as
	//         when a text is read. The same holds for the functions below.
	Rxml_node_add_child :: proc(parent: Rid, tag: cstring, index: i32) -> Rid ---
	Rxml_node_add_comment :: proc(parent: Rid, text: cstring, index: i32) -> Rid ---
	// Reads a piece of XML (one element and what it contains) and
	// adds it: what a palette, a paste or a template inserts.
	Rxml_node_add_xml :: proc(parent: Rid, xml: cstring, size: uint, index: i32) -> Rid ---
	// Removes a node and what it contains. The root stays.
	Rxml_node_remove :: proc(node: Rid) ---
	// Moves a node, with what it contains, under another parent of
	// the same document or elsewhere under its own.
	// index Where among the children of the new parent, counted once the node has left its place; -1 for the end.
	// Returns: R_FALSE when it cannot be done (the root, a parent that is inside the node, or a place too deep).
	Rxml_node_move :: proc(node: Rid, new_parent: Rid, index: i32) -> i32 ---
	// Copies a node and what it contains, right after it. Returns the copy.
	Rxml_node_duplicate :: proc(node: Rid) -> Rid ---
	// The node as text. Valid until the next call for this document.
	Rxml_node_to_string :: proc(node: Rid) -> cstring ---
	Rxml_node_get_attribute_count :: proc(node: Rid) -> i32 ---
	Rxml_node_get_attribute_name :: proc(node: Rid, index: i32) -> cstring ---
	Rxml_node_get_attribute_value :: proc(node: Rid, index: i32) -> cstring ---
	// Returns: The value of an attribute (names are compared without case), or NULL when the node does not have it.
	Rxml_node_get_attribute :: proc(node: Rid, name: cstring) -> cstring ---
	// Sets an attribute, added after the others when it is new. A NULL value removes it.
	Rxml_node_set_attribute :: proc(node: Rid, name: cstring, value: cstring) ---
	// Returns: The line of the node, starting at 1, in the text the document
	// was read from or last written to; 0 for a node added since.
	Rxml_node_get_line :: proc(node: Rid) -> i32 ---
	// Returns: The widget (the window, for a <Window>) the node became the
	// last time the document was built, or R_INVALID_ID: never built,
	// destroyed since, or not a widget (an <Item>, a comment). A tag declared
	// with <Widget> gives the widget it stands for.
	Rxml_node_get_widget :: proc(node: Rid) -> Rid ---
	// Returns: The node a widget was built from, or R_INVALID_ID: not built
	// from a document, or the document is gone. The widgets inside a tag
	// declared with <Widget> all come from the node where the tag is used.
	Rwidget_get_xml_node :: proc(widget: Rid) -> Rid ---
	// The tags of the library, <Window> included. Those declared with <Widget> are listed by Rxml_get_registered_widget_name.
	Rxml_get_tag_count :: proc() -> i32 ---
	Rxml_get_tag_name :: proc(index: i32) -> cstring ---
	// Returns: One line about a tag, "" when there is nothing to say.
	Rxml_get_tag_description :: proc(tag: cstring) -> cstring ---
	// Returns: 0: no child widget; 1: a single one; 2: any number. Declared and unknown tags give 2.
	Rxml_get_tag_children :: proc(tag: cstring) -> i32 ---
	// The attributes of a tag: its own first, then those every widget
	// has, then those of its place in its parent. For a tag declared with
	// <Widget>, its own are its parameters.
	Rxml_get_attribute_count :: proc(tag: cstring) -> i32 ---
	Rxml_get_attribute_info :: proc(tag: cstring, index: i32, info: ^RXmlAttributeInfo) -> i32 ---
	// Loads a CSV table of translations from a file. Returns the number of keys read, -1 on an error.
	Rlocale_load_csv :: proc(path: cstring) -> i32 ---
	// Same, from text in memory (UTF-8).
	Rlocale_load_csv_from_memory :: proc(csv: cstring, size: uint) -> i32 ---
	// Forgets every table.
	Rlocale_clear :: proc() ---
	// Adds or replaces one text of one language.
	Rlocale_set_text :: proc(language: cstring, key: cstring, text: cstring) ---
	// Number of languages in the tables, and their codes, in the order of the columns.
	Rlocale_get_language_count :: proc() -> i32 ---
	Rlocale_get_language_at :: proc(index: i32) -> cstring ---
	// Chooses the language: an exact code ("pt-BR"), or its first part
	// ("pt") when the tables have no such column. Every widget that shows a key
	// is updated, and the layout of the windows follows its direction (an
	// Arabic, Hebrew, Persian or Urdu language is laid out from right to left,
	// see Rset_layout_direction). Returns R_FALSE when no column matches (the
	// language is set all the same: the texts fall back).
	Rlocale_set_language :: proc(code: cstring) -> i32 ---
	Rlocale_get_language :: proc() -> cstring ---
	// The language of the user, from the system ("fr-FR", "en-US"...).
	Rlocale_get_system_language :: proc() -> cstring ---
	// The language whose texts are shown when the current one has none (default: the first column).
	Rlocale_set_fallback_language :: proc(code: cstring) ---
	// R_TRUE when the language is written from right to left.
	Rlocale_is_rtl :: proc() -> i32 ---
	// Called after the language or the tables changed (the widgets are already updated).
	Rlocale_set_changed_callback :: proc(callback: RLocaleChangedCallback, user_data: rawptr) ---
	// The text of a key in the current language (see the fallbacks above). Never NULL.
	Rtr :: proc(key: cstring) -> cstring ---
	// R_TRUE when the key has a text in the current language or the fallback one.
	Rtr_exists :: proc(key: cstring) -> i32 ---
	// The text of a key with its {0} {1} ... replaced by the arguments
	// (translators may put them in any order). "{{" is a brace.
	Rtr_format :: proc(key: cstring, args: [^]cstring, arg_count: i32) -> cstring ---
	// The plural form of a key for a count: the key followed by the CLDR
	// category of the count in the current language (".zero", ".one", ".two",
	// ".few", ".many", ".other"), else ".other", else the key itself. {n} is
	// replaced by the count, written in the conventions of the language.
	Rtr_plural :: proc(key: cstring, count: i32) -> cstring ---
	// Makes a widget show a key: an attribute ("Text", "Tooltip",
	// "Title", "Placeholder"... any attribute that takes a text) is set to the
	// translation now and every time the language changes. NULL or "" stops it.
	// Same as Rwidget_set_attribute(widget, attribute, "@key").
	Rwidget_set_text_key :: proc(widget: Rid, attribute: cstring, key: cstring) ---
	// A number written in the conventions of the language: decimal separator and digit grouping ("1 234,5" in French).
	Rlocale_format_number :: proc(value: f64, decimals: i32) -> cstring ---
	// Size of every text, 1 = as designed (0.5 to 4). Controls grow with their text.
	Raccessibility_set_text_scale :: proc(scale: f32) ---
	Raccessibility_get_text_scale :: proc() -> f32 ---
	// Themes of high contrast: black and white, yellow focus, thick
	// borders (the dark one, or the light one when the theme in use is light).
	// R_FALSE goes back to the theme used before.
	Raccessibility_set_high_contrast :: proc(enabled: i32) ---
	Raccessibility_get_high_contrast :: proc() -> i32 ---
	// R_TRUE: the focus ring is always shown (not only after the keyboard was used), and thicker.
	Raccessibility_set_focus_always_visible :: proc(enabled: i32) ---
	Raccessibility_get_focus_always_visible :: proc() -> i32 ---
	// Reduced motion, for the whole application: R_TRUE and nothing
	// moves by itself. Changes are immediate (no spring, no slide, no bounce,
	// no transition), and the effects (raven_shader.h) stand still; they still
	// follow the pointer. Same as Rset_animations_enabled(R_FALSE).
	Raccessibility_set_reduce_motion :: proc(enabled: i32) ---
	Raccessibility_get_reduce_motion :: proc() -> i32 ---
	// A filter on the colors of the interface (pictures and viewports keep theirs).
	Raccessibility_set_color_filter :: proc(filter: RColorFilter) ---
	Raccessibility_get_color_filter :: proc() -> RColorFilter ---
	Raccessibility_get_system_settings :: proc(settings: ^RAccessibilitySettings) ---
	// Applies the settings of the system (text size, contrast, motion).
	Raccessibility_follow_system :: proc() ---
	// What the screen reader says of the widget; NULL: what it shows (or the label before it).
	Rwidget_set_accessible_name :: proc(widget: Rid, name: cstring) ---
	// The name the screen reader uses (given, or found). Valid until the next call.
	Rwidget_get_accessible_name :: proc(widget: Rid) -> cstring ---
	// A longer explanation, read after the name.
	Rwidget_set_accessible_description :: proc(widget: Rid, description: cstring) ---
	Rwidget_set_accessible_role :: proc(widget: Rid, role: RAccessibleRole) ---
	// The role the screen reader sees (given, or the one of the type).
	Rwidget_get_accessible_role :: proc(widget: Rid) -> RAccessibleRole ---
	// R_TRUE: hidden from screen readers, with what it contains (decoration).
	Rwidget_set_accessible_hidden :: proc(widget: Rid, hidden: i32) ---
	// The widget is named by another one (a label): its text is the name.
	Rwidget_set_accessible_labelled_by :: proc(widget: Rid, label: Rid) ---
	// Explicit screen-reader press action, including for custom widgets.
	// NULL removes the override. Does not change pointer or keyboard handling.
	// Disabled widgets (including disabled ancestors) cannot be pressed.
	Rwidget_set_accessible_action :: proc(widget: Rid, press: RAccessiblePressCallback, user_data: rawptr) ---
	// Override the checked state: 1 checked, 0 unchecked, -1 restores the
	// widget's default (not checkable for a custom widget). The application owns
	// this state; a press callback updates it when appropriate.
	Rwidget_set_accessible_checked :: proc(widget: Rid, checked: i32) ---
	// Has a message read (a save that ended, an error). `assertive`: interrupts what is being read.
	Raccessibility_announce :: proc(text: cstring, assertive: i32) ---
	// R_TRUE when a screen reader is reading the windows of the application.
	Raccessibility_is_screen_reader_active :: proc() -> i32 ---
	// Makes the next Rinit use the host platform. The structure is copied; NULL restores the default.
	Rhost_platform_register :: proc(desc: ^RHostPlatformDesc) ---
	// Returns: R_TRUE while the running library uses the host platform.
	Rhost_platform_is_active :: proc() -> i32 ---
	// Size of a host window in pixels, and pixels per logical unit.
	Rhost_window_set_size :: proc(window: Rid, width: i32, height: i32, scale: f32) ---
	Rhost_inject_mouse_move :: proc(window: Rid, x: i32, y: i32, modifiers: u32) ---
	Rhost_inject_mouse_button :: proc(window: Rid, x: i32, y: i32, button: RMouseButton, pressed: i32, modifiers: u32) ---
	Rhost_inject_mouse_wheel :: proc(window: Rid, wheel_x: f32, wheel_y: f32, modifiers: u32) ---
	// The pointer left the window: hover states end.
	Rhost_inject_mouse_leave :: proc(window: Rid) ---
	// repeat R_TRUE for a key held down (auto-repeat).
	Rhost_inject_key :: proc(window: Rid, key: RKey, pressed: i32, modifiers: u32, repeat: i32) ---
	// One typed Unicode code point.
	Rhost_inject_text :: proc(window: Rid, codepoint: u32) ---
	// The window gained or lost the keyboard. Losing it closes popups and drops captures.
	Rhost_inject_focus :: proc(window: Rid, focused: i32) ---
	// The user asked to close the window (its close callback decides).
	Rhost_inject_close :: proc(window: Rid) ---
}

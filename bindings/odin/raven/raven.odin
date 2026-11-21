// Raven for Odin.
//
// Generated from the C headers by tools/bindgen (Raven 0.3): do not edit.
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
//       r.Rinit(r.R_BACKEND_DEFAULT)
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
RAVEN_VERSION_MINOR :: 3
RAVEN_VERSION_PATCH :: 0
RAVEN_API_VERSION :: "0.3"
R_FALSE :: 0
R_TRUE :: 1
R_INVALID_ID : Rid : 0xFFFFFFFF

RActionContext :: struct {}
RPainter :: struct {}

RBackend :: i32
// The renderer of the platform: WebGL in browser builds, Metal on macOS and iOS when it is compiled, Vulkan otherwise.
R_BACKEND_DEFAULT :: 0
// Built-in Vulkan renderer.
R_BACKEND_VULKAN :: 1
// Renders nothing. Used for headless runs and unit tests.
R_BACKEND_NULL :: 2
// Backend supplied with Rbackend_register (see raven_backend.h).
R_BACKEND_CUSTOM :: 3
// Browser renderer (Emscripten, WebGL 2).
R_BACKEND_WEBGL :: 4
// Built-in Metal renderer (macOS, iOS).
R_BACKEND_METAL :: 5

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
// A diagnostic of Raven Script (raven_script.h): the text is the whole message, "error[RS102]: ...".
R_ERROR_SCRIPT :: 0x007C

RSeverity :: i32
R_SEVERITY_WEAK_WARNING :: 0x0080
R_SEVERITY_WARNING :: 0x0081
R_SEVERITY_ERROR :: 0x0082
R_SEVERITY_FATAL :: 0x0083

// What the system does with the application, on iOS and Android (desktop
// systems send none of these). A pause is not always followed by
// R_APP_DID_PAUSE: pulling down the notifications (iOS) or a dialog of the
// system in front (Android) gives R_APP_WILL_PAUSE, then R_APP_DID_RESUME.
// At the start, R_APP_WILL_RESUME and R_APP_DID_RESUME may come too.
//
//   leaving the front   R_APP_WILL_PAUSE    still seen: save what must not be lost
//   in the background   R_APP_DID_PAUSE     not seen, no GPU work: Raven stops drawing
//   coming back         R_APP_WILL_RESUME   Raven draws again
//   in front again      R_APP_DID_RESUME    takes input again
RAppEvent :: i32
// The application is about to leave the front (home, app switcher, a call). Save the state now.
R_APP_WILL_PAUSE :: 0x0B80
// The application is in the background: Raven has stopped drawing, and waited until the GPU was idle.
R_APP_DID_PAUSE :: 0x0B81
// The application is coming back to the front: Raven draws again.
R_APP_WILL_RESUME :: 0x0B82
// The application is in front and takes input again.
R_APP_DID_RESUME :: 0x0B83
// The system is low on memory: free what can be made again. Raven then drops its own caches and trims its heap.
R_APP_LOW_MEMORY :: 0x0B84
// The application is about to end (Android: the activity is finishing; its window is closed right after).
R_APP_WILL_TERMINATE :: 0x0B85

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

// What the application that takes a drag does with it (Rdrag_begin_external): a bitmask of what is allowed, or the one done.
RDragOperation :: i32
// Nothing was taken: dropped where nothing accepted it, or cancelled.
R_DRAG_OPERATION_NONE :: 0
// The target made a copy: the source keeps its data.
R_DRAG_OPERATION_COPY :: 1
// The target took the data: the source removes it (a file the file manager moved...).
R_DRAG_OPERATION_MOVE :: 2
// The target made a link, an alias or a shortcut to it.
R_DRAG_OPERATION_LINK :: 4

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
// The matches of a search in a text editor (usually see-through).
R_COLOR_SEARCH_MATCH :: 0x0124
// The selected match of a search in a text editor.
R_COLOR_SEARCH_CURRENT :: 0x0125
// Wavy underline of the misspelled words of a TextInput or a TextEditor (spell checking).
R_COLOR_SPELLING :: 0x0126
// Diff view: the background of an added line and of a removed line (see-through: the background shows).
R_COLOR_DIFF_ADDED :: 0x0127
R_COLOR_DIFF_REMOVED :: 0x0128
// Diff view: the words that changed inside an added line and inside a removed line, over the color of the line.
R_COLOR_DIFF_ADDED_WORD :: 0x0129
R_COLOR_DIFF_REMOVED_WORD :: 0x012A

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
// The charts of dashboards: pie, donut, gauge, heatmap, sparkline (RChartKind).
R_WIDGET_CHART :: 0x0234
// A map of slippy-map tiles given by the application (or read from files): panned, zoomed, with markers.
R_WIDGET_MAP_VIEW :: 0x0235
// Pages shown one at a time and swiped (a page view), with dots and arrows.
R_WIDGET_CAROUSEL :: 0x0236
// A web page shown by the engine of the system (WKWebView) in a view kept over the widget.
R_WIDGET_SYSTEM_WEB_VIEW :: 0x0237
// The differences between two texts or a patch: unified or side by side, folded, virtualized (raven_tools.h).
R_WIDGET_DIFF_VIEW :: 0x0238

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

// Where a tile of a MapView stands (Rmap_view_get_tile_state).
RMapTileState :: i32
// Not in the view's cache: never asked for, no longer wanted, or evicted.
R_MAP_TILE_NONE :: 0x0600
// Asked for (the tile callback was called, or the file is being read): waiting for its pixels.
R_MAP_TILE_PENDING :: 0x0601
// Its pixels are in the cache.
R_MAP_TILE_LOADED :: 0x0602
// The application said it has none (Rmap_view_set_tile_failed), or its file could not be read.
R_MAP_TILE_FAILED :: 0x0603

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

// What a Chart draws.
RChartKind :: i32
// Slices of a whole, as parts of a disc (the default).
R_CHART_PIE :: 0x0590
// Slices of a whole, as parts of a ring; its middle shows the total, or the slice under the pointer.
R_CHART_DONUT :: 0x0591
// One value within a range, on an arc; bands color parts of the range.
R_CHART_GAUGE :: 0x0592
// A grid of values, each cell colored from the low color to the high one.
R_CHART_HEATMAP :: 0x0593
// A small line of values without axes, to sit in a tile or a cell of a table.
R_CHART_SPARKLINE :: 0x0594

// What happens in a SystemWebView (Rsystem_web_view_set_event_callback).
RSystemWebViewEvent :: i32
// The view shows another address (Rsystem_web_view_get_url).
R_SYSTEM_WEB_VIEW_NAVIGATED :: 0x0B90
// The title of the page changed (Rsystem_web_view_get_title).
R_SYSTEM_WEB_VIEW_TITLE :: 0x0B91
// A page starts loading.
R_SYSTEM_WEB_VIEW_LOADING :: 0x0B92
// The page and what it refers to are loaded.
R_SYSTEM_WEB_VIEW_LOADED :: 0x0B93
// The page could not be loaded, or the engine showing it stopped (Rsystem_web_view_get_error).
R_SYSTEM_WEB_VIEW_FAILED :: 0x0B94

// How the rows of a Table are selected (Rtable_set_selection_mode).
RTableSelectionMode :: i32
// One row at a time (the default).
R_TABLE_SELECTION_SINGLE :: 0x0BC0
// Any number of rows: Ctrl/Cmd+click, Shift+click, Shift+arrows, Space, Ctrl/Cmd+A.
R_TABLE_SELECTION_MULTIPLE :: 0x0BC1
// The user selects nothing (the rows are only shown; cells may still be edited and checked).
R_TABLE_SELECTION_NONE :: 0x0BC2

// What the cells of a column show (Rtable_set_column_kind).
RTableColumnKind :: i32
// Their text (the default).
R_TABLE_COLUMN_TEXT :: 0x0BD0
// A check box, checked when the text is "1", "true" or "yes". The user
// toggles it with a click or Space unless the column is made not
// editable; the cell edited callback is then called with "1" or "0".
R_TABLE_COLUMN_CHECK :: 0x0BD1
// A progress bar, from a number 0..1 or a percentage ("45%"); an empty cell shows nothing.
R_TABLE_COLUMN_PROGRESS :: 0x0BD2

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

// Options of a search (Rtext_editor_set_search), a bitmask.
RSearchFlags :: i32
// Upper and lower case differ. Without it the case is ignored, for the
// letters of ASCII, Latin-1, the Latin letters of Unicode, Greek,
// Cyrillic and Armenian.
R_SEARCH_MATCH_CASE :: 1
// A match is a whole word: "for" does not find the start of "format".
R_SEARCH_WHOLE_WORD :: 2

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

RDockFloatMode :: i32
R_DOCK_FLOAT_WINDOW :: 0
R_DOCK_FLOAT_IN_WINDOW :: 1

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

RDiffViewMode :: i32
// One column: the removed lines, then the added lines, between the lines kept.
R_DIFF_VIEW_UNIFIED :: 0x0C50
// Two columns: the left text and the right text side by side, aligned.
R_DIFF_VIEW_SPLIT :: 0x0C51

RDiffSide :: i32
// The text before (the old one, "a/", the lines removed).
R_DIFF_LEFT :: 0x0C54
// The text after (the new one, "b/", the lines added).
R_DIFF_RIGHT :: 0x0C55

RDiffRowKind :: i32
// A line that is on both sides.
R_DIFF_ROW_SAME :: 0x0C58
// A line of the left text only.
R_DIFF_ROW_REMOVED :: 0x0C59
// A line of the right text only.
R_DIFF_ROW_ADDED :: 0x0C5A
// Split mode: a removed line facing the added line that replaces it.
R_DIFF_ROW_CHANGED :: 0x0C5B
// Unchanged lines folded away ("12 unchanged lines"); Rdiff_view_expand shows them.
R_DIFF_ROW_FOLD :: 0x0C5C
// A patch: the header of a hunk ("@@ -12,7 +12,8 @@ main").
R_DIFF_ROW_HUNK :: 0x0C5D
// A patch: the names of a file.
R_DIFF_ROW_FILE :: 0x0C5E
// A note attached to the line above it (Rdiff_view_add_annotation).
R_DIFF_ROW_NOTE :: 0x0C5F

// How lines are compared (Rdiff_view_set_flags, Rdiff_unified).
RDiffFlags :: i32
// Spaces and tabs at the end of a line (and a carriage return) do not count.
R_DIFF_IGNORE_TRAILING_SPACE :: 1
// No space or tab counts, wherever it is.
R_DIFF_IGNORE_SPACE :: 2
// Upper and lower case letters are the same (A to Z).
R_DIFF_IGNORE_CASE :: 4

// How much a notification of the system asks for attention (Rnotification_show).
RNotificationUrgency :: i32
// No sound; may only go to the list of notifications.
R_NOTIFICATION_LOW :: 0x04F0
R_NOTIFICATION_NORMAL :: 0x04F1
// Stays until the user dismisses it, where the system allows it.
R_NOTIFICATION_CRITICAL :: 0x04F2

// What happened to a notification (Rnotification_set_callback).
RNotificationEvent :: i32
// The user clicked the notification itself (not one of its buttons).
R_NOTIFICATION_ACTIVATED :: 0x0C20
// The user clicked one of its buttons: `action` is the id of that button.
R_NOTIFICATION_ACTION_INVOKED :: 0x0C21
// It went away without being clicked: closed by the user, or its time ran out (where the system tells).
R_NOTIFICATION_DISMISSED :: 0x0C22

// What the notifications of the system can do here (Rnotification_supports).
RNotificationFeature :: i32
// Buttons (RNotificationDesc::actions).
R_NOTIFICATION_FEATURE_ACTIONS :: 1
// A notification shown again with the same tag replaces the first one.
R_NOTIFICATION_FEATURE_REPLACE :: 2
// Rnotification_close takes a notification away.
R_NOTIFICATION_FEATURE_CLOSE :: 4
// An image beside the text (RNotificationDesc::icon).
R_NOTIFICATION_FEATURE_ICON :: 8
// A click on the notification is reported (R_NOTIFICATION_ACTIVATED).
R_NOTIFICATION_FEATURE_ACTIVATED :: 0x0010
// Its dismissal is reported (R_NOTIFICATION_DISMISSED).
R_NOTIFICATION_FEATURE_DISMISSED :: 0x0020

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

// What the application may be allowed to use (Rpermission_request).
RPermission :: i32
R_PERMISSION_CAMERA :: 0x0C00
// Notifications of the system (Rnotification_show).
R_PERMISSION_NOTIFICATIONS :: 0x0C01
// Reading the whole photo library. Rphoto_pick needs none: the system's picker gives only what the user chose.
R_PERMISSION_PHOTOS :: 0x0C02
R_PERMISSION_MICROPHONE :: 0x0C03
// The location of the device while the application is in use.
R_PERMISSION_LOCATION :: 0x0C04

// What the user answered, or would be asked (Rpermission_get_status).
RPermissionStatus :: i32
// There is nothing to ask for here: the platform has no such permission,
// or Raven cannot ask for it on this platform. Whether the service itself
// exists is told by its own function (Rcamera_is_available,
// Rnotification_is_supported).
R_PERMISSION_UNSUPPORTED :: 0x0C10
// Not asked yet: Rpermission_request shows the question of the system.
R_PERMISSION_NOT_ASKED :: 0x0C11
// Allowed (also where the system asks nothing: notifications on Windows and Linux).
R_PERMISSION_GRANTED :: 0x0C12
// Allowed in part: some photos only (iOS, Android 14), quiet notifications (iOS provisional), the approximate location only.
R_PERMISSION_LIMITED :: 0x0C13
// Refused, and the system may ask again (Android, after a first refusal).
R_PERMISSION_DENIED :: 0x0C14
// Refused for good, or not the user's to give (parental controls, a
// profile of the company): the system shows no question any more, only
// its Settings can change it (Rpermission_open_settings).
R_PERMISSION_BLOCKED :: 0x0C15

// How a request of a picker, the camera or a share sheet ended.
RDeviceResult :: i32
// Done: the files were chosen, the photo taken, the file saved, the sheet shown.
R_DEVICE_DONE :: 0x0C20
// The user closed the screen of the system without choosing.
R_DEVICE_CANCELLED :: 0x0C21
// A permission it needs was refused (Rcamera_take_photo: the camera).
R_DEVICE_DENIED :: 0x0C22
// The platform does not have this service.
R_DEVICE_UNSUPPORTED :: 0x0C23
// Another screen of the system is still open: one at a time.
R_DEVICE_BUSY :: 0x0C24
// The system reported an error (also reported as an R_ERROR_PLATFORM warning).
R_DEVICE_FAILED :: 0x0C25

// A short vibration that answers a touch (Rhaptic_play).
RHaptic :: i32
// Impacts: something touched, snapped, landed.
R_HAPTIC_LIGHT :: 0x0C30
R_HAPTIC_MEDIUM :: 0x0C31
R_HAPTIC_HEAVY :: 0x0C32
// The selection moved (a picker wheel, a switch, a segmented control).
R_HAPTIC_SELECTION :: 0x0C33
// The outcome of an action.
R_HAPTIC_SUCCESS :: 0x0C34
R_HAPTIC_WARNING :: 0x0C35
R_HAPTIC_ERROR :: 0x0C36

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

// How the hours of a time are shown.
RClock :: i32
// As the locale does (Rlocale_uses_12_hour_clock).
R_CLOCK_AUTO :: 0x0C30
// 1 to 12, with AM and PM.
R_CLOCK_12_HOUR :: 0x0C31
// 0 to 23.
R_CLOCK_24_HOUR :: 0x0C32

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

// What a request of a service of the device asks the application for (RHostServiceRequest).
RHostService :: i32
// Files to read (Rfile_open_async): `filters`, `flags` (R_FILE_DIALOG_MULTIPLE).
R_HOST_SERVICE_OPEN_FILES :: 0x0C40
// Save `data` / `size`, or the file `path` when data is NULL, as `file_name` (Rfile_save_async, Rfile_export_async).
R_HOST_SERVICE_SAVE_FILE :: 0x0C41
// A folder (Rfile_select_folder_async).
R_HOST_SERVICE_SELECT_FOLDER :: 0x0C42
// A photo from the camera (Rcamera_take_photo); camera permission already asked.
R_HOST_SERVICE_TAKE_PHOTO :: 0x0C43
// Up to `max_count` photos (0: no limit) from the library (Rphoto_pick).
R_HOST_SERVICE_PICK_PHOTOS :: 0x0C44
// The share sheet: `text` and / or the file `path` (`mime_type`), pointing at `anchor_*` (Rshare_show).
R_HOST_SERVICE_SHARE :: 0x0C45

RGoldenStatus :: i32
// The capture matches the golden.
R_GOLDEN_MATCH :: 0x0BA0
// RAVEN_UPDATE_GOLDENS=1: the capture was written as the golden.
R_GOLDEN_UPDATED :: 0x0BA1
// Too many pixels differ: NAME.actual.png and NAME.diff.png were written.
R_GOLDEN_MISMATCH :: 0x0BA2
// The capture and the golden do not have the same size: NAME.actual.png was written.
R_GOLDEN_SIZE_MISMATCH :: 0x0BA3
// There is no golden yet: NAME.actual.png was written (run with RAVEN_UPDATE_GOLDENS=1 to keep it).
R_GOLDEN_MISSING :: 0x0BA4
// The window could not be captured, or a file could not be read or written.
R_GOLDEN_FAILED :: 0x0BA5

// Registers a function called once per frame, before the layout.
// The body of your loop does the same job; the callback is also called
// while the OS keeps the thread in a loop of its own (on Windows, while
// the user moves or resizes a window), when your loop is not running.
RFrameCallback :: #type proc "c" (delta_seconds: f64, user_data: rawptr)
// Registers a function called when the application changes state
// (RAppEvent), on the UI thread, while the system waits for it: keep it
// short. Between R_APP_DID_PAUSE and R_APP_WILL_RESUME Raven lays out but
// draws nothing, animations stand still and Rwait_frame sleeps until an
// event, a posted task or a timer: the timers of the application keep
// running, so stop those that only drive drawing. Set it after Rinit;
// NULL removes it.
RAppEventCallback :: #type proc "c" (event: RAppEvent, user_data: rawptr)
RTaskCallback :: #type proc "c" (user_data: rawptr)
RParallelRangeCallback :: #type proc "c" (begin: u32, end: u32, user_data: rawptr)
RTimerCallback :: #type proc "c" (timer: Rid, user_data: rawptr)
// Actions connect widgets to application code by name. A widget carries an
// action name (set from XML with OnClicked="file.save", or from C with
// Rwidget_set_action); when its main event fires, the function bound to that
// name is called. Names are resolved when the event fires, so actions can be
// bound before or after the widgets are created.
RActionCallback :: #type proc "c" (sender: Rid, action: cstring, user_data: rawptr)
RActionParametersCallback :: #type proc "c" (context_: ^RActionContext, user_data: rawptr)
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
// The end of a drag out of the window (Rdrag_begin_external):
// `operation` is what the target did, R_DRAG_OPERATION_NONE when nothing
// took it. Called once per drag, whatever ended it.
RDragFinishedCallback :: #type proc "c" (source: Rid, operation: RDragOperation, user_data: rawptr)
// Called on Back when no popup or dialog of Raven is open. Return
// R_TRUE when the application went back, R_FALSE when it had nowhere to
// go (the system then does its default). NULL removes it.
RBackCallback :: #type proc "c" (window: Rid, user_data: rawptr) -> i32
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
// Called when the animation stops after its last play (at the next frame), before the action of the image.
RImageFinishedCallback :: #type proc "c" (image: Rid, user_data: rawptr)
RTextChangedCallback :: #type proc "c" (text_input: Rid, text: cstring, user_data: rawptr)
RSpinBoxChangedCallback :: #type proc "c" (spin_box: Rid, value: f64, user_data: rawptr)
RComboBoxChangedCallback :: #type proc "c" (combo_box: Rid, index: i32, user_data: rawptr)
// Called each time the user changes the text of an editable or searchable combo box.
RComboBoxTextCallback :: #type proc "c" (combo_box: Rid, text: cstring, user_data: rawptr)
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
// Describes section `section`: when the sections are given (Rtile_view_set_sections,
// `first` is read) and when its header is drawn (`text`, `detail`). UI thread.
RTileSectionCallback :: #type proc "c" (tile_view: Rid, section: i32, out: ^RTileSection, user_data: rawptr)
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
// keeps the color set with Rtable_set_cell_text_color, else the theme's.
// The color is kept on a selected or hovered row. NULL removes the callback.
RTableCellColorCallback :: #type proc "c" (table: Rid, row: i32, column: i32, r: ^f32, g: ^f32, b: ^f32, a: ^f32, user_data: rawptr) -> i32
RTableEventCallback :: #type proc "c" (table: Rid, row: i32, user_data: rawptr)
// Called when the user clicks a header; `column` -1: not sorted.
RTableSortCallback :: #type proc "c" (table: Rid, column: i32, descending: i32, user_data: rawptr)
// Called when the user keeps the text typed in a cell (Enter, Tab,
// a click elsewhere). Return R_TRUE to accept it: a stored table then
// keeps the text in the cell, unless the callback set the cell itself
// (Rtable_set_cell, to keep "12.50" for "12.5"). A virtual table stores
// nothing: the application keeps the text in its data. Return R_FALSE to
// refuse it: after Enter or Tab the field stays open on the text, so the
// user can correct it; after a click elsewhere the edit is dropped.
// Without a callback, a stored table keeps every text.
RTableCellEditedCallback :: #type proc "c" (table: Rid, row: i32, column: i32, text: cstring, user_data: rawptr) -> i32
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
// Called when the user clicks a slice; `selected` tells whether it is now the selected one.
RChartSliceCallback :: #type proc "c" (chart: Rid, slice: i32, selected: i32, user_data: rawptr)
// Called when the user clicks a cell of a heatmap.
RChartCellCallback :: #type proc "c" (chart: Rid, row: i32, column: i32, value: f32, user_data: rawptr)
// Called when the current page changes as the user scrolls. This also fires the action of the widget.
RPdfPageChangedCallback :: #type proc "c" (view: Rid, page: i32, user_data: rawptr)
// Called when the user goes to another page (a swipe, the keys, a
// dot, an arrow) and when the autoplay does. This also fires the action
// of the widget.
RCarouselPageChangedCallback :: #type proc "c" (carousel: Rid, page: i32, user_data: rawptr)
// Asks the application for a tile (z, x, y), or tells it that a tile asked for is no longer wanted.
RMapTileCallback :: #type proc "c" (map_: Rid, z: i32, x: i32, y: i32, user_data: rawptr)
// Called when the user clicks a marker.
RMapMarkerCallback :: #type proc "c" (map_: Rid, marker: i32, user_data: rawptr)
// Called when the user moves or zooms the map (during a drag, at the
// end of an animated zoom), with its new center and zoom. This also fires
// the action of the widget.
RMapViewCallback :: #type proc "c" (map_: Rid, latitude: f64, longitude: f64, zoom: f64, user_data: rawptr)
// Receives the result of a script: `result` is its value written
// as JSON ("42", "\"text\"", "{\"a\":1}", "null"), or NULL when the
// script failed, `error` then telling why.
RSystemWebViewResultCallback :: #type proc "c" (view: Rid, result: cstring, error: cstring, user_data: rawptr)
// Receives what the page sends with `window.raven.postMessage(value)`.
RSystemWebViewMessageCallback :: #type proc "c" (view: Rid, message: cstring, user_data: rawptr)
RSystemWebViewEventCallback :: #type proc "c" (view: Rid, event: RSystemWebViewEvent, user_data: rawptr)
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
// Called when a panel leaves for a window of its own, once the
// window and its floating area are made, before the panel goes there and
// before anything is drawn.
// area          The area the panel belongs to (its home).
// panel         The panel that leaves.
// window        The new window.
// floating_area The dock area of the new window.
// Returns: The content of the window: a widget that holds floating_area
// (a VBox with a header bar above it, for a window made with
// R_WINDOW_CUSTOM_HEADER), or R_INVALID_ID for floating_area itself.
RDockFloatWindowCallback :: #type proc "c" (area: Rid, panel: Rid, window: Rid, floating_area: Rid, user_data: rawptr) -> Rid
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
// A line, or its number in the gutter, was clicked. `side` is the side
// of the line clicked (in unified mode: the left side for a removed line
// and for the left column of numbers, else the right side), `line` its
// number on that side, from 1.
RDiffViewLineCallback :: #type proc "c" (diff_view: Rid, side: RDiffSide, line: i32, in_gutter: i32, user_data: rawptr)
// Called when a dialog closes. `result` is the value given to Rdialog_close, 0 when dismissed.
RDialogClosedCallback :: #type proc "c" (content: Rid, result: i32, user_data: rawptr)
// Called when the user chooses: a double click or Enter on a file,
// the Open / Save button. `path` is the first chosen path; with several
// files, read them with Rfile_browser_get_selection. The action of the
// widget (OnActivated in XML) fires at the same time.
RFileBrowserCallback :: #type proc "c" (browser: Rid, path: cstring, user_data: rawptr)
RNotificationFallback :: #type proc "c" (title: cstring, body: cstring, urgency: RNotificationUrgency, user_data: rawptr)
RNotificationClickedCallback :: #type proc "c" (user_data: rawptr)
// Called on the UI thread when the user clicks a notification or
// one of its buttons, or dismisses it. `tag` names the notification,
// `action` is the id of the button for R_NOTIFICATION_ACTION_INVOKED, else NULL.
// NULL: none.
RNotificationCallback :: #type proc "c" (tag: cstring, event: RNotificationEvent, action: cstring, user_data: rawptr)
// The answer to Rpermission_request.
RPermissionCallback :: #type proc "c" (permission: RPermission, status: RPermissionStatus, user_data: rawptr)
// The end of a request of a picker, the camera, a save or a share
// sheet. `files` (count of them) are what it gave: none unless `result` is
// R_DEVICE_DONE. Called on the UI thread, once.
RDeviceCallback :: #type proc "c" (result: RDeviceResult, files: ^RPickedFile, count: i32, user_data: rawptr)
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
// Where app.log writes: the standard output when NULL.
RScriptLogCallback :: #type proc "c" (message: cstring, user_data: rawptr)
// Receives the diagnostics of the scripts instead of the error
// callback. `text` is the whole message, as the command line shows it;
// `code` is its code ("RS102"); `line` and `column` start at 1 (0 when
// the diagnostic has no place). NULL gives them back to the error callback.
RScriptDiagnosticCallback :: #type proc "c" (severity: RSeverity, code: cstring, file: cstring, line: i32, column: i32, text: cstring, user_data: rawptr)
RLocaleChangedCallback :: #type proc "c" (language: cstring, user_data: rawptr)
RAccessiblePressCallback :: #type proc "c" (widget: Rid, user_data: rawptr)

// Optional settings for Rinit_ex. A zero-initialized structure selects the
// default for every field, so new fields can be added without breaking code.
RInitDesc :: struct {
	// Rendering backend. R_BACKEND_DEFAULT (0) selects WebGL in browser builds, Metal on macOS and iOS when it is compiled, Vulkan otherwise.
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

// One format of something given to the other applications: put on
// the clipboard (Rclipboard_set_data) or dragged out of a window
// (Rdrag_begin_external). The program that receives it takes the format
// it prefers.
//
// `mime` names the format. Raven gives these their native names on each
// system (and converts them), so that every program understands them:
//
//   text/plain      UTF-8 text
//   text/html       HTML (UTF-8)
//   image/png       a PNG file (Rimage_encode_png); on Windows it is also
//                   given as a bitmap (CF_DIBV5), on macOS also as TIFF
//   text/uri-list   files, as file:// URLs, one per line: the file
//                   manager copies or moves them, the other programs
//                   open them
//   text/rtf, image/jpeg, image/tiff, image/gif, application/pdf,
//   application/json, text/csv, image/svg+xml
//
// Any other name is a format of the application ("application/x-myapp-node"):
// passed as it is, and read back by the programs that know it (other
// instances of the application).
RDataItem :: struct {
	// The format: a MIME type.
	mime: cstring,
	// The bytes (copied by the function they are given to). Text is not followed by a '\0'.
	data: rawptr,
	size: uint,
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
	// Smallest height (logical units) of what a finger presses: buttons,
	// fields, combo boxes, tabs and bars are at least this high, rows of
	// lists, trees, tables and menus too, and so are check boxes, switches
	// and sliders, whose box and handle grow to half of it. Whatever the
	// theme or a widget asks for, 0 leaves the sizes as they are.
	// Default 44 on iOS (Apple: 44 points), 48 on Android (Material: 48 dp),
	// 0 elsewhere.
	min_touch_size: f32,
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
	// Icon texture (R_INVALID_ID: none), the region of it and its tint (white by default; an SVG in currentColor is also tinted with the color of the name).
	icon: Rid,
	uv: [4]f32,
	tint: [4]f32,
	icon_radius: f32,
	// Color of the name; alpha 0 = the theme.
	text_color: [4]f32,
	// Layout="Rows": a third line under the detail, dimmer, or NULL.
	note: cstring,
	// Color of the detail line; alpha 0 = the theme (dimmer than the name).
	detail_color: [4]f32,
}

// A section of a tile view: its first tile, and what its header shows.
RTileSection :: struct {
	// Index of the first tile of the section (read when the sections are given).
	first: i32,
	// Title of the header (valid until the callback returns).
	text: cstring,
	// Dimmer text after the title (a count...), or NULL.
	detail: cstring,
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

// A button of a notification.
RNotificationAction :: struct {
	// Given back to the callback ("reply", "snooze"); not empty, and not "default".
	id: cstring,
	// Shown to the user ("Reply").
	label: cstring,
}

// A notification for Rnotification_show_desc. `size` must be
// sizeof(RNotificationDesc): the fields that later versions of Raven add
// come after the others, and a smaller size tells Raven they were not
// given. The other fields may be left to zero.
RNotificationDesc :: struct {
	// sizeof(RNotificationDesc).
	size: u32,
	// Names the notification: shown again with the same tag, it replaces the
	// one still shown instead of adding another; Rnotification_close takes it
	// away; the callback is given it. NULL or "": a tag of its own
	// ("raven.1", "raven.2"...).
	tag: cstring,
	title: cstring,
	body: cstring,
	// 0: R_NOTIFICATION_NORMAL.
	urgency: RNotificationUrgency,
	// An image file shown beside the text (PNG, JPEG, SVG...: what Raven reads), or NULL.
	icon: cstring,
	// Up to 3 buttons, or NULL.
	actions: ^RNotificationAction,
	action_count: i32,
	// The window brought back when the notification is clicked. R_INVALID_ID: the main window (the first one opened).
	window: Rid,
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

// A file given by a service of the device. Valid during the callback only: copy what you keep.
RPickedFile :: struct {
	// A path Raven and the C library can read (Rfile_read, textures, fonts,
	// fopen). On iOS and Android a copy in the cache of the application.
	// NULL for Rfile_save_async on iOS and Android: the file went where the
	// user chose, which the application cannot write to again.
	path: cstring,
	// The name the user saw ("IMG_0042.JPG"), to show.
	name: cstring,
	// The MIME type when the system tells ("image/jpeg"), or NULL.
	mime_type: cstring,
	// Bytes.
	size: u64,
	// Pixels of a photo (Rcamera_take_photo, Rphoto_pick) when known, else 0.
	width: i32,
	// Pixels of a photo (Rcamera_take_photo, Rphoto_pick) when known, else 0.
	height: i32,
}

// What a share sheet offers to the other applications (Rshare_show). Every field may be left to zero, but not all of them.
RShareDesc :: struct {
	// Text, or a link ("https://..."). May be NULL.
	text: cstring,
	// A file to send (a path Raven can read), or NULL.
	path: cstring,
	// Its MIME type ("image/png"); NULL: guessed from its extension.
	mime_type: cstring,
	// The title of the sheet, where the system shows one (Android's chooser). May be NULL.
	title: cstring,
	// The widget the sheet points at on a tablet (the button that shares). 0 or R_INVALID_ID: the middle of the window.
	anchor: Rid,
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
	// Vulkan: VkFormat of the color attachment; Metal: MTLPixelFormat.
	color_format: u32,
	// Metal: id<MTLRenderCommandEncoder> of the frame, inside its render pass (do not end it).
	metal_encoder: rawptr,
	// Metal: id<MTLCommandBuffer> the encoder belongs to.
	metal_command_buffer: rawptr,
	// Metal: id<MTLDevice> of the backend, to make pipeline states that match color_format.
	metal_device: rawptr,
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

// A request of a service of the device, valid during the call only. Fields that do not concern the service are zero.
RHostServiceRequest :: struct {
	// Given back to Rhost_service_complete.
	id: u32,
	service: RHostService,
	// The window it is for (R_INVALID_ID: none).
	window: Rid,
	// What it points at (a share sheet on a tablet), in pixels of the window; width 0: the middle of the window.
	anchor_x: f32,
	// What it points at (a share sheet on a tablet), in pixels of the window; width 0: the middle of the window.
	anchor_y: f32,
	// What it points at (a share sheet on a tablet), in pixels of the window; width 0: the middle of the window.
	anchor_width: f32,
	// What it points at (a share sheet on a tablet), in pixels of the window; width 0: the middle of the window.
	anchor_height: f32,
	title: cstring,
	filters: ^RFileFilter,
	filter_count: i32,
	// Bitmask of RFileDialogFlags.
	flags: u32,
	file_name: cstring,
	default_extension: cstring,
	data: rawptr,
	size: uint,
	path: cstring,
	max_count: i32,
	text: cstring,
	mime_type: cstring,
	share_title: cstring,
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
	// An RPermissionStatus, without asking. NULL: as on desktops (notifications granted, the rest unsupported).
	permission_status: rawptr,
	// Ask the user (the status is not asked or denied); answer with Rhost_permission_answer.
	permission_request: rawptr,
	// A picker, the camera, a save or a share sheet; answer with Rhost_service_complete. NULL: the file dialogs of Raven, nothing else.
	service_request: rawptr,
	// R_TRUE: there is a camera (Rcamera_is_available) for R_HOST_SERVICE_TAKE_PHOTO.
	has_camera: i32,
	// Ropen_url. Returns R_FALSE when it could not. NULL: as on desktops (`open`, `xdg-open`...).
	open_url: rawptr,
	// Rhaptic_play: an RHaptic. Returns R_TRUE when played.
	haptic: rawptr,
	// Downloads an http(s) URL (images from a URL): returns its content in a
	// buffer from Rmemory_alloc and its size, or NULL. Called on worker
	// threads, several at a time, and may block. NULL: as on desktops.
	download: rawptr,
}

// How Rtest_expect_golden compares. NULL options: a tolerance of 2 per channel and no different pixel.
RGoldenOptions :: struct {
	// Largest difference on one channel (0..255) for which two pixels are still the same.
	channel_tolerance: i32,
	// Share of the pixels (0..1) that may differ while the images still match.
	max_diff_ratio: f32,
	// R_TRUE: the golden of this OS and backend, "button.png" read as "button.macos-metal.png" (Rtest_get_platform_tag).
	per_platform: i32,
	// Folder of the .actual.png and .diff.png written on a mismatch. NULL: beside the golden.
	output_directory: cstring,
}

// What Rtest_expect_golden found.
RGoldenResult :: struct {
	status: RGoldenStatus,
	// Size of the capture, in pixels.
	width: i32,
	// Size of the capture, in pixels.
	height: i32,
	// Pixels farther from the golden than the tolerance.
	differing_pixels: i32,
	// Largest difference found on one channel (0..255).
	max_channel_difference: i32,
	// The golden that was read or written (with the platform tag). Valid until the next call.
	golden_path: cstring,
	// One line that says what happened, for the log of the test. Valid until the next call.
	message: cstring,
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
	Rset_app_event_callback :: proc(callback: RAppEventCallback, user_data: rawptr) ---
	// Returns: R_TRUE between R_APP_DID_PAUSE and R_APP_WILL_RESUME: the application is in the background.
	Ris_app_paused :: proc() -> i32 ---
	// Handles an RAppEvent as if the system had sent it: for tests,
	// and for a host application (raven_host.h) that knows the lifecycle.
	// UI thread.
	Rinject_app_event :: proc(event: RAppEvent) ---
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
	Raction_bind_with_parameters :: proc(name: cstring, signature: cstring, callback: RActionParametersCallback, user_data: rawptr) -> i32 ---
	// Returns: The widget whose event fired the action, R_INVALID_ID when a script calls it.
	Raction_get_sender :: proc(context_: ^RActionContext) -> Rid ---
	Raction_get_name :: proc(context_: ^RActionContext) -> cstring ---
	// Returns: The number of arguments given: the parameters of the signature, fewer when an event gives fewer... never more.
	Raction_get_argument_count :: proc(context_: ^RActionContext) -> i32 ---
	// The arguments, by position from 0. A wrong index or type reports an error and gives 0, "" or R_FALSE.
	Raction_get_number :: proc(context_: ^RActionContext, index: i32) -> f64 ---
	// Returns: Valid until the function returns.
	Raction_get_string :: proc(context_: ^RActionContext, index: i32) -> cstring ---
	Raction_get_bool :: proc(context_: ^RActionContext, index: i32) -> i32 ---
	// The value returned to the script; of the type of the signature (what is set last counts).
	Raction_return_number :: proc(context_: ^RActionContext, value: f64) ---
	Raction_return_string :: proc(context_: ^RActionContext, value: cstring) ---
	Raction_return_bool :: proc(context_: ^RActionContext, value: i32) ---
	// The function failed: the script that called it stops with error
	// RS205 and this message (the application goes on; so does the next event).
	Raction_fail :: proc(context_: ^RActionContext, message: cstring) ---
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
	// application. Returns at once. On iOS and Android the system opens it
	// (a web page, a tel:, a mailto:; not a file: share it with Rshare_show),
	// and Raven must be initialized and this be called on the UI thread.
	// Returns: R_FALSE when the OS could not be asked (nothing tells whether
	// a program then really opened it).
	Ropen_url :: proc(url: cstring) -> i32 ---
	Rclipboard_set_text :: proc(text: cstring) ---
	// Copies one content in two formats: `html` for the programs that
	// paste formatting (word processors, chat, mail) and `text` for the
	// others (editors, terminals). Each program takes the format it prefers.
	// Where the window system has no clipboard formats, or `html` is NULL,
	// it is Rclipboard_set_text(text).
	Rclipboard_set_html :: proc(text: cstring, html: cstring) ---
	// Returns: The clipboard text, or "" when empty. Valid until the next call.
	// On iOS a read that does not follow a paste of the user (the Paste of a
	// menu, a button, Command-V) shows the system's question "Allow Paste?",
	// and on Android 12+ the system says that the application pasted: read it
	// when the user pastes, not to look at it. Android lets only the
	// application in front read it.
	Rclipboard_get_text :: proc() -> cstring ---
	// Returns: R_TRUE when the clipboard holds text, without reading it (no
	// question of the system on iOS): to enable a Paste button.
	Rclipboard_has_text :: proc() -> i32 ---
	// Replaces what the clipboard holds with one content in several
	// formats (RDataItem): each program pastes the one it prefers.
	// Returns: R_TRUE when every format was put on the clipboard; R_FALSE when
	// some could not be (a window system that only takes text: X11 and
	// Wayland for now, where the text/plain item, if any, still is).
	Rclipboard_set_data :: proc(items: ^RDataItem, count: i32) -> i32 ---
	// Returns: R_TRUE when the clipboard holds this format (Rclipboard_set_data), or one Raven converts to it.
	Rclipboard_has_data :: proc(mime: cstring) -> i32 ---
	// What the clipboard holds in one format, converted from the
	// native one when needed (a bitmap given as image/png...).
	// Returns: The bytes, followed by a '\0' that `size` does not count, to
	// release with Rmemory_free; NULL when the clipboard does not hold it.
	Rclipboard_get_data :: proc(mime: cstring, size: ^uint) -> cstring ---
	// Copies a picture: RGBA8 pixels, straight alpha, rows `stride`
	// bytes apart (0: width * 4). It goes on the clipboard as image/png
	// (and as the bitmaps of the system), so that image editors, documents
	// and mail paste it.
	// Returns: R_FALSE when it could not be put there.
	Rclipboard_set_image :: proc(rgba: [^]u8, width: i32, height: i32, stride: i32) -> i32 ---
	// Returns: R_TRUE when the clipboard holds a picture Rclipboard_get_image can read.
	Rclipboard_has_image :: proc() -> i32 ---
	// The picture on the clipboard (copied from any program: PNG,
	// TIFF, a bitmap...), as tightly packed RGBA8 pixels, straight alpha.
	// Returns: The pixels, to release with Rpixels_free; NULL when there is
	// none (or no decoder: RAVEN_WITH_STB_IMAGE off).
	Rclipboard_get_image :: proc(width: ^i32, height: ^i32) -> [^]u8 ---
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
	// none (desktop). The content is laid out above it already, and the
	// field that has the focus is scrolled into view above it, by the
	// ScrollBoxes that hold it, when the keyboard comes or changes height,
	// when the window turns, and when another field takes the focus.
	Rwindow_get_keyboard_area :: proc(window: Rid, insets: ^RInsets) ---
	// Called when the on-screen keyboard appears, changes height or goes away (mobile). NULL removes it.
	Rwindow_set_keyboard_callback :: proc(window: Rid, callback: RKeyboardCallback, user_data: rawptr) ---
	// No widget of the window has the keyboard focus any more: on mobile, the on-screen keyboard goes away.
	Rwindow_clear_focus :: proc(window: Rid) ---
	Rwindow_set_back_callback :: proc(window: Rid, callback: RBackCallback, user_data: rawptr) ---
	// R_TRUE while the application has somewhere to go back to (a page
	// under the one shown). Android 13 and later ask for Back only while the
	// window wants it (predictive back): at the root page the system handles
	// Back itself and animates back to the home screen. Default R_FALSE.
	Rwindow_set_back_enabled :: proc(window: Rid, enabled: i32) ---
	// Returns: R_TRUE while Back would be taken: a popup or dialog is open, or Rwindow_set_back_enabled said so.
	Rwindow_wants_back :: proc(window: Rid) -> i32 ---
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
	// The text an input method is composing (preedit: Chinese,
	// Japanese, Korean, dead keys), as the platforms send it. The focused
	// TextInput, SpinBox or TextEditor shows it at its caret, underlined, in
	// place of its selection, but it is not in its text and no changed
	// callback runs. [selection_start, selection_end) are bytes of `text`:
	// the part the input method works on (underlined thicker), or the caret
	// when they are equal; -1 is the end of `text`. NULL or "" ends the
	// composition without typing anything (cancel). The committed text comes
	// through Rwindow_inject_text, which ends the composition too, and so
	// does a change of focus. Ignored when the focus takes no text.
	Rwindow_inject_composition :: proc(window: Rid, text: cstring, selection_start: i32, selection_end: i32) ---
	// A drag from another application moves over the window
	// (`inside` R_TRUE) or leaves it.
	// Returns: R_TRUE when a drop at this position would be accepted.
	Rwindow_inject_drag :: proc(window: Rid, x: f32, y: f32, inside: i32) -> i32 ---
	// Drops files and / or text at a position. Returns R_TRUE when a target took them.
	Rwindow_inject_drop :: proc(window: Rid, x: f32, y: f32, paths: [^]cstring, path_count: i32, text: cstring) -> i32 ---
	// The end of the drag out of the window that the window system
	// holds (Rdrag_begin_external), as it reports it: `operation` is what the
	// target did (an RDragOperation). Returns R_FALSE when no such drag is in
	// progress.
	Rwindow_inject_drag_end :: proc(window: Rid, operation: RDragOperation) -> i32 ---
	// Back, as the system sends it (Rwindow_set_back_callback). Returns R_TRUE when something took it.
	Rwindow_inject_back :: proc(window: Rid) -> i32 ---
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
	// The composition of the input method in progress (UTF-8, valid
	// until it changes) and its selection in bytes; NULL when there is none.
	// Any output pointer may be NULL.
	Rwindow_get_text_composition :: proc(window: Rid, selection_start: ^i32, selection_end: ^i32) -> cstring ---
	// R_TRUE while a menu, list or dialog is open over the window.
	Rwindow_has_popup :: proc(window: Rid) -> i32 ---
	// Last painted caret in window pixels (x, y, width, height), including scrolling and scale.
	// Call after Rdraw_frame. Independent of the blink phase. Returns R_FALSE and zeros the
	// outputs when there is no visible editable caret, the window is inactive, or focus changed
	// since painting. Any output pointer may be NULL. While an input method
	// composes, it is the caret inside the composition: where its candidate
	// window goes.
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
	// Lets the user drag data out of the window, to another
	// application: a picture into a document, files into the file manager,
	// text into an editor. `items` is one content in several formats
	// (RDataItem, copied): the target takes the one it prefers.
	// `operations` is a bitmask of RDragOperation the target may do (0:
	// R_DRAG_OPERATION_COPY).
	//
	// Call it as Rdrag_begin, while the mouse button is held (from a mouse
	// move once the pointer has moved a few units). The window system then
	// takes the drag at the next move of the pointer: it shows the picture
	// that follows the pointer (Rdrag_set_image) and the cursor that tells
	// what a drop there does; the button released, `callback` (may be NULL)
	// is told what the target did.
	//
	// Called while a drag inside the application (Rdrag_begin) is in progress
	// in the window of `source`, it gives that drag the data for outside:
	// the drag stays inside the window, to its drop targets, and becomes a
	// drag of the window system only when the pointer leaves the window.
	// Brought back over a window of the application, it still comes to the
	// drop targets with the type and data given to Rdrag_begin.
	//
	// Implemented on macOS and Windows; elsewhere it returns R_FALSE
	// (Rdrag_external_supported).
	// Returns: R_FALSE when the widget is not in a window or the window
	// system cannot drag out of its windows.
	Rdrag_begin_external :: proc(source: Rid, items: ^RDataItem, count: i32, operations: u32, callback: RDragFinishedCallback, user_data: rawptr) -> i32 ---
	// The picture that follows the pointer during the drag given by
	// Rdrag_begin_external (RGBA8 pixels, straight alpha, tightly packed,
	// copied), held by the pointer at its center. Call it right after
	// Rdrag_begin_external. Without it the system chooses: the picture
	// dragged, the icons of the files, or a generic one.
	Rdrag_set_image :: proc(rgba: [^]u8, width: i32, height: i32) ---
	// R_TRUE when the window system lets Raven drag data out of its windows (Rdrag_begin_external).
	Rdrag_external_supported :: proc() -> i32 ---
	// R_TRUE from Rdrag_begin_external until its callback was called.
	Rdrag_external_is_active :: proc() -> i32 ---
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
	// Changes a size of the theme in use. It lasts until the look or
	// the theme is chosen again (Rlook_set, Rtheme_use), but not through a
	// switch between light and dark made by Rtheme_follow_system.
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
	//
	// A switch is Rlook_set, except that the sizes given with
	// Rtheme_set_metric stay. Colors and styles set with Rtheme_set_... are
	// those of one mode and are lost; to keep colors, sizes and styles of
	// the application through the switch, put them in a theme that names no
	// look (Rtheme_load_from_memory, then Rtheme_use): it is laid over the
	// new look.
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
	// BMP, TGA, GIF, PSD, SVG). Decoding happens on the calling thread. An
	// animated GIF or APNG keeps its frames: an Image showing it plays them.
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
	//
	// currentColor: an SVG painted only in currentColor (fill="currentColor",
	// stroke="currentColor") is drawn in white, keeping its shape and its
	// opacity, and the widgets that show it (Image, the icons of menu items,
	// tabs, tree items, dock panels, tiles, nodes) tint it with the color of
	// their text at draw time: it follows the theme, hover, pressed and
	// disabled without being drawn again. Rdraw_image draws it in its tint
	// color. In an SVG that also uses other colors, currentColor is black.
	// See Rtexture_uses_current_color.
	Rtexture_create_from_svg :: proc(svg: cstring, svg_size: uint, scale: f32) -> Rid ---
	// R_TRUE when the texture is an SVG painted only in currentColor:
	// white where it is painted, tinted with the color of the text by the
	// widgets that show it (see Rtexture_create_from_svg).
	Rtexture_uses_current_color :: proc(texture: Rid) -> i32 ---
	// Pixels of the texture per logical unit: the scale of an SVG, 1 otherwise.
	Rtexture_get_density :: proc(texture: Rid) -> f32 ---
	Rtexture_create_from_file :: proc(path: cstring) -> Rid ---
	// Reads and decodes the file on a worker thread.
	Rtexture_create_from_file_async :: proc(path: cstring) -> Rid ---
	// Downloads an http:// or https:// image and decodes it on a worker
	// thread, as an Image whose Source is a URL does. The handle is usable at
	// once: widgets showing it draw nothing until it is ready (Rtexture_is_ready;
	// a texture that could not be downloaded is ready with a size of 0, and the
	// failure is reported as a warning). Desktops download with WinHTTP or
	// `curl`; iOS and Android through their platform. Android needs the INTERNET
	// permission in the manifest; iOS refuses http:// unless Info.plist allows
	// it (App Transport Security). Up to 32 MiB.
	Rtexture_create_from_url_async :: proc(url: cstring) -> Rid ---
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
	// Reads what an image file in memory holds without keeping its
	// pixels: its size, its frames (1 for a still image, more for an animated
	// GIF or APNG) and how many times it plays (0: forever). Thread-safe.
	// Each pointer may be NULL.
	// Returns: R_FALSE when the data is not an image Raven can read.
	Rimage_decode_info :: proc(data: rawptr, data_size: uint, width: ^i32, height: ^i32, frame_count: ^i32, loop_count: ^i32) -> i32 ---
	// Decodes one frame of an animated GIF or APNG into tightly packed
	// RGBA8 pixels, composed as it is shown (the frames before it drawn and
	// disposed of). Frame 0 of a still image is the image. Thread-safe.
	// delay Receives the seconds the frame is shown (0 for a still image); may be NULL.
	// Returns: The pixels, to release with Rpixels_free; NULL when the frame does not exist.
	Rimage_decode_frame :: proc(data: rawptr, data_size: uint, frame: i32, width: ^i32, height: ^i32, delay: ^f64) -> [^]u8 ---
	// Releases pixels returned by Raven (Rimage_decode, Rwindow_capture). Thread-safe.
	Rpixels_free :: proc(pixels: [^]u8) ---
	// Encodes tightly packed RGBA8 pixels (what Rwindow_capture
	// returns) as a PNG file in memory. Thread-safe.
	// size Receives the number of bytes.
	// Returns: The bytes of the file, to release with Rpixels_free; NULL when
	// there are no pixels.
	Rimage_encode_png :: proc(rgba: [^]u8, width: i32, height: i32, size: ^uint) -> [^]u8 ---
	// Writes tightly packed RGBA8 pixels to a PNG file: a screenshot
	// of Rwindow_capture, a picture made by the application. Thread-safe.
	// Returns: R_FALSE when the file cannot be written.
	Rimage_save_png :: proc(path: cstring, rgba: [^]u8, width: i32, height: i32) -> i32 ---
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
	// An icon shipped with Raven, by its name: "search", "settings",
	// "close"... (Ricon_get_name lists them; "raven:search" works too, as in
	// XML: Source="raven:search"). The icons are the ones of Lucide (ISC
	// License), 24 x 24 logical units, drawn with lines in currentColor: the
	// widgets that show them tint them with the color of their text, so they
	// follow the theme and the states (Rtexture_create_from_svg). Drawn at
	// the density they are shown with, sharp on every screen.
	// Returns: A texture owned by Raven, made on first use and then shared
	// (do not destroy it; if it was, the next call makes it again);
	// R_INVALID_ID, with an error, when there is no icon of that name.
	Ricon_get :: proc(name: cstring) -> Rid ---
	// Returns: The number of icons shipped with Raven.
	Ricon_get_count :: proc() -> i32 ---
	// Returns: The name of an icon, from 0 to Ricon_get_count() - 1, in alphabetical order; NULL out of range.
	Ricon_get_name :: proc(index: i32) -> cstring ---
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
	//         R_INVALID_ID when the child replaced in a single-child container
	//         had a destroy callback that destroyed the container or `child`;
	//         `child`, when it is still there, is then not added.
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
	// Text shown when the cursor rests on the widget. NULL removes it. A label, which otherwise
	//  lets the mouse through, takes the hover while it has a tooltip (events it does not use still bubble).
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
	// Gives the keyboard focus to the widget, and scrolls it into view (Rwidget_scroll_into_view).
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
	// Returns: The metric the widget uses: its own, else the one of a style,
	// else the one of its theme; grown for larger text
	// (Raccessibility_set_text_scale) and for a finger (RTouchSettings::min_touch_size).
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
	// Scrolls every ScrollBox above the widget so that the widget is
	// visible. The library does it by itself for the focused widget when the
	// focus comes from Tab or Rwidget_focus, and for a text field above the
	// on-screen keyboard (Rwindow_get_keyboard_area).
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
	// Icon before the title (documents). R_INVALID_ID removes it; tint: a mask in the text color (always, for an SVG in currentColor). XML: Slot.Icon.
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
	// Color the image is multiplied by (white by default). An SVG in
	// currentColor (Ricon_get, Rtexture_create_from_svg) is drawn in the color of
	// the text around it until a tint is given; then in the tint. XML: Tint.
	Rimage_set_tint_color :: proc(image: Rid, r: f32, g: f32, b: f32, a: f32) ---
	// R_TRUE: the image is tinted with the text color around it (white icons that follow the theme). XML: TintText.
	Rimage_set_tint_text :: proc(image: Rid, enabled: i32) ---
	Rimage_set_stretch :: proc(image: Rid, stretch: RStretch) ---
	// Part of the texture to show, in UV coordinates. Default: 0, 0, 1, 1.
	Rimage_set_region :: proc(image: Rid, min_u: f32, min_v: f32, max_u: f32, max_v: f32) ---
	Rimage_set_corner_radius :: proc(image: Rid, radius: f32) ---
	// R_TRUE when the texture of the image is an animated GIF or APNG (two frames or more).
	Rimage_is_animated :: proc(image: Rid) -> i32 ---
	// Returns: The frames of its animation; 1 for a still image, 0 without a texture.
	Rimage_get_frame_count :: proc(image: Rid) -> i32 ---
	// Returns: The seconds one play of its animation lasts (0 for a still image).
	Rimage_get_duration :: proc(image: Rid) -> f64 ---
	// Returns: The seconds a frame of its animation is shown (0 for a frame that does not exist).
	Rimage_get_frame_delay :: proc(image: Rid, frame: i32) -> f64 ---
	// Plays the animation from the frame it is on (from the first
	// again when it had finished). An Image plays as soon as it is given an
	// animated texture, unless XML says Animate="false".
	Rimage_play :: proc(image: Rid) ---
	// Stops the animation on the frame it is on.
	Rimage_pause :: proc(image: Rid) ---
	// Stops the animation and shows its first frame.
	Rimage_stop :: proc(image: Rid) ---
	Rimage_is_playing :: proc(image: Rid) -> i32 ---
	// Shows a frame (from 0), playing on from it if it plays.
	Rimage_set_frame :: proc(image: Rid, frame: i32) ---
	Rimage_get_frame :: proc(image: Rid) -> i32 ---
	// How many times the animation plays before it stops on its last
	// frame: 0 forever, -1 what the file says (the default). XML: Loop
	// (a number; also Forever, Once).
	Rimage_set_loop_count :: proc(image: Rid, count: i32) ---
	// Returns: The plays of the animation (0: forever), the file's unless it was set.
	Rimage_get_loop_count :: proc(image: Rid) -> i32 ---
	Rimage_set_finished_callback :: proc(image: Rid, callback: RImageFinishedCallback, user_data: rawptr) ---
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
	// Underlines the misspelled words with a wavy line (R_COLOR_SPELLING);
	// a right click on one offers corrections (see SPELL CHECKING). Off by
	// default. A password is never checked.
	Rtext_input_set_spell_check :: proc(text_input: Rid, enabled: i32) ---
	Rtext_input_get_spell_check :: proc(text_input: Rid) -> i32 ---
	// Language of the spell checking ("en_US", "fr"...). NULL: the default (Rspell_set_language).
	Rtext_input_set_spell_language :: proc(text_input: Rid, language: cstring) ---
	// R_TRUE when the system has a spell checker that Raven can use.
	Rspell_is_supported :: proc() -> i32 ---
	// The languages the checker has dictionaries for.
	Rspell_get_language_count :: proc() -> i32 ---
	Rspell_get_language_at :: proc(index: i32) -> cstring ---
	// The language used when a widget or a call gives none. NULL
	// follows the locale again.
	// Returns: R_FALSE when the checker has no dictionary for it (the default
	// of the system is then used).
	Rspell_set_language :: proc(language: cstring) -> i32 ---
	// The code of the language used by default, as the checker names it; "" when there is no checker.
	Rspell_get_language :: proc() -> cstring ---
	// Checks a text. `language` NULL: the default.
	// Returns: The number of misspelled words, read with Rspell_get_misspelling.
	Rspell_check :: proc(text: cstring, language: cstring) -> i32 ---
	// A misspelled word found by the last Rspell_check: bytes [start, end) of its text.
	Rspell_get_misspelling :: proc(index: i32, start: ^i32, end: ^i32) -> i32 ---
	// Returns: The number of corrections for a word (best first), read with Rspell_get_suggestion.
	Rspell_suggest :: proc(word: cstring, language: cstring) -> i32 ---
	// A correction found by the last Rspell_suggest; valid until the next call.
	Rspell_get_suggestion :: proc(index: i32) -> cstring ---
	// Adds a word to the dictionary of the user, which the system keeps (with Hunspell alone: for the session).
	Rspell_learn :: proc(word: cstring, language: cstring) ---
	// The word is no longer a misspelling, in every language, until the application quits.
	Rspell_ignore :: proc(word: cstring) ---
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
	// R_TRUE: the combo box is a text field with its list: the user
	// may type a text that is not one of the items (Editable="true"). The text
	// is Rcombo_box_get_text; choosing an item puts its text there. A typed
	// text equal to an item selects it, any other selects nothing (-1).
	Rcombo_box_set_editable :: proc(combo_box: Rid, editable: i32) ---
	Rcombo_box_is_editable :: proc(combo_box: Rid) -> i32 ---
	// R_TRUE: typing in the combo box filters its list (Searchable="true"):
	// the items that contain what was typed, without regard to case or
	// accents ("zoe" finds "Zoë", "sao" finds "São Paulo"), or a row saying that nothing
	// matches. Up and Down move in the list, Enter chooses, Escape closes it.
	// A searchable combo box that is not editable comes back to the item
	// chosen when nothing is.
	Rcombo_box_set_searchable :: proc(combo_box: Rid, searchable: i32) ---
	Rcombo_box_is_searchable :: proc(combo_box: Rid) -> i32 ---
	// The text shown: what was typed in an editable combo box, else the text of the selected item ("" for none).
	Rcombo_box_get_text :: proc(combo_box: Rid) -> cstring ---
	// Editable combo box: sets the text of the field (the item equal to it is selected); otherwise selects the item with this text.
	Rcombo_box_set_text :: proc(combo_box: Rid, text: cstring) ---
	Rcombo_box_set_text_changed_callback :: proc(combo_box: Rid, callback: RComboBoxTextCallback, user_data: rawptr) ---
	// Opens (R_TRUE) or closes the list.
	Rcombo_box_set_open :: proc(combo_box: Rid, open: i32) ---
	Rcombo_box_is_open :: proc(combo_box: Rid) -> i32 ---
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
	// An SVG in currentColor (Ricon_get) takes the color of the text. XML: Icon on an Item.
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
	// A noninteractive caption row, skipped by keyboard navigation.
	Rmenu_add_header :: proc(menu: Rid, text: cstring) -> Rid ---
	// A vertical divider between titles of an in-window MenuBar. Ignored by native macOS bars.
	Rmenu_bar_add_divider :: proc(menu_bar: Rid) -> Rid ---
	// Title color in an in-window bar; alpha 0 restores the theme. Ignored by native macOS bars.
	Rmenu_set_title_color :: proc(menu: Rid, r: f32, g: f32, b: f32, a: f32) ---
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
	//                An SVG in currentColor (Ricon_get) always is. XML: Icon.
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
	// Layout="Rows": width of the box the icon is drawn in (its height is the
	// icon size), for icons that are not square; 0 = square (default). XML: IconWidth.
	Rtile_view_set_icon_width :: proc(tile_view: Rid, width: f32) ---
	// `count` sections, described by `callback`, all unfolded. Rtile_view_set_items removes them: give them after the tiles.
	Rtile_view_set_sections :: proc(tile_view: Rid, count: i32, callback: RTileSectionCallback, user_data: rawptr) ---
	Rtile_view_get_section_count :: proc(tile_view: Rid) -> i32 ---
	// The section of a tile, or -1 (before the first section).
	Rtile_view_get_section_of :: proc(tile_view: Rid, index: i32) -> i32 ---
	// Folds or unfolds a section (the toggled callback is not called).
	Rtile_view_set_section_folded :: proc(tile_view: Rid, section: i32, folded: i32) ---
	Rtile_view_is_section_folded :: proc(tile_view: Rid, section: i32) -> i32 ---
	// The section whose header is at a point relative to the widget, or -1.
	Rtile_view_get_section_at :: proc(tile_view: Rid, local_x: f32, local_y: f32) -> i32 ---
	// Called when the user folds or unfolds a section (`index`: the section).
	Rtile_view_set_section_toggled_callback :: proc(tile_view: Rid, callback: RTileEventCallback, user_data: rawptr) ---
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
	// Tiles can be dragged out of the window, to other applications:
	// when the user starts dragging the selected tiles (`index` is the one
	// under the pointer), the callback gives their data by calling
	// Rdrag_begin_external (the pictures, the files they stand for...), and
	// Rdrag_set_image. With a drag type too (Rtile_view_set_drag_type), the
	// drag first goes to the drop targets of the window and only leaves it
	// with the pointer. NULL: the tiles stay in the application (default).
	Rtile_view_set_drag_out_callback :: proc(tile_view: Rid, callback: RTileEventCallback, user_data: rawptr) ---
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
	// Stored rows: a fixed text color; column -1 colors the row; cells override it.
	// Alpha 0 restores the inherited/theme color. A color from the cell color callback wins over it.
	Rtable_set_cell_text_color :: proc(table: Rid, row: i32, column: i32, r: f32, g: f32, b: f32, a: f32) ---
	// Opt into Ctrl/Cmd toggles and Shift ranges; single selection is the default. Same as Rtable_set_selection_mode(MULTIPLE or SINGLE).
	Rtable_set_multi_select :: proc(table: Rid, enabled: i32) ---
	// Single (default), multiple or no selection (SelectionMode="Multiple" in XML).
	// Going to single keeps the selected row, going to none clears the selection.
	Rtable_set_selection_mode :: proc(table: Rid, mode: RTableSelectionMode) ---
	Rtable_get_selection_mode :: proc(table: Rid) -> RTableSelectionMode ---
	Rtable_set_row_selected :: proc(table: Rid, row: i32, selected: i32) ---
	Rtable_is_row_selected :: proc(table: Rid, row: i32) -> i32 ---
	// Selects (`selected` R_TRUE) or deselects the rows first..last, both
	// included, keeping the others: multiple selection only (in single
	// selection, selecting a range selects `last`).
	Rtable_select_range :: proc(table: Rid, first: i32, last: i32, selected: i32) ---
	// Multiple selection: selects every row (Ctrl/Cmd+A). Nothing in the other modes.
	Rtable_select_all :: proc(table: Rid) ---
	Rtable_clear_selection :: proc(table: Rid) ---
	Rtable_get_selection_count :: proc(table: Rid) -> i32 ---
	// Selected row at index, top to bottom; -1 outside the selection.
	Rtable_get_selection :: proc(table: Rid, index: i32) -> i32 ---
	// The selection as runs of consecutive rows, top to bottom: the
	// fast way to go through a large selection (select all of a million rows
	// is one range).
	Rtable_get_selection_range_count :: proc(table: Rid) -> i32 ---
	// The rows first..last (both included) of a range of the selection. Returns: R_FALSE outside the ranges.
	Rtable_get_selection_range :: proc(table: Rid, index: i32, first: ^i32, last: ^i32) -> i32 ---
	// Replaces the selection with one row. -1 selects nothing, including in multi-select mode.
	Rtable_set_selected :: proc(table: Rid, row: i32) ---
	// The selected row (-1: none). In multiple selection, the one
	// selected last, or else the first one; the selection changed callback
	// receives it.
	Rtable_get_selected :: proc(table: Rid) -> i32 ---
	// The row the keyboard is on: the selected row in single
	// selection; in multiple selection the row last clicked or reached with
	// the arrows, which Space toggles (it may be unselected). -1: none.
	Rtable_get_current_row :: proc(table: Rid) -> i32 ---
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
	// R_TRUE: the cells of the column are edited in place by the user (default R_FALSE).
	Rtable_set_column_editable :: proc(table: Rid, column: i32, editable: i32) ---
	Rtable_is_column_editable :: proc(table: Rid, column: i32) -> i32 ---
	Rtable_set_cell_edited_callback :: proc(table: Rid, callback: RTableCellEditedCallback, user_data: rawptr) ---
	// Opens the editor on a cell, its text selected, as F2 does (the
	// row is selected and scrolled into view). Any column can be edited this
	// way, editable or not. Returns: R_TRUE when the editor is open.
	Rtable_edit_cell :: proc(table: Rid, row: i32, column: i32) -> i32 ---
	Rtable_is_editing :: proc(table: Rid) -> i32 ---
	// Closes the editor: `commit` R_TRUE keeps the text (through the
	// cell edited callback), R_FALSE forgets it. Returns: R_FALSE when the text
	// was refused (the editor then stays open), R_TRUE otherwise.
	Rtable_end_edit :: proc(table: Rid, commit: i32) -> i32 ---
	// The first `count` columns stay in place when the table scrolls
	// sideways; a thin shadow marks where the others pass under them. 0 (the
	// default): every column scrolls.
	Rtable_set_frozen_columns :: proc(table: Rid, count: i32) ---
	Rtable_get_frozen_columns :: proc(table: Rid) -> i32 ---
	// Text (default), check box or progress bar: how the cells of the
	// column are drawn from their texts. A check column is made editable (the
	// user toggles it; Rtable_set_column_editable(R_FALSE) after this shows it
	// only) and centered. For anything else, a cell color callback and the
	// texts are what the table offers: it draws no other widget in its cells.
	Rtable_set_column_kind :: proc(table: Rid, column: i32, kind: RTableColumnKind) ---
	Rtable_get_column_kind :: proc(table: Rid, column: i32) -> RTableColumnKind ---
	// A cell of a check column: R_TRUE when its text is "1", "true" or "yes". Any cell can be asked.
	Rtable_is_cell_checked :: proc(table: Rid, row: i32, column: i32) -> i32 ---
	// Stored rows: writes "1" or "0" in the cell (no callback is called).
	Rtable_set_cell_checked :: proc(table: Rid, row: i32, column: i32, checked: i32) ---
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
	// The time zone of the date, for the instants below; NULL or "" (default): the zone of the system. TimeZone="..." in XML.
	Rdate_picker_set_time_zone :: proc(date_picker: Rid, zone: cstring) ---
	Rdate_picker_get_time_zone :: proc(date_picker: Rid) -> cstring ---
	// Shows the date an instant (seconds since 1970, UTC) has in the zone of the picker.
	Rdate_picker_set_instant :: proc(date_picker: Rid, utc: f64) ---
	// The first instant of the date in the zone: its midnight (or, where the clocks skip midnight, the time they go to).
	Rdate_picker_get_instant :: proc(date_picker: Rid) -> f64 ---
	Rtime_picker_set_time :: proc(time_picker: Rid, hour: i32, minute: i32, second: i32) ---
	Rtime_picker_get_time :: proc(time_picker: Rid, hour: ^i32, minute: ^i32, second: ^i32) ---
	Rtime_picker_set_seconds_visible :: proc(time_picker: Rid, visible: i32) ---
	// Step of the minutes for the arrows and the wheel (default 1).
	Rtime_picker_set_minute_step :: proc(time_picker: Rid, step: i32) ---
	Rtime_picker_set_changed_callback :: proc(time_picker: Rid, callback: RTimeChangedCallback, user_data: rawptr) ---
	// 12 or 24 hours for this picker; R_CLOCK_AUTO (default): the clock of the locale. Clock="Auto|12|24" in XML.
	Rtime_picker_set_clock :: proc(time_picker: Rid, clock: RClock) ---
	Rtime_picker_get_clock :: proc(time_picker: Rid) -> RClock ---
	// The time as the field shows it: "3:05 PM", "15:05", with the zone when it is shown (valid until the next call).
	Rtime_picker_get_text :: proc(time_picker: Rid) -> cstring ---
	// The time zone of the time ("America/New_York", see
	// Rtimezone_is_valid); NULL or "" (default): the zone of the system. An
	// unknown zone is refused. TimeZone="..." in XML.
	Rtime_picker_set_time_zone :: proc(time_picker: Rid, zone: cstring) ---
	// The zone set, "" for none.
	Rtime_picker_get_time_zone :: proc(time_picker: Rid) -> cstring ---
	// Writes the zone after the time ("EST", "CET", "UTC+3"). ShowTimeZone="true" in XML.
	Rtime_picker_set_time_zone_visible :: proc(time_picker: Rid, visible: i32) ---
	// The day of the time, for its zone (month 1 to 12); year 0: today (default). Date="YYYY-MM-DD" in XML.
	Rtime_picker_set_date :: proc(time_picker: Rid, year: i32, month: i32, day: i32) ---
	Rtime_picker_get_date :: proc(time_picker: Rid, year: ^i32, month: ^i32, day: ^i32) ---
	// Shows an instant (seconds since 1970, UTC) in the zone of the picker: its day and its time.
	Rtime_picker_set_instant :: proc(time_picker: Rid, utc: f64) ---
	// The instant the day and the time are in the zone. A time the
	// clocks skipped is moved forward, a time they repeated is the first
	// (see Rtimezone_from_local).
	Rtime_picker_get_instant :: proc(time_picker: Rid) -> f64 ---
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
	// A chart of a kind (Rwidget_create(R_WIDGET_CHART) makes a pie).
	Rchart_create :: proc(kind: RChartKind) -> Rid ---
	Rchart_set_kind :: proc(chart: Rid, kind: RChartKind) ---
	Rchart_get_kind :: proc(chart: Rid) -> RChartKind ---
	// The title over a pie, a donut or a heatmap, under the value of a gauge (a sparkline only tells it to screen readers).
	Rchart_set_title :: proc(chart: Rid, title: cstring) ---
	Rchart_get_title :: proc(chart: Rid) -> cstring ---
	// How the values are written (the gauge, the middle of a donut, what
	// the pointer shows): a printf format for one double with its unit, "%.0f%%",
	// "$%.2f". NULL: whole numbers as they are, others with up to two decimals.
	Rchart_set_format :: proc(chart: Rid, format: cstring) ---
	// The color of the arc of a gauge outside any band, and of the line of a sparkline (default: the accent).
	Rchart_set_color :: proc(chart: Rid, r: f32, g: f32, b: f32, a: f32) ---
	// The legend: the slices beside a pie or a donut, the scale of colors under a heatmap (shown by default).
	Rchart_set_legend :: proc(chart: Rid, shown: i32) ---
	// Removes the slices, the bands, the grid and the values.
	Rchart_clear :: proc(chart: Rid) ---
	// What is under the pointer: a slice, a point of a sparkline, or a
	// cell of a heatmap as row * columns + column; -1 for nothing.
	Rchart_get_hovered :: proc(chart: Rid) -> i32 ---
	// Adds a slice (a value of 0 or more). Returns its index, -1 when the value is not valid.
	Rchart_add_slice :: proc(chart: Rid, label: cstring, value: f64) -> i32 ---
	Rchart_remove_slice :: proc(chart: Rid, slice: i32) ---
	Rchart_get_slice_count :: proc(chart: Rid) -> i32 ---
	Rchart_set_slice_value :: proc(chart: Rid, slice: i32, value: f64) ---
	Rchart_get_slice_value :: proc(chart: Rid, slice: i32) -> f64 ---
	Rchart_set_slice_label :: proc(chart: Rid, slice: i32, label: cstring) ---
	Rchart_get_slice_label :: proc(chart: Rid, slice: i32) -> cstring ---
	// The color of a slice; an alpha of 0 gives it back its color of the palette.
	Rchart_set_slice_color :: proc(chart: Rid, slice: i32, r: f32, g: f32, b: f32, a: f32) ---
	// The sum of the values of the slices.
	Rchart_get_total :: proc(chart: Rid) -> f64 ---
	// The slice lifted as selected, or -1 for none (the user selects one with a click).
	Rchart_set_selected :: proc(chart: Rid, slice: i32) ---
	Rchart_get_selected :: proc(chart: Rid) -> i32 ---
	// The hole of a donut, as a part of its radius, from 0.2 to 0.9 (default 0.62).
	Rchart_set_hole :: proc(chart: Rid, ratio: f32) ---
	// What the middle of a donut shows when the pointer is not over a
	// slice: `text` in large (NULL or "": the total) over `caption` in small
	// (NULL or "": none).
	Rchart_set_center_text :: proc(chart: Rid, text: cstring, caption: cstring) ---
	Rchart_set_slice_clicked_callback :: proc(chart: Rid, callback: RChartSliceCallback, user_data: rawptr) ---
	// The range of a gauge (default 0 to 100), and a fixed range for the
	// colors of a heatmap and the height of a sparkline, which otherwise follow
	// their values. min == max goes back to these defaults.
	Rchart_set_range :: proc(chart: Rid, min_: f64, max_: f64) ---
	// The range in use now (a gauge's, or the one of the values of a heatmap or a sparkline).
	Rchart_get_range :: proc(chart: Rid, min_: ^f64, max_: ^f64) ---
	// The value of a gauge, kept within its range when drawn.
	Rchart_set_value :: proc(chart: Rid, value: f64) ---
	Rchart_get_value :: proc(chart: Rid) -> f64 ---
	// Colors a part of the range of a gauge (an alpha of 0: the success,
	// warning and danger colors of the theme, one after the other). The arc
	// takes the color of the band its value is in. Returns the index of the band.
	Rchart_add_band :: proc(chart: Rid, from: f64, to: f64, r: f32, g: f32, b: f32, a: f32) -> i32 ---
	Rchart_clear_bands :: proc(chart: Rid) ---
	Rchart_get_band_count :: proc(chart: Rid) -> i32 ---
	// The cells of a heatmap: `rows` x `columns` values, row after row,
	// copied (NULL: all 0). NaN leaves a cell empty.
	Rchart_set_grid :: proc(chart: Rid, rows: i32, columns: i32, values: [^]f32) ---
	Rchart_get_row_count :: proc(chart: Rid) -> i32 ---
	Rchart_get_column_count :: proc(chart: Rid) -> i32 ---
	Rchart_set_cell :: proc(chart: Rid, row: i32, column: i32, value: f32) ---
	Rchart_get_cell :: proc(chart: Rid, row: i32, column: i32) -> f32 ---
	// The names of the rows, separated by '|': "Mon|Tue|Wed" (shown at their left).
	Rchart_set_row_labels :: proc(chart: Rid, labels: cstring) ---
	// The names of the columns, separated by '|' (shown over them, as many as fit).
	Rchart_set_column_labels :: proc(chart: Rid, labels: cstring) ---
	// The name of a row or of a column, "" when it has none.
	Rchart_get_row_label :: proc(chart: Rid, row: i32) -> cstring ---
	Rchart_get_column_label :: proc(chart: Rid, column: i32) -> cstring ---
	// The colors of the lowest and of the highest values of a heatmap (default: a tint of the accent, the accent).
	Rchart_set_color_scale :: proc(chart: Rid, low_r: f32, low_g: f32, low_b: f32, low_a: f32, high_r: f32, high_g: f32, high_b: f32, high_a: f32) ---
	Rchart_set_cell_clicked_callback :: proc(chart: Rid, callback: RChartCellCallback, user_data: rawptr) ---
	// The values of a sparkline, copied. NaN leaves a gap.
	Rchart_set_values :: proc(chart: Rid, values: [^]f32, count: i32) ---
	// Adds a value after the others, for live data; only the last `max_count` are kept (0: all of them).
	Rchart_push_value :: proc(chart: Rid, value: f32, max_count: i32) ---
	Rchart_get_value_count :: proc(chart: Rid) -> i32 ---
	Rchart_get_value_at :: proc(chart: Rid, index: i32) -> f32 ---
	// Shades the area under the line of a sparkline (on by default).
	Rchart_set_fill :: proc(chart: Rid, fill: i32) ---
	// A dot on the last value of a sparkline (on by default).
	Rchart_set_marker :: proc(chart: Rid, shown: i32) ---
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
	Rcarousel_create :: proc() -> Rid ---
	// Shows a page, gliding to it when `animated` (and the animations
	// are on), at once otherwise. The page changed callback is not called.
	Rcarousel_set_page :: proc(carousel: Rid, page: i32, animated: i32) ---
	// The page shown, -1 when the carousel has none.
	Rcarousel_get_page :: proc(carousel: Rid) -> i32 ---
	// Its children.
	Rcarousel_get_page_count :: proc(carousel: Rid) -> i32 ---
	// Glides to the next / previous page; nothing at the last / first one unless it loops.
	Rcarousel_next :: proc(carousel: Rid) ---
	Rcarousel_prev :: proc(carousel: Rid) ---
	// R_TRUE: after the last page comes the first one, and before the first the last (default R_FALSE).
	Rcarousel_set_loop :: proc(carousel: Rid, loop: i32) ---
	Rcarousel_get_loop :: proc(carousel: Rid) -> i32 ---
	// Goes to the next page by itself every `seconds` (0: never, the
	// default); after the last page it comes back to the first. It waits
	// while the pointer is on the carousel, while the pages are dragged and
	// while the keyboard is in it; a page chosen by the user starts the
	// interval again.
	Rcarousel_set_autoplay :: proc(carousel: Rid, seconds: f64) ---
	Rcarousel_get_autoplay :: proc(carousel: Rid) -> f64 ---
	// The dots along the bottom edge (the right edge when vertical), one per page (default R_TRUE).
	Rcarousel_set_indicators_visible :: proc(carousel: Rid, visible: i32) ---
	Rcarousel_get_indicators_visible :: proc(carousel: Rid) -> i32 ---
	// The previous / next buttons shown while the pointer is on the carousel (default R_TRUE).
	Rcarousel_set_arrows_visible :: proc(carousel: Rid, visible: i32) ---
	Rcarousel_get_arrows_visible :: proc(carousel: Rid) -> i32 ---
	// R_HORIZONTAL: the pages side by side (default); R_VERTICAL: one above the other.
	Rcarousel_set_orientation :: proc(carousel: Rid, orientation: ROrientation) ---
	Rcarousel_get_orientation :: proc(carousel: Rid) -> ROrientation ---
	Rcarousel_set_page_changed_callback :: proc(carousel: Rid, callback: RCarouselPageChangedCallback, user_data: rawptr) ---
	// Prints the document shown (see Rpdf_print); the job is named after its title.
	Rpdf_view_print :: proc(view: Rid, desc: ^RPdfPrintDesc) -> i32 ---
	Rmap_view_create :: proc() -> Rid ---
	// Centers the map on a place. The latitude is held within +/-85.0511; the longitude wraps.
	Rmap_view_set_center :: proc(map_: Rid, latitude: f64, longitude: f64) ---
	// The place at the middle of the map (longitude from -180 to 180).
	Rmap_view_get_center :: proc(map_: Rid, latitude: ^f64, longitude: ^f64) ---
	// The zoom: 0 shows the whole world in one tile, each step doubles the scale. Fractions are allowed.
	Rmap_view_set_zoom :: proc(map_: Rid, zoom: f64) ---
	Rmap_view_get_zoom :: proc(map_: Rid) -> f64 ---
	// The zooms the user can reach (default 0 to 19, at most 0 to 24). Tiles are asked for at whole zooms of this range.
	Rmap_view_set_zoom_range :: proc(map_: Rid, min_zoom: f64, max_zoom: f64) ---
	Rmap_view_get_zoom_range :: proc(map_: Rid, min_zoom: ^f64, max_zoom: ^f64) ---
	// Glides to a place and a zoom (at once when animations are off).
	Rmap_view_animate_to :: proc(map_: Rid, latitude: f64, longitude: f64, zoom: f64) ---
	// The place at a point of the widget.
	Rmap_view_screen_to_latlon :: proc(map_: Rid, x: f32, y: f32, latitude: ^f64, longitude: ^f64) ---
	// Where a place is in the widget (of the copies of the world side by side, the one nearest the middle).
	Rmap_view_latlon_to_screen :: proc(map_: Rid, latitude: f64, longitude: f64, x: ^f32, y: ^f32) ---
	// Called once for each tile the map needs. Answer with
	// Rmap_view_set_tile, Rmap_view_set_tile_data or Rmap_view_set_tile_failed,
	// at once or later. Setting it forgets the tiles of the previous source.
	Rmap_view_set_tile_callback :: proc(map_: Rid, callback: RMapTileCallback, user_data: rawptr) ---
	// Called when a tile asked for and not answered yet is no longer wanted (the user went elsewhere). Optional.
	Rmap_view_set_tile_cancel_callback :: proc(map_: Rid, callback: RMapTileCallback, user_data: rawptr) ---
	// The pixels of a tile: `width` x `height` RGBA8, tightly packed
	// (they are copied; any size, shown at the tile size). Thread-safe: from
	// another thread it is applied on the UI thread during the next frame.
	Rmap_view_set_tile :: proc(map_: Rid, z: i32, x: i32, y: i32, rgba: [^]u8, width: i32, height: i32) ---
	// A tile as the bytes of an image file (PNG, JPEG...), decoded on the calling thread. Thread-safe.
	Rmap_view_set_tile_data :: proc(map_: Rid, z: i32, x: i32, y: i32, data: rawptr, data_size: uint) ---
	// There is no such tile: the coarser one stays shown in its place. Thread-safe.
	Rmap_view_set_tile_failed :: proc(map_: Rid, z: i32, x: i32, y: i32) ---
	// Reads the tiles from files instead of asking for them: a path
	// where {z}, {x} and {y} are replaced ({-y}: the row counted from the
	// south, as in TMS and MBTiles). The files are read and decoded on a
	// worker thread; a missing file is a tile that failed. NULL or "": the
	// tile callback again. Local files only.
	Rmap_view_set_tile_path :: proc(map_: Rid, path_template: cstring) ---
	Rmap_view_get_tile_path :: proc(map_: Rid) -> cstring ---
	// Logical size of a tile at a whole zoom (default 256, from 16 to 1024): larger tiles show the map larger.
	Rmap_view_set_tile_size :: proc(map_: Rid, size: i32) ---
	Rmap_view_get_tile_size :: proc(map_: Rid) -> i32 ---
	// Memory the decoded tiles may take, in bytes (default 64 MiB). The tiles on screen are always kept.
	Rmap_view_set_cache_size :: proc(map_: Rid, bytes: uint) ---
	// Memory the decoded tiles take now, in bytes.
	Rmap_view_get_cache_usage :: proc(map_: Rid) -> uint ---
	// Forgets every tile (they are asked for again): the tiles of the source changed.
	Rmap_view_clear_tiles :: proc(map_: Rid) ---
	Rmap_view_get_tile_state :: proc(map_: Rid, z: i32, x: i32, y: i32) -> RMapTileState ---
	// Adds a pin at a place, with a label written next to it (may be NULL). Returns its id (from 1), or -1.
	Rmap_view_add_marker :: proc(map_: Rid, latitude: f64, longitude: f64, label: cstring) -> i32 ---
	Rmap_view_remove_marker :: proc(map_: Rid, marker: i32) ---
	Rmap_view_clear_markers :: proc(map_: Rid) ---
	Rmap_view_get_marker_count :: proc(map_: Rid) -> i32 ---
	Rmap_view_set_marker_position :: proc(map_: Rid, marker: i32, latitude: f64, longitude: f64) ---
	Rmap_view_set_marker_label :: proc(map_: Rid, marker: i32, label: cstring) ---
	// The color of a pin (by default the accent of the theme).
	Rmap_view_set_marker_color :: proc(map_: Rid, marker: i32, r: f32, g: f32, b: f32, a: f32) ---
	Rmap_view_set_marker_clicked_callback :: proc(map_: Rid, callback: RMapMarkerCallback, user_data: rawptr) ---
	Rmap_view_set_view_changed_callback :: proc(map_: Rid, callback: RMapViewCallback, user_data: rawptr) ---
	// A line written in the bottom right corner: the credit most tile
	// licences ask for, such as "(c) OpenStreetMap contributors". NULL or "": none.
	Rmap_view_set_attribution :: proc(map_: Rid, text: cstring) ---
	Rmap_view_get_attribution :: proc(map_: Rid) -> cstring ---
	// Returns: R_TRUE when web views show pages here (the system has an engine Raven can embed, and Raven runs with a window system).
	Rsystem_web_view_is_supported :: proc() -> i32 ---
	Rsystem_web_view_create :: proc() -> Rid ---
	// Goes to an address ("https://...", "file:///...", "about:blank"). A file: address may read the files of its folder.
	Rsystem_web_view_load_url :: proc(view: Rid, url: cstring) ---
	// Shows a file of the computer (an .html file, and what it refers to in its folder).
	Rsystem_web_view_load_file :: proc(view: Rid, path: cstring) ---
	// Shows HTML given as text.
	// base_url Where the relative addresses of the text are looked
	// for ("file:///Users/me/app/pages/", "https://example.org/"), or NULL.
	Rsystem_web_view_load_html :: proc(view: Rid, html: cstring, base_url: cstring) ---
	Rsystem_web_view_reload :: proc(view: Rid) ---
	Rsystem_web_view_stop :: proc(view: Rid) ---
	Rsystem_web_view_go_back :: proc(view: Rid) ---
	Rsystem_web_view_go_forward :: proc(view: Rid) ---
	Rsystem_web_view_can_go_back :: proc(view: Rid) -> i32 ---
	Rsystem_web_view_can_go_forward :: proc(view: Rid) -> i32 ---
	// Returns: The address of the page shown (the one asked for, where unsupported). Valid until it changes.
	Rsystem_web_view_get_url :: proc(view: Rid) -> cstring ---
	// Returns: The title of the page, "" when it has none. Valid until it changes.
	Rsystem_web_view_get_title :: proc(view: Rid) -> cstring ---
	Rsystem_web_view_is_loading :: proc(view: Rid) -> i32 ---
	// Returns: Why the last page could not be shown, or "" when all is well.
	Rsystem_web_view_get_error :: proc(view: Rid) -> cstring ---
	// Runs JavaScript in the page; the value of its last statement is
	// the result. If it is a promise, the result is what the promise
	// resolves to. The callback (may be NULL) always comes later, never
	// during this call.
	Rsystem_web_view_run_javascript :: proc(view: Rid, script: cstring, callback: RSystemWebViewResultCallback, user_data: rawptr) ---
	Rsystem_web_view_set_message_callback :: proc(view: Rid, callback: RSystemWebViewMessageCallback, user_data: rawptr) ---
	// Called when something happens in the page. The action of the
	// widget (Rwidget_set_action, OnChanged in XML) is fired as well each
	// time the address changes.
	Rsystem_web_view_set_event_callback :: proc(view: Rid, callback: RSystemWebViewEventCallback, user_data: rawptr) ---
	// The view of the system, to go further than these functions:
	// a WKWebView* on macOS and iOS. NULL where unsupported.
	Rsystem_web_view_get_native_handle :: proc(view: Rid) -> rawptr ---
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
	// would (at every cursor when there are several): it can be undone and
	// the changed callback is called.
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
	// Adds a cursor (a caret) to the editor; the main cursor stays
	// the main one. A cursor over another one, or at the place of another
	// caret, becomes one with it. Rtext_editor_set_cursor,
	// Rtext_editor_select and Rtext_editor_set_text go back to one cursor.
	Rtext_editor_add_cursor :: proc(editor: Rid, line: i32, column: i32) ---
	// Adds a cursor that selects from one position to another (its caret on the second).
	Rtext_editor_add_selection :: proc(editor: Rid, from_line: i32, from_column: i32, to_line: i32, to_column: i32) ---
	// Returns: The number of cursors, the main one included (1 at least).
	Rtext_editor_get_cursor_count :: proc(editor: Rid) -> i32 ---
	// A cursor by its index, in the order of the text (0 for the
	// first): its caret and the anchor of its selection (the same position
	// when nothing is selected). Returns R_FALSE when `index` is out of range.
	Rtext_editor_get_cursor_at :: proc(editor: Rid, index: i32, line: ^i32, column: ^i32, anchor_line: ^i32, anchor_column: ^i32) -> i32 ---
	// Returns: The index of the main cursor, in the order of Rtext_editor_get_cursor_at.
	Rtext_editor_get_main_cursor :: proc(editor: Rid) -> i32 ---
	// Back to the main cursor alone, as Escape does.
	Rtext_editor_clear_cursors :: proc(editor: Rid) ---
	// What Ctrl+D does: with nothing selected, selects the word at
	// each caret; else adds a cursor on the next occurrence of the main
	// selection (with its case, a whole word when it was selected so) that no
	// cursor selects, going on from the start of the text. Returns R_FALSE
	// when there is none.
	Rtext_editor_add_next_occurrence :: proc(editor: Rid) -> i32 ---
	// What Ctrl+Shift+L does: a cursor on every occurrence of the main selection (or of the word at the caret). Returns their number.
	Rtext_editor_select_all_occurrences :: proc(editor: Rid) -> i32 ---
	// Folds the region that starts at a line: the lines after it, up
	// to the one before the bracket that closes it (or to the last one more
	// indented), are hidden. When the line starts none, or it is folded
	// already, the nearest region around the line is folded. Cursors in it
	// go to the end of its first line. Returns R_FALSE when there is none.
	// Line numbers stay those of the text. Folds follow the lines when text
	// is added or removed elsewhere; editing a hidden line (or the line break
	// of the first line) unfolds its region, and so does a match of the
	// search or a cursor that goes into it.
	Rtext_editor_fold :: proc(editor: Rid, line: i32) -> i32 ---
	// Unfolds the region that starts at a line, or what hides it. Returns R_FALSE when nothing was folded there.
	Rtext_editor_unfold :: proc(editor: Rid, line: i32) -> i32 ---
	// Folds every region, those inside others too.
	Rtext_editor_fold_all :: proc(editor: Rid) ---
	Rtext_editor_unfold_all :: proc(editor: Rid) ---
	// Returns: R_TRUE when a folded region starts at this line.
	Rtext_editor_is_folded :: proc(editor: Rid, line: i32) -> i32 ---
	// Returns: R_TRUE when the line is hidden in a folded region.
	Rtext_editor_is_line_hidden :: proc(editor: Rid, line: i32) -> i32 ---
	// Returns: The last line of the region that starts at a line (folded or not), -1 when it starts none.
	Rtext_editor_get_fold_end :: proc(editor: Rid, line: i32) -> i32 ---
	// R_FALSE unfolds everything and stops folding: no markers, and
	// Ctrl+Shift+[ / ] go to the shortcuts of the window. On by default.
	// XML: Folding="false".
	Rtext_editor_set_folding :: proc(editor: Rid, enabled: i32) ---
	Rtext_editor_get_folding :: proc(editor: Rid) -> i32 ---
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
	// Underlines the misspelled words with a wavy line (R_COLOR_SPELLING);
	// a right click on one offers corrections (see SPELL CHECKING in
	// raven_controls.h). Only the lines shown are checked, again when they
	// change. Off by default: it is meant for prose, not for code.
	Rtext_editor_set_spell_check :: proc(editor: Rid, enabled: i32) ---
	Rtext_editor_get_spell_check :: proc(editor: Rid) -> i32 ---
	// Language of the spell checking ("en_US", "fr"...). NULL: the default (Rspell_set_language).
	Rtext_editor_set_spell_language :: proc(editor: Rid, language: cstring) ---
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
	// Sets what is searched for, with RSearchFlags, and shows every
	// match of the visible text (theme colors R_COLOR_SEARCH_MATCH and
	// R_COLOR_SEARCH_CURRENT). A match lies on one line: a query with a line
	// break finds nothing. NULL or "" clears the search. The selection does
	// not move; Rtext_editor_find_next goes to a match.
	Rtext_editor_set_search :: proc(editor: Rid, query: cstring, flags: u32) ---
	// Returns: The text searched for, "" when there is none.
	Rtext_editor_get_search :: proc(editor: Rid) -> cstring ---
	// Returns: The RSearchFlags of the search.
	Rtext_editor_get_search_flags :: proc(editor: Rid) -> u32 ---
	// Selects the next match after the selection (or the cursor),
	// going on from the start of the text after its end, and scrolls to it.
	// Returns R_FALSE when the text has no match.
	Rtext_editor_find_next :: proc(editor: Rid) -> i32 ---
	// The same, toward the start: the match before the selection, from the end of the text after its start.
	Rtext_editor_find_previous :: proc(editor: Rid) -> i32 ---
	// Returns: The number of matches in the whole text. Counted once per change of the text or of the search.
	Rtext_editor_get_match_count :: proc(editor: Rid) -> i32 ---
	// Returns: The index of the selected match (0 for the first in the text), -1 when the selection is not a match.
	Rtext_editor_get_match_index :: proc(editor: Rid) -> i32 ---
	// Where a match is: its line and the columns of its first byte and
	// of the byte after it. Returns R_FALSE when `index` is out of range.
	Rtext_editor_get_match :: proc(editor: Rid, index: i32, line: ^i32, from_column: ^i32, to_column: ^i32) -> i32 ---
	// Selects a match by its index and scrolls to it. Returns R_FALSE when `index` is out of range.
	Rtext_editor_select_match :: proc(editor: Rid, index: i32) -> i32 ---
	// Replaces the selected match by `replacement`, as one step of
	// the history, then selects the next match. When the selection is not a
	// match, only selects the next one (the user sees what will be replaced).
	// Returns R_TRUE when something was replaced. As with
	// Rtext_editor_insert_text, the changed callback is called.
	Rtext_editor_replace :: proc(editor: Rid, replacement: cstring) -> i32 ---
	// Replaces every match by `replacement`, as a single step of the
	// history (one undo puts them all back). Returns the number of matches
	// replaced.
	Rtext_editor_replace_all :: proc(editor: Rid, replacement: cstring) -> i32 ---
	// Opens the find bar of the editor (Ctrl+F), with the row to
	// replace when `replace` is R_TRUE (Ctrl+H), and gives it the keyboard.
	// Text selected on one line becomes the text searched for.
	// Its texts are English unless the locale has the keys Raven.Find,
	// Raven.Replace, Raven.ReplaceAll, Raven.FindNoResults, Raven.FindCount
	// ("{0} of {1}"), Raven.FindMatchCase, Raven.FindWholeWord,
	// Raven.FindPrevious, Raven.FindNext, Raven.FindClose and
	// Raven.FindToggleReplace.
	Rtext_editor_show_find :: proc(editor: Rid, replace: i32) ---
	// Closes the find bar; the matches are no longer shown, F3 still goes to the next one.
	Rtext_editor_hide_find :: proc(editor: Rid) ---
	Rtext_editor_is_find_shown :: proc(editor: Rid) -> i32 ---
	// R_FALSE removes the find bar and its keys (Ctrl+F, Ctrl+H, F3,
	// Cmd+G), which then go to the shortcuts of the window: for an
	// application with its own search, built on Rtext_editor_set_search. On
	// by default. XML: FindBar="false".
	Rtext_editor_set_find_bar :: proc(editor: Rid, enabled: i32) ---
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
	// that match no panel are ignored in legacy descriptions. Versioned
	// descriptions with floating cards reject unknown or duplicate names.
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
	// Default for future floats/tears; existing floating panels keep their mode.
	Rdock_area_set_float_mode :: proc(area: Rid, mode: RDockFloatMode) ---
	// How the windows of torn-off panels are made (native windows,
	// R_DOCK_FLOAT_WINDOW; in-window cards are not windows).
	// flags    RWindowFlags they are created with: R_WINDOW_NO_RESIZE,
	//                 R_WINDOW_BORDERLESS, R_WINDOW_CUSTOM_HEADER and
	//                 R_WINDOW_TRANSPARENT (the others are ignored).
	//                 R_WINDOW_DEFAULT by default.
	// callback May be NULL: the floating area is the whole window.
	// Dragging the only tab of such a window still moves it, and closing
	// it (its header bar's close button too) docks its panels back.
	Rdock_area_set_float_window :: proc(area: Rid, flags: u32, callback: RDockFloatWindowCallback, user_data: rawptr) ---
	// Floats a panel. Native mode: screen x/y; in-window mode: window-local logical x/y. Returns its containing window.
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
	// Color of the badge pill of one node (Rnode_graph_set_node_badge); its text turns dark or light to suit it. A negative component restores the look's color.
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
	// The differences between two texts as a unified diff (the format
	// of `diff -u` and `git diff`), with `context` unchanged lines around each
	// change (3 is usual). `left_name` and `right_name` follow `---` and `+++`
	// (NULL: "a" and "b"). `flags`: RDiffFlags. It touches nothing else of
	// the library: it may be called from any thread.
	// Returns: The patch followed by a '\0' (an empty text when the texts are
	// the same), to release with Rmemory_free; NULL when a text is NULL.
	// `size` receives its length in bytes, and may be NULL.
	Rdiff_unified :: proc(left: cstring, right: cstring, left_name: cstring, right_name: cstring, context_: i32, flags: u32, size: ^uint) -> cstring ---
	// The number of changes (runs of lines removed and / or added) between two texts. Any thread.
	Rdiff_count_changes :: proc(left: cstring, right: cstring, flags: u32) -> i32 ---
	Rdiff_view_create :: proc() -> Rid ---
	// The two texts to compare (NULL: empty). Lines end with "\n" or "\r\n".
	Rdiff_view_set_texts :: proc(diff_view: Rid, left: cstring, right: cstring) ---
	// Names shown over the two sides (file names, "Before" / "After"); NULL: none. Their extension chooses the colors of the code.
	Rdiff_view_set_names :: proc(diff_view: Rid, left_name: cstring, right_name: cstring) ---
	// Compares two files. Their paths become the names unless names
	// were given. Returns: R_FALSE when a file cannot be read (it is compared
	// as empty).
	Rdiff_view_load_files :: proc(diff_view: Rid, left_path: cstring, right_path: cstring) -> i32 ---
	// Shows a patch (unified diff: the output of `git diff`, `diff -u`)
	// instead of two texts: each file with its name, each hunk with its header.
	// Returns: R_FALSE when the text holds no hunk (nothing is shown).
	Rdiff_view_set_patch :: proc(diff_view: Rid, patch: cstring) -> i32 ---
	Rdiff_view_set_mode :: proc(diff_view: Rid, mode: RDiffViewMode) ---
	Rdiff_view_get_mode :: proc(diff_view: Rid) -> RDiffViewMode ---
	// Unchanged lines shown around each change (3 by default); the
	// others are folded. A negative number shows every line.
	Rdiff_view_set_context :: proc(diff_view: Rid, lines: i32) ---
	Rdiff_view_get_context :: proc(diff_view: Rid) -> i32 ---
	// How lines are compared: RDiffFlags (0 by default). Not used by a patch.
	Rdiff_view_set_flags :: proc(diff_view: Rid, flags: u32) ---
	Rdiff_view_get_flags :: proc(diff_view: Rid) -> u32 ---
	// Marks the words that changed inside a changed line (default R_TRUE).
	Rdiff_view_set_word_diff :: proc(diff_view: Rid, enabled: i32) ---
	// Shows the numbers of the lines (default R_TRUE).
	Rdiff_view_set_line_numbers :: proc(diff_view: Rid, visible: i32) ---
	// Colors the code as the text editor does: a language name of
	// Rlanguage_register ("C", "Python", "JSON"...), "None" for plain text,
	// NULL (default) to guess it from the extension of the names.
	Rdiff_view_set_language :: proc(diff_view: Rid, language: cstring) ---
	// The number of changes: runs of lines removed and / or added.
	Rdiff_view_get_change_count :: proc(diff_view: Rid) -> i32 ---
	// Where a change is: its first line and its number of lines on
	// each side (a count of 0: nothing on that side; the line is then the one
	// after which the other side's lines go). Returns: R_FALSE when there is
	// no such change.
	Rdiff_view_get_change :: proc(diff_view: Rid, change: i32, left_line: ^i32, left_count: ^i32, right_line: ^i32, right_count: ^i32) -> i32 ---
	// Scrolls to a change (from 0) and marks it as the current one.
	Rdiff_view_go_to_change :: proc(diff_view: Rid, change: i32) ---
	// The current change, -1 before the first one is gone to.
	Rdiff_view_get_current_change :: proc(diff_view: Rid) -> i32 ---
	// Goes to the next / previous change. Returns: Its index, -1 when there is none.
	Rdiff_view_next_change :: proc(diff_view: Rid) -> i32 ---
	Rdiff_view_prev_change :: proc(diff_view: Rid) -> i32 ---
	// The rows shown, folds, hunk headers and notes included.
	Rdiff_view_get_row_count :: proc(diff_view: Rid) -> i32 ---
	// What a row shows, and the numbers of its lines (0 for a side it
	// has no line of). A fold gives the first line it hides on each side.
	Rdiff_view_get_row :: proc(diff_view: Rid, row: i32, left_line: ^i32, right_line: ^i32) -> RDiffRowKind ---
	// The text of a row on one side (in unified mode the side does not
	// matter for a line); a fold, a hunk, a file or a note give their label.
	// Valid until the next call. "" when there is none.
	Rdiff_view_get_row_text :: proc(diff_view: Rid, row: i32, side: RDiffSide) -> cstring ---
	// Shows the lines of a fold. Returns: R_FALSE when the row is not a fold.
	Rdiff_view_expand :: proc(diff_view: Rid, row: i32) -> i32 ---
	// The first row at the top of the view; Rdiff_view_scroll_to_row brings one there.
	Rdiff_view_get_first_visible_row :: proc(diff_view: Rid) -> i32 ---
	Rdiff_view_scroll_to_row :: proc(diff_view: Rid, row: i32) ---
	// Attaches a short note to a line (a review comment, an error): a
	// mark in the gutter and the text in a row under the line, which is never
	// folded away. Several notes may go to one line.
	Rdiff_view_add_annotation :: proc(diff_view: Rid, side: RDiffSide, line: i32, text: cstring) ---
	Rdiff_view_clear_annotations :: proc(diff_view: Rid) ---
	// The text selected by the user ("" when none), one line after another.
	Rdiff_view_get_selected_text :: proc(diff_view: Rid) -> cstring ---
	// Called when a line or its number is clicked (the action of the widget is fired too).
	Rdiff_view_set_line_clicked_callback :: proc(diff_view: Rid, callback: RDiffViewLineCallback, user_data: rawptr) ---
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
	// On Android and iOS, a relative path that names no file held in memory
	// and no file on disk is read from the app's package (the APK's assets/,
	// the bundle's resources), and so is every file Raven reads by its path.
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
	// R_FALSE when the system shows no notifications here (Rnotification_show then always uses
	// the fallback): the null platform, no notification server on Linux, an executable outside of a
	// bundle on macOS.
	Rnotification_is_supported :: proc() -> i32 ---
	// Called with the notifications the system could not show; NULL: none.
	Rnotification_set_fallback :: proc(callback: RNotificationFallback, user_data: rawptr) ---
	// Called when the user clicks a notification of Rnotification_show
	// (on the UI thread), as R_NOTIFICATION_ACTIVATED of
	// Rnotification_set_callback, which also tells which one. NULL: none.
	Rnotification_set_clicked_callback :: proc(callback: RNotificationClickedCallback, user_data: rawptr) ---
	// Shows a notification described by `desc` (see RNotificationDesc).
	// Returns: R_TRUE when the system took it; R_FALSE when it went to the fallback (or nowhere).
	Rnotification_show_desc :: proc(desc: ^RNotificationDesc) -> i32 ---
	// Takes away the notification of that tag, shown by the system or by the fallback toast. The callback is not called.
	Rnotification_close :: proc(tag: cstring) ---
	Rnotification_set_callback :: proc(callback: RNotificationCallback, user_data: rawptr) ---
	// R_TRUE (the default): a click on a notification (not on one of
	// its buttons) brings its window back before the callback is called:
	// shown again if it was hidden or minimized, in front, with the keyboard.
	// R_FALSE: the application does what it wants in the callback.
	Rnotification_set_bring_to_front :: proc(enabled: i32) ---
	// A Toast that shows the notifications the system could not show,
	// with their title, icon and buttons; a click on one, or on a button, is
	// reported to Rnotification_set_callback as for the system's. Put it in
	// an Overlay (Queue="Stack" shows several at once). It takes the place
	// of the fallback callback (Rnotification_set_fallback). R_INVALID_ID: none.
	Rnotification_set_fallback_toast :: proc(toast: Rid) ---
	// Whether the notifications of the system have that feature here (R_FALSE where there are none).
	Rnotification_supports :: proc(feature: RNotificationFeature) -> i32 ---
	// What the user answered, without asking. Cheap: the platform
	// remembers it. On iOS the status of notifications is known shortly after
	// the start (R_PERMISSION_NOT_ASKED until then).
	Rpermission_get_status :: proc(permission: RPermission) -> RPermissionStatus ---
	// Asks the user, with the question of the system, when the status
	// is R_PERMISSION_NOT_ASKED or R_PERMISSION_DENIED; otherwise only answers
	// with the status. Several requests for the same permission share one
	// question. `callback` may be NULL.
	Rpermission_request :: proc(permission: RPermission, callback: RPermissionCallback, user_data: rawptr) ---
	// Opens the page of the application in the Settings of the system,
	// where the user changes what was refused for good (R_PERMISSION_BLOCKED).
	// Returns: R_FALSE when the platform has no such page.
	Rpermission_open_settings :: proc() -> i32 ---
	// Returns: The name of a permission or a status ("camera", "granted"...), for logs.
	Rpermission_to_string :: proc(permission: RPermission) -> cstring ---
	Rpermission_status_to_string :: proc(status: RPermissionStatus) -> cstring ---
	// Asks for one file to read (several with R_FILE_DIALOG_MULTIPLE). `desc` may be NULL.
	Rfile_open_async :: proc(desc: ^RFileDialogDesc, callback: RDeviceCallback, user_data: rawptr) ---
	// Saves `size` bytes (copied: `data` may be freed at once) as a
	// file where the user chooses, proposed under `desc->file_name`. The
	// callback gives the file as saved: its name, and its path on desktops.
	Rfile_save_async :: proc(desc: ^RFileDialogDesc, data: rawptr, size: uint, callback: RDeviceCallback, user_data: rawptr) ---
	// Same as Rfile_save_async, for a file that already exists (a path Raven can read), copied where the user chooses.
	Rfile_export_async :: proc(desc: ^RFileDialogDesc, path: cstring, callback: RDeviceCallback, user_data: rawptr) ---
	// Asks for a folder: desktops only for now (R_DEVICE_UNSUPPORTED on iOS and Android).
	Rfile_select_folder_async :: proc(desc: ^RFileDialogDesc, callback: RDeviceCallback, user_data: rawptr) ---
	// Returns: R_TRUE when Rcamera_take_photo can show a camera (none on desktops, nor in the iOS Simulator).
	Rcamera_is_available :: proc() -> i32 ---
	// Shows the camera of the system to take one photo. It asks for
	// R_PERMISSION_CAMERA first when needed (R_DEVICE_DENIED when refused).
	// The photo is a JPEG file in the cache of the application, with its
	// size in pixels, upright.
	Rcamera_take_photo :: proc(callback: RDeviceCallback, user_data: rawptr) ---
	// Shows the photo picker of the system (desktops: a file dialog for
	// images). No permission is needed: the application only sees what the
	// user picks. Each photo is a file Raven can load (iOS: converted to JPEG
	// when it is HEIC).
	// max_count How many photos may be picked; 0: no limit.
	Rphoto_pick :: proc(max_count: i32, callback: RDeviceCallback, user_data: rawptr) ---
	// Returns: R_TRUE when the device can play haptics (a phone; never a desktop).
	Rhaptic_is_supported :: proc() -> i32 ---
	// A short vibration, as the controls of the system play when
	// touched. Nothing happens where there are none, or when the user turned
	// them off. Returns: R_TRUE when it was handed to the system.
	Rhaptic_play :: proc(haptic: RHaptic) -> i32 ---
	// Shows the share sheet of the system with a text and / or a file,
	// to send them to another application (mail, chat, Files...). The
	// callback says R_DEVICE_DONE once something was chosen (Android: once the
	// sheet was shown; it does not tell more), R_DEVICE_CANCELLED when it was
	// closed. R_DEVICE_UNSUPPORTED on desktops.
	Rshare_show :: proc(desc: ^RShareDesc, callback: RDeviceCallback, user_data: rawptr) ---
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
	// Soft shadow or glow: a rounded rectangle blurred by `blur` logical units.
	// Offset the rectangle for a drop shadow, or draw it under a shape for a glow.
	// Nonpositive width, height or blur draws nothing. Uses the same primitive as Border shadows.
	Rdraw_shadow :: proc(painter: ^RPainter, x: f32, y: f32, width: f32, height: f32, radius: f32, blur: f32, r: f32, g: f32, b: f32, a: f32) ---
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
	// Turns live reload on or off. It is on by default, except in
	// the browser (Emscripten), where watching files would keep the page
	// drawing; the environment variable RAVEN_LIVE_RELOAD=0 or 1 changes the
	// default without rebuilding. When off, files are never looked at again.
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
	// Loads a script given as text (not an XML document): its
	// functions become actions, its globals are initialized.
	// name Names it in the diagnostics (a file name); may be NULL.
	// Returns: R_FALSE when it has an error (nothing of it is installed).
	Rscript_load :: proc(source: cstring, size: uint, name: cstring) -> i32 ---
	// The same from a file (.rvs).
	Rscript_load_file :: proc(path: cstring) -> i32 ---
	// Checks a script, or the scripts of an XML document and the
	// functions its widgets name, without running anything nor building a
	// widget. The diagnostics are reported as usual.
	// source A script, or an XML document (text that starts with '<').
	// Returns: The number of errors.
	Rscript_check :: proc(source: cstring, size: uint, name: cstring) -> i32 ---
	// Returns: The number of errors in the file (a .rvs or an .xml), -1 when it cannot be read.
	Rscript_check_file :: proc(path: cstring) -> i32 ---
	// Formats a script: one statement per line, blocks indented,
	// spaces around operators; comments are kept.
	// indent One level of indentation; NULL for four spaces.
	// Returns: The text, valid until the next call; NULL when the script has a
	// syntax error (it is not changed).
	Rscript_format :: proc(source: cstring, size: uint, indent: cstring) -> cstring ---
	// Removes every script function (their actions are unbound) and every global. Not while a script runs.
	Rscript_reset :: proc() ---
	// Returns: R_TRUE when a script function has that name.
	Rscript_has_function :: proc(name: cstring) -> i32 ---
	// R_FALSE: the <Script> elements of the documents are read but not
	// compiled nor run (a design tool that shows interfaces without running
	// them). On by default.
	Rscript_set_enabled :: proc(enabled: i32) ---
	Rscript_is_enabled :: proc() -> i32 ---
	// The limits of the scripts. 0 keeps a value.
	// max_instructions Instructions one invocation (an event) may run (10 000 000).
	// max_depth Calls nested (200).
	// max_memory Bytes of strings and arrays alive (64 MB).
	Rscript_set_limits :: proc(max_instructions: u32, max_depth: i32, max_memory: uint) ---
	Rscript_set_log_callback :: proc(callback: RScriptLogCallback, user_data: rawptr) ---
	Rscript_set_diagnostic_callback :: proc(callback: RScriptDiagnosticCallback, user_data: rawptr) ---
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
	// Forces the clock of every time shown (a setting of the
	// application): R_CLOCK_12_HOUR, R_CLOCK_24_HOUR, or R_CLOCK_AUTO (default)
	// to follow the locale. A TimePicker can still choose its own (Clock="24").
	Rlocale_set_clock :: proc(clock: RClock) ---
	Rlocale_get_clock :: proc() -> RClock ---
	// R_TRUE when times are shown on 12 hours with AM and PM. With
	// Rlocale_set_clock, what it forces; else, when the application chose a
	// language (Rlocale_set_language), the usage of that language (from CLDR:
	// en-US, en-CA, en-AU, en-IN, hi, ar, ko, zh-TW, fil... use 12 hours, most
	// others 24); else the setting of the user on the system (Windows, macOS,
	// the LC_TIME of Linux), or the usage of the language of the system.
	Rlocale_uses_12_hour_clock :: proc() -> i32 ---
	// "AM" (pm = R_FALSE) or "PM" in the language: the keys Raven.AM / Raven.PM when the tables have them. Never NULL.
	Rlocale_get_day_period :: proc(pm: i32) -> cstring ---
	// A time of the day (hour 0 to 23) as the locale writes it: "3:05 PM", "15:05", "오후 3:05".
	Rlocale_format_time :: proc(hour: i32, minute: i32, second: i32, show_seconds: i32) -> cstring ---
	// The zone of the system ("Europe/Paris"): the TZ variable, the
	// link /etc/localtime, else what the system says; "UTC" when it says
	// nothing. Valid until the next call.
	Rtimezone_get_system :: proc() -> cstring ---
	// The folder of the tz database (the files "Europe/Paris"...), or
	// the packed tzdata file of Android, to read instead of the system's: for
	// a database shipped with the application, or tests. It is then the only
	// one read. NULL: the system's (or the TZDIR variable). Forgets the zones
	// read so far.
	Rtimezone_set_directory :: proc(path: cstring) ---
	// R_TRUE when the zone is known. Every function takes a name of
	// the tz database, "UTC", a POSIX TZ rule ("EST5EDT,M3.2.0,M11.1.0"), or
	// NULL / "" for the zone of the system.
	Rtimezone_is_valid :: proc(zone: cstring) -> i32 ---
	// The date and time an instant shows in a zone (month 1 to 12). R_FALSE for an unknown zone.
	Rtimezone_to_local :: proc(zone: cstring, utc: f64, year: ^i32, month: ^i32, day: ^i32, hour: ^i32, minute: ^i32, second: ^i32) -> i32 ---
	// The instant a date and time of a zone is. When the clocks go
	// forward, the times they skip do not exist: such a time is moved forward
	// by the length of the gap (02:30 becomes 03:30). When they go back, the
	// times of the hour repeated happen twice: the first is taken (still in
	// summer time). Rtimezone_count_local tells these cases apart.
	Rtimezone_from_local :: proc(zone: cstring, year: i32, month: i32, day: i32, hour: i32, minute: i32, second: i32) -> f64 ---
	// How many instants show this date and time in the zone: 1, 0 (skipped when the clocks went forward) or 2 (repeated when they went back).
	Rtimezone_count_local :: proc(zone: cstring, year: i32, month: i32, day: i32, hour: i32, minute: i32, second: i32) -> i32 ---
	// Seconds the zone is ahead of UTC at an instant (3600 in Paris in winter, -14400 in New York in summer).
	Rtimezone_get_offset :: proc(zone: cstring, utc: f64) -> i32 ---
	// R_TRUE when the zone is on summer (daylight saving) time at the instant.
	Rtimezone_is_daylight_saving :: proc(zone: cstring, utc: f64) -> i32 ---
	// The abbreviation of the zone at an instant ("CET", "EDT"; "+03" where the database has no name).
	Rtimezone_get_abbreviation :: proc(zone: cstring, utc: f64) -> cstring ---
	// The zone at an instant, to show: "CET, UTC+1", "EDT, UTC-4", "IST, UTC+5:30", "UTC+3" when it has no name.
	Rtimezone_get_label :: proc(zone: cstring, utc: f64) -> cstring ---
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
	// What covers a host window on a phone or a tablet, in pixels: the
	// safe area (status bar, notch, home indicator; NULL: none) and the height
	// of the on-screen keyboard over its bottom (0: hidden). The content is laid
	// out inside them (Rwindow_get_safe_area, Rwindow_get_keyboard_area).
	Rhost_window_set_insets :: proc(window: Rid, safe_area: ^RInsets, keyboard_height: i32) ---
	// The application's system is in dark (R_TRUE) or light (R_FALSE)
	// mode, -1 when it does not tell (the default): Rtheme_get_system_dark,
	// and Rtheme_follow_system follows it.
	Rhost_set_system_dark :: proc(dark: i32) ---
	Rhost_inject_mouse_move :: proc(window: Rid, x: i32, y: i32, modifiers: u32) ---
	Rhost_inject_mouse_button :: proc(window: Rid, x: i32, y: i32, button: RMouseButton, pressed: i32, modifiers: u32) ---
	Rhost_inject_mouse_wheel :: proc(window: Rid, wheel_x: f32, wheel_y: f32, modifiers: u32) ---
	// The pointer left the window: hover states end.
	Rhost_inject_mouse_leave :: proc(window: Rid) ---
	// repeat R_TRUE for a key held down (auto-repeat).
	Rhost_inject_key :: proc(window: Rid, key: RKey, pressed: i32, modifiers: u32, repeat: i32) ---
	// One typed Unicode code point.
	Rhost_inject_text :: proc(window: Rid, codepoint: u32) ---
	// The input method of the application's system composes `text`
	// (preedit), with its selection in bytes (see Rwindow_inject_composition).
	// NULL or "" cancels it; the committed text then comes through
	// Rhost_inject_text. Rwindow_get_text_input_rect tells where to put the
	// candidate window.
	Rhost_inject_composition :: proc(window: Rid, text: cstring, selection_start: i32, selection_end: i32) ---
	// The window gained or lost the keyboard. Losing it closes popups and drops captures.
	Rhost_inject_focus :: proc(window: Rid, focused: i32) ---
	// The user asked to close the window (its close callback decides).
	Rhost_inject_close :: proc(window: Rid) ---
	// The user answered a permission_request (or the status changed
	// by itself). The waiting Rpermission_request callbacks get it at the
	// next frame. Any thread.
	Rhost_permission_answer :: proc(permission: RPermission, status: RPermissionStatus) ---
	// The end of a service_request: the files it gave (copied; paths
	// Raven can read; size 0 is read from the file), at the next frame. Once
	// per request. Any thread.
	Rhost_service_complete :: proc(request: u32, result: RDeviceResult, files: ^RPickedFile, count: i32) ---
	// R_TRUE makes what is drawn depend only on what the test does:
	// every frame advances the animations by 1/60 s (Rset_animation_time_step),
	// and the carets of the text fields stay lit instead of blinking. R_FALSE
	// gives the clock back (time step 0).
	Rtest_set_deterministic :: proc(enabled: i32) ---
	Rtest_is_deterministic :: proc() -> i32 ---
	// Returns: What the goldens of this run are made with, "OS-backend":
	// "macos-metal", "linux-vulkan", "windows-vulkan", "web-webgl"... and
	// "null" with the null backend.
	Rtest_get_platform_tag :: proc() -> cstring ---
	// Finds a widget by name (Rwidget_set_name, Name="" in XML). The
	// path may have several names separated by '/': "settings/ok" is the
	// widget named "ok" inside the one named "settings". In a window, the
	// open popups (menus, dialogs) are searched first, top first, then the
	// content.
	// scope A window or a widget.
	// Returns: R_INVALID_ID when nothing has that name (no error is reported).
	Rtest_find :: proc(scope: Rid, path: cstring) -> Rid ---
	// Moves the mouse to the middle of the widget, presses and releases the left button.
	Rtest_click :: proc(widget: Rid) -> i32 ---
	// Two clicks in a row on the middle of the widget, seen as a double click.
	Rtest_double_click :: proc(widget: Rid) -> i32 ---
	// A click of the right button on the middle of the widget (a context menu).
	Rtest_right_click :: proc(widget: Rid) -> i32 ---
	// Clicks at a position of a widget or a window.
	// clicks 1 for a click, 2 for a double click, 3 for a triple click.
	// modifiers RModifier held during the clicks (R_MOD_SHIFT...).
	Rtest_click_at :: proc(target: Rid, x: f32, y: f32, button: RMouseButton, clicks: i32, modifiers: u32) -> i32 ---
	// Moves the mouse over the middle of the widget, without clicking.
	Rtest_hover :: proc(widget: Rid) -> i32 ---
	// Moves the mouse to a position of a widget or a window.
	Rtest_hover_at :: proc(target: Rid, x: f32, y: f32) -> i32 ---
	// Presses the left button on the middle of the widget, moves by
	// (dx, dy) in several steps, and releases: a slider, a splitter, a
	// window dragged by a widget.
	Rtest_drag :: proc(widget: Rid, dx: f32, dy: f32) -> i32 ---
	// Drags from the middle of one widget to the middle of another (drag and drop).
	Rtest_drag_to :: proc(from: Rid, to: Rid) -> i32 ---
	// Turns the mouse wheel over the middle of the widget: `wheel_y`
	// notches (positive: toward the top, as a wheel turned away from the
	// user), `wheel_x` notches sideways.
	Rtest_scroll :: proc(widget: Rid, wheel_x: f32, wheel_y: f32) -> i32 ---
	// Presses and releases a key, with RModifier held. A widget
	// target gets the keyboard focus first; a window sends the key to what
	// has the focus.
	Rtest_key :: proc(target: Rid, key: RKey, modifiers: u32) -> i32 ---
	// Types UTF-8 text as a keyboard would. A widget target gets the
	// keyboard focus first. '\n', '\t' and '\b' are sent as the keys Enter,
	// Tab and Backspace.
	Rtest_type_text :: proc(target: Rid, text: cstring) -> i32 ---
	// A finger touches the middle of the widget and leaves at once.
	Rtest_tap :: proc(widget: Rid) -> i32 ---
	// A finger put on the middle of the widget moves by (dx, dy) in
	// `seconds`, and leaves: a fast swipe flicks a list, which then coasts
	// (Rtest_settle lets it stop).
	Rtest_swipe :: proc(widget: Rid, dx: f32, dy: f32, seconds: f64) -> i32 ---
	// Produces `frames` frames (Rdraw_frame). Returns: R_FALSE once the application quits.
	Rtest_pump :: proc(frames: i32) -> i32 ---
	// Returns: R_TRUE when nothing waits for a frame: no posted task, no layout
	// to do, nothing to paint, no animation, no layout spring, no inertia.
	// Windows that redraw all the time (Rwindow_set_continuous_redraw, a
	// realtime viewport) and the timers are not counted.
	Rtest_is_idle :: proc() -> i32 ---
	// Produces frames until Rtest_is_idle, each advancing the
	// animations by the time step (1/60 s unless Rset_animation_time_step
	// gives another), for at most `max_seconds` of that time.
	// Returns: R_FALSE when it was still moving at the end.
	Rtest_settle :: proc(max_seconds: f64) -> i32 ---
	// Returns: What the user reads in the widget: the text of a field, else
	// its label (as screen readers say it: the text of a Label, a Button, a
	// Checkbox...). "" when it has none. Valid until the next call.
	Rtest_get_text :: proc(widget: Rid) -> cstring ---
	// Returns: R_TRUE when the widget can be seen: in a window, Visible with
	// all that holds it, on the page a Switcher or a TabView shows, and laid
	// out with a size.
	Rtest_is_shown :: proc(widget: Rid) -> i32 ---
	// Returns: R_TRUE for a checked Checkbox, Switch, RadioButton, toggle Button or menu item.
	Rtest_is_checked :: proc(widget: Rid) -> i32 ---
	// Captures a window, or the rectangle of one widget, and compares
	// it with the PNG file `golden_path` (see the top of this file).
	// target A window or a widget.
	// options NULL for the defaults.
	// result Receives the details; may be NULL.
	// Returns: R_TRUE when the capture matches (or the golden was written
	// because RAVEN_UPDATE_GOLDENS=1).
	Rtest_expect_golden :: proc(target: Rid, golden_path: cstring, options: ^RGoldenOptions, result: ^RGoldenResult) -> i32 ---
	// Compares two RGBA8 images of the same size as Rtest_expect_golden
	// does, without files.
	// diff_rgba When not NULL, receives width * height * 4 bytes: the
	// expected image faded, with the different pixels in red.
	// Returns: The number of pixels farther apart than `channel_tolerance`;
	// -1 when an image is missing.
	Rtest_compare_pixels :: proc(actual: [^]u8, expected: [^]u8, width: i32, height: i32, channel_tolerance: i32, max_channel_difference: ^i32, diff_rgba: [^]u8) -> i32 ---
	// The widgets as JSON, what a test, a script or an agent reads of
	// the interface. Each widget gives its "type" (its XML tag), "name" and
	// "path" (for Rtest_find) when it has a name, its accessibility "role",
	// its "text" (Rtest_get_text), "label" and "value" when they differ,
	// "bounds" [x, y, width, height] in logical units of its window,
	// "shown", "enabled", and when they apply "focused", "checked",
	// "selected", "expanded", "number"/"minimum"/"maximum", the rows of a
	// list ("items") and its "children", in the order of the layout.
	// target A window or a widget; R_INVALID_ID for every window of
	// the application: {"raven": version, "backend": tag, "windows": [...]},
	// each with its "title", "size", "scale", "content", "popups".
	// Returns: The JSON text, valid until the next call. "" on error.
	Rtest_dump_tree :: proc(target: Rid) -> cstring ---
	// Rtest_dump_tree written to a file ("-": the standard output).
	Rtest_save_tree :: proc(target: Rid, path: cstring) -> i32 ---
	// Captures a window, or the rectangle of a widget, into a PNG
	// file. The null backend draws nothing: its picture is the clear color.
	Rtest_save_png :: proc(target: Rid, path: cstring) -> i32 ---
	// Runs a script of steps, one per line, as RAVEN_SCRIPT does (see
	// "Driving any application headlessly" in README.md):
	//
	//     click form/user
	//     type "ada"
	//     key Enter
	//     expect status text "Welcome, ada"
	//     snap after-login
	//
	// The frames are produced by the function (Rdraw_frame) until the script
	// ends. A widget is named by its name or path, by "text:Sign in" (the
	// first one shown that reads it), or a point of the window by "@x,y".
	// script The text of the script.
	// source_name The name its messages give it ("login.script"); may be NULL.
	// Returns: R_TRUE when every step was done and every check was true;
	// R_FALSE otherwise, with the reason in Rtest_get_script_error.
	Rtest_run_script :: proc(script: cstring, source_name: cstring) -> i32 ---
	// Returns: Why the last Rtest_run_script failed ("login.script:4: expect: ..."), "" when it passed.
	Rtest_get_script_error :: proc() -> cstring ---
}

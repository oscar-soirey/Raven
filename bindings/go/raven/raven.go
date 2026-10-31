// Package raven is Raven for Go.
//
// Generated from the C headers by tools/bindgen (Raven 0.1): do not edit.
//
// Each C function whose parameters Go can express directly has a Go function:
// Rwindow_set_title is WindowSetTitle. The enumerations keep their C names
// (R_WIDGET_BUTTON). Texts are strings; a parameter the C function writes
// through is a result; the size that follows a text or bytes is not passed.
// An empty string is passed as an empty text, never as NULL: where the C API
// gives NULL a meaning there is a function of its own (XmlNodeRemoveAttribute).
//
// The functions that take a pointer to a structure, a void* or a C callback
// are left to cgo (they are listed at the end of this file). Three callbacks
// are bridged, which is what an interface described in XML needs:
// ActionBind, ErrorSetCallback and TimerStart.
//
//	raven.Init(raven.R_BACKEND_VULKAN)
//	raven.WindowCreateFromXml(`<Window Title="Hi"><Button Text="Quit" OnClicked="quit"/></Window>`)
//	raven.ActionBind("quit", func(sender raven.ID, action string) { raven.Quit(0) })
//	for raven.WaitFrame(-1.0) != 0 {
//	}
//	raven.Shutdown()
//
// Raven belongs to the thread that started it: call runtime.LockOSThread()
// at the top of main.
//
// To build: CGO_CFLAGS="-I<raven>/include" CGO_LDFLAGS="-L<folder of the library>".
package raven

/*
#cgo LDFLAGS: -lraven
#include <stdlib.h>
#include <string.h>
#include <Raven.h>

static int raven_go_init_headless(void) {
	RInitDesc desc;
	memset(&desc, 0, sizeof(desc));
	desc.backend = R_BACKEND_NULL;
	desc.headless = 1;
	return Rinit_ex(&desc);
}

static const char* raven_go_file_dialog(int save, Rid parent, const char* title, const char* file_name) {
	RFileDialogDesc desc;
	memset(&desc, 0, sizeof(desc));
	desc.parent = parent;
	desc.title = title[0] ? title : NULL;
	desc.file_name = file_name[0] ? file_name : NULL;
	return save ? Rfile_dialog_save(&desc) : Rfile_dialog_open(&desc);
}
*/
import "C"

import "unsafe"

// ID is a handle: a window, a widget, a font...
type ID uint32

// R_INVALID_ID is the handle of nothing.
const R_INVALID_ID ID = 0xFFFFFFFF

// InitHeadless starts Raven without a window system and without a GPU (tests, tools).
func InitHeadless() bool {
	return C.raven_go_init_headless() != 0
}

// XmlDocumentParse reads a text into a document. When the text has a mistake
// the document is R_INVALID_ID and the mistake is described by the other results.
func XmlDocumentParse(xml string) (document ID, line int, column int, message string) {
	var failure C.RXmlError
	xml_ := C.CString(xml)
	defer C.free(unsafe.Pointer(xml_))
	document = ID(C.Rxml_document_parse(xml_, C.size_t(len(xml)), &failure))
	return document, int(failure.line), int(failure.column), C.GoString(&failure.message[0])
}

// FileDialogOpen asks for a file to open; "" when the user cancelled.
func FileDialogOpen(parent ID, title string) string {
	title_ := C.CString(title)
	defer C.free(unsafe.Pointer(title_))
	empty := C.CString("")
	defer C.free(unsafe.Pointer(empty))
	return C.GoString(C.raven_go_file_dialog(0, C.Rid(parent), title_, empty))
}

// FileDialogSave asks where to save a file; "" when the user cancelled.
func FileDialogSave(parent ID, title string, fileName string) string {
	title_ := C.CString(title)
	defer C.free(unsafe.Pointer(title_))
	name_ := C.CString(fileName)
	defer C.free(unsafe.Pointer(name_))
	return C.GoString(C.raven_go_file_dialog(1, C.Rid(parent), title_, name_))
}

// FileRead reads a whole file (paths are UTF-8 on every platform); nil when it cannot be read.
func FileRead(path string) []byte {
	path_ := C.CString(path)
	defer C.free(unsafe.Pointer(path_))
	var size C.size_t
	data := C.Rfile_read(path_, &size)
	if data == nil {
		return nil
	}
	defer C.Rmemory_free(unsafe.Pointer(data))
	return C.GoBytes(unsafe.Pointer(data), C.int(size))
}

// XmlNodeRemoveAttribute removes an attribute (Rxml_node_set_attribute with a NULL value).
func XmlNodeRemoveAttribute(node ID, name string) {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	C.Rxml_node_set_attribute(C.Rid(node), name_, nil)
}

type RBackend int32

const (
	R_BACKEND_VULKAN RBackend = 1
	R_BACKEND_NULL   RBackend = 2
	R_BACKEND_CUSTOM RBackend = 3
)

type RError int32

const (
	R_NO_ERROR                  RError = 0x0070
	R_ERROR_INVALID_ID          RError = 0x0071
	R_ERROR_INVALID_ENUM        RError = 0x0072
	R_ERROR_INVALID_VALUE       RError = 0x0073
	R_ERROR_INVALID_OPERATION   RError = 0x0074
	R_ERROR_BACKEND             RError = 0x0075
	R_ERROR_PLATFORM            RError = 0x0076
	R_ERROR_OUT_OF_MEMORY       RError = 0x0077
	R_ERROR_INVALID_FILE_FORMAT RError = 0x0078
	R_ERROR_FILE_NOT_FOUND      RError = 0x0079
	R_ERROR_XML                 RError = 0x007A
	R_ERROR_WRONG_THREAD        RError = 0x007B
)

type RSeverity int32

const (
	R_SEVERITY_WEAK_WARNING RSeverity = 0x0080
	R_SEVERITY_WARNING      RSeverity = 0x0081
	R_SEVERITY_ERROR        RSeverity = 0x0082
	R_SEVERITY_FATAL        RSeverity = 0x0083
)

type RWindowFlags int32

const (
	R_WINDOW_DEFAULT       RWindowFlags = 0
	R_WINDOW_NO_RESIZE     RWindowFlags = 1
	R_WINDOW_HIDDEN        RWindowFlags = 2
	R_WINDOW_BORDERLESS    RWindowFlags = 4
	R_WINDOW_MAXIMIZED     RWindowFlags = 8
	R_WINDOW_CUSTOM_HEADER RWindowFlags = 0x0010
	R_WINDOW_TRANSPARENT   RWindowFlags = 0x0020
)

// RBackdrop: What is seen behind a window created with R_WINDOW_TRANSPARENT. Windows 11 every value Windows
// 10 TRANSPARENT, BLUR, ACRYLIC (MICA gives ACRYLIC) macOS TRANSPARENT; other backdrops fall back to
// transparency X11 TRANSPARENT with any compositing manager; BLUR where the compositor blurs on request
// (KWin; picom and others with their own settings); ACRYLIC and MICA give BLUR no compositor nothing: the
// window stays opaque
type RBackdrop int32

const (
	R_BACKDROP_NONE        RBackdrop = 0x0400
	R_BACKDROP_TRANSPARENT RBackdrop = 0x0401
	R_BACKDROP_BLUR        RBackdrop = 0x0402
	R_BACKDROP_ACRYLIC     RBackdrop = 0x0403
	R_BACKDROP_MICA        RBackdrop = 0x0404
)

type RWindowSystem int32

const (
	R_WINDOW_SYSTEM_NONE    RWindowSystem = 0x0090
	R_WINDOW_SYSTEM_WIN32   RWindowSystem = 0x0091
	R_WINDOW_SYSTEM_X11     RWindowSystem = 0x0092
	R_WINDOW_SYSTEM_WAYLAND RWindowSystem = 0x0093
	R_WINDOW_SYSTEM_COCOA   RWindowSystem = 0x0094
)

// RWindowEdge: Edges and corners of a window, for Rwindow_begin_resize.
type RWindowEdge int32

const (
	R_EDGE_LEFT         RWindowEdge = 0x00D0
	R_EDGE_RIGHT        RWindowEdge = 0x00D1
	R_EDGE_TOP          RWindowEdge = 0x00D2
	R_EDGE_BOTTOM       RWindowEdge = 0x00D3
	R_EDGE_TOP_LEFT     RWindowEdge = 0x00D4
	R_EDGE_TOP_RIGHT    RWindowEdge = 0x00D5
	R_EDGE_BOTTOM_LEFT  RWindowEdge = 0x00D6
	R_EDGE_BOTTOM_RIGHT RWindowEdge = 0x00D7
)

type RMouseButton int32

const (
	R_MOUSE_LEFT   RMouseButton = 0
	R_MOUSE_RIGHT  RMouseButton = 1
	R_MOUSE_MIDDLE RMouseButton = 2
	R_MOUSE_X1     RMouseButton = 3
	R_MOUSE_X2     RMouseButton = 4
)

type RModifier int32

const (
	R_MOD_SHIFT RModifier = 1
	R_MOD_CTRL  RModifier = 2
	R_MOD_ALT   RModifier = 4
	R_MOD_SUPER RModifier = 8
)

type RCursor int32

const (
	R_CURSOR_ARROW       RCursor = 0x00B0
	R_CURSOR_IBEAM       RCursor = 0x00B1
	R_CURSOR_HAND        RCursor = 0x00B2
	R_CURSOR_CROSSHAIR   RCursor = 0x00B3
	R_CURSOR_RESIZE_H    RCursor = 0x00B4
	R_CURSOR_RESIZE_V    RCursor = 0x00B5
	R_CURSOR_RESIZE_NWSE RCursor = 0x00B6
	R_CURSOR_RESIZE_NESW RCursor = 0x00B7
	R_CURSOR_MOVE        RCursor = 0x00B8
	R_CURSOR_NOT_ALLOWED RCursor = 0x00B9
	R_CURSOR_NONE        RCursor = 0x00BA
)

// RKey: Keyboard keys. Printable keys use their upper-case ASCII code.
type RKey int32

const (
	R_KEY_UNKNOWN       RKey = 0
	R_KEY_SPACE         RKey = 0x0020
	R_KEY_APOSTROPHE    RKey = 0x0027
	R_KEY_COMMA         RKey = 0x002C
	R_KEY_MINUS         RKey = 0x002D
	R_KEY_PERIOD        RKey = 0x002E
	R_KEY_SLASH         RKey = 0x002F
	R_KEY_0             RKey = 0x0030
	R_KEY_1             RKey = 0x0031
	R_KEY_2             RKey = 0x0032
	R_KEY_3             RKey = 0x0033
	R_KEY_4             RKey = 0x0034
	R_KEY_5             RKey = 0x0035
	R_KEY_6             RKey = 0x0036
	R_KEY_7             RKey = 0x0037
	R_KEY_8             RKey = 0x0038
	R_KEY_9             RKey = 0x0039
	R_KEY_SEMICOLON     RKey = 0x003B
	R_KEY_EQUAL         RKey = 0x003D
	R_KEY_A             RKey = 0x0041
	R_KEY_B             RKey = 0x0042
	R_KEY_C             RKey = 0x0043
	R_KEY_D             RKey = 0x0044
	R_KEY_E             RKey = 0x0045
	R_KEY_F             RKey = 0x0046
	R_KEY_G             RKey = 0x0047
	R_KEY_H             RKey = 0x0048
	R_KEY_I             RKey = 0x0049
	R_KEY_J             RKey = 0x004A
	R_KEY_K             RKey = 0x004B
	R_KEY_L             RKey = 0x004C
	R_KEY_M             RKey = 0x004D
	R_KEY_N             RKey = 0x004E
	R_KEY_O             RKey = 0x004F
	R_KEY_P             RKey = 0x0050
	R_KEY_Q             RKey = 0x0051
	R_KEY_R             RKey = 0x0052
	R_KEY_S             RKey = 0x0053
	R_KEY_T             RKey = 0x0054
	R_KEY_U             RKey = 0x0055
	R_KEY_V             RKey = 0x0056
	R_KEY_W             RKey = 0x0057
	R_KEY_X             RKey = 0x0058
	R_KEY_Y             RKey = 0x0059
	R_KEY_Z             RKey = 0x005A
	R_KEY_LEFT_BRACKET  RKey = 0x005B
	R_KEY_BACKSLASH     RKey = 0x005C
	R_KEY_RIGHT_BRACKET RKey = 0x005D
	R_KEY_GRAVE         RKey = 0x0060
	R_KEY_ESCAPE        RKey = 0x0100
	R_KEY_ENTER         RKey = 0x0101
	R_KEY_TAB           RKey = 0x0102
	R_KEY_BACKSPACE     RKey = 0x0103
	R_KEY_INSERT        RKey = 0x0104
	R_KEY_DELETE        RKey = 0x0105
	R_KEY_RIGHT         RKey = 0x0106
	R_KEY_LEFT          RKey = 0x0107
	R_KEY_DOWN          RKey = 0x0108
	R_KEY_UP            RKey = 0x0109
	R_KEY_PAGE_UP       RKey = 0x010A
	R_KEY_PAGE_DOWN     RKey = 0x010B
	R_KEY_HOME          RKey = 0x010C
	R_KEY_END           RKey = 0x010D
	R_KEY_CAPS_LOCK     RKey = 0x010E
	R_KEY_PRINT_SCREEN  RKey = 0x010F
	R_KEY_PAUSE         RKey = 0x0110
	R_KEY_F1            RKey = 0x0111
	R_KEY_F2            RKey = 0x0112
	R_KEY_F3            RKey = 0x0113
	R_KEY_F4            RKey = 0x0114
	R_KEY_F5            RKey = 0x0115
	R_KEY_F6            RKey = 0x0116
	R_KEY_F7            RKey = 0x0117
	R_KEY_F8            RKey = 0x0118
	R_KEY_F9            RKey = 0x0119
	R_KEY_F10           RKey = 0x011A
	R_KEY_F11           RKey = 0x011B
	R_KEY_F12           RKey = 0x011C
	R_KEY_KP_0          RKey = 0x011D
	R_KEY_KP_1          RKey = 0x011E
	R_KEY_KP_2          RKey = 0x011F
	R_KEY_KP_3          RKey = 0x0120
	R_KEY_KP_4          RKey = 0x0121
	R_KEY_KP_5          RKey = 0x0122
	R_KEY_KP_6          RKey = 0x0123
	R_KEY_KP_7          RKey = 0x0124
	R_KEY_KP_8          RKey = 0x0125
	R_KEY_KP_9          RKey = 0x0126
	R_KEY_KP_DECIMAL    RKey = 0x0127
	R_KEY_KP_DIVIDE     RKey = 0x0128
	R_KEY_KP_MULTIPLY   RKey = 0x0129
	R_KEY_KP_SUBTRACT   RKey = 0x012A
	R_KEY_KP_ADD        RKey = 0x012B
	R_KEY_KP_ENTER      RKey = 0x012C
	R_KEY_LEFT_SHIFT    RKey = 0x012D
	R_KEY_LEFT_CTRL     RKey = 0x012E
	R_KEY_LEFT_ALT      RKey = 0x012F
	R_KEY_LEFT_SUPER    RKey = 0x0130
	R_KEY_RIGHT_SHIFT   RKey = 0x0131
	R_KEY_RIGHT_CTRL    RKey = 0x0132
	R_KEY_RIGHT_ALT     RKey = 0x0133
	R_KEY_RIGHT_SUPER   RKey = 0x0134
	R_KEY_MENU          RKey = 0x0135
	R_KEY_COUNT         RKey = 0x0136
)

type REventType int32

const (
	R_EVENT_NONE         REventType = 0x00C0
	R_EVENT_MOUSE_MOVE   REventType = 0x00C1
	R_EVENT_MOUSE_DOWN   REventType = 0x00C2
	R_EVENT_MOUSE_UP     REventType = 0x00C3
	R_EVENT_MOUSE_WHEEL  REventType = 0x00C4
	R_EVENT_MOUSE_ENTER  REventType = 0x00C5
	R_EVENT_MOUSE_LEAVE  REventType = 0x00C6
	R_EVENT_KEY_DOWN     REventType = 0x00C7
	R_EVENT_KEY_UP       REventType = 0x00C8
	R_EVENT_TEXT         REventType = 0x00C9
	R_EVENT_FOCUS_GAINED REventType = 0x00CA
	R_EVENT_FOCUS_LOST   REventType = 0x00CB
)

// RDropPhase: Moments of a drag coming from another application (files from the file manager, text...).
type RDropPhase int32

const (
	R_DROP_ENTER   RDropPhase = 0x00E0
	R_DROP_MOVE    RDropPhase = 0x00E1
	R_DROP_LEAVE   RDropPhase = 0x00E2
	R_DROP_DROPPED RDropPhase = 0x00E3
)

// RLook: The looks. Each one comes dark and light. Raven's looks are the default; the ones of the platforms
// imitate them closely (their shapes, colors, sizes, states and motion), without using their widgets. The
// looks of the platforms are opaque, except macOS Glass (macOS 26), whose bars, menus and floating surfaces
// are glass as on the system; the glass materials themselves (Rtheme_set_material) belong to the glass looks
// of Raven.
type RLook int32

const (
	R_LOOK_RAVEN_DARK                RLook = 0x00E0
	R_LOOK_RAVEN_LIGHT               RLook = 0x00E1
	R_LOOK_RAVEN_GLASS_DARK          RLook = 0x00E2
	R_LOOK_RAVEN_GLASS_LIGHT         RLook = 0x00E3
	R_LOOK_RAVEN_HIGH_CONTRAST_DARK  RLook = 0x00E4
	R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT RLook = 0x00E5
	R_LOOK_WINDOWS_DARK              RLook = 0x00E6
	R_LOOK_WINDOWS_LIGHT             RLook = 0x00E7
	R_LOOK_MACOS_DARK                RLook = 0x00E8
	R_LOOK_MACOS_LIGHT               RLook = 0x00E9
	R_LOOK_GNOME_DARK                RLook = 0x00EA
	R_LOOK_GNOME_LIGHT               RLook = 0x00EB
	R_LOOK_NATIVE_DARK               RLook = 0x00EC
	R_LOOK_NATIVE_LIGHT              RLook = 0x00ED
	R_LOOK_MACOS_GLASS_DARK          RLook = 0x00EE
	R_LOOK_MACOS_GLASS_LIGHT         RLook = 0x00EF
)

// RTheme: @deprecated The built-in themes of Raven 0.2, now looks: R_THEME_DARK is R_LOOK_RAVEN_DARK and so
// on, and Rtheme_apply is Rlook_set.
type RTheme int32

const (
	R_THEME_DARK                RTheme = 0x00E0
	R_THEME_LIGHT               RTheme = 0x00E1
	R_THEME_GLASS_DARK          RTheme = 0x00E2
	R_THEME_GLASS_LIGHT         RTheme = 0x00E3
	R_THEME_HIGH_CONTRAST_DARK  RTheme = 0x00E4
	R_THEME_HIGH_CONTRAST_LIGHT RTheme = 0x00E5
)

// RMaterial: What surfaces are made of, from the clearest to the most opaque. A theme has one material for
// everything (Rtheme_set_material); any widget can be given its own (Rwidget_set_material, Material="..." in
// XML). The three glasses share the same anatomy (a tint over what is behind, a rim that bends it and catches
// the light, a shadow) and differ by how much they let through. Within one material, surfaces are also told
// apart by their height above the window: bars lie flat, panels float a little, controls a little more, menus
// and dialogs highest; each level has a brighter rim and a deeper shadow than the one below.
type RMaterial int32

const (
	R_MATERIAL_SOLID         RMaterial = 0x02E0
	R_MATERIAL_GLASS         RMaterial = 0x02E1
	R_MATERIAL_FROSTED_GLASS RMaterial = 0x02E2
	R_MATERIAL_TINTED_GLASS  RMaterial = 0x02E3
	R_MATERIAL_DEFAULT       RMaterial = 0x02E4
)

// RColorRole: Color roles. The comment lists the widgets reading each role.
type RColorRole int32

const (
	R_COLOR_WINDOW           RColorRole = 0x0100
	R_COLOR_PANEL            RColorRole = 0x0101
	R_COLOR_BORDER           RColorRole = 0x0102
	R_COLOR_TEXT             RColorRole = 0x0103
	R_COLOR_TEXT_DISABLED    RColorRole = 0x0104
	R_COLOR_TEXT_PLACEHOLDER RColorRole = 0x0105
	R_COLOR_ACCENT           RColorRole = 0x0106
	R_COLOR_ACCENT_HOVERED   RColorRole = 0x0107
	R_COLOR_ACCENT_PRESSED   RColorRole = 0x0108
	R_COLOR_ACCENT_TEXT      RColorRole = 0x0109
	R_COLOR_CONTROL          RColorRole = 0x010A
	R_COLOR_CONTROL_HOVERED  RColorRole = 0x010B
	R_COLOR_CONTROL_PRESSED  RColorRole = 0x010C
	R_COLOR_CONTROL_BORDER   RColorRole = 0x010D
	R_COLOR_INPUT            RColorRole = 0x010E
	R_COLOR_FOCUS            RColorRole = 0x010F
	R_COLOR_SELECTION        RColorRole = 0x0110
	R_COLOR_ROW_HOVERED      RColorRole = 0x0111
	R_COLOR_TRACK            RColorRole = 0x0112
	R_COLOR_HANDLE           RColorRole = 0x0113
	R_COLOR_HANDLE_HOVERED   RColorRole = 0x0114
	R_COLOR_SEPARATOR        RColorRole = 0x0115
	R_COLOR_HEADER           RColorRole = 0x0116
	R_COLOR_HEADER_HOVERED   RColorRole = 0x0117
	R_COLOR_TAB_ACTIVE       RColorRole = 0x0118
	R_COLOR_POPUP            RColorRole = 0x0119
	R_COLOR_POPUP_BORDER     RColorRole = 0x011A
	R_COLOR_SHADOW           RColorRole = 0x011B
	R_COLOR_TOOLTIP          RColorRole = 0x011C
	R_COLOR_TOOLTIP_TEXT     RColorRole = 0x011D
	R_COLOR_SWITCH           RColorRole = 0x011E
	R_COLOR_SUCCESS          RColorRole = 0x011F
	R_COLOR_WARNING          RColorRole = 0x0120
	R_COLOR_DANGER           RColorRole = 0x0121
	R_COLOR_LINK             RColorRole = 0x0122
	R_COLOR_CODE             RColorRole = 0x0123
)

// RMetric: Sizes, in logical units.
type RMetric int32

const (
	R_METRIC_FONT_SIZE          RMetric = 0x0140
	R_METRIC_CORNER_RADIUS      RMetric = 0x0141
	R_METRIC_BORDER_WIDTH       RMetric = 0x0142
	R_METRIC_CONTROL_HEIGHT     RMetric = 0x0143
	R_METRIC_PADDING_X          RMetric = 0x0144
	R_METRIC_PADDING_Y          RMetric = 0x0145
	R_METRIC_SPACING            RMetric = 0x0146
	R_METRIC_SCROLLBAR_SIZE     RMetric = 0x0147
	R_METRIC_ROW_HEIGHT         RMetric = 0x0148
	R_METRIC_INDENT             RMetric = 0x0149
	R_METRIC_SPLITTER_SIZE      RMetric = 0x014A
	R_METRIC_CHECK_SIZE         RMetric = 0x014B
	R_METRIC_SLIDER_HANDLE_SIZE RMetric = 0x014C
	R_METRIC_POPUP_RADIUS       RMetric = 0x014D
	R_METRIC_GLASS_FROST        RMetric = 0x014E
	R_METRIC_GLASS_REFRACTION   RMetric = 0x014F
	R_METRIC_GLASS_SATURATION   RMetric = 0x0150
	R_METRIC_GLASS_RIM          RMetric = 0x0151
	R_METRIC_GLASS_SHADOW       RMetric = 0x0152
	R_METRIC_BLUR_RADIUS        RMetric = 0x0153
)

// RFontStyle: The styles of a font (Rfont_get_variant).
type RFontStyle int32

const (
	R_FONT_REGULAR     RFontStyle = 0x0420
	R_FONT_BOLD        RFontStyle = 0x0421
	R_FONT_ITALIC      RFontStyle = 0x0422
	R_FONT_BOLD_ITALIC RFontStyle = 0x0423
)

type RFilterType int32

const (
	R_FILTER_LINEAR  RFilterType = 0x02C0
	R_FILTER_NEAREST RFilterType = 0x02C1
	R_FILTER_MIPMAP  RFilterType = 0x02C2
)

type RWidgetType int32

const (
	R_WIDGET_VBOX            RWidgetType = 0x0200
	R_WIDGET_HBOX            RWidgetType = 0x0201
	R_WIDGET_GRID            RWidgetType = 0x0202
	R_WIDGET_UNIFORM_GRID    RWidgetType = 0x0203
	R_WIDGET_OVERLAY         RWidgetType = 0x0204
	R_WIDGET_BORDER          RWidgetType = 0x0205
	R_WIDGET_SIZE_BOX        RWidgetType = 0x0206
	R_WIDGET_SPACER          RWidgetType = 0x0207
	R_WIDGET_SCROLL_BOX      RWidgetType = 0x0208
	R_WIDGET_SPLITTER        RWidgetType = 0x0209
	R_WIDGET_WRAP_BOX        RWidgetType = 0x020A
	R_WIDGET_CANVAS          RWidgetType = 0x020B
	R_WIDGET_SWITCHER        RWidgetType = 0x020C
	R_WIDGET_TAB_VIEW        RWidgetType = 0x020D
	R_WIDGET_EXPANDER        RWidgetType = 0x020E
	R_WIDGET_LABEL           RWidgetType = 0x020F
	R_WIDGET_BUTTON          RWidgetType = 0x0210
	R_WIDGET_CHECKBOX        RWidgetType = 0x0211
	R_WIDGET_SLIDER          RWidgetType = 0x0212
	R_WIDGET_PROGRESSBAR     RWidgetType = 0x0213
	R_WIDGET_IMAGE           RWidgetType = 0x0214
	R_WIDGET_TEXT_INPUT      RWidgetType = 0x0215
	R_WIDGET_SPIN_BOX        RWidgetType = 0x0216
	R_WIDGET_COMBO_BOX       RWidgetType = 0x0217
	R_WIDGET_SEPARATOR       RWidgetType = 0x0218
	R_WIDGET_LIST_VIEW       RWidgetType = 0x0219
	R_WIDGET_TREE_VIEW       RWidgetType = 0x021A
	R_WIDGET_MENU_BAR        RWidgetType = 0x021B
	R_WIDGET_MENU            RWidgetType = 0x021C
	R_WIDGET_MENU_ITEM       RWidgetType = 0x021D
	R_WIDGET_VIEWPORT        RWidgetType = 0x021E
	R_WIDGET_CUSTOM          RWidgetType = 0x021F
	R_WIDGET_TEXT_EDITOR     RWidgetType = 0x0220
	R_WIDGET_HEADER_BAR      RWidgetType = 0x0221
	R_WIDGET_DOCK_AREA       RWidgetType = 0x0222
	R_WIDGET_ISLAND          RWidgetType = 0x0223
	R_WIDGET_FILE_BROWSER    RWidgetType = 0x0224
	R_WIDGET_BACKGROUND_BLUR RWidgetType = 0x0225
	R_WIDGET_COLOR_PICKER    RWidgetType = 0x0226
	R_WIDGET_TIMELINE        RWidgetType = 0x0227
	R_WIDGET_NODE_GRAPH      RWidgetType = 0x0228
	R_WIDGET_TILE_VIEW       RWidgetType = 0x0229
	R_WIDGET_BLOCK_EDITOR    RWidgetType = 0x022A
)

type RHAlign int32

const (
	R_HALIGN_FILL   RHAlign = 0x0240
	R_HALIGN_LEFT   RHAlign = 0x0241
	R_HALIGN_CENTER RHAlign = 0x0242
	R_HALIGN_RIGHT  RHAlign = 0x0243
)

type RVAlign int32

const (
	R_VALIGN_FILL   RVAlign = 0x0250
	R_VALIGN_TOP    RVAlign = 0x0251
	R_VALIGN_CENTER RVAlign = 0x0252
	R_VALIGN_BOTTOM RVAlign = 0x0253
)

type RVisibility int32

const (
	R_VISIBLE   RVisibility = 0x0260
	R_HIDDEN    RVisibility = 0x0261
	R_COLLAPSED RVisibility = 0x0262
)

// RTransition: How a widget arrives. A transition animates what the widget and its children draw; the layout
// is not affected, the widget is in place from the first frame for its neighbours and for the mouse.
type RTransition int32

const (
	R_TRANSITION_NONE        RTransition = 0x0300
	R_TRANSITION_FADE        RTransition = 0x0301
	R_TRANSITION_POP         RTransition = 0x0302
	R_TRANSITION_ZOOM        RTransition = 0x0303
	R_TRANSITION_FLY_UP      RTransition = 0x0304
	R_TRANSITION_FLY_DOWN    RTransition = 0x0305
	R_TRANSITION_FLY_LEFT    RTransition = 0x0306
	R_TRANSITION_FLY_RIGHT   RTransition = 0x0307
	R_TRANSITION_DROP        RTransition = 0x0308
	R_TRANSITION_SLIDE_LEFT  RTransition = 0x0309
	R_TRANSITION_SLIDE_RIGHT RTransition = 0x030A
)

type ROrientation int32

const (
	R_HORIZONTAL ROrientation = 0x0270
	R_VERTICAL   ROrientation = 0x0271
)

// RBlurStyle: The looks of a R_WIDGET_BACKGROUND_BLUR. The first three are drawn by Raven from what the
// window shows behind the widget; the last one is the blur of the OS, which sees what is behind the window
// itself.
type RBlurStyle int32

const (
	R_BLUR_PLAIN   RBlurStyle = 0x0440
	R_BLUR_FROSTED RBlurStyle = 0x0441
	R_BLUR_GLASS   RBlurStyle = 0x0442
	R_BLUR_NATIVE  RBlurStyle = 0x0443
)

type RDirection int32

const (
	R_DIRECTION_INHERIT RDirection = 0
	R_DIRECTION_LTR     RDirection = 0x0720
	R_DIRECTION_RTL     RDirection = 0x0721
	R_DIRECTION_AUTO    RDirection = 0x0722
)

type RTextAlign int32

const (
	R_TEXT_ALIGN_LEFT   RTextAlign = 0x0280
	R_TEXT_ALIGN_CENTER RTextAlign = 0x0281
	R_TEXT_ALIGN_RIGHT  RTextAlign = 0x0282
)

type RButtonStyle int32

const (
	R_BUTTON_DEFAULT RButtonStyle = 0x0290
	R_BUTTON_PRIMARY RButtonStyle = 0x0291
	R_BUTTON_FLAT    RButtonStyle = 0x0292
)

type RCheckboxStyle int32

const (
	R_CHECKBOX_CHECK  RCheckboxStyle = 0x02A0
	R_CHECKBOX_RADIO  RCheckboxStyle = 0x02A1
	R_CHECKBOX_SWITCH RCheckboxStyle = 0x02A2
)

type RStretch int32

const (
	R_STRETCH_FILL  RStretch = 0x02B0
	R_STRETCH_FIT   RStretch = 0x02B1
	R_STRETCH_COVER RStretch = 0x02B2
	R_STRETCH_NONE  RStretch = 0x02B3
)

// RSyntax: Shorthands of Rtext_editor_set_language for two languages.
type RSyntax int32

const (
	R_SYNTAX_NONE RSyntax = 0x0340
	R_SYNTAX_XML  RSyntax = 0x0341
	R_SYNTAX_C    RSyntax = 0x0342
)

type RLanguageFlags int32

const (
	R_LANGUAGE_MARKUP          RLanguageFlags = 1
	R_LANGUAGE_STYLE_SHEET     RLanguageFlags = 2
	R_LANGUAGE_NESTED_COMMENTS RLanguageFlags = 4
	R_LANGUAGE_CHAR_LITERALS   RLanguageFlags = 8
	R_LANGUAGE_INDENT_ON_COLON RLanguageFlags = 0x0010
	R_LANGUAGE_IGNORE_CASE     RLanguageFlags = 0x0020
	R_LANGUAGE_EMBEDS          RLanguageFlags = 0x0040
	R_LANGUAGE_FORMATTED_TEXT  RLanguageFlags = 0x0080
)

// RToken: What a part of the text is, for the language of the editor (Rtext_editor_get_token).
type RToken int32

const (
	R_TOKEN_TEXT        RToken = 0x03A0
	R_TOKEN_COMMENT     RToken = 0x03A1
	R_TOKEN_STRING      RToken = 0x03A2
	R_TOKEN_NUMBER      RToken = 0x03A3
	R_TOKEN_KEYWORD     RToken = 0x03A4
	R_TOKEN_TYPE        RToken = 0x03A5
	R_TOKEN_FUNCTION    RToken = 0x03A6
	R_TOKEN_TAG         RToken = 0x03A7
	R_TOKEN_ATTRIBUTE   RToken = 0x03A8
	R_TOKEN_PUNCTUATION RToken = 0x03A9
	R_TOKEN_DIRECTIVE   RToken = 0x03AA
	R_TOKEN_HEADING     RToken = 0x03AB
	R_TOKEN_STRONG      RToken = 0x03AC
	R_TOKEN_EMPHASIS    RToken = 0x03AD
	R_TOKEN_LINK        RToken = 0x03AE
)

// RMarkerKind: What a marker says about the text it underlines.
type RMarkerKind int32

const (
	R_MARKER_ERROR   RMarkerKind = 0x0350
	R_MARKER_WARNING RMarkerKind = 0x0351
	R_MARKER_INFO    RMarkerKind = 0x0352
)

// RHeaderButtons: Buttons of a header bar.
type RHeaderButtons int32

const (
	R_HEADER_NO_BUTTONS  RHeaderButtons = 0
	R_HEADER_CLOSE       RHeaderButtons = 1
	R_HEADER_MINIMIZE    RHeaderButtons = 2
	R_HEADER_MAXIMIZE    RHeaderButtons = 4
	R_HEADER_ALL_BUTTONS RHeaderButtons = 7
)

// RDockSide: Where a panel goes in a dock area, relative to another panel or to the whole area.
type RDockSide int32

const (
	R_DOCK_LEFT   RDockSide = 0x0360
	R_DOCK_RIGHT  RDockSide = 0x0361
	R_DOCK_TOP    RDockSide = 0x0362
	R_DOCK_BOTTOM RDockSide = 0x0363
	R_DOCK_TAB    RDockSide = 0x0364
)

// RTimelineItem: What an item of a timeline is.
type RTimelineItem int32

const (
	R_TIMELINE_NONE   RTimelineItem = 0x0460
	R_TIMELINE_CLIP   RTimelineItem = 0x0461
	R_TIMELINE_KEY    RTimelineItem = 0x0462
	R_TIMELINE_MARKER RTimelineItem = 0x0463
)

type RBlockShape int32

const (
	R_BLOCK_STATEMENT RBlockShape = 0x0700
	R_BLOCK_HAT       RBlockShape = 0x0701
	R_BLOCK_CAP       RBlockShape = 0x0702
	R_BLOCK_C         RBlockShape = 0x0703
	R_BLOCK_REPORTER  RBlockShape = 0x0704
	R_BLOCK_BOOLEAN   RBlockShape = 0x0705
)

type RBlockPlace int32

const (
	R_BLOCK_NEXT  RBlockPlace = 0x0710
	R_BLOCK_BODY  RBlockPlace = 0x0711
	R_BLOCK_INPUT RBlockPlace = 0x0712
)

type RFileDialogFlags int32

const (
	R_FILE_DIALOG_MULTIPLE    RFileDialogFlags = 1
	R_FILE_DIALOG_RAVEN       RFileDialogFlags = 2
	R_FILE_DIALOG_SHOW_HIDDEN RFileDialogFlags = 4
)

type RFileBrowserMode int32

const (
	R_FILE_BROWSER_OPEN   RFileBrowserMode = 0x0380
	R_FILE_BROWSER_SAVE   RFileBrowserMode = 0x0381
	R_FILE_BROWSER_FOLDER RFileBrowserMode = 0x0382
)

type RMessageButtons int32

const (
	R_MESSAGE_OK            RMessageButtons = 0x0390
	R_MESSAGE_OK_CANCEL     RMessageButtons = 0x0391
	R_MESSAGE_YES_NO        RMessageButtons = 0x0392
	R_MESSAGE_YES_NO_CANCEL RMessageButtons = 0x0393
)

type RMessageResult int32

const (
	R_MESSAGE_RESULT_CANCEL RMessageResult = 0
	R_MESSAGE_RESULT_OK     RMessageResult = 1
	R_MESSAGE_RESULT_YES    RMessageResult = 2
	R_MESSAGE_RESULT_NO     RMessageResult = 3
)

// RMotion: How a spring moves. Animations in Raven are springs, as on iOS: a value is pulled toward its
// target and may run past it before settling, which is what makes a control feel alive rather than
// mechanical.
type RMotion int32

const (
	R_MOTION_SNAPPY RMotion = 0x0320
	R_MOTION_SMOOTH RMotion = 0x0321
	R_MOTION_BOUNCY RMotion = 0x0322
	R_MOTION_WOBBLY RMotion = 0x0323
)

// REffect: The effects that come with Raven (Reffect_get).
type REffect int32

const (
	R_EFFECT_AURORA    REffect = 0x0B40
	R_EFFECT_SHIMMER   REffect = 0x0B41
	R_EFFECT_NEON      REffect = 0x0B42
	R_EFFECT_RIPPLE    REffect = 0x0B43
	R_EFFECT_PLASMA    REffect = 0x0B44
	R_EFFECT_GRAIN     REffect = 0x0B45
	R_EFFECT_SPOTLIGHT REffect = 0x0B46
	R_EFFECT_MESH      REffect = 0x0B47
)

// RShaderFlags: Flags of Rshader_create.
type RShaderFlags int32

const (
	R_SHADER_BACKDROP RShaderFlags = 1
	R_SHADER_ANIMATED RShaderFlags = 2
)

// RXmlValueType: The kind of value an attribute takes.
type RXmlValueType int32

const (
	R_XML_TEXT    RXmlValueType = 0x03C0
	R_XML_BOOL    RXmlValueType = 0x03C1
	R_XML_NUMBER  RXmlValueType = 0x03C2
	R_XML_CHOICE  RXmlValueType = 0x03C3
	R_XML_COLOR   RXmlValueType = 0x03C4
	R_XML_ACTION  RXmlValueType = 0x03C5
	R_XML_FILE    RXmlValueType = 0x03C6
	R_XML_NUMBERS RXmlValueType = 0x03C7
	R_XML_LIST    RXmlValueType = 0x03C8
)

type RColorFilter int32

const (
	R_COLOR_FILTER_NONE                   RColorFilter = 0x0740
	R_COLOR_FILTER_PROTANOPIA             RColorFilter = 0x0741
	R_COLOR_FILTER_DEUTERANOPIA           RColorFilter = 0x0742
	R_COLOR_FILTER_TRITANOPIA             RColorFilter = 0x0743
	R_COLOR_FILTER_GRAYSCALE              RColorFilter = 0x0744
	R_COLOR_FILTER_INVERT                 RColorFilter = 0x0745
	R_COLOR_FILTER_SIMULATE_PROTANOPIA    RColorFilter = 0x0746
	R_COLOR_FILTER_SIMULATE_DEUTERANOPIA  RColorFilter = 0x0747
	R_COLOR_FILTER_SIMULATE_TRITANOPIA    RColorFilter = 0x0748
	R_COLOR_FILTER_SIMULATE_ACHROMATOPSIA RColorFilter = 0x0749
)

type RAccessibleRole int32

const (
	R_ROLE_AUTO          RAccessibleRole = 0
	R_ROLE_NONE          RAccessibleRole = 0x0760
	R_ROLE_WINDOW        RAccessibleRole = 0x0761
	R_ROLE_GROUP         RAccessibleRole = 0x0762
	R_ROLE_TEXT          RAccessibleRole = 0x0763
	R_ROLE_HEADING       RAccessibleRole = 0x0764
	R_ROLE_LINK          RAccessibleRole = 0x0765
	R_ROLE_BUTTON        RAccessibleRole = 0x0766
	R_ROLE_TOGGLE_BUTTON RAccessibleRole = 0x0767
	R_ROLE_CHECKBOX      RAccessibleRole = 0x0768
	R_ROLE_RADIO_BUTTON  RAccessibleRole = 0x0769
	R_ROLE_SLIDER        RAccessibleRole = 0x076A
	R_ROLE_PROGRESS_BAR  RAccessibleRole = 0x076B
	R_ROLE_SPIN_BUTTON   RAccessibleRole = 0x076C
	R_ROLE_TEXT_FIELD    RAccessibleRole = 0x076D
	R_ROLE_DOCUMENT      RAccessibleRole = 0x076E
	R_ROLE_COMBO_BOX     RAccessibleRole = 0x076F
	R_ROLE_LIST          RAccessibleRole = 0x0770
	R_ROLE_LIST_ITEM     RAccessibleRole = 0x0771
	R_ROLE_TREE          RAccessibleRole = 0x0772
	R_ROLE_TREE_ITEM     RAccessibleRole = 0x0773
	R_ROLE_TABLE         RAccessibleRole = 0x0774
	R_ROLE_MENU_BAR      RAccessibleRole = 0x0775
	R_ROLE_MENU          RAccessibleRole = 0x0776
	R_ROLE_MENU_ITEM     RAccessibleRole = 0x0777
	R_ROLE_TAB_LIST      RAccessibleRole = 0x0778
	R_ROLE_TAB           RAccessibleRole = 0x0779
	R_ROLE_TAB_PANEL     RAccessibleRole = 0x077A
	R_ROLE_TOOLBAR       RAccessibleRole = 0x077B
	R_ROLE_STATUS_BAR    RAccessibleRole = 0x077C
	R_ROLE_DIALOG        RAccessibleRole = 0x077D
	R_ROLE_ALERT         RAccessibleRole = 0x077E
	R_ROLE_IMAGE         RAccessibleRole = 0x077F
	R_ROLE_CANVAS        RAccessibleRole = 0x0780
	R_ROLE_SEPARATOR     RAccessibleRole = 0x0781
	R_ROLE_SCROLL_BAR    RAccessibleRole = 0x0782
	R_ROLE_PANE          RAccessibleRole = 0x0783
)

// Init: Starts the library with default settings on the given backend. The calling thread becomes the UI
// thread. Returns: R_TRUE on success. On failure, see Rerror_get_last.
func Init(backend RBackend) int {
	return int(C.Rinit(C.RBackend(backend)))
}

// Shutdown: Destroys every window, widget and resource, stops the worker and render threads and releases the
// backend.
func Shutdown() {
	C.Rshutdown()
}

func IsInitialized() int {
	return int(C.Ris_initialized())
}

// GetVersion: Returns: The library version, e.g. "0.1.0".
func GetVersion() string {
	return C.GoString(C.Rget_version())
}

// MemoryCheck: Walks the whole heap of Raven and verifies its structure. Returns: R_FALSE when it is
// corrupted (a buffer overrun wrote over a block header). Slow: for tests and for hunting a bug. Thread-safe.
func MemoryCheck() int {
	return int(C.Rmemory_check())
}

// MemoryTrim: Returns to the OS the regions of the heap that hold nothing. Thread-safe.
func MemoryTrim() {
	C.Rmemory_trim()
}

// DrawFrame: Produces one frame: processes the pending OS events without waiting, runs the posted tasks and
// the timers, then lays out, paints and submits every window that changed. A window in which nothing changed
// is not drawn again, so the call is cheap when the UI is idle. Returns: R_FALSE once Rquit was called or the
// last window is closed.
func DrawFrame() int {
	return int(C.Rdraw_frame())
}

// WaitFrame: Same as Rdraw_frame, but first sleeps until there is something to do: an OS event, a posted
// task, a timer, an animation, a pending redraw. timeout_seconds Longest sleep. Negative sleeps without
// limit, 0 does not sleep.
func WaitFrame(timeout_seconds float64) int {
	return int(C.Rwait_frame(C.double(timeout_seconds)))
}

// Quit: Makes Rdraw_frame and Rwait_frame return R_FALSE. Thread-safe.
func Quit(exit_code int) {
	C.Rquit(C.int(exit_code))
}

// GetExitCode: Returns: The code given to Rquit, 0 when the last window was closed.
func GetExitCode() int {
	return int(C.Rget_exit_code())
}

// IsRunning: Returns: R_FALSE once Rquit was called or every window is closed.
func IsRunning() int {
	return int(C.Ris_running())
}

// PollEvents: Processes the pending OS events without producing a frame. Rdraw_frame does it already: only
// useful to keep the windows responsive during a long operation.
func PollEvents() {
	C.Rpoll_events()
}

// WaitEvents: Sleeps until an OS event, a posted task, a timer or a pending redraw, and processes the OS
// events. Rwait_frame does it already. timeout_seconds Maximum wait. Negative waits without limit.
func WaitEvents(timeout_seconds float64) {
	C.Rwait_events(C.double(timeout_seconds))
}

// GetDeltaTime: Returns: Seconds between the last two frames.
func GetDeltaTime() float64 {
	return float64(C.Rget_delta_time())
}

// GetTime: Returns: Seconds elapsed since Rinit, from a monotonic clock. Thread-safe.
func GetTime() float64 {
	return float64(C.Rget_time())
}

// SetUiScale: Scale of the whole interface, on top of the one of the monitor (RInitDesc.ui_scale at start):
// every window is laid out again. 0.25 .. 4.
func SetUiScale(scale float32) {
	C.Rset_ui_scale(C.float(scale))
}

// SetVsync: R_TRUE: every frame waits for the vertical synchronization of the screen (no tearing, no wasted
// frames); R_FALSE: as fast as possible (RInitDesc.disable_vsync at start). Takes effect at the next frame.
func SetVsync(enabled int) {
	C.Rset_vsync(C.int(enabled))
}

func GetVsync() int {
	return int(C.Rget_vsync())
}

func GetUiScale() float32 {
	return float32(C.Rget_ui_scale())
}

// SetAnimationsEnabled: R_FALSE makes every change immediate: for users who ask for reduced motion
// (Raccessibility_set_reduce_motion), and for tests that read the state right after an input.
func SetAnimationsEnabled(enabled int) {
	C.Rset_animations_enabled(C.int(enabled))
}

func GetAnimationsEnabled() int {
	return int(C.Rget_animations_enabled())
}

// SetAnimationSpeed: Multiplies the speed of every animation. 1 is the default, 0.25 a slow motion.
func SetAnimationSpeed(speed float32) {
	C.Rset_animation_speed(C.float(speed))
}

func GetAnimationSpeed() float32 {
	return float32(C.Rget_animation_speed())
}

// SetAnimationTimeStep: When positive, each frame advances the animations by exactly this many seconds
// instead of the time really elapsed: frames rendered one by one for a video or a screenshot, deterministic
// tests. 0 follows the clock.
func SetAnimationTimeStep(seconds float64) {
	C.Rset_animation_time_step(C.double(seconds))
}

// WakeUp: Wakes Rwait_frame and Rwait_events from any thread.
func WakeUp() {
	C.Rwake_up()
}

func GetWorkerThreadCount() int {
	return int(C.Rget_worker_thread_count())
}

func IsUiThread() int {
	return int(C.Ris_ui_thread())
}

func TimerDestroy(timer ID) {
	C.Rtimer_destroy(C.Rid(timer))
}

func TimerIsValid(timer ID) int {
	return int(C.Rtimer_is_valid(C.Rid(timer)))
}

func ActionUnbind(name string) {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	C.Raction_unbind(name_)
}

// ActionFire: Fires an action as a widget would: the function bound to the name is called, or the default
// handler. What a tool bar button, a script or a test uses to do what a menu item does. sender Passed to the
// function; R_INVALID_ID when there is none.
func ActionFire(name string, sender ID) {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	C.Raction_fire(name_, C.Rid(sender))
}

// OpenUrl: Opens an address with the program the user has for it: a web page in the browser, a mailto: in the
// mail program, a file in its application. Returns at once. Returns: R_FALSE when the OS could not be asked
// (nothing tells whether a program then really opened it).
func OpenUrl(url string) int {
	url_ := C.CString(url)
	defer C.free(unsafe.Pointer(url_))
	return int(C.Ropen_url(url_))
}

func ClipboardSetText(text string) {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	C.Rclipboard_set_text(text_)
}

// ClipboardGetText: Returns: The clipboard text, or "" when empty. Valid until the next call.
func ClipboardGetText() string {
	return C.GoString(C.Rclipboard_get_text())
}

func ErrorToString(error_ RError) string {
	return C.GoString(C.Rerror_to_string(C.RError(error_)))
}

func SeverityToString(severity RSeverity) string {
	return C.GoString(C.Rseverity_to_string(C.RSeverity(severity)))
}

// WindowCreate: Opens a top-level window. width,height Client size in logical units. flags Bitmask of
// RWindowFlags.
func WindowCreate(title string, width int, height int, flags uint32) ID {
	title_ := C.CString(title)
	defer C.free(unsafe.Pointer(title_))
	return ID(C.Rwindow_create(title_, C.int(width), C.int(height), C.Ruint(flags)))
}

// WindowDestroy: Closes a window and deletes its content widgets.
func WindowDestroy(window ID) {
	C.Rwindow_destroy(C.Rid(window))
}

func WindowIsValid(window ID) int {
	return int(C.Rwindow_is_valid(C.Rid(window)))
}

// WindowSetContent: Sets the root widget of the window. The window takes ownership of the widget; the
// previous content is deleted.
func WindowSetContent(window ID, widget ID) {
	C.Rwindow_set_content(C.Rid(window), C.Rid(widget))
}

func WindowGetContent(window ID) ID {
	return ID(C.Rwindow_get_content(C.Rid(window)))
}

func WindowSetTitle(window ID, title string) {
	title_ := C.CString(title)
	defer C.free(unsafe.Pointer(title_))
	C.Rwindow_set_title(C.Rid(window), title_)
}

func WindowGetTitle(window ID) string {
	return C.GoString(C.Rwindow_get_title(C.Rid(window)))
}

func WindowSetSize(window ID, width int, height int) {
	C.Rwindow_set_size(C.Rid(window), C.int(width), C.int(height))
}

func WindowGetSize(window ID) (width int, height int) {
	var width_ C.int
	var height_ C.int
	C.Rwindow_get_size(C.Rid(window), &width_, &height_)
	width = int(width_)
	height = int(height_)
	return
}

func WindowSetMinSize(window ID, width int, height int) {
	C.Rwindow_set_min_size(C.Rid(window), C.int(width), C.int(height))
}

func WindowSetPosition(window ID, x int, y int) {
	C.Rwindow_set_position(C.Rid(window), C.int(x), C.int(y))
}

func WindowGetPosition(window ID) (x int, y int) {
	var x_ C.int
	var y_ C.int
	C.Rwindow_get_position(C.Rid(window), &x_, &y_)
	x = int(x_)
	y = int(y_)
	return
}

// WindowGetFramebufferSize: Size of the drawable area in pixels.
func WindowGetFramebufferSize(window ID) (width int, height int) {
	var width_ C.int
	var height_ C.int
	C.Rwindow_get_framebuffer_size(C.Rid(window), &width_, &height_)
	width = int(width_)
	height = int(height_)
	return
}

// WindowGetScale: Returns: Pixels per logical unit (monitor scale x global UI scale).
func WindowGetScale(window ID) float32 {
	return float32(C.Rwindow_get_scale(C.Rid(window)))
}

func WindowShow(window ID) {
	C.Rwindow_show(C.Rid(window))
}

func WindowHide(window ID) {
	C.Rwindow_hide(C.Rid(window))
}

func WindowMaximize(window ID) {
	C.Rwindow_maximize(C.Rid(window))
}

func WindowMinimize(window ID) {
	C.Rwindow_minimize(C.Rid(window))
}

func WindowRestore(window ID) {
	C.Rwindow_restore(C.Rid(window))
}

func WindowIsMaximized(window ID) int {
	return int(C.Rwindow_is_maximized(C.Rid(window)))
}

func WindowIsMinimized(window ID) int {
	return int(C.Rwindow_is_minimized(C.Rid(window)))
}

// WindowIsActive: Returns: R_TRUE while the window is the one the user is working in.
func WindowIsActive(window ID) int {
	return int(C.Rwindow_is_active(C.Rid(window)))
}

// WindowClose: Asks to close the window, as the close button of its title bar does: the close callback is
// asked first and may refuse.
func WindowClose(window ID) {
	C.Rwindow_close(C.Rid(window))
}

// WindowBeginMove: Custom headers. Call these from a mouse-down handler, while the left button is held: the
// window follows the pointer until the button is released (the widget does not receive the release). A
// R_WIDGET_HEADER_BAR and the widgets given to Rwidget_set_window_drag do it by themselves.
func WindowBeginMove(window ID) {
	C.Rwindow_begin_move(C.Rid(window))
}

func WindowBeginResize(window ID, edge RWindowEdge) {
	C.Rwindow_begin_resize(C.Rid(window), C.RWindowEdge(edge))
}

func WindowSetClearColor(window ID, r float32, g float32, b float32, a float32) {
	C.Rwindow_set_clear_color(C.Rid(window), C.float(r), C.float(g), C.float(b), C.float(a))
}

// WindowSetBackground: Image drawn behind the content, scaled to cover the window: what the glass of a glass
// theme looks through. R_INVALID_ID removes it.
func WindowSetBackground(window ID, texture ID) {
	C.Rwindow_set_background(C.Rid(window), C.Rid(texture))
}

// WindowSetBackdrop: Chooses what shows behind a window created with R_WINDOW_TRANSPARENT (see RBackdrop).
// With anything but R_BACKDROP_NONE the window is not cleared with its color and the wallpaper of the theme
// is not drawn: only what the widgets draw hides the screen. In XML: <Window Backdrop="Blur">. Returns:
// R_TRUE when the window system does it; R_FALSE (the window then stays as it was) without
// R_WINDOW_TRANSPARENT, or when it cannot.
func WindowSetBackdrop(window ID, backdrop RBackdrop) int {
	return int(C.Rwindow_set_backdrop(C.Rid(window), C.RBackdrop(backdrop)))
}

func WindowGetBackdrop(window ID) RBackdrop {
	return RBackdrop(C.Rwindow_get_backdrop(C.Rid(window)))
}

// WindowSetOpacity: Opacity of the whole window, its frame included, from 0 to 1: a window that fades, a tool
// that lets what is under it show. Returns: R_FALSE when the window system cannot do it.
func WindowSetOpacity(window ID, opacity float32) int {
	return int(C.Rwindow_set_opacity(C.Rid(window), C.float(opacity)))
}

func WindowGetOpacity(window ID) float32 {
	return float32(C.Rwindow_get_opacity(C.Rid(window)))
}

// WindowSetTopmost: Keeps the window above the other windows (splash screens, tools). Returns: R_FALSE when
// the window system cannot do it.
func WindowSetTopmost(window ID, topmost int) int {
	return int(C.Rwindow_set_topmost(C.Rid(window), C.int(topmost)))
}

// WindowCenter: Centers the window on the work area of its monitor (no task bar).
func WindowCenter(window ID) {
	C.Rwindow_center(C.Rid(window))
}

// WindowRequestRedraw: Asks for a new frame even if no widget changed. Thread-safe.
func WindowRequestRedraw(window ID) {
	C.Rwindow_request_redraw(C.Rid(window))
}

// WindowSetContinuousRedraw: R_TRUE redraws the window every frame (games, live viewports).
func WindowSetContinuousRedraw(window ID, enabled int) {
	C.Rwindow_set_continuous_redraw(C.Rid(window), C.int(enabled))
}

// WindowGetFocusedWidget: Widget that has the keyboard focus in the window, or R_INVALID_ID.
func WindowGetFocusedWidget(window ID) ID {
	return ID(C.Rwindow_get_focused_widget(C.Rid(window)))
}

// WindowGetHoveredWidget: Deepest widget under the cursor, or R_INVALID_ID.
func WindowGetHoveredWidget(window ID) ID {
	return ID(C.Rwindow_get_hovered_widget(C.Rid(window)))
}

func WindowGetMousePosition(window ID) (x float32, y float32) {
	var x_ C.float
	var y_ C.float
	C.Rwindow_get_mouse_position(C.Rid(window), &x_, &y_)
	x = float32(x_)
	y = float32(y_)
	return
}

// WindowInjectMouseMove: Inject input as if it came from the OS. Positions are in logical units. Useful for
// automated tests, replays and when Raven is driven by a host application.
func WindowInjectMouseMove(window ID, x float32, y float32) {
	C.Rwindow_inject_mouse_move(C.Rid(window), C.float(x), C.float(y))
}

func WindowInjectMouseButton(window ID, button RMouseButton, pressed int) {
	C.Rwindow_inject_mouse_button(C.Rid(window), C.RMouseButton(button), C.int(pressed))
}

func WindowInjectMouseWheel(window ID, wheel_x float32, wheel_y float32) {
	C.Rwindow_inject_mouse_wheel(C.Rid(window), C.float(wheel_x), C.float(wheel_y))
}

func WindowInjectKey(window ID, key RKey, pressed int, modifiers uint32) {
	C.Rwindow_inject_key(C.Rid(window), C.RKey(key), C.int(pressed), C.Ruint(modifiers))
}

func WindowInjectText(window ID, text string) {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	C.Rwindow_inject_text(C.Rid(window), text_)
}

// WindowInjectDrag: A drag from another application moves over the window (`inside` R_TRUE) or leaves it.
// Returns: R_TRUE when a drop at this position would be accepted.
func WindowInjectDrag(window ID, x float32, y float32, inside int) int {
	return int(C.Rwindow_inject_drag(C.Rid(window), C.float(x), C.float(y), C.int(inside)))
}

// WindowIsKeyDown: R_TRUE while the key is held down in the window (cleared when the window loses the
// keyboard).
func WindowIsKeyDown(window ID, key RKey) int {
	return int(C.Rwindow_is_key_down(C.Rid(window), C.RKey(key)))
}

// WindowIsMouseButtonDown: R_TRUE while the mouse button is held down over (or captured by) the window.
func WindowIsMouseButtonDown(window ID, button RMouseButton) int {
	return int(C.Rwindow_is_mouse_button_down(C.Rid(window), C.RMouseButton(button)))
}

// WindowGetModifiers: Modifiers (RModifier) of the last input event.
func WindowGetModifiers(window ID) uint32 {
	return uint32(C.Rwindow_get_modifiers(C.Rid(window)))
}

// WindowIsMouseInside: R_TRUE while the pointer is over the client area.
func WindowIsMouseInside(window ID) int {
	return int(C.Rwindow_is_mouse_inside(C.Rid(window)))
}

// WindowWantsTextInput: R_TRUE when the keyboard focus is in a text input, spin box or text editor.
func WindowWantsTextInput(window ID) int {
	return int(C.Rwindow_wants_text_input(C.Rid(window)))
}

// WindowHasPopup: R_TRUE while a menu, list or dialog is open over the window.
func WindowHasPopup(window ID) int {
	return int(C.Rwindow_has_popup(C.Rid(window)))
}

// WindowGetTextInputRect: Last painted caret in window pixels (x, y, width, height), including scrolling and
// scale. Call after Rdraw_frame. Independent of the blink phase. Returns R_FALSE and zeros the outputs when
// there is no visible editable caret, the window is inactive, or focus changed since painting. Any output
// pointer may be NULL.
func WindowGetTextInputRect(window ID) (result int, x int, y int, width int, height int) {
	var x_ C.int
	var y_ C.int
	var width_ C.int
	var height_ C.int
	result = int(C.Rwindow_get_text_input_rect(C.Rid(window), &x_, &y_, &width_, &height_))
	x = int(x_)
	y = int(y_)
	width = int(width_)
	height = int(height_)
	return
}

// WindowGetPopupCount: Returns: Number of open in-window popups (menus, combo lists, pickers), bottom to top.
func WindowGetPopupCount(window ID) int {
	return int(C.Rwindow_get_popup_count(C.Rid(window)))
}

// WindowGetPopup: Returns: Root widget of popup `index`, whose rect is in window coordinates; R_INVALID_ID
// out of range.
func WindowGetPopup(window ID, index int) ID {
	return ID(C.Rwindow_get_popup(C.Rid(window), C.int(index)))
}

// WindowSetTabNavigation: R_FALSE: Tab no longer moves the keyboard focus (the application uses it as a
// shortcut). Default R_TRUE.
func WindowSetTabNavigation(window ID, enabled int) {
	C.Rwindow_set_tab_navigation(C.Rid(window), C.int(enabled))
}

// DragBegin: Starts dragging `data` from `source` (call it while a mouse button is held, typically from a
// mouse-move event once the pointer has moved a few units). Until the button is released, the widgets with a
// drop callback (Rwidget_set_drop_callback) under the pointer receive R_DROP_ENTER / MOVE / LEAVE and, on
// release, R_DROP_DROPPED, with `drag_type` set and `text` = `data`. Returning R_FALSE from the callback
// refuses the drop there. Escape cancels. `label` follows the pointer. Returns: R_FALSE when the widget is
// not in a window.
func DragBegin(source ID, type_ string, data string, label string) int {
	type__ := C.CString(type_)
	defer C.free(unsafe.Pointer(type__))
	data_ := C.CString(data)
	defer C.free(unsafe.Pointer(data_))
	label_ := C.CString(label)
	defer C.free(unsafe.Pointer(label_))
	return int(C.Rdrag_begin(C.Rid(source), type__, data_, label_))
}

// DragIsActive: R_TRUE while a drag started by Rdrag_begin is in progress in the window.
func DragIsActive(window ID) int {
	return int(C.Rdrag_is_active(C.Rid(window)))
}

// DragGetType: Type of the drag in progress, or NULL.
func DragGetType(window ID) string {
	return C.GoString(C.Rdrag_get_type(C.Rid(window)))
}

// DragGetData: Data of the drag in progress, or NULL.
func DragGetData(window ID) string {
	return C.GoString(C.Rdrag_get_data(C.Rid(window)))
}

// DragCancel: Ends the drag in progress without dropping (the target receives R_DROP_LEAVE).
func DragCancel(window ID) {
	C.Rdrag_cancel(C.Rid(window))
}

func WindowRegisterShortcut(window ID, key RKey, modifiers uint32, action string) {
	action_ := C.CString(action)
	defer C.free(unsafe.Pointer(action_))
	C.Rwindow_register_shortcut(C.Rid(window), C.RKey(key), C.Ruint(modifiers), action_)
}

func WindowUnregisterShortcut(window ID, key RKey, modifiers uint32) {
	C.Rwindow_unregister_shortcut(C.Rid(window), C.RKey(key), C.Ruint(modifiers))
}

// LookSet: Changes the look of the application. The theme file in use, if it names no look of its own, is
// laid again over the new one; colors and metrics changed with Rtheme_set_... are lost; what was given to
// widgets (Rwidget_set_color...) is kept. Rlook_set(R_LOOK_NATIVE_LIGHT); the look of the platform, light
// Theme files that name no look (no Look attribute) follow it too, so a part of a window given one
// (Theme="paper.xml") changes with it.
func LookSet(look RLook) {
	C.Rlook_set(C.RLook(look))
}

// LookGet: The look given to Rlook_set (R_LOOK_RAVEN_DARK at start).
func LookGet() RLook {
	return RLook(C.Rlook_get())
}

// ThemeApply: @deprecated Rlook_set: a built-in theme is a look of Raven.
func ThemeApply(theme RTheme) {
	C.Rtheme_apply(C.RTheme(theme))
}

func ThemeSetColor(role RColorRole, r float32, g float32, b float32, a float32) {
	C.Rtheme_set_color(C.RColorRole(role), C.float(r), C.float(g), C.float(b), C.float(a))
}

func ThemeGetColor(role RColorRole) (r float32, g float32, b float32, a float32) {
	var r_ C.float
	var g_ C.float
	var b_ C.float
	var a_ C.float
	C.Rtheme_get_color(C.RColorRole(role), &r_, &g_, &b_, &a_)
	r = float32(r_)
	g = float32(g_)
	b = float32(b_)
	a = float32(a_)
	return
}

func ThemeSetMetric(metric RMetric, value float32) {
	C.Rtheme_set_metric(C.RMetric(metric), C.float(value))
}

func ThemeGetMetric(metric RMetric) float32 {
	return float32(C.Rtheme_get_metric(C.RMetric(metric)))
}

// ThemeSetMaterial: Material of every widget that was not given one with Rwidget_set_material. Rtheme_apply
// sets it: solid for the flat themes, frosted glass for the glass themes.
func ThemeSetMaterial(material RMaterial) {
	C.Rtheme_set_material(C.RMaterial(material))
}

func ThemeGetMaterial() RMaterial {
	return RMaterial(C.Rtheme_get_material())
}

// ThemeSetFont: Font used by widgets that have no font of their own.
func ThemeSetFont(font ID) {
	C.Rtheme_set_font(C.Rid(font))
}

func ThemeGetFont() ID {
	return ID(C.Rtheme_get_font())
}

// ThemeSetAccent: Sets the accent color and everything that follows from it: its hovered and pressed shades,
// the focus outline, the selection, the links.
func ThemeSetAccent(r float32, g float32, b float32) {
	C.Rtheme_set_accent(C.float(r), C.float(g), C.float(b))
}

// ThemeSetDark: Tells whether the theme is a dark one. It decides the tint of the glass, the colors of the
// code and which wallpaper the glass themes use. Rtheme_apply and the theme files set it.
func ThemeSetDark(dark int) {
	C.Rtheme_set_dark(C.int(dark))
}

func ThemeIsDark() int {
	return int(C.Rtheme_is_dark())
}

// ThemeSetWallpaper: Image drawn behind the content of the windows that have no background of their own, or
// R_INVALID_ID for none (the window color).
func ThemeSetWallpaper(texture ID) {
	C.Rtheme_set_wallpaper(C.Rid(texture))
}

func ThemeGetWallpaper() ID {
	return ID(C.Rtheme_get_wallpaper())
}

// ThemeSetStyleColor: Gives a color to the widgets of a type and / or of a class, instead of the color of the
// theme. widget Tag of a type of widget ("Button"), or NULL for every type. style_class A class
// (Rwidget_set_class, Class="..."), or NULL for the widgets of the type whatever their classes. When several
// styles give the same value to a widget, the one with a type and a class wins over the one with a class,
// which wins over the one with a type; between equals, the last one set wins.
func ThemeSetStyleColor(widget string, style_class string, role RColorRole, r float32, g float32, b float32, a float32) {
	widget_ := C.CString(widget)
	defer C.free(unsafe.Pointer(widget_))
	style_class_ := C.CString(style_class)
	defer C.free(unsafe.Pointer(style_class_))
	C.Rtheme_set_style_color(widget_, style_class_, C.RColorRole(role), C.float(r), C.float(g), C.float(b), C.float(a))
}

func ThemeSetStyleMetric(widget string, style_class string, metric RMetric, value float32) {
	widget_ := C.CString(widget)
	defer C.free(unsafe.Pointer(widget_))
	style_class_ := C.CString(style_class)
	defer C.free(unsafe.Pointer(style_class_))
	C.Rtheme_set_style_metric(widget_, style_class_, C.RMetric(metric), C.float(value))
}

// ThemeSetStyleMaterial: R_MATERIAL_DEFAULT removes the material of the style.
func ThemeSetStyleMaterial(widget string, style_class string, material RMaterial) {
	widget_ := C.CString(widget)
	defer C.free(unsafe.Pointer(widget_))
	style_class_ := C.CString(style_class)
	defer C.free(unsafe.Pointer(style_class_))
	C.Rtheme_set_style_material(widget_, style_class_, C.RMaterial(material))
}

// ThemeClearStyles: Removes every style of the theme. Rtheme_apply and Rtheme_use do it too.
func ThemeClearStyles() {
	C.Rtheme_clear_styles()
}

// ThemeLoad: Reads a theme file and keeps the theme under its name (the Name attribute, else the name of the
// file). It does not change the theme in use unless a theme of that name is the one in use. The file is
// watched like the interface files (Rxml_set_live_reload): saving it updates every widget that uses the
// theme. Returns: The name of the theme, valid until Rshutdown, or NULL after reporting an error.
func ThemeLoad(path string) string {
	path_ := C.CString(path)
	defer C.free(unsafe.Pointer(path_))
	return C.GoString(C.Rtheme_load(path_))
}

// ThemeLoadFromMemory: Same, from a text. `size` may be 0 for a null-terminated text.
func ThemeLoadFromMemory(xml string) string {
	xml_ := C.CString(xml)
	defer C.free(unsafe.Pointer(xml_))
	return C.GoString(C.Rtheme_load_from_memory(xml_, C.size_t(len(xml))))
}

// ThemeUse: Makes a named theme the theme of the application: a theme given by Rtheme_load (laid over the
// look in use when it names no look), or the name of a look ("WindowsLight", "RavenDark"; the old "Dark",
// "Light", "GlassDark"... too), which is then Rlook_set. Values changed with Rtheme_set_... are lost.
// Returns: R_FALSE, after reporting an error, when no theme has that name.
func ThemeUse(name string) int {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	return int(C.Rtheme_use(name_))
}

// ThemeGetName: Name of the theme in use ("Dark" at start).
func ThemeGetName() string {
	return C.GoString(C.Rtheme_get_name())
}

// ThemeGetCount: Number of named themes, the four built-in ones included.
func ThemeGetCount() int {
	return int(C.Rtheme_get_count())
}

// ThemeGetNameAt: Name of the index-th one, or NULL.
func ThemeGetNameAt(index int) string {
	return C.GoString(C.Rtheme_get_name_at(C.int(index)))
}

// ThemeSave: Writes the theme in use, with every change made since it was applied, as a theme file. name Name
// given to the theme in the file; NULL keeps the current one.
func ThemeSave(path string, name string) int {
	path_ := C.CString(path)
	defer C.free(unsafe.Pointer(path_))
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	return int(C.Rtheme_save(path_, name_))
}

// FontCreate: Creates a font from the content of a .ttf / .otf file. The buffer is copied. Glyphs are
// rasterized on demand, at the exact pixel size needed by each window scale, into a shared atlas. Returns:
// R_INVALID_ID on failure.
func FontCreate(data []byte) ID {
	var data_ *C.char
	if len(data) > 0 {
		data_ = (*C.char)(unsafe.Pointer(&data[0]))
	}
	return ID(C.Rfont_create(data_, C.size_t(len(data))))
}

func FontCreateFromFile(path string) ID {
	path_ := C.CString(path)
	defer C.free(unsafe.Pointer(path_))
	return ID(C.Rfont_create_from_file(path_))
}

func FontDestroy(font ID) {
	C.Rfont_destroy(C.Rid(font))
}

func FontIsValid(font ID) int {
	return int(C.Rfont_is_valid(C.Rid(font)))
}

// FontGetDefault: Returns: The font embedded in the library (Roboto, Latin and Latin Extended-A).
func FontGetDefault() ID {
	return ID(C.Rfont_get_default())
}

// FontGetMonospace: Returns: The fixed-width font embedded in the library (Roboto Mono, same characters),
// used by default by the text editors. In XML: Font="monospace".
func FontGetMonospace() ID {
	return ID(C.Rfont_get_monospace())
}

// FontSetFallback: Font used for the characters missing from `font` (symbols, icons, other scripts).
// Fallbacks can be chained.
func FontSetFallback(font ID, fallback ID) {
	C.Rfont_set_fallback(C.Rid(font), C.Rid(fallback))
}

// FontSetVariant: Gives a font the font to use for one of its styles: the bold file of the same family, its
// italic file. Formatted text (Markdown labels, the Markdown of the text editor) uses them. A font that was
// given none still has the three styles: Raven draws its glyphs thicker for bold and slanted for italic,
// keeping their advances, so that text does not move when its style changes. variant R_INVALID_ID goes back
// to the style Raven derives.
func FontSetVariant(font ID, style RFontStyle, variant ID) {
	C.Rfont_set_variant(C.Rid(font), C.RFontStyle(style), C.Rid(variant))
}

// FontGetVariant: Returns: The font drawn for a style of `font`: the one given with Rfont_set_variant, else
// the one Raven derives from it (made on first use; it lives as long as `font` and cannot be deleted by
// itself). Usable wherever a font is: Rlabel_set_font, Rdraw_text...
func FontGetVariant(font ID, style RFontStyle) ID {
	return ID(C.Rfont_get_variant(C.Rid(font), C.RFontStyle(style)))
}

// FontMeasureText: Measures a UTF-8 string, in logical units, at scale 1. font R_INVALID_ID uses the theme
// font. size Font size; 0 uses R_METRIC_FONT_SIZE.
func FontMeasureText(font ID, size float32, text string) (width float32, height float32) {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	var width_ C.float
	var height_ C.float
	C.Rfont_measure_text(C.Rid(font), C.float(size), text_, &width_, &height_)
	width = float32(width_)
	height = float32(height_)
	return
}

// TextureCreate: Creates a texture from the content of an image file (PNG, JPEG, BMP, TGA, GIF, PSD).
// Decoding happens on the calling thread. Returns: R_INVALID_ID on failure.
func TextureCreate(data []byte) ID {
	var data_ *C.char
	if len(data) > 0 {
		data_ = (*C.char)(unsafe.Pointer(&data[0]))
	}
	return ID(C.Rtexture_create(data_, C.size_t(len(data))))
}

// TextureCreateAsync: Same as Rtexture_create, decoded on a worker thread. The handle is usable at once;
// widgets showing it draw nothing until it is ready. The buffer is copied.
func TextureCreateAsync(data []byte) ID {
	var data_ *C.char
	if len(data) > 0 {
		data_ = (*C.char)(unsafe.Pointer(&data[0]))
	}
	return ID(C.Rtexture_create_async(data_, C.size_t(len(data))))
}

func TextureCreateFromFile(path string) ID {
	path_ := C.CString(path)
	defer C.free(unsafe.Pointer(path_))
	return ID(C.Rtexture_create_from_file(path_))
}

// TextureCreateFromFileAsync: Reads and decodes the file on a worker thread.
func TextureCreateFromFileAsync(path string) ID {
	path_ := C.CString(path)
	defer C.free(unsafe.Pointer(path_))
	return ID(C.Rtexture_create_from_file_async(path_))
}

// TextureIsReady: Returns: R_TRUE once an asynchronous texture is decoded (or failed).
func TextureIsReady(texture ID) int {
	return int(C.Rtexture_is_ready(C.Rid(texture)))
}

// TextureWait: Blocks until an asynchronous texture is ready.
func TextureWait(texture ID) {
	C.Rtexture_wait(C.Rid(texture))
}

func TextureDestroy(texture ID) {
	C.Rtexture_destroy(C.Rid(texture))
}

func TextureIsValid(texture ID) int {
	return int(C.Rtexture_is_valid(C.Rid(texture)))
}

func TextureGetSize(texture ID) (width int, height int) {
	var width_ C.int
	var height_ C.int
	C.Rtexture_get_size(C.Rid(texture), &width_, &height_)
	width = int(width_)
	height = int(height_)
	return
}

// TextureSetFilter: How the texture is smoothed when it is scaled. The textures made from image files start
// with R_FILTER_MIPMAP, the others with R_FILTER_LINEAR.
func TextureSetFilter(texture ID, filter RFilterType) {
	C.Rtexture_set_filter(C.Rid(texture), C.RFilterType(filter))
}

// WidgetCreate: Creates a widget. It is not shown until it is added to a parent that belongs to a window
// (Rwidget_add_child, Rwindow_set_content).
func WidgetCreate(type_ RWidgetType) ID {
	return ID(C.Rwidget_create(C.RWidgetType(type_)))
}

// WidgetDestroy: Deletes a widget and all its descendants.
func WidgetDestroy(widget ID) {
	C.Rwidget_destroy(C.Rid(widget))
}

func WidgetIsValid(widget ID) int {
	return int(C.Rwidget_is_valid(C.Rid(widget)))
}

func WidgetGetType(widget ID) RWidgetType {
	return RWidgetType(C.Rwidget_get_type(C.Rid(widget)))
}

// WidgetAddChild: Appends a child to a panel. The parent takes ownership. A widget that already has a parent
// is moved. Returns: `child`, so calls can be nested: Rid ok = Rwidget_add_child(row, Rbutton_create("OK"));
func WidgetAddChild(parent ID, child ID) ID {
	return ID(C.Rwidget_add_child(C.Rid(parent), C.Rid(child)))
}

func WidgetInsertChild(parent ID, child ID, index int) ID {
	return ID(C.Rwidget_insert_child(C.Rid(parent), C.Rid(child), C.int(index)))
}

// WidgetRemoveChild: Detaches a child without deleting it. The caller owns it again.
func WidgetRemoveChild(parent ID, child ID) {
	C.Rwidget_remove_child(C.Rid(parent), C.Rid(child))
}

// WidgetClearChildren: Deletes every child of a panel.
func WidgetClearChildren(parent ID) {
	C.Rwidget_clear_children(C.Rid(parent))
}

func WidgetGetParent(widget ID) ID {
	return ID(C.Rwidget_get_parent(C.Rid(widget)))
}

func WidgetGetChildCount(widget ID) int {
	return int(C.Rwidget_get_child_count(C.Rid(widget)))
}

func WidgetGetChild(widget ID, index int) ID {
	return ID(C.Rwidget_get_child(C.Rid(widget), C.int(index)))
}

// WidgetGetWindow: Returns: The window showing the widget, or R_INVALID_ID.
func WidgetGetWindow(widget ID) ID {
	return ID(C.Rwidget_get_window(C.Rid(widget)))
}

func WidgetSetName(widget ID, name string) {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	C.Rwidget_set_name(C.Rid(widget), name_)
}

func WidgetGetName(widget ID) string {
	return C.GoString(C.Rwidget_get_name(C.Rid(widget)))
}

// WidgetFind: Finds a widget by name. scope A window or a widget; the search covers its whole subtree.
// Returns: R_INVALID_ID when no widget has that name.
func WidgetFind(scope ID, name string) ID {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	return ID(C.Rwidget_find(C.Rid(scope), name_))
}

// SlotSetPadding: Space kept around the child, inside its slot. All panels.
func SlotSetPadding(child ID, left float32, top float32, right float32, bottom float32) {
	C.Rslot_set_padding(C.Rid(child), C.float(left), C.float(top), C.float(right), C.float(bottom))
}

// SlotSetHalign: Alignment of the child inside its slot. Default: fill. All panels.
func SlotSetHalign(child ID, align RHAlign) {
	C.Rslot_set_halign(C.Rid(child), C.RHAlign(align))
}

func SlotSetValign(child ID, align RVAlign) {
	C.Rslot_set_valign(C.Rid(child), C.RVAlign(align))
}

// SlotSetAuto: The slot takes the desired size of the child along the panel direction (Slate AutoWidth /
// AutoHeight). This is the default. VBox, HBox.
func SlotSetAuto(child ID) {
	C.Rslot_set_auto(C.Rid(child))
}

// SlotSetFill: The slot shares the space left by the auto slots, in proportion to `coefficient` (Slate
// FillWidth / FillHeight). VBox, HBox. In a Splitter, the coefficient is the relative size of the pane.
func SlotSetFill(child ID, coefficient float32) {
	C.Rslot_set_fill(C.Rid(child), C.float(coefficient))
}

// SlotSetMaxSize: Upper bound of the slot along the panel direction. 0 = none. VBox, HBox.
func SlotSetMaxSize(child ID, max_size float32) {
	C.Rslot_set_max_size(C.Rid(child), C.float(max_size))
}

// SlotSetGrid: Cell of the child. Grid, UniformGrid.
func SlotSetGrid(child ID, column int, row int) {
	C.Rslot_set_grid(C.Rid(child), C.int(column), C.int(row))
}

func SlotSetGridSpan(child ID, column_span int, row_span int) {
	C.Rslot_set_grid_span(C.Rid(child), C.int(column_span), C.int(row_span))
}

// SlotSetAnchors: Canvas placement. Anchors are fractions of the canvas size (0..1). - anchors equal on an
// axis: the child is at anchor + position, with the slot size (or its desired size when the slot size is <=
// 0); - anchors different on an axis: the child stretches between them, position and size are then the near
// and far margins. Alignment is the pivot of the child (0,0 = top-left, 0.5,0.5 = center).
func SlotSetAnchors(child ID, min_x float32, min_y float32, max_x float32, max_y float32) {
	C.Rslot_set_anchors(C.Rid(child), C.float(min_x), C.float(min_y), C.float(max_x), C.float(max_y))
}

func SlotSetPosition(child ID, x float32, y float32) {
	C.Rslot_set_position(C.Rid(child), C.float(x), C.float(y))
}

func SlotSetSize(child ID, width float32, height float32) {
	C.Rslot_set_size(C.Rid(child), C.float(width), C.float(height))
}

func SlotSetAlignment(child ID, x float32, y float32) {
	C.Rslot_set_alignment(C.Rid(child), C.float(x), C.float(y))
}

// SlotSetTitle: Title of the tab showing the child. TabView.
func SlotSetTitle(child ID, title string) {
	title_ := C.CString(title)
	defer C.free(unsafe.Pointer(title_))
	C.Rslot_set_title(C.Rid(child), title_)
}

func WidgetSetVisibility(widget ID, visibility RVisibility) {
	C.Rwidget_set_visibility(C.Rid(widget), C.RVisibility(visibility))
}

func WidgetGetVisibility(widget ID) RVisibility {
	return RVisibility(C.Rwidget_get_visibility(C.Rid(widget)))
}

// WidgetSetEnabled: A disabled widget ignores input and is drawn dimmed, with its descendants.
func WidgetSetEnabled(widget ID, enabled int) {
	C.Rwidget_set_enabled(C.Rid(widget), C.int(enabled))
}

func WidgetIsEnabled(widget ID) int {
	return int(C.Rwidget_is_enabled(C.Rid(widget)))
}

// WidgetSetHitTestable: R_FALSE lets mouse events pass through the widget (not its children).
func WidgetSetHitTestable(widget ID, hit_testable int) {
	C.Rwidget_set_hit_testable(C.Rid(widget), C.int(hit_testable))
}

// WidgetSetOpacity: Opacity multiplier applied to the widget and its descendants (0..1).
func WidgetSetOpacity(widget ID, opacity float32) {
	C.Rwidget_set_opacity(C.Rid(widget), C.float(opacity))
}

// WidgetSetTransition: Gives the widget a transition that plays each time it appears: when its window is
// first drawn, when it is added to a window that is shown, and when it becomes visible again
// (Rwidget_set_visibility, a tab or a switcher showing it). In XML: Appear="FlyUp" AppearDelay="0.1".
// delay_seconds Time before it starts. Give growing delays to the items of a group to make them arrive one
// after the other.
func WidgetSetTransition(widget ID, transition RTransition, delay_seconds float32) {
	C.Rwidget_set_transition(C.Rid(widget), C.RTransition(transition), C.float(delay_seconds))
}

// WidgetPlayTransition: Plays a transition once, now, whatever the widget was given with
// Rwidget_set_transition.
func WidgetPlayTransition(widget ID, transition RTransition, delay_seconds float32) {
	C.Rwidget_play_transition(C.Rid(widget), C.RTransition(transition), C.float(delay_seconds))
}

// WidgetSetClipping: R_TRUE clips the descendants to the bounds of the widget.
func WidgetSetClipping(widget ID, clip int) {
	C.Rwidget_set_clipping(C.Rid(widget), C.int(clip))
}

// WidgetSetTooltip: Text shown when the cursor rests on the widget. NULL removes it.
func WidgetSetTooltip(widget ID, text string) {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	C.Rwidget_set_tooltip(C.Rid(widget), text_)
}

// WidgetGetTooltip: Returns: The tooltip text of the widget, "" when it has none.
func WidgetGetTooltip(widget ID) string {
	return C.GoString(C.Rwidget_get_tooltip(C.Rid(widget)))
}

func WidgetSetCursor(widget ID, cursor RCursor) {
	C.Rwidget_set_cursor(C.Rid(widget), C.RCursor(cursor))
}

// WidgetSetSize: Size constraints, in logical units. They apply to the desired size the widget reports to its
// parent. A negative value removes the constraint on that axis.
func WidgetSetSize(widget ID, width float32, height float32) {
	C.Rwidget_set_size(C.Rid(widget), C.float(width), C.float(height))
}

func WidgetSetMinSize(widget ID, width float32, height float32) {
	C.Rwidget_set_min_size(C.Rid(widget), C.float(width), C.float(height))
}

func WidgetSetMaxSize(widget ID, width float32, height float32) {
	C.Rwidget_set_max_size(C.Rid(widget), C.float(width), C.float(height))
}

// WidgetGetRect: Bounds of the widget in its window, in logical units, after the last layout.
func WidgetGetRect(widget ID) (x float32, y float32, width float32, height float32) {
	var x_ C.float
	var y_ C.float
	var width_ C.float
	var height_ C.float
	C.Rwidget_get_rect(C.Rid(widget), &x_, &y_, &width_, &height_)
	x = float32(x_)
	y = float32(y_)
	width = float32(width_)
	height = float32(height_)
	return
}

func WidgetGetDesiredSize(widget ID) (width float32, height float32) {
	var width_ C.float
	var height_ C.float
	C.Rwidget_get_desired_size(C.Rid(widget), &width_, &height_)
	width = float32(width_)
	height = float32(height_)
	return
}

func WidgetIsHovered(widget ID) int {
	return int(C.Rwidget_is_hovered(C.Rid(widget)))
}

func WidgetSetFocusable(widget ID, focusable int) {
	C.Rwidget_set_focusable(C.Rid(widget), C.int(focusable))
}

// WidgetFocus: Gives the keyboard focus to the widget.
func WidgetFocus(widget ID) {
	C.Rwidget_focus(C.Rid(widget))
}

func WidgetIsFocused(widget ID) int {
	return int(C.Rwidget_is_focused(C.Rid(widget)))
}

// WidgetSetAction: Name of the action fired by the main event of the widget: button click, value change, text
// commit, item activation (see Raction_bind).
func WidgetSetAction(widget ID, action string) {
	action_ := C.CString(action)
	defer C.free(unsafe.Pointer(action_))
	C.Rwidget_set_action(C.Rid(widget), action_)
}

func WidgetGetAction(widget ID) string {
	return C.GoString(C.Rwidget_get_action(C.Rid(widget)))
}

// WidgetSetWindowDrag: In a window created with R_WINDOW_CUSTOM_HEADER: dragging the widget moves the window
// and a double click maximizes it, as a title bar does. The buttons, fields and other controls inside it keep
// working. In XML: WindowDrag="true".
func WidgetSetWindowDrag(widget ID, enabled int) {
	C.Rwidget_set_window_drag(C.Rid(widget), C.int(enabled))
}

// WidgetHitTest: Deepest widget of a subtree under a point of its window, or R_INVALID_ID.
func WidgetHitTest(root ID, x float32, y float32) ID {
	return ID(C.Rwidget_hit_test(C.Rid(root), C.float(x), C.float(y)))
}

// WidgetPick: Deepest visible widget of a subtree under a point of its window, whether it reacts to the
// pointer or not: a label, a panel, a disabled button. What a design tool selects with a click.
func WidgetPick(root ID, x float32, y float32) ID {
	return ID(C.Rwidget_pick(C.Rid(root), C.float(x), C.float(y)))
}

// WidgetSetContextMenu: Menu opened by a right click on the widget. The widget does not own it.
func WidgetSetContextMenu(widget ID, menu ID) {
	C.Rwidget_set_context_menu(C.Rid(widget), C.Rid(menu))
}

// WidgetInvalidate: Asks for the widget to be laid out and painted again.
func WidgetInvalidate(widget ID) {
	C.Rwidget_invalidate(C.Rid(widget))
}

// WidgetSetColor: Overrides a theme color for one widget. R_COLOR_TEXT is also inherited by its descendants.
func WidgetSetColor(widget ID, role RColorRole, r float32, g float32, b float32, a float32) {
	C.Rwidget_set_color(C.Rid(widget), C.RColorRole(role), C.float(r), C.float(g), C.float(b), C.float(a))
}

func WidgetClearColor(widget ID, role RColorRole) {
	C.Rwidget_clear_color(C.Rid(widget), C.RColorRole(role))
}

// WidgetGetColor: The color the widget is drawn with for a role: its own, else the one of the styles of its
// theme that apply to it, else the one of its theme. What a custom widget should draw itself with.
func WidgetGetColor(widget ID, role RColorRole) (r float32, g float32, b float32, a float32) {
	var r_ C.float
	var g_ C.float
	var b_ C.float
	var a_ C.float
	C.Rwidget_get_color(C.Rid(widget), C.RColorRole(role), &r_, &g_, &b_, &a_)
	r = float32(r_)
	g = float32(g_)
	b = float32(b_)
	a = float32(a_)
	return
}

// WidgetSetMaterial: Gives one widget a material of its own, whatever the theme: a solid button in a glass
// window, a panel of clear glass in a flat one. A widget that draws no surface (a box, a label) gets one of
// this material behind it: any widget can become a panel of glass. In XML: Material="Glass | FrostedGlass |
// TintedGlass | Solid". material R_MATERIAL_DEFAULT follows the theme again.
func WidgetSetMaterial(widget ID, material RMaterial) {
	C.Rwidget_set_material(C.Rid(widget), C.RMaterial(material))
}

// WidgetGetMaterial: Returns: The material the widget is drawn with: its own, or the one of the theme.
func WidgetGetMaterial(widget ID) RMaterial {
	return RMaterial(C.Rwidget_get_material(C.Rid(widget)))
}

// WidgetSetMetric: Overrides a theme metric for one widget.
func WidgetSetMetric(widget ID, metric RMetric, value float32) {
	C.Rwidget_set_metric(C.Rid(widget), C.RMetric(metric), C.float(value))
}

func WidgetClearMetric(widget ID, metric RMetric) {
	C.Rwidget_clear_metric(C.Rid(widget), C.RMetric(metric))
}

// WidgetGetMetric: Returns: The metric the widget uses: its own, else the one of a style, else the one of its
// theme.
func WidgetGetMetric(widget ID, metric RMetric) float32 {
	return float32(C.Rwidget_get_metric(C.Rid(widget), C.RMetric(metric)))
}

// WidgetSetClass: Classes of the widget, separated by spaces ("danger large"): the styles of the theme made
// for these classes apply to it (Rtheme_set_style_color, <Style Class="danger" .../> in a theme file). In
// XML: Class="...". NULL or "" removes them.
func WidgetSetClass(widget ID, classes string) {
	classes_ := C.CString(classes)
	defer C.free(unsafe.Pointer(classes_))
	C.Rwidget_set_class(C.Rid(widget), classes_)
}

// WidgetGetClass: Returns: The classes, "" when it has none. Valid until they change.
func WidgetGetClass(widget ID) string {
	return C.GoString(C.Rwidget_get_class(C.Rid(widget)))
}

func WidgetAddClass(widget ID, style_class string) {
	style_class_ := C.CString(style_class)
	defer C.free(unsafe.Pointer(style_class_))
	C.Rwidget_add_class(C.Rid(widget), style_class_)
}

func WidgetRemoveClass(widget ID, style_class string) {
	style_class_ := C.CString(style_class)
	defer C.free(unsafe.Pointer(style_class_))
	C.Rwidget_remove_class(C.Rid(widget), style_class_)
}

func WidgetHasClass(widget ID, style_class string) int {
	style_class_ := C.CString(style_class)
	defer C.free(unsafe.Pointer(style_class_))
	return int(C.Rwidget_has_class(C.Rid(widget), style_class_))
}

// WidgetSetTheme: Gives the widget and everything inside it a theme of their own, by name ("Dark",
// "GlassLight", or a theme of Rtheme_load): a dark side bar in a light window, a preview of another theme.
// Menus and lists opened from inside follow. In XML: Theme="...". name NULL or "" follows the parent (the
// application theme) again. Returns: R_FALSE, after reporting an error, when no theme has that name.
func WidgetSetTheme(widget ID, name string) int {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	return int(C.Rwidget_set_theme(C.Rid(widget), name_))
}

// WidgetGetTheme: Returns: Name of the theme the widget is drawn with.
func WidgetGetTheme(widget ID) string {
	return C.GoString(C.Rwidget_get_theme(C.Rid(widget)))
}

// PanelSetSpacing: Gap inserted between neighbouring children.
func PanelSetSpacing(panel ID, spacing float32) {
	C.Rpanel_set_spacing(C.Rid(panel), C.float(spacing))
}

func PanelGetSpacing(panel ID) float32 {
	return float32(C.Rpanel_get_spacing(C.Rid(panel)))
}

// GridSetColumnFill: Lets a column / row grow to take a share of the free space (Slate FillColumn / FillRow).
// 0 sizes it to its content (default).
func GridSetColumnFill(grid ID, column int, coefficient float32) {
	C.Rgrid_set_column_fill(C.Rid(grid), C.int(column), C.float(coefficient))
}

func GridSetRowFill(grid ID, row int, coefficient float32) {
	C.Rgrid_set_row_fill(C.Rid(grid), C.int(row), C.float(coefficient))
}

// UniformGridSetColumns: Places children automatically, left to right then top to bottom, in the given number
// of columns. 0 uses the grid cell of each slot (default).
func UniformGridSetColumns(grid ID, columns int) {
	C.Runiform_grid_set_columns(C.Rid(grid), C.int(columns))
}

func UniformGridSetMinCellSize(grid ID, width float32, height float32) {
	C.Runiform_grid_set_min_cell_size(C.Rid(grid), C.float(width), C.float(height))
}

// BorderSetPadding: Space between the outline and the child.
func BorderSetPadding(border ID, left float32, top float32, right float32, bottom float32) {
	C.Rborder_set_padding(C.Rid(border), C.float(left), C.float(top), C.float(right), C.float(bottom))
}

// BorderSetBackground: Background color (R_COLOR_PANEL of this widget). Alpha 0 draws no background.
func BorderSetBackground(border ID, r float32, g float32, b float32, a float32) {
	C.Rborder_set_background(C.Rid(border), C.float(r), C.float(g), C.float(b), C.float(a))
}

// BorderSetOutline: Outline width and color. Width 0 draws no outline (default).
func BorderSetOutline(border ID, width float32, r float32, g float32, b float32, a float32) {
	C.Rborder_set_outline(C.Rid(border), C.float(width), C.float(r), C.float(g), C.float(b), C.float(a))
}

func BorderSetCornerRadius(border ID, top_left float32, top_right float32, bottom_right float32, bottom_left float32) {
	C.Rborder_set_corner_radius(C.Rid(border), C.float(top_left), C.float(top_right), C.float(bottom_right), C.float(bottom_left))
}

// BorderSetShadow: Soft drop shadow behind the border. Blur 0 disables it (default).
func BorderSetShadow(border ID, blur float32, offset_x float32, offset_y float32) {
	C.Rborder_set_shadow(C.Rid(border), C.float(blur), C.float(offset_x), C.float(offset_y))
}

// BorderSetMaterial: What the border is made of. R_MATERIAL_GLASS turns it into a panel of glass tinted by
// its background color (the alpha of the color is the strength of the tint): floating tool bars, side bars,
// cards.
func BorderSetMaterial(border ID, material RMaterial) {
	C.Rborder_set_material(C.Rid(border), C.RMaterial(material))
}

// SizeBoxSetAspectRatio: Keeps the child at a fixed width / height ratio, as large as fits. 0 disables it
// (default). The fixed, minimum and maximum sizes are set with Rwidget_set_size, Rwidget_set_min_size and
// Rwidget_set_max_size.
func SizeBoxSetAspectRatio(size_box ID, ratio float32) {
	C.Rsize_box_set_aspect_ratio(C.Rid(size_box), C.float(ratio))
}

func SpacerSetSize(spacer ID, width float32, height float32) {
	C.Rspacer_set_size(C.Rid(spacer), C.float(width), C.float(height))
}

// ScrollBoxSetOrientation: Direction in which children are stacked and scrolled. Default: R_VERTICAL.
func ScrollBoxSetOrientation(scroll_box ID, orientation ROrientation) {
	C.Rscroll_box_set_orientation(C.Rid(scroll_box), C.ROrientation(orientation))
}

// ScrollBoxSetScrollBoth: R_TRUE also scrolls across the stacking direction when the content is too large.
func ScrollBoxSetScrollBoth(scroll_box ID, enabled int) {
	C.Rscroll_box_set_scroll_both(C.Rid(scroll_box), C.int(enabled))
}

func ScrollBoxSetOffset(scroll_box ID, x float32, y float32) {
	C.Rscroll_box_set_offset(C.Rid(scroll_box), C.float(x), C.float(y))
}

func ScrollBoxGetOffset(scroll_box ID) (x float32, y float32) {
	var x_ C.float
	var y_ C.float
	C.Rscroll_box_get_offset(C.Rid(scroll_box), &x_, &y_)
	x = float32(x_)
	y = float32(y_)
	return
}

func ScrollBoxScrollToEnd(scroll_box ID) {
	C.Rscroll_box_scroll_to_end(C.Rid(scroll_box))
}

// WidgetScrollIntoView: Scrolls every ScrollBox above the widget so that the widget is visible.
func WidgetScrollIntoView(widget ID) {
	C.Rwidget_scroll_into_view(C.Rid(widget))
}

// SplitterSetOrientation: R_HORIZONTAL puts the panes side by side (default), R_VERTICAL stacks them.
func SplitterSetOrientation(splitter ID, orientation ROrientation) {
	C.Rsplitter_set_orientation(C.Rid(splitter), C.ROrientation(orientation))
}

func SplitterSetMinPaneSize(splitter ID, size float32) {
	C.Rsplitter_set_min_pane_size(C.Rid(splitter), C.float(size))
}

// BackgroundBlurCreate: Creates a panel that shows what is behind it out of focus, with one child over it. In
// XML: <BackgroundBlur Style="Frosted" Radius="24" CornerRadius="18" Padding="16"> <Label>Over a blurred view
// of the window</Label> </BackgroundBlur> It works in any theme and with any material: it is not the glass of
// the glass themes (which bends and reflects), but the looks of RBlurStyle. Unlike the opacity of a widget,
// it does not let what is behind show sharp.
func BackgroundBlurCreate() ID {
	return ID(C.Rbackground_blur_create())
}

func BackgroundBlurSetStyle(blur ID, style RBlurStyle) {
	C.Rbackground_blur_set_style(C.Rid(blur), C.RBlurStyle(style))
}

func BackgroundBlurGetStyle(blur ID) RBlurStyle {
	return RBlurStyle(C.Rbackground_blur_get_style(C.Rid(blur)))
}

// BackgroundBlurSetRadius: How far what is behind goes out of focus, in logical units: 4 softens, 20 hides
// the details, 60 leaves only colors. A negative value (the default) uses R_METRIC_BLUR_RADIUS of the theme.
func BackgroundBlurSetRadius(blur ID, radius float32) {
	C.Rbackground_blur_set_radius(C.Rid(blur), C.float(radius))
}

func BackgroundBlurGetRadius(blur ID) float32 {
	return float32(C.Rbackground_blur_get_radius(C.Rid(blur)))
}

// BackgroundBlurSetTint: Color laid over the blur; its alpha is how much of it. Without one, R_BLUR_PLAIN has
// none and the other styles take theirs from the theme (R_COLOR_PANEL).
func BackgroundBlurSetTint(blur ID, r float32, g float32, b float32, a float32) {
	C.Rbackground_blur_set_tint(C.Rid(blur), C.float(r), C.float(g), C.float(b), C.float(a))
}

// BackgroundBlurClearTint: Removes the tint given with Rbackground_blur_set_tint.
func BackgroundBlurClearTint(blur ID) {
	C.Rbackground_blur_clear_tint(C.Rid(blur))
}

// BackgroundBlurSetSaturation: Saturation of what is seen through: 1 = as it is, 0 = gray, 1.5 = vivid.
// Negative = the one of the style.
func BackgroundBlurSetSaturation(blur ID, saturation float32) {
	C.Rbackground_blur_set_saturation(C.Rid(blur), C.float(saturation))
}

// BackgroundBlurSetGrain: Fine grain over the blur, 0 = none .. 1. Negative = the one of the style.
func BackgroundBlurSetGrain(blur ID, grain float32) {
	C.Rbackground_blur_set_grain(C.Rid(blur), C.float(grain))
}

func BackgroundBlurSetCornerRadius(blur ID, top_left float32, top_right float32, bottom_right float32, bottom_left float32) {
	C.Rbackground_blur_set_corner_radius(C.Rid(blur), C.float(top_left), C.float(top_right), C.float(bottom_right), C.float(bottom_left))
}

// BackgroundBlurSetPadding: Space between the edge and the child.
func BackgroundBlurSetPadding(blur ID, left float32, top float32, right float32, bottom float32) {
	C.Rbackground_blur_set_padding(C.Rid(blur), C.float(left), C.float(top), C.float(right), C.float(bottom))
}

// BackgroundBlurSetShadow: Soft shadow under the panel; blur 0 = none (default).
func BackgroundBlurSetShadow(blur ID, shadow_blur float32) {
	C.Rbackground_blur_set_shadow(C.Rid(blur), C.float(shadow_blur))
}

func SwitcherSetIndex(switcher ID, index int) {
	C.Rswitcher_set_index(C.Rid(switcher), C.int(index))
}

func SwitcherGetIndex(switcher ID) int {
	return int(C.Rswitcher_get_index(C.Rid(switcher)))
}

// TabViewAddTab: Adds a page. Same as Rwidget_add_child followed by Rslot_set_title. Returns: The index of
// the new tab, or -1.
func TabViewAddTab(tab_view ID, title string, content ID) int {
	title_ := C.CString(title)
	defer C.free(unsafe.Pointer(title_))
	return int(C.Rtab_view_add_tab(C.Rid(tab_view), title_, C.Rid(content)))
}

func TabViewSetActive(tab_view ID, index int) {
	C.Rtab_view_set_active(C.Rid(tab_view), C.int(index))
}

func TabViewGetActive(tab_view ID) int {
	return int(C.Rtab_view_get_active(C.Rid(tab_view)))
}

// TabViewSetDocuments: Documents mode: the major tabs of an application (the level, then one tab per open
// asset, as in Unreal): tabs on the left with an icon and a close button (a middle click closes too),
// shrinking when they do not fit, no slide between pages. XML: Documents="true".
func TabViewSetDocuments(tab_view ID, documents int) {
	C.Rtab_view_set_documents(C.Rid(tab_view), C.int(documents))
}

func TabViewGetTabCount(tab_view ID) int {
	return int(C.Rtab_view_get_tab_count(C.Rid(tab_view)))
}

// TabViewGetTab: The page of a tab, R_INVALID_ID if none.
func TabViewGetTab(tab_view ID, index int) ID {
	return ID(C.Rtab_view_get_tab(C.Rid(tab_view), C.int(index)))
}

// TabViewFindTab: Index of the tab of a page, -1 if none.
func TabViewFindTab(tab_view ID, content ID) int {
	return int(C.Rtab_view_find_tab(C.Rid(tab_view), C.Rid(content)))
}

func TabViewSetTabTitle(tab_view ID, content ID, title string) {
	title_ := C.CString(title)
	defer C.free(unsafe.Pointer(title_))
	C.Rtab_view_set_tab_title(C.Rid(tab_view), C.Rid(content), title_)
}

// TabViewSetTabIcon: Icon before the title (documents). R_INVALID_ID removes it; tint: a mask in the text
// color.
func TabViewSetTabIcon(tab_view ID, content ID, texture ID, u0 float32, v0 float32, u1 float32, v1 float32, tint int) {
	C.Rtab_view_set_tab_icon(C.Rid(tab_view), C.Rid(content), C.Rid(texture), C.float(u0), C.float(v0), C.float(u1), C.float(v1), C.int(tint))
}

// TabViewSetTabClosable: R_FALSE: no close button (documents ; the level tab). Tabs can be closed by default.
func TabViewSetTabClosable(tab_view ID, content ID, closable int) {
	C.Rtab_view_set_tab_closable(C.Rid(tab_view), C.Rid(content), C.int(closable))
}

// TabViewShowTab: Shows the tab of a page (the changed callback is called).
func TabViewShowTab(tab_view ID, content ID) {
	C.Rtab_view_show_tab(C.Rid(tab_view), C.Rid(content))
}

func ExpanderSetTitle(expander ID, title string) {
	title_ := C.CString(title)
	defer C.free(unsafe.Pointer(title_))
	C.Rexpander_set_title(C.Rid(expander), title_)
}

func ExpanderGetTitle(expander ID) string {
	return C.GoString(C.Rexpander_get_title(C.Rid(expander)))
}

func ExpanderSetExpanded(expander ID, expanded int) {
	C.Rexpander_set_expanded(C.Rid(expander), C.int(expanded))
}

func ExpanderIsExpanded(expander ID) int {
	return int(C.Rexpander_is_expanded(C.Rid(expander)))
}

// SetLayoutDirection: Direction of every window: R_DIRECTION_AUTO (the default), _LTR or _RTL.
func SetLayoutDirection(direction RDirection) {
	C.Rset_layout_direction(C.RDirection(direction))
}

// GetLayoutDirection: The direction the windows use now: R_DIRECTION_LTR or _RTL.
func GetLayoutDirection() RDirection {
	return RDirection(C.Rget_layout_direction())
}

// WidgetSetDirection: Direction of a part of a window: R_DIRECTION_INHERIT, _LTR or _RTL.
func WidgetSetDirection(widget ID, direction RDirection) {
	C.Rwidget_set_direction(C.Rid(widget), C.RDirection(direction))
}

// WidgetGetDirection: The direction the widget is laid out in: R_DIRECTION_LTR or _RTL.
func WidgetGetDirection(widget ID) RDirection {
	return RDirection(C.Rwidget_get_direction(C.Rid(widget)))
}

// LabelCreate: Shortcut for Rwidget_create(R_WIDGET_LABEL) + Rlabel_set_text.
func LabelCreate(text string) ID {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	return ID(C.Rlabel_create(text_))
}

// LabelSetText: Sets the text. '\n' starts a new line.
func LabelSetText(label ID, text string) {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	C.Rlabel_set_text(C.Rid(label), text_)
}

func LabelGetText(label ID) string {
	return C.GoString(C.Rlabel_get_text(C.Rid(label)))
}

// LabelSetFont: R_INVALID_ID uses the theme font.
func LabelSetFont(label ID, font ID) {
	C.Rlabel_set_font(C.Rid(label), C.Rid(font))
}

// LabelSetTextSize: Font size in logical units. 0 uses R_METRIC_FONT_SIZE.
func LabelSetTextSize(label ID, size float32) {
	C.Rlabel_set_text_size(C.Rid(label), C.float(size))
}

func LabelSetTextAlign(label ID, align RTextAlign) {
	C.Rlabel_set_text_align(C.Rid(label), C.RTextAlign(align))
}

// LabelSetWrap: R_TRUE wraps the text on the width given by the parent.
func LabelSetWrap(label ID, wrap int) {
	C.Rlabel_set_wrap(C.Rid(label), C.int(wrap))
}

// LabelSetColor: Text color (R_COLOR_TEXT of this widget).
func LabelSetColor(label ID, r float32, g float32, b float32, a float32) {
	C.Rlabel_set_color(C.Rid(label), C.float(r), C.float(g), C.float(b), C.float(a))
}

// LabelSetMarkdown: R_TRUE reads the text as Markdown and shows it formatted (Markdown="true" in XML): #
// titles, **strong**, *emphasis*, `code`, ~~struck~~, [links](url), lists and lists in lists, - [x] task
// lists, > quotes, --- rules, | tables |, images alone on their line, and ``` blocks of code with the colors
// of their language (```c, ```python...: the languages of the text editor, raven_text_editor.h). The text
// wraps on the width of the label unless Rlabel_set_wrap said otherwise; put the label in a scroll box for a
// long document. Strong and emphasized text use the bold and italic of the font (Rfont_set_variant), which
// Raven derives from the font itself when it was given none. The colors come from the theme: R_COLOR_LINK,
// R_COLOR_CODE, R_COLOR_SEPARATOR, R_COLOR_ACCENT.
func LabelSetMarkdown(label ID, enabled int) {
	C.Rlabel_set_markdown(C.Rid(label), C.int(enabled))
}

func LabelGetMarkdown(label ID) int {
	return int(C.Rlabel_get_markdown(C.Rid(label)))
}

// LabelGetPlainText: The text of a Markdown label without its marks, as a reader sees it. Valid until the
// next call.
func LabelGetPlainText(label ID) string {
	return C.GoString(C.Rlabel_get_plain_text(C.Rid(label)))
}

// ButtonCreate: Shortcut for Rwidget_create(R_WIDGET_BUTTON) + Rbutton_set_text.
func ButtonCreate(text string) ID {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	return ID(C.Rbutton_create(text_))
}

// ButtonSetText: Sets the content of the button to a label. A button can instead hold any single child widget
// (Rwidget_add_child), e.g. an HBox with an icon.
func ButtonSetText(button ID, text string) {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	C.Rbutton_set_text(C.Rid(button), text_)
}

func ButtonSetStyle(button ID, style RButtonStyle) {
	C.Rbutton_set_style(C.Rid(button), C.RButtonStyle(style))
}

func CheckboxCreate(text string) ID {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	return ID(C.Rcheckbox_create(text_))
}

func CheckboxSetText(checkbox ID, text string) {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	C.Rcheckbox_set_text(C.Rid(checkbox), text_)
}

func CheckboxGetText(checkbox ID) string {
	return C.GoString(C.Rcheckbox_get_text(C.Rid(checkbox)))
}

func CheckboxSetChecked(checkbox ID, checked int) {
	C.Rcheckbox_set_checked(C.Rid(checkbox), C.int(checked))
}

func CheckboxIsChecked(checkbox ID) int {
	return int(C.Rcheckbox_is_checked(C.Rid(checkbox)))
}

func CheckboxSetStyle(checkbox ID, style RCheckboxStyle) {
	C.Rcheckbox_set_style(C.Rid(checkbox), C.RCheckboxStyle(style))
}

func SliderSetRange(slider ID, minimum float32, maximum float32) {
	C.Rslider_set_range(C.Rid(slider), C.float(minimum), C.float(maximum))
}

func SliderSetValue(slider ID, value float32) {
	C.Rslider_set_value(C.Rid(slider), C.float(value))
}

func SliderGetValue(slider ID) float32 {
	return float32(C.Rslider_get_value(C.Rid(slider)))
}

// SliderGetRange: The range given with Rslider_set_range and the step (NULL for what is not wanted).
func SliderGetRange(slider ID) (minimum float32, maximum float32, step float32) {
	var minimum_ C.float
	var maximum_ C.float
	var step_ C.float
	C.Rslider_get_range(C.Rid(slider), &minimum_, &maximum_, &step_)
	minimum = float32(minimum_)
	maximum = float32(maximum_)
	step = float32(step_)
	return
}

// SliderSetStep: Snaps the value to multiples of `step`. 0 = continuous (default).
func SliderSetStep(slider ID, step float32) {
	C.Rslider_set_step(C.Rid(slider), C.float(step))
}

func SliderSetOrientation(slider ID, orientation ROrientation) {
	C.Rslider_set_orientation(C.Rid(slider), C.ROrientation(orientation))
}

// ProgressBarSetValue: Progress between 0 and 1.
func ProgressBarSetValue(progress_bar ID, value float32) {
	C.Rprogress_bar_set_value(C.Rid(progress_bar), C.float(value))
}

func ProgressBarGetValue(progress_bar ID) float32 {
	return float32(C.Rprogress_bar_get_value(C.Rid(progress_bar)))
}

func ImageSetTexture(image ID, texture ID) {
	C.Rimage_set_texture(C.Rid(image), C.Rid(texture))
}

func ImageSetTintColor(image ID, r float32, g float32, b float32, a float32) {
	C.Rimage_set_tint_color(C.Rid(image), C.float(r), C.float(g), C.float(b), C.float(a))
}

// ImageSetTintText: R_TRUE: the image is tinted with the text color around it (white icons that follow the
// theme). XML: TintText.
func ImageSetTintText(image ID, enabled int) {
	C.Rimage_set_tint_text(C.Rid(image), C.int(enabled))
}

func ImageSetStretch(image ID, stretch RStretch) {
	C.Rimage_set_stretch(C.Rid(image), C.RStretch(stretch))
}

// ImageSetRegion: Part of the texture to show, in UV coordinates. Default: 0, 0, 1, 1.
func ImageSetRegion(image ID, min_u float32, min_v float32, max_u float32, max_v float32) {
	C.Rimage_set_region(C.Rid(image), C.float(min_u), C.float(min_v), C.float(max_u), C.float(max_v))
}

func ImageSetCornerRadius(image ID, radius float32) {
	C.Rimage_set_corner_radius(C.Rid(image), C.float(radius))
}

func TextInputSetText(text_input ID, text string) {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	C.Rtext_input_set_text(C.Rid(text_input), text_)
}

func TextInputGetText(text_input ID) string {
	return C.GoString(C.Rtext_input_get_text(C.Rid(text_input)))
}

// TextInputSetPlaceholder: Dimmed text shown while the input is empty.
func TextInputSetPlaceholder(text_input ID, text string) {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	C.Rtext_input_set_placeholder(C.Rid(text_input), text_)
}

func TextInputSetPassword(text_input ID, password int) {
	C.Rtext_input_set_password(C.Rid(text_input), C.int(password))
}

func TextInputIsPassword(text_input ID) int {
	return int(C.Rtext_input_is_password(C.Rid(text_input)))
}

func TextInputIsReadOnly(text_input ID) int {
	return int(C.Rtext_input_is_read_only(C.Rid(text_input)))
}

func TextInputSetReadOnly(text_input ID, read_only int) {
	C.Rtext_input_set_read_only(C.Rid(text_input), C.int(read_only))
}

// TextInputSetMaxLength: Maximum number of characters. 0 = no limit (default).
func TextInputSetMaxLength(text_input ID, max_length int) {
	C.Rtext_input_set_max_length(C.Rid(text_input), C.int(max_length))
}

func TextInputSelectAll(text_input ID) {
	C.Rtext_input_select_all(C.Rid(text_input))
}

func SpinBoxSetRange(spin_box ID, minimum float64, maximum float64) {
	C.Rspin_box_set_range(C.Rid(spin_box), C.double(minimum), C.double(maximum))
}

func SpinBoxSetValue(spin_box ID, value float64) {
	C.Rspin_box_set_value(C.Rid(spin_box), C.double(value))
}

func SpinBoxGetValue(spin_box ID) float64 {
	return float64(C.Rspin_box_get_value(C.Rid(spin_box)))
}

// SpinBoxGetRange: The range and the step (NULL for what is not wanted).
func SpinBoxGetRange(spin_box ID) (minimum float64, maximum float64, step float64) {
	var minimum_ C.double
	var maximum_ C.double
	var step_ C.double
	C.Rspin_box_get_range(C.Rid(spin_box), &minimum_, &maximum_, &step_)
	minimum = float64(minimum_)
	maximum = float64(maximum_)
	step = float64(step_)
	return
}

func SpinBoxSetStep(spin_box ID, step float64) {
	C.Rspin_box_set_step(C.Rid(spin_box), C.double(step))
}

// SpinBoxSetDecimals: Digits shown after the decimal point. Default: 2.
func SpinBoxSetDecimals(spin_box ID, decimals int) {
	C.Rspin_box_set_decimals(C.Rid(spin_box), C.int(decimals))
}

// ComboBoxAddItem: Returns: The index of the new item.
func ComboBoxAddItem(combo_box ID, text string) int {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	return int(C.Rcombo_box_add_item(C.Rid(combo_box), text_))
}

func ComboBoxClearItems(combo_box ID) {
	C.Rcombo_box_clear_items(C.Rid(combo_box))
}

func ComboBoxGetItemCount(combo_box ID) int {
	return int(C.Rcombo_box_get_item_count(C.Rid(combo_box)))
}

func ComboBoxGetItemText(combo_box ID, index int) string {
	return C.GoString(C.Rcombo_box_get_item_text(C.Rid(combo_box), C.int(index)))
}

// ComboBoxSetSelected: Selects an item. -1 selects nothing.
func ComboBoxSetSelected(combo_box ID, index int) {
	C.Rcombo_box_set_selected(C.Rid(combo_box), C.int(index))
}

func ComboBoxGetSelected(combo_box ID) int {
	return int(C.Rcombo_box_get_selected(C.Rid(combo_box)))
}

// SeparatorSetOrientation: R_HORIZONTAL draws a horizontal line (default).
func SeparatorSetOrientation(separator ID, orientation ROrientation) {
	C.Rseparator_set_orientation(C.Rid(separator), C.ROrientation(orientation))
}

// ListViewAddItem: Returns: The index of the new item.
func ListViewAddItem(list_view ID, text string) int {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	return int(C.Rlist_view_add_item(C.Rid(list_view), text_))
}

func ListViewSetItemText(list_view ID, index int, text string) {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	C.Rlist_view_set_item_text(C.Rid(list_view), C.int(index), text_)
}

func ListViewGetItemText(list_view ID, index int) string {
	return C.GoString(C.Rlist_view_get_item_text(C.Rid(list_view), C.int(index)))
}

func ListViewRemoveItem(list_view ID, index int) {
	C.Rlist_view_remove_item(C.Rid(list_view), C.int(index))
}

func ListViewClearItems(list_view ID) {
	C.Rlist_view_clear_items(C.Rid(list_view))
}

func ListViewGetItemCount(list_view ID) int {
	return int(C.Rlist_view_get_item_count(C.Rid(list_view)))
}

// ListViewSetSelected: Selects a row. -1 selects nothing.
func ListViewSetSelected(list_view ID, index int) {
	C.Rlist_view_set_selected(C.Rid(list_view), C.int(index))
}

func ListViewGetSelected(list_view ID) int {
	return int(C.Rlist_view_get_selected(C.Rid(list_view)))
}

func ListViewScrollToItem(list_view ID, index int) {
	C.Rlist_view_scroll_to_item(C.Rid(list_view), C.int(index))
}

// TreeViewAddItem: Adds an item. parent_item Item to add under, or R_INVALID_ID for a top-level item.
// Returns: The handle of the new item.
func TreeViewAddItem(tree_view ID, parent_item ID, text string) ID {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	return ID(C.Rtree_view_add_item(C.Rid(tree_view), C.Rid(parent_item), text_))
}

// TreeItemDestroy: Deletes an item and its descendants.
func TreeItemDestroy(item ID) {
	C.Rtree_item_destroy(C.Rid(item))
}

func TreeViewClearItems(tree_view ID) {
	C.Rtree_view_clear_items(C.Rid(tree_view))
}

func TreeItemIsValid(item ID) int {
	return int(C.Rtree_item_is_valid(C.Rid(item)))
}

func TreeItemSetText(item ID, text string) {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	C.Rtree_item_set_text(C.Rid(item), text_)
}

func TreeItemGetText(item ID) string {
	return C.GoString(C.Rtree_item_get_text(C.Rid(item)))
}

func TreeItemSetExpanded(item ID, expanded int) {
	C.Rtree_item_set_expanded(C.Rid(item), C.int(expanded))
}

func TreeItemIsExpanded(item ID) int {
	return int(C.Rtree_item_is_expanded(C.Rid(item)))
}

// TreeViewGetItemCount: Top-level items of a tree.
func TreeViewGetItemCount(tree_view ID) int {
	return int(C.Rtree_view_get_item_count(C.Rid(tree_view)))
}

func TreeViewGetItem(tree_view ID, index int) ID {
	return ID(C.Rtree_view_get_item(C.Rid(tree_view), C.int(index)))
}

// TreeItemGetParent: Returns: The parent item, or R_INVALID_ID for a top-level item.
func TreeItemGetParent(item ID) ID {
	return ID(C.Rtree_item_get_parent(C.Rid(item)))
}

func TreeItemGetChildCount(item ID) int {
	return int(C.Rtree_item_get_child_count(C.Rid(item)))
}

func TreeItemGetChild(item ID, index int) ID {
	return ID(C.Rtree_item_get_child(C.Rid(item), C.int(index)))
}

// TreeViewSetSelectedItem: Selects an item. R_INVALID_ID selects nothing.
func TreeViewSetSelectedItem(tree_view ID, item ID) {
	C.Rtree_view_set_selected_item(C.Rid(tree_view), C.Rid(item))
}

func TreeViewGetSelectedItem(tree_view ID) ID {
	return ID(C.Rtree_view_get_selected_item(C.Rid(tree_view)))
}

// TreeItemSetIcon: Icon drawn before the text of the item (a region u0,v0,u1,v1 of the texture). R_INVALID_ID
// removes it.
func TreeItemSetIcon(item ID, texture ID, u0 float32, v0 float32, u1 float32, v1 float32) {
	C.Rtree_item_set_icon(C.Rid(item), C.Rid(texture), C.float(u0), C.float(v0), C.float(u1), C.float(v1))
}

// TreeItemSetTextColor: Color of the text of the item; alpha 0 goes back to the theme.
func TreeItemSetTextColor(item ID, r float32, g float32, b float32, a float32) {
	C.Rtree_item_set_text_color(C.Rid(item), C.float(r), C.float(g), C.float(b), C.float(a))
}

// TreeViewSetMultiSelect: Several items can be selected: Ctrl + click adds or removes one, Shift + click
// selects a range. The selection callback receives the item clicked last (the primary one,
// Rtree_view_get_selected_item).
func TreeViewSetMultiSelect(tree_view ID, enabled int) {
	C.Rtree_view_set_multi_select(C.Rid(tree_view), C.int(enabled))
}

func TreeItemSetSelected(item ID, selected int) {
	C.Rtree_item_set_selected(C.Rid(item), C.int(selected))
}

func TreeItemIsSelected(item ID) int {
	return int(C.Rtree_item_is_selected(C.Rid(item)))
}

func TreeViewClearSelection(tree_view ID) {
	C.Rtree_view_clear_selection(C.Rid(tree_view))
}

// TreeViewGetSelectionCount: Selected items, top to bottom.
func TreeViewGetSelectionCount(tree_view ID) int {
	return int(C.Rtree_view_get_selection_count(C.Rid(tree_view)))
}

func TreeViewGetSelection(tree_view ID, index int) ID {
	return ID(C.Rtree_view_get_selection(C.Rid(tree_view), C.int(index)))
}

func TreeViewScrollToItem(tree_view ID, item ID) {
	C.Rtree_view_scroll_to_item(C.Rid(tree_view), C.Rid(item))
}

// TreeViewSetDragType: Items can be dragged: the selection is given to Rdrag_begin with this type and, as
// data, the handles of the items in decimal separated by spaces. NULL: no drag (default).
func TreeViewSetDragType(tree_view ID, type_ string) {
	type__ := C.CString(type_)
	defer C.free(unsafe.Pointer(type__))
	C.Rtree_view_set_drag_type(C.Rid(tree_view), type__)
}

// TreeViewGetItemAt: Item at a point relative to the tree (drop callbacks give local_x, local_y), or
// R_INVALID_ID.
func TreeViewGetItemAt(tree_view ID, local_x float32, local_y float32) ID {
	return ID(C.Rtree_view_get_item_at(C.Rid(tree_view), C.float(local_x), C.float(local_y)))
}

// TreeViewSetDropHighlight: Highlights where a drop would go: an item, or the whole tree (`whole_tree`).
// R_INVALID_ID, 0 removes it.
func TreeViewSetDropHighlight(tree_view ID, item ID, whole_tree int) {
	C.Rtree_view_set_drop_highlight(C.Rid(tree_view), C.Rid(item), C.int(whole_tree))
}

// MenuAddMenu: Creates a menu inside a menu bar or as a sub-menu of another menu.
func MenuAddMenu(menu_bar_or_menu ID, title string) ID {
	title_ := C.CString(title)
	defer C.free(unsafe.Pointer(title_))
	return ID(C.Rmenu_add_menu(C.Rid(menu_bar_or_menu), title_))
}

func MenuSetTitle(menu ID, title string) {
	title_ := C.CString(title)
	defer C.free(unsafe.Pointer(title_))
	C.Rmenu_set_title(C.Rid(menu), title_)
}

func MenuAddItem(menu ID, text string) ID {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	return ID(C.Rmenu_add_item(C.Rid(menu), text_))
}

func MenuAddSeparator(menu ID) ID {
	return ID(C.Rmenu_add_separator(C.Rid(menu)))
}

func MenuItemSetText(item ID, text string) {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	C.Rmenu_item_set_text(C.Rid(item), text_)
}

func MenuItemGetText(item ID) string {
	return C.GoString(C.Rmenu_item_get_text(C.Rid(item)))
}

// MenuItemSetShortcut: Shows a shortcut next to the item and triggers the item when the keys are pressed in
// the window of its menu bar.
func MenuItemSetShortcut(item ID, key RKey, modifiers uint32) {
	C.Rmenu_item_set_shortcut(C.Rid(item), C.RKey(key), C.Ruint(modifiers))
}

func MenuItemSetCheckable(item ID, checkable int) {
	C.Rmenu_item_set_checkable(C.Rid(item), C.int(checkable))
}

func MenuItemSetChecked(item ID, checked int) {
	C.Rmenu_item_set_checked(C.Rid(item), C.int(checked))
}

// MenuItemSetIcon: Icon drawn before the text of a menu item (after the check mark column). texture
// R_INVALID_ID removes it. u0, v0, u1, v1 : region of the texture. tint R_TRUE: the texture is a mask drawn
// in the text color (monochrome icons).
func MenuItemSetIcon(item ID, texture ID, u0 float32, v0 float32, u1 float32, v1 float32, tint int) {
	C.Rmenu_item_set_icon(C.Rid(item), C.Rid(texture), C.float(u0), C.float(v0), C.float(u1), C.float(v1), C.int(tint))
}

func MenuItemIsChecked(item ID) int {
	return int(C.Rmenu_item_is_checked(C.Rid(item)))
}

// MenuOpen: Opens a menu as a popup at a position of the window (logical units).
func MenuOpen(menu ID, window ID, x float32, y float32) {
	C.Rmenu_open(C.Rid(menu), C.Rid(window), C.float(x), C.float(y))
}

// WindowClosePopups: Closes every open menu and popup of the window.
func WindowClosePopups(window ID) {
	C.Rwindow_close_popups(C.Rid(window))
}

// TileViewRefresh: Draws the tiles again (their description changed, not their number).
func TileViewRefresh(tile_view ID) {
	C.Rtile_view_refresh(C.Rid(tile_view))
}

func TileViewGetCount(tile_view ID) int {
	return int(C.Rtile_view_get_count(C.Rid(tile_view)))
}

// TileViewSetTileSize: Size of a tile (its name included) and of its icon. Default 112 x 112, icon 64.
func TileViewSetTileSize(tile_view ID, width float32, height float32, icon_size float32) {
	C.Rtile_view_set_tile_size(C.Rid(tile_view), C.float(width), C.float(height), C.float(icon_size))
}

func TileViewSetEmptyText(tile_view ID, text string) {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	C.Rtile_view_set_empty_text(C.Rid(tile_view), text_)
}

func TileViewSetSelected(tile_view ID, index int, selected int) {
	C.Rtile_view_set_selected(C.Rid(tile_view), C.int(index), C.int(selected))
}

func TileViewIsSelected(tile_view ID, index int) int {
	return int(C.Rtile_view_is_selected(C.Rid(tile_view), C.int(index)))
}

func TileViewClearSelection(tile_view ID) {
	C.Rtile_view_clear_selection(C.Rid(tile_view))
}

func TileViewSelectOnly(tile_view ID, index int) {
	C.Rtile_view_select_only(C.Rid(tile_view), C.int(index))
}

// TileViewGetCurrent: The tile clicked last, or -1.
func TileViewGetCurrent(tile_view ID) int {
	return int(C.Rtile_view_get_current(C.Rid(tile_view)))
}

func TileViewGetSelectionCount(tile_view ID) int {
	return int(C.Rtile_view_get_selection_count(C.Rid(tile_view)))
}

// TileViewGetSelection: Index of the nth selected tile, in order.
func TileViewGetSelection(tile_view ID, nth int) int {
	return int(C.Rtile_view_get_selection(C.Rid(tile_view), C.int(nth)))
}

func TileViewScrollTo(tile_view ID, index int) {
	C.Rtile_view_scroll_to(C.Rid(tile_view), C.int(index))
}

// TileViewGetItemAt: Tile at a point relative to the widget (drop callbacks give local_x, local_y), or -1.
func TileViewGetItemAt(tile_view ID, local_x float32, local_y float32) int {
	return int(C.Rtile_view_get_item_at(C.Rid(tile_view), C.float(local_x), C.float(local_y)))
}

// TileViewSetDropHighlight: Highlights the tile a drop would go to; -1 removes it.
func TileViewSetDropHighlight(tile_view ID, index int) {
	C.Rtile_view_set_drop_highlight(C.Rid(tile_view), C.int(index))
}

// TileViewSetDragType: Tiles can be dragged: the selected tiles are given to Rdrag_begin with this type and,
// as data, their indices separated by spaces.
func TileViewSetDragType(tile_view ID, type_ string) {
	type__ := C.CString(type_)
	defer C.free(unsafe.Pointer(type__))
	C.Rtile_view_set_drag_type(C.Rid(tile_view), type__)
}

// TextEditorCreate: text Initial content, or NULL.
func TextEditorCreate(text string) ID {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	return ID(C.Rtext_editor_create(text_))
}

// TextEditorSetText: Replaces the whole text. The history of undo is cleared; the changed callback is not
// called.
func TextEditorSetText(editor ID, text string) {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	C.Rtext_editor_set_text(C.Rid(editor), text_)
}

// TextEditorGetText: Returns: The whole text, lines separated by '\n'. Valid until the next call or the next
// edit.
func TextEditorGetText(editor ID) string {
	return C.GoString(C.Rtext_editor_get_text(C.Rid(editor)))
}

// TextEditorGetLength: Returns: The number of bytes of Rtext_editor_get_text.
func TextEditorGetLength(editor ID) int {
	return int(C.Rtext_editor_get_length(C.Rid(editor)))
}

func TextEditorGetLineCount(editor ID) int {
	return int(C.Rtext_editor_get_line_count(C.Rid(editor)))
}

// TextEditorGetLine: Returns: One line, without its line break. Valid until the next edit. NULL when `line`
// is out of range.
func TextEditorGetLine(editor ID, line int) string {
	return C.GoString(C.Rtext_editor_get_line(C.Rid(editor), C.int(line)))
}

// TextEditorInsertText: Types a text at the cursor, replacing the selection, as the user would: it can be
// undone and the changed callback is called.
func TextEditorInsertText(editor ID, text string) {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	C.Rtext_editor_insert_text(C.Rid(editor), text_)
}

func TextEditorSetCursor(editor ID, line int, column int) {
	C.Rtext_editor_set_cursor(C.Rid(editor), C.int(line), C.int(column))
}

func TextEditorGetCursor(editor ID) (line int, column int) {
	var line_ C.int
	var column_ C.int
	C.Rtext_editor_get_cursor(C.Rid(editor), &line_, &column_)
	line = int(line_)
	column = int(column_)
	return
}

// TextEditorSelect: Selects from one position to another; the cursor is on the second.
func TextEditorSelect(editor ID, from_line int, from_column int, to_line int, to_column int) {
	C.Rtext_editor_select(C.Rid(editor), C.int(from_line), C.int(from_column), C.int(to_line), C.int(to_column))
}

func TextEditorSelectAll(editor ID) {
	C.Rtext_editor_select_all(C.Rid(editor))
}

// TextEditorGetSelection: Start and end of the selection, in the order of the text. Returns R_FALSE when
// nothing is selected.
func TextEditorGetSelection(editor ID) (result int, from_line int, from_column int, to_line int, to_column int) {
	var from_line_ C.int
	var from_column_ C.int
	var to_line_ C.int
	var to_column_ C.int
	result = int(C.Rtext_editor_get_selection(C.Rid(editor), &from_line_, &from_column_, &to_line_, &to_column_))
	from_line = int(from_line_)
	from_column = int(from_column_)
	to_line = int(to_line_)
	to_column = int(to_column_)
	return
}

// TextEditorGetSelectedText: Returns: The selected text, "" when there is none. Valid until the next call or
// the next edit.
func TextEditorGetSelectedText(editor ID) string {
	return C.GoString(C.Rtext_editor_get_selected_text(C.Rid(editor)))
}

// TextEditorScrollToLine: Scrolls so that a line is visible.
func TextEditorScrollToLine(editor ID, line int) {
	C.Rtext_editor_scroll_to_line(C.Rid(editor), C.int(line))
}

func TextEditorUndo(editor ID) {
	C.Rtext_editor_undo(C.Rid(editor))
}

func TextEditorRedo(editor ID) {
	C.Rtext_editor_redo(C.Rid(editor))
}

func TextEditorCanUndo(editor ID) int {
	return int(C.Rtext_editor_can_undo(C.Rid(editor)))
}

func TextEditorCanRedo(editor ID) int {
	return int(C.Rtext_editor_can_redo(C.Rid(editor)))
}

// TextEditorSetReadOnly: A read-only editor can still be scrolled, selected and copied.
func TextEditorSetReadOnly(editor ID, read_only int) {
	C.Rtext_editor_set_read_only(C.Rid(editor), C.int(read_only))
}

// TextEditorSetWordWrap: Long lines continue on the next row instead of scrolling sideways. Off by default.
func TextEditorSetWordWrap(editor ID, enabled int) {
	C.Rtext_editor_set_word_wrap(C.Rid(editor), C.int(enabled))
}

// TextEditorSetLineNumbers: Shows the line numbers in a margin. Off by default.
func TextEditorSetLineNumbers(editor ID, enabled int) {
	C.Rtext_editor_set_line_numbers(C.Rid(editor), C.int(enabled))
}

// TextEditorSetTabSize: Width of a tab, in spaces (4 by default).
func TextEditorSetTabSize(editor ID, spaces int) {
	C.Rtext_editor_set_tab_size(C.Rid(editor), C.int(spaces))
}

// TextEditorSetTabInsertsSpaces: R_TRUE makes the Tab key insert spaces instead of a tab character. Off by
// default.
func TextEditorSetTabInsertsSpaces(editor ID, enabled int) {
	C.Rtext_editor_set_tab_inserts_spaces(C.Rid(editor), C.int(enabled))
}

// TextEditorSetSyntax: The same as Rtext_editor_set_language with "XML", "C" or NULL.
func TextEditorSetSyntax(editor ID, syntax RSyntax) {
	C.Rtext_editor_set_syntax(C.Rid(editor), C.RSyntax(syntax))
}

// TextEditorSetFont: font R_INVALID_ID uses the font of the theme (or the fixed-width font, with a language).
// size 0 uses the font size of the theme.
func TextEditorSetFont(editor ID, font ID, size float32) {
	C.Rtext_editor_set_font(C.Rid(editor), C.Rid(font), C.float(size))
}

// TextEditorSetPlaceholder: Text shown, dimmed, while the editor is empty.
func TextEditorSetPlaceholder(editor ID, text string) {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	C.Rtext_editor_set_placeholder(C.Rid(editor), text_)
}

// TextEditorSetZoom: Zoom of the text: a factor of its font size, 1 = 100 % (0.5 .. 4). The user zooms with
// Ctrl + wheel (around the pointer), Ctrl + '+', Ctrl + '-' and Ctrl + 0 (back to 100 %), keypad included.
// Set by the program, the top of the view stays and the zoom callback is not called. XML: Zoom="1.25".
func TextEditorSetZoom(editor ID, zoom float32) {
	C.Rtext_editor_set_zoom(C.Rid(editor), C.float(zoom))
}

func TextEditorGetZoom(editor ID) float32 {
	return float32(C.Rtext_editor_get_zoom(C.Rid(editor)))
}

// TextEditorSetZoomable: R_FALSE: Ctrl + wheel and the keys do not zoom (the wheel scrolls). On by default.
// XML: Zoomable="false".
func TextEditorSetZoomable(editor ID, zoomable int) {
	C.Rtext_editor_set_zoomable(C.Rid(editor), C.int(zoomable))
}

func LanguageGetCount() int {
	return int(C.Rlanguage_get_count())
}

func LanguageGetName(index int) string {
	return C.GoString(C.Rlanguage_get_name(C.int(index)))
}

// LanguageForFile: Returns: The name of the language of a file, from its extension, or NULL.
func LanguageForFile(path string) string {
	path_ := C.CString(path)
	defer C.free(unsafe.Pointer(path_))
	return C.GoString(C.Rlanguage_for_file(path_))
}

// TextEditorSetLanguage: Gives the editor a language, by name ("C++", "Python"...), by another usual name
// ("cpp", "js") or by the extension of its files ("rs"). NULL or "" removes it. An editor with a language
// uses the fixed-width font unless it was given a font.
func TextEditorSetLanguage(editor ID, name string) {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	C.Rtext_editor_set_language(C.Rid(editor), name_)
}

// TextEditorGetLanguage: Returns: The name of the language of the editor, or "".
func TextEditorGetLanguage(editor ID) string {
	return C.GoString(C.Rtext_editor_get_language(C.Rid(editor)))
}

// TextEditorSetMarkdown: R_TRUE makes the editor an editor of Markdown (the "Markdown" language;
// Markdown="true" in XML): the text stays as it is written, and what it formats shows while typing, strong
// text in bold, emphasized text in italic, titles in bold, code, links and marks in their colors. A label
// with Rlabel_set_markdown shows the same text formatted. R_FALSE removes the language.
func TextEditorSetMarkdown(editor ID, enabled int) {
	C.Rtext_editor_set_markdown(C.Rid(editor), C.int(enabled))
}

func TextEditorGetMarkdown(editor ID) int {
	return int(C.Rtext_editor_get_markdown(C.Rid(editor)))
}

// TextEditorGetToken: What the language of the editor makes of the character at a position: the colors come
// from it. R_TOKEN_TEXT without a language.
func TextEditorGetToken(editor ID, line int, column int) RToken {
	return RToken(C.Rtext_editor_get_token(C.Rid(editor), C.int(line), C.int(column)))
}

// TextEditorSetCompletion: R_FALSE stops the editor from proposing words by itself; Ctrl+Space still does. On
// by default.
func TextEditorSetCompletion(editor ID, enabled int) {
	C.Rtext_editor_set_completion(C.Rid(editor), C.int(enabled))
}

// TextEditorShowCompletions: Proposes words for what is before the cursor, as Ctrl+Space does.
func TextEditorShowCompletions(editor ID) {
	C.Rtext_editor_show_completions(C.Rid(editor))
}

func TextEditorIsCompleting(editor ID) int {
	return int(C.Rtext_editor_is_completing(C.Rid(editor)))
}

// TextEditorAddCompletion: Adds a proposal. Only from a completion callback. Proposals that do not start with
// the prefix are left out; the others keep the order they were added in. detail A few words shown beside it
// ("function", "int"), or NULL.
func TextEditorAddCompletion(editor ID, text string, detail string) {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	detail_ := C.CString(detail)
	defer C.free(unsafe.Pointer(detail_))
	C.Rtext_editor_add_completion(C.Rid(editor), text_, detail_)
}

// TextEditorSetAutoPairs: R_FALSE stops the editor from closing brackets and quotes and from indenting after
// them. On by default; it only acts with a language.
func TextEditorSetAutoPairs(editor ID, enabled int) {
	C.Rtext_editor_set_auto_pairs(C.Rid(editor), C.int(enabled))
}

// TextEditorAddMarker: Underlines a part of a line and attaches a message to it, shown when the pointer rests
// there: an error of a compiler, a warning. Markers follow their line when text is added or removed above it,
// and go away when their own line is edited. to_column -1 for the end of the line.
func TextEditorAddMarker(editor ID, line int, from_column int, to_column int, kind RMarkerKind, message string) {
	message_ := C.CString(message)
	defer C.free(unsafe.Pointer(message_))
	C.Rtext_editor_add_marker(C.Rid(editor), C.int(line), C.int(from_column), C.int(to_column), C.RMarkerKind(kind), message_)
}

func TextEditorClearMarkers(editor ID) {
	C.Rtext_editor_clear_markers(C.Rid(editor))
}

func TextEditorGetMarkerCount(editor ID) int {
	return int(C.Rtext_editor_get_marker_count(C.Rid(editor)))
}

// ThemeSetTokenColor: Color of a kind of token in the theme in use, instead of the one Raven picks for dark
// and for light themes. Theme files set them with <Syntax Keyword="#FF7AB2" String="..."/>.
func ThemeSetTokenColor(token RToken, r float32, g float32, b float32, a float32) {
	C.Rtheme_set_token_color(C.RToken(token), C.float(r), C.float(g), C.float(b), C.float(a))
}

func ThemeGetTokenColor(token RToken) (r float32, g float32, b float32, a float32) {
	var r_ C.float
	var g_ C.float
	var b_ C.float
	var a_ C.float
	C.Rtheme_get_token_color(C.RToken(token), &r_, &g_, &b_, &a_)
	r = float32(r_)
	g = float32(g_)
	b = float32(b_)
	a = float32(a_)
	return
}

// ThemeClearTokenColors: Back to the colors Raven picks.
func ThemeClearTokenColors() {
	C.Rtheme_clear_token_colors()
}

// HeaderBarCreate: title Text shown in the bar; NULL shows the title of the window.
func HeaderBarCreate(title string) ID {
	title_ := C.CString(title)
	defer C.free(unsafe.Pointer(title_))
	return ID(C.Rheader_bar_create(title_))
}

// HeaderBarSetTitle: title NULL shows the title of the window.
func HeaderBarSetTitle(bar ID, title string) {
	title_ := C.CString(title)
	defer C.free(unsafe.Pointer(title_))
	C.Rheader_bar_set_title(C.Rid(bar), title_)
}

// HeaderBarSetButtons: buttons Bitmask of RHeaderButtons. All by default; the maximize button only shows on a
// resizable window.
func HeaderBarSetButtons(bar ID, buttons uint32) {
	C.Rheader_bar_set_buttons(C.Rid(bar), C.Ruint(buttons))
}

// HeaderBarSetButtonSide: side R_HALIGN_RIGHT (default): minimize, maximize, close, as on Windows.
// R_HALIGN_LEFT: three round colored buttons, as on macOS.
func HeaderBarSetButtonSide(bar ID, side RHAlign) {
	C.Rheader_bar_set_button_side(C.Rid(bar), C.RHAlign(side))
}

// HeaderBarSetTitleAlign: align R_HALIGN_CENTER (default) or R_HALIGN_LEFT.
func HeaderBarSetTitleAlign(bar ID, align RHAlign) {
	C.Rheader_bar_set_title_align(C.Rid(bar), C.RHAlign(align))
}

// HeaderBarSetTitleNotch: R_TRUE: the title sits in a dark tab hanging from the top edge of the bar (slanted
// sides, round bottom corners). XML: TitleStyle="Notch".
func HeaderBarSetTitleNotch(bar ID, notch int) {
	C.Rheader_bar_set_title_notch(C.Rid(bar), C.int(notch))
}

// HeaderBarSetBackground: R_FALSE removes the background of the bar: for a header that floats above the
// content (in an Overlay) or sits inside a glass panel.
func HeaderBarSetBackground(bar ID, enabled int) {
	C.Rheader_bar_set_background(C.Rid(bar), C.int(enabled))
}

func DockAreaCreate() ID {
	return ID(C.Rdock_area_create())
}

// DockAreaAddPanel: Adds a panel. The dock area takes ownership of the widget. title Text of its tab. side
// Where it goes. relative A panel of the area to split or to share tabs with, or R_INVALID_ID for the whole
// area (with R_DOCK_TAB: the first group of tabs). size Part of the split space given to the new panel, 0..1
// (0 = a half, or a quarter along the whole area).
func DockAreaAddPanel(area ID, panel ID, title string, side RDockSide, relative ID, size float32) {
	title_ := C.CString(title)
	defer C.free(unsafe.Pointer(title_))
	C.Rdock_area_add_panel(C.Rid(area), C.Rid(panel), title_, C.RDockSide(side), C.Rid(relative), C.float(size))
}

// DockAreaMovePanel: Moves a panel of the area somewhere else in it.
func DockAreaMovePanel(area ID, panel ID, side RDockSide, relative ID, size float32) {
	C.Rdock_area_move_panel(C.Rid(area), C.Rid(panel), C.RDockSide(side), C.Rid(relative), C.float(size))
}

// DockAreaClosePanel: Removes a panel and deletes it.
func DockAreaClosePanel(area ID, panel ID) {
	C.Rdock_area_close_panel(C.Rid(area), C.Rid(panel))
}

func DockAreaSetPanelTitle(area ID, panel ID, title string) {
	title_ := C.CString(title)
	defer C.free(unsafe.Pointer(title_))
	C.Rdock_area_set_panel_title(C.Rid(area), C.Rid(panel), title_)
}

// DockAreaSetPanelIcon: Icon drawn before the title of the tab of a panel (and on the chip while it is
// dragged). texture Texture of the icon, R_INVALID_ID removes it. u0,v0,u1,v1 Region of the texture (0, 0, 1,
// 1 : all of it). tint R_TRUE: the texture is a mask drawn in the text color of the tab (monochrome icons).
func DockAreaSetPanelIcon(area ID, panel ID, texture ID, u0 float32, v0 float32, u1 float32, v1 float32, tint int) {
	C.Rdock_area_set_panel_icon(C.Rid(area), C.Rid(panel), C.Rid(texture), C.float(u0), C.float(v0), C.float(u1), C.float(v1), C.int(tint))
}

// DockAreaSetPanelClosable: R_FALSE removes the close button of the tab of a panel. Panels can be closed by
// default.
func DockAreaSetPanelClosable(area ID, panel ID, closable int) {
	C.Rdock_area_set_panel_closable(C.Rid(area), C.Rid(panel), C.int(closable))
}

// DockAreaHidePanel: Takes a panel out of the arrangement without deleting it (a window of an application
// closed from its menu). Its tab disappears and its neighbours take its place; the area remembers where it
// was.
func DockAreaHidePanel(area ID, panel ID) {
	C.Rdock_area_hide_panel(C.Rid(area), C.Rid(panel))
}

// DockAreaOpenPanel: Puts a hidden panel back where it was, and brings its tab to the front (also for a panel
// that is not hidden).
func DockAreaOpenPanel(area ID, panel ID) {
	C.Rdock_area_open_panel(C.Rid(area), C.Rid(panel))
}

// DockAreaIsPanelOpen: Returns: R_TRUE when the panel is in the arrangement (not hidden), shown or behind
// another tab.
func DockAreaIsPanelOpen(area ID, panel ID) int {
	return int(C.Rdock_area_is_panel_open(C.Rid(area), C.Rid(panel)))
}

// DockAreaSetCloseHides: R_TRUE: the close button of a tab hides its panel (Rdock_area_hide_panel) instead of
// deleting it. XML: CloseHides="true". The close callback is asked first either way.
func DockAreaSetCloseHides(area ID, hides int) {
	C.Rdock_area_set_close_hides(C.Rid(area), C.int(hides))
}

// DockAreaShowPanel: Brings the tab of a panel to the front.
func DockAreaShowPanel(area ID, panel ID) {
	C.Rdock_area_show_panel(C.Rid(area), C.Rid(panel))
}

// DockAreaIsPanelShown: Returns: R_TRUE when the panel is the one shown in its group of tabs.
func DockAreaIsPanelShown(area ID, panel ID) int {
	return int(C.Rdock_area_is_panel_shown(C.Rid(area), C.Rid(panel)))
}

// DockAreaSetMovable: R_FALSE stops the user from moving the panels (the borders can still be dragged).
func DockAreaSetMovable(area ID, movable int) {
	C.Rdock_area_set_movable(C.Rid(area), C.int(movable))
}

// DockAreaSaveLayout: Describes how the panels are arranged, as a short text to keep in a settings file.
// Panels are named by their widget name (Rwidget_set_name): unnamed panels are not part of the description.
// Returns: A string valid until the next call.
func DockAreaSaveLayout(area ID) string {
	return C.GoString(C.Rdock_area_save_layout(C.Rid(area)))
}

// DockAreaLoadLayout: Arranges the panels as a saved description says. Panels of the area that the
// description does not name keep a place; names it has that match no panel are ignored. Returns: R_FALSE when
// the text is not a layout (nothing is changed).
func DockAreaLoadLayout(area ID, layout string) int {
	layout_ := C.CString(layout)
	defer C.free(unsafe.Pointer(layout_))
	return int(C.Rdock_area_load_layout(C.Rid(area), layout_))
}

// DockAreaSetTearable: Tearing: a tab dragged out of the area takes its panel to a window of its own (a
// floating area of the same family), and a tab dragged from such a window over the area docks it back where
// it is dropped. A double click on the tab of a floating window, or closing that window, puts its panels back
// where they were. Dragging the only tab of a floating window moves the window. R_TRUE by default; XML:
// Tearable="false". The functions of this area that take a panel also work on its panels that are in floating
// windows.
func DockAreaSetTearable(area ID, tearable int) {
	C.Rdock_area_set_tearable(C.Rid(area), C.int(tearable))
}

func DockAreaIsTearable(area ID) int {
	return int(C.Rdock_area_is_tearable(C.Rid(area)))
}

// DockAreaFloatPanel: Takes a panel to a window of its own, its top left corner at (x, y) on the screen.
// Returns the window.
func DockAreaFloatPanel(area ID, panel ID, x int, y int) ID {
	return ID(C.Rdock_area_float_panel(C.Rid(area), C.Rid(panel), C.int(x), C.int(y)))
}

// DockAreaDockPanel: A floating panel goes back where it was in the area.
func DockAreaDockPanel(area ID, panel ID) {
	C.Rdock_area_dock_panel(C.Rid(area), C.Rid(panel))
}

// DockAreaGetPanelWindow: The window a floating panel is in, R_INVALID_ID when it is docked in the area.
func DockAreaGetPanelWindow(area ID, panel ID) ID {
	return ID(C.Rdock_area_get_panel_window(C.Rid(area), C.Rid(panel)))
}

func IslandCreate() ID {
	return ID(C.Risland_create())
}

// IslandSetExpanded: Unfolds or folds the island. The changed callback is not called.
func IslandSetExpanded(island ID, expanded int) {
	C.Risland_set_expanded(C.Rid(island), C.int(expanded))
}

func IslandIsExpanded(island ID) int {
	return int(C.Risland_is_expanded(C.Rid(island)))
}

// IslandExpandFor: Unfolds the island, and folds it back by itself after a while: to tell something to the
// user. It stays open while the pointer is on it.
func IslandExpandFor(island ID, seconds float64) {
	C.Risland_expand_for(C.Rid(island), C.double(seconds))
}

// IslandSetExpandOnHover: R_TRUE (default): the island unfolds under the pointer and folds back when it
// leaves.
func IslandSetExpandOnHover(island ID, enabled int) {
	C.Risland_set_expand_on_hover(C.Rid(island), C.int(enabled))
}

// IslandSetExpandOnClick: R_TRUE (default): a click on the island unfolds or folds it.
func IslandSetExpandOnClick(island ID, enabled int) {
	C.Risland_set_expand_on_click(C.Rid(island), C.int(enabled))
}

// IslandSetAttached: R_TRUE gives the island square upper corners: a notch that hangs from the edge it
// touches, instead of a capsule that floats under it.
func IslandSetAttached(island ID, attached int) {
	C.Risland_set_attached(C.Rid(island), C.int(attached))
}

// WindowSetIsland: Shows a widget (usually an island) floating at the top of a window, in the middle, above
// its content. The window takes ownership; the previous one is deleted. R_INVALID_ID removes it. The padding
// of its slot (Rslot_set_padding) is its distance from the top edge.
func WindowSetIsland(window ID, widget ID) {
	C.Rwindow_set_island(C.Rid(window), C.Rid(widget))
}

func WindowGetIsland(window ID) ID {
	return ID(C.Rwindow_get_island(C.Rid(window)))
}

func ColorPickerCreate() ID {
	return ID(C.Rcolor_picker_create())
}

// ColorPickerSetColor: Sets the color (0..1 each). The callback is not called.
func ColorPickerSetColor(picker ID, r float32, g float32, b float32, a float32) {
	C.Rcolor_picker_set_color(C.Rid(picker), C.float(r), C.float(g), C.float(b), C.float(a))
}

func ColorPickerGetColor(picker ID) (r float32, g float32, b float32, a float32) {
	var r_ C.float
	var g_ C.float
	var b_ C.float
	var a_ C.float
	C.Rcolor_picker_get_color(C.Rid(picker), &r_, &g_, &b_, &a_)
	r = float32(r_)
	g = float32(g_)
	b = float32(b_)
	a = float32(a_)
	return
}

// ColorPickerSetHsv: The same color as hue (0..360), saturation and value (0..1). The picker remembers the
// hue of a gray and the saturation of black, which red, green and blue cannot tell.
func ColorPickerSetHsv(picker ID, hue float32, saturation float32, value float32) {
	C.Rcolor_picker_set_hsv(C.Rid(picker), C.float(hue), C.float(saturation), C.float(value))
}

func ColorPickerGetHsv(picker ID) (hue float32, saturation float32, value float32) {
	var hue_ C.float
	var saturation_ C.float
	var value_ C.float
	C.Rcolor_picker_get_hsv(C.Rid(picker), &hue_, &saturation_, &value_)
	hue = float32(hue_)
	saturation = float32(saturation_)
	value = float32(value_)
	return
}

// ColorPickerSetHex: Sets the color from text: "#RGB", "#RRGGBB", "#RRGGBBAA" (the '#' may be left out) or
// one of the color names of the XML attributes. A code without opacity leaves the opacity as it is. Returns:
// R_FALSE when the text is not a color (the color is unchanged).
func ColorPickerSetHex(picker ID, text string) int {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	return int(C.Rcolor_picker_set_hex(C.Rid(picker), text_))
}

// ColorPickerGetHex: Returns: "#RRGGBB", or "#RRGGBBAA" when the opacity is shown and not full. Valid until
// the color changes.
func ColorPickerGetHex(picker ID) string {
	return C.GoString(C.Rcolor_picker_get_hex(C.Rid(picker)))
}

// ColorPickerSetAlphaEnabled: R_TRUE (default) shows the opacity strip and the fourth number. R_FALSE: colors
// are opaque.
func ColorPickerSetAlphaEnabled(picker ID, enabled int) {
	C.Rcolor_picker_set_alpha_enabled(C.Rid(picker), C.int(enabled))
}

// ColorPickerSetPreviousColor: The color shown next to the current one, to compare with what was there
// before: clicking it comes back to it. Rcolor_picker_set_color does not change it; a popup picker takes it
// when it opens.
func ColorPickerSetPreviousColor(picker ID, r float32, g float32, b float32, a float32) {
	C.Rcolor_picker_set_previous_color(C.Rid(picker), C.float(r), C.float(g), C.float(b), C.float(a))
}

// ColorPickerAddSwatch: Adds a color to the row of swatches: one click takes it.
func ColorPickerAddSwatch(picker ID, r float32, g float32, b float32, a float32) {
	C.Rcolor_picker_add_swatch(C.Rid(picker), C.float(r), C.float(g), C.float(b), C.float(a))
}

func ColorPickerClearSwatches(picker ID) {
	C.Rcolor_picker_clear_swatches(C.Rid(picker))
}

func ColorPickerGetSwatchCount(picker ID) int {
	return int(C.Rcolor_picker_get_swatch_count(C.Rid(picker)))
}

// ColorPickerSetPopup: R_TRUE turns the widget into a swatch that opens the picker in a popup.
func ColorPickerSetPopup(picker ID, popup int) {
	C.Rcolor_picker_set_popup(C.Rid(picker), C.int(popup))
}

// ColorPickerSetCompact: R_TRUE hides the code and the numbers: only the square, the strips and the swatches.
func ColorPickerSetCompact(picker ID, compact int) {
	C.Rcolor_picker_set_compact(C.Rid(picker), C.int(compact))
}

// ColorRgbToHsv: Red, green, blue (0..1) to hue (0..360), saturation, value (0..1).
func ColorRgbToHsv(r float32, g float32, b float32) (hue float32, saturation float32, value float32) {
	var hue_ C.float
	var saturation_ C.float
	var value_ C.float
	C.Rcolor_rgb_to_hsv(C.float(r), C.float(g), C.float(b), &hue_, &saturation_, &value_)
	hue = float32(hue_)
	saturation = float32(saturation_)
	value = float32(value_)
	return
}

func ColorHsvToRgb(hue float32, saturation float32, value float32) (r float32, g float32, b float32) {
	var r_ C.float
	var g_ C.float
	var b_ C.float
	C.Rcolor_hsv_to_rgb(C.float(hue), C.float(saturation), C.float(value), &r_, &g_, &b_)
	r = float32(r_)
	g = float32(g_)
	b = float32(b_)
	return
}

func TimelineCreate() ID {
	return ID(C.Rtimeline_create())
}

// TimelineSetDuration: Length of the sequence. The view and the playhead stay within 0 .. duration.
func TimelineSetDuration(timeline ID, seconds float64) {
	C.Rtimeline_set_duration(C.Rid(timeline), C.double(seconds))
}

func TimelineGetDuration(timeline ID) float64 {
	return float64(C.Rtimeline_get_duration(C.Rid(timeline)))
}

// TimelineSetTime: Moves the playhead. The time callback is not called.
func TimelineSetTime(timeline ID, seconds float64) {
	C.Rtimeline_set_time(C.Rid(timeline), C.double(seconds))
}

func TimelineGetTime(timeline ID) float64 {
	return float64(C.Rtimeline_get_time(C.Rid(timeline)))
}

// TimelineSetFrameRate: Frames per second. With a frame rate the ruler counts in frames ("2:12" is second 2,
// frame 12) and everything snaps to frames; 0 (default) counts in seconds and snaps to the marks of the
// ruler.
func TimelineSetFrameRate(timeline ID, frames_per_second float64) {
	C.Rtimeline_set_frame_rate(C.Rid(timeline), C.double(frames_per_second))
}

func TimelineGetFrameRate(timeline ID) float64 {
	return float64(C.Rtimeline_get_frame_rate(C.Rid(timeline)))
}

// TimelineSetSnapping: R_FALSE lets items be placed anywhere (holding Alt while dragging does it for one
// move).
func TimelineSetSnapping(timeline ID, enabled int) {
	C.Rtimeline_set_snapping(C.Rid(timeline), C.int(enabled))
}

// TimelineSetView: The part of the sequence that is shown. Zooming is showing less.
func TimelineSetView(timeline ID, start float64, end float64) {
	C.Rtimeline_set_view(C.Rid(timeline), C.double(start), C.double(end))
}

func TimelineGetView(timeline ID) (start float64, end float64) {
	var start_ C.double
	var end_ C.double
	C.Rtimeline_get_view(C.Rid(timeline), &start_, &end_)
	start = float64(start_)
	end = float64(end_)
	return
}

// TimelineSetPlaying: Makes the playhead run by itself, at the speed of the clock, calling the time callback
// at each frame; at the end it stops, or starts again with Rtimeline_set_loop.
func TimelineSetPlaying(timeline ID, playing int) {
	C.Rtimeline_set_playing(C.Rid(timeline), C.int(playing))
}

func TimelineIsPlaying(timeline ID) int {
	return int(C.Rtimeline_is_playing(C.Rid(timeline)))
}

func TimelineSetLoop(timeline ID, loop int) {
	C.Rtimeline_set_loop(C.Rid(timeline), C.int(loop))
}

// TimelineSetHeaderWidth: Width of the column of the names of the tracks. 0 (default): fits the longest name.
func TimelineSetHeaderWidth(timeline ID, width float32) {
	C.Rtimeline_set_header_width(C.Rid(timeline), C.float(width))
}

// TimelineGetHeaderWidth: Returns: The width of the column of the names, as laid out now.
func TimelineGetHeaderWidth(timeline ID) float32 {
	return float32(C.Rtimeline_get_header_width(C.Rid(timeline)))
}

// TimelineSetRulerHeight: Height of the ruler of time (and of the markers' flags band). 0 (default):
// automatic.
func TimelineSetRulerHeight(timeline ID, height float32) {
	C.Rtimeline_set_ruler_height(C.Rid(timeline), C.float(height))
}

func TimelineGetRulerHeight(timeline ID) float32 {
	return float32(C.Rtimeline_get_ruler_height(C.Rid(timeline)))
}

// TimelineSetDefaultTrackHeight: Height of the tracks that have none of their own
// (Rtimeline_set_track_height). 0 (default): automatic.
func TimelineSetDefaultTrackHeight(timeline ID, height float32) {
	C.Rtimeline_set_default_track_height(C.Rid(timeline), C.float(height))
}

func TimelineGetDefaultTrackHeight(timeline ID) float32 {
	return float32(C.Rtimeline_get_default_track_height(C.Rid(timeline)))
}

// TimelineAddTrack: Returns: The track, or 0. Tracks are shown in the order they were added.
func TimelineAddTrack(timeline ID, name string) int {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	return int(C.Rtimeline_add_track(C.Rid(timeline), name_))
}

// TimelineRemoveTrack: Removes a track and everything on it.
func TimelineRemoveTrack(timeline ID, track int) {
	C.Rtimeline_remove_track(C.Rid(timeline), C.int(track))
}

func TimelineGetTrackCount(timeline ID) int {
	return int(C.Rtimeline_get_track_count(C.Rid(timeline)))
}

// TimelineGetTrack: Returns: The index-th track from the top, or 0.
func TimelineGetTrack(timeline ID, index int) int {
	return int(C.Rtimeline_get_track(C.Rid(timeline), C.int(index)))
}

func TimelineSetTrackName(timeline ID, track int, name string) {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	C.Rtimeline_set_track_name(C.Rid(timeline), C.int(track), name_)
}

func TimelineGetTrackName(timeline ID, track int) string {
	return C.GoString(C.Rtimeline_get_track_name(C.Rid(timeline), C.int(track)))
}

// TimelineSetTrackColor: Color of the clips and keys of the track that have none of their own.
func TimelineSetTrackColor(timeline ID, track int, r float32, g float32, b float32) {
	C.Rtimeline_set_track_color(C.Rid(timeline), C.int(track), C.float(r), C.float(g), C.float(b))
}

// TimelineSetTrackHeight: Height of the track in logical units; 0 = the default.
func TimelineSetTrackHeight(timeline ID, track int, height float32) {
	C.Rtimeline_set_track_height(C.Rid(timeline), C.int(track), C.float(height))
}

// TimelineSetTrackLocked: A locked track shows its items but does not let the user change them.
func TimelineSetTrackLocked(timeline ID, track int, locked int) {
	C.Rtimeline_set_track_locked(C.Rid(timeline), C.int(track), C.int(locked))
}

// TimelineSetTrackMuted: A muted track is drawn dimmed; what muting means is up to the application.
func TimelineSetTrackMuted(timeline ID, track int, muted int) {
	C.Rtimeline_set_track_muted(C.Rid(timeline), C.int(track), C.int(muted))
}

func TimelineIsTrackMuted(timeline ID, track int) int {
	return int(C.Rtimeline_is_track_muted(C.Rid(timeline), C.int(track)))
}

// TimelineAddClip: Returns: The clip, or 0 when the track does not exist.
func TimelineAddClip(timeline ID, track int, start float64, length float64, label string) int {
	label_ := C.CString(label)
	defer C.free(unsafe.Pointer(label_))
	return int(C.Rtimeline_add_clip(C.Rid(timeline), C.int(track), C.double(start), C.double(length), label_))
}

// TimelineAddKey: Returns: The key, or 0.
func TimelineAddKey(timeline ID, track int, time float64) int {
	return int(C.Rtimeline_add_key(C.Rid(timeline), C.int(track), C.double(time)))
}

// TimelineAddMarker: A named moment of the whole sequence, shown on the ruler and across the tracks.
func TimelineAddMarker(timeline ID, time float64, label string) int {
	label_ := C.CString(label)
	defer C.free(unsafe.Pointer(label_))
	return int(C.Rtimeline_add_marker(C.Rid(timeline), C.double(time), label_))
}

func TimelineRemoveItem(timeline ID, item int) {
	C.Rtimeline_remove_item(C.Rid(timeline), C.int(item))
}

// TimelineClear: Removes every track, clip, key and marker.
func TimelineClear(timeline ID) {
	C.Rtimeline_clear(C.Rid(timeline))
}

func TimelineGetItemType(timeline ID, item int) RTimelineItem {
	return RTimelineItem(C.Rtimeline_get_item_type(C.Rid(timeline), C.int(item)))
}

// TimelineGetItemTrack: Returns: The track of a clip or of a key; 0 for a marker.
func TimelineGetItemTrack(timeline ID, item int) int {
	return int(C.Rtimeline_get_item_track(C.Rid(timeline), C.int(item)))
}

func TimelineGetItemStart(timeline ID, item int) float64 {
	return float64(C.Rtimeline_get_item_start(C.Rid(timeline), C.int(item)))
}

// TimelineGetItemLength: Returns: The length of a clip; 0 for a key or a marker.
func TimelineGetItemLength(timeline ID, item int) float64 {
	return float64(C.Rtimeline_get_item_length(C.Rid(timeline), C.int(item)))
}

// TimelineSetItem: Moves an item: to another track (0 = keep), another start, another length (clips; negative
// = keep).
func TimelineSetItem(timeline ID, item int, track int, start float64, length float64) {
	C.Rtimeline_set_item(C.Rid(timeline), C.int(item), C.int(track), C.double(start), C.double(length))
}

func TimelineSetItemLabel(timeline ID, item int, label string) {
	label_ := C.CString(label)
	defer C.free(unsafe.Pointer(label_))
	C.Rtimeline_set_item_label(C.Rid(timeline), C.int(item), label_)
}

func TimelineGetItemLabel(timeline ID, item int) string {
	return C.GoString(C.Rtimeline_get_item_label(C.Rid(timeline), C.int(item)))
}

func TimelineSetItemColor(timeline ID, item int, r float32, g float32, b float32) {
	C.Rtimeline_set_item_color(C.Rid(timeline), C.int(item), C.float(r), C.float(g), C.float(b))
}

func TimelineGetItemCount(timeline ID) int {
	return int(C.Rtimeline_get_item_count(C.Rid(timeline)))
}

// TimelineGetItem: Returns: The index-th item, in the order they were added, or 0.
func TimelineGetItem(timeline ID, index int) int {
	return int(C.Rtimeline_get_item(C.Rid(timeline), C.int(index)))
}

// TimelineSelect: Selects an item, alone or (add) with the ones already selected.
func TimelineSelect(timeline ID, item int, add int) {
	C.Rtimeline_select(C.Rid(timeline), C.int(item), C.int(add))
}

func TimelineClearSelection(timeline ID) {
	C.Rtimeline_clear_selection(C.Rid(timeline))
}

func TimelineIsSelected(timeline ID, item int) int {
	return int(C.Rtimeline_is_selected(C.Rid(timeline), C.int(item)))
}

func TimelineGetSelectionCount(timeline ID) int {
	return int(C.Rtimeline_get_selection_count(C.Rid(timeline)))
}

func TimelineGetSelected(timeline ID, index int) int {
	return int(C.Rtimeline_get_selected(C.Rid(timeline), C.int(index)))
}

func NodeGraphCreate() ID {
	return ID(C.Rnode_graph_create())
}

// NodeGraphAddNode: x,y Top-left corner of the node, in the units of the canvas.
func NodeGraphAddNode(graph ID, title string, x float32, y float32) int {
	title_ := C.CString(title)
	defer C.free(unsafe.Pointer(title_))
	return int(C.Rnode_graph_add_node(C.Rid(graph), title_, C.float(x), C.float(y)))
}

// NodeGraphRemoveNode: Removes a node, its pins, their links, and the widget it holds.
func NodeGraphRemoveNode(graph ID, node int) {
	C.Rnode_graph_remove_node(C.Rid(graph), C.int(node))
}

func NodeGraphClear(graph ID) {
	C.Rnode_graph_clear(C.Rid(graph))
}

func NodeGraphGetNodeCount(graph ID) int {
	return int(C.Rnode_graph_get_node_count(C.Rid(graph)))
}

func NodeGraphGetNode(graph ID, index int) int {
	return int(C.Rnode_graph_get_node(C.Rid(graph), C.int(index)))
}

func NodeGraphSetNodeTitle(graph ID, node int, title string) {
	title_ := C.CString(title)
	defer C.free(unsafe.Pointer(title_))
	C.Rnode_graph_set_node_title(C.Rid(graph), C.int(node), title_)
}

func NodeGraphGetNodeTitle(graph ID, node int) string {
	return C.GoString(C.Rnode_graph_get_node_title(C.Rid(graph), C.int(node)))
}

func NodeGraphSetNodePosition(graph ID, node int, x float32, y float32) {
	C.Rnode_graph_set_node_position(C.Rid(graph), C.int(node), C.float(x), C.float(y))
}

func NodeGraphGetNodePosition(graph ID, node int) (x float32, y float32) {
	var x_ C.float
	var y_ C.float
	C.Rnode_graph_get_node_position(C.Rid(graph), C.int(node), &x_, &y_)
	x = float32(x_)
	y = float32(y_)
	return
}

// NodeGraphGetNodeSize: Size of a node on the canvas: Raven works it out from its title, its pins and its
// content.
func NodeGraphGetNodeSize(graph ID, node int) (width float32, height float32) {
	var width_ C.float
	var height_ C.float
	C.Rnode_graph_get_node_size(C.Rid(graph), C.int(node), &width_, &height_)
	width = float32(width_)
	height = float32(height_)
	return
}

// NodeGraphSetNodeColor: Color of the header of the node; without one it takes the accent color of the theme.
func NodeGraphSetNodeColor(graph ID, node int, r float32, g float32, b float32) {
	C.Rnode_graph_set_node_color(C.Rid(graph), C.int(node), C.float(r), C.float(g), C.float(b))
}

// NodeGraphSetNodeContent: Puts a widget inside the node, under its pins: the graph owns it from now on.
// R_INVALID_ID removes (and deletes) the one it had.
func NodeGraphSetNodeContent(graph ID, node int, widget ID) {
	C.Rnode_graph_set_node_content(C.Rid(graph), C.int(node), C.Rid(widget))
}

func NodeGraphGetNodeContent(graph ID, node int) ID {
	return ID(C.Rnode_graph_get_node_content(C.Rid(graph), C.int(node)))
}

// NodeGraphSetNodeName: Name of the node, the `Name` of its XML element: what <Link From="name.pin"> and
// Rnode_graph_find_node look for. Not shown.
func NodeGraphSetNodeName(graph ID, node int, name string) {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	C.Rnode_graph_set_node_name(C.Rid(graph), C.int(node), name_)
}

func NodeGraphGetNodeName(graph ID, node int) string {
	return C.GoString(C.Rnode_graph_get_node_name(C.Rid(graph), C.int(node)))
}

// NodeGraphFindNode: Returns: The node of that name, or 0.
func NodeGraphFindNode(graph ID, name string) int {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	return int(C.Rnode_graph_find_node(C.Rid(graph), name_))
}

// NodeGraphAddInput: type Free text, or NULL for a pin that links to any type. Returns: The pin, or 0.
func NodeGraphAddInput(graph ID, node int, label string, type_ string) int {
	label_ := C.CString(label)
	defer C.free(unsafe.Pointer(label_))
	type__ := C.CString(type_)
	defer C.free(unsafe.Pointer(type__))
	return int(C.Rnode_graph_add_input(C.Rid(graph), C.int(node), label_, type__))
}

func NodeGraphAddOutput(graph ID, node int, label string, type_ string) int {
	label_ := C.CString(label)
	defer C.free(unsafe.Pointer(label_))
	type__ := C.CString(type_)
	defer C.free(unsafe.Pointer(type__))
	return int(C.Rnode_graph_add_output(C.Rid(graph), C.int(node), label_, type__))
}

func NodeGraphRemovePin(graph ID, pin int) {
	C.Rnode_graph_remove_pin(C.Rid(graph), C.int(pin))
}

func NodeGraphGetPinNode(graph ID, pin int) int {
	return int(C.Rnode_graph_get_pin_node(C.Rid(graph), C.int(pin)))
}

func NodeGraphIsPinOutput(graph ID, pin int) int {
	return int(C.Rnode_graph_is_pin_output(C.Rid(graph), C.int(pin)))
}

func NodeGraphGetPinLabel(graph ID, pin int) string {
	return C.GoString(C.Rnode_graph_get_pin_label(C.Rid(graph), C.int(pin)))
}

func NodeGraphGetPinType(graph ID, pin int) string {
	return C.GoString(C.Rnode_graph_get_pin_type(C.Rid(graph), C.int(pin)))
}

// NodeGraphSetPinName: Name of the pin for Rnode_graph_find_pin and for the links of XML; a pin without one
// answers to its label.
func NodeGraphSetPinName(graph ID, pin int, name string) {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	C.Rnode_graph_set_pin_name(C.Rid(graph), C.int(pin), name_)
}

// NodeGraphFindPin: Returns: The pin of a node with that name (or label), or 0.
func NodeGraphFindPin(graph ID, node int, name string) int {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	return int(C.Rnode_graph_find_pin(C.Rid(graph), C.int(node), name_))
}

// NodeGraphGetPinPosition: Where a pin is on the canvas (the middle of its dot).
func NodeGraphGetPinPosition(graph ID, pin int) (x float32, y float32) {
	var x_ C.float
	var y_ C.float
	C.Rnode_graph_get_pin_position(C.Rid(graph), C.int(pin), &x_, &y_)
	x = float32(x_)
	y = float32(y_)
	return
}

// NodeGraphGetInput: Returns: The index-th input (or output) of a node, or 0.
func NodeGraphGetInput(graph ID, node int, index int) int {
	return int(C.Rnode_graph_get_input(C.Rid(graph), C.int(node), C.int(index)))
}

func NodeGraphGetOutput(graph ID, node int, index int) int {
	return int(C.Rnode_graph_get_output(C.Rid(graph), C.int(node), C.int(index)))
}

// NodeGraphSetTypeColor: Color of the pins of a type and of the links between them.
func NodeGraphSetTypeColor(graph ID, type_ string, r float32, g float32, b float32) {
	type__ := C.CString(type_)
	defer C.free(unsafe.Pointer(type__))
	C.Rnode_graph_set_type_color(C.Rid(graph), type__, C.float(r), C.float(g), C.float(b))
}

// NodeGraphConnect: Links an output to an input, as the user does by dragging. The connect callback is not
// asked. Returns: The link; 0 when the pins cannot be linked (two inputs, the same node, different types).
func NodeGraphConnect(graph ID, output_pin int, input_pin int) int {
	return int(C.Rnode_graph_connect(C.Rid(graph), C.int(output_pin), C.int(input_pin)))
}

func NodeGraphDisconnect(graph ID, link int) {
	C.Rnode_graph_disconnect(C.Rid(graph), C.int(link))
}

func NodeGraphGetLinkCount(graph ID) int {
	return int(C.Rnode_graph_get_link_count(C.Rid(graph)))
}

func NodeGraphGetLink(graph ID, index int) int {
	return int(C.Rnode_graph_get_link(C.Rid(graph), C.int(index)))
}

func NodeGraphGetLinkOutput(graph ID, link int) int {
	return int(C.Rnode_graph_get_link_output(C.Rid(graph), C.int(link)))
}

func NodeGraphGetLinkInput(graph ID, link int) int {
	return int(C.Rnode_graph_get_link_input(C.Rid(graph), C.int(link)))
}

// NodeGraphGetInputLink: Returns: The link that arrives at an input pin, or 0.
func NodeGraphGetInputLink(graph ID, input_pin int) int {
	return int(C.Rnode_graph_get_input_link(C.Rid(graph), C.int(input_pin)))
}

// NodeGraphSetView: What the middle of the widget shows: a point of the canvas, and how large (1 = one unit
// per logical unit).
func NodeGraphSetView(graph ID, center_x float32, center_y float32, zoom float32) {
	C.Rnode_graph_set_view(C.Rid(graph), C.float(center_x), C.float(center_y), C.float(zoom))
}

func NodeGraphGetView(graph ID) (center_x float32, center_y float32, zoom float32) {
	var center_x_ C.float
	var center_y_ C.float
	var zoom_ C.float
	C.Rnode_graph_get_view(C.Rid(graph), &center_x_, &center_y_, &zoom_)
	center_x = float32(center_x_)
	center_y = float32(center_y_)
	zoom = float32(zoom_)
	return
}

// NodeGraphToCanvas: Converts a point of the widget (from its corner, as in the events) to the canvas, and
// back, with the view as it is shown now.
func NodeGraphToCanvas(graph ID, x float32, y float32) (canvas_x float32, canvas_y float32) {
	var canvas_x_ C.float
	var canvas_y_ C.float
	C.Rnode_graph_to_canvas(C.Rid(graph), C.float(x), C.float(y), &canvas_x_, &canvas_y_)
	canvas_x = float32(canvas_x_)
	canvas_y = float32(canvas_y_)
	return
}

func NodeGraphFromCanvas(graph ID, canvas_x float32, canvas_y float32) (x float32, y float32) {
	var x_ C.float
	var y_ C.float
	C.Rnode_graph_from_canvas(C.Rid(graph), C.float(canvas_x), C.float(canvas_y), &x_, &y_)
	x = float32(x_)
	y = float32(y_)
	return
}

// NodeGraphFrameAll: Moves and zooms so that every node shows.
func NodeGraphFrameAll(graph ID) {
	C.Rnode_graph_frame_all(C.Rid(graph))
}

// NodeGraphSetGridVisible: R_FALSE hides the grid of dots (shown by default).
func NodeGraphSetGridVisible(graph ID, visible int) {
	C.Rnode_graph_set_grid_visible(C.Rid(graph), C.int(visible))
}

// NodeGraphSetFlowVisible: The pulses of light that run along the links, from output to input. By default
// they run while the pointer is over the graph, and the window rests the rest of the time. R_TRUE makes them
// run all the time (the window then draws without pause); R_FALSE stops them.
func NodeGraphSetFlowVisible(graph ID, visible int) {
	C.Rnode_graph_set_flow_visible(C.Rid(graph), C.int(visible))
}

// NodeGraphSetReadOnly: R_TRUE: the user can look but not move, link or delete.
func NodeGraphSetReadOnly(graph ID, read_only int) {
	C.Rnode_graph_set_read_only(C.Rid(graph), C.int(read_only))
}

func NodeGraphSelectNode(graph ID, node int, add int) {
	C.Rnode_graph_select_node(C.Rid(graph), C.int(node), C.int(add))
}

func NodeGraphClearSelection(graph ID) {
	C.Rnode_graph_clear_selection(C.Rid(graph))
}

func NodeGraphIsNodeSelected(graph ID, node int) int {
	return int(C.Rnode_graph_is_node_selected(C.Rid(graph), C.int(node)))
}

func NodeGraphGetSelectionCount(graph ID) int {
	return int(C.Rnode_graph_get_selection_count(C.Rid(graph)))
}

func NodeGraphGetSelectedNode(graph ID, index int) int {
	return int(C.Rnode_graph_get_selected_node(C.Rid(graph), C.int(index)))
}

// NodeGraphSetPinMultiple: R_TRUE: this input keeps every link made to it (a state that several transitions
// enter) instead of one. Pulling a wire from it starts a new one.
func NodeGraphSetPinMultiple(graph ID, pin int, multiple int) {
	C.Rnode_graph_set_pin_multiple(C.Rid(graph), C.int(pin), C.int(multiple))
}

// NodeGraphSetNodeActive: R_TRUE: the node is running (live debug) : a soft ring breathes around it.
func NodeGraphSetNodeActive(graph ID, node int, active int) {
	C.Rnode_graph_set_node_active(C.Rid(graph), C.int(node), C.int(active))
}

// NodeGraphSetNodeActiveColor: Color of that ring (the success color of the theme by default).
func NodeGraphSetNodeActiveColor(graph ID, node int, r float32, g float32, b float32) {
	C.Rnode_graph_set_node_active_color(C.Rid(graph), C.int(node), C.float(r), C.float(g), C.float(b))
}

// NodeGraphSetNodeDeletable: R_FALSE: the Delete key does not remove this node (the root of a tree, the
// output of a graph).
func NodeGraphSetNodeDeletable(graph ID, node int, deletable int) {
	C.Rnode_graph_set_node_deletable(C.Rid(graph), C.int(node), C.int(deletable))
}

// NodeGraphSetNodeBadge: A short text in a pill on the top right corner of the node ("1", "!"...); NULL or ""
// removes it.
func NodeGraphSetNodeBadge(graph ID, node int, text string) {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	C.Rnode_graph_set_node_badge(C.Rid(graph), C.int(node), text_)
}

func BlockEditorCreate() ID {
	return ID(C.Rblock_editor_create())
}

// BlockEditorAddKind: A kind of block, and its block in the palette. Returns its index, -1 on an error.
func BlockEditorAddKind(editor ID, name string, label string, shape RBlockShape, category string) int {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	label_ := C.CString(label)
	defer C.free(unsafe.Pointer(label_))
	category_ := C.CString(category)
	defer C.free(unsafe.Pointer(category_))
	return int(C.Rblock_editor_add_kind(C.Rid(editor), name_, label_, C.RBlockShape(shape), category_))
}

// BlockEditorSetCategoryColor: Color of the blocks of a category (Raven picks one from its name otherwise).
func BlockEditorSetCategoryColor(editor ID, category string, r float32, g float32, b float32) {
	category_ := C.CString(category)
	defer C.free(unsafe.Pointer(category_))
	C.Rblock_editor_set_category_color(C.Rid(editor), category_, C.float(r), C.float(g), C.float(b))
}

func BlockEditorSetPaletteVisible(editor ID, visible int) {
	C.Rblock_editor_set_palette_visible(C.Rid(editor), C.int(visible))
}

// BlockEditorAddBlock: A new block, the top of a script of its own at x, y (canvas units). Returns it, 0 on
// an error.
func BlockEditorAddBlock(editor ID, kind string, x float32, y float32) int {
	kind_ := C.CString(kind)
	defer C.free(unsafe.Pointer(kind_))
	return int(C.Rblock_editor_add_block(C.Rid(editor), kind_, C.float(x), C.float(y)))
}

// BlockEditorRemoveBlock: Removes a block and everything that hangs under it.
func BlockEditorRemoveBlock(editor ID, block int) {
	C.Rblock_editor_remove_block(C.Rid(editor), C.int(block))
}

func BlockEditorClear(editor ID) {
	C.Rblock_editor_clear(C.Rid(editor))
}

// BlockEditorAttach: Puts a block (with what is under it) at a place of another one. R_FALSE when it does not
// fit there.
func BlockEditorAttach(editor ID, block int, target int, place RBlockPlace, index int) int {
	return int(C.Rblock_editor_attach(C.Rid(editor), C.int(block), C.int(target), C.RBlockPlace(place), C.int(index)))
}

// BlockEditorDetach: Takes a block (with what is under it) out: a script of its own at x, y.
func BlockEditorDetach(editor ID, block int, x float32, y float32) {
	C.Rblock_editor_detach(C.Rid(editor), C.int(block), C.float(x), C.float(y))
}

func BlockEditorGetScriptCount(editor ID) int {
	return int(C.Rblock_editor_get_script_count(C.Rid(editor)))
}

// BlockEditorGetScript: The top block of a script.
func BlockEditorGetScript(editor ID, index int) int {
	return int(C.Rblock_editor_get_script(C.Rid(editor), C.int(index)))
}

func BlockEditorGetKind(editor ID, block int) string {
	return C.GoString(C.Rblock_editor_get_kind(C.Rid(editor), C.int(block)))
}

// BlockEditorGetNext: The block under it, 0 if none.
func BlockEditorGetNext(editor ID, block int) int {
	return int(C.Rblock_editor_get_next(C.Rid(editor), C.int(block)))
}

func BlockEditorGetMouthCount(editor ID, block int) int {
	return int(C.Rblock_editor_get_mouth_count(C.Rid(editor), C.int(block)))
}

// BlockEditorGetBody: The first block in a mouth of a C block, 0 if empty.
func BlockEditorGetBody(editor ID, block int, mouth int) int {
	return int(C.Rblock_editor_get_body(C.Rid(editor), C.int(block), C.int(mouth)))
}

func BlockEditorGetSlotCount(editor ID, block int) int {
	return int(C.Rblock_editor_get_slot_count(C.Rid(editor), C.int(block)))
}

// BlockEditorGetInput: The value block in a slot, 0 if the slot holds a typed value.
func BlockEditorGetInput(editor ID, block int, slot int) int {
	return int(C.Rblock_editor_get_input(C.Rid(editor), C.int(block), C.int(slot)))
}

// BlockEditorGetValue: The typed value of a slot.
func BlockEditorGetValue(editor ID, block int, slot int) string {
	return C.GoString(C.Rblock_editor_get_value(C.Rid(editor), C.int(block), C.int(slot)))
}

func BlockEditorSetValue(editor ID, block int, slot int, value string) {
	value_ := C.CString(value)
	defer C.free(unsafe.Pointer(value_))
	C.Rblock_editor_set_value(C.Rid(editor), C.int(block), C.int(slot), value_)
}

// BlockEditorGetParent: Where a block hangs: the block it hangs from (0: the top of a script).
func BlockEditorGetParent(editor ID, block int) (result int, place RBlockPlace, index int) {
	var place_ C.RBlockPlace
	var index_ C.int
	result = int(C.Rblock_editor_get_parent(C.Rid(editor), C.int(block), &place_, &index_))
	place = RBlockPlace(place_)
	index = int(index_)
	return
}

func BlockEditorGetPosition(editor ID, block int) (x float32, y float32) {
	var x_ C.float
	var y_ C.float
	C.Rblock_editor_get_position(C.Rid(editor), C.int(block), &x_, &y_)
	x = float32(x_)
	y = float32(y_)
	return
}

// BlockEditorSetBlockGlow: R_TRUE: the block is running: a halo breathes around it.
func BlockEditorSetBlockGlow(editor ID, block int, glow int) {
	C.Rblock_editor_set_block_glow(C.Rid(editor), C.int(block), C.int(glow))
}

// BlockEditorSave: Every script as text (valid until the next call).
func BlockEditorSave(editor ID) string {
	return C.GoString(C.Rblock_editor_save(C.Rid(editor)))
}

// BlockEditorLoad: Replaces the scripts with those of a text from Rblock_editor_save.
func BlockEditorLoad(editor ID, text string) int {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	return int(C.Rblock_editor_load(C.Rid(editor), text_))
}

// BlockEditorSetSlotOptions: The choices of a %m slot of a kind, separated by '\n' ("" : the slot is typed
// in).
func BlockEditorSetSlotOptions(editor ID, kind string, slot int, options string) {
	kind_ := C.CString(kind)
	defer C.free(unsafe.Pointer(kind_))
	options_ := C.CString(options)
	defer C.free(unsafe.Pointer(options_))
	C.Rblock_editor_set_slot_options(C.Rid(editor), kind_, C.int(slot), options_)
}

// BlockEditorSetKindVisible: R_FALSE: the kind is not in the palette (its blocks stay where they are).
func BlockEditorSetKindVisible(editor ID, kind string, visible int) {
	kind_ := C.CString(kind)
	defer C.free(unsafe.Pointer(kind_))
	C.Rblock_editor_set_kind_visible(C.Rid(editor), kind_, C.int(visible))
}

// BlockEditorSetKindLabel: A new label for a kind (a function that takes one more input...). The blocks of
// that kind keep the values of the slots that are still there.
func BlockEditorSetKindLabel(editor ID, kind string, label string) int {
	kind_ := C.CString(kind)
	defer C.free(unsafe.Pointer(kind_))
	label_ := C.CString(label)
	defer C.free(unsafe.Pointer(label_))
	return int(C.Rblock_editor_set_kind_label(C.Rid(editor), kind_, label_))
}

// BlockEditorSetBlockDeletable: R_FALSE: the user cannot delete the block (the entry of a function). Not
// saved.
func BlockEditorSetBlockDeletable(editor ID, block int, deletable int) {
	C.Rblock_editor_set_block_deletable(C.Rid(editor), C.int(block), C.int(deletable))
}

func BlockEditorGetSelectionCount(editor ID) int {
	return int(C.Rblock_editor_get_selection_count(C.Rid(editor)))
}

func BlockEditorGetSelected(editor ID, index int) int {
	return int(C.Rblock_editor_get_selected(C.Rid(editor), C.int(index)))
}

// BlockEditorSelect: Selects a block (add: R_FALSE clears the selection first ; block 0 : only clears).
func BlockEditorSelect(editor ID, block int, add int) {
	C.Rblock_editor_select(C.Rid(editor), C.int(block), C.int(add))
}

// BlockEditorGetView: Pan (canvas units at the top left) and zoom.
func BlockEditorGetView(editor ID) (x float32, y float32, zoom float32) {
	var x_ C.float
	var y_ C.float
	var zoom_ C.float
	C.Rblock_editor_get_view(C.Rid(editor), &x_, &y_, &zoom_)
	x = float32(x_)
	y = float32(y_)
	zoom = float32(zoom_)
	return
}

func BlockEditorSetView(editor ID, x float32, y float32, zoom float32) {
	C.Rblock_editor_set_view(C.Rid(editor), C.float(x), C.float(y), C.float(zoom))
}

// DialogClose: Closes the dialog that contains a widget (the content itself or anything inside it: the button
// that was clicked). result Passed to the closed callback and returned by Rdialog_run.
func DialogClose(widget ID, result int) {
	C.Rdialog_close(C.Rid(widget), C.int(result))
}

// DialogRun: Shows a dialog and waits until it closes. Returns its result.
func DialogRun(window ID, content ID) int {
	return int(C.Rdialog_run(C.Rid(window), C.Rid(content)))
}

// WindowHasDialog: Returns: R_TRUE while a dialog is shown in the window.
func WindowHasDialog(window ID) int {
	return int(C.Rwindow_has_dialog(C.Rid(window)))
}

// MessageBox: Shows a message with buttons and waits for the answer. window R_INVALID_ID: the first window of
// the application. title May be NULL.
func MessageBox(window ID, title string, text string, buttons RMessageButtons) RMessageResult {
	title_ := C.CString(title)
	defer C.free(unsafe.Pointer(title_))
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	return RMessageResult(C.Rmessage_box(C.Rid(window), title_, text_, C.RMessageButtons(buttons)))
}

// FileDialogGetResultCount: What the last file dialog returned: several paths with R_FILE_DIALOG_MULTIPLE.
func FileDialogGetResultCount() int {
	return int(C.Rfile_dialog_get_result_count())
}

func FileDialogGetResult(index int) string {
	return C.GoString(C.Rfile_dialog_get_result(C.int(index)))
}

// FileRegisterMemory: Registers a file held in memory under a path: every file Raven reads (XML documents and
// their Include, Source and Font attributes, themes, textures, fonts, Rfile_read) is looked up here first, so
// that resources embedded in the executable need no file beside it. A relative path names the file as it is
// written in the XML once joined to the folder of the document that names it (Rxml_set_base_path for a
// document given as text), e.g. "ui/editor.xml". Paths compare with '/' and '\\' alike. Memory files are not
// watched for live reload. Call it after Rinit; Rshutdown forgets every file. copy R_TRUE: Raven keeps a
// copy; R_FALSE: `data` must outlive Raven (static data).
func FileRegisterMemory(path string, data []byte, copy_ int) int {
	path_ := C.CString(path)
	defer C.free(unsafe.Pointer(path_))
	var data_ unsafe.Pointer
	if len(data) > 0 {
		data_ = unsafe.Pointer(&data[0])
	}
	return int(C.Rfile_register_memory(path_, data_, C.size_t(len(data)), C.int(copy_)))
}

func FileUnregisterMemory(path string) {
	path_ := C.CString(path)
	defer C.free(unsafe.Pointer(path_))
	C.Rfile_unregister_memory(path_)
}

// FileIsMemory: R_TRUE when `path` is a file registered with Rfile_register_memory.
func FileIsMemory(path string) int {
	path_ := C.CString(path)
	defer C.free(unsafe.Pointer(path_))
	return int(C.Rfile_is_memory(path_))
}

func FileWrite(path string, data []byte) int {
	path_ := C.CString(path)
	defer C.free(unsafe.Pointer(path_))
	var data_ unsafe.Pointer
	if len(data) > 0 {
		data_ = unsafe.Pointer(&data[0])
	}
	return int(C.Rfile_write(path_, data_, C.size_t(len(data))))
}

func FileBrowserCreate() ID {
	return ID(C.Rfile_browser_create())
}

func FileBrowserSetMode(browser ID, mode RFileBrowserMode) {
	C.Rfile_browser_set_mode(C.Rid(browser), C.RFileBrowserMode(mode))
}

// FileBrowserSetDirectory: Shows a folder. "~" is the home of the user. Returns R_FALSE when it cannot be
// read.
func FileBrowserSetDirectory(browser ID, path string) int {
	path_ := C.CString(path)
	defer C.free(unsafe.Pointer(path_))
	return int(C.Rfile_browser_set_directory(C.Rid(browser), path_))
}

func FileBrowserGetDirectory(browser ID) string {
	return C.GoString(C.Rfile_browser_get_directory(C.Rid(browser)))
}

// FileBrowserRefresh: Reads the folder again.
func FileBrowserRefresh(browser ID) {
	C.Rfile_browser_refresh(C.Rid(browser))
}

func FileBrowserSetShowHidden(browser ID, show int) {
	C.Rfile_browser_set_show_hidden(C.Rid(browser), C.int(show))
}

// FileBrowserSetMultiple: R_TRUE lets several files be selected (Ctrl and Shift with a click).
func FileBrowserSetMultiple(browser ID, multiple int) {
	C.Rfile_browser_set_multiple(C.Rid(browser), C.int(multiple))
}

// FileBrowserSetControlsVisible: R_FALSE hides the name field, the filter list and the buttons: what remains
// is an explorer to put in a side panel. Shown by default.
func FileBrowserSetControlsVisible(browser ID, visible int) {
	C.Rfile_browser_set_controls_visible(C.Rid(browser), C.int(visible))
}

// FileBrowserSetPlacesVisible: R_FALSE hides the list of places on the left.
func FileBrowserSetPlacesVisible(browser ID, visible int) {
	C.Rfile_browser_set_places_visible(C.Rid(browser), C.int(visible))
}

// FileBrowserSetFileName: Name typed in the name field (save mode).
func FileBrowserSetFileName(browser ID, name string) {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	C.Rfile_browser_set_file_name(C.Rid(browser), name_)
}

// FileBrowserSetDefaultExtension: Added to a name typed without extension, in save mode.
func FileBrowserSetDefaultExtension(browser ID, extension string) {
	extension_ := C.CString(extension)
	defer C.free(unsafe.Pointer(extension_))
	C.Rfile_browser_set_default_extension(C.Rid(browser), extension_)
}

// FileBrowserGetSelectionCount: Paths of the selected entries. The strings are valid until the selection or
// the folder changes.
func FileBrowserGetSelectionCount(browser ID) int {
	return int(C.Rfile_browser_get_selection_count(C.Rid(browser)))
}

func FileBrowserGetSelection(browser ID, index int) string {
	return C.GoString(C.Rfile_browser_get_selection(C.Rid(browser), C.int(index)))
}

// FileBrowserSelect: Selects an entry of the folder by its name.
func FileBrowserSelect(browser ID, name string) {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	C.Rfile_browser_select(C.Rid(browser), name_)
}

// ViewportSetTexture: Shows a texture. R_INVALID_ID shows nothing.
func ViewportSetTexture(viewport ID, texture ID) {
	C.Rviewport_set_texture(C.Rid(viewport), C.Rid(texture))
}

// ViewportSetStretch: How the texture is fitted in the widget. Default: R_STRETCH_FILL.
func ViewportSetStretch(viewport ID, stretch RStretch) {
	C.Rviewport_set_stretch(C.Rid(viewport), C.RStretch(stretch))
}

// ViewportSetFlipY: R_TRUE flips the texture vertically (OpenGL framebuffers are bottom-up).
func ViewportSetFlipY(viewport ID, flip int) {
	C.Rviewport_set_flip_y(C.Rid(viewport), C.int(flip))
}

// ViewportSetNative: Creates (or destroys) the native child window of the viewport. The viewport must already
// be in a window. Returns: R_TRUE on success.
func ViewportSetNative(viewport ID, enabled int) int {
	return int(C.Rviewport_set_native(C.Rid(viewport), C.int(enabled)))
}

// ViewportGetPixelSize: Size of the viewport in pixels after the last layout.
func ViewportGetPixelSize(viewport ID) (width int, height int) {
	var width_ C.int
	var height_ C.int
	C.Rviewport_get_pixel_size(C.Rid(viewport), &width_, &height_)
	width = int(width_)
	height = int(height_)
	return
}

// ViewportSetRealtime: R_TRUE redraws the window every frame while the viewport is visible (game view).
// R_FALSE redraws only on Rwidget_invalidate (default).
func ViewportSetRealtime(viewport ID, realtime int) {
	C.Rviewport_set_realtime(C.Rid(viewport), C.int(realtime))
}

// ShaderCreate: A shader from SPIR-V code (a fragment shader written with raven_effect.glsl, as in a .spv
// file): `size` bytes, copied. R_INVALID_ID when the code is not SPIR-V; a backend that refuses it reports an
// error and the effect is drawn as a rectangle.
func ShaderCreate(spirv []byte, flags uint32) ID {
	var spirv_ unsafe.Pointer
	if len(spirv) > 0 {
		spirv_ = unsafe.Pointer(&spirv[0])
	}
	return ID(C.Rshader_create(spirv_, C.size_t(len(spirv)), C.Ruint(flags)))
}

// ShaderCreateFromFile: The same, from a .spv file (UTF-8 path, relative to the current folder).
func ShaderCreateFromFile(path string, flags uint32) ID {
	path_ := C.CString(path)
	defer C.free(unsafe.Pointer(path_))
	return ID(C.Rshader_create_from_file(path_, C.Ruint(flags)))
}

func ShaderDestroy(shader ID) {
	C.Rshader_destroy(C.Rid(shader))
}

func ShaderIsValid(shader ID) int {
	return int(C.Rshader_is_valid(C.Rid(shader)))
}

func ShaderGetFlags(shader ID) uint32 {
	return uint32(C.Rshader_get_flags(C.Rid(shader)))
}

// EffectGet: A built-in effect, created the first time it is asked for.
func EffectGet(effect REffect) ID {
	return ID(C.Reffect_get(C.REffect(effect)))
}

// EffectFind: A built-in effect by its name ("aurora", "shimmer", "neon", "ripple", "plasma", "grain",
// "spotlight", "mesh"), or R_INVALID_ID.
func EffectFind(name string) ID {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	return ID(C.Reffect_find(name_))
}

func EffectGetName(effect REffect) string {
	return C.GoString(C.Reffect_get_name(C.REffect(effect)))
}

func BorderGetEffect(border ID) ID {
	return ID(C.Rborder_get_effect(C.Rid(border)))
}

// WindowCreateFromXml: Creates a window from a document whose root element is <Window>. xml UTF-8 text. It
// does not need to be null-terminated. size Size of the text in bytes. Returns: The window, or R_INVALID_ID.
// Parse errors are reported with their line and column through the error callback (R_ERROR_XML).
func WindowCreateFromXml(xml string) ID {
	xml_ := C.CString(xml)
	defer C.free(unsafe.Pointer(xml_))
	return ID(C.Rwindow_create_from_xml(xml_, C.size_t(len(xml))))
}

func WindowCreateFromXmlFile(path string) ID {
	path_ := C.CString(path)
	defer C.free(unsafe.Pointer(path_))
	return ID(C.Rwindow_create_from_xml_file(path_))
}

// WidgetCreateFromXml: Creates a widget tree from a document whose root element is a widget. The caller owns
// the returned widget until it is added to a parent.
func WidgetCreateFromXml(xml string) ID {
	xml_ := C.CString(xml)
	defer C.free(unsafe.Pointer(xml_))
	return ID(C.Rwidget_create_from_xml(xml_, C.size_t(len(xml))))
}

func WidgetCreateFromXmlFile(path string) ID {
	path_ := C.CString(path)
	defer C.free(unsafe.Pointer(path_))
	return ID(C.Rwidget_create_from_xml_file(path_))
}

// XmlSetBasePath: Folder used to resolve relative paths found in XML (Source="...", Font="..."). The
// *FromXmlFile functions use the folder of the file.
func XmlSetBasePath(path string) {
	path_ := C.CString(path)
	defer C.free(unsafe.Pointer(path_))
	C.Rxml_set_base_path(path_)
}

// XmlSetLiveReload: Turns live reload on or off. It is on by default; the environment variable
// RAVEN_LIVE_RELOAD=0 turns it off without rebuilding. When off, files are never looked at again.
func XmlSetLiveReload(enabled int) {
	C.Rxml_set_live_reload(C.int(enabled))
}

func XmlGetLiveReload() int {
	return int(C.Rxml_get_live_reload())
}

// XmlCheckFiles: Looks at the files now, without waiting for the next look (there are five per second), even
// when live reload is off. Returns: The number of files found changed.
func XmlCheckFiles() int {
	return int(C.Rxml_check_files())
}

// XmlRegisterWidgets: Reads the <Widget Name="..."> declarations of a document (its root, or the children of
// a <Widgets> root) so that the tags they declare can be used in the documents loaded afterwards. Returns:
// The number of tags declared.
func XmlRegisterWidgets(xml string) int {
	xml_ := C.CString(xml)
	defer C.free(unsafe.Pointer(xml_))
	return int(C.Rxml_register_widgets(xml_, C.size_t(len(xml))))
}

func XmlRegisterWidgetsFromFile(path string) int {
	path_ := C.CString(path)
	defer C.free(unsafe.Pointer(path_))
	return int(C.Rxml_register_widgets_from_file(path_))
}

func XmlUnregisterWidget(name string) {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	C.Rxml_unregister_widget(name_)
}

func XmlIsWidgetRegistered(name string) int {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	return int(C.Rxml_is_widget_registered(name_))
}

// XmlGetRegisteredWidgetCount: The declared tags, for tools that list them.
func XmlGetRegisteredWidgetCount() int {
	return int(C.Rxml_get_registered_widget_count())
}

func XmlGetRegisteredWidgetName(index int) string {
	return C.GoString(C.Rxml_get_registered_widget_name(C.int(index)))
}

func XmlUnregisterTag(tag string) {
	tag_ := C.CString(tag)
	defer C.free(unsafe.Pointer(tag_))
	C.Rxml_unregister_tag(tag_)
}

func XmlIsTagRegistered(tag string) int {
	tag_ := C.CString(tag)
	defer C.free(unsafe.Pointer(tag_))
	return int(C.Rxml_is_tag_registered(tag_))
}

// XmlResolvePath: The path of a file named in a document: as the attributes Source of Raven read it, relative
// to the folder of the document being loaded (while it loads), else to the base path. For the attributes of
// the tags made by code that name files. Returns: The path; valid until the next call.
func XmlResolvePath(path string) string {
	path_ := C.CString(path)
	defer C.free(unsafe.Pointer(path_))
	return C.GoString(C.Rxml_resolve_path(path_))
}

// WidgetSetAttribute: Sets a property from text, with the names and value syntax of the XML attributes. Handy
// for scripting and data-driven tools. Returns: R_TRUE when the attribute exists for this widget and the
// value is valid.
func WidgetSetAttribute(widget ID, name string, value string) int {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	value_ := C.CString(value)
	defer C.free(unsafe.Pointer(value_))
	return int(C.Rwidget_set_attribute(C.Rid(widget), name_, value_))
}

// XmlDocumentCreate: A new document with one element.
func XmlDocumentCreate(root_tag string) ID {
	root_tag_ := C.CString(root_tag)
	defer C.free(unsafe.Pointer(root_tag_))
	return ID(C.Rxml_document_create(root_tag_))
}

// XmlDocumentLoad: Reads a file. The paths written in it are relative to its folder.
func XmlDocumentLoad(path string) ID {
	path_ := C.CString(path)
	defer C.free(unsafe.Pointer(path_))
	return ID(C.Rxml_document_load(path_))
}

// XmlDocumentDestroy: Releases a document and its nodes. What was built from it lives on.
func XmlDocumentDestroy(document ID) {
	C.Rxml_document_destroy(C.Rid(document))
}

func XmlDocumentIsValid(document ID) int {
	return int(C.Rxml_document_is_valid(C.Rid(document)))
}

func XmlDocumentGetRoot(document ID) ID {
	return ID(C.Rxml_document_get_root(C.Rid(document)))
}

// XmlDocumentToString: Writes the document as text, one element per line, indented with tabs.
// Rxml_node_get_line then says where each node was written. Returns: The text, valid until the next call for
// this document or its destruction.
func XmlDocumentToString(document ID) string {
	return C.GoString(C.Rxml_document_to_string(C.Rid(document)))
}

func XmlDocumentSave(document ID, path string) int {
	path_ := C.CString(path)
	defer C.free(unsafe.Pointer(path_))
	return int(C.Rxml_document_save(C.Rid(document), path_))
}

// XmlDocumentCreateWidget: Builds the widgets the document describes, as Rwidget_create_from_xml does. When
// the root is a <Window>, builds what the window would show (R_INVALID_ID when it is empty).
func XmlDocumentCreateWidget(document ID) ID {
	return ID(C.Rxml_document_create_widget(C.Rid(document)))
}

// XmlDocumentCreateWindow: Builds the window the document describes, as Rwindow_create_from_xml does.
func XmlDocumentCreateWindow(document ID) ID {
	return ID(C.Rxml_document_create_window(C.Rid(document)))
}

func XmlNodeIsValid(node ID) int {
	return int(C.Rxml_node_is_valid(C.Rid(node)))
}

func XmlNodeGetDocument(node ID) ID {
	return ID(C.Rxml_node_get_document(C.Rid(node)))
}

// XmlNodeGetTag: Returns: The tag of an element, "" for a comment.
func XmlNodeGetTag(node ID) string {
	return C.GoString(C.Rxml_node_get_tag(C.Rid(node)))
}

func XmlNodeSetTag(node ID, tag string) {
	tag_ := C.CString(tag)
	defer C.free(unsafe.Pointer(tag_))
	C.Rxml_node_set_tag(C.Rid(node), tag_)
}

func XmlNodeIsComment(node ID) int {
	return int(C.Rxml_node_is_comment(C.Rid(node)))
}

// XmlNodeGetText: The text inside an element (<Label>this</Label>), or the text of a comment.
func XmlNodeGetText(node ID) string {
	return C.GoString(C.Rxml_node_get_text(C.Rid(node)))
}

func XmlNodeSetText(node ID, text string) {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	C.Rxml_node_set_text(C.Rid(node), text_)
}

func XmlNodeGetParent(node ID) ID {
	return ID(C.Rxml_node_get_parent(C.Rid(node)))
}

func XmlNodeGetChildCount(node ID) int {
	return int(C.Rxml_node_get_child_count(C.Rid(node)))
}

func XmlNodeGetChild(node ID, index int) ID {
	return ID(C.Rxml_node_get_child(C.Rid(node), C.int(index)))
}

// XmlNodeGetIndex: Returns: The place of a node among the children of its parent; 0 for the root.
func XmlNodeGetIndex(node ID) int {
	return int(C.Rxml_node_get_index(C.Rid(node)))
}

// XmlNodeAddChild: index Where among the children; -1 for the end.
func XmlNodeAddChild(parent ID, tag string, index int) ID {
	tag_ := C.CString(tag)
	defer C.free(unsafe.Pointer(tag_))
	return ID(C.Rxml_node_add_child(C.Rid(parent), tag_, C.int(index)))
}

func XmlNodeAddComment(parent ID, text string, index int) ID {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	return ID(C.Rxml_node_add_comment(C.Rid(parent), text_, C.int(index)))
}

// XmlNodeAddXml: Reads a piece of XML (one element and what it contains) and adds it: what a palette, a paste
// or a template inserts.
func XmlNodeAddXml(parent ID, xml string, index int) ID {
	xml_ := C.CString(xml)
	defer C.free(unsafe.Pointer(xml_))
	return ID(C.Rxml_node_add_xml(C.Rid(parent), xml_, C.size_t(len(xml)), C.int(index)))
}

// XmlNodeRemove: Removes a node and what it contains. The root stays.
func XmlNodeRemove(node ID) {
	C.Rxml_node_remove(C.Rid(node))
}

// XmlNodeMove: Moves a node, with what it contains, under another parent of the same document or elsewhere
// under its own. index Where among the children of the new parent, counted once the node has left its place;
// -1 for the end. Returns: R_FALSE when it cannot be done (the root, or a parent that is inside the node).
func XmlNodeMove(node ID, new_parent ID, index int) int {
	return int(C.Rxml_node_move(C.Rid(node), C.Rid(new_parent), C.int(index)))
}

// XmlNodeDuplicate: Copies a node and what it contains, right after it. Returns the copy.
func XmlNodeDuplicate(node ID) ID {
	return ID(C.Rxml_node_duplicate(C.Rid(node)))
}

// XmlNodeToString: The node as text. Valid until the next call for this document.
func XmlNodeToString(node ID) string {
	return C.GoString(C.Rxml_node_to_string(C.Rid(node)))
}

func XmlNodeGetAttributeCount(node ID) int {
	return int(C.Rxml_node_get_attribute_count(C.Rid(node)))
}

func XmlNodeGetAttributeName(node ID, index int) string {
	return C.GoString(C.Rxml_node_get_attribute_name(C.Rid(node), C.int(index)))
}

func XmlNodeGetAttributeValue(node ID, index int) string {
	return C.GoString(C.Rxml_node_get_attribute_value(C.Rid(node), C.int(index)))
}

// XmlNodeGetAttribute: Returns: The value of an attribute (names are compared without case), or NULL when the
// node does not have it.
func XmlNodeGetAttribute(node ID, name string) string {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	return C.GoString(C.Rxml_node_get_attribute(C.Rid(node), name_))
}

// XmlNodeSetAttribute: Sets an attribute, added after the others when it is new. A NULL value removes it.
func XmlNodeSetAttribute(node ID, name string, value string) {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	value_ := C.CString(value)
	defer C.free(unsafe.Pointer(value_))
	C.Rxml_node_set_attribute(C.Rid(node), name_, value_)
}

// XmlNodeGetLine: Returns: The line of the node, starting at 1, in the text the document was read from or
// last written to; 0 for a node added since.
func XmlNodeGetLine(node ID) int {
	return int(C.Rxml_node_get_line(C.Rid(node)))
}

// XmlNodeGetWidget: Returns: The widget (the window, for a <Window>) the node became the last time the
// document was built, or R_INVALID_ID: never built, destroyed since, or not a widget (an <Item>, a comment).
// A tag declared with <Widget> gives the widget it stands for.
func XmlNodeGetWidget(node ID) ID {
	return ID(C.Rxml_node_get_widget(C.Rid(node)))
}

// WidgetGetXmlNode: Returns: The node a widget was built from, or R_INVALID_ID: not built from a document, or
// the document is gone. The widgets inside a tag declared with <Widget> all come from the node where the tag
// is used.
func WidgetGetXmlNode(widget ID) ID {
	return ID(C.Rwidget_get_xml_node(C.Rid(widget)))
}

// XmlGetTagCount: The tags of the library, <Window> included. Those declared with <Widget> are listed by
// Rxml_get_registered_widget_name.
func XmlGetTagCount() int {
	return int(C.Rxml_get_tag_count())
}

func XmlGetTagName(index int) string {
	return C.GoString(C.Rxml_get_tag_name(C.int(index)))
}

// XmlGetTagDescription: Returns: One line about a tag, "" when there is nothing to say.
func XmlGetTagDescription(tag string) string {
	tag_ := C.CString(tag)
	defer C.free(unsafe.Pointer(tag_))
	return C.GoString(C.Rxml_get_tag_description(tag_))
}

// XmlGetTagChildren: Returns: 0: no child widget; 1: a single one; 2: any number. Declared and unknown tags
// give 2.
func XmlGetTagChildren(tag string) int {
	tag_ := C.CString(tag)
	defer C.free(unsafe.Pointer(tag_))
	return int(C.Rxml_get_tag_children(tag_))
}

// XmlGetAttributeCount: The attributes of a tag: its own first, then those every widget has, then those of
// its place in its parent. For a tag declared with <Widget>, its own are its parameters.
func XmlGetAttributeCount(tag string) int {
	tag_ := C.CString(tag)
	defer C.free(unsafe.Pointer(tag_))
	return int(C.Rxml_get_attribute_count(tag_))
}

// LocaleLoadCsv: Loads a CSV table of translations from a file. Returns the number of keys read, -1 on an
// error.
func LocaleLoadCsv(path string) int {
	path_ := C.CString(path)
	defer C.free(unsafe.Pointer(path_))
	return int(C.Rlocale_load_csv(path_))
}

// LocaleLoadCsvFromMemory: Same, from text in memory (UTF-8).
func LocaleLoadCsvFromMemory(csv string) int {
	csv_ := C.CString(csv)
	defer C.free(unsafe.Pointer(csv_))
	return int(C.Rlocale_load_csv_from_memory(csv_, C.size_t(len(csv))))
}

// LocaleClear: Forgets every table.
func LocaleClear() {
	C.Rlocale_clear()
}

// LocaleSetText: Adds or replaces one text of one language.
func LocaleSetText(language string, key string, text string) {
	language_ := C.CString(language)
	defer C.free(unsafe.Pointer(language_))
	key_ := C.CString(key)
	defer C.free(unsafe.Pointer(key_))
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	C.Rlocale_set_text(language_, key_, text_)
}

// LocaleGetLanguageCount: Number of languages in the tables, and their codes, in the order of the columns.
func LocaleGetLanguageCount() int {
	return int(C.Rlocale_get_language_count())
}

func LocaleGetLanguageAt(index int) string {
	return C.GoString(C.Rlocale_get_language_at(C.int(index)))
}

// LocaleSetLanguage: Chooses the language: an exact code ("pt-BR"), or its first part ("pt") when the tables
// have no such column. Every widget that shows a key is updated, and the layout of the windows follows its
// direction (an Arabic, Hebrew, Persian or Urdu language is laid out from right to left, see
// Rset_layout_direction). Returns R_FALSE when no column matches (the language is set all the same: the texts
// fall back).
func LocaleSetLanguage(code string) int {
	code_ := C.CString(code)
	defer C.free(unsafe.Pointer(code_))
	return int(C.Rlocale_set_language(code_))
}

func LocaleGetLanguage() string {
	return C.GoString(C.Rlocale_get_language())
}

// LocaleGetSystemLanguage: The language of the user, from the system ("fr-FR", "en-US"...).
func LocaleGetSystemLanguage() string {
	return C.GoString(C.Rlocale_get_system_language())
}

// LocaleSetFallbackLanguage: The language whose texts are shown when the current one has none (default: the
// first column).
func LocaleSetFallbackLanguage(code string) {
	code_ := C.CString(code)
	defer C.free(unsafe.Pointer(code_))
	C.Rlocale_set_fallback_language(code_)
}

// LocaleIsRtl: R_TRUE when the language is written from right to left.
func LocaleIsRtl() int {
	return int(C.Rlocale_is_rtl())
}

// Tr: The text of a key in the current language (see the fallbacks above). Never NULL.
func Tr(key string) string {
	key_ := C.CString(key)
	defer C.free(unsafe.Pointer(key_))
	return C.GoString(C.Rtr(key_))
}

// TrExists: R_TRUE when the key has a text in the current language or the fallback one.
func TrExists(key string) int {
	key_ := C.CString(key)
	defer C.free(unsafe.Pointer(key_))
	return int(C.Rtr_exists(key_))
}

// TrPlural: The plural form of a key for a count: the key followed by the CLDR category of the count in the
// current language (".zero", ".one", ".two", ".few", ".many", ".other"), else ".other", else the key itself.
// {n} is replaced by the count, written in the conventions of the language.
func TrPlural(key string, count int) string {
	key_ := C.CString(key)
	defer C.free(unsafe.Pointer(key_))
	return C.GoString(C.Rtr_plural(key_, C.int(count)))
}

// WidgetSetTextKey: Makes a widget show a key: an attribute ("Text", "Tooltip", "Title", "Placeholder"... any
// attribute that takes a text) is set to the translation now and every time the language changes. NULL or ""
// stops it. Same as Rwidget_set_attribute(widget, attribute, "@key").
func WidgetSetTextKey(widget ID, attribute string, key string) {
	attribute_ := C.CString(attribute)
	defer C.free(unsafe.Pointer(attribute_))
	key_ := C.CString(key)
	defer C.free(unsafe.Pointer(key_))
	C.Rwidget_set_text_key(C.Rid(widget), attribute_, key_)
}

// LocaleFormatNumber: A number written in the conventions of the language: decimal separator and digit
// grouping ("1 234,5" in French).
func LocaleFormatNumber(value float64, decimals int) string {
	return C.GoString(C.Rlocale_format_number(C.double(value), C.int(decimals)))
}

// AccessibilitySetTextScale: Size of every text, 1 = as designed (0.5 to 4). Controls grow with their text.
func AccessibilitySetTextScale(scale float32) {
	C.Raccessibility_set_text_scale(C.float(scale))
}

func AccessibilityGetTextScale() float32 {
	return float32(C.Raccessibility_get_text_scale())
}

// AccessibilitySetHighContrast: Themes of high contrast: black and white, yellow focus, thick borders (the
// dark one, or the light one when the theme in use is light). R_FALSE goes back to the theme used before.
func AccessibilitySetHighContrast(enabled int) {
	C.Raccessibility_set_high_contrast(C.int(enabled))
}

func AccessibilityGetHighContrast() int {
	return int(C.Raccessibility_get_high_contrast())
}

// AccessibilitySetFocusAlwaysVisible: R_TRUE: the focus ring is always shown (not only after the keyboard was
// used), and thicker.
func AccessibilitySetFocusAlwaysVisible(enabled int) {
	C.Raccessibility_set_focus_always_visible(C.int(enabled))
}

func AccessibilityGetFocusAlwaysVisible() int {
	return int(C.Raccessibility_get_focus_always_visible())
}

// AccessibilitySetReduceMotion: Reduced motion, for the whole application: R_TRUE and nothing moves by
// itself. Changes are immediate (no spring, no slide, no bounce, no transition), and the effects
// (raven_shader.h) stand still; they still follow the pointer. Same as Rset_animations_enabled(R_FALSE).
func AccessibilitySetReduceMotion(enabled int) {
	C.Raccessibility_set_reduce_motion(C.int(enabled))
}

func AccessibilityGetReduceMotion() int {
	return int(C.Raccessibility_get_reduce_motion())
}

// AccessibilitySetColorFilter: A filter on the colors of the interface (pictures and viewports keep theirs).
func AccessibilitySetColorFilter(filter RColorFilter) {
	C.Raccessibility_set_color_filter(C.RColorFilter(filter))
}

func AccessibilityGetColorFilter() RColorFilter {
	return RColorFilter(C.Raccessibility_get_color_filter())
}

// AccessibilityFollowSystem: Applies the settings of the system (text size, contrast, motion).
func AccessibilityFollowSystem() {
	C.Raccessibility_follow_system()
}

// WidgetSetAccessibleName: What the screen reader says of the widget; NULL: what it shows (or the label
// before it).
func WidgetSetAccessibleName(widget ID, name string) {
	name_ := C.CString(name)
	defer C.free(unsafe.Pointer(name_))
	C.Rwidget_set_accessible_name(C.Rid(widget), name_)
}

// WidgetGetAccessibleName: The name the screen reader uses (given, or found). Valid until the next call.
func WidgetGetAccessibleName(widget ID) string {
	return C.GoString(C.Rwidget_get_accessible_name(C.Rid(widget)))
}

// WidgetSetAccessibleDescription: A longer explanation, read after the name.
func WidgetSetAccessibleDescription(widget ID, description string) {
	description_ := C.CString(description)
	defer C.free(unsafe.Pointer(description_))
	C.Rwidget_set_accessible_description(C.Rid(widget), description_)
}

func WidgetSetAccessibleRole(widget ID, role RAccessibleRole) {
	C.Rwidget_set_accessible_role(C.Rid(widget), C.RAccessibleRole(role))
}

// WidgetGetAccessibleRole: The role the screen reader sees (given, or the one of the type).
func WidgetGetAccessibleRole(widget ID) RAccessibleRole {
	return RAccessibleRole(C.Rwidget_get_accessible_role(C.Rid(widget)))
}

// WidgetSetAccessibleHidden: R_TRUE: hidden from screen readers, with what it contains (decoration).
func WidgetSetAccessibleHidden(widget ID, hidden int) {
	C.Rwidget_set_accessible_hidden(C.Rid(widget), C.int(hidden))
}

// WidgetSetAccessibleLabelledBy: The widget is named by another one (a label): its text is the name.
func WidgetSetAccessibleLabelledBy(widget ID, label ID) {
	C.Rwidget_set_accessible_labelled_by(C.Rid(widget), C.Rid(label))
}

// AccessibilityAnnounce: Has a message read (a save that ended, an error). `assertive`: interrupts what is
// being read.
func AccessibilityAnnounce(text string, assertive int) {
	text_ := C.CString(text)
	defer C.free(unsafe.Pointer(text_))
	C.Raccessibility_announce(text_, C.int(assertive))
}

// AccessibilityIsScreenReaderActive: R_TRUE when a screen reader is reading the windows of the application.
func AccessibilityIsScreenReaderActive() int {
	return int(C.Raccessibility_is_screen_reader_active())
}

// HostPlatformIsActive: Returns: R_TRUE while the running library uses the host platform.
func HostPlatformIsActive() int {
	return int(C.Rhost_platform_is_active())
}

// HostWindowSetSize: Size of a host window in pixels, and pixels per logical unit.
func HostWindowSetSize(window ID, width int, height int, scale float32) {
	C.Rhost_window_set_size(C.Rid(window), C.int(width), C.int(height), C.float(scale))
}

func HostInjectMouseMove(window ID, x int, y int, modifiers uint32) {
	C.Rhost_inject_mouse_move(C.Rid(window), C.int(x), C.int(y), C.Ruint(modifiers))
}

func HostInjectMouseButton(window ID, x int, y int, button RMouseButton, pressed int, modifiers uint32) {
	C.Rhost_inject_mouse_button(C.Rid(window), C.int(x), C.int(y), C.RMouseButton(button), C.int(pressed), C.Ruint(modifiers))
}

func HostInjectMouseWheel(window ID, wheel_x float32, wheel_y float32, modifiers uint32) {
	C.Rhost_inject_mouse_wheel(C.Rid(window), C.float(wheel_x), C.float(wheel_y), C.Ruint(modifiers))
}

// HostInjectMouseLeave: The pointer left the window: hover states end.
func HostInjectMouseLeave(window ID) {
	C.Rhost_inject_mouse_leave(C.Rid(window))
}

// HostInjectKey: repeat R_TRUE for a key held down (auto-repeat).
func HostInjectKey(window ID, key RKey, pressed int, modifiers uint32, repeat int) {
	C.Rhost_inject_key(C.Rid(window), C.RKey(key), C.int(pressed), C.Ruint(modifiers), C.int(repeat))
}

// HostInjectText: One typed Unicode code point.
func HostInjectText(window ID, codepoint uint32) {
	C.Rhost_inject_text(C.Rid(window), C.Ruint(codepoint))
}

// HostInjectFocus: The window gained or lost the keyboard. Losing it closes popups and drops captures.
func HostInjectFocus(window ID, focused int) {
	C.Rhost_inject_focus(C.Rid(window), C.int(focused))
}

// HostInjectClose: The user asked to close the window (its close callback decides).
func HostInjectClose(window ID) {
	C.Rhost_inject_close(C.Rid(window))
}

// Left to cgo (a pointer to a structure, a void* or a C callback among the parameters):
// Rinit_ex, Rset_allocator, Rallocator_get_system, Rmemory_alloc, Rmemory_realloc, Rmemory_free
// Rmemory_get_stats, Rset_frame_callback, Rtask_post, Rtask_run_async, Rparallel_for, Rtimer_create
// Raction_bind, Raction_set_default_handler, Rerror_get_last, Rerror_set_callback
// Rset_device_reset_callback, Rwindow_get_frame_stats, Rwindow_set_icon, Rwindow_set_close_callback
// Rwindow_set_resize_callback, Rwindow_set_drop_callback, Rwindow_set_user_handle, Rwindow_get_user_handle
// Rwindow_get_native_handle, Rwindow_capture, Rwindow_inject_drop, Rwindow_set_event_hook
// Rtexture_create_from_pixels, Rtexture_update, Rimage_decode, Rpixels_free, Rwidget_set_user_handle
// Rwidget_get_user_handle, Rwidget_set_destroy_callback, Rwidget_set_event_callback
// Rwidget_set_drop_callback, Rtab_view_set_changed_callback, Rtab_view_set_close_callback
// Rexpander_set_changed_callback, Rlabel_set_link_callback, Rbutton_set_clicked_callback
// Rcheckbox_set_changed_callback, Rslider_set_changed_callback, Rtext_input_set_changed_callback
// Rtext_input_set_committed_callback, Rspin_box_set_changed_callback, Rcombo_box_set_changed_callback
// Rlist_view_set_virtual_items, Rlist_view_set_selection_changed_callback, Rlist_view_set_activated_callback
// Rtree_item_set_user_handle, Rtree_item_get_user_handle, Rtree_view_set_selection_changed_callback
// Rtree_view_set_activated_callback, Rtree_view_set_context_callback, Rmenu_item_set_clicked_callback
// Rtile_view_set_items, Rtile_view_set_drag_data_callback, Rtile_view_set_selection_changed_callback
// Rtile_view_set_activated_callback, Rtile_view_set_context_callback, Rtext_editor_set_zoom_callback
// Rlanguage_register, Rlanguage_get, Rtext_editor_set_completion_callback, Rtext_editor_set_changed_callback
// Rdock_area_set_close_callback, Rdock_area_set_changed_callback, Risland_set_changed_callback
// Rcolor_picker_set_changed_callback, Rtimeline_set_time_callback, Rtimeline_set_changed_callback
// Rtimeline_set_selection_callback, Rtimeline_set_activated_callback, Rtimeline_set_removed_callback
// Rnode_graph_set_node_user_data, Rnode_graph_get_node_user_data, Rnode_graph_set_connect_callback
// Rnode_graph_set_linked_callback, Rnode_graph_set_unlinked_callback, Rnode_graph_set_moved_callback
// Rnode_graph_set_selection_callback, Rnode_graph_set_removed_callback, Rnode_graph_set_activated_callback
// Rnode_graph_set_context_callback, Rblock_editor_set_changed_callback, Rblock_editor_set_clicked_callback
// Rblock_editor_set_context_callback, Rdialog_open, Rfile_dialog_open, Rfile_dialog_save
// Rfile_dialog_select_folder, Rfile_read, Rfile_browser_set_filters, Rfile_browser_set_accepted_callback
// Rfile_browser_set_cancelled_callback, Rfile_browser_set_selection_changed_callback
// Rviewport_set_draw_callback, Rviewport_get_native_handle, Rviewport_set_resize_callback
// Rcustom_set_paint_callback, Rcustom_set_measure_callback, Rdraw_rect, Rdraw_rect_outline, Rdraw_circle
// Rdraw_line, Rdraw_curve, Rdraw_triangle, Rdraw_image, Rdraw_text, Rdraw_glass, Rdraw_spring, Rspring_kick
// Rspring_set, Rdraw_push_clip_rect, Rdraw_pop_clip_rect, Reffect_get_defaults, Rshader_look_init
// Rshader_look_defaults, Rdraw_shader, Rborder_set_effect, Rxml_set_reload_callback, Rxml_register_tag
// Rxml_document_parse, Rxml_get_attribute_info, Rlocale_set_changed_callback, Rtr_format
// Raccessibility_get_system_settings, Rhost_platform_register

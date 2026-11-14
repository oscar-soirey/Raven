// Raven for Odin: the enumerations of the C API.
//
// Generated from the C headers by tools/bindgen (Raven 0.2): do not edit.
//
// Values are those of the C constants, so the raw i32 of the generated package converts both
// ways: Widget_Type(raw.Rwidget_get_type(widget)). Groups of flags are bit_sets (Window_Flags);
// the member that is 0 in C is the empty set {}.
package raven_odin

Backend :: enum i32 {
	// Built-in Vulkan renderer.
	Vulkan = 1,
	// Renders nothing. Used for headless runs and unit tests.
	Null = 2,
	// Backend supplied with Rbackend_register (see raven_backend.h).
	Custom = 3,
	// Browser renderer (Emscripten, WebGL 2).
	Webgl = 4,
}

Error :: enum i32 {
	No_Error = 0x0070,
	Error_Invalid_Id = 0x0071,
	Error_Invalid_Enum = 0x0072,
	Error_Invalid_Value = 0x0073,
	Error_Invalid_Operation = 0x0074,
	Error_Backend = 0x0075,
	Error_Platform = 0x0076,
	Error_Out_Of_Memory = 0x0077,
	Error_Invalid_File_Format = 0x0078,
	Error_File_Not_Found = 0x0079,
	Error_Xml = 0x007A,
	Error_Wrong_Thread = 0x007B,
}

Severity :: enum i32 {
	Weak_Warning = 0x0080,
	Warning = 0x0081,
	Error = 0x0082,
	Fatal = 0x0083,
}

Window_Flag :: enum u32 {
	No_Resize = 0,
	Hidden = 1,
	Borderless = 2,
	Maximized = 3,
	// No title bar from the OS: the content covers the whole window and
	// draws its own header, with a R_WIDGET_HEADER_BAR (raven_shell.h) or
	// with any widget given to Rwidget_set_window_drag. The window is still
	// moved, resized, snapped and maximized by the OS, as it does for its own
	// title bars.
	Custom_Header = 4,
	// The window can be seen through: where the application draws nothing
	// opaque, what is behind the window on the screen shows (the desktop,
	// the other applications), not a color of the application. Glass then
	// looks at the screen itself. What the OS does to what shows through is
	// chosen with Rwindow_set_backdrop. See RBackdrop for what each window
	// system can do.
	Transparent = 5,
	// A splash screen: shown while the application starts. No frame (it
	// implies R_WINDOW_BORDERLESS and R_WINDOW_NO_RESIZE, and cancels
	// R_WINDOW_MAXIMIZED and R_WINDOW_CUSTOM_HEADER), centered on the screen
	// before it is first shown and again when its size changes, above the
	// other windows, out of the taskbar and of Alt+Tab (Windows), of the
	// Window menu and of Cmd+` (macOS), typed _NET_WM_WINDOW_TYPE_SPLASH on
	// X11. Wayland gives it no place of its own: it is a window without frame
	// there. The application destroys it once its main window is ready
	// (creating the main window first keeps Ris_running true).
	Splash_Screen = 6,
}
Window_Flags :: bit_set[Window_Flag; u32]
// R_WINDOW_DEFAULT (0 in C) is {}

// Where a window in picture in picture goes (Rwindow_set_picture_in_picture_layout).
Picture_In_Picture_Corner :: enum i32 {
	Bottom_Right = 0x0780,
	Bottom_Left = 0x0781,
	Top_Right = 0x0782,
	Top_Left = 0x0783,
}

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
Backdrop :: enum i32 {
	// Opaque: the window color, or the wallpaper of the theme.
	None = 0x0400,
	// The screen behind the window, as it is.
	Transparent = 0x0401,
	// The screen behind the window, blurred by the OS.
	Blur = 0x0402,
	// Blurred, tinted and grained by the OS (the acrylic of Windows).
	Acrylic = 0x0403,
	// A soft tint taken from the wallpaper of the desktop (the mica of Windows 11).
	Mica = 0x0404,
}

Window_System :: enum i32 {
	None = 0x0090,
	Win32 = 0x0091,
	X11 = 0x0092,
	Wayland = 0x0093,
	Cocoa = 0x0094,
	Emscripten = 0x0095,
	Android = 0x0096,
	Uikit = 0x0097,
}

// What a text input holds: on mobile it picks the on-screen keyboard (letters, @, digits...).
Text_Input_Type :: enum i32 {
	Text = 0x0B60,
	Email = 0x0B61,
	Number = 0x0B62,
	// Digits and a decimal separator.
	Decimal = 0x0B63,
	Phone = 0x0B64,
	Url = 0x0B65,
	Search = 0x0B66,
}

// The label and meaning of the return key of the on-screen keyboard. Enter does the same on every key.
Return_Key :: enum i32 {
	Default = 0x0B70,
	Done = 0x0B71,
	Go = 0x0B72,
	Next = 0x0B73,
	Search = 0x0B74,
	Send = 0x0B75,
}

// Edges and corners of a window, for Rwindow_begin_resize.
Window_Edge :: enum i32 {
	Left = 0x00D0,
	Right = 0x00D1,
	Top = 0x00D2,
	Bottom = 0x00D3,
	Top_Left = 0x00D4,
	Top_Right = 0x00D5,
	Bottom_Left = 0x00D6,
	Bottom_Right = 0x00D7,
}

Mouse_Button :: enum i32 {
	Left = 0,
	Right = 1,
	Middle = 2,
	X1 = 3,
	X2 = 4,
}

Modifier :: enum u32 {
	Shift = 0,
	Ctrl = 1,
	Alt = 2,
	Super = 3,
}
Modifiers :: bit_set[Modifier; u32]

Cursor :: enum i32 {
	Arrow = 0x00B0,
	Ibeam = 0x00B1,
	Hand = 0x00B2,
	Crosshair = 0x00B3,
	Resize_H = 0x00B4,
	Resize_V = 0x00B5,
	Resize_Nwse = 0x00B6,
	Resize_Nesw = 0x00B7,
	Move = 0x00B8,
	Not_Allowed = 0x00B9,
	// No pointer at all (a game that hides it). Windows; elsewhere the arrow.
	None = 0x00BA,
}

// Keyboard keys. Printable keys use their upper-case ASCII code.
Key :: enum i32 {
	Unknown = 0,
	Space = 0x0020,
	Apostrophe = 0x0027,
	Comma = 0x002C,
	Minus = 0x002D,
	Period = 0x002E,
	Slash = 0x002F,
	_0 = 0x0030,
	_1 = 0x0031,
	_2 = 0x0032,
	_3 = 0x0033,
	_4 = 0x0034,
	_5 = 0x0035,
	_6 = 0x0036,
	_7 = 0x0037,
	_8 = 0x0038,
	_9 = 0x0039,
	Semicolon = 0x003B,
	Equal = 0x003D,
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
	Left_Bracket = 0x005B,
	Backslash = 0x005C,
	Right_Bracket = 0x005D,
	Grave = 0x0060,
	Escape = 0x0100,
	Enter = 0x0101,
	Tab = 0x0102,
	Backspace = 0x0103,
	Insert = 0x0104,
	Delete = 0x0105,
	Right = 0x0106,
	Left = 0x0107,
	Down = 0x0108,
	Up = 0x0109,
	Page_Up = 0x010A,
	Page_Down = 0x010B,
	Home = 0x010C,
	End = 0x010D,
	Caps_Lock = 0x010E,
	Print_Screen = 0x010F,
	Pause = 0x0110,
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
	Kp_0 = 0x011D,
	Kp_1 = 0x011E,
	Kp_2 = 0x011F,
	Kp_3 = 0x0120,
	Kp_4 = 0x0121,
	Kp_5 = 0x0122,
	Kp_6 = 0x0123,
	Kp_7 = 0x0124,
	Kp_8 = 0x0125,
	Kp_9 = 0x0126,
	Kp_Decimal = 0x0127,
	Kp_Divide = 0x0128,
	Kp_Multiply = 0x0129,
	Kp_Subtract = 0x012A,
	Kp_Add = 0x012B,
	Kp_Enter = 0x012C,
	Left_Shift = 0x012D,
	Left_Ctrl = 0x012E,
	Left_Alt = 0x012F,
	Left_Super = 0x0130,
	Right_Shift = 0x0131,
	Right_Ctrl = 0x0132,
	Right_Alt = 0x0133,
	Right_Super = 0x0134,
	Menu = 0x0135,
	Count = 0x0136,
}

Event_Type :: enum i32 {
	None = 0x00C0,
	Mouse_Move = 0x00C1,
	Mouse_Down = 0x00C2,
	Mouse_Up = 0x00C3,
	Mouse_Wheel = 0x00C4,
	Mouse_Enter = 0x00C5,
	Mouse_Leave = 0x00C6,
	Key_Down = 0x00C7,
	Key_Up = 0x00C8,
	// A character was typed. `codepoint` holds the Unicode scalar value.
	Text = 0x00C9,
	Focus_Gained = 0x00CA,
	Focus_Lost = 0x00CB,
}

// Moments of a drag coming from another application (files from the file manager, text...).
Drop_Phase :: enum i32 {
	// The pointer came over the target.
	Enter = 0x00E0,
	// It moves over the target.
	Move = 0x00E1,
	// It left the target without dropping.
	Leave = 0x00E2,
	// The user dropped: `paths` and `text` hold what was dropped.
	Dropped = 0x00E3,
}

// What a finger does on a widget (Rwidget_set_touch_mode).
Touch_Mode :: enum i32 {
	// The finger scrolls what scrolls under it, and is a mouse elsewhere (default).
	Auto = 0x0480,
	// A finger moving on the widget always scrolls what contains it, even over a control that drags.
	Scroll = 0x0481,
	// The finger is a mouse on the widget: pressing presses at once, moving drags, nothing scrolls.
	Mouse = 0x0482,
	// Fingers are ignored on the widget (only its touch callback sees them): a surface for the pen.
	None = 0x0483,
}

Touch_Phase :: enum i32 {
	// A finger came down.
	Down = 0x0490,
	Move = 0x0491,
	// The finger was lifted.
	Up = 0x0492,
	// The system took the finger away (a gesture of its own, the window lost it): nothing must happen as for a lift.
	Cancel = 0x0493,
}

Pen_Phase :: enum i32 {
	// The tip touched the screen.
	Down = 0x04A0,
	// The pen moves while touching.
	Move = 0x04A1,
	// The tip left the screen.
	Up = 0x04A2,
	// The pen moves near the screen without touching it.
	Hover = 0x04A3,
	// The pen went out of reach.
	Leave = 0x04A4,
}

// Buttons on the side of a pen (RPenEvent::buttons).
Pen_Button :: enum u32 {
	Barrel = 0,
	Barrel2 = 1,
}
Pen_Buttons :: bit_set[Pen_Button; u32]

Gesture_Phase :: enum i32 {
	// A second finger came down (a touch screen), or the system recognized a gesture (a trackpad).
	Begin = 0x04B0,
	Update = 0x04B1,
	// Fewer than two fingers remain, or the fingers left the trackpad (RGestureEvent::cancelled: the system took the gesture back).
	End = 0x04B2,
}

// What the fingers of a gesture do (RGestureEvent::kind).
Gesture_Kind :: enum i32 {
	// Two fingers that pinch, turn and move together: scale, rotation and pan.
	Pinch = 0x04C0,
	// Three or more fingers that move together on a trackpad: only the pan
	// (scale stays 1). By default, a swipe across goes back or forward.
	Swipe = 0x04C1,
}

// Where the fingers of a gesture are (RGestureEvent::device).
Gesture_Device :: enum i32 {
	// On the screen: the library recognized the gesture.
	Touchscreen = 0x04D0,
	// On a trackpad: the system recognized the gesture; the fingers are not on the window (x, y is the pointer).
	Trackpad = 0x04D1,
}

// The looks. Each one comes dark and light.
//
// Raven's looks are the default; the ones of the platforms imitate them
// closely (their shapes, colors, sizes, states and motion), without using
// their widgets. The looks of the platforms are opaque, except macOS Glass
// (macOS 26), whose bars, menus and floating surfaces are glass as on the
// system; the glass materials themselves (Rtheme_set_material) belong to
// the glass looks of Raven.
Look :: enum i32 {
	// Raven, flat and dense, for tools. The look at start.
	Raven_Dark = 0x00E0,
	Raven_Light = 0x00E1,
	// Raven in glass: controls are capsules of translucent glass that refract
	// and blur what is behind them, catch the light on their rim and react
	// to the pointer with short animations. Windows get a soft colored
	// background unless they have their own (Rwindow_set_background,
	// Rwindow_set_clear_color).
	Raven_Glass_Dark = 0x00E2,
	Raven_Glass_Light = 0x00E3,
	// High contrast, for low vision: pure black and white, yellow (or blue)
	// focus and selection, thick borders, larger text (raven_accessibility.h,
	// Raccessibility_set_high_contrast).
	Raven_High_Contrast_Dark = 0x00E4,
	Raven_High_Contrast_Light = 0x00E5,
	// Windows 11 (Fluent): its controls, its colors, the accent color the
	// user chose (on Windows), the Segoe UI font when the system has it, and
	// motion without bounce.
	Windows_Dark = 0x00E6,
	Windows_Light = 0x00E7,
	// macOS (Sequoia, Aqua): its controls, its colors, the accent color the
	// user chose (on macOS), the San Francisco font when the system has it,
	// no change under the pointer, and selections in the accent color.
	Macos_Dark = 0x00E8,
	Macos_Light = 0x00E9,
	// GNOME (libadwaita), the desktop of most Linux distributions: flat
	// rounded controls, the accent and the font of the desktop (on Linux).
	Gnome_Dark = 0x00EA,
	Gnome_Light = 0x00EB,
	// The look of the platform the application runs on: Windows on Windows,
	// macOS on macOS (macOS Glass from macOS 26 on), GNOME on Linux and the
	// other systems.
	Native_Dark = 0x00EC,
	Native_Light = 0x00ED,
	// macOS 26 (Tahoe, Liquid Glass): the controls of macOS, larger and
	// rounder, and glass where the system puts it, on what floats above the
	// content: header bars and their buttons, menus, drop-down lists,
	// tooltips, sheets; the knobs of sliders and switches turn into a lens
	// of clear glass while they are held. The content itself (fields, lists,
	// text) stays opaque. The accent color and the font of the system, on
	// macOS.
	Macos_Glass_Dark = 0x00EE,
	Macos_Glass_Light = 0x00EF,
}

// @deprecated The built-in themes of Raven 0.2, now looks: R_THEME_DARK is
// R_LOOK_RAVEN_DARK and so on, and Rtheme_apply is Rlook_set.
Theme :: enum i32 {
	// Flat, dense themes for tools.
	Dark = 0x00E0,
	Light = 0x00E1,
	// Glass themes: controls are capsules of translucent glass that refract
	// and blur what is behind them, catch the light on their rim and react
	// to the pointer with short animations. Windows get a soft colored
	// background unless they have their own (Rwindow_set_background,
	// Rwindow_set_clear_color).
	Glass_Dark = 0x00E2,
	Glass_Light = 0x00E3,
	// High contrast, for low vision: pure black and white, yellow (or blue)
	// focus and selection, thick borders, larger text (raven_accessibility.h,
	// Raccessibility_set_high_contrast).
	High_Contrast_Dark = 0x00E4,
	High_Contrast_Light = 0x00E5,
}

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
Material :: enum i32 {
	// Opaque: filled with the colors of the theme. The material of R_THEME_DARK and R_THEME_LIGHT.
	Solid = 0x02E0,
	// Clear glass: what is behind stays sharp enough to be recognized, strongly bent at the rim.
	Glass = 0x02E1,
	// Frosted glass: what is behind is blurred into soft colors. The material of the glass themes.
	Frosted_Glass = 0x02E2,
	// Frosted glass colored by the accent color of the theme (or of the widget): for what must stand out.
	Tinted_Glass = 0x02E3,
	// Not a material: "the one of the theme". Removes the material given to a widget.
	Default = 0x02E4,
}

// Color roles. The comment lists the widgets reading each role.
Color_Role :: enum i32 {
	// Window clear color.
	Window = 0x0100,
	// Border background.
	Panel = 0x0101,
	// Border outline, list and input outlines.
	Border = 0x0102,
	// Text. Inherited by the children of the widget it is set on.
	Text = 0x0103,
	Text_Disabled = 0x0104,
	Text_Placeholder = 0x0105,
	// Primary buttons, checked boxes, slider fill, progress fill.
	Accent = 0x0106,
	Accent_Hovered = 0x0107,
	Accent_Pressed = 0x0108,
	// Text and marks drawn on top of the accent color.
	Accent_Text = 0x0109,
	// Button, combo box, check box background.
	Control = 0x010A,
	Control_Hovered = 0x010B,
	Control_Pressed = 0x010C,
	Control_Border = 0x010D,
	// Text input, spin box, list and tree background.
	Input = 0x010E,
	// Keyboard focus outline.
	Focus = 0x010F,
	// Selected rows and selected text.
	Selection = 0x0110,
	Row_Hovered = 0x0111,
	// Slider track, progress background, scrollbar track.
	Track = 0x0112,
	// Scrollbar thumb, slider handle.
	Handle = 0x0113,
	Handle_Hovered = 0x0114,
	Separator = 0x0115,
	// Menu bar, tab bar, expander header.
	Header = 0x0116,
	Header_Hovered = 0x0117,
	// Active tab.
	Tab_Active = 0x0118,
	// Menus, combo box lists.
	Popup = 0x0119,
	Popup_Border = 0x011A,
	Shadow = 0x011B,
	Tooltip = 0x011C,
	Tooltip_Text = 0x011D,
	// Track of a switch that is on.
	Switch = 0x011E,
	// What went well, what needs attention, what is wrong or destroys something (markers, badges, classes).
	Success = 0x011F,
	Warning = 0x0120,
	Danger = 0x0121,
	// Links of formatted text.
	Link = 0x0122,
	// Background of code in formatted text.
	Code = 0x0123,
}

// Sizes, in logical units.
Metric :: enum i32 {
	Font_Size = 0x0140,
	Corner_Radius = 0x0141,
	Border_Width = 0x0142,
	// Height of buttons, inputs, combo boxes, tabs, menu bars.
	Control_Height = 0x0143,
	// Horizontal / vertical padding inside controls.
	Padding_X = 0x0144,
	Padding_Y = 0x0145,
	// Gap between an icon or a check mark and its text.
	Spacing = 0x0146,
	Scrollbar_Size = 0x0147,
	// Height of list, tree and menu rows.
	Row_Height = 0x0148,
	// Indentation per tree level.
	Indent = 0x0149,
	Splitter_Size = 0x014A,
	Check_Size = 0x014B,
	Slider_Handle_Size = 0x014C,
	// Corner radius of menus, drop-down lists and tooltips.
	Popup_Radius = 0x014D,
	// The glass of the theme: each one multiplies what the material gives,
	// 1 = unchanged. Frost (how much is blurred), refraction (how far the rim
	// bends what is behind), saturation (of what is seen through), rim (the
	// light it catches) and shadow (the one it drops).
	Glass_Frost = 0x014E,
	Glass_Refraction = 0x014F,
	Glass_Saturation = 0x0150,
	Glass_Rim = 0x0151,
	Glass_Shadow = 0x0152,
	// Radius of the blur of a BackgroundBlur that was not given one.
	Blur_Radius = 0x0153,
}

// The styles of a font (Rfont_get_variant).
Font_Style :: enum i32 {
	Regular = 0x0420,
	Bold = 0x0421,
	Italic = 0x0422,
	Bold_Italic = 0x0423,
}

Filter_Type :: enum i32 {
	Linear = 0x02C0,
	// No smoothing: keeps pixel art sharp.
	Nearest = 0x02C1,
	// Smooth, with mipmaps: an image shown much smaller than its size stays
	// clean instead of shimmering, for a third more GPU memory. The default
	// of the textures made from image files (Rtexture_create...). Backends
	// without mipmaps treat it as R_FILTER_LINEAR.
	Mipmap = 0x02C2,
}

Widget_Type :: enum i32 {
	// Stacks children top to bottom. Slate: SVerticalBox.
	Vbox = 0x0200,
	// Stacks children left to right. Slate: SHorizontalBox.
	Hbox = 0x0201,
	// Rows and columns with spans and fill coefficients. Slate: SGridPanel.
	Grid = 0x0202,
	// Grid whose cells all have the same size. Slate: SUniformGridPanel.
	Uniform_Grid = 0x0203,
	// Draws children on top of each other. Slate: SOverlay.
	Overlay = 0x0204,
	// One child with a background, an outline and inner padding. Slate: SBorder.
	Border = 0x0205,
	// One child with size constraints or a fixed aspect ratio. Slate: SBox.
	Size_Box = 0x0206,
	// Empty space. Slate: SSpacer.
	Spacer = 0x0207,
	// Scrollable stack of children. Slate: SScrollBox.
	Scroll_Box = 0x0208,
	// Children separated by draggable handles. Slate: SSplitter.
	Splitter = 0x0209,
	// Flows children and wraps to the next line. Slate: SWrapBox.
	Wrap_Box = 0x020A,
	// Children placed with anchors and offsets. Slate: SConstraintCanvas.
	Canvas = 0x020B,
	// Shows one child at a time. Slate: SWidgetSwitcher.
	Switcher = 0x020C,
	// Tab bar + one page per child.
	Tab_View = 0x020D,
	// Collapsible header + one child. Slate: SExpandableArea.
	Expander = 0x020E,
	Label = 0x020F,
	Button = 0x0210,
	Checkbox = 0x0211,
	Slider = 0x0212,
	Progressbar = 0x0213,
	Image = 0x0214,
	Text_Input = 0x0215,
	Spin_Box = 0x0216,
	Combo_Box = 0x0217,
	Separator = 0x0218,
	List_View = 0x0219,
	Tree_View = 0x021A,
	Menu_Bar = 0x021B,
	Menu = 0x021C,
	Menu_Item = 0x021D,
	// Area showing content rendered by the application.
	Viewport = 0x021E,
	// Widget painted and measured by application callbacks.
	Custom = 0x021F,
	// Multi-line text editor.
	Text_Editor = 0x0220,
	// Title bar of a window with a custom header, holding any widget.
	Header_Bar = 0x0221,
	// Panels docked side by side or behind tabs, that the user rearranges.
	Dock_Area = 0x0222,
	// A capsule that unfolds into a panel, at the top of a window.
	Island = 0x0223,
	// Explorer of the files of the computer.
	File_Browser = 0x0224,
	// One child over a blurred view of what is behind it.
	Background_Blur = 0x0225,
	// Chooses a color: hue, saturation, value, opacity, by hand or by numbers.
	Color_Picker = 0x0226,
	// Tracks of clips and keys along a time ruler.
	Timeline = 0x0227,
	// Nodes linked by wires, on a canvas that pans and zooms.
	Node_Graph = 0x0228,
	// Grid of tiles (an icon over a name), virtualized. Slate: STileView.
	Tile_View = 0x0229,
	// Blocks that snap into each other, in the style of Scratch.
	Block_Editor = 0x022A,
	// Rows of cells under columns that can be resized and sorted, virtualized.
	Table = 0x022B,
	// A date, chosen in a calendar that drops down.
	Date_Picker = 0x022C,
	// A time of the day: hours, minutes, optionally seconds.
	Time_Picker = 0x022D,
	// A short message that rises for a few seconds, with an optional action ("Undo").
	Toast = 0x022E,
	// The path to the current place: links separated by chevrons.
	Breadcrumb = 0x022F,
	// A score shown and chosen with stars.
	Rating = 0x0230,
	// Page numbers with previous and next.
	Pagination = 0x0231,
	// Curves of X / Y data: legend, line styles, markers, log axes, a second X and Y axis, pan and zoom.
	Plot = 0x0232,
	// The pages of a PDF document, scrolled and zoomed (drawn by PDFium, loaded at run time).
	Pdf_View = 0x0233,
}

H_Align :: enum i32 {
	Fill = 0x0240,
	Left = 0x0241,
	Center = 0x0242,
	Right = 0x0243,
}

V_Align :: enum i32 {
	Fill = 0x0250,
	Top = 0x0251,
	Center = 0x0252,
	Bottom = 0x0253,
}

Visibility :: enum i32 {
	// Drawn and laid out.
	Visible = 0x0260,
	// Not drawn, but still takes its space in the layout.
	Hidden = 0x0261,
	// Not drawn and takes no space.
	Collapsed = 0x0262,
}

// How a widget arrives. A transition animates what the widget and its
// children draw; the layout is not affected, the widget is in place from the
// first frame for its neighbours and for the mouse.
Transition :: enum i32 {
	None = 0x0300,
	// Becomes opaque.
	Fade = 0x0301,
	// Grows from a smaller size and bounces a little past its own.
	Pop = 0x0302,
	// Shrinks from a larger size.
	Zoom = 0x0303,
	// Flies in from below / above / the right / the left, and settles with a bounce.
	Fly_Up = 0x0304,
	Fly_Down = 0x0305,
	Fly_Left = 0x0306,
	Fly_Right = 0x0307,
	// Falls from above, squashes on landing and wobbles.
	Drop = 0x0308,
	// Short slide from the right / from the left, without bounce: pages that replace one another.
	Slide_Left = 0x0309,
	Slide_Right = 0x030A,
}

Orientation :: enum i32 {
	Horizontal = 0x0270,
	Vertical = 0x0271,
}

// The looks of a R_WIDGET_BACKGROUND_BLUR. The first three are drawn by
// Raven from what the window shows behind the widget; the last one is the
// blur of the OS, which sees what is behind the window itself.
Blur_Style :: enum i32 {
	// What is behind, out of focus, and nothing else (a Gaussian blur).
	Plain = 0x0440,
	// Frosted: blurred, its colors a little stronger, under a veil of the
	// panel color of the theme, a fine grain and a hairline. The default.
	Frosted = 0x0441,
	// Glass: a slab that bends what is behind at its rim and catches the
	// light, as the glass materials do (RMaterial); the radius says how
	// frosted it is.
	Glass = 0x0442,
	// Native: the widget opens the window there, and what shows is what the
	// OS puts behind the window (Rwindow_set_backdrop: the blurred desktop,
	// acrylic, mica), under a light veil. It needs a window created with
	// R_WINDOW_TRANSPARENT that the OS could make see-through; anywhere
	// else it is drawn as R_BLUR_FROSTED. The radius is the one of the OS.
	Native = 0x0443,
}

Direction :: enum i32 {
	// A widget: the direction of its parent (the default).
	Inherit = 0,
	Ltr = 0x0720,
	Rtl = 0x0721,
	// The windows: the direction of the language.
	Auto = 0x0722,
}

Text_Align :: enum i32 {
	Left = 0x0280,
	Center = 0x0281,
	Right = 0x0282,
}

Toast_Kind :: enum i32 {
	// A neutral message (the accent of the theme).
	Info = 0x04E0,
	Success = 0x04E1,
	Warning = 0x04E2,
	Error = 0x04E3,
}

// What a Toast does with a message that comes while another one is shown.
Toast_Queue :: enum i32 {
	// The new message takes the place of the one shown (the default).
	Replace = 0x0500,
	// The messages are shown one after the other, each for its own time.
	Sequence = 0x0501,
	// Up to Rtoast_set_max_visible messages are shown at once, one above the other; the others wait.
	Stack = 0x0502,
}

Button_Style :: enum i32 {
	// Filled with R_COLOR_CONTROL.
	Default = 0x0290,
	// Filled with R_COLOR_ACCENT.
	Primary = 0x0291,
	// No background until hovered (toolbars).
	Flat = 0x0292,
}

Checkbox_Style :: enum i32 {
	Check = 0x02A0,
	// Round mark. Checking one unchecks the other radio boxes of the same parent.
	Radio = 0x02A1,
	// On / off switch.
	Switch = 0x02A2,
}

Stretch :: enum i32 {
	// Stretches the image to the widget, ignoring its aspect ratio.
	Fill = 0x02B0,
	// Largest size that fits entirely, keeping the aspect ratio.
	Fit = 0x02B1,
	// Smallest size that covers the widget, keeping the aspect ratio (cropped).
	Cover = 0x02B2,
	// Original size, centered.
	None = 0x02B3,
}

// The axes of a Plot.
Plot_Axis :: enum i32 {
	// The horizontal axis under the data.
	X = 0x0520,
	// The vertical axis on the left.
	Y = 0x0521,
	// A second horizontal axis, over the data: shown when a series or a drag line uses it.
	X2 = 0x0522,
	// A second vertical axis, on the right: shown when a series or a drag line uses it.
	Y2 = 0x0523,
}

// How a PdfView sizes its pages.
Pdf_Fit :: enum i32 {
	// The widest page fills the width of the view (default).
	Width = 0x05C0,
	// The largest page fits whole in the view.
	Page = 0x05C1,
	// The zoom of Rpdf_view_set_zoom; zooming with the wheel or the keys switches to it.
	None = 0x05C2,
}

// What a PdfView holds.
Pdf_Status :: enum i32 {
	// No document.
	Empty = 0x05D0,
	// A document is shown.
	Loaded = 0x05D1,
	// The document needs a password, or the one given is wrong: load it again with one.
	Needs_Password = 0x05D2,
	// The file could not be read, or is not a PDF document.
	Error = 0x05D3,
	// The PDFium library was not found (see Rpdf_is_available).
	Unavailable = 0x05D4,
}

// The fonts of a PDF document (Rpdf_document_set_font): those of Raven, then the ones added (2, 3...).
Pdf_Font :: enum i32 {
	// Roboto, the font of the interface of Raven.
	Sans = 0,
	// Roboto Mono.
	Mono = 1,
}

// How a shape of a PDF document is painted: a bitmask.
Pdf_Paint :: enum u32 {
	// Filled with the fill color.
	Fill = 0,
	// Outlined with the stroke color and the line width.
	Stroke = 1,
}
Pdf_Paints :: bit_set[Pdf_Paint; u32]

// A bitmask of styles of the text of a PDF document.
Pdf_Text_Style :: enum u32 {
	Bold = 0,
	Italic = 1,
	Underline = 2,
}
Pdf_Text_Styles :: bit_set[Pdf_Text_Style; u32]

Pdf_Print_Flag :: enum u32 {
	// No dialog: printed at once on the printer named, else the default one.
	Dialog = 0,
}
Pdf_Print_Flags :: bit_set[Pdf_Print_Flag; u32]

// How an axis of a Plot spreads its values.
Plot_Scale :: enum i32 {
	Linear = 0x0530,
	// Powers of ten at even distances; only values above zero are shown.
	Log = 0x0531,
	// Symmetric logarithm: linear between -t and t (Rplot_set_axis_linear_threshold,
	// 1 by default), logarithmic beyond on both sides: shows zero and the
	// negative values a logarithm cannot.
	Symlog = 0x0532,
}

// How the line of a series of a Plot is drawn.
Plot_Line :: enum i32 {
	Solid = 0x0540,
	Dashed = 0x0541,
	Dotted = 0x0542,
	Dash_Dot = 0x0543,
	// No line: only the markers (a scatter plot).
	None = 0x0544,
}

// The mark drawn on each point of a series of a Plot.
Plot_Marker :: enum i32 {
	None = 0x0550,
	Circle = 0x0551,
	Square = 0x0552,
	Diamond = 0x0553,
	Triangle_Up = 0x0554,
	Triangle_Down = 0x0555,
	Cross = 0x0556,
	Plus = 0x0557,
	Asterisk = 0x0558,
}

// Where the legend of a Plot is.
Plot_Legend :: enum i32 {
	Hidden = 0x0560,
	Top_Left = 0x0561,
	Top = 0x0562,
	Top_Right = 0x0563,
	Right = 0x0564,
	Bottom_Right = 0x0565,
	Bottom = 0x0566,
	Bottom_Left = 0x0567,
	Left = 0x0568,
	// To the right of the data, outside it.
	Outside = 0x0569,
}

// Shorthands of Rtext_editor_set_language for two languages.
Syntax :: enum i32 {
	None = 0x0340,
	Xml = 0x0341,
	C = 0x0342,
}

Language_Flag :: enum u32 {
	// Tags and attributes instead of code: XML, HTML. The words and the comments of the description are not used.
	Markup = 0,
	// Selectors, properties and values: CSS.
	Style_Sheet = 1,
	// A block comment may hold another one (Rust).
	Nested_Comments = 2,
	// Between single quotes there is one character; any other single quote is not a text (C, Java, Rust).
	Char_Literals = 3,
	// A line that ends with ':' indents the next one (Python).
	Indent_On_Colon = 4,
	// The words of the language are recognized whatever their case.
	Ignore_Case = 5,
	// Markup that holds JavaScript in script elements and CSS in style elements (HTML).
	Embeds = 6,
	// Formatted text written in Markdown: titles, strong and emphasized
	// text, code, links, lists. The editor shows the text as it is written,
	// marks included, with strong text in bold, emphasized text in italic
	// and titles in bold: what will be formatted is seen while typing.
	Formatted_Text = 7,
}
Language_Flags :: bit_set[Language_Flag; u32]

// What a part of the text is, for the language of the editor (Rtext_editor_get_token).
Token :: enum i32 {
	Text = 0x03A0,
	Comment = 0x03A1,
	String = 0x03A2,
	Number = 0x03A3,
	Keyword = 0x03A4,
	// A type, or a name that is always there.
	Type = 0x03A5,
	// A name followed by an opening parenthesis.
	Function = 0x03A6,
	// The name of an element, in markup and in the selectors of a style sheet.
	Tag = 0x03A7,
	// The name of an attribute (markup), of a class or of a property (style sheets), of a key (JSON).
	Attribute = 0x03A8,
	Punctuation = 0x03A9,
	// A line for the preprocessor, an annotation, a declaration of a document.
	Directive = 0x03AA,
	// Formatted text (the Markdown language): a title, strong text, emphasized text, a link.
	Heading = 0x03AB,
	Strong = 0x03AC,
	Emphasis = 0x03AD,
	Link = 0x03AE,
}

// What a marker says about the text it underlines.
Marker_Kind :: enum i32 {
	Error = 0x0350,
	Warning = 0x0351,
	Info = 0x0352,
}

// Buttons of a header bar.
Header_Button :: enum u32 {
	Close = 0,
	Minimize = 1,
	Maximize = 2,
}
Header_Buttons :: bit_set[Header_Button; u32]
// R_HEADER_NO_BUTTONS (0 in C) is {}
Header_Buttons_All_Buttons :: Header_Buttons{.Close, .Minimize, .Maximize}

// A progress bar on the icon of the application (Rwindow_set_taskbar_progress).
Taskbar_Progress :: enum i32 {
	None = 0x07A0,
	Normal = 0x07A1,
	// Shown as paused (yellow on Windows).
	Paused = 0x07A2,
	// Shown as failed (red on Windows).
	Error = 0x07A3,
	// Something goes on, without knowing how far it is.
	Indeterminate = 0x07A4,
}

// Where a panel goes in a dock area, relative to another panel or to the whole area.
Dock_Side :: enum i32 {
	Left = 0x0360,
	Right = 0x0361,
	Top = 0x0362,
	Bottom = 0x0363,
	// Behind the same tabs as the other panel.
	Tab = 0x0364,
}

Visual_Preset :: enum i32 {
	// The look of the theme of the window: its platform, dark or light.
	Auto = 0x0510,
	// Cards with soft shadows, few lines (follows dark or light).
	Modern = 0x0511,
	// Headers in gradients, a grid of lines, thick links that glow; pins of the type "exec" are arrows.
	Blueprint = 0x0512,
	// Compact nodes, flat headers in muted colors, thin links.
	Blender = 0x0513,
	// Almost black, outlines and links of light.
	Neon = 0x0514,
}

// What an item of a timeline is.
Timeline_Item :: enum i32 {
	None = 0x0460,
	Clip = 0x0461,
	Key = 0x0462,
	Marker = 0x0463,
}

Block_Shape :: enum i32 {
	// A step: snaps under another one, others snap under it.
	Statement = 0x0700,
	// Starts a script ("when the game starts"): nothing goes above it.
	Hat = 0x0701,
	// Ends a script ("stop"): nothing goes under it.
	Cap = 0x0702,
	// Wraps stacks in its mouths ("repeat", "if ... else").
	C = 0x0703,
	// A rounded value that goes into a slot.
	Reporter = 0x0704,
	// A hexagonal condition that goes into a %b slot (or a %r one).
	Boolean = 0x0705,
}

Block_Place :: enum i32 {
	Next = 0x0710,
	Body = 0x0711,
	Input = 0x0712,
}

// How much a notification of the system asks for attention (Rnotification_show).
Notification_Urgency :: enum i32 {
	// No sound; may only go to the list of notifications.
	Low = 0x04F0,
	Normal = 0x04F1,
	// Stays until the user dismisses it, where the system allows it.
	Critical = 0x04F2,
}

File_Dialog_Flag :: enum u32 {
	// Several files can be chosen (Rfile_dialog_open): see Rfile_dialog_get_result.
	Multiple = 0,
	// Never the dialog of the OS: the file browser of Raven, in a dialog of the parent window.
	Raven = 1,
	Show_Hidden = 2,
}
File_Dialog_Flags :: bit_set[File_Dialog_Flag; u32]

File_Browser_Mode :: enum i32 {
	// Choose an existing file.
	Open = 0x0380,
	// Choose where to save: a name is typed; an existing file must be confirmed.
	Save = 0x0381,
	// Choose a folder.
	Folder = 0x0382,
}

Message_Buttons :: enum i32 {
	Ok = 0x0390,
	Ok_Cancel = 0x0391,
	Yes_No = 0x0392,
	Yes_No_Cancel = 0x0393,
}

Message_Result :: enum i32 {
	// Dismissed (Escape), or the Cancel button.
	Cancel = 0,
	Ok = 1,
	Yes = 2,
	No = 3,
}

// How a spring moves. Animations in Raven are springs, as on iOS: a value is
// pulled toward its target and may run past it before settling, which is what
// makes a control feel alive rather than mechanical.
Motion :: enum i32 {
	// Fast, no bounce: hover, highlights.
	Snappy = 0x0320,
	// Slower, no bounce: scrolling, fades, sizes.
	Smooth = 0x0321,
	// Runs a little past the target and comes back: switches, popups, pressed buttons.
	Bouncy = 0x0322,
	// Swings several times around the target: things that fly in or get dropped.
	Wobbly = 0x0323,
}

// The effects that come with Raven (Reffect_get).
Effect :: enum i32 {
	// Soft lights that drift slowly: color, color2; params: speed, scale, intensity, -. Animated.
	Aurora = 0x0B40,
	// A band of light that sweeps across (loading placeholders): color = base, color2 = light; params: speed, width, angle, -. Animated.
	Shimmer = 0x0B41,
	// A glowing edge whose colors run around the shape: color, color2; params: speed, width, glow, -. Animated.
	Neon = 0x0B42,
	// Rings that ripple what is behind, like water (reads the backdrop): color = tint; params: speed, strength, density, center x (0..1). Animated.
	Ripple = 0x0B43,
	// Bands of color that flow into each other: color, color2; params: speed, scale, contrast, -. Animated.
	Plasma = 0x0B44,
	// A gradient with the fine grain of film: color (top left) to color2 (bottom right); params: grain, angle, -, -. Animated grain.
	Grain = 0x0B45,
	// A soft light that follows a point: color = light, color2 = background; params: x, y (0..1), radius (0..1), -.
	Spotlight = 0x0B46,
	// Four colors blended across the surface, slowly moving: color, color2 and their mix; params: speed, -, -, -. Animated.
	Mesh = 0x0B47,
}

// Flags of Rshader_create.
Shader_Flag :: enum u32 {
	// The effect reads what is drawn behind it (raven_backdrop, raven_backdrop_blur).
	Backdrop = 0,
	// The effect changes with time: the window keeps drawing frames while it is shown.
	Animated = 1,
}
Shader_Flags :: bit_set[Shader_Flag; u32]

// The kind of value an attribute takes.
Xml_Value_Type :: enum i32 {
	Text = 0x03C0,
	// "true" or "false".
	Bool = 0x03C1,
	Number = 0x03C2,
	// One of the words of `choices`.
	Choice = 0x03C3,
	// "#RGB", "#RRGGBB", "#RRGGBBAA", or a name: white, black, red, transparent...
	Color = 0x03C4,
	// The name of an action (Raction_bind).
	Action = 0x03C5,
	// The path of a file.
	File = 0x03C6,
	// Several numbers separated by commas: "8,4", "0,0,1,1".
	Numbers = 0x03C7,
	// Texts separated by '|': "One|Two|Three".
	List = 0x03C8,
}

Color_Filter :: enum i32 {
	None = 0x0740,
	// Corrections: the colors that cannot be told apart are moved to colors that can.
	Protanopia = 0x0741,
	Deuteranopia = 0x0742,
	Tritanopia = 0x0743,
	Grayscale = 0x0744,
	Invert = 0x0745,
	// Simulations, to see an interface as the color blind see it.
	Simulate_Protanopia = 0x0746,
	Simulate_Deuteranopia = 0x0747,
	Simulate_Tritanopia = 0x0748,
	Simulate_Achromatopsia = 0x0749,
}

Accessible_Role :: enum i32 {
	// The role of the type of the widget (the default).
	Auto = 0,
	// Not exposed: its children are seen as the children of its parent.
	None = 0x0760,
	Window = 0x0761,
	Group = 0x0762,
	Text = 0x0763,
	Heading = 0x0764,
	Link = 0x0765,
	Button = 0x0766,
	Toggle_Button = 0x0767,
	Checkbox = 0x0768,
	Radio_Button = 0x0769,
	Slider = 0x076A,
	Progress_Bar = 0x076B,
	Spin_Button = 0x076C,
	Text_Field = 0x076D,
	Document = 0x076E,
	Combo_Box = 0x076F,
	List = 0x0770,
	List_Item = 0x0771,
	Tree = 0x0772,
	Tree_Item = 0x0773,
	Table = 0x0774,
	Menu_Bar = 0x0775,
	Menu = 0x0776,
	Menu_Item = 0x0777,
	Tab_List = 0x0778,
	Tab = 0x0779,
	Tab_Panel = 0x077A,
	Toolbar = 0x077B,
	Status_Bar = 0x077C,
	Dialog = 0x077D,
	Alert = 0x077E,
	Image = 0x077F,
	Canvas = 0x0780,
	Separator = 0x0781,
	Scroll_Bar = 0x0782,
	Pane = 0x0783,
}

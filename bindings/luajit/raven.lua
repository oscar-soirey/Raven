-- Raven for LuaJIT.
--
-- Generated from the C headers by tools/bindgen (Raven 0.1): do not edit.
--
-- The FFI of LuaJIT reads C: these are the declarations of the public
-- headers, and the module is the library itself. Enumerations are numbers,
-- found both in the module (raven.R_WIDGET_BUTTON) and as strings where a
-- parameter has the type of the enumeration. A Lua function passes where
-- a callback is expected; keep it referenced while the library may call it.
--
--   local r = require("raven")
--   r.Rinit(r.R_BACKEND_VULKAN)
--   local xml = [[<Window Title="Hi"><Button Text="Quit" OnClicked="quit"/></Window>]]
--   r.Rwindow_create_from_xml(xml, #xml)
--   local quit = require("ffi").cast("RActionCallback", function() r.Rquit(0) end)
--   r.Raction_bind("quit", quit, nil)
--   while r.Rwait_frame(-1.0) ~= 0 do end
--   r.Rshutdown()
--
-- The library is looked for in the RAVEN_LIBRARY environment variable, then
-- where the system looks (raven.dll, libraven.so).

local ffi = require("ffi")

ffi.cdef[[
typedef uint32_t Rid;
typedef uint32_t Ruint;
typedef struct RPainter RPainter;
typedef enum RBackend {
	R_BACKEND_VULKAN = 1,
	R_BACKEND_NULL = 2,
	R_BACKEND_CUSTOM = 3,
} RBackend;
typedef enum RError {
	R_NO_ERROR = 112,
	R_ERROR_INVALID_ID = 113,
	R_ERROR_INVALID_ENUM = 114,
	R_ERROR_INVALID_VALUE = 115,
	R_ERROR_INVALID_OPERATION = 116,
	R_ERROR_BACKEND = 117,
	R_ERROR_PLATFORM = 118,
	R_ERROR_OUT_OF_MEMORY = 119,
	R_ERROR_INVALID_FILE_FORMAT = 120,
	R_ERROR_FILE_NOT_FOUND = 121,
	R_ERROR_XML = 122,
	R_ERROR_WRONG_THREAD = 123,
} RError;
typedef enum RSeverity {
	R_SEVERITY_WEAK_WARNING = 128,
	R_SEVERITY_WARNING = 129,
	R_SEVERITY_ERROR = 130,
	R_SEVERITY_FATAL = 131,
} RSeverity;
typedef enum RWindowFlags {
	R_WINDOW_DEFAULT = 0,
	R_WINDOW_NO_RESIZE = 1,
	R_WINDOW_HIDDEN = 2,
	R_WINDOW_BORDERLESS = 4,
	R_WINDOW_MAXIMIZED = 8,
	R_WINDOW_CUSTOM_HEADER = 16,
	R_WINDOW_TRANSPARENT = 32,
} RWindowFlags;
typedef enum RBackdrop {
	R_BACKDROP_NONE = 1024,
	R_BACKDROP_TRANSPARENT = 1025,
	R_BACKDROP_BLUR = 1026,
	R_BACKDROP_ACRYLIC = 1027,
	R_BACKDROP_MICA = 1028,
} RBackdrop;
typedef enum RWindowSystem {
	R_WINDOW_SYSTEM_NONE = 144,
	R_WINDOW_SYSTEM_WIN32 = 145,
	R_WINDOW_SYSTEM_X11 = 146,
	R_WINDOW_SYSTEM_WAYLAND = 147,
	R_WINDOW_SYSTEM_COCOA = 148,
} RWindowSystem;
typedef enum RWindowEdge {
	R_EDGE_LEFT = 208,
	R_EDGE_RIGHT = 209,
	R_EDGE_TOP = 210,
	R_EDGE_BOTTOM = 211,
	R_EDGE_TOP_LEFT = 212,
	R_EDGE_TOP_RIGHT = 213,
	R_EDGE_BOTTOM_LEFT = 214,
	R_EDGE_BOTTOM_RIGHT = 215,
} RWindowEdge;
typedef enum RMouseButton {
	R_MOUSE_LEFT = 0,
	R_MOUSE_RIGHT = 1,
	R_MOUSE_MIDDLE = 2,
	R_MOUSE_X1 = 3,
	R_MOUSE_X2 = 4,
} RMouseButton;
typedef enum RModifier {
	R_MOD_SHIFT = 1,
	R_MOD_CTRL = 2,
	R_MOD_ALT = 4,
	R_MOD_SUPER = 8,
} RModifier;
typedef enum RCursor {
	R_CURSOR_ARROW = 176,
	R_CURSOR_IBEAM = 177,
	R_CURSOR_HAND = 178,
	R_CURSOR_CROSSHAIR = 179,
	R_CURSOR_RESIZE_H = 180,
	R_CURSOR_RESIZE_V = 181,
	R_CURSOR_RESIZE_NWSE = 182,
	R_CURSOR_RESIZE_NESW = 183,
	R_CURSOR_MOVE = 184,
	R_CURSOR_NOT_ALLOWED = 185,
	R_CURSOR_NONE = 186,
} RCursor;
typedef enum RKey {
	R_KEY_UNKNOWN = 0,
	R_KEY_SPACE = 32,
	R_KEY_APOSTROPHE = 39,
	R_KEY_COMMA = 44,
	R_KEY_MINUS = 45,
	R_KEY_PERIOD = 46,
	R_KEY_SLASH = 47,
	R_KEY_0 = 48,
	R_KEY_1 = 49,
	R_KEY_2 = 50,
	R_KEY_3 = 51,
	R_KEY_4 = 52,
	R_KEY_5 = 53,
	R_KEY_6 = 54,
	R_KEY_7 = 55,
	R_KEY_8 = 56,
	R_KEY_9 = 57,
	R_KEY_SEMICOLON = 59,
	R_KEY_EQUAL = 61,
	R_KEY_A = 65,
	R_KEY_B = 66,
	R_KEY_C = 67,
	R_KEY_D = 68,
	R_KEY_E = 69,
	R_KEY_F = 70,
	R_KEY_G = 71,
	R_KEY_H = 72,
	R_KEY_I = 73,
	R_KEY_J = 74,
	R_KEY_K = 75,
	R_KEY_L = 76,
	R_KEY_M = 77,
	R_KEY_N = 78,
	R_KEY_O = 79,
	R_KEY_P = 80,
	R_KEY_Q = 81,
	R_KEY_R = 82,
	R_KEY_S = 83,
	R_KEY_T = 84,
	R_KEY_U = 85,
	R_KEY_V = 86,
	R_KEY_W = 87,
	R_KEY_X = 88,
	R_KEY_Y = 89,
	R_KEY_Z = 90,
	R_KEY_LEFT_BRACKET = 91,
	R_KEY_BACKSLASH = 92,
	R_KEY_RIGHT_BRACKET = 93,
	R_KEY_GRAVE = 96,
	R_KEY_ESCAPE = 256,
	R_KEY_ENTER = 257,
	R_KEY_TAB = 258,
	R_KEY_BACKSPACE = 259,
	R_KEY_INSERT = 260,
	R_KEY_DELETE = 261,
	R_KEY_RIGHT = 262,
	R_KEY_LEFT = 263,
	R_KEY_DOWN = 264,
	R_KEY_UP = 265,
	R_KEY_PAGE_UP = 266,
	R_KEY_PAGE_DOWN = 267,
	R_KEY_HOME = 268,
	R_KEY_END = 269,
	R_KEY_CAPS_LOCK = 270,
	R_KEY_PRINT_SCREEN = 271,
	R_KEY_PAUSE = 272,
	R_KEY_F1 = 273,
	R_KEY_F2 = 274,
	R_KEY_F3 = 275,
	R_KEY_F4 = 276,
	R_KEY_F5 = 277,
	R_KEY_F6 = 278,
	R_KEY_F7 = 279,
	R_KEY_F8 = 280,
	R_KEY_F9 = 281,
	R_KEY_F10 = 282,
	R_KEY_F11 = 283,
	R_KEY_F12 = 284,
	R_KEY_KP_0 = 285,
	R_KEY_KP_1 = 286,
	R_KEY_KP_2 = 287,
	R_KEY_KP_3 = 288,
	R_KEY_KP_4 = 289,
	R_KEY_KP_5 = 290,
	R_KEY_KP_6 = 291,
	R_KEY_KP_7 = 292,
	R_KEY_KP_8 = 293,
	R_KEY_KP_9 = 294,
	R_KEY_KP_DECIMAL = 295,
	R_KEY_KP_DIVIDE = 296,
	R_KEY_KP_MULTIPLY = 297,
	R_KEY_KP_SUBTRACT = 298,
	R_KEY_KP_ADD = 299,
	R_KEY_KP_ENTER = 300,
	R_KEY_LEFT_SHIFT = 301,
	R_KEY_LEFT_CTRL = 302,
	R_KEY_LEFT_ALT = 303,
	R_KEY_LEFT_SUPER = 304,
	R_KEY_RIGHT_SHIFT = 305,
	R_KEY_RIGHT_CTRL = 306,
	R_KEY_RIGHT_ALT = 307,
	R_KEY_RIGHT_SUPER = 308,
	R_KEY_MENU = 309,
	R_KEY_COUNT = 310,
} RKey;
typedef enum REventType {
	R_EVENT_NONE = 192,
	R_EVENT_MOUSE_MOVE = 193,
	R_EVENT_MOUSE_DOWN = 194,
	R_EVENT_MOUSE_UP = 195,
	R_EVENT_MOUSE_WHEEL = 196,
	R_EVENT_MOUSE_ENTER = 197,
	R_EVENT_MOUSE_LEAVE = 198,
	R_EVENT_KEY_DOWN = 199,
	R_EVENT_KEY_UP = 200,
	R_EVENT_TEXT = 201,
	R_EVENT_FOCUS_GAINED = 202,
	R_EVENT_FOCUS_LOST = 203,
} REventType;
typedef enum RDropPhase {
	R_DROP_ENTER = 224,
	R_DROP_MOVE = 225,
	R_DROP_LEAVE = 226,
	R_DROP_DROPPED = 227,
} RDropPhase;
typedef enum RLook {
	R_LOOK_RAVEN_DARK = 224,
	R_LOOK_RAVEN_LIGHT = 225,
	R_LOOK_RAVEN_GLASS_DARK = 226,
	R_LOOK_RAVEN_GLASS_LIGHT = 227,
	R_LOOK_RAVEN_HIGH_CONTRAST_DARK = 228,
	R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT = 229,
	R_LOOK_WINDOWS_DARK = 230,
	R_LOOK_WINDOWS_LIGHT = 231,
	R_LOOK_MACOS_DARK = 232,
	R_LOOK_MACOS_LIGHT = 233,
	R_LOOK_GNOME_DARK = 234,
	R_LOOK_GNOME_LIGHT = 235,
	R_LOOK_NATIVE_DARK = 236,
	R_LOOK_NATIVE_LIGHT = 237,
} RLook;
typedef enum RTheme {
	R_THEME_DARK = 224,
	R_THEME_LIGHT = 225,
	R_THEME_GLASS_DARK = 226,
	R_THEME_GLASS_LIGHT = 227,
	R_THEME_HIGH_CONTRAST_DARK = 228,
	R_THEME_HIGH_CONTRAST_LIGHT = 229,
} RTheme;
typedef enum RMaterial {
	R_MATERIAL_SOLID = 736,
	R_MATERIAL_GLASS = 737,
	R_MATERIAL_FROSTED_GLASS = 738,
	R_MATERIAL_TINTED_GLASS = 739,
	R_MATERIAL_DEFAULT = 740,
} RMaterial;
typedef enum RColorRole {
	R_COLOR_WINDOW = 256,
	R_COLOR_PANEL = 257,
	R_COLOR_BORDER = 258,
	R_COLOR_TEXT = 259,
	R_COLOR_TEXT_DISABLED = 260,
	R_COLOR_TEXT_PLACEHOLDER = 261,
	R_COLOR_ACCENT = 262,
	R_COLOR_ACCENT_HOVERED = 263,
	R_COLOR_ACCENT_PRESSED = 264,
	R_COLOR_ACCENT_TEXT = 265,
	R_COLOR_CONTROL = 266,
	R_COLOR_CONTROL_HOVERED = 267,
	R_COLOR_CONTROL_PRESSED = 268,
	R_COLOR_CONTROL_BORDER = 269,
	R_COLOR_INPUT = 270,
	R_COLOR_FOCUS = 271,
	R_COLOR_SELECTION = 272,
	R_COLOR_ROW_HOVERED = 273,
	R_COLOR_TRACK = 274,
	R_COLOR_HANDLE = 275,
	R_COLOR_HANDLE_HOVERED = 276,
	R_COLOR_SEPARATOR = 277,
	R_COLOR_HEADER = 278,
	R_COLOR_HEADER_HOVERED = 279,
	R_COLOR_TAB_ACTIVE = 280,
	R_COLOR_POPUP = 281,
	R_COLOR_POPUP_BORDER = 282,
	R_COLOR_SHADOW = 283,
	R_COLOR_TOOLTIP = 284,
	R_COLOR_TOOLTIP_TEXT = 285,
	R_COLOR_SWITCH = 286,
	R_COLOR_SUCCESS = 287,
	R_COLOR_WARNING = 288,
	R_COLOR_DANGER = 289,
	R_COLOR_LINK = 290,
	R_COLOR_CODE = 291,
} RColorRole;
typedef enum RMetric {
	R_METRIC_FONT_SIZE = 320,
	R_METRIC_CORNER_RADIUS = 321,
	R_METRIC_BORDER_WIDTH = 322,
	R_METRIC_CONTROL_HEIGHT = 323,
	R_METRIC_PADDING_X = 324,
	R_METRIC_PADDING_Y = 325,
	R_METRIC_SPACING = 326,
	R_METRIC_SCROLLBAR_SIZE = 327,
	R_METRIC_ROW_HEIGHT = 328,
	R_METRIC_INDENT = 329,
	R_METRIC_SPLITTER_SIZE = 330,
	R_METRIC_CHECK_SIZE = 331,
	R_METRIC_SLIDER_HANDLE_SIZE = 332,
	R_METRIC_POPUP_RADIUS = 333,
	R_METRIC_GLASS_FROST = 334,
	R_METRIC_GLASS_REFRACTION = 335,
	R_METRIC_GLASS_SATURATION = 336,
	R_METRIC_GLASS_RIM = 337,
	R_METRIC_GLASS_SHADOW = 338,
	R_METRIC_BLUR_RADIUS = 339,
} RMetric;
typedef enum RFontStyle {
	R_FONT_REGULAR = 1056,
	R_FONT_BOLD = 1057,
	R_FONT_ITALIC = 1058,
	R_FONT_BOLD_ITALIC = 1059,
} RFontStyle;
typedef enum RFilterType {
	R_FILTER_LINEAR = 704,
	R_FILTER_NEAREST = 705,
	R_FILTER_MIPMAP = 706,
} RFilterType;
typedef enum RWidgetType {
	R_WIDGET_VBOX = 512,
	R_WIDGET_HBOX = 513,
	R_WIDGET_GRID = 514,
	R_WIDGET_UNIFORM_GRID = 515,
	R_WIDGET_OVERLAY = 516,
	R_WIDGET_BORDER = 517,
	R_WIDGET_SIZE_BOX = 518,
	R_WIDGET_SPACER = 519,
	R_WIDGET_SCROLL_BOX = 520,
	R_WIDGET_SPLITTER = 521,
	R_WIDGET_WRAP_BOX = 522,
	R_WIDGET_CANVAS = 523,
	R_WIDGET_SWITCHER = 524,
	R_WIDGET_TAB_VIEW = 525,
	R_WIDGET_EXPANDER = 526,
	R_WIDGET_LABEL = 527,
	R_WIDGET_BUTTON = 528,
	R_WIDGET_CHECKBOX = 529,
	R_WIDGET_SLIDER = 530,
	R_WIDGET_PROGRESSBAR = 531,
	R_WIDGET_IMAGE = 532,
	R_WIDGET_TEXT_INPUT = 533,
	R_WIDGET_SPIN_BOX = 534,
	R_WIDGET_COMBO_BOX = 535,
	R_WIDGET_SEPARATOR = 536,
	R_WIDGET_LIST_VIEW = 537,
	R_WIDGET_TREE_VIEW = 538,
	R_WIDGET_MENU_BAR = 539,
	R_WIDGET_MENU = 540,
	R_WIDGET_MENU_ITEM = 541,
	R_WIDGET_VIEWPORT = 542,
	R_WIDGET_CUSTOM = 543,
	R_WIDGET_TEXT_EDITOR = 544,
	R_WIDGET_HEADER_BAR = 545,
	R_WIDGET_DOCK_AREA = 546,
	R_WIDGET_ISLAND = 547,
	R_WIDGET_FILE_BROWSER = 548,
	R_WIDGET_BACKGROUND_BLUR = 549,
	R_WIDGET_COLOR_PICKER = 550,
	R_WIDGET_TIMELINE = 551,
	R_WIDGET_NODE_GRAPH = 552,
	R_WIDGET_TILE_VIEW = 553,
	R_WIDGET_BLOCK_EDITOR = 554,
} RWidgetType;
typedef enum RHAlign {
	R_HALIGN_FILL = 576,
	R_HALIGN_LEFT = 577,
	R_HALIGN_CENTER = 578,
	R_HALIGN_RIGHT = 579,
} RHAlign;
typedef enum RVAlign {
	R_VALIGN_FILL = 592,
	R_VALIGN_TOP = 593,
	R_VALIGN_CENTER = 594,
	R_VALIGN_BOTTOM = 595,
} RVAlign;
typedef enum RVisibility {
	R_VISIBLE = 608,
	R_HIDDEN = 609,
	R_COLLAPSED = 610,
} RVisibility;
typedef enum RTransition {
	R_TRANSITION_NONE = 768,
	R_TRANSITION_FADE = 769,
	R_TRANSITION_POP = 770,
	R_TRANSITION_ZOOM = 771,
	R_TRANSITION_FLY_UP = 772,
	R_TRANSITION_FLY_DOWN = 773,
	R_TRANSITION_FLY_LEFT = 774,
	R_TRANSITION_FLY_RIGHT = 775,
	R_TRANSITION_DROP = 776,
	R_TRANSITION_SLIDE_LEFT = 777,
	R_TRANSITION_SLIDE_RIGHT = 778,
} RTransition;
typedef enum ROrientation {
	R_HORIZONTAL = 624,
	R_VERTICAL = 625,
} ROrientation;
typedef enum RBlurStyle {
	R_BLUR_PLAIN = 1088,
	R_BLUR_FROSTED = 1089,
	R_BLUR_GLASS = 1090,
	R_BLUR_NATIVE = 1091,
} RBlurStyle;
typedef enum RDirection {
	R_DIRECTION_INHERIT = 0,
	R_DIRECTION_LTR = 1824,
	R_DIRECTION_RTL = 1825,
	R_DIRECTION_AUTO = 1826,
} RDirection;
typedef enum RTextAlign {
	R_TEXT_ALIGN_LEFT = 640,
	R_TEXT_ALIGN_CENTER = 641,
	R_TEXT_ALIGN_RIGHT = 642,
} RTextAlign;
typedef enum RButtonStyle {
	R_BUTTON_DEFAULT = 656,
	R_BUTTON_PRIMARY = 657,
	R_BUTTON_FLAT = 658,
} RButtonStyle;
typedef enum RCheckboxStyle {
	R_CHECKBOX_CHECK = 672,
	R_CHECKBOX_RADIO = 673,
	R_CHECKBOX_SWITCH = 674,
} RCheckboxStyle;
typedef enum RStretch {
	R_STRETCH_FILL = 688,
	R_STRETCH_FIT = 689,
	R_STRETCH_COVER = 690,
	R_STRETCH_NONE = 691,
} RStretch;
typedef enum RSyntax {
	R_SYNTAX_NONE = 832,
	R_SYNTAX_XML = 833,
	R_SYNTAX_C = 834,
} RSyntax;
typedef enum RLanguageFlags {
	R_LANGUAGE_MARKUP = 1,
	R_LANGUAGE_STYLE_SHEET = 2,
	R_LANGUAGE_NESTED_COMMENTS = 4,
	R_LANGUAGE_CHAR_LITERALS = 8,
	R_LANGUAGE_INDENT_ON_COLON = 16,
	R_LANGUAGE_IGNORE_CASE = 32,
	R_LANGUAGE_EMBEDS = 64,
	R_LANGUAGE_FORMATTED_TEXT = 128,
} RLanguageFlags;
typedef enum RToken {
	R_TOKEN_TEXT = 928,
	R_TOKEN_COMMENT = 929,
	R_TOKEN_STRING = 930,
	R_TOKEN_NUMBER = 931,
	R_TOKEN_KEYWORD = 932,
	R_TOKEN_TYPE = 933,
	R_TOKEN_FUNCTION = 934,
	R_TOKEN_TAG = 935,
	R_TOKEN_ATTRIBUTE = 936,
	R_TOKEN_PUNCTUATION = 937,
	R_TOKEN_DIRECTIVE = 938,
	R_TOKEN_HEADING = 939,
	R_TOKEN_STRONG = 940,
	R_TOKEN_EMPHASIS = 941,
	R_TOKEN_LINK = 942,
} RToken;
typedef enum RMarkerKind {
	R_MARKER_ERROR = 848,
	R_MARKER_WARNING = 849,
	R_MARKER_INFO = 850,
} RMarkerKind;
typedef enum RHeaderButtons {
	R_HEADER_NO_BUTTONS = 0,
	R_HEADER_CLOSE = 1,
	R_HEADER_MINIMIZE = 2,
	R_HEADER_MAXIMIZE = 4,
	R_HEADER_ALL_BUTTONS = 7,
} RHeaderButtons;
typedef enum RDockSide {
	R_DOCK_LEFT = 864,
	R_DOCK_RIGHT = 865,
	R_DOCK_TOP = 866,
	R_DOCK_BOTTOM = 867,
	R_DOCK_TAB = 868,
} RDockSide;
typedef enum RTimelineItem {
	R_TIMELINE_NONE = 1120,
	R_TIMELINE_CLIP = 1121,
	R_TIMELINE_KEY = 1122,
	R_TIMELINE_MARKER = 1123,
} RTimelineItem;
typedef enum RBlockShape {
	R_BLOCK_STATEMENT = 1792,
	R_BLOCK_HAT = 1793,
	R_BLOCK_CAP = 1794,
	R_BLOCK_C = 1795,
	R_BLOCK_REPORTER = 1796,
	R_BLOCK_BOOLEAN = 1797,
} RBlockShape;
typedef enum RBlockPlace {
	R_BLOCK_NEXT = 1808,
	R_BLOCK_BODY = 1809,
	R_BLOCK_INPUT = 1810,
} RBlockPlace;
typedef enum RFileDialogFlags {
	R_FILE_DIALOG_MULTIPLE = 1,
	R_FILE_DIALOG_RAVEN = 2,
	R_FILE_DIALOG_SHOW_HIDDEN = 4,
} RFileDialogFlags;
typedef enum RFileBrowserMode {
	R_FILE_BROWSER_OPEN = 896,
	R_FILE_BROWSER_SAVE = 897,
	R_FILE_BROWSER_FOLDER = 898,
} RFileBrowserMode;
typedef enum RMessageButtons {
	R_MESSAGE_OK = 912,
	R_MESSAGE_OK_CANCEL = 913,
	R_MESSAGE_YES_NO = 914,
	R_MESSAGE_YES_NO_CANCEL = 915,
} RMessageButtons;
typedef enum RMessageResult {
	R_MESSAGE_RESULT_CANCEL = 0,
	R_MESSAGE_RESULT_OK = 1,
	R_MESSAGE_RESULT_YES = 2,
	R_MESSAGE_RESULT_NO = 3,
} RMessageResult;
typedef enum RMotion {
	R_MOTION_SNAPPY = 800,
	R_MOTION_SMOOTH = 801,
	R_MOTION_BOUNCY = 802,
	R_MOTION_WOBBLY = 803,
} RMotion;
typedef enum REffect {
	R_EFFECT_AURORA = 2880,
	R_EFFECT_SHIMMER = 2881,
	R_EFFECT_NEON = 2882,
	R_EFFECT_RIPPLE = 2883,
	R_EFFECT_PLASMA = 2884,
	R_EFFECT_GRAIN = 2885,
	R_EFFECT_SPOTLIGHT = 2886,
	R_EFFECT_MESH = 2887,
} REffect;
typedef enum RShaderFlags {
	R_SHADER_BACKDROP = 1,
	R_SHADER_ANIMATED = 2,
} RShaderFlags;
typedef enum RXmlValueType {
	R_XML_TEXT = 960,
	R_XML_BOOL = 961,
	R_XML_NUMBER = 962,
	R_XML_CHOICE = 963,
	R_XML_COLOR = 964,
	R_XML_ACTION = 965,
	R_XML_FILE = 966,
	R_XML_NUMBERS = 967,
	R_XML_LIST = 968,
} RXmlValueType;
typedef enum RColorFilter {
	R_COLOR_FILTER_NONE = 1856,
	R_COLOR_FILTER_PROTANOPIA = 1857,
	R_COLOR_FILTER_DEUTERANOPIA = 1858,
	R_COLOR_FILTER_TRITANOPIA = 1859,
	R_COLOR_FILTER_GRAYSCALE = 1860,
	R_COLOR_FILTER_INVERT = 1861,
	R_COLOR_FILTER_SIMULATE_PROTANOPIA = 1862,
	R_COLOR_FILTER_SIMULATE_DEUTERANOPIA = 1863,
	R_COLOR_FILTER_SIMULATE_TRITANOPIA = 1864,
	R_COLOR_FILTER_SIMULATE_ACHROMATOPSIA = 1865,
} RColorFilter;
typedef enum RAccessibleRole {
	R_ROLE_AUTO = 0,
	R_ROLE_NONE = 1888,
	R_ROLE_WINDOW = 1889,
	R_ROLE_GROUP = 1890,
	R_ROLE_TEXT = 1891,
	R_ROLE_HEADING = 1892,
	R_ROLE_LINK = 1893,
	R_ROLE_BUTTON = 1894,
	R_ROLE_TOGGLE_BUTTON = 1895,
	R_ROLE_CHECKBOX = 1896,
	R_ROLE_RADIO_BUTTON = 1897,
	R_ROLE_SLIDER = 1898,
	R_ROLE_PROGRESS_BAR = 1899,
	R_ROLE_SPIN_BUTTON = 1900,
	R_ROLE_TEXT_FIELD = 1901,
	R_ROLE_DOCUMENT = 1902,
	R_ROLE_COMBO_BOX = 1903,
	R_ROLE_LIST = 1904,
	R_ROLE_LIST_ITEM = 1905,
	R_ROLE_TREE = 1906,
	R_ROLE_TREE_ITEM = 1907,
	R_ROLE_TABLE = 1908,
	R_ROLE_MENU_BAR = 1909,
	R_ROLE_MENU = 1910,
	R_ROLE_MENU_ITEM = 1911,
	R_ROLE_TAB_LIST = 1912,
	R_ROLE_TAB = 1913,
	R_ROLE_TAB_PANEL = 1914,
	R_ROLE_TOOLBAR = 1915,
	R_ROLE_STATUS_BAR = 1916,
	R_ROLE_DIALOG = 1917,
	R_ROLE_ALERT = 1918,
	R_ROLE_IMAGE = 1919,
	R_ROLE_CANVAS = 1920,
	R_ROLE_SEPARATOR = 1921,
	R_ROLE_SCROLL_BAR = 1922,
	R_ROLE_PANE = 1923,
} RAccessibleRole;
typedef struct RInitDesc RInitDesc;
typedef struct RAllocator RAllocator;
typedef struct RMemoryStats RMemoryStats;
typedef struct RFrameStats RFrameStats;
typedef struct RNativeWindow RNativeWindow;
typedef struct REvent REvent;
typedef struct RDropEvent RDropEvent;
typedef struct RTileItem RTileItem;
typedef struct RLanguageDesc RLanguageDesc;
typedef struct RFileFilter RFileFilter;
typedef struct RFileDialogDesc RFileDialogDesc;
typedef struct RViewportDrawInfo RViewportDrawInfo;
typedef struct RSpring RSpring;
typedef struct RShaderLook RShaderLook;
typedef struct RXmlError RXmlError;
typedef struct RXmlAttributeInfo RXmlAttributeInfo;
typedef struct RAccessibilitySettings RAccessibilitySettings;
typedef struct RHostPlatformDesc RHostPlatformDesc;
typedef void (*RFrameCallback)(double delta_seconds, void* user_data);
typedef void (*RTaskCallback)(void* user_data);
typedef void (*RParallelRangeCallback)(Ruint begin, Ruint end, void* user_data);
typedef void (*RTimerCallback)(Rid timer, void* user_data);
typedef void (*RActionCallback)(Rid sender, const char* action, void* user_data);
typedef void (*RErrorCallback)(RError code, RSeverity severity, const char* detail);
typedef void (*RDeviceResetCallback)(void* user_data);
typedef int (*RDropCallback)(Rid target, const RDropEvent* event, void* user_data);
typedef int (*RWindowCloseCallback)(Rid window, void* user_data);
typedef void (*RWindowResizeCallback)(Rid window, int width, int height, void* user_data);
typedef int (*RWindowEventHook)(Rid window, const REvent* event, void* user_data);
typedef void (*RWidgetDestroyCallback)(Rid widget, void* user_data);
typedef int (*RWidgetEventCallback)(Rid widget, const REvent* event, void* user_data);
typedef void (*RTabChangedCallback)(Rid tab_view, int index, void* user_data);
typedef int (*RTabCloseCallback)(Rid tab_view, int index, Rid content, void* user_data);
typedef void (*RExpanderChangedCallback)(Rid expander, int expanded, void* user_data);
typedef void (*RLabelLinkCallback)(Rid label, const char* url, void* user_data);
typedef void (*RButtonClickedCallback)(Rid button, void* user_data);
typedef void (*RCheckboxChangedCallback)(Rid checkbox, int checked, void* user_data);
typedef void (*RSliderChangedCallback)(Rid slider, float value, void* user_data);
typedef void (*RTextChangedCallback)(Rid text_input, const char* text, void* user_data);
typedef void (*RSpinBoxChangedCallback)(Rid spin_box, double value, void* user_data);
typedef void (*RComboBoxChangedCallback)(Rid combo_box, int index, void* user_data);
typedef const char* (*RListItemTextCallback)(Rid list_view, int index, void* user_data);
typedef void (*RListEventCallback)(Rid list_view, int index, void* user_data);
typedef void (*RTreeEventCallback)(Rid tree_view, Rid item, void* user_data);
typedef void (*RTreeContextCallback)(Rid tree_view, Rid item, float x, float y, void* user_data);
typedef void (*RMenuItemClickedCallback)(Rid item, void* user_data);
typedef void (*RTileItemCallback)(Rid tile_view, int index, RTileItem* item, void* user_data);
typedef void (*RTileEventCallback)(Rid tile_view, int index, void* user_data);
typedef void (*RTileContextCallback)(Rid tile_view, int index, float x, float y, void* user_data);
typedef const char* (*RTileDragDataCallback)(Rid tile_view, void* user_data);
typedef void (*RTextEditorZoomCallback)(Rid editor, float zoom, void* user_data);
typedef void (*RCompletionCallback)(Rid editor, const char* prefix, void* user_data);
typedef void (*RTextEditorChangedCallback)(Rid editor, void* user_data);
typedef int (*RDockCloseCallback)(Rid area, Rid panel, void* user_data);
typedef void (*RDockChangedCallback)(Rid area, void* user_data);
typedef void (*RIslandChangedCallback)(Rid island, int expanded, void* user_data);
typedef void (*RColorChangedCallback)(Rid picker, float r, float g, float b, float a, void* user_data);
typedef void (*RTimelineTimeCallback)(Rid timeline, double seconds, void* user_data);
typedef void (*RTimelineItemCallback)(Rid timeline, int item, void* user_data);
typedef int (*RNodeGraphConnectCallback)(Rid graph, int output_pin, int input_pin, void* user_data);
typedef void (*RNodeGraphItemCallback)(Rid graph, int item, void* user_data);
typedef void (*RNodeGraphContextCallback)(Rid graph, int node, float x, float y, void* user_data);
typedef void (*RBlockEditorCallback)(Rid editor, int block, void* user_data);
typedef void (*RBlockEditorContextCallback)(Rid editor, int block, float x, float y, void* user_data);
typedef void (*RDialogClosedCallback)(Rid content, int result, void* user_data);
typedef void (*RFileBrowserCallback)(Rid browser, const char* path, void* user_data);
typedef void (*RViewportDrawCallback)(const RViewportDrawInfo* info, void* user_data);
typedef void (*RViewportResizeCallback)(Rid viewport, int width, int height, void* user_data);
typedef void (*RPaintCallback)(Rid widget, RPainter* painter, float width, float height, void* user_data);
typedef void (*RMeasureCallback)(Rid widget, float* width, float* height, void* user_data);
typedef void (*RXmlReloadCallback)(const char* path, int ok, void* user_data);
typedef Rid (*RXmlTagCreateCallback)(const char* tag, void* user_data);
typedef int (*RXmlTagAttributeCallback)(Rid widget, const char* name, const char* value, void* user_data);
typedef void (*RLocaleChangedCallback)(const char* language, void* user_data);
struct RInitDesc {
	RBackend backend;
	const char* app_name;
	int worker_threads;
	int disable_render_thread;
	int disable_vsync;
	int enable_validation;
	int headless;
	float ui_scale;
};
struct RAllocator {
	void* alloc;
	void* realloc;
	void* free;
	void* user_data;
};
struct RMemoryStats {
	size_t used_bytes;
	size_t peak_bytes;
	size_t reserved_bytes;
	size_t allocation_count;
	uint64_t total_allocations;
};
struct RFrameStats {
	double layout_ms;
	double paint_ms;
	double render_ms;
	Ruint widget_count;
	Ruint instance_count;
	Ruint draw_call_count;
	Ruint backdrop_count;
	Ruint paint_job_count;
	uint64_t frame_index;
};
struct RNativeWindow {
	RWindowSystem system;
	void* display;
	uintptr_t window;
	uintptr_t visual;
	int transparent;
};
struct REvent {
	REventType type;
	Rid window;
	Rid target;
	float x;
	float y;
	float local_x;
	float local_y;
	float delta_x;
	float delta_y;
	float wheel_x;
	float wheel_y;
	RMouseButton button;
	int clicks;
	RKey key;
	Ruint modifiers;
	int repeat;
	Ruint codepoint;
};
struct RDropEvent {
	RDropPhase phase;
	Rid window;
	float x;
	float y;
	float local_x;
	float local_y;
	const char* const* paths;
	int path_count;
	const char* text;
	const char* drag_type;
	Rid drag_source;
};
struct RTileItem {
	const char* text;
	const char* detail;
	const char* badge;
	Rid icon;
	float uv[4];
	float tint[4];
	float icon_radius;
	float text_color[4];
};
struct RLanguageDesc {
	const char* name;
	const char* extensions;
	const char* keywords;
	const char* types;
	const char* constants;
	const char* line_comment;
	const char* block_comment_start;
	const char* block_comment_end;
	const char* string_quotes;
	const char* multiline_strings;
	const char* directive;
	const char* annotation;
	Ruint flags;
	const char* script_language;
};
struct RFileFilter {
	const char* name;
	const char* patterns;
};
struct RFileDialogDesc {
	Rid parent;
	const char* title;
	const char* directory;
	const char* file_name;
	const RFileFilter* filters;
	int filter_count;
	const char* default_extension;
	Ruint flags;
};
struct RViewportDrawInfo {
	RBackend backend;
	Rid viewport;
	Rid window;
	int x;
	int y;
	int width;
	int height;
	int clip_x;
	int clip_y;
	int clip_width;
	int clip_height;
	int framebuffer_width;
	int framebuffer_height;
	uint64_t frame_index;
	void* command_buffer;
	void* render_pass;
	Ruint color_format;
};
struct RSpring {
	float value;
	float velocity;
	int started;
};
struct RShaderLook {
	float color[4];
	float color2[4];
	float params[4];
	float radius;
	Rid texture;
	int animate;
	int pointer;
	float hover;
	float press;
};
struct RXmlError {
	int line;
	int column;
	char message[160];
};
struct RXmlAttributeInfo {
	const char* name;
	RXmlValueType type;
	const char* choices;
	const char* group;
	const char* description;
};
struct RAccessibilitySettings {
	float text_scale;
	int high_contrast;
	int reduce_motion;
	int screen_reader;
};
struct RHostPlatformDesc {
	void* set_cursor;
	void* clipboard_set;
	void* clipboard_get;
	void* wake;
	void* user_data;
};
int Rinit(RBackend backend);
int Rinit_ex(const RInitDesc* desc);
void Rshutdown(void);
int Ris_initialized(void);
const char* Rget_version(void);
void Rset_allocator(const RAllocator* allocator);
const RAllocator* Rallocator_get_system(void);
void* Rmemory_alloc(size_t size);
void* Rmemory_realloc(void* ptr, size_t size);
void Rmemory_free(void* ptr);
void Rmemory_get_stats(RMemoryStats* stats);
int Rmemory_check(void);
void Rmemory_trim(void);
int Rdraw_frame(void);
int Rwait_frame(double timeout_seconds);
void Rquit(int exit_code);
int Rget_exit_code(void);
int Ris_running(void);
void Rpoll_events(void);
void Rwait_events(double timeout_seconds);
void Rset_frame_callback(RFrameCallback callback, void* user_data);
double Rget_delta_time(void);
double Rget_time(void);
void Rset_ui_scale(float scale);
void Rset_vsync(int enabled);
int Rget_vsync(void);
float Rget_ui_scale(void);
void Rset_animations_enabled(int enabled);
int Rget_animations_enabled(void);
void Rset_animation_speed(float speed);
float Rget_animation_speed(void);
void Rset_animation_time_step(double seconds);
void Rtask_post(RTaskCallback task, void* user_data);
void Rtask_run_async(RTaskCallback task, void* user_data);
void Rparallel_for(Ruint count, Ruint grain, RParallelRangeCallback function, void* user_data);
void Rwake_up(void);
int Rget_worker_thread_count(void);
int Ris_ui_thread(void);
Rid Rtimer_create(double interval_seconds, int repeat, RTimerCallback callback, void* user_data);
void Rtimer_destroy(Rid timer);
int Rtimer_is_valid(Rid timer);
void Raction_bind(const char* name, RActionCallback callback, void* user_data);
void Raction_unbind(const char* name);
void Raction_fire(const char* name, Rid sender);
void Raction_set_default_handler(RActionCallback callback, void* user_data);
int Ropen_url(const char* url);
void Rclipboard_set_text(const char* text);
const char* Rclipboard_get_text(void);
RError Rerror_get_last(const char** detail, RSeverity* severity);
const char* Rerror_to_string(RError error);
const char* Rseverity_to_string(RSeverity severity);
void Rerror_set_callback(RErrorCallback callback);
void Rset_device_reset_callback(RDeviceResetCallback callback, void* user_data);
void Rwindow_get_frame_stats(Rid window, RFrameStats* stats);
Rid Rwindow_create(const char* title, int width, int height, Ruint flags);
void Rwindow_destroy(Rid window);
int Rwindow_is_valid(Rid window);
void Rwindow_set_content(Rid window, Rid widget);
Rid Rwindow_get_content(Rid window);
void Rwindow_set_title(Rid window, const char* title);
const char* Rwindow_get_title(Rid window);
void Rwindow_set_size(Rid window, int width, int height);
void Rwindow_get_size(Rid window, int* width, int* height);
void Rwindow_set_min_size(Rid window, int width, int height);
void Rwindow_set_position(Rid window, int x, int y);
void Rwindow_get_position(Rid window, int* x, int* y);
void Rwindow_get_framebuffer_size(Rid window, int* width, int* height);
float Rwindow_get_scale(Rid window);
void Rwindow_show(Rid window);
void Rwindow_hide(Rid window);
void Rwindow_maximize(Rid window);
void Rwindow_minimize(Rid window);
void Rwindow_restore(Rid window);
int Rwindow_is_maximized(Rid window);
int Rwindow_is_minimized(Rid window);
int Rwindow_is_active(Rid window);
void Rwindow_close(Rid window);
void Rwindow_begin_move(Rid window);
void Rwindow_begin_resize(Rid window, RWindowEdge edge);
void Rwindow_set_icon(Rid window, const uint8_t* rgba, int width, int height);
void Rwindow_set_clear_color(Rid window, float r, float g, float b, float a);
void Rwindow_set_background(Rid window, Rid texture);
int Rwindow_set_backdrop(Rid window, RBackdrop backdrop);
RBackdrop Rwindow_get_backdrop(Rid window);
int Rwindow_set_opacity(Rid window, float opacity);
float Rwindow_get_opacity(Rid window);
int Rwindow_set_topmost(Rid window, int topmost);
void Rwindow_center(Rid window);
void Rwindow_request_redraw(Rid window);
void Rwindow_set_continuous_redraw(Rid window, int enabled);
void Rwindow_set_close_callback(Rid window, RWindowCloseCallback callback, void* user_data);
void Rwindow_set_resize_callback(Rid window, RWindowResizeCallback callback, void* user_data);
void Rwindow_set_drop_callback(Rid window, RDropCallback callback, void* user_data);
void Rwindow_set_user_handle(Rid window, void* handle);
void* Rwindow_get_user_handle(Rid window);
int Rwindow_get_native_handle(Rid window, RNativeWindow* native);
uint8_t* Rwindow_capture(Rid window, int* width, int* height);
Rid Rwindow_get_focused_widget(Rid window);
Rid Rwindow_get_hovered_widget(Rid window);
void Rwindow_get_mouse_position(Rid window, float* x, float* y);
void Rwindow_inject_mouse_move(Rid window, float x, float y);
void Rwindow_inject_mouse_button(Rid window, RMouseButton button, int pressed);
void Rwindow_inject_mouse_wheel(Rid window, float wheel_x, float wheel_y);
void Rwindow_inject_key(Rid window, RKey key, int pressed, Ruint modifiers);
void Rwindow_inject_text(Rid window, const char* text);
int Rwindow_inject_drag(Rid window, float x, float y, int inside);
int Rwindow_inject_drop(Rid window, float x, float y, const char* const* paths, int path_count, const char* text);
int Rwindow_is_key_down(Rid window, RKey key);
int Rwindow_is_mouse_button_down(Rid window, RMouseButton button);
Ruint Rwindow_get_modifiers(Rid window);
int Rwindow_is_mouse_inside(Rid window);
int Rwindow_wants_text_input(Rid window);
int Rwindow_has_popup(Rid window);
int Rwindow_get_text_input_rect(Rid window, int* x, int* y, int* width, int* height);
int Rwindow_get_popup_count(Rid window);
Rid Rwindow_get_popup(Rid window, int index);
void Rwindow_set_tab_navigation(Rid window, int enabled);
void Rwindow_set_event_hook(Rid window, RWindowEventHook hook, void* user_data);
int Rdrag_begin(Rid source, const char* type, const char* data, const char* label);
int Rdrag_is_active(Rid window);
const char* Rdrag_get_type(Rid window);
const char* Rdrag_get_data(Rid window);
void Rdrag_cancel(Rid window);
void Rwindow_register_shortcut(Rid window, RKey key, Ruint modifiers, const char* action);
void Rwindow_unregister_shortcut(Rid window, RKey key, Ruint modifiers);
void Rlook_set(RLook look);
RLook Rlook_get(void);
void Rtheme_apply(RTheme theme);
void Rtheme_set_color(RColorRole role, float r, float g, float b, float a);
void Rtheme_get_color(RColorRole role, float* r, float* g, float* b, float* a);
void Rtheme_set_metric(RMetric metric, float value);
float Rtheme_get_metric(RMetric metric);
void Rtheme_set_material(RMaterial material);
RMaterial Rtheme_get_material(void);
void Rtheme_set_font(Rid font);
Rid Rtheme_get_font(void);
void Rtheme_set_accent(float r, float g, float b);
void Rtheme_set_dark(int dark);
int Rtheme_is_dark(void);
void Rtheme_set_wallpaper(Rid texture);
Rid Rtheme_get_wallpaper(void);
void Rtheme_set_style_color(const char* widget, const char* style_class, RColorRole role, float r, float g, float b, float a);
void Rtheme_set_style_metric(const char* widget, const char* style_class, RMetric metric, float value);
void Rtheme_set_style_material(const char* widget, const char* style_class, RMaterial material);
void Rtheme_clear_styles(void);
const char* Rtheme_load(const char* path);
const char* Rtheme_load_from_memory(const char* xml, size_t size);
int Rtheme_use(const char* name);
const char* Rtheme_get_name(void);
int Rtheme_get_count(void);
const char* Rtheme_get_name_at(int index);
int Rtheme_save(const char* path, const char* name);
Rid Rfont_create(const char* data, size_t data_size);
Rid Rfont_create_from_file(const char* path);
void Rfont_destroy(Rid font);
int Rfont_is_valid(Rid font);
Rid Rfont_get_default(void);
Rid Rfont_get_monospace(void);
void Rfont_set_fallback(Rid font, Rid fallback);
void Rfont_set_variant(Rid font, RFontStyle style, Rid variant);
Rid Rfont_get_variant(Rid font, RFontStyle style);
void Rfont_measure_text(Rid font, float size, const char* text, float* width, float* height);
Rid Rtexture_create(const char* data, size_t data_size);
Rid Rtexture_create_async(const char* data, size_t data_size);
Rid Rtexture_create_from_file(const char* path);
Rid Rtexture_create_from_file_async(const char* path);
Rid Rtexture_create_from_pixels(const uint8_t* rgba, int width, int height);
void Rtexture_update(Rid texture, const uint8_t* rgba, int width, int height);
uint8_t* Rimage_decode(const void* data, size_t data_size, int* width, int* height);
void Rpixels_free(uint8_t* pixels);
int Rtexture_is_ready(Rid texture);
void Rtexture_wait(Rid texture);
void Rtexture_destroy(Rid texture);
int Rtexture_is_valid(Rid texture);
void Rtexture_get_size(Rid texture, int* width, int* height);
void Rtexture_set_filter(Rid texture, RFilterType filter);
Rid Rwidget_create(RWidgetType type);
void Rwidget_destroy(Rid widget);
int Rwidget_is_valid(Rid widget);
RWidgetType Rwidget_get_type(Rid widget);
Rid Rwidget_add_child(Rid parent, Rid child);
Rid Rwidget_insert_child(Rid parent, Rid child, int index);
void Rwidget_remove_child(Rid parent, Rid child);
void Rwidget_clear_children(Rid parent);
Rid Rwidget_get_parent(Rid widget);
int Rwidget_get_child_count(Rid widget);
Rid Rwidget_get_child(Rid widget, int index);
Rid Rwidget_get_window(Rid widget);
void Rwidget_set_name(Rid widget, const char* name);
const char* Rwidget_get_name(Rid widget);
Rid Rwidget_find(Rid scope, const char* name);
void Rslot_set_padding(Rid child, float left, float top, float right, float bottom);
void Rslot_set_halign(Rid child, RHAlign align);
void Rslot_set_valign(Rid child, RVAlign align);
void Rslot_set_auto(Rid child);
void Rslot_set_fill(Rid child, float coefficient);
void Rslot_set_max_size(Rid child, float max_size);
void Rslot_set_grid(Rid child, int column, int row);
void Rslot_set_grid_span(Rid child, int column_span, int row_span);
void Rslot_set_anchors(Rid child, float min_x, float min_y, float max_x, float max_y);
void Rslot_set_position(Rid child, float x, float y);
void Rslot_set_size(Rid child, float width, float height);
void Rslot_set_alignment(Rid child, float x, float y);
void Rslot_set_title(Rid child, const char* title);
void Rwidget_set_visibility(Rid widget, RVisibility visibility);
RVisibility Rwidget_get_visibility(Rid widget);
void Rwidget_set_enabled(Rid widget, int enabled);
int Rwidget_is_enabled(Rid widget);
void Rwidget_set_hit_testable(Rid widget, int hit_testable);
void Rwidget_set_opacity(Rid widget, float opacity);
void Rwidget_set_transition(Rid widget, RTransition transition, float delay_seconds);
void Rwidget_play_transition(Rid widget, RTransition transition, float delay_seconds);
void Rwidget_set_clipping(Rid widget, int clip);
void Rwidget_set_tooltip(Rid widget, const char* text);
const char* Rwidget_get_tooltip(Rid widget);
void Rwidget_set_cursor(Rid widget, RCursor cursor);
void Rwidget_set_size(Rid widget, float width, float height);
void Rwidget_set_min_size(Rid widget, float width, float height);
void Rwidget_set_max_size(Rid widget, float width, float height);
void Rwidget_get_rect(Rid widget, float* x, float* y, float* width, float* height);
void Rwidget_get_desired_size(Rid widget, float* width, float* height);
int Rwidget_is_hovered(Rid widget);
void Rwidget_set_focusable(Rid widget, int focusable);
void Rwidget_focus(Rid widget);
int Rwidget_is_focused(Rid widget);
void Rwidget_set_user_handle(Rid widget, void* handle);
void* Rwidget_get_user_handle(Rid widget);
void Rwidget_set_destroy_callback(Rid widget, RWidgetDestroyCallback callback, void* user_data);
void Rwidget_set_event_callback(Rid widget, RWidgetEventCallback callback, void* user_data);
void Rwidget_set_action(Rid widget, const char* action);
const char* Rwidget_get_action(Rid widget);
void Rwidget_set_drop_callback(Rid widget, RDropCallback callback, void* user_data);
void Rwidget_set_window_drag(Rid widget, int enabled);
Rid Rwidget_hit_test(Rid root, float x, float y);
Rid Rwidget_pick(Rid root, float x, float y);
void Rwidget_set_context_menu(Rid widget, Rid menu);
void Rwidget_invalidate(Rid widget);
void Rwidget_set_color(Rid widget, RColorRole role, float r, float g, float b, float a);
void Rwidget_clear_color(Rid widget, RColorRole role);
void Rwidget_get_color(Rid widget, RColorRole role, float* r, float* g, float* b, float* a);
void Rwidget_set_material(Rid widget, RMaterial material);
RMaterial Rwidget_get_material(Rid widget);
void Rwidget_set_metric(Rid widget, RMetric metric, float value);
void Rwidget_clear_metric(Rid widget, RMetric metric);
float Rwidget_get_metric(Rid widget, RMetric metric);
void Rwidget_set_class(Rid widget, const char* classes);
const char* Rwidget_get_class(Rid widget);
void Rwidget_add_class(Rid widget, const char* style_class);
void Rwidget_remove_class(Rid widget, const char* style_class);
int Rwidget_has_class(Rid widget, const char* style_class);
int Rwidget_set_theme(Rid widget, const char* name);
const char* Rwidget_get_theme(Rid widget);
void Rpanel_set_spacing(Rid panel, float spacing);
float Rpanel_get_spacing(Rid panel);
void Rgrid_set_column_fill(Rid grid, int column, float coefficient);
void Rgrid_set_row_fill(Rid grid, int row, float coefficient);
void Runiform_grid_set_columns(Rid grid, int columns);
void Runiform_grid_set_min_cell_size(Rid grid, float width, float height);
void Rborder_set_padding(Rid border, float left, float top, float right, float bottom);
void Rborder_set_background(Rid border, float r, float g, float b, float a);
void Rborder_set_outline(Rid border, float width, float r, float g, float b, float a);
void Rborder_set_corner_radius(Rid border, float top_left, float top_right, float bottom_right, float bottom_left);
void Rborder_set_shadow(Rid border, float blur, float offset_x, float offset_y);
void Rborder_set_material(Rid border, RMaterial material);
void Rsize_box_set_aspect_ratio(Rid size_box, float ratio);
void Rspacer_set_size(Rid spacer, float width, float height);
void Rscroll_box_set_orientation(Rid scroll_box, ROrientation orientation);
void Rscroll_box_set_scroll_both(Rid scroll_box, int enabled);
void Rscroll_box_set_offset(Rid scroll_box, float x, float y);
void Rscroll_box_get_offset(Rid scroll_box, float* x, float* y);
void Rscroll_box_scroll_to_end(Rid scroll_box);
void Rwidget_scroll_into_view(Rid widget);
void Rsplitter_set_orientation(Rid splitter, ROrientation orientation);
void Rsplitter_set_min_pane_size(Rid splitter, float size);
Rid Rbackground_blur_create(void);
void Rbackground_blur_set_style(Rid blur, RBlurStyle style);
RBlurStyle Rbackground_blur_get_style(Rid blur);
void Rbackground_blur_set_radius(Rid blur, float radius);
float Rbackground_blur_get_radius(Rid blur);
void Rbackground_blur_set_tint(Rid blur, float r, float g, float b, float a);
void Rbackground_blur_clear_tint(Rid blur);
void Rbackground_blur_set_saturation(Rid blur, float saturation);
void Rbackground_blur_set_grain(Rid blur, float grain);
void Rbackground_blur_set_corner_radius(Rid blur, float top_left, float top_right, float bottom_right, float bottom_left);
void Rbackground_blur_set_padding(Rid blur, float left, float top, float right, float bottom);
void Rbackground_blur_set_shadow(Rid blur, float shadow_blur);
void Rswitcher_set_index(Rid switcher, int index);
int Rswitcher_get_index(Rid switcher);
int Rtab_view_add_tab(Rid tab_view, const char* title, Rid content);
void Rtab_view_set_active(Rid tab_view, int index);
int Rtab_view_get_active(Rid tab_view);
void Rtab_view_set_changed_callback(Rid tab_view, RTabChangedCallback callback, void* user_data);
void Rtab_view_set_documents(Rid tab_view, int documents);
int Rtab_view_get_tab_count(Rid tab_view);
Rid Rtab_view_get_tab(Rid tab_view, int index);
int Rtab_view_find_tab(Rid tab_view, Rid content);
void Rtab_view_set_tab_title(Rid tab_view, Rid content, const char* title);
void Rtab_view_set_tab_icon(Rid tab_view, Rid content, Rid texture, float u0, float v0, float u1, float v1, int tint);
void Rtab_view_set_tab_closable(Rid tab_view, Rid content, int closable);
void Rtab_view_show_tab(Rid tab_view, Rid content);
void Rtab_view_set_close_callback(Rid tab_view, RTabCloseCallback callback, void* user_data);
void Rexpander_set_title(Rid expander, const char* title);
const char* Rexpander_get_title(Rid expander);
void Rexpander_set_expanded(Rid expander, int expanded);
int Rexpander_is_expanded(Rid expander);
void Rexpander_set_changed_callback(Rid expander, RExpanderChangedCallback callback, void* user_data);
void Rset_layout_direction(RDirection direction);
RDirection Rget_layout_direction(void);
void Rwidget_set_direction(Rid widget, RDirection direction);
RDirection Rwidget_get_direction(Rid widget);
Rid Rlabel_create(const char* text);
void Rlabel_set_text(Rid label, const char* text);
const char* Rlabel_get_text(Rid label);
void Rlabel_set_font(Rid label, Rid font);
void Rlabel_set_text_size(Rid label, float size);
void Rlabel_set_text_align(Rid label, RTextAlign align);
void Rlabel_set_wrap(Rid label, int wrap);
void Rlabel_set_color(Rid label, float r, float g, float b, float a);
void Rlabel_set_markdown(Rid label, int enabled);
int Rlabel_get_markdown(Rid label);
void Rlabel_set_link_callback(Rid label, RLabelLinkCallback callback, void* user_data);
const char* Rlabel_get_plain_text(Rid label);
Rid Rbutton_create(const char* text);
void Rbutton_set_text(Rid button, const char* text);
void Rbutton_set_style(Rid button, RButtonStyle style);
void Rbutton_set_clicked_callback(Rid button, RButtonClickedCallback callback, void* user_data);
Rid Rcheckbox_create(const char* text);
void Rcheckbox_set_text(Rid checkbox, const char* text);
const char* Rcheckbox_get_text(Rid checkbox);
void Rcheckbox_set_checked(Rid checkbox, int checked);
int Rcheckbox_is_checked(Rid checkbox);
void Rcheckbox_set_style(Rid checkbox, RCheckboxStyle style);
void Rcheckbox_set_changed_callback(Rid checkbox, RCheckboxChangedCallback callback, void* user_data);
void Rslider_set_range(Rid slider, float minimum, float maximum);
void Rslider_set_value(Rid slider, float value);
float Rslider_get_value(Rid slider);
void Rslider_get_range(Rid slider, float* minimum, float* maximum, float* step);
void Rslider_set_step(Rid slider, float step);
void Rslider_set_orientation(Rid slider, ROrientation orientation);
void Rslider_set_changed_callback(Rid slider, RSliderChangedCallback callback, void* user_data);
void Rprogress_bar_set_value(Rid progress_bar, float value);
float Rprogress_bar_get_value(Rid progress_bar);
void Rimage_set_texture(Rid image, Rid texture);
void Rimage_set_tint_color(Rid image, float r, float g, float b, float a);
void Rimage_set_tint_text(Rid image, int enabled);
void Rimage_set_stretch(Rid image, RStretch stretch);
void Rimage_set_region(Rid image, float min_u, float min_v, float max_u, float max_v);
void Rimage_set_corner_radius(Rid image, float radius);
void Rtext_input_set_text(Rid text_input, const char* text);
const char* Rtext_input_get_text(Rid text_input);
void Rtext_input_set_placeholder(Rid text_input, const char* text);
void Rtext_input_set_password(Rid text_input, int password);
int Rtext_input_is_password(Rid text_input);
int Rtext_input_is_read_only(Rid text_input);
void Rtext_input_set_read_only(Rid text_input, int read_only);
void Rtext_input_set_max_length(Rid text_input, int max_length);
void Rtext_input_select_all(Rid text_input);
void Rtext_input_set_changed_callback(Rid text_input, RTextChangedCallback callback, void* user_data);
void Rtext_input_set_committed_callback(Rid text_input, RTextChangedCallback callback, void* user_data);
void Rspin_box_set_range(Rid spin_box, double minimum, double maximum);
void Rspin_box_set_value(Rid spin_box, double value);
double Rspin_box_get_value(Rid spin_box);
void Rspin_box_get_range(Rid spin_box, double* minimum, double* maximum, double* step);
void Rspin_box_set_step(Rid spin_box, double step);
void Rspin_box_set_decimals(Rid spin_box, int decimals);
void Rspin_box_set_changed_callback(Rid spin_box, RSpinBoxChangedCallback callback, void* user_data);
int Rcombo_box_add_item(Rid combo_box, const char* text);
void Rcombo_box_clear_items(Rid combo_box);
int Rcombo_box_get_item_count(Rid combo_box);
const char* Rcombo_box_get_item_text(Rid combo_box, int index);
void Rcombo_box_set_selected(Rid combo_box, int index);
int Rcombo_box_get_selected(Rid combo_box);
void Rcombo_box_set_changed_callback(Rid combo_box, RComboBoxChangedCallback callback, void* user_data);
void Rseparator_set_orientation(Rid separator, ROrientation orientation);
int Rlist_view_add_item(Rid list_view, const char* text);
void Rlist_view_set_item_text(Rid list_view, int index, const char* text);
const char* Rlist_view_get_item_text(Rid list_view, int index);
void Rlist_view_remove_item(Rid list_view, int index);
void Rlist_view_clear_items(Rid list_view);
int Rlist_view_get_item_count(Rid list_view);
void Rlist_view_set_virtual_items(Rid list_view, int count, RListItemTextCallback callback, void* user_data);
void Rlist_view_set_selected(Rid list_view, int index);
int Rlist_view_get_selected(Rid list_view);
void Rlist_view_scroll_to_item(Rid list_view, int index);
void Rlist_view_set_selection_changed_callback(Rid list_view, RListEventCallback callback, void* user_data);
void Rlist_view_set_activated_callback(Rid list_view, RListEventCallback callback, void* user_data);
Rid Rtree_view_add_item(Rid tree_view, Rid parent_item, const char* text);
void Rtree_item_destroy(Rid item);
void Rtree_view_clear_items(Rid tree_view);
int Rtree_item_is_valid(Rid item);
void Rtree_item_set_text(Rid item, const char* text);
const char* Rtree_item_get_text(Rid item);
void Rtree_item_set_expanded(Rid item, int expanded);
int Rtree_item_is_expanded(Rid item);
void Rtree_item_set_user_handle(Rid item, void* handle);
void* Rtree_item_get_user_handle(Rid item);
int Rtree_view_get_item_count(Rid tree_view);
Rid Rtree_view_get_item(Rid tree_view, int index);
Rid Rtree_item_get_parent(Rid item);
int Rtree_item_get_child_count(Rid item);
Rid Rtree_item_get_child(Rid item, int index);
void Rtree_view_set_selected_item(Rid tree_view, Rid item);
Rid Rtree_view_get_selected_item(Rid tree_view);
void Rtree_view_set_selection_changed_callback(Rid tree_view, RTreeEventCallback callback, void* user_data);
void Rtree_view_set_activated_callback(Rid tree_view, RTreeEventCallback callback, void* user_data);
void Rtree_item_set_icon(Rid item, Rid texture, float u0, float v0, float u1, float v1);
void Rtree_item_set_text_color(Rid item, float r, float g, float b, float a);
void Rtree_view_set_multi_select(Rid tree_view, int enabled);
void Rtree_item_set_selected(Rid item, int selected);
int Rtree_item_is_selected(Rid item);
void Rtree_view_clear_selection(Rid tree_view);
int Rtree_view_get_selection_count(Rid tree_view);
Rid Rtree_view_get_selection(Rid tree_view, int index);
void Rtree_view_scroll_to_item(Rid tree_view, Rid item);
void Rtree_view_set_context_callback(Rid tree_view, RTreeContextCallback callback, void* user_data);
void Rtree_view_set_drag_type(Rid tree_view, const char* type);
Rid Rtree_view_get_item_at(Rid tree_view, float local_x, float local_y);
void Rtree_view_set_drop_highlight(Rid tree_view, Rid item, int whole_tree);
Rid Rmenu_add_menu(Rid menu_bar_or_menu, const char* title);
void Rmenu_set_title(Rid menu, const char* title);
Rid Rmenu_add_item(Rid menu, const char* text);
Rid Rmenu_add_separator(Rid menu);
void Rmenu_item_set_text(Rid item, const char* text);
const char* Rmenu_item_get_text(Rid item);
void Rmenu_item_set_shortcut(Rid item, RKey key, Ruint modifiers);
void Rmenu_item_set_checkable(Rid item, int checkable);
void Rmenu_item_set_checked(Rid item, int checked);
void Rmenu_item_set_icon(Rid item, Rid texture, float u0, float v0, float u1, float v1, int tint);
int Rmenu_item_is_checked(Rid item);
void Rmenu_item_set_clicked_callback(Rid item, RMenuItemClickedCallback callback, void* user_data);
void Rmenu_open(Rid menu, Rid window, float x, float y);
void Rwindow_close_popups(Rid window);
void Rtile_view_set_items(Rid tile_view, int count, RTileItemCallback callback, void* user_data);
void Rtile_view_refresh(Rid tile_view);
int Rtile_view_get_count(Rid tile_view);
void Rtile_view_set_tile_size(Rid tile_view, float width, float height, float icon_size);
void Rtile_view_set_empty_text(Rid tile_view, const char* text);
void Rtile_view_set_selected(Rid tile_view, int index, int selected);
int Rtile_view_is_selected(Rid tile_view, int index);
void Rtile_view_clear_selection(Rid tile_view);
void Rtile_view_select_only(Rid tile_view, int index);
int Rtile_view_get_current(Rid tile_view);
int Rtile_view_get_selection_count(Rid tile_view);
int Rtile_view_get_selection(Rid tile_view, int nth);
void Rtile_view_scroll_to(Rid tile_view, int index);
int Rtile_view_get_item_at(Rid tile_view, float local_x, float local_y);
void Rtile_view_set_drop_highlight(Rid tile_view, int index);
void Rtile_view_set_drag_type(Rid tile_view, const char* type);
void Rtile_view_set_drag_data_callback(Rid tile_view, RTileDragDataCallback callback, void* user_data);
void Rtile_view_set_selection_changed_callback(Rid tile_view, RTileEventCallback callback, void* user_data);
void Rtile_view_set_activated_callback(Rid tile_view, RTileEventCallback callback, void* user_data);
void Rtile_view_set_context_callback(Rid tile_view, RTileContextCallback callback, void* user_data);
Rid Rtext_editor_create(const char* text);
void Rtext_editor_set_text(Rid editor, const char* text);
const char* Rtext_editor_get_text(Rid editor);
int Rtext_editor_get_length(Rid editor);
int Rtext_editor_get_line_count(Rid editor);
const char* Rtext_editor_get_line(Rid editor, int line);
void Rtext_editor_insert_text(Rid editor, const char* text);
void Rtext_editor_set_cursor(Rid editor, int line, int column);
void Rtext_editor_get_cursor(Rid editor, int* line, int* column);
void Rtext_editor_select(Rid editor, int from_line, int from_column, int to_line, int to_column);
void Rtext_editor_select_all(Rid editor);
int Rtext_editor_get_selection(Rid editor, int* from_line, int* from_column, int* to_line, int* to_column);
const char* Rtext_editor_get_selected_text(Rid editor);
void Rtext_editor_scroll_to_line(Rid editor, int line);
void Rtext_editor_undo(Rid editor);
void Rtext_editor_redo(Rid editor);
int Rtext_editor_can_undo(Rid editor);
int Rtext_editor_can_redo(Rid editor);
void Rtext_editor_set_read_only(Rid editor, int read_only);
void Rtext_editor_set_word_wrap(Rid editor, int enabled);
void Rtext_editor_set_line_numbers(Rid editor, int enabled);
void Rtext_editor_set_tab_size(Rid editor, int spaces);
void Rtext_editor_set_tab_inserts_spaces(Rid editor, int enabled);
void Rtext_editor_set_syntax(Rid editor, RSyntax syntax);
void Rtext_editor_set_font(Rid editor, Rid font, float size);
void Rtext_editor_set_placeholder(Rid editor, const char* text);
void Rtext_editor_set_zoom(Rid editor, float zoom);
float Rtext_editor_get_zoom(Rid editor);
void Rtext_editor_set_zoomable(Rid editor, int zoomable);
void Rtext_editor_set_zoom_callback(Rid editor, RTextEditorZoomCallback callback, void* user_data);
int Rlanguage_register(const RLanguageDesc* desc);
int Rlanguage_get(const char* name, RLanguageDesc* desc);
int Rlanguage_get_count(void);
const char* Rlanguage_get_name(int index);
const char* Rlanguage_for_file(const char* path);
void Rtext_editor_set_language(Rid editor, const char* name);
const char* Rtext_editor_get_language(Rid editor);
void Rtext_editor_set_markdown(Rid editor, int enabled);
int Rtext_editor_get_markdown(Rid editor);
RToken Rtext_editor_get_token(Rid editor, int line, int column);
void Rtext_editor_set_completion(Rid editor, int enabled);
void Rtext_editor_show_completions(Rid editor);
int Rtext_editor_is_completing(Rid editor);
void Rtext_editor_set_completion_callback(Rid editor, RCompletionCallback callback, void* user_data);
void Rtext_editor_add_completion(Rid editor, const char* text, const char* detail);
void Rtext_editor_set_auto_pairs(Rid editor, int enabled);
void Rtext_editor_add_marker(Rid editor, int line, int from_column, int to_column, RMarkerKind kind, const char* message);
void Rtext_editor_clear_markers(Rid editor);
int Rtext_editor_get_marker_count(Rid editor);
void Rtheme_set_token_color(RToken token, float r, float g, float b, float a);
void Rtheme_get_token_color(RToken token, float* r, float* g, float* b, float* a);
void Rtheme_clear_token_colors(void);
void Rtext_editor_set_changed_callback(Rid editor, RTextEditorChangedCallback callback, void* user_data);
Rid Rheader_bar_create(const char* title);
void Rheader_bar_set_title(Rid bar, const char* title);
void Rheader_bar_set_buttons(Rid bar, Ruint buttons);
void Rheader_bar_set_button_side(Rid bar, RHAlign side);
void Rheader_bar_set_title_align(Rid bar, RHAlign align);
void Rheader_bar_set_title_notch(Rid bar, int notch);
void Rheader_bar_set_background(Rid bar, int enabled);
Rid Rdock_area_create(void);
void Rdock_area_add_panel(Rid area, Rid panel, const char* title, RDockSide side, Rid relative, float size);
void Rdock_area_move_panel(Rid area, Rid panel, RDockSide side, Rid relative, float size);
void Rdock_area_close_panel(Rid area, Rid panel);
void Rdock_area_set_panel_title(Rid area, Rid panel, const char* title);
void Rdock_area_set_panel_icon(Rid area, Rid panel, Rid texture, float u0, float v0, float u1, float v1, int tint);
void Rdock_area_set_panel_closable(Rid area, Rid panel, int closable);
void Rdock_area_hide_panel(Rid area, Rid panel);
void Rdock_area_open_panel(Rid area, Rid panel);
int Rdock_area_is_panel_open(Rid area, Rid panel);
void Rdock_area_set_close_hides(Rid area, int hides);
void Rdock_area_show_panel(Rid area, Rid panel);
int Rdock_area_is_panel_shown(Rid area, Rid panel);
void Rdock_area_set_movable(Rid area, int movable);
const char* Rdock_area_save_layout(Rid area);
int Rdock_area_load_layout(Rid area, const char* layout);
void Rdock_area_set_close_callback(Rid area, RDockCloseCallback callback, void* user_data);
void Rdock_area_set_tearable(Rid area, int tearable);
int Rdock_area_is_tearable(Rid area);
Rid Rdock_area_float_panel(Rid area, Rid panel, int x, int y);
void Rdock_area_dock_panel(Rid area, Rid panel);
Rid Rdock_area_get_panel_window(Rid area, Rid panel);
void Rdock_area_set_changed_callback(Rid area, RDockChangedCallback callback, void* user_data);
Rid Risland_create(void);
void Risland_set_expanded(Rid island, int expanded);
int Risland_is_expanded(Rid island);
void Risland_expand_for(Rid island, double seconds);
void Risland_set_expand_on_hover(Rid island, int enabled);
void Risland_set_expand_on_click(Rid island, int enabled);
void Risland_set_attached(Rid island, int attached);
void Risland_set_changed_callback(Rid island, RIslandChangedCallback callback, void* user_data);
void Rwindow_set_island(Rid window, Rid widget);
Rid Rwindow_get_island(Rid window);
Rid Rcolor_picker_create(void);
void Rcolor_picker_set_color(Rid picker, float r, float g, float b, float a);
void Rcolor_picker_get_color(Rid picker, float* r, float* g, float* b, float* a);
void Rcolor_picker_set_hsv(Rid picker, float hue, float saturation, float value);
void Rcolor_picker_get_hsv(Rid picker, float* hue, float* saturation, float* value);
int Rcolor_picker_set_hex(Rid picker, const char* text);
const char* Rcolor_picker_get_hex(Rid picker);
void Rcolor_picker_set_alpha_enabled(Rid picker, int enabled);
void Rcolor_picker_set_previous_color(Rid picker, float r, float g, float b, float a);
void Rcolor_picker_add_swatch(Rid picker, float r, float g, float b, float a);
void Rcolor_picker_clear_swatches(Rid picker);
int Rcolor_picker_get_swatch_count(Rid picker);
void Rcolor_picker_set_popup(Rid picker, int popup);
void Rcolor_picker_set_compact(Rid picker, int compact);
void Rcolor_picker_set_changed_callback(Rid picker, RColorChangedCallback callback, void* user_data);
void Rcolor_rgb_to_hsv(float r, float g, float b, float* hue, float* saturation, float* value);
void Rcolor_hsv_to_rgb(float hue, float saturation, float value, float* r, float* g, float* b);
Rid Rtimeline_create(void);
void Rtimeline_set_duration(Rid timeline, double seconds);
double Rtimeline_get_duration(Rid timeline);
void Rtimeline_set_time(Rid timeline, double seconds);
double Rtimeline_get_time(Rid timeline);
void Rtimeline_set_frame_rate(Rid timeline, double frames_per_second);
double Rtimeline_get_frame_rate(Rid timeline);
void Rtimeline_set_snapping(Rid timeline, int enabled);
void Rtimeline_set_view(Rid timeline, double start, double end);
void Rtimeline_get_view(Rid timeline, double* start, double* end);
void Rtimeline_set_playing(Rid timeline, int playing);
int Rtimeline_is_playing(Rid timeline);
void Rtimeline_set_loop(Rid timeline, int loop);
void Rtimeline_set_header_width(Rid timeline, float width);
float Rtimeline_get_header_width(Rid timeline);
void Rtimeline_set_ruler_height(Rid timeline, float height);
float Rtimeline_get_ruler_height(Rid timeline);
void Rtimeline_set_default_track_height(Rid timeline, float height);
float Rtimeline_get_default_track_height(Rid timeline);
int Rtimeline_add_track(Rid timeline, const char* name);
void Rtimeline_remove_track(Rid timeline, int track);
int Rtimeline_get_track_count(Rid timeline);
int Rtimeline_get_track(Rid timeline, int index);
void Rtimeline_set_track_name(Rid timeline, int track, const char* name);
const char* Rtimeline_get_track_name(Rid timeline, int track);
void Rtimeline_set_track_color(Rid timeline, int track, float r, float g, float b);
void Rtimeline_set_track_height(Rid timeline, int track, float height);
void Rtimeline_set_track_locked(Rid timeline, int track, int locked);
void Rtimeline_set_track_muted(Rid timeline, int track, int muted);
int Rtimeline_is_track_muted(Rid timeline, int track);
int Rtimeline_add_clip(Rid timeline, int track, double start, double length, const char* label);
int Rtimeline_add_key(Rid timeline, int track, double time);
int Rtimeline_add_marker(Rid timeline, double time, const char* label);
void Rtimeline_remove_item(Rid timeline, int item);
void Rtimeline_clear(Rid timeline);
RTimelineItem Rtimeline_get_item_type(Rid timeline, int item);
int Rtimeline_get_item_track(Rid timeline, int item);
double Rtimeline_get_item_start(Rid timeline, int item);
double Rtimeline_get_item_length(Rid timeline, int item);
void Rtimeline_set_item(Rid timeline, int item, int track, double start, double length);
void Rtimeline_set_item_label(Rid timeline, int item, const char* label);
const char* Rtimeline_get_item_label(Rid timeline, int item);
void Rtimeline_set_item_color(Rid timeline, int item, float r, float g, float b);
int Rtimeline_get_item_count(Rid timeline);
int Rtimeline_get_item(Rid timeline, int index);
void Rtimeline_select(Rid timeline, int item, int add);
void Rtimeline_clear_selection(Rid timeline);
int Rtimeline_is_selected(Rid timeline, int item);
int Rtimeline_get_selection_count(Rid timeline);
int Rtimeline_get_selected(Rid timeline, int index);
void Rtimeline_set_time_callback(Rid timeline, RTimelineTimeCallback callback, void* user_data);
void Rtimeline_set_changed_callback(Rid timeline, RTimelineItemCallback callback, void* user_data);
void Rtimeline_set_selection_callback(Rid timeline, RTimelineItemCallback callback, void* user_data);
void Rtimeline_set_activated_callback(Rid timeline, RTimelineItemCallback callback, void* user_data);
void Rtimeline_set_removed_callback(Rid timeline, RTimelineItemCallback callback, void* user_data);
Rid Rnode_graph_create(void);
int Rnode_graph_add_node(Rid graph, const char* title, float x, float y);
void Rnode_graph_remove_node(Rid graph, int node);
void Rnode_graph_clear(Rid graph);
int Rnode_graph_get_node_count(Rid graph);
int Rnode_graph_get_node(Rid graph, int index);
void Rnode_graph_set_node_title(Rid graph, int node, const char* title);
const char* Rnode_graph_get_node_title(Rid graph, int node);
void Rnode_graph_set_node_position(Rid graph, int node, float x, float y);
void Rnode_graph_get_node_position(Rid graph, int node, float* x, float* y);
void Rnode_graph_get_node_size(Rid graph, int node, float* width, float* height);
void Rnode_graph_set_node_color(Rid graph, int node, float r, float g, float b);
void Rnode_graph_set_node_content(Rid graph, int node, Rid widget);
Rid Rnode_graph_get_node_content(Rid graph, int node);
void Rnode_graph_set_node_name(Rid graph, int node, const char* name);
const char* Rnode_graph_get_node_name(Rid graph, int node);
int Rnode_graph_find_node(Rid graph, const char* name);
void Rnode_graph_set_node_user_data(Rid graph, int node, void* user_data);
void* Rnode_graph_get_node_user_data(Rid graph, int node);
int Rnode_graph_add_input(Rid graph, int node, const char* label, const char* type);
int Rnode_graph_add_output(Rid graph, int node, const char* label, const char* type);
void Rnode_graph_remove_pin(Rid graph, int pin);
int Rnode_graph_get_pin_node(Rid graph, int pin);
int Rnode_graph_is_pin_output(Rid graph, int pin);
const char* Rnode_graph_get_pin_label(Rid graph, int pin);
const char* Rnode_graph_get_pin_type(Rid graph, int pin);
void Rnode_graph_set_pin_name(Rid graph, int pin, const char* name);
int Rnode_graph_find_pin(Rid graph, int node, const char* name);
void Rnode_graph_get_pin_position(Rid graph, int pin, float* x, float* y);
int Rnode_graph_get_input(Rid graph, int node, int index);
int Rnode_graph_get_output(Rid graph, int node, int index);
void Rnode_graph_set_type_color(Rid graph, const char* type, float r, float g, float b);
int Rnode_graph_connect(Rid graph, int output_pin, int input_pin);
void Rnode_graph_disconnect(Rid graph, int link);
int Rnode_graph_get_link_count(Rid graph);
int Rnode_graph_get_link(Rid graph, int index);
int Rnode_graph_get_link_output(Rid graph, int link);
int Rnode_graph_get_link_input(Rid graph, int link);
int Rnode_graph_get_input_link(Rid graph, int input_pin);
void Rnode_graph_set_view(Rid graph, float center_x, float center_y, float zoom);
void Rnode_graph_get_view(Rid graph, float* center_x, float* center_y, float* zoom);
void Rnode_graph_to_canvas(Rid graph, float x, float y, float* canvas_x, float* canvas_y);
void Rnode_graph_from_canvas(Rid graph, float canvas_x, float canvas_y, float* x, float* y);
void Rnode_graph_frame_all(Rid graph);
void Rnode_graph_set_grid_visible(Rid graph, int visible);
void Rnode_graph_set_flow_visible(Rid graph, int visible);
void Rnode_graph_set_read_only(Rid graph, int read_only);
void Rnode_graph_select_node(Rid graph, int node, int add);
void Rnode_graph_clear_selection(Rid graph);
int Rnode_graph_is_node_selected(Rid graph, int node);
int Rnode_graph_get_selection_count(Rid graph);
int Rnode_graph_get_selected_node(Rid graph, int index);
void Rnode_graph_set_connect_callback(Rid graph, RNodeGraphConnectCallback callback, void* user_data);
void Rnode_graph_set_linked_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data);
void Rnode_graph_set_unlinked_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data);
void Rnode_graph_set_moved_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data);
void Rnode_graph_set_selection_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data);
void Rnode_graph_set_removed_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data);
void Rnode_graph_set_activated_callback(Rid graph, RNodeGraphItemCallback callback, void* user_data);
void Rnode_graph_set_context_callback(Rid graph, RNodeGraphContextCallback callback, void* user_data);
void Rnode_graph_set_pin_multiple(Rid graph, int pin, int multiple);
void Rnode_graph_set_node_active(Rid graph, int node, int active);
void Rnode_graph_set_node_active_color(Rid graph, int node, float r, float g, float b);
void Rnode_graph_set_node_deletable(Rid graph, int node, int deletable);
void Rnode_graph_set_node_badge(Rid graph, int node, const char* text);
Rid Rblock_editor_create(void);
int Rblock_editor_add_kind(Rid editor, const char* name, const char* label, RBlockShape shape, const char* category);
void Rblock_editor_set_category_color(Rid editor, const char* category, float r, float g, float b);
void Rblock_editor_set_palette_visible(Rid editor, int visible);
int Rblock_editor_add_block(Rid editor, const char* kind, float x, float y);
void Rblock_editor_remove_block(Rid editor, int block);
void Rblock_editor_clear(Rid editor);
int Rblock_editor_attach(Rid editor, int block, int target, RBlockPlace place, int index);
void Rblock_editor_detach(Rid editor, int block, float x, float y);
int Rblock_editor_get_script_count(Rid editor);
int Rblock_editor_get_script(Rid editor, int index);
const char* Rblock_editor_get_kind(Rid editor, int block);
int Rblock_editor_get_next(Rid editor, int block);
int Rblock_editor_get_mouth_count(Rid editor, int block);
int Rblock_editor_get_body(Rid editor, int block, int mouth);
int Rblock_editor_get_slot_count(Rid editor, int block);
int Rblock_editor_get_input(Rid editor, int block, int slot);
const char* Rblock_editor_get_value(Rid editor, int block, int slot);
void Rblock_editor_set_value(Rid editor, int block, int slot, const char* value);
int Rblock_editor_get_parent(Rid editor, int block, RBlockPlace* place, int* index);
void Rblock_editor_get_position(Rid editor, int block, float* x, float* y);
void Rblock_editor_set_block_glow(Rid editor, int block, int glow);
const char* Rblock_editor_save(Rid editor);
int Rblock_editor_load(Rid editor, const char* text);
void Rblock_editor_set_changed_callback(Rid editor, RBlockEditorCallback callback, void* user_data);
void Rblock_editor_set_clicked_callback(Rid editor, RBlockEditorCallback callback, void* user_data);
void Rblock_editor_set_context_callback(Rid editor, RBlockEditorContextCallback callback, void* user_data);
void Rblock_editor_set_slot_options(Rid editor, const char* kind, int slot, const char* options);
void Rblock_editor_set_kind_visible(Rid editor, const char* kind, int visible);
int Rblock_editor_set_kind_label(Rid editor, const char* kind, const char* label);
void Rblock_editor_set_block_deletable(Rid editor, int block, int deletable);
int Rblock_editor_get_selection_count(Rid editor);
int Rblock_editor_get_selected(Rid editor, int index);
void Rblock_editor_select(Rid editor, int block, int add);
void Rblock_editor_get_view(Rid editor, float* x, float* y, float* zoom);
void Rblock_editor_set_view(Rid editor, float x, float y, float zoom);
void Rdialog_open(Rid window, Rid content, RDialogClosedCallback closed, void* user_data);
void Rdialog_close(Rid widget, int result);
int Rdialog_run(Rid window, Rid content);
int Rwindow_has_dialog(Rid window);
RMessageResult Rmessage_box(Rid window, const char* title, const char* text, RMessageButtons buttons);
const char* Rfile_dialog_open(const RFileDialogDesc* desc);
const char* Rfile_dialog_save(const RFileDialogDesc* desc);
const char* Rfile_dialog_select_folder(const RFileDialogDesc* desc);
int Rfile_dialog_get_result_count(void);
const char* Rfile_dialog_get_result(int index);
int Rfile_register_memory(const char* path, const void* data, size_t size, int copy);
void Rfile_unregister_memory(const char* path);
int Rfile_is_memory(const char* path);
char* Rfile_read(const char* path, size_t* size);
int Rfile_write(const char* path, const void* data, size_t size);
Rid Rfile_browser_create(void);
void Rfile_browser_set_mode(Rid browser, RFileBrowserMode mode);
int Rfile_browser_set_directory(Rid browser, const char* path);
const char* Rfile_browser_get_directory(Rid browser);
void Rfile_browser_refresh(Rid browser);
void Rfile_browser_set_filters(Rid browser, const RFileFilter* filters, int count);
void Rfile_browser_set_show_hidden(Rid browser, int show);
void Rfile_browser_set_multiple(Rid browser, int multiple);
void Rfile_browser_set_controls_visible(Rid browser, int visible);
void Rfile_browser_set_places_visible(Rid browser, int visible);
void Rfile_browser_set_file_name(Rid browser, const char* name);
void Rfile_browser_set_default_extension(Rid browser, const char* extension);
int Rfile_browser_get_selection_count(Rid browser);
const char* Rfile_browser_get_selection(Rid browser, int index);
void Rfile_browser_select(Rid browser, const char* name);
void Rfile_browser_set_accepted_callback(Rid browser, RFileBrowserCallback callback, void* user_data);
void Rfile_browser_set_cancelled_callback(Rid browser, RFileBrowserCallback callback, void* user_data);
void Rfile_browser_set_selection_changed_callback(Rid browser, RFileBrowserCallback callback, void* user_data);
void Rviewport_set_texture(Rid viewport, Rid texture);
void Rviewport_set_stretch(Rid viewport, RStretch stretch);
void Rviewport_set_flip_y(Rid viewport, int flip);
void Rviewport_set_draw_callback(Rid viewport, RViewportDrawCallback callback, void* user_data);
int Rviewport_set_native(Rid viewport, int enabled);
int Rviewport_get_native_handle(Rid viewport, RNativeWindow* native);
void Rviewport_get_pixel_size(Rid viewport, int* width, int* height);
void Rviewport_set_resize_callback(Rid viewport, RViewportResizeCallback callback, void* user_data);
void Rviewport_set_realtime(Rid viewport, int realtime);
void Rcustom_set_paint_callback(Rid widget, RPaintCallback callback, void* user_data);
void Rcustom_set_measure_callback(Rid widget, RMeasureCallback callback, void* user_data);
void Rdraw_rect(RPainter* painter, float x, float y, float width, float height, float radius, float r, float g, float b, float a);
void Rdraw_rect_outline(RPainter* painter, float x, float y, float width, float height, float radius, float thickness, float r, float g, float b, float a);
void Rdraw_circle(RPainter* painter, float center_x, float center_y, float radius, float r, float g, float b, float a);
void Rdraw_line(RPainter* painter, float x0, float y0, float x1, float y1, float thickness, float r, float g, float b, float a);
void Rdraw_curve(RPainter* painter, float x0, float y0, float cx0, float cy0, float cx1, float cy1, float x1, float y1, float thickness, float r, float g, float b, float a);
void Rdraw_triangle(RPainter* painter, float x0, float y0, float x1, float y1, float x2, float y2, float r, float g, float b, float a);
void Rdraw_image(RPainter* painter, Rid texture, float x, float y, float width, float height, float r, float g, float b, float a);
void Rdraw_text(RPainter* painter, Rid font, float size, float x, float y, const char* text, float r, float g, float b, float a);
void Rdraw_glass(RPainter* painter, float x, float y, float width, float height, float radius, float r, float g, float b, float a);
float Rdraw_spring(RPainter* painter, RSpring* spring, float target, RMotion motion);
void Rspring_kick(RSpring* spring, float velocity);
void Rspring_set(RSpring* spring, float value);
void Rdraw_push_clip_rect(RPainter* painter, float x, float y, float width, float height);
void Rdraw_pop_clip_rect(RPainter* painter);
Rid Rshader_create(const void* spirv, size_t size, Ruint flags);
Rid Rshader_create_from_file(const char* path, Ruint flags);
void Rshader_destroy(Rid shader);
int Rshader_is_valid(Rid shader);
Ruint Rshader_get_flags(Rid shader);
Rid Reffect_get(REffect effect);
Rid Reffect_find(const char* name);
const char* Reffect_get_name(REffect effect);
void Reffect_get_defaults(REffect effect, RShaderLook* look);
void Rshader_look_init(RShaderLook* look);
void Rshader_look_defaults(Rid shader, RShaderLook* look);
void Rdraw_shader(RPainter* painter, Rid shader, float x, float y, float width, float height, const RShaderLook* look);
void Rborder_set_effect(Rid border, Rid shader, const RShaderLook* look);
Rid Rborder_get_effect(Rid border);
Rid Rwindow_create_from_xml(const char* xml, size_t size);
Rid Rwindow_create_from_xml_file(const char* path);
Rid Rwidget_create_from_xml(const char* xml, size_t size);
Rid Rwidget_create_from_xml_file(const char* path);
void Rxml_set_base_path(const char* path);
void Rxml_set_live_reload(int enabled);
int Rxml_get_live_reload(void);
void Rxml_set_reload_callback(RXmlReloadCallback callback, void* user_data);
int Rxml_check_files(void);
int Rxml_register_widgets(const char* xml, size_t size);
int Rxml_register_widgets_from_file(const char* path);
void Rxml_unregister_widget(const char* name);
int Rxml_is_widget_registered(const char* name);
int Rxml_get_registered_widget_count(void);
const char* Rxml_get_registered_widget_name(int index);
int Rxml_register_tag(const char* tag, RXmlTagCreateCallback create, RXmlTagAttributeCallback set_attribute, void* user_data);
void Rxml_unregister_tag(const char* tag);
int Rxml_is_tag_registered(const char* tag);
const char* Rxml_resolve_path(const char* path);
int Rwidget_set_attribute(Rid widget, const char* name, const char* value);
Rid Rxml_document_create(const char* root_tag);
Rid Rxml_document_parse(const char* xml, size_t size, RXmlError* error);
Rid Rxml_document_load(const char* path);
void Rxml_document_destroy(Rid document);
int Rxml_document_is_valid(Rid document);
Rid Rxml_document_get_root(Rid document);
const char* Rxml_document_to_string(Rid document);
int Rxml_document_save(Rid document, const char* path);
Rid Rxml_document_create_widget(Rid document);
Rid Rxml_document_create_window(Rid document);
int Rxml_node_is_valid(Rid node);
Rid Rxml_node_get_document(Rid node);
const char* Rxml_node_get_tag(Rid node);
void Rxml_node_set_tag(Rid node, const char* tag);
int Rxml_node_is_comment(Rid node);
const char* Rxml_node_get_text(Rid node);
void Rxml_node_set_text(Rid node, const char* text);
Rid Rxml_node_get_parent(Rid node);
int Rxml_node_get_child_count(Rid node);
Rid Rxml_node_get_child(Rid node, int index);
int Rxml_node_get_index(Rid node);
Rid Rxml_node_add_child(Rid parent, const char* tag, int index);
Rid Rxml_node_add_comment(Rid parent, const char* text, int index);
Rid Rxml_node_add_xml(Rid parent, const char* xml, size_t size, int index);
void Rxml_node_remove(Rid node);
int Rxml_node_move(Rid node, Rid new_parent, int index);
Rid Rxml_node_duplicate(Rid node);
const char* Rxml_node_to_string(Rid node);
int Rxml_node_get_attribute_count(Rid node);
const char* Rxml_node_get_attribute_name(Rid node, int index);
const char* Rxml_node_get_attribute_value(Rid node, int index);
const char* Rxml_node_get_attribute(Rid node, const char* name);
void Rxml_node_set_attribute(Rid node, const char* name, const char* value);
int Rxml_node_get_line(Rid node);
Rid Rxml_node_get_widget(Rid node);
Rid Rwidget_get_xml_node(Rid widget);
int Rxml_get_tag_count(void);
const char* Rxml_get_tag_name(int index);
const char* Rxml_get_tag_description(const char* tag);
int Rxml_get_tag_children(const char* tag);
int Rxml_get_attribute_count(const char* tag);
int Rxml_get_attribute_info(const char* tag, int index, RXmlAttributeInfo* info);
int Rlocale_load_csv(const char* path);
int Rlocale_load_csv_from_memory(const char* csv, size_t size);
void Rlocale_clear(void);
void Rlocale_set_text(const char* language, const char* key, const char* text);
int Rlocale_get_language_count(void);
const char* Rlocale_get_language_at(int index);
int Rlocale_set_language(const char* code);
const char* Rlocale_get_language(void);
const char* Rlocale_get_system_language(void);
void Rlocale_set_fallback_language(const char* code);
int Rlocale_is_rtl(void);
void Rlocale_set_changed_callback(RLocaleChangedCallback callback, void* user_data);
const char* Rtr(const char* key);
int Rtr_exists(const char* key);
const char* Rtr_format(const char* key, const char* const* args, int arg_count);
const char* Rtr_plural(const char* key, int count);
void Rwidget_set_text_key(Rid widget, const char* attribute, const char* key);
const char* Rlocale_format_number(double value, int decimals);
void Raccessibility_set_text_scale(float scale);
float Raccessibility_get_text_scale(void);
void Raccessibility_set_high_contrast(int enabled);
int Raccessibility_get_high_contrast(void);
void Raccessibility_set_focus_always_visible(int enabled);
int Raccessibility_get_focus_always_visible(void);
void Raccessibility_set_reduce_motion(int enabled);
int Raccessibility_get_reduce_motion(void);
void Raccessibility_set_color_filter(RColorFilter filter);
RColorFilter Raccessibility_get_color_filter(void);
void Raccessibility_get_system_settings(RAccessibilitySettings* settings);
void Raccessibility_follow_system(void);
void Rwidget_set_accessible_name(Rid widget, const char* name);
const char* Rwidget_get_accessible_name(Rid widget);
void Rwidget_set_accessible_description(Rid widget, const char* description);
void Rwidget_set_accessible_role(Rid widget, RAccessibleRole role);
RAccessibleRole Rwidget_get_accessible_role(Rid widget);
void Rwidget_set_accessible_hidden(Rid widget, int hidden);
void Rwidget_set_accessible_labelled_by(Rid widget, Rid label);
void Raccessibility_announce(const char* text, int assertive);
int Raccessibility_is_screen_reader_active(void);
void Rhost_platform_register(const RHostPlatformDesc* desc);
int Rhost_platform_is_active(void);
void Rhost_window_set_size(Rid window, int width, int height, float scale);
void Rhost_inject_mouse_move(Rid window, int x, int y, Ruint modifiers);
void Rhost_inject_mouse_button(Rid window, int x, int y, RMouseButton button, int pressed, Ruint modifiers);
void Rhost_inject_mouse_wheel(Rid window, float wheel_x, float wheel_y, Ruint modifiers);
void Rhost_inject_mouse_leave(Rid window);
void Rhost_inject_key(Rid window, RKey key, int pressed, Ruint modifiers, int repeat);
void Rhost_inject_text(Rid window, uint32_t codepoint);
void Rhost_inject_focus(Rid window, int focused);
void Rhost_inject_close(Rid window);
]]

local library = ffi.load(os.getenv("RAVEN_LIBRARY") or "raven")

-- the constants of the headers that are not enumerations
local constants = {
	RAVEN_VERSION_MAJOR = 0,
	RAVEN_VERSION_MINOR = 1,
	RAVEN_VERSION_PATCH = 0,
	RAVEN_API_VERSION = "0.1",
	R_FALSE = 0,
	R_TRUE = 1,
	R_INVALID_ID = 0xFFFFFFFF,
}

return setmetatable({ ffi = ffi, C = library }, {
	__index = function(_, name)
		local constant = constants[name]
		if constant ~= nil then return constant end
		return library[name]
	end
})

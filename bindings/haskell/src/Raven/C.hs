{-# LANGUAGE PatternSynonyms, DerivingStrategies, GeneralizedNewtypeDeriving #-}
{-# OPTIONS_GHC -Wno-missing-pattern-synonym-signatures -Wno-unused-imports #-}

-- | Raven for Haskell: the C API as it is.
--
-- Generated from the C headers by tools/bindgen (Raven 0.2): do not edit.
--
-- Every function of C is here, its first letter lowered (@Rwindow_create@ is
-- 'rwindow_create'), with the types of "Foreign.C": texts are 'CString', pointers
-- 'Ptr'. Enumerations are newtypes over 'CInt' whose values keep the names of C
-- ('R_BACKEND_NULL' is a pattern). Structures are records with a 'Storable'
-- instance; their fields are named after the structure (@rXmlError_line@), and
-- @rXmlError@ is the zeroed structure. A callback of type @RXxxCallback@ is made
-- of a Haskell function by @mkRXxxCallback@.
--
-- The module "Raven" re-exports this one, with helpers for texts and closures.
module Raven.C where

import Data.Bits (Bits)
import Data.Word (Word8, Word32, Word64)
import Foreign.C.String (CString)
import Foreign.C.Types
import Foreign.Marshal.Array (peekArray, pokeArray)
import Foreign.Ptr
import Foreign.Storable

-- | A handle: every object of the library is one.
type Rid = Word32

-- | An unsigned number of the API (flags, codepoints).
type Ruint = Word32

pattern RAVEN_VERSION_MAJOR :: CInt
pattern RAVEN_VERSION_MAJOR = 0
pattern RAVEN_VERSION_MINOR :: CInt
pattern RAVEN_VERSION_MINOR = 2
pattern RAVEN_VERSION_PATCH :: CInt
pattern RAVEN_VERSION_PATCH = 3
rAVEN_API_VERSION :: String
rAVEN_API_VERSION = "0.2"
pattern R_FALSE :: CInt
pattern R_FALSE = 0
pattern R_TRUE :: CInt
pattern R_TRUE = 1
pattern R_INVALID_ID :: Rid
pattern R_INVALID_ID = 0xFFFFFFFF

-- | An object of the library seen through a pointer only.
data RPainter

newtype RBackend = RBackend CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | Built-in Vulkan renderer.
pattern R_BACKEND_VULKAN :: RBackend
pattern R_BACKEND_VULKAN = RBackend 1
-- | Renders nothing. Used for headless runs and unit tests.
pattern R_BACKEND_NULL :: RBackend
pattern R_BACKEND_NULL = RBackend 2
-- | Backend supplied with Rbackend_register (see raven_backend.h).
pattern R_BACKEND_CUSTOM :: RBackend
pattern R_BACKEND_CUSTOM = RBackend 3
-- | Browser renderer (Emscripten, WebGL 2).
pattern R_BACKEND_WEBGL :: RBackend
pattern R_BACKEND_WEBGL = RBackend 4

newtype RError = RError CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_NO_ERROR :: RError
pattern R_NO_ERROR = RError 112
pattern R_ERROR_INVALID_ID :: RError
pattern R_ERROR_INVALID_ID = RError 113
pattern R_ERROR_INVALID_ENUM :: RError
pattern R_ERROR_INVALID_ENUM = RError 114
pattern R_ERROR_INVALID_VALUE :: RError
pattern R_ERROR_INVALID_VALUE = RError 115
pattern R_ERROR_INVALID_OPERATION :: RError
pattern R_ERROR_INVALID_OPERATION = RError 116
pattern R_ERROR_BACKEND :: RError
pattern R_ERROR_BACKEND = RError 117
pattern R_ERROR_PLATFORM :: RError
pattern R_ERROR_PLATFORM = RError 118
pattern R_ERROR_OUT_OF_MEMORY :: RError
pattern R_ERROR_OUT_OF_MEMORY = RError 119
pattern R_ERROR_INVALID_FILE_FORMAT :: RError
pattern R_ERROR_INVALID_FILE_FORMAT = RError 120
pattern R_ERROR_FILE_NOT_FOUND :: RError
pattern R_ERROR_FILE_NOT_FOUND = RError 121
pattern R_ERROR_XML :: RError
pattern R_ERROR_XML = RError 122
pattern R_ERROR_WRONG_THREAD :: RError
pattern R_ERROR_WRONG_THREAD = RError 123

newtype RSeverity = RSeverity CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_SEVERITY_WEAK_WARNING :: RSeverity
pattern R_SEVERITY_WEAK_WARNING = RSeverity 128
pattern R_SEVERITY_WARNING :: RSeverity
pattern R_SEVERITY_WARNING = RSeverity 129
pattern R_SEVERITY_ERROR :: RSeverity
pattern R_SEVERITY_ERROR = RSeverity 130
pattern R_SEVERITY_FATAL :: RSeverity
pattern R_SEVERITY_FATAL = RSeverity 131

newtype RWindowFlags = RWindowFlags CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_WINDOW_DEFAULT :: RWindowFlags
pattern R_WINDOW_DEFAULT = RWindowFlags 0
pattern R_WINDOW_NO_RESIZE :: RWindowFlags
pattern R_WINDOW_NO_RESIZE = RWindowFlags 1
pattern R_WINDOW_HIDDEN :: RWindowFlags
pattern R_WINDOW_HIDDEN = RWindowFlags 2
pattern R_WINDOW_BORDERLESS :: RWindowFlags
pattern R_WINDOW_BORDERLESS = RWindowFlags 4
pattern R_WINDOW_MAXIMIZED :: RWindowFlags
pattern R_WINDOW_MAXIMIZED = RWindowFlags 8
-- | No title bar from the OS: the content covers the whole window and
-- draws its own header, with a R_WIDGET_HEADER_BAR (raven_shell.h) or
-- with any widget given to Rwidget_set_window_drag. The window is still
-- moved, resized, snapped and maximized by the OS, as it does for its own
-- title bars.
pattern R_WINDOW_CUSTOM_HEADER :: RWindowFlags
pattern R_WINDOW_CUSTOM_HEADER = RWindowFlags 16
-- | The window can be seen through: where the application draws nothing
-- opaque, what is behind the window on the screen shows (the desktop,
-- the other applications), not a color of the application. Glass then
-- looks at the screen itself. What the OS does to what shows through is
-- chosen with Rwindow_set_backdrop. See RBackdrop for what each window
-- system can do.
pattern R_WINDOW_TRANSPARENT :: RWindowFlags
pattern R_WINDOW_TRANSPARENT = RWindowFlags 32
-- | A splash screen: shown while the application starts. No frame (it
-- implies R_WINDOW_BORDERLESS and R_WINDOW_NO_RESIZE, and cancels
-- R_WINDOW_MAXIMIZED and R_WINDOW_CUSTOM_HEADER), centered on the screen
-- before it is first shown and again when its size changes, above the
-- other windows, out of the taskbar and of Alt+Tab (Windows), of the
-- Window menu and of Cmd+` (macOS), typed _NET_WM_WINDOW_TYPE_SPLASH on
-- X11. Wayland gives it no place of its own: it is a window without frame
-- there. The application destroys it once its main window is ready
-- (creating the main window first keeps Ris_running true).
pattern R_WINDOW_SPLASH_SCREEN :: RWindowFlags
pattern R_WINDOW_SPLASH_SCREEN = RWindowFlags 64

-- | Where a window in picture in picture goes (Rwindow_set_picture_in_picture_layout).
newtype RPictureInPictureCorner = RPictureInPictureCorner CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_PIP_BOTTOM_RIGHT :: RPictureInPictureCorner
pattern R_PIP_BOTTOM_RIGHT = RPictureInPictureCorner 1920
pattern R_PIP_BOTTOM_LEFT :: RPictureInPictureCorner
pattern R_PIP_BOTTOM_LEFT = RPictureInPictureCorner 1921
pattern R_PIP_TOP_RIGHT :: RPictureInPictureCorner
pattern R_PIP_TOP_RIGHT = RPictureInPictureCorner 1922
pattern R_PIP_TOP_LEFT :: RPictureInPictureCorner
pattern R_PIP_TOP_LEFT = RPictureInPictureCorner 1923

-- | What is seen behind a window created with R_WINDOW_TRANSPARENT.
--
--   Windows 11          every value
--   Windows 10          TRANSPARENT, BLUR, ACRYLIC (MICA gives ACRYLIC)
--   macOS               TRANSPARENT; other backdrops fall back to transparency
--   X11                 TRANSPARENT with any compositing manager; BLUR where
--                       the compositor blurs on request (KWin; picom and
--                       others with their own settings); ACRYLIC and MICA
--                       give BLUR
--   no compositor       nothing: the window stays opaque
newtype RBackdrop = RBackdrop CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | Opaque: the window color, or the wallpaper of the theme.
pattern R_BACKDROP_NONE :: RBackdrop
pattern R_BACKDROP_NONE = RBackdrop 1024
-- | The screen behind the window, as it is.
pattern R_BACKDROP_TRANSPARENT :: RBackdrop
pattern R_BACKDROP_TRANSPARENT = RBackdrop 1025
-- | The screen behind the window, blurred by the OS.
pattern R_BACKDROP_BLUR :: RBackdrop
pattern R_BACKDROP_BLUR = RBackdrop 1026
-- | Blurred, tinted and grained by the OS (the acrylic of Windows).
pattern R_BACKDROP_ACRYLIC :: RBackdrop
pattern R_BACKDROP_ACRYLIC = RBackdrop 1027
-- | A soft tint taken from the wallpaper of the desktop (the mica of Windows 11).
pattern R_BACKDROP_MICA :: RBackdrop
pattern R_BACKDROP_MICA = RBackdrop 1028

newtype RWindowSystem = RWindowSystem CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_WINDOW_SYSTEM_NONE :: RWindowSystem
pattern R_WINDOW_SYSTEM_NONE = RWindowSystem 144
pattern R_WINDOW_SYSTEM_WIN32 :: RWindowSystem
pattern R_WINDOW_SYSTEM_WIN32 = RWindowSystem 145
pattern R_WINDOW_SYSTEM_X11 :: RWindowSystem
pattern R_WINDOW_SYSTEM_X11 = RWindowSystem 146
pattern R_WINDOW_SYSTEM_WAYLAND :: RWindowSystem
pattern R_WINDOW_SYSTEM_WAYLAND = RWindowSystem 147
pattern R_WINDOW_SYSTEM_COCOA :: RWindowSystem
pattern R_WINDOW_SYSTEM_COCOA = RWindowSystem 148
pattern R_WINDOW_SYSTEM_EMSCRIPTEN :: RWindowSystem
pattern R_WINDOW_SYSTEM_EMSCRIPTEN = RWindowSystem 149

-- | Edges and corners of a window, for Rwindow_begin_resize.
newtype RWindowEdge = RWindowEdge CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_EDGE_LEFT :: RWindowEdge
pattern R_EDGE_LEFT = RWindowEdge 208
pattern R_EDGE_RIGHT :: RWindowEdge
pattern R_EDGE_RIGHT = RWindowEdge 209
pattern R_EDGE_TOP :: RWindowEdge
pattern R_EDGE_TOP = RWindowEdge 210
pattern R_EDGE_BOTTOM :: RWindowEdge
pattern R_EDGE_BOTTOM = RWindowEdge 211
pattern R_EDGE_TOP_LEFT :: RWindowEdge
pattern R_EDGE_TOP_LEFT = RWindowEdge 212
pattern R_EDGE_TOP_RIGHT :: RWindowEdge
pattern R_EDGE_TOP_RIGHT = RWindowEdge 213
pattern R_EDGE_BOTTOM_LEFT :: RWindowEdge
pattern R_EDGE_BOTTOM_LEFT = RWindowEdge 214
pattern R_EDGE_BOTTOM_RIGHT :: RWindowEdge
pattern R_EDGE_BOTTOM_RIGHT = RWindowEdge 215

newtype RMouseButton = RMouseButton CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_MOUSE_LEFT :: RMouseButton
pattern R_MOUSE_LEFT = RMouseButton 0
pattern R_MOUSE_RIGHT :: RMouseButton
pattern R_MOUSE_RIGHT = RMouseButton 1
pattern R_MOUSE_MIDDLE :: RMouseButton
pattern R_MOUSE_MIDDLE = RMouseButton 2
pattern R_MOUSE_X1 :: RMouseButton
pattern R_MOUSE_X1 = RMouseButton 3
pattern R_MOUSE_X2 :: RMouseButton
pattern R_MOUSE_X2 = RMouseButton 4

newtype RModifier = RModifier CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_MOD_SHIFT :: RModifier
pattern R_MOD_SHIFT = RModifier 1
pattern R_MOD_CTRL :: RModifier
pattern R_MOD_CTRL = RModifier 2
pattern R_MOD_ALT :: RModifier
pattern R_MOD_ALT = RModifier 4
pattern R_MOD_SUPER :: RModifier
pattern R_MOD_SUPER = RModifier 8

newtype RCursor = RCursor CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_CURSOR_ARROW :: RCursor
pattern R_CURSOR_ARROW = RCursor 176
pattern R_CURSOR_IBEAM :: RCursor
pattern R_CURSOR_IBEAM = RCursor 177
pattern R_CURSOR_HAND :: RCursor
pattern R_CURSOR_HAND = RCursor 178
pattern R_CURSOR_CROSSHAIR :: RCursor
pattern R_CURSOR_CROSSHAIR = RCursor 179
pattern R_CURSOR_RESIZE_H :: RCursor
pattern R_CURSOR_RESIZE_H = RCursor 180
pattern R_CURSOR_RESIZE_V :: RCursor
pattern R_CURSOR_RESIZE_V = RCursor 181
pattern R_CURSOR_RESIZE_NWSE :: RCursor
pattern R_CURSOR_RESIZE_NWSE = RCursor 182
pattern R_CURSOR_RESIZE_NESW :: RCursor
pattern R_CURSOR_RESIZE_NESW = RCursor 183
pattern R_CURSOR_MOVE :: RCursor
pattern R_CURSOR_MOVE = RCursor 184
pattern R_CURSOR_NOT_ALLOWED :: RCursor
pattern R_CURSOR_NOT_ALLOWED = RCursor 185
-- | No pointer at all (a game that hides it). Windows; elsewhere the arrow.
pattern R_CURSOR_NONE :: RCursor
pattern R_CURSOR_NONE = RCursor 186

-- | Keyboard keys. Printable keys use their upper-case ASCII code.
newtype RKey = RKey CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_KEY_UNKNOWN :: RKey
pattern R_KEY_UNKNOWN = RKey 0
pattern R_KEY_SPACE :: RKey
pattern R_KEY_SPACE = RKey 32
pattern R_KEY_APOSTROPHE :: RKey
pattern R_KEY_APOSTROPHE = RKey 39
pattern R_KEY_COMMA :: RKey
pattern R_KEY_COMMA = RKey 44
pattern R_KEY_MINUS :: RKey
pattern R_KEY_MINUS = RKey 45
pattern R_KEY_PERIOD :: RKey
pattern R_KEY_PERIOD = RKey 46
pattern R_KEY_SLASH :: RKey
pattern R_KEY_SLASH = RKey 47
pattern R_KEY_0 :: RKey
pattern R_KEY_0 = RKey 48
pattern R_KEY_1 :: RKey
pattern R_KEY_1 = RKey 49
pattern R_KEY_2 :: RKey
pattern R_KEY_2 = RKey 50
pattern R_KEY_3 :: RKey
pattern R_KEY_3 = RKey 51
pattern R_KEY_4 :: RKey
pattern R_KEY_4 = RKey 52
pattern R_KEY_5 :: RKey
pattern R_KEY_5 = RKey 53
pattern R_KEY_6 :: RKey
pattern R_KEY_6 = RKey 54
pattern R_KEY_7 :: RKey
pattern R_KEY_7 = RKey 55
pattern R_KEY_8 :: RKey
pattern R_KEY_8 = RKey 56
pattern R_KEY_9 :: RKey
pattern R_KEY_9 = RKey 57
pattern R_KEY_SEMICOLON :: RKey
pattern R_KEY_SEMICOLON = RKey 59
pattern R_KEY_EQUAL :: RKey
pattern R_KEY_EQUAL = RKey 61
pattern R_KEY_A :: RKey
pattern R_KEY_A = RKey 65
pattern R_KEY_B :: RKey
pattern R_KEY_B = RKey 66
pattern R_KEY_C :: RKey
pattern R_KEY_C = RKey 67
pattern R_KEY_D :: RKey
pattern R_KEY_D = RKey 68
pattern R_KEY_E :: RKey
pattern R_KEY_E = RKey 69
pattern R_KEY_F :: RKey
pattern R_KEY_F = RKey 70
pattern R_KEY_G :: RKey
pattern R_KEY_G = RKey 71
pattern R_KEY_H :: RKey
pattern R_KEY_H = RKey 72
pattern R_KEY_I :: RKey
pattern R_KEY_I = RKey 73
pattern R_KEY_J :: RKey
pattern R_KEY_J = RKey 74
pattern R_KEY_K :: RKey
pattern R_KEY_K = RKey 75
pattern R_KEY_L :: RKey
pattern R_KEY_L = RKey 76
pattern R_KEY_M :: RKey
pattern R_KEY_M = RKey 77
pattern R_KEY_N :: RKey
pattern R_KEY_N = RKey 78
pattern R_KEY_O :: RKey
pattern R_KEY_O = RKey 79
pattern R_KEY_P :: RKey
pattern R_KEY_P = RKey 80
pattern R_KEY_Q :: RKey
pattern R_KEY_Q = RKey 81
pattern R_KEY_R :: RKey
pattern R_KEY_R = RKey 82
pattern R_KEY_S :: RKey
pattern R_KEY_S = RKey 83
pattern R_KEY_T :: RKey
pattern R_KEY_T = RKey 84
pattern R_KEY_U :: RKey
pattern R_KEY_U = RKey 85
pattern R_KEY_V :: RKey
pattern R_KEY_V = RKey 86
pattern R_KEY_W :: RKey
pattern R_KEY_W = RKey 87
pattern R_KEY_X :: RKey
pattern R_KEY_X = RKey 88
pattern R_KEY_Y :: RKey
pattern R_KEY_Y = RKey 89
pattern R_KEY_Z :: RKey
pattern R_KEY_Z = RKey 90
pattern R_KEY_LEFT_BRACKET :: RKey
pattern R_KEY_LEFT_BRACKET = RKey 91
pattern R_KEY_BACKSLASH :: RKey
pattern R_KEY_BACKSLASH = RKey 92
pattern R_KEY_RIGHT_BRACKET :: RKey
pattern R_KEY_RIGHT_BRACKET = RKey 93
pattern R_KEY_GRAVE :: RKey
pattern R_KEY_GRAVE = RKey 96
pattern R_KEY_ESCAPE :: RKey
pattern R_KEY_ESCAPE = RKey 256
pattern R_KEY_ENTER :: RKey
pattern R_KEY_ENTER = RKey 257
pattern R_KEY_TAB :: RKey
pattern R_KEY_TAB = RKey 258
pattern R_KEY_BACKSPACE :: RKey
pattern R_KEY_BACKSPACE = RKey 259
pattern R_KEY_INSERT :: RKey
pattern R_KEY_INSERT = RKey 260
pattern R_KEY_DELETE :: RKey
pattern R_KEY_DELETE = RKey 261
pattern R_KEY_RIGHT :: RKey
pattern R_KEY_RIGHT = RKey 262
pattern R_KEY_LEFT :: RKey
pattern R_KEY_LEFT = RKey 263
pattern R_KEY_DOWN :: RKey
pattern R_KEY_DOWN = RKey 264
pattern R_KEY_UP :: RKey
pattern R_KEY_UP = RKey 265
pattern R_KEY_PAGE_UP :: RKey
pattern R_KEY_PAGE_UP = RKey 266
pattern R_KEY_PAGE_DOWN :: RKey
pattern R_KEY_PAGE_DOWN = RKey 267
pattern R_KEY_HOME :: RKey
pattern R_KEY_HOME = RKey 268
pattern R_KEY_END :: RKey
pattern R_KEY_END = RKey 269
pattern R_KEY_CAPS_LOCK :: RKey
pattern R_KEY_CAPS_LOCK = RKey 270
pattern R_KEY_PRINT_SCREEN :: RKey
pattern R_KEY_PRINT_SCREEN = RKey 271
pattern R_KEY_PAUSE :: RKey
pattern R_KEY_PAUSE = RKey 272
pattern R_KEY_F1 :: RKey
pattern R_KEY_F1 = RKey 273
pattern R_KEY_F2 :: RKey
pattern R_KEY_F2 = RKey 274
pattern R_KEY_F3 :: RKey
pattern R_KEY_F3 = RKey 275
pattern R_KEY_F4 :: RKey
pattern R_KEY_F4 = RKey 276
pattern R_KEY_F5 :: RKey
pattern R_KEY_F5 = RKey 277
pattern R_KEY_F6 :: RKey
pattern R_KEY_F6 = RKey 278
pattern R_KEY_F7 :: RKey
pattern R_KEY_F7 = RKey 279
pattern R_KEY_F8 :: RKey
pattern R_KEY_F8 = RKey 280
pattern R_KEY_F9 :: RKey
pattern R_KEY_F9 = RKey 281
pattern R_KEY_F10 :: RKey
pattern R_KEY_F10 = RKey 282
pattern R_KEY_F11 :: RKey
pattern R_KEY_F11 = RKey 283
pattern R_KEY_F12 :: RKey
pattern R_KEY_F12 = RKey 284
pattern R_KEY_KP_0 :: RKey
pattern R_KEY_KP_0 = RKey 285
pattern R_KEY_KP_1 :: RKey
pattern R_KEY_KP_1 = RKey 286
pattern R_KEY_KP_2 :: RKey
pattern R_KEY_KP_2 = RKey 287
pattern R_KEY_KP_3 :: RKey
pattern R_KEY_KP_3 = RKey 288
pattern R_KEY_KP_4 :: RKey
pattern R_KEY_KP_4 = RKey 289
pattern R_KEY_KP_5 :: RKey
pattern R_KEY_KP_5 = RKey 290
pattern R_KEY_KP_6 :: RKey
pattern R_KEY_KP_6 = RKey 291
pattern R_KEY_KP_7 :: RKey
pattern R_KEY_KP_7 = RKey 292
pattern R_KEY_KP_8 :: RKey
pattern R_KEY_KP_8 = RKey 293
pattern R_KEY_KP_9 :: RKey
pattern R_KEY_KP_9 = RKey 294
pattern R_KEY_KP_DECIMAL :: RKey
pattern R_KEY_KP_DECIMAL = RKey 295
pattern R_KEY_KP_DIVIDE :: RKey
pattern R_KEY_KP_DIVIDE = RKey 296
pattern R_KEY_KP_MULTIPLY :: RKey
pattern R_KEY_KP_MULTIPLY = RKey 297
pattern R_KEY_KP_SUBTRACT :: RKey
pattern R_KEY_KP_SUBTRACT = RKey 298
pattern R_KEY_KP_ADD :: RKey
pattern R_KEY_KP_ADD = RKey 299
pattern R_KEY_KP_ENTER :: RKey
pattern R_KEY_KP_ENTER = RKey 300
pattern R_KEY_LEFT_SHIFT :: RKey
pattern R_KEY_LEFT_SHIFT = RKey 301
pattern R_KEY_LEFT_CTRL :: RKey
pattern R_KEY_LEFT_CTRL = RKey 302
pattern R_KEY_LEFT_ALT :: RKey
pattern R_KEY_LEFT_ALT = RKey 303
pattern R_KEY_LEFT_SUPER :: RKey
pattern R_KEY_LEFT_SUPER = RKey 304
pattern R_KEY_RIGHT_SHIFT :: RKey
pattern R_KEY_RIGHT_SHIFT = RKey 305
pattern R_KEY_RIGHT_CTRL :: RKey
pattern R_KEY_RIGHT_CTRL = RKey 306
pattern R_KEY_RIGHT_ALT :: RKey
pattern R_KEY_RIGHT_ALT = RKey 307
pattern R_KEY_RIGHT_SUPER :: RKey
pattern R_KEY_RIGHT_SUPER = RKey 308
pattern R_KEY_MENU :: RKey
pattern R_KEY_MENU = RKey 309
pattern R_KEY_COUNT :: RKey
pattern R_KEY_COUNT = RKey 310

newtype REventType = REventType CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_EVENT_NONE :: REventType
pattern R_EVENT_NONE = REventType 192
pattern R_EVENT_MOUSE_MOVE :: REventType
pattern R_EVENT_MOUSE_MOVE = REventType 193
pattern R_EVENT_MOUSE_DOWN :: REventType
pattern R_EVENT_MOUSE_DOWN = REventType 194
pattern R_EVENT_MOUSE_UP :: REventType
pattern R_EVENT_MOUSE_UP = REventType 195
pattern R_EVENT_MOUSE_WHEEL :: REventType
pattern R_EVENT_MOUSE_WHEEL = REventType 196
pattern R_EVENT_MOUSE_ENTER :: REventType
pattern R_EVENT_MOUSE_ENTER = REventType 197
pattern R_EVENT_MOUSE_LEAVE :: REventType
pattern R_EVENT_MOUSE_LEAVE = REventType 198
pattern R_EVENT_KEY_DOWN :: REventType
pattern R_EVENT_KEY_DOWN = REventType 199
pattern R_EVENT_KEY_UP :: REventType
pattern R_EVENT_KEY_UP = REventType 200
-- | A character was typed. `codepoint` holds the Unicode scalar value.
pattern R_EVENT_TEXT :: REventType
pattern R_EVENT_TEXT = REventType 201
pattern R_EVENT_FOCUS_GAINED :: REventType
pattern R_EVENT_FOCUS_GAINED = REventType 202
pattern R_EVENT_FOCUS_LOST :: REventType
pattern R_EVENT_FOCUS_LOST = REventType 203

-- | Moments of a drag coming from another application (files from the file manager, text...).
newtype RDropPhase = RDropPhase CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | The pointer came over the target.
pattern R_DROP_ENTER :: RDropPhase
pattern R_DROP_ENTER = RDropPhase 224
-- | It moves over the target.
pattern R_DROP_MOVE :: RDropPhase
pattern R_DROP_MOVE = RDropPhase 225
-- | It left the target without dropping.
pattern R_DROP_LEAVE :: RDropPhase
pattern R_DROP_LEAVE = RDropPhase 226
-- | The user dropped: `paths` and `text` hold what was dropped.
pattern R_DROP_DROPPED :: RDropPhase
pattern R_DROP_DROPPED = RDropPhase 227

-- | What a finger does on a widget (Rwidget_set_touch_mode).
newtype RTouchMode = RTouchMode CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | The finger scrolls what scrolls under it, and is a mouse elsewhere (default).
pattern R_TOUCH_MODE_AUTO :: RTouchMode
pattern R_TOUCH_MODE_AUTO = RTouchMode 1152
-- | A finger moving on the widget always scrolls what contains it, even over a control that drags.
pattern R_TOUCH_MODE_SCROLL :: RTouchMode
pattern R_TOUCH_MODE_SCROLL = RTouchMode 1153
-- | The finger is a mouse on the widget: pressing presses at once, moving drags, nothing scrolls.
pattern R_TOUCH_MODE_MOUSE :: RTouchMode
pattern R_TOUCH_MODE_MOUSE = RTouchMode 1154
-- | Fingers are ignored on the widget (only its touch callback sees them): a surface for the pen.
pattern R_TOUCH_MODE_NONE :: RTouchMode
pattern R_TOUCH_MODE_NONE = RTouchMode 1155

newtype RTouchPhase = RTouchPhase CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | A finger came down.
pattern R_TOUCH_DOWN :: RTouchPhase
pattern R_TOUCH_DOWN = RTouchPhase 1168
pattern R_TOUCH_MOVE :: RTouchPhase
pattern R_TOUCH_MOVE = RTouchPhase 1169
-- | The finger was lifted.
pattern R_TOUCH_UP :: RTouchPhase
pattern R_TOUCH_UP = RTouchPhase 1170
-- | The system took the finger away (a gesture of its own, the window lost it): nothing must happen as for a lift.
pattern R_TOUCH_CANCEL :: RTouchPhase
pattern R_TOUCH_CANCEL = RTouchPhase 1171

newtype RPenPhase = RPenPhase CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | The tip touched the screen.
pattern R_PEN_DOWN :: RPenPhase
pattern R_PEN_DOWN = RPenPhase 1184
-- | The pen moves while touching.
pattern R_PEN_MOVE :: RPenPhase
pattern R_PEN_MOVE = RPenPhase 1185
-- | The tip left the screen.
pattern R_PEN_UP :: RPenPhase
pattern R_PEN_UP = RPenPhase 1186
-- | The pen moves near the screen without touching it.
pattern R_PEN_HOVER :: RPenPhase
pattern R_PEN_HOVER = RPenPhase 1187
-- | The pen went out of reach.
pattern R_PEN_LEAVE :: RPenPhase
pattern R_PEN_LEAVE = RPenPhase 1188

-- | Buttons on the side of a pen (RPenEvent::buttons).
newtype RPenButton = RPenButton CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_PEN_BUTTON_BARREL :: RPenButton
pattern R_PEN_BUTTON_BARREL = RPenButton 1
pattern R_PEN_BUTTON_BARREL2 :: RPenButton
pattern R_PEN_BUTTON_BARREL2 = RPenButton 2

newtype RGesturePhase = RGesturePhase CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | A second finger came down (a touch screen), or the system recognized a gesture (a trackpad).
pattern R_GESTURE_BEGIN :: RGesturePhase
pattern R_GESTURE_BEGIN = RGesturePhase 1200
pattern R_GESTURE_UPDATE :: RGesturePhase
pattern R_GESTURE_UPDATE = RGesturePhase 1201
-- | Fewer than two fingers remain, or the fingers left the trackpad (RGestureEvent::cancelled: the system took the gesture back).
pattern R_GESTURE_END :: RGesturePhase
pattern R_GESTURE_END = RGesturePhase 1202

-- | What the fingers of a gesture do (RGestureEvent::kind).
newtype RGestureKind = RGestureKind CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | Two fingers that pinch, turn and move together: scale, rotation and pan.
pattern R_GESTURE_PINCH :: RGestureKind
pattern R_GESTURE_PINCH = RGestureKind 1216
-- | Three or more fingers that move together on a trackpad: only the pan
-- (scale stays 1). By default, a swipe across goes back or forward.
pattern R_GESTURE_SWIPE :: RGestureKind
pattern R_GESTURE_SWIPE = RGestureKind 1217

-- | Where the fingers of a gesture are (RGestureEvent::device).
newtype RGestureDevice = RGestureDevice CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | On the screen: the library recognized the gesture.
pattern R_GESTURE_DEVICE_TOUCHSCREEN :: RGestureDevice
pattern R_GESTURE_DEVICE_TOUCHSCREEN = RGestureDevice 1232
-- | On a trackpad: the system recognized the gesture; the fingers are not on the window (x, y is the pointer).
pattern R_GESTURE_DEVICE_TRACKPAD :: RGestureDevice
pattern R_GESTURE_DEVICE_TRACKPAD = RGestureDevice 1233

-- | The looks. Each one comes dark and light.
--
-- Raven's looks are the default; the ones of the platforms imitate them
-- closely (their shapes, colors, sizes, states and motion), without using
-- their widgets. The looks of the platforms are opaque, except macOS Glass
-- (macOS 26), whose bars, menus and floating surfaces are glass as on the
-- system; the glass materials themselves (Rtheme_set_material) belong to
-- the glass looks of Raven.
newtype RLook = RLook CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | Raven, flat and dense, for tools. The look at start.
pattern R_LOOK_RAVEN_DARK :: RLook
pattern R_LOOK_RAVEN_DARK = RLook 224
pattern R_LOOK_RAVEN_LIGHT :: RLook
pattern R_LOOK_RAVEN_LIGHT = RLook 225
-- | Raven in glass: controls are capsules of translucent glass that refract
-- and blur what is behind them, catch the light on their rim and react
-- to the pointer with short animations. Windows get a soft colored
-- background unless they have their own (Rwindow_set_background,
-- Rwindow_set_clear_color).
pattern R_LOOK_RAVEN_GLASS_DARK :: RLook
pattern R_LOOK_RAVEN_GLASS_DARK = RLook 226
pattern R_LOOK_RAVEN_GLASS_LIGHT :: RLook
pattern R_LOOK_RAVEN_GLASS_LIGHT = RLook 227
-- | High contrast, for low vision: pure black and white, yellow (or blue)
-- focus and selection, thick borders, larger text (raven_accessibility.h,
-- Raccessibility_set_high_contrast).
pattern R_LOOK_RAVEN_HIGH_CONTRAST_DARK :: RLook
pattern R_LOOK_RAVEN_HIGH_CONTRAST_DARK = RLook 228
pattern R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT :: RLook
pattern R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT = RLook 229
-- | Windows 11 (Fluent): its controls, its colors, the accent color the
-- user chose (on Windows), the Segoe UI font when the system has it, and
-- motion without bounce.
pattern R_LOOK_WINDOWS_DARK :: RLook
pattern R_LOOK_WINDOWS_DARK = RLook 230
pattern R_LOOK_WINDOWS_LIGHT :: RLook
pattern R_LOOK_WINDOWS_LIGHT = RLook 231
-- | macOS (Sequoia, Aqua): its controls, its colors, the accent color the
-- user chose (on macOS), the San Francisco font when the system has it,
-- no change under the pointer, and selections in the accent color.
pattern R_LOOK_MACOS_DARK :: RLook
pattern R_LOOK_MACOS_DARK = RLook 232
pattern R_LOOK_MACOS_LIGHT :: RLook
pattern R_LOOK_MACOS_LIGHT = RLook 233
-- | GNOME (libadwaita), the desktop of most Linux distributions: flat
-- rounded controls, the accent and the font of the desktop (on Linux).
pattern R_LOOK_GNOME_DARK :: RLook
pattern R_LOOK_GNOME_DARK = RLook 234
pattern R_LOOK_GNOME_LIGHT :: RLook
pattern R_LOOK_GNOME_LIGHT = RLook 235
-- | The look of the platform the application runs on: Windows on Windows,
-- macOS on macOS (macOS Glass from macOS 26 on), GNOME on Linux and the
-- other systems.
pattern R_LOOK_NATIVE_DARK :: RLook
pattern R_LOOK_NATIVE_DARK = RLook 236
pattern R_LOOK_NATIVE_LIGHT :: RLook
pattern R_LOOK_NATIVE_LIGHT = RLook 237
-- | macOS 26 (Tahoe, Liquid Glass): the controls of macOS, larger and
-- rounder, and glass where the system puts it, on what floats above the
-- content: header bars and their buttons, menus, drop-down lists,
-- tooltips, sheets; the knobs of sliders and switches turn into a lens
-- of clear glass while they are held. The content itself (fields, lists,
-- text) stays opaque. The accent color and the font of the system, on
-- macOS.
pattern R_LOOK_MACOS_GLASS_DARK :: RLook
pattern R_LOOK_MACOS_GLASS_DARK = RLook 238
pattern R_LOOK_MACOS_GLASS_LIGHT :: RLook
pattern R_LOOK_MACOS_GLASS_LIGHT = RLook 239

-- | @deprecated The built-in themes of Raven 0.2, now looks: R_THEME_DARK is
-- R_LOOK_RAVEN_DARK and so on, and Rtheme_apply is Rlook_set.
newtype RTheme = RTheme CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | Flat, dense themes for tools.
pattern R_THEME_DARK :: RTheme
pattern R_THEME_DARK = RTheme 224
pattern R_THEME_LIGHT :: RTheme
pattern R_THEME_LIGHT = RTheme 225
-- | Glass themes: controls are capsules of translucent glass that refract
-- and blur what is behind them, catch the light on their rim and react
-- to the pointer with short animations. Windows get a soft colored
-- background unless they have their own (Rwindow_set_background,
-- Rwindow_set_clear_color).
pattern R_THEME_GLASS_DARK :: RTheme
pattern R_THEME_GLASS_DARK = RTheme 226
pattern R_THEME_GLASS_LIGHT :: RTheme
pattern R_THEME_GLASS_LIGHT = RTheme 227
-- | High contrast, for low vision: pure black and white, yellow (or blue)
-- focus and selection, thick borders, larger text (raven_accessibility.h,
-- Raccessibility_set_high_contrast).
pattern R_THEME_HIGH_CONTRAST_DARK :: RTheme
pattern R_THEME_HIGH_CONTRAST_DARK = RTheme 228
pattern R_THEME_HIGH_CONTRAST_LIGHT :: RTheme
pattern R_THEME_HIGH_CONTRAST_LIGHT = RTheme 229

-- | What surfaces are made of, from the clearest to the most opaque. A theme
-- has one material for everything (Rtheme_set_material); any widget can be
-- given its own (Rwidget_set_material, Material="..." in XML).
--
-- The three glasses share the same anatomy (a tint over what is behind, a
-- rim that bends it and catches the light, a shadow) and differ by how much
-- they let through. Within one material, surfaces are also told apart by
-- their height above the window: bars lie flat, panels float a little,
-- controls a little more, menus and dialogs highest; each level has a
-- brighter rim and a deeper shadow than the one below.
newtype RMaterial = RMaterial CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | Opaque: filled with the colors of the theme. The material of R_THEME_DARK and R_THEME_LIGHT.
pattern R_MATERIAL_SOLID :: RMaterial
pattern R_MATERIAL_SOLID = RMaterial 736
-- | Clear glass: what is behind stays sharp enough to be recognized, strongly bent at the rim.
pattern R_MATERIAL_GLASS :: RMaterial
pattern R_MATERIAL_GLASS = RMaterial 737
-- | Frosted glass: what is behind is blurred into soft colors. The material of the glass themes.
pattern R_MATERIAL_FROSTED_GLASS :: RMaterial
pattern R_MATERIAL_FROSTED_GLASS = RMaterial 738
-- | Frosted glass colored by the accent color of the theme (or of the widget): for what must stand out.
pattern R_MATERIAL_TINTED_GLASS :: RMaterial
pattern R_MATERIAL_TINTED_GLASS = RMaterial 739
-- | Not a material: "the one of the theme". Removes the material given to a widget.
pattern R_MATERIAL_DEFAULT :: RMaterial
pattern R_MATERIAL_DEFAULT = RMaterial 740

-- | Color roles. The comment lists the widgets reading each role.
newtype RColorRole = RColorRole CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | Window clear color.
pattern R_COLOR_WINDOW :: RColorRole
pattern R_COLOR_WINDOW = RColorRole 256
-- | Border background.
pattern R_COLOR_PANEL :: RColorRole
pattern R_COLOR_PANEL = RColorRole 257
-- | Border outline, list and input outlines.
pattern R_COLOR_BORDER :: RColorRole
pattern R_COLOR_BORDER = RColorRole 258
-- | Text. Inherited by the children of the widget it is set on.
pattern R_COLOR_TEXT :: RColorRole
pattern R_COLOR_TEXT = RColorRole 259
pattern R_COLOR_TEXT_DISABLED :: RColorRole
pattern R_COLOR_TEXT_DISABLED = RColorRole 260
pattern R_COLOR_TEXT_PLACEHOLDER :: RColorRole
pattern R_COLOR_TEXT_PLACEHOLDER = RColorRole 261
-- | Primary buttons, checked boxes, slider fill, progress fill.
pattern R_COLOR_ACCENT :: RColorRole
pattern R_COLOR_ACCENT = RColorRole 262
pattern R_COLOR_ACCENT_HOVERED :: RColorRole
pattern R_COLOR_ACCENT_HOVERED = RColorRole 263
pattern R_COLOR_ACCENT_PRESSED :: RColorRole
pattern R_COLOR_ACCENT_PRESSED = RColorRole 264
-- | Text and marks drawn on top of the accent color.
pattern R_COLOR_ACCENT_TEXT :: RColorRole
pattern R_COLOR_ACCENT_TEXT = RColorRole 265
-- | Button, combo box, check box background.
pattern R_COLOR_CONTROL :: RColorRole
pattern R_COLOR_CONTROL = RColorRole 266
pattern R_COLOR_CONTROL_HOVERED :: RColorRole
pattern R_COLOR_CONTROL_HOVERED = RColorRole 267
pattern R_COLOR_CONTROL_PRESSED :: RColorRole
pattern R_COLOR_CONTROL_PRESSED = RColorRole 268
pattern R_COLOR_CONTROL_BORDER :: RColorRole
pattern R_COLOR_CONTROL_BORDER = RColorRole 269
-- | Text input, spin box, list and tree background.
pattern R_COLOR_INPUT :: RColorRole
pattern R_COLOR_INPUT = RColorRole 270
-- | Keyboard focus outline.
pattern R_COLOR_FOCUS :: RColorRole
pattern R_COLOR_FOCUS = RColorRole 271
-- | Selected rows and selected text.
pattern R_COLOR_SELECTION :: RColorRole
pattern R_COLOR_SELECTION = RColorRole 272
pattern R_COLOR_ROW_HOVERED :: RColorRole
pattern R_COLOR_ROW_HOVERED = RColorRole 273
-- | Slider track, progress background, scrollbar track.
pattern R_COLOR_TRACK :: RColorRole
pattern R_COLOR_TRACK = RColorRole 274
-- | Scrollbar thumb, slider handle.
pattern R_COLOR_HANDLE :: RColorRole
pattern R_COLOR_HANDLE = RColorRole 275
pattern R_COLOR_HANDLE_HOVERED :: RColorRole
pattern R_COLOR_HANDLE_HOVERED = RColorRole 276
pattern R_COLOR_SEPARATOR :: RColorRole
pattern R_COLOR_SEPARATOR = RColorRole 277
-- | Menu bar, tab bar, expander header.
pattern R_COLOR_HEADER :: RColorRole
pattern R_COLOR_HEADER = RColorRole 278
pattern R_COLOR_HEADER_HOVERED :: RColorRole
pattern R_COLOR_HEADER_HOVERED = RColorRole 279
-- | Active tab.
pattern R_COLOR_TAB_ACTIVE :: RColorRole
pattern R_COLOR_TAB_ACTIVE = RColorRole 280
-- | Menus, combo box lists.
pattern R_COLOR_POPUP :: RColorRole
pattern R_COLOR_POPUP = RColorRole 281
pattern R_COLOR_POPUP_BORDER :: RColorRole
pattern R_COLOR_POPUP_BORDER = RColorRole 282
pattern R_COLOR_SHADOW :: RColorRole
pattern R_COLOR_SHADOW = RColorRole 283
pattern R_COLOR_TOOLTIP :: RColorRole
pattern R_COLOR_TOOLTIP = RColorRole 284
pattern R_COLOR_TOOLTIP_TEXT :: RColorRole
pattern R_COLOR_TOOLTIP_TEXT = RColorRole 285
-- | Track of a switch that is on.
pattern R_COLOR_SWITCH :: RColorRole
pattern R_COLOR_SWITCH = RColorRole 286
-- | What went well, what needs attention, what is wrong or destroys something (markers, badges, classes).
pattern R_COLOR_SUCCESS :: RColorRole
pattern R_COLOR_SUCCESS = RColorRole 287
pattern R_COLOR_WARNING :: RColorRole
pattern R_COLOR_WARNING = RColorRole 288
pattern R_COLOR_DANGER :: RColorRole
pattern R_COLOR_DANGER = RColorRole 289
-- | Links of formatted text.
pattern R_COLOR_LINK :: RColorRole
pattern R_COLOR_LINK = RColorRole 290
-- | Background of code in formatted text.
pattern R_COLOR_CODE :: RColorRole
pattern R_COLOR_CODE = RColorRole 291

-- | Sizes, in logical units.
newtype RMetric = RMetric CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_METRIC_FONT_SIZE :: RMetric
pattern R_METRIC_FONT_SIZE = RMetric 320
pattern R_METRIC_CORNER_RADIUS :: RMetric
pattern R_METRIC_CORNER_RADIUS = RMetric 321
pattern R_METRIC_BORDER_WIDTH :: RMetric
pattern R_METRIC_BORDER_WIDTH = RMetric 322
-- | Height of buttons, inputs, combo boxes, tabs, menu bars.
pattern R_METRIC_CONTROL_HEIGHT :: RMetric
pattern R_METRIC_CONTROL_HEIGHT = RMetric 323
-- | Horizontal / vertical padding inside controls.
pattern R_METRIC_PADDING_X :: RMetric
pattern R_METRIC_PADDING_X = RMetric 324
pattern R_METRIC_PADDING_Y :: RMetric
pattern R_METRIC_PADDING_Y = RMetric 325
-- | Gap between an icon or a check mark and its text.
pattern R_METRIC_SPACING :: RMetric
pattern R_METRIC_SPACING = RMetric 326
pattern R_METRIC_SCROLLBAR_SIZE :: RMetric
pattern R_METRIC_SCROLLBAR_SIZE = RMetric 327
-- | Height of list, tree and menu rows.
pattern R_METRIC_ROW_HEIGHT :: RMetric
pattern R_METRIC_ROW_HEIGHT = RMetric 328
-- | Indentation per tree level.
pattern R_METRIC_INDENT :: RMetric
pattern R_METRIC_INDENT = RMetric 329
pattern R_METRIC_SPLITTER_SIZE :: RMetric
pattern R_METRIC_SPLITTER_SIZE = RMetric 330
pattern R_METRIC_CHECK_SIZE :: RMetric
pattern R_METRIC_CHECK_SIZE = RMetric 331
pattern R_METRIC_SLIDER_HANDLE_SIZE :: RMetric
pattern R_METRIC_SLIDER_HANDLE_SIZE = RMetric 332
-- | Corner radius of menus, drop-down lists and tooltips.
pattern R_METRIC_POPUP_RADIUS :: RMetric
pattern R_METRIC_POPUP_RADIUS = RMetric 333
-- | The glass of the theme: each one multiplies what the material gives,
-- 1 = unchanged. Frost (how much is blurred), refraction (how far the rim
-- bends what is behind), saturation (of what is seen through), rim (the
-- light it catches) and shadow (the one it drops).
pattern R_METRIC_GLASS_FROST :: RMetric
pattern R_METRIC_GLASS_FROST = RMetric 334
pattern R_METRIC_GLASS_REFRACTION :: RMetric
pattern R_METRIC_GLASS_REFRACTION = RMetric 335
pattern R_METRIC_GLASS_SATURATION :: RMetric
pattern R_METRIC_GLASS_SATURATION = RMetric 336
pattern R_METRIC_GLASS_RIM :: RMetric
pattern R_METRIC_GLASS_RIM = RMetric 337
pattern R_METRIC_GLASS_SHADOW :: RMetric
pattern R_METRIC_GLASS_SHADOW = RMetric 338
-- | Radius of the blur of a BackgroundBlur that was not given one.
pattern R_METRIC_BLUR_RADIUS :: RMetric
pattern R_METRIC_BLUR_RADIUS = RMetric 339

-- | The styles of a font (Rfont_get_variant).
newtype RFontStyle = RFontStyle CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_FONT_REGULAR :: RFontStyle
pattern R_FONT_REGULAR = RFontStyle 1056
pattern R_FONT_BOLD :: RFontStyle
pattern R_FONT_BOLD = RFontStyle 1057
pattern R_FONT_ITALIC :: RFontStyle
pattern R_FONT_ITALIC = RFontStyle 1058
pattern R_FONT_BOLD_ITALIC :: RFontStyle
pattern R_FONT_BOLD_ITALIC = RFontStyle 1059

newtype RFilterType = RFilterType CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_FILTER_LINEAR :: RFilterType
pattern R_FILTER_LINEAR = RFilterType 704
-- | No smoothing: keeps pixel art sharp.
pattern R_FILTER_NEAREST :: RFilterType
pattern R_FILTER_NEAREST = RFilterType 705
-- | Smooth, with mipmaps: an image shown much smaller than its size stays
-- clean instead of shimmering, for a third more GPU memory. The default
-- of the textures made from image files (Rtexture_create...). Backends
-- without mipmaps treat it as R_FILTER_LINEAR.
pattern R_FILTER_MIPMAP :: RFilterType
pattern R_FILTER_MIPMAP = RFilterType 706

newtype RWidgetType = RWidgetType CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | Stacks children top to bottom. Slate: SVerticalBox.
pattern R_WIDGET_VBOX :: RWidgetType
pattern R_WIDGET_VBOX = RWidgetType 512
-- | Stacks children left to right. Slate: SHorizontalBox.
pattern R_WIDGET_HBOX :: RWidgetType
pattern R_WIDGET_HBOX = RWidgetType 513
-- | Rows and columns with spans and fill coefficients. Slate: SGridPanel.
pattern R_WIDGET_GRID :: RWidgetType
pattern R_WIDGET_GRID = RWidgetType 514
-- | Grid whose cells all have the same size. Slate: SUniformGridPanel.
pattern R_WIDGET_UNIFORM_GRID :: RWidgetType
pattern R_WIDGET_UNIFORM_GRID = RWidgetType 515
-- | Draws children on top of each other. Slate: SOverlay.
pattern R_WIDGET_OVERLAY :: RWidgetType
pattern R_WIDGET_OVERLAY = RWidgetType 516
-- | One child with a background, an outline and inner padding. Slate: SBorder.
pattern R_WIDGET_BORDER :: RWidgetType
pattern R_WIDGET_BORDER = RWidgetType 517
-- | One child with size constraints or a fixed aspect ratio. Slate: SBox.
pattern R_WIDGET_SIZE_BOX :: RWidgetType
pattern R_WIDGET_SIZE_BOX = RWidgetType 518
-- | Empty space. Slate: SSpacer.
pattern R_WIDGET_SPACER :: RWidgetType
pattern R_WIDGET_SPACER = RWidgetType 519
-- | Scrollable stack of children. Slate: SScrollBox.
pattern R_WIDGET_SCROLL_BOX :: RWidgetType
pattern R_WIDGET_SCROLL_BOX = RWidgetType 520
-- | Children separated by draggable handles. Slate: SSplitter.
pattern R_WIDGET_SPLITTER :: RWidgetType
pattern R_WIDGET_SPLITTER = RWidgetType 521
-- | Flows children and wraps to the next line. Slate: SWrapBox.
pattern R_WIDGET_WRAP_BOX :: RWidgetType
pattern R_WIDGET_WRAP_BOX = RWidgetType 522
-- | Children placed with anchors and offsets. Slate: SConstraintCanvas.
pattern R_WIDGET_CANVAS :: RWidgetType
pattern R_WIDGET_CANVAS = RWidgetType 523
-- | Shows one child at a time. Slate: SWidgetSwitcher.
pattern R_WIDGET_SWITCHER :: RWidgetType
pattern R_WIDGET_SWITCHER = RWidgetType 524
-- | Tab bar + one page per child.
pattern R_WIDGET_TAB_VIEW :: RWidgetType
pattern R_WIDGET_TAB_VIEW = RWidgetType 525
-- | Collapsible header + one child. Slate: SExpandableArea.
pattern R_WIDGET_EXPANDER :: RWidgetType
pattern R_WIDGET_EXPANDER = RWidgetType 526
pattern R_WIDGET_LABEL :: RWidgetType
pattern R_WIDGET_LABEL = RWidgetType 527
pattern R_WIDGET_BUTTON :: RWidgetType
pattern R_WIDGET_BUTTON = RWidgetType 528
pattern R_WIDGET_CHECKBOX :: RWidgetType
pattern R_WIDGET_CHECKBOX = RWidgetType 529
pattern R_WIDGET_SLIDER :: RWidgetType
pattern R_WIDGET_SLIDER = RWidgetType 530
pattern R_WIDGET_PROGRESSBAR :: RWidgetType
pattern R_WIDGET_PROGRESSBAR = RWidgetType 531
pattern R_WIDGET_IMAGE :: RWidgetType
pattern R_WIDGET_IMAGE = RWidgetType 532
pattern R_WIDGET_TEXT_INPUT :: RWidgetType
pattern R_WIDGET_TEXT_INPUT = RWidgetType 533
pattern R_WIDGET_SPIN_BOX :: RWidgetType
pattern R_WIDGET_SPIN_BOX = RWidgetType 534
pattern R_WIDGET_COMBO_BOX :: RWidgetType
pattern R_WIDGET_COMBO_BOX = RWidgetType 535
pattern R_WIDGET_SEPARATOR :: RWidgetType
pattern R_WIDGET_SEPARATOR = RWidgetType 536
pattern R_WIDGET_LIST_VIEW :: RWidgetType
pattern R_WIDGET_LIST_VIEW = RWidgetType 537
pattern R_WIDGET_TREE_VIEW :: RWidgetType
pattern R_WIDGET_TREE_VIEW = RWidgetType 538
pattern R_WIDGET_MENU_BAR :: RWidgetType
pattern R_WIDGET_MENU_BAR = RWidgetType 539
pattern R_WIDGET_MENU :: RWidgetType
pattern R_WIDGET_MENU = RWidgetType 540
pattern R_WIDGET_MENU_ITEM :: RWidgetType
pattern R_WIDGET_MENU_ITEM = RWidgetType 541
-- | Area showing content rendered by the application.
pattern R_WIDGET_VIEWPORT :: RWidgetType
pattern R_WIDGET_VIEWPORT = RWidgetType 542
-- | Widget painted and measured by application callbacks.
pattern R_WIDGET_CUSTOM :: RWidgetType
pattern R_WIDGET_CUSTOM = RWidgetType 543
-- | Multi-line text editor.
pattern R_WIDGET_TEXT_EDITOR :: RWidgetType
pattern R_WIDGET_TEXT_EDITOR = RWidgetType 544
-- | Title bar of a window with a custom header, holding any widget.
pattern R_WIDGET_HEADER_BAR :: RWidgetType
pattern R_WIDGET_HEADER_BAR = RWidgetType 545
-- | Panels docked side by side or behind tabs, that the user rearranges.
pattern R_WIDGET_DOCK_AREA :: RWidgetType
pattern R_WIDGET_DOCK_AREA = RWidgetType 546
-- | A capsule that unfolds into a panel, at the top of a window.
pattern R_WIDGET_ISLAND :: RWidgetType
pattern R_WIDGET_ISLAND = RWidgetType 547
-- | Explorer of the files of the computer.
pattern R_WIDGET_FILE_BROWSER :: RWidgetType
pattern R_WIDGET_FILE_BROWSER = RWidgetType 548
-- | One child over a blurred view of what is behind it.
pattern R_WIDGET_BACKGROUND_BLUR :: RWidgetType
pattern R_WIDGET_BACKGROUND_BLUR = RWidgetType 549
-- | Chooses a color: hue, saturation, value, opacity, by hand or by numbers.
pattern R_WIDGET_COLOR_PICKER :: RWidgetType
pattern R_WIDGET_COLOR_PICKER = RWidgetType 550
-- | Tracks of clips and keys along a time ruler.
pattern R_WIDGET_TIMELINE :: RWidgetType
pattern R_WIDGET_TIMELINE = RWidgetType 551
-- | Nodes linked by wires, on a canvas that pans and zooms.
pattern R_WIDGET_NODE_GRAPH :: RWidgetType
pattern R_WIDGET_NODE_GRAPH = RWidgetType 552
-- | Grid of tiles (an icon over a name), virtualized. Slate: STileView.
pattern R_WIDGET_TILE_VIEW :: RWidgetType
pattern R_WIDGET_TILE_VIEW = RWidgetType 553
-- | Blocks that snap into each other, in the style of Scratch.
pattern R_WIDGET_BLOCK_EDITOR :: RWidgetType
pattern R_WIDGET_BLOCK_EDITOR = RWidgetType 554
-- | Rows of cells under columns that can be resized and sorted, virtualized.
pattern R_WIDGET_TABLE :: RWidgetType
pattern R_WIDGET_TABLE = RWidgetType 555
-- | A date, chosen in a calendar that drops down.
pattern R_WIDGET_DATE_PICKER :: RWidgetType
pattern R_WIDGET_DATE_PICKER = RWidgetType 556
-- | A time of the day: hours, minutes, optionally seconds.
pattern R_WIDGET_TIME_PICKER :: RWidgetType
pattern R_WIDGET_TIME_PICKER = RWidgetType 557
-- | A short message that rises for a few seconds, with an optional action ("Undo").
pattern R_WIDGET_TOAST :: RWidgetType
pattern R_WIDGET_TOAST = RWidgetType 558
-- | The path to the current place: links separated by chevrons.
pattern R_WIDGET_BREADCRUMB :: RWidgetType
pattern R_WIDGET_BREADCRUMB = RWidgetType 559
-- | A score shown and chosen with stars.
pattern R_WIDGET_RATING :: RWidgetType
pattern R_WIDGET_RATING = RWidgetType 560
-- | Page numbers with previous and next.
pattern R_WIDGET_PAGINATION :: RWidgetType
pattern R_WIDGET_PAGINATION = RWidgetType 561
-- | Curves of X / Y data: legend, line styles, markers, log axes, a second X and Y axis, pan and zoom.
pattern R_WIDGET_PLOT :: RWidgetType
pattern R_WIDGET_PLOT = RWidgetType 562

newtype RHAlign = RHAlign CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_HALIGN_FILL :: RHAlign
pattern R_HALIGN_FILL = RHAlign 576
pattern R_HALIGN_LEFT :: RHAlign
pattern R_HALIGN_LEFT = RHAlign 577
pattern R_HALIGN_CENTER :: RHAlign
pattern R_HALIGN_CENTER = RHAlign 578
pattern R_HALIGN_RIGHT :: RHAlign
pattern R_HALIGN_RIGHT = RHAlign 579

newtype RVAlign = RVAlign CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_VALIGN_FILL :: RVAlign
pattern R_VALIGN_FILL = RVAlign 592
pattern R_VALIGN_TOP :: RVAlign
pattern R_VALIGN_TOP = RVAlign 593
pattern R_VALIGN_CENTER :: RVAlign
pattern R_VALIGN_CENTER = RVAlign 594
pattern R_VALIGN_BOTTOM :: RVAlign
pattern R_VALIGN_BOTTOM = RVAlign 595

newtype RVisibility = RVisibility CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | Drawn and laid out.
pattern R_VISIBLE :: RVisibility
pattern R_VISIBLE = RVisibility 608
-- | Not drawn, but still takes its space in the layout.
pattern R_HIDDEN :: RVisibility
pattern R_HIDDEN = RVisibility 609
-- | Not drawn and takes no space.
pattern R_COLLAPSED :: RVisibility
pattern R_COLLAPSED = RVisibility 610

-- | How a widget arrives. A transition animates what the widget and its
-- children draw; the layout is not affected, the widget is in place from the
-- first frame for its neighbours and for the mouse.
newtype RTransition = RTransition CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_TRANSITION_NONE :: RTransition
pattern R_TRANSITION_NONE = RTransition 768
-- | Becomes opaque.
pattern R_TRANSITION_FADE :: RTransition
pattern R_TRANSITION_FADE = RTransition 769
-- | Grows from a smaller size and bounces a little past its own.
pattern R_TRANSITION_POP :: RTransition
pattern R_TRANSITION_POP = RTransition 770
-- | Shrinks from a larger size.
pattern R_TRANSITION_ZOOM :: RTransition
pattern R_TRANSITION_ZOOM = RTransition 771
-- | Flies in from below / above / the right / the left, and settles with a bounce.
pattern R_TRANSITION_FLY_UP :: RTransition
pattern R_TRANSITION_FLY_UP = RTransition 772
pattern R_TRANSITION_FLY_DOWN :: RTransition
pattern R_TRANSITION_FLY_DOWN = RTransition 773
pattern R_TRANSITION_FLY_LEFT :: RTransition
pattern R_TRANSITION_FLY_LEFT = RTransition 774
pattern R_TRANSITION_FLY_RIGHT :: RTransition
pattern R_TRANSITION_FLY_RIGHT = RTransition 775
-- | Falls from above, squashes on landing and wobbles.
pattern R_TRANSITION_DROP :: RTransition
pattern R_TRANSITION_DROP = RTransition 776
-- | Short slide from the right / from the left, without bounce: pages that replace one another.
pattern R_TRANSITION_SLIDE_LEFT :: RTransition
pattern R_TRANSITION_SLIDE_LEFT = RTransition 777
pattern R_TRANSITION_SLIDE_RIGHT :: RTransition
pattern R_TRANSITION_SLIDE_RIGHT = RTransition 778

newtype ROrientation = ROrientation CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_HORIZONTAL :: ROrientation
pattern R_HORIZONTAL = ROrientation 624
pattern R_VERTICAL :: ROrientation
pattern R_VERTICAL = ROrientation 625

-- | The looks of a R_WIDGET_BACKGROUND_BLUR. The first three are drawn by
-- Raven from what the window shows behind the widget; the last one is the
-- blur of the OS, which sees what is behind the window itself.
newtype RBlurStyle = RBlurStyle CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | What is behind, out of focus, and nothing else (a Gaussian blur).
pattern R_BLUR_PLAIN :: RBlurStyle
pattern R_BLUR_PLAIN = RBlurStyle 1088
-- | Frosted: blurred, its colors a little stronger, under a veil of the
-- panel color of the theme, a fine grain and a hairline. The default.
pattern R_BLUR_FROSTED :: RBlurStyle
pattern R_BLUR_FROSTED = RBlurStyle 1089
-- | Glass: a slab that bends what is behind at its rim and catches the
-- light, as the glass materials do (RMaterial); the radius says how
-- frosted it is.
pattern R_BLUR_GLASS :: RBlurStyle
pattern R_BLUR_GLASS = RBlurStyle 1090
-- | Native: the widget opens the window there, and what shows is what the
-- OS puts behind the window (Rwindow_set_backdrop: the blurred desktop,
-- acrylic, mica), under a light veil. It needs a window created with
-- R_WINDOW_TRANSPARENT that the OS could make see-through; anywhere
-- else it is drawn as R_BLUR_FROSTED. The radius is the one of the OS.
pattern R_BLUR_NATIVE :: RBlurStyle
pattern R_BLUR_NATIVE = RBlurStyle 1091

newtype RDirection = RDirection CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | A widget: the direction of its parent (the default).
pattern R_DIRECTION_INHERIT :: RDirection
pattern R_DIRECTION_INHERIT = RDirection 0
pattern R_DIRECTION_LTR :: RDirection
pattern R_DIRECTION_LTR = RDirection 1824
pattern R_DIRECTION_RTL :: RDirection
pattern R_DIRECTION_RTL = RDirection 1825
-- | The windows: the direction of the language.
pattern R_DIRECTION_AUTO :: RDirection
pattern R_DIRECTION_AUTO = RDirection 1826

newtype RTextAlign = RTextAlign CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_TEXT_ALIGN_LEFT :: RTextAlign
pattern R_TEXT_ALIGN_LEFT = RTextAlign 640
pattern R_TEXT_ALIGN_CENTER :: RTextAlign
pattern R_TEXT_ALIGN_CENTER = RTextAlign 641
pattern R_TEXT_ALIGN_RIGHT :: RTextAlign
pattern R_TEXT_ALIGN_RIGHT = RTextAlign 642

newtype RToastKind = RToastKind CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | A neutral message (the accent of the theme).
pattern R_TOAST_INFO :: RToastKind
pattern R_TOAST_INFO = RToastKind 1248
pattern R_TOAST_SUCCESS :: RToastKind
pattern R_TOAST_SUCCESS = RToastKind 1249
pattern R_TOAST_WARNING :: RToastKind
pattern R_TOAST_WARNING = RToastKind 1250
pattern R_TOAST_ERROR :: RToastKind
pattern R_TOAST_ERROR = RToastKind 1251

-- | What a Toast does with a message that comes while another one is shown.
newtype RToastQueue = RToastQueue CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | The new message takes the place of the one shown (the default).
pattern R_TOAST_REPLACE :: RToastQueue
pattern R_TOAST_REPLACE = RToastQueue 1280
-- | The messages are shown one after the other, each for its own time.
pattern R_TOAST_SEQUENCE :: RToastQueue
pattern R_TOAST_SEQUENCE = RToastQueue 1281
-- | Up to Rtoast_set_max_visible messages are shown at once, one above the other; the others wait.
pattern R_TOAST_STACK :: RToastQueue
pattern R_TOAST_STACK = RToastQueue 1282

newtype RButtonStyle = RButtonStyle CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | Filled with R_COLOR_CONTROL.
pattern R_BUTTON_DEFAULT :: RButtonStyle
pattern R_BUTTON_DEFAULT = RButtonStyle 656
-- | Filled with R_COLOR_ACCENT.
pattern R_BUTTON_PRIMARY :: RButtonStyle
pattern R_BUTTON_PRIMARY = RButtonStyle 657
-- | No background until hovered (toolbars).
pattern R_BUTTON_FLAT :: RButtonStyle
pattern R_BUTTON_FLAT = RButtonStyle 658

newtype RCheckboxStyle = RCheckboxStyle CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_CHECKBOX_CHECK :: RCheckboxStyle
pattern R_CHECKBOX_CHECK = RCheckboxStyle 672
-- | Round mark. Checking one unchecks the other radio boxes of the same parent.
pattern R_CHECKBOX_RADIO :: RCheckboxStyle
pattern R_CHECKBOX_RADIO = RCheckboxStyle 673
-- | On / off switch.
pattern R_CHECKBOX_SWITCH :: RCheckboxStyle
pattern R_CHECKBOX_SWITCH = RCheckboxStyle 674

newtype RStretch = RStretch CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | Stretches the image to the widget, ignoring its aspect ratio.
pattern R_STRETCH_FILL :: RStretch
pattern R_STRETCH_FILL = RStretch 688
-- | Largest size that fits entirely, keeping the aspect ratio.
pattern R_STRETCH_FIT :: RStretch
pattern R_STRETCH_FIT = RStretch 689
-- | Smallest size that covers the widget, keeping the aspect ratio (cropped).
pattern R_STRETCH_COVER :: RStretch
pattern R_STRETCH_COVER = RStretch 690
-- | Original size, centered.
pattern R_STRETCH_NONE :: RStretch
pattern R_STRETCH_NONE = RStretch 691

-- | The axes of a Plot.
newtype RPlotAxis = RPlotAxis CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | The horizontal axis under the data.
pattern R_PLOT_X :: RPlotAxis
pattern R_PLOT_X = RPlotAxis 1312
-- | The vertical axis on the left.
pattern R_PLOT_Y :: RPlotAxis
pattern R_PLOT_Y = RPlotAxis 1313
-- | A second horizontal axis, over the data: shown when a series or a drag line uses it.
pattern R_PLOT_X2 :: RPlotAxis
pattern R_PLOT_X2 = RPlotAxis 1314
-- | A second vertical axis, on the right: shown when a series or a drag line uses it.
pattern R_PLOT_Y2 :: RPlotAxis
pattern R_PLOT_Y2 = RPlotAxis 1315

-- | How an axis of a Plot spreads its values.
newtype RPlotScale = RPlotScale CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_PLOT_SCALE_LINEAR :: RPlotScale
pattern R_PLOT_SCALE_LINEAR = RPlotScale 1328
-- | Powers of ten at even distances; only values above zero are shown.
pattern R_PLOT_SCALE_LOG :: RPlotScale
pattern R_PLOT_SCALE_LOG = RPlotScale 1329
-- | Symmetric logarithm: linear between -t and t (Rplot_set_axis_linear_threshold,
-- 1 by default), logarithmic beyond on both sides: shows zero and the
-- negative values a logarithm cannot.
pattern R_PLOT_SCALE_SYMLOG :: RPlotScale
pattern R_PLOT_SCALE_SYMLOG = RPlotScale 1330

-- | How the line of a series of a Plot is drawn.
newtype RPlotLine = RPlotLine CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_PLOT_LINE_SOLID :: RPlotLine
pattern R_PLOT_LINE_SOLID = RPlotLine 1344
pattern R_PLOT_LINE_DASHED :: RPlotLine
pattern R_PLOT_LINE_DASHED = RPlotLine 1345
pattern R_PLOT_LINE_DOTTED :: RPlotLine
pattern R_PLOT_LINE_DOTTED = RPlotLine 1346
pattern R_PLOT_LINE_DASH_DOT :: RPlotLine
pattern R_PLOT_LINE_DASH_DOT = RPlotLine 1347
-- | No line: only the markers (a scatter plot).
pattern R_PLOT_LINE_NONE :: RPlotLine
pattern R_PLOT_LINE_NONE = RPlotLine 1348

-- | The mark drawn on each point of a series of a Plot.
newtype RPlotMarker = RPlotMarker CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_PLOT_MARKER_NONE :: RPlotMarker
pattern R_PLOT_MARKER_NONE = RPlotMarker 1360
pattern R_PLOT_MARKER_CIRCLE :: RPlotMarker
pattern R_PLOT_MARKER_CIRCLE = RPlotMarker 1361
pattern R_PLOT_MARKER_SQUARE :: RPlotMarker
pattern R_PLOT_MARKER_SQUARE = RPlotMarker 1362
pattern R_PLOT_MARKER_DIAMOND :: RPlotMarker
pattern R_PLOT_MARKER_DIAMOND = RPlotMarker 1363
pattern R_PLOT_MARKER_TRIANGLE_UP :: RPlotMarker
pattern R_PLOT_MARKER_TRIANGLE_UP = RPlotMarker 1364
pattern R_PLOT_MARKER_TRIANGLE_DOWN :: RPlotMarker
pattern R_PLOT_MARKER_TRIANGLE_DOWN = RPlotMarker 1365
pattern R_PLOT_MARKER_CROSS :: RPlotMarker
pattern R_PLOT_MARKER_CROSS = RPlotMarker 1366
pattern R_PLOT_MARKER_PLUS :: RPlotMarker
pattern R_PLOT_MARKER_PLUS = RPlotMarker 1367
pattern R_PLOT_MARKER_ASTERISK :: RPlotMarker
pattern R_PLOT_MARKER_ASTERISK = RPlotMarker 1368

-- | Where the legend of a Plot is.
newtype RPlotLegend = RPlotLegend CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_PLOT_LEGEND_HIDDEN :: RPlotLegend
pattern R_PLOT_LEGEND_HIDDEN = RPlotLegend 1376
pattern R_PLOT_LEGEND_TOP_LEFT :: RPlotLegend
pattern R_PLOT_LEGEND_TOP_LEFT = RPlotLegend 1377
pattern R_PLOT_LEGEND_TOP :: RPlotLegend
pattern R_PLOT_LEGEND_TOP = RPlotLegend 1378
pattern R_PLOT_LEGEND_TOP_RIGHT :: RPlotLegend
pattern R_PLOT_LEGEND_TOP_RIGHT = RPlotLegend 1379
pattern R_PLOT_LEGEND_RIGHT :: RPlotLegend
pattern R_PLOT_LEGEND_RIGHT = RPlotLegend 1380
pattern R_PLOT_LEGEND_BOTTOM_RIGHT :: RPlotLegend
pattern R_PLOT_LEGEND_BOTTOM_RIGHT = RPlotLegend 1381
pattern R_PLOT_LEGEND_BOTTOM :: RPlotLegend
pattern R_PLOT_LEGEND_BOTTOM = RPlotLegend 1382
pattern R_PLOT_LEGEND_BOTTOM_LEFT :: RPlotLegend
pattern R_PLOT_LEGEND_BOTTOM_LEFT = RPlotLegend 1383
pattern R_PLOT_LEGEND_LEFT :: RPlotLegend
pattern R_PLOT_LEGEND_LEFT = RPlotLegend 1384
-- | To the right of the data, outside it.
pattern R_PLOT_LEGEND_OUTSIDE :: RPlotLegend
pattern R_PLOT_LEGEND_OUTSIDE = RPlotLegend 1385

-- | Shorthands of Rtext_editor_set_language for two languages.
newtype RSyntax = RSyntax CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_SYNTAX_NONE :: RSyntax
pattern R_SYNTAX_NONE = RSyntax 832
pattern R_SYNTAX_XML :: RSyntax
pattern R_SYNTAX_XML = RSyntax 833
pattern R_SYNTAX_C :: RSyntax
pattern R_SYNTAX_C = RSyntax 834

newtype RLanguageFlags = RLanguageFlags CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | Tags and attributes instead of code: XML, HTML. The words and the comments of the description are not used.
pattern R_LANGUAGE_MARKUP :: RLanguageFlags
pattern R_LANGUAGE_MARKUP = RLanguageFlags 1
-- | Selectors, properties and values: CSS.
pattern R_LANGUAGE_STYLE_SHEET :: RLanguageFlags
pattern R_LANGUAGE_STYLE_SHEET = RLanguageFlags 2
-- | A block comment may hold another one (Rust).
pattern R_LANGUAGE_NESTED_COMMENTS :: RLanguageFlags
pattern R_LANGUAGE_NESTED_COMMENTS = RLanguageFlags 4
-- | Between single quotes there is one character; any other single quote is not a text (C, Java, Rust).
pattern R_LANGUAGE_CHAR_LITERALS :: RLanguageFlags
pattern R_LANGUAGE_CHAR_LITERALS = RLanguageFlags 8
-- | A line that ends with ':' indents the next one (Python).
pattern R_LANGUAGE_INDENT_ON_COLON :: RLanguageFlags
pattern R_LANGUAGE_INDENT_ON_COLON = RLanguageFlags 16
-- | The words of the language are recognized whatever their case.
pattern R_LANGUAGE_IGNORE_CASE :: RLanguageFlags
pattern R_LANGUAGE_IGNORE_CASE = RLanguageFlags 32
-- | Markup that holds JavaScript in script elements and CSS in style elements (HTML).
pattern R_LANGUAGE_EMBEDS :: RLanguageFlags
pattern R_LANGUAGE_EMBEDS = RLanguageFlags 64
-- | Formatted text written in Markdown: titles, strong and emphasized
-- text, code, links, lists. The editor shows the text as it is written,
-- marks included, with strong text in bold, emphasized text in italic
-- and titles in bold: what will be formatted is seen while typing.
pattern R_LANGUAGE_FORMATTED_TEXT :: RLanguageFlags
pattern R_LANGUAGE_FORMATTED_TEXT = RLanguageFlags 128

-- | What a part of the text is, for the language of the editor (Rtext_editor_get_token).
newtype RToken = RToken CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_TOKEN_TEXT :: RToken
pattern R_TOKEN_TEXT = RToken 928
pattern R_TOKEN_COMMENT :: RToken
pattern R_TOKEN_COMMENT = RToken 929
pattern R_TOKEN_STRING :: RToken
pattern R_TOKEN_STRING = RToken 930
pattern R_TOKEN_NUMBER :: RToken
pattern R_TOKEN_NUMBER = RToken 931
pattern R_TOKEN_KEYWORD :: RToken
pattern R_TOKEN_KEYWORD = RToken 932
-- | A type, or a name that is always there.
pattern R_TOKEN_TYPE :: RToken
pattern R_TOKEN_TYPE = RToken 933
-- | A name followed by an opening parenthesis.
pattern R_TOKEN_FUNCTION :: RToken
pattern R_TOKEN_FUNCTION = RToken 934
-- | The name of an element, in markup and in the selectors of a style sheet.
pattern R_TOKEN_TAG :: RToken
pattern R_TOKEN_TAG = RToken 935
-- | The name of an attribute (markup), of a class or of a property (style sheets), of a key (JSON).
pattern R_TOKEN_ATTRIBUTE :: RToken
pattern R_TOKEN_ATTRIBUTE = RToken 936
pattern R_TOKEN_PUNCTUATION :: RToken
pattern R_TOKEN_PUNCTUATION = RToken 937
-- | A line for the preprocessor, an annotation, a declaration of a document.
pattern R_TOKEN_DIRECTIVE :: RToken
pattern R_TOKEN_DIRECTIVE = RToken 938
-- | Formatted text (the Markdown language): a title, strong text, emphasized text, a link.
pattern R_TOKEN_HEADING :: RToken
pattern R_TOKEN_HEADING = RToken 939
pattern R_TOKEN_STRONG :: RToken
pattern R_TOKEN_STRONG = RToken 940
pattern R_TOKEN_EMPHASIS :: RToken
pattern R_TOKEN_EMPHASIS = RToken 941
pattern R_TOKEN_LINK :: RToken
pattern R_TOKEN_LINK = RToken 942

-- | What a marker says about the text it underlines.
newtype RMarkerKind = RMarkerKind CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_MARKER_ERROR :: RMarkerKind
pattern R_MARKER_ERROR = RMarkerKind 848
pattern R_MARKER_WARNING :: RMarkerKind
pattern R_MARKER_WARNING = RMarkerKind 849
pattern R_MARKER_INFO :: RMarkerKind
pattern R_MARKER_INFO = RMarkerKind 850

-- | Buttons of a header bar.
newtype RHeaderButtons = RHeaderButtons CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_HEADER_NO_BUTTONS :: RHeaderButtons
pattern R_HEADER_NO_BUTTONS = RHeaderButtons 0
pattern R_HEADER_CLOSE :: RHeaderButtons
pattern R_HEADER_CLOSE = RHeaderButtons 1
pattern R_HEADER_MINIMIZE :: RHeaderButtons
pattern R_HEADER_MINIMIZE = RHeaderButtons 2
pattern R_HEADER_MAXIMIZE :: RHeaderButtons
pattern R_HEADER_MAXIMIZE = RHeaderButtons 4
pattern R_HEADER_ALL_BUTTONS :: RHeaderButtons
pattern R_HEADER_ALL_BUTTONS = RHeaderButtons 7

-- | A progress bar on the icon of the application (Rwindow_set_taskbar_progress).
newtype RTaskbarProgress = RTaskbarProgress CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_TASKBAR_PROGRESS_NONE :: RTaskbarProgress
pattern R_TASKBAR_PROGRESS_NONE = RTaskbarProgress 1952
pattern R_TASKBAR_PROGRESS_NORMAL :: RTaskbarProgress
pattern R_TASKBAR_PROGRESS_NORMAL = RTaskbarProgress 1953
-- | Shown as paused (yellow on Windows).
pattern R_TASKBAR_PROGRESS_PAUSED :: RTaskbarProgress
pattern R_TASKBAR_PROGRESS_PAUSED = RTaskbarProgress 1954
-- | Shown as failed (red on Windows).
pattern R_TASKBAR_PROGRESS_ERROR :: RTaskbarProgress
pattern R_TASKBAR_PROGRESS_ERROR = RTaskbarProgress 1955
-- | Something goes on, without knowing how far it is.
pattern R_TASKBAR_PROGRESS_INDETERMINATE :: RTaskbarProgress
pattern R_TASKBAR_PROGRESS_INDETERMINATE = RTaskbarProgress 1956

-- | Where a panel goes in a dock area, relative to another panel or to the whole area.
newtype RDockSide = RDockSide CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_DOCK_LEFT :: RDockSide
pattern R_DOCK_LEFT = RDockSide 864
pattern R_DOCK_RIGHT :: RDockSide
pattern R_DOCK_RIGHT = RDockSide 865
pattern R_DOCK_TOP :: RDockSide
pattern R_DOCK_TOP = RDockSide 866
pattern R_DOCK_BOTTOM :: RDockSide
pattern R_DOCK_BOTTOM = RDockSide 867
-- | Behind the same tabs as the other panel.
pattern R_DOCK_TAB :: RDockSide
pattern R_DOCK_TAB = RDockSide 868

newtype RVisualPreset = RVisualPreset CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | The look of the theme of the window: its platform, dark or light.
pattern R_PRESET_AUTO :: RVisualPreset
pattern R_PRESET_AUTO = RVisualPreset 1296
-- | Cards with soft shadows, few lines (follows dark or light).
pattern R_PRESET_MODERN :: RVisualPreset
pattern R_PRESET_MODERN = RVisualPreset 1297
-- | Headers in gradients, a grid of lines, thick links that glow; pins of the type "exec" are arrows.
pattern R_PRESET_BLUEPRINT :: RVisualPreset
pattern R_PRESET_BLUEPRINT = RVisualPreset 1298
-- | Compact nodes, flat headers in muted colors, thin links.
pattern R_PRESET_BLENDER :: RVisualPreset
pattern R_PRESET_BLENDER = RVisualPreset 1299
-- | Almost black, outlines and links of light.
pattern R_PRESET_NEON :: RVisualPreset
pattern R_PRESET_NEON = RVisualPreset 1300

-- | What an item of a timeline is.
newtype RTimelineItem = RTimelineItem CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_TIMELINE_NONE :: RTimelineItem
pattern R_TIMELINE_NONE = RTimelineItem 1120
pattern R_TIMELINE_CLIP :: RTimelineItem
pattern R_TIMELINE_CLIP = RTimelineItem 1121
pattern R_TIMELINE_KEY :: RTimelineItem
pattern R_TIMELINE_KEY = RTimelineItem 1122
pattern R_TIMELINE_MARKER :: RTimelineItem
pattern R_TIMELINE_MARKER = RTimelineItem 1123

newtype RBlockShape = RBlockShape CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | A step: snaps under another one, others snap under it.
pattern R_BLOCK_STATEMENT :: RBlockShape
pattern R_BLOCK_STATEMENT = RBlockShape 1792
-- | Starts a script ("when the game starts"): nothing goes above it.
pattern R_BLOCK_HAT :: RBlockShape
pattern R_BLOCK_HAT = RBlockShape 1793
-- | Ends a script ("stop"): nothing goes under it.
pattern R_BLOCK_CAP :: RBlockShape
pattern R_BLOCK_CAP = RBlockShape 1794
-- | Wraps stacks in its mouths ("repeat", "if ... else").
pattern R_BLOCK_C :: RBlockShape
pattern R_BLOCK_C = RBlockShape 1795
-- | A rounded value that goes into a slot.
pattern R_BLOCK_REPORTER :: RBlockShape
pattern R_BLOCK_REPORTER = RBlockShape 1796
-- | A hexagonal condition that goes into a %b slot (or a %r one).
pattern R_BLOCK_BOOLEAN :: RBlockShape
pattern R_BLOCK_BOOLEAN = RBlockShape 1797

newtype RBlockPlace = RBlockPlace CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_BLOCK_NEXT :: RBlockPlace
pattern R_BLOCK_NEXT = RBlockPlace 1808
pattern R_BLOCK_BODY :: RBlockPlace
pattern R_BLOCK_BODY = RBlockPlace 1809
pattern R_BLOCK_INPUT :: RBlockPlace
pattern R_BLOCK_INPUT = RBlockPlace 1810

-- | How much a notification of the system asks for attention (Rnotification_show).
newtype RNotificationUrgency = RNotificationUrgency CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | No sound; may only go to the list of notifications.
pattern R_NOTIFICATION_LOW :: RNotificationUrgency
pattern R_NOTIFICATION_LOW = RNotificationUrgency 1264
pattern R_NOTIFICATION_NORMAL :: RNotificationUrgency
pattern R_NOTIFICATION_NORMAL = RNotificationUrgency 1265
-- | Stays until the user dismisses it, where the system allows it.
pattern R_NOTIFICATION_CRITICAL :: RNotificationUrgency
pattern R_NOTIFICATION_CRITICAL = RNotificationUrgency 1266

newtype RFileDialogFlags = RFileDialogFlags CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | Several files can be chosen (Rfile_dialog_open): see Rfile_dialog_get_result.
pattern R_FILE_DIALOG_MULTIPLE :: RFileDialogFlags
pattern R_FILE_DIALOG_MULTIPLE = RFileDialogFlags 1
-- | Never the dialog of the OS: the file browser of Raven, in a dialog of the parent window.
pattern R_FILE_DIALOG_RAVEN :: RFileDialogFlags
pattern R_FILE_DIALOG_RAVEN = RFileDialogFlags 2
pattern R_FILE_DIALOG_SHOW_HIDDEN :: RFileDialogFlags
pattern R_FILE_DIALOG_SHOW_HIDDEN = RFileDialogFlags 4

newtype RFileBrowserMode = RFileBrowserMode CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | Choose an existing file.
pattern R_FILE_BROWSER_OPEN :: RFileBrowserMode
pattern R_FILE_BROWSER_OPEN = RFileBrowserMode 896
-- | Choose where to save: a name is typed; an existing file must be confirmed.
pattern R_FILE_BROWSER_SAVE :: RFileBrowserMode
pattern R_FILE_BROWSER_SAVE = RFileBrowserMode 897
-- | Choose a folder.
pattern R_FILE_BROWSER_FOLDER :: RFileBrowserMode
pattern R_FILE_BROWSER_FOLDER = RFileBrowserMode 898

newtype RMessageButtons = RMessageButtons CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_MESSAGE_OK :: RMessageButtons
pattern R_MESSAGE_OK = RMessageButtons 912
pattern R_MESSAGE_OK_CANCEL :: RMessageButtons
pattern R_MESSAGE_OK_CANCEL = RMessageButtons 913
pattern R_MESSAGE_YES_NO :: RMessageButtons
pattern R_MESSAGE_YES_NO = RMessageButtons 914
pattern R_MESSAGE_YES_NO_CANCEL :: RMessageButtons
pattern R_MESSAGE_YES_NO_CANCEL = RMessageButtons 915

newtype RMessageResult = RMessageResult CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | Dismissed (Escape), or the Cancel button.
pattern R_MESSAGE_RESULT_CANCEL :: RMessageResult
pattern R_MESSAGE_RESULT_CANCEL = RMessageResult 0
pattern R_MESSAGE_RESULT_OK :: RMessageResult
pattern R_MESSAGE_RESULT_OK = RMessageResult 1
pattern R_MESSAGE_RESULT_YES :: RMessageResult
pattern R_MESSAGE_RESULT_YES = RMessageResult 2
pattern R_MESSAGE_RESULT_NO :: RMessageResult
pattern R_MESSAGE_RESULT_NO = RMessageResult 3

-- | How a spring moves. Animations in Raven are springs, as on iOS: a value is
-- pulled toward its target and may run past it before settling, which is what
-- makes a control feel alive rather than mechanical.
newtype RMotion = RMotion CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | Fast, no bounce: hover, highlights.
pattern R_MOTION_SNAPPY :: RMotion
pattern R_MOTION_SNAPPY = RMotion 800
-- | Slower, no bounce: scrolling, fades, sizes.
pattern R_MOTION_SMOOTH :: RMotion
pattern R_MOTION_SMOOTH = RMotion 801
-- | Runs a little past the target and comes back: switches, popups, pressed buttons.
pattern R_MOTION_BOUNCY :: RMotion
pattern R_MOTION_BOUNCY = RMotion 802
-- | Swings several times around the target: things that fly in or get dropped.
pattern R_MOTION_WOBBLY :: RMotion
pattern R_MOTION_WOBBLY = RMotion 803

-- | The effects that come with Raven (Reffect_get).
newtype REffect = REffect CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | Soft lights that drift slowly: color, color2; params: speed, scale, intensity, -. Animated.
pattern R_EFFECT_AURORA :: REffect
pattern R_EFFECT_AURORA = REffect 2880
-- | A band of light that sweeps across (loading placeholders): color = base, color2 = light; params: speed, width, angle, -. Animated.
pattern R_EFFECT_SHIMMER :: REffect
pattern R_EFFECT_SHIMMER = REffect 2881
-- | A glowing edge whose colors run around the shape: color, color2; params: speed, width, glow, -. Animated.
pattern R_EFFECT_NEON :: REffect
pattern R_EFFECT_NEON = REffect 2882
-- | Rings that ripple what is behind, like water (reads the backdrop): color = tint; params: speed, strength, density, center x (0..1). Animated.
pattern R_EFFECT_RIPPLE :: REffect
pattern R_EFFECT_RIPPLE = REffect 2883
-- | Bands of color that flow into each other: color, color2; params: speed, scale, contrast, -. Animated.
pattern R_EFFECT_PLASMA :: REffect
pattern R_EFFECT_PLASMA = REffect 2884
-- | A gradient with the fine grain of film: color (top left) to color2 (bottom right); params: grain, angle, -, -. Animated grain.
pattern R_EFFECT_GRAIN :: REffect
pattern R_EFFECT_GRAIN = REffect 2885
-- | A soft light that follows a point: color = light, color2 = background; params: x, y (0..1), radius (0..1), -.
pattern R_EFFECT_SPOTLIGHT :: REffect
pattern R_EFFECT_SPOTLIGHT = REffect 2886
-- | Four colors blended across the surface, slowly moving: color, color2 and their mix; params: speed, -, -, -. Animated.
pattern R_EFFECT_MESH :: REffect
pattern R_EFFECT_MESH = REffect 2887

-- | Flags of Rshader_create.
newtype RShaderFlags = RShaderFlags CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | The effect reads what is drawn behind it (raven_backdrop, raven_backdrop_blur).
pattern R_SHADER_BACKDROP :: RShaderFlags
pattern R_SHADER_BACKDROP = RShaderFlags 1
-- | The effect changes with time: the window keeps drawing frames while it is shown.
pattern R_SHADER_ANIMATED :: RShaderFlags
pattern R_SHADER_ANIMATED = RShaderFlags 2

-- | The kind of value an attribute takes.
newtype RXmlValueType = RXmlValueType CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_XML_TEXT :: RXmlValueType
pattern R_XML_TEXT = RXmlValueType 960
-- | "true" or "false".
pattern R_XML_BOOL :: RXmlValueType
pattern R_XML_BOOL = RXmlValueType 961
pattern R_XML_NUMBER :: RXmlValueType
pattern R_XML_NUMBER = RXmlValueType 962
-- | One of the words of `choices`.
pattern R_XML_CHOICE :: RXmlValueType
pattern R_XML_CHOICE = RXmlValueType 963
-- | "#RGB", "#RRGGBB", "#RRGGBBAA", or a name: white, black, red, transparent...
pattern R_XML_COLOR :: RXmlValueType
pattern R_XML_COLOR = RXmlValueType 964
-- | The name of an action (Raction_bind).
pattern R_XML_ACTION :: RXmlValueType
pattern R_XML_ACTION = RXmlValueType 965
-- | The path of a file.
pattern R_XML_FILE :: RXmlValueType
pattern R_XML_FILE = RXmlValueType 966
-- | Several numbers separated by commas: "8,4", "0,0,1,1".
pattern R_XML_NUMBERS :: RXmlValueType
pattern R_XML_NUMBERS = RXmlValueType 967
-- | Texts separated by '|': "One|Two|Three".
pattern R_XML_LIST :: RXmlValueType
pattern R_XML_LIST = RXmlValueType 968

newtype RColorFilter = RColorFilter CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

pattern R_COLOR_FILTER_NONE :: RColorFilter
pattern R_COLOR_FILTER_NONE = RColorFilter 1856
-- | Corrections: the colors that cannot be told apart are moved to colors that can.
pattern R_COLOR_FILTER_PROTANOPIA :: RColorFilter
pattern R_COLOR_FILTER_PROTANOPIA = RColorFilter 1857
pattern R_COLOR_FILTER_DEUTERANOPIA :: RColorFilter
pattern R_COLOR_FILTER_DEUTERANOPIA = RColorFilter 1858
pattern R_COLOR_FILTER_TRITANOPIA :: RColorFilter
pattern R_COLOR_FILTER_TRITANOPIA = RColorFilter 1859
pattern R_COLOR_FILTER_GRAYSCALE :: RColorFilter
pattern R_COLOR_FILTER_GRAYSCALE = RColorFilter 1860
pattern R_COLOR_FILTER_INVERT :: RColorFilter
pattern R_COLOR_FILTER_INVERT = RColorFilter 1861
-- | Simulations, to see an interface as the color blind see it.
pattern R_COLOR_FILTER_SIMULATE_PROTANOPIA :: RColorFilter
pattern R_COLOR_FILTER_SIMULATE_PROTANOPIA = RColorFilter 1862
pattern R_COLOR_FILTER_SIMULATE_DEUTERANOPIA :: RColorFilter
pattern R_COLOR_FILTER_SIMULATE_DEUTERANOPIA = RColorFilter 1863
pattern R_COLOR_FILTER_SIMULATE_TRITANOPIA :: RColorFilter
pattern R_COLOR_FILTER_SIMULATE_TRITANOPIA = RColorFilter 1864
pattern R_COLOR_FILTER_SIMULATE_ACHROMATOPSIA :: RColorFilter
pattern R_COLOR_FILTER_SIMULATE_ACHROMATOPSIA = RColorFilter 1865

newtype RAccessibleRole = RAccessibleRole CInt
    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)

-- | The role of the type of the widget (the default).
pattern R_ROLE_AUTO :: RAccessibleRole
pattern R_ROLE_AUTO = RAccessibleRole 0
-- | Not exposed: its children are seen as the children of its parent.
pattern R_ROLE_NONE :: RAccessibleRole
pattern R_ROLE_NONE = RAccessibleRole 1888
pattern R_ROLE_WINDOW :: RAccessibleRole
pattern R_ROLE_WINDOW = RAccessibleRole 1889
pattern R_ROLE_GROUP :: RAccessibleRole
pattern R_ROLE_GROUP = RAccessibleRole 1890
pattern R_ROLE_TEXT :: RAccessibleRole
pattern R_ROLE_TEXT = RAccessibleRole 1891
pattern R_ROLE_HEADING :: RAccessibleRole
pattern R_ROLE_HEADING = RAccessibleRole 1892
pattern R_ROLE_LINK :: RAccessibleRole
pattern R_ROLE_LINK = RAccessibleRole 1893
pattern R_ROLE_BUTTON :: RAccessibleRole
pattern R_ROLE_BUTTON = RAccessibleRole 1894
pattern R_ROLE_TOGGLE_BUTTON :: RAccessibleRole
pattern R_ROLE_TOGGLE_BUTTON = RAccessibleRole 1895
pattern R_ROLE_CHECKBOX :: RAccessibleRole
pattern R_ROLE_CHECKBOX = RAccessibleRole 1896
pattern R_ROLE_RADIO_BUTTON :: RAccessibleRole
pattern R_ROLE_RADIO_BUTTON = RAccessibleRole 1897
pattern R_ROLE_SLIDER :: RAccessibleRole
pattern R_ROLE_SLIDER = RAccessibleRole 1898
pattern R_ROLE_PROGRESS_BAR :: RAccessibleRole
pattern R_ROLE_PROGRESS_BAR = RAccessibleRole 1899
pattern R_ROLE_SPIN_BUTTON :: RAccessibleRole
pattern R_ROLE_SPIN_BUTTON = RAccessibleRole 1900
pattern R_ROLE_TEXT_FIELD :: RAccessibleRole
pattern R_ROLE_TEXT_FIELD = RAccessibleRole 1901
pattern R_ROLE_DOCUMENT :: RAccessibleRole
pattern R_ROLE_DOCUMENT = RAccessibleRole 1902
pattern R_ROLE_COMBO_BOX :: RAccessibleRole
pattern R_ROLE_COMBO_BOX = RAccessibleRole 1903
pattern R_ROLE_LIST :: RAccessibleRole
pattern R_ROLE_LIST = RAccessibleRole 1904
pattern R_ROLE_LIST_ITEM :: RAccessibleRole
pattern R_ROLE_LIST_ITEM = RAccessibleRole 1905
pattern R_ROLE_TREE :: RAccessibleRole
pattern R_ROLE_TREE = RAccessibleRole 1906
pattern R_ROLE_TREE_ITEM :: RAccessibleRole
pattern R_ROLE_TREE_ITEM = RAccessibleRole 1907
pattern R_ROLE_TABLE :: RAccessibleRole
pattern R_ROLE_TABLE = RAccessibleRole 1908
pattern R_ROLE_MENU_BAR :: RAccessibleRole
pattern R_ROLE_MENU_BAR = RAccessibleRole 1909
pattern R_ROLE_MENU :: RAccessibleRole
pattern R_ROLE_MENU = RAccessibleRole 1910
pattern R_ROLE_MENU_ITEM :: RAccessibleRole
pattern R_ROLE_MENU_ITEM = RAccessibleRole 1911
pattern R_ROLE_TAB_LIST :: RAccessibleRole
pattern R_ROLE_TAB_LIST = RAccessibleRole 1912
pattern R_ROLE_TAB :: RAccessibleRole
pattern R_ROLE_TAB = RAccessibleRole 1913
pattern R_ROLE_TAB_PANEL :: RAccessibleRole
pattern R_ROLE_TAB_PANEL = RAccessibleRole 1914
pattern R_ROLE_TOOLBAR :: RAccessibleRole
pattern R_ROLE_TOOLBAR = RAccessibleRole 1915
pattern R_ROLE_STATUS_BAR :: RAccessibleRole
pattern R_ROLE_STATUS_BAR = RAccessibleRole 1916
pattern R_ROLE_DIALOG :: RAccessibleRole
pattern R_ROLE_DIALOG = RAccessibleRole 1917
pattern R_ROLE_ALERT :: RAccessibleRole
pattern R_ROLE_ALERT = RAccessibleRole 1918
pattern R_ROLE_IMAGE :: RAccessibleRole
pattern R_ROLE_IMAGE = RAccessibleRole 1919
pattern R_ROLE_CANVAS :: RAccessibleRole
pattern R_ROLE_CANVAS = RAccessibleRole 1920
pattern R_ROLE_SEPARATOR :: RAccessibleRole
pattern R_ROLE_SEPARATOR = RAccessibleRole 1921
pattern R_ROLE_SCROLL_BAR :: RAccessibleRole
pattern R_ROLE_SCROLL_BAR = RAccessibleRole 1922
pattern R_ROLE_PANE :: RAccessibleRole
pattern R_ROLE_PANE = RAccessibleRole 1923

-- | Registers a function called once per frame, before the layout.
-- The body of your loop does the same job; the callback is also called
-- while the OS keeps the thread in a loop of its own (on Windows, while
-- the user moves or resizes a window), when your loop is not running.
type RFrameCallback = FunPtr (CDouble -> Ptr () -> IO ())

-- | Makes a 'RFrameCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRFrameCallback :: (CDouble -> Ptr () -> IO ()) -> IO RFrameCallback

type RTaskCallback = FunPtr (Ptr () -> IO ())

-- | Makes a 'RTaskCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRTaskCallback :: (Ptr () -> IO ()) -> IO RTaskCallback

type RParallelRangeCallback = FunPtr (Word32 -> Word32 -> Ptr () -> IO ())

-- | Makes a 'RParallelRangeCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRParallelRangeCallback :: (Word32 -> Word32 -> Ptr () -> IO ()) -> IO RParallelRangeCallback

type RTimerCallback = FunPtr (Rid -> Ptr () -> IO ())

-- | Makes a 'RTimerCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRTimerCallback :: (Rid -> Ptr () -> IO ()) -> IO RTimerCallback

-- | Actions connect widgets to application code by name. A widget carries an
-- action name (set from XML with OnClicked="file.save", or from C with
-- Rwidget_set_action); when its main event fires, the function bound to that
-- name is called. Names are resolved when the event fires, so actions can be
-- bound before or after the widgets are created.
type RActionCallback = FunPtr (Rid -> CString -> Ptr () -> IO ())

-- | Makes a 'RActionCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRActionCallback :: (Rid -> CString -> Ptr () -> IO ()) -> IO RActionCallback

-- | Registers a function called whenever an error is raised.
-- The function may be registered before Rinit. It is called on the
-- thread that raised the error: errors of the renderer (and the messages
-- of the Vulkan validation layers) come from the render thread.
type RErrorCallback = FunPtr (RError -> RSeverity -> CString -> IO ())

-- | Makes a 'RErrorCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRErrorCallback :: (RError -> RSeverity -> CString -> IO ()) -> IO RErrorCallback

-- | Registers a function called after Raven recovered from the loss
-- of the GPU device (a driver reset or update, a GPU removed, a long sleep).
--
-- Raven then starts the backend again and remakes what it can by itself:
-- the surfaces of the windows, the fonts, the shaders, the textures made
-- from image files and the small textures (up to 256 x 256). The pixels of
-- the larger textures made from pixels are lost: the textures keep their
-- handle and size but are transparent until Rtexture_update fills them
-- again, which is what this function is for. Textures that wrap objects of
-- the backend (raven_vulkan.h) must be made again, and the Vulkan handles
-- returned by Rvulkan_get_device... have changed.
--
-- Called on the UI thread, during Rdraw_frame / Rwait_frame. Set it after
-- Rinit; NULL removes it.
type RDeviceResetCallback = FunPtr (Ptr () -> IO ())

-- | Makes a 'RDeviceResetCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRDeviceResetCallback :: (Ptr () -> IO ()) -> IO RDeviceResetCallback

-- | Called for a drop target (a widget, or a window: `target` is its
-- handle). Return R_TRUE to accept: while the pointer is over the target this
-- shows the "can drop here" cursor and highlights the target; for
-- R_DROP_DROPPED it tells the source that the data was taken.
type RDropCallback = FunPtr (Rid -> Ptr RDropEvent -> Ptr () -> IO CInt)

-- | Makes a 'RDropCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRDropCallback :: (Rid -> Ptr RDropEvent -> Ptr () -> IO CInt) -> IO RDropCallback

-- | Called when the user asks to close the window.
-- Return R_TRUE to let the window close, R_FALSE to keep it open.
type RWindowCloseCallback = FunPtr (Rid -> Ptr () -> IO CInt)

-- | Makes a 'RWindowCloseCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRWindowCloseCallback :: (Rid -> Ptr () -> IO CInt) -> IO RWindowCloseCallback

-- | Called after the window was resized. Sizes are in logical units.
type RWindowResizeCallback = FunPtr (Rid -> CInt -> CInt -> Ptr () -> IO ())

-- | Makes a 'RWindowResizeCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRWindowResizeCallback :: (Rid -> CInt -> CInt -> Ptr () -> IO ()) -> IO RWindowResizeCallback

-- | Sees every input event of the window (mouse, wheel, keys, text)
-- before any widget, with `target` set to the widget under the pointer.
-- Returning R_TRUE consumes the event: the widgets do not get it. The
-- state read by Rwindow_is_key_down is updated either way.
type RWindowEventHook = FunPtr (Rid -> Ptr REvent -> Ptr () -> IO CInt)

-- | Makes a 'RWindowEventHook' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRWindowEventHook :: (Rid -> Ptr REvent -> Ptr () -> IO CInt) -> IO RWindowEventHook

-- | Return R_TRUE to take the finger: the library then neither clicks nor scrolls with it.
type RTouchCallback = FunPtr (Rid -> Ptr RTouchEvent -> Ptr () -> IO CInt)

-- | Makes a 'RTouchCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRTouchCallback :: (Rid -> Ptr RTouchEvent -> Ptr () -> IO CInt) -> IO RTouchCallback

-- | Return R_TRUE (on R_PEN_DOWN) to take the stroke: the pen does not act as a mouse until it is lifted.
type RPenCallback = FunPtr (Rid -> Ptr RPenEvent -> Ptr () -> IO CInt)

-- | Makes a 'RPenCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRPenCallback :: (Rid -> Ptr RPenEvent -> Ptr () -> IO CInt) -> IO RPenCallback

-- | Return R_TRUE (on R_GESTURE_BEGIN) to take the gesture: nothing zooms, pans or navigates by itself.
type RGestureCallback = FunPtr (Rid -> Ptr RGestureEvent -> Ptr () -> IO CInt)

-- | Makes a 'RGestureCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRGestureCallback :: (Rid -> Ptr RGestureEvent -> Ptr () -> IO CInt) -> IO RGestureCallback

-- | Called once when the widget is gone: deleted by the application,
-- with its parent or its window, or by Rshutdown. The place to release
-- what belonged to it (a texture, a connection). The handle it receives
-- names nothing any more: it is only there to tell widgets apart.
-- Called on the UI thread, shortly after the deletion.
type RWidgetDestroyCallback = FunPtr (Rid -> Ptr () -> IO ())

-- | Makes a 'RWidgetDestroyCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRWidgetDestroyCallback :: (Rid -> Ptr () -> IO ()) -> IO RWidgetDestroyCallback

-- | Receives the input events of a widget before the widget itself.
-- Return R_TRUE to consume the event (the widget and its parents will
-- not see it), R_FALSE to let it continue.
type RWidgetEventCallback = FunPtr (Rid -> Ptr REvent -> Ptr () -> IO CInt)

-- | Makes a 'RWidgetEventCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRWidgetEventCallback :: (Rid -> Ptr REvent -> Ptr () -> IO CInt) -> IO RWidgetEventCallback

type RTabChangedCallback = FunPtr (Rid -> CInt -> Ptr () -> IO ())

-- | Makes a 'RTabChangedCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRTabChangedCallback :: (Rid -> CInt -> Ptr () -> IO ()) -> IO RTabChangedCallback

-- | The user closes a tab (its button, a middle click). Return R_TRUE
-- to delete its page now, R_FALSE to keep it (the application may ask to
-- save, then delete the page itself with Rwidget_destroy).
type RTabCloseCallback = FunPtr (Rid -> CInt -> Rid -> Ptr () -> IO CInt)

-- | Makes a 'RTabCloseCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRTabCloseCallback :: (Rid -> CInt -> Rid -> Ptr () -> IO CInt) -> IO RTabCloseCallback

type RExpanderChangedCallback = FunPtr (Rid -> CInt -> Ptr () -> IO ())

-- | Makes a 'RExpanderChangedCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRExpanderChangedCallback :: (Rid -> CInt -> Ptr () -> IO ()) -> IO RExpanderChangedCallback

-- | Called when the user clicks a link of a Markdown label.
--
-- Without a callback, a link written [Save](action:file.save) fires the
-- action "file.save" (Raction_bind), and http, https and mailto links
-- open in the browser or the mail program of the user (Ropen_url).
type RLabelLinkCallback = FunPtr (Rid -> CString -> Ptr () -> IO ())

-- | Makes a 'RLabelLinkCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRLabelLinkCallback :: (Rid -> CString -> Ptr () -> IO ()) -> IO RLabelLinkCallback

type RButtonClickedCallback = FunPtr (Rid -> Ptr () -> IO ())

-- | Makes a 'RButtonClickedCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRButtonClickedCallback :: (Rid -> Ptr () -> IO ()) -> IO RButtonClickedCallback

type RCheckboxChangedCallback = FunPtr (Rid -> CInt -> Ptr () -> IO ())

-- | Makes a 'RCheckboxChangedCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRCheckboxChangedCallback :: (Rid -> CInt -> Ptr () -> IO ()) -> IO RCheckboxChangedCallback

type RSliderChangedCallback = FunPtr (Rid -> CFloat -> Ptr () -> IO ())

-- | Makes a 'RSliderChangedCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRSliderChangedCallback :: (Rid -> CFloat -> Ptr () -> IO ()) -> IO RSliderChangedCallback

type RTextChangedCallback = FunPtr (Rid -> CString -> Ptr () -> IO ())

-- | Makes a 'RTextChangedCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRTextChangedCallback :: (Rid -> CString -> Ptr () -> IO ()) -> IO RTextChangedCallback

type RSpinBoxChangedCallback = FunPtr (Rid -> CDouble -> Ptr () -> IO ())

-- | Makes a 'RSpinBoxChangedCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRSpinBoxChangedCallback :: (Rid -> CDouble -> Ptr () -> IO ()) -> IO RSpinBoxChangedCallback

type RComboBoxChangedCallback = FunPtr (Rid -> CInt -> Ptr () -> IO ())

-- | Makes a 'RComboBoxChangedCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRComboBoxChangedCallback :: (Rid -> CInt -> Ptr () -> IO ()) -> IO RComboBoxChangedCallback

-- | Virtual mode, for large or computed data: the list stores nothing and
-- asks for the text of the rows it draws. The returned string must stay
-- valid until the next call.
type RListItemTextCallback = FunPtr (Rid -> CInt -> Ptr () -> IO CString)

-- | Makes a 'RListItemTextCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRListItemTextCallback :: (Rid -> CInt -> Ptr () -> IO CString) -> IO RListItemTextCallback

-- | Colors the text of rows (a level, a verdict): the list asks for the color
-- of each row it draws, stored or virtual. Set r, g, b, a (0..1) and return
-- R_TRUE; R_FALSE (or alpha 0) keeps the color of the theme. The color is
-- kept on a selected or hovered row. NULL removes the callback.
type RListItemColorCallback = FunPtr (Rid -> CInt -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> Ptr () -> IO CInt)

-- | Makes a 'RListItemColorCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRListItemColorCallback :: (Rid -> CInt -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> Ptr () -> IO CInt) -> IO RListItemColorCallback

type RListEventCallback = FunPtr (Rid -> CInt -> Ptr () -> IO ())

-- | Makes a 'RListEventCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRListEventCallback :: (Rid -> CInt -> Ptr () -> IO ()) -> IO RListEventCallback

type RTreeEventCallback = FunPtr (Rid -> Rid -> Ptr () -> IO ())

-- | Makes a 'RTreeEventCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRTreeEventCallback :: (Rid -> Rid -> Ptr () -> IO ()) -> IO RTreeEventCallback

-- | Right click: `item` is the item under the pointer (selected
-- first), or R_INVALID_ID in the empty space; x, y in the window, for Rmenu_open.
type RTreeContextCallback = FunPtr (Rid -> Rid -> CFloat -> CFloat -> Ptr () -> IO ())

-- | Makes a 'RTreeContextCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRTreeContextCallback :: (Rid -> Rid -> CFloat -> CFloat -> Ptr () -> IO ()) -> IO RTreeContextCallback

type RMenuItemClickedCallback = FunPtr (Rid -> Ptr () -> IO ())

-- | Makes a 'RMenuItemClickedCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRMenuItemClickedCallback :: (Rid -> Ptr () -> IO ()) -> IO RMenuItemClickedCallback

-- | Fills `item` for tile `index`. Called on the UI thread, only for the tiles in view.
type RTileItemCallback = FunPtr (Rid -> CInt -> Ptr RTileItem -> Ptr () -> IO ())

-- | Makes a 'RTileItemCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRTileItemCallback :: (Rid -> CInt -> Ptr RTileItem -> Ptr () -> IO ()) -> IO RTileItemCallback

-- | Selection change (`index`: the current tile, or -1) and activation (double click, Enter).
type RTileEventCallback = FunPtr (Rid -> CInt -> Ptr () -> IO ())

-- | Makes a 'RTileEventCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRTileEventCallback :: (Rid -> CInt -> Ptr () -> IO ()) -> IO RTileEventCallback

-- | Right click: the tile under the pointer (selected first), or -1 in the empty space; x, y in the window.
type RTileContextCallback = FunPtr (Rid -> CInt -> CFloat -> CFloat -> Ptr () -> IO ())

-- | Makes a 'RTileContextCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRTileContextCallback :: (Rid -> CInt -> CFloat -> CFloat -> Ptr () -> IO ()) -> IO RTileContextCallback

-- | The data of a drag, given by the application (paths of files...)
-- instead of the indices: the dragged tiles are the selected ones
-- (Rtile_view_get_selection). The text returned is copied; NULL keeps
-- the indices.
type RTileDragDataCallback = FunPtr (Rid -> Ptr () -> IO CString)

-- | Makes a 'RTileDragDataCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRTileDragDataCallback :: (Rid -> Ptr () -> IO CString) -> IO RTileDragDataCallback

-- | Virtual mode, for large or computed data: the table stores nothing and
-- asks for the texts of the cells it draws. The returned string must stay
-- valid until the next call.
type RTableCellTextCallback = FunPtr (Rid -> CInt -> CInt -> Ptr () -> IO CString)

-- | Makes a 'RTableCellTextCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRTableCellTextCallback :: (Rid -> CInt -> CInt -> Ptr () -> IO CString) -> IO RTableCellTextCallback

-- | Colors the text of cells (a verdict, a state): the table asks for the
-- color of each cell it draws, stored or virtual (`row` is the index of the
-- order shown). Set r, g, b, a (0..1) and return R_TRUE; R_FALSE (or alpha 0)
-- keeps the color of the theme. The color is kept on a selected or hovered
-- row. NULL removes the callback.
type RTableCellColorCallback = FunPtr (Rid -> CInt -> CInt -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> Ptr () -> IO CInt)

-- | Makes a 'RTableCellColorCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRTableCellColorCallback :: (Rid -> CInt -> CInt -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> Ptr () -> IO CInt) -> IO RTableCellColorCallback

type RTableEventCallback = FunPtr (Rid -> CInt -> Ptr () -> IO ())

-- | Makes a 'RTableEventCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRTableEventCallback :: (Rid -> CInt -> Ptr () -> IO ()) -> IO RTableEventCallback

-- | Called when the user clicks a header; `column` -1: not sorted.
type RTableSortCallback = FunPtr (Rid -> CInt -> CInt -> Ptr () -> IO ())

-- | Makes a 'RTableSortCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRTableSortCallback :: (Rid -> CInt -> CInt -> Ptr () -> IO ()) -> IO RTableSortCallback

-- | Called when the user chooses a date. This also fires the action of the widget.
type RDateChangedCallback = FunPtr (Rid -> CInt -> CInt -> CInt -> Ptr () -> IO ())

-- | Makes a 'RDateChangedCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRDateChangedCallback :: (Rid -> CInt -> CInt -> CInt -> Ptr () -> IO ()) -> IO RDateChangedCallback

-- | Called when the user changes the time. This also fires the action of the widget.
type RTimeChangedCallback = FunPtr (Rid -> CInt -> CInt -> CInt -> Ptr () -> IO ())

-- | Makes a 'RTimeChangedCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRTimeChangedCallback :: (Rid -> CInt -> CInt -> CInt -> Ptr () -> IO ()) -> IO RTimeChangedCallback

-- | Called by the button of a message, which then hides. This also fires the action of the widget.
type RToastCallback = FunPtr (Rid -> Ptr () -> IO ())

-- | Makes a 'RToastCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRToastCallback :: (Rid -> Ptr () -> IO ()) -> IO RToastCallback

-- | Called when the user clicks an item. This also fires the action of the widget.
type RBreadcrumbCallback = FunPtr (Rid -> CInt -> Ptr () -> IO ())

-- | Makes a 'RBreadcrumbCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRBreadcrumbCallback :: (Rid -> CInt -> Ptr () -> IO ()) -> IO RBreadcrumbCallback

-- | Called when the user changes the score. This also fires the action of the widget.
type RRatingChangedCallback = FunPtr (Rid -> CFloat -> Ptr () -> IO ())

-- | Makes a 'RRatingChangedCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRRatingChangedCallback :: (Rid -> CFloat -> Ptr () -> IO ()) -> IO RRatingChangedCallback

-- | Called when the user goes to another page. This also fires the action of the widget.
type RPageChangedCallback = FunPtr (Rid -> CInt -> Ptr () -> IO ())

-- | Makes a 'RPageChangedCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRPageChangedCallback :: (Rid -> CInt -> Ptr () -> IO ()) -> IO RPageChangedCallback

-- | Called when the user moves, zooms or fits the view (Rplot_get_axis_range tells where it is).
type RPlotViewCallback = FunPtr (Rid -> Ptr () -> IO ())

-- | Makes a 'RPlotViewCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRPlotViewCallback :: (Rid -> Ptr () -> IO ()) -> IO RPlotViewCallback

-- | Called while the user drags a drag line. This also fires the action of the widget.
type RPlotDragLineCallback = FunPtr (Rid -> CInt -> CDouble -> Ptr () -> IO ())

-- | Makes a 'RPlotDragLineCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRPlotDragLineCallback :: (Rid -> CInt -> CDouble -> Ptr () -> IO ()) -> IO RPlotDragLineCallback

-- | Called when the user hides or shows a series from the legend.
type RPlotSeriesCallback = FunPtr (Rid -> CInt -> CInt -> Ptr () -> IO ())

-- | Makes a 'RPlotSeriesCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRPlotSeriesCallback :: (Rid -> CInt -> CInt -> Ptr () -> IO ()) -> IO RPlotSeriesCallback

-- | Called when the user zoomed (to zoom other editors the same, or to keep it for the next time).
type RTextEditorZoomCallback = FunPtr (Rid -> CFloat -> Ptr () -> IO ())

-- | Makes a 'RTextEditorZoomCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRTextEditorZoomCallback :: (Rid -> CFloat -> Ptr () -> IO ()) -> IO RTextEditorZoomCallback

-- | Called each time the editor gathers its proposals, with the
-- start of the word being typed: add your own with
-- Rtext_editor_add_completion (the fields of a structure, the answers of
-- a language server...). They are listed before the others.
type RCompletionCallback = FunPtr (Rid -> CString -> Ptr () -> IO ())

-- | Makes a 'RCompletionCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRCompletionCallback :: (Rid -> CString -> Ptr () -> IO ()) -> IO RCompletionCallback

-- | Called while the pointer moves over the text where no marker
-- is, with the position under it: the text returned (NULL or "" for
-- none) is shown as the tooltip, as a marker's message is (what a name
-- stands for, the value of a variable). The text must stay valid until
-- the next call.
type RTextEditorHoverCallback = FunPtr (Rid -> CInt -> CInt -> Ptr () -> IO CString)

-- | Makes a 'RTextEditorHoverCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRTextEditorHoverCallback :: (Rid -> CInt -> CInt -> Ptr () -> IO CString) -> IO RTextEditorHoverCallback

-- | Called after each change made by the user (typing, pasting,
-- undoing...) or by Rtext_editor_insert_text. The action of the widget
-- (OnChanged in XML) fires at the same time.
type RTextEditorChangedCallback = FunPtr (Rid -> Ptr () -> IO ())

-- | Makes a 'RTextEditorChangedCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRTextEditorChangedCallback :: (Rid -> Ptr () -> IO ()) -> IO RTextEditorChangedCallback

-- | Called when the tray icon is clicked (Rtray_set_clicked_callback).
type RTrayClickedCallback = FunPtr (Rid -> RMouseButton -> Ptr () -> IO ())

-- | Makes a 'RTrayClickedCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRTrayClickedCallback :: (Rid -> RMouseButton -> Ptr () -> IO ()) -> IO RTrayClickedCallback

-- | Called when the user closes a panel, before it is deleted.
-- Return R_FALSE to keep the panel.
type RDockCloseCallback = FunPtr (Rid -> Rid -> Ptr () -> IO CInt)

-- | Makes a 'RDockCloseCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRDockCloseCallback :: (Rid -> Rid -> Ptr () -> IO CInt) -> IO RDockCloseCallback

-- | Called after the user moved, resized, showed or closed a panel.
type RDockChangedCallback = FunPtr (Rid -> Ptr () -> IO ())

-- | Makes a 'RDockChangedCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRDockChangedCallback :: (Rid -> Ptr () -> IO ()) -> IO RDockChangedCallback

-- | Called when the user unfolds or folds the island. The action of the widget (OnChanged) fires too.
type RIslandChangedCallback = FunPtr (Rid -> CInt -> Ptr () -> IO ())

-- | Makes a 'RIslandChangedCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRIslandChangedCallback :: (Rid -> CInt -> Ptr () -> IO ()) -> IO RIslandChangedCallback

-- | Called each time the user changes the color: while dragging,
-- and when a code or a number is entered. The action of the widget
-- (OnChanged in XML) fires too.
type RColorChangedCallback = FunPtr (Rid -> CFloat -> CFloat -> CFloat -> CFloat -> Ptr () -> IO ())

-- | Makes a 'RColorChangedCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRColorChangedCallback :: (Rid -> CFloat -> CFloat -> CFloat -> CFloat -> Ptr () -> IO ()) -> IO RColorChangedCallback

-- | The playhead moved: scrubbed by the user, or running by itself.
type RTimelineTimeCallback = FunPtr (Rid -> CDouble -> Ptr () -> IO ())

-- | Makes a 'RTimelineTimeCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRTimelineTimeCallback :: (Rid -> CDouble -> Ptr () -> IO ()) -> IO RTimelineTimeCallback

-- | Something happened to an item; which thing depends on the function the callback was given to.
type RTimelineItemCallback = FunPtr (Rid -> CInt -> Ptr () -> IO ())

-- | Makes a 'RTimelineItemCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRTimelineItemCallback :: (Rid -> CInt -> Ptr () -> IO ()) -> IO RTimelineItemCallback

-- | Asked when the user is about to link two pins that may be
-- linked: return R_FALSE to refuse (a loop, a rule of the application).
type RNodeGraphConnectCallback = FunPtr (Rid -> CInt -> CInt -> Ptr () -> IO CInt)

-- | Makes a 'RNodeGraphConnectCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRNodeGraphConnectCallback :: (Rid -> CInt -> CInt -> Ptr () -> IO CInt) -> IO RNodeGraphConnectCallback

-- | Something happened to a link or to a node; which thing depends on the function the callback was given to.
type RNodeGraphItemCallback = FunPtr (Rid -> CInt -> Ptr () -> IO ())

-- | Makes a 'RNodeGraphItemCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRNodeGraphItemCallback :: (Rid -> CInt -> Ptr () -> IO ()) -> IO RNodeGraphItemCallback

-- | Right click: `node` is the node under the pointer (selected first),
-- or 0 on the empty canvas; x, y are in the widget (Rnode_graph_to_canvas).
-- Without this callback the right button does nothing.
type RNodeGraphContextCallback = FunPtr (Rid -> CInt -> CFloat -> CFloat -> Ptr () -> IO ())

-- | Makes a 'RNodeGraphContextCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRNodeGraphContextCallback :: (Rid -> CInt -> CFloat -> CFloat -> Ptr () -> IO ()) -> IO RNodeGraphContextCallback

-- | Something happened at a point of the canvas: `item` depends on the
-- function the callback was given to; x, y are in canvas units (as node positions).
type RNodeGraphPointCallback = FunPtr (Rid -> CInt -> CFloat -> CFloat -> Ptr () -> IO ())

-- | Makes a 'RNodeGraphPointCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRNodeGraphPointCallback :: (Rid -> CInt -> CFloat -> CFloat -> Ptr () -> IO ()) -> IO RNodeGraphPointCallback

-- | A block changed (0: several, or one was deleted), or a script was clicked (its top block).
type RBlockEditorCallback = FunPtr (Rid -> CInt -> Ptr () -> IO ())

-- | Makes a 'RBlockEditorCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRBlockEditorCallback :: (Rid -> CInt -> Ptr () -> IO ()) -> IO RBlockEditorCallback

-- | Right click: on a block (0 on the canvas), at x, y in the widget.
type RBlockEditorContextCallback = FunPtr (Rid -> CInt -> CFloat -> CFloat -> Ptr () -> IO ())

-- | Makes a 'RBlockEditorContextCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRBlockEditorContextCallback :: (Rid -> CInt -> CFloat -> CFloat -> Ptr () -> IO ()) -> IO RBlockEditorContextCallback

-- | Called when a dialog closes. `result` is the value given to Rdialog_close, 0 when dismissed.
type RDialogClosedCallback = FunPtr (Rid -> CInt -> Ptr () -> IO ())

-- | Makes a 'RDialogClosedCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRDialogClosedCallback :: (Rid -> CInt -> Ptr () -> IO ()) -> IO RDialogClosedCallback

-- | Called when the user chooses: a double click or Enter on a file,
-- the Open / Save button. `path` is the first chosen path; with several
-- files, read them with Rfile_browser_get_selection. The action of the
-- widget (OnActivated in XML) fires at the same time.
type RFileBrowserCallback = FunPtr (Rid -> CString -> Ptr () -> IO ())

-- | Makes a 'RFileBrowserCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRFileBrowserCallback :: (Rid -> CString -> Ptr () -> IO ()) -> IO RFileBrowserCallback

type RNotificationFallback = FunPtr (CString -> CString -> RNotificationUrgency -> Ptr () -> IO ())

-- | Makes a 'RNotificationFallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRNotificationFallback :: (CString -> CString -> RNotificationUrgency -> Ptr () -> IO ()) -> IO RNotificationFallback

type RNotificationClickedCallback = FunPtr (Ptr () -> IO ())

-- | Makes a 'RNotificationClickedCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRNotificationClickedCallback :: (Ptr () -> IO ()) -> IO RNotificationClickedCallback

-- | Called on the render thread each time the window is rendered.
-- State you bind (pipeline, descriptor sets, viewport, scissor) does not
-- need to be restored. NULL removes the callback.
type RViewportDrawCallback = FunPtr (Ptr RViewportDrawInfo -> Ptr () -> IO ())

-- | Makes a 'RViewportDrawCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRViewportDrawCallback :: (Ptr RViewportDrawInfo -> Ptr () -> IO ()) -> IO RViewportDrawCallback

-- | Called on the UI thread when the pixel size changes: resize your render target here.
type RViewportResizeCallback = FunPtr (Rid -> CInt -> CInt -> Ptr () -> IO ())

-- | Makes a 'RViewportResizeCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRViewportResizeCallback :: (Rid -> CInt -> CInt -> Ptr () -> IO ()) -> IO RViewportResizeCallback

type RPaintCallback = FunPtr (Rid -> Ptr RPainter -> CFloat -> CFloat -> Ptr () -> IO ())

-- | Makes a 'RPaintCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRPaintCallback :: (Rid -> Ptr RPainter -> CFloat -> CFloat -> Ptr () -> IO ()) -> IO RPaintCallback

type RMeasureCallback = FunPtr (Rid -> Ptr CFloat -> Ptr CFloat -> Ptr () -> IO ())

-- | Makes a 'RMeasureCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRMeasureCallback :: (Rid -> Ptr CFloat -> Ptr CFloat -> Ptr () -> IO ()) -> IO RMeasureCallback

-- | Called each time a file was loaded again.
-- path The file given when the interface was created.
-- ok   R_FALSE when the file could not be used (the interface is unchanged).
--
-- Widgets that had to be built again have new handles: this is the
-- place to look them up again (Rwidget_find).
type RXmlReloadCallback = FunPtr (CString -> CInt -> Ptr () -> IO ())

-- | Makes a 'RXmlReloadCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRXmlReloadCallback :: (CString -> CInt -> Ptr () -> IO ()) -> IO RXmlReloadCallback

-- | Tags made by code. A library or an application that builds a widget of
-- its own (a R_WIDGET_CUSTOM with its callbacks, or an assembly of
-- widgets) can give it a tag: documents then write <MyWidget .../> as
-- they write <Button/>.
--
-- `create` makes the widget and returns it. `set_attribute` (may be
-- NULL) receives the attributes of the element before Raven looks at
-- them: it returns R_TRUE for those it understands, R_FALSE to leave
-- them to the widget (Name, Width, Slot..., and what its type knows).
-- Rwidget_set_attribute goes through it too.
type RXmlTagCreateCallback = FunPtr (CString -> Ptr () -> IO Rid)

-- | Makes a 'RXmlTagCreateCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRXmlTagCreateCallback :: (CString -> Ptr () -> IO Rid) -> IO RXmlTagCreateCallback

type RXmlTagAttributeCallback = FunPtr (Rid -> CString -> CString -> Ptr () -> IO CInt)

-- | Makes a 'RXmlTagAttributeCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRXmlTagAttributeCallback :: (Rid -> CString -> CString -> Ptr () -> IO CInt) -> IO RXmlTagAttributeCallback

type RLocaleChangedCallback = FunPtr (CString -> Ptr () -> IO ())

-- | Makes a 'RLocaleChangedCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRLocaleChangedCallback :: (CString -> Ptr () -> IO ()) -> IO RLocaleChangedCallback

type RAccessiblePressCallback = FunPtr (Rid -> Ptr () -> IO ())

-- | Makes a 'RAccessiblePressCallback' of a Haskell function; free it with 'freeHaskellFunPtr' once the library
-- no longer calls it.
foreign import ccall "wrapper" mkRAccessiblePressCallback :: (Rid -> Ptr () -> IO ()) -> IO RAccessiblePressCallback

-- | Optional settings for Rinit_ex. A zero-initialized structure selects the
-- default for every field, so new fields can be added without breaking code.
data RInitDesc = RInitDesc
    { rInitDesc_backend :: RBackend
      -- ^ Rendering backend. 0 selects WebGL in browser builds, Vulkan otherwise.
    , rInitDesc_app_name :: CString
      -- ^ Application name reported to the graphics driver. May be NULL.
    , rInitDesc_worker_threads :: CInt
      -- ^ Worker threads of the job system. 0 = one per spare CPU core, negative = none.
    , rInitDesc_disable_render_thread :: CInt
      -- ^ R_TRUE renders on the UI thread instead of a dedicated render thread.
    , rInitDesc_disable_vsync :: CInt
      -- ^ R_TRUE presents without waiting for the vertical blank.
    , rInitDesc_enable_validation :: CInt
      -- ^ R_TRUE enables the backend debug layer (Vulkan validation) when installed.
    , rInitDesc_headless :: CInt
      -- ^ R_TRUE opens no OS window (null platform). Combine with R_BACKEND_NULL for tests.
    , rInitDesc_ui_scale :: CFloat
      -- ^ Global UI scale multiplier applied on top of the monitor scale. 0 = 1.0.
    } deriving (Eq, Show)

-- | The zeroed structure (what @RInitDesc x = {0};@ is in C).
rInitDesc :: RInitDesc
rInitDesc = RInitDesc (0) (nullPtr) (0) (0) (0) (0) (0) (0)

instance Storable RInitDesc where
    sizeOf _ = 40
    alignment _ = 8
    peek p = RInitDesc
        <$> peekByteOff p 0
        <*> peekByteOff p 8
        <*> peekByteOff p 16
        <*> peekByteOff p 20
        <*> peekByteOff p 24
        <*> peekByteOff p 28
        <*> peekByteOff p 32
        <*> peekByteOff p 36
    poke p v = do
        pokeByteOff p 0 (rInitDesc_backend v)
        pokeByteOff p 8 (rInitDesc_app_name v)
        pokeByteOff p 16 (rInitDesc_worker_threads v)
        pokeByteOff p 20 (rInitDesc_disable_render_thread v)
        pokeByteOff p 24 (rInitDesc_disable_vsync v)
        pokeByteOff p 28 (rInitDesc_enable_validation v)
        pokeByteOff p 32 (rInitDesc_headless v)
        pokeByteOff p 36 (rInitDesc_ui_scale v)

-- | Memory functions of the application, to replace the allocator of Raven. See Rset_allocator.
data RAllocator = RAllocator
    { rAllocator_alloc :: FunPtr ()
    , rAllocator_realloc :: FunPtr ()
    , rAllocator_free :: FunPtr ()
    , rAllocator_user_data :: Ptr ()
    } deriving (Eq, Show)

-- | The zeroed structure (what @RAllocator x = {0};@ is in C).
rAllocator :: RAllocator
rAllocator = RAllocator (nullFunPtr) (nullFunPtr) (nullFunPtr) (nullPtr)

instance Storable RAllocator where
    sizeOf _ = 32
    alignment _ = 8
    peek p = RAllocator
        <$> peekByteOff p 0
        <*> peekByteOff p 8
        <*> peekByteOff p 16
        <*> peekByteOff p 24
    poke p v = do
        pokeByteOff p 0 (rAllocator_alloc v)
        pokeByteOff p 8 (rAllocator_realloc v)
        pokeByteOff p 16 (rAllocator_free v)
        pokeByteOff p 24 (rAllocator_user_data v)

-- | What the library holds. See Rmemory_get_stats.
data RMemoryStats = RMemoryStats
    { rMemoryStats_used_bytes :: CSize
      -- ^ Bytes in the blocks currently allocated. 0 with the allocator of the application.
    , rMemoryStats_peak_bytes :: CSize
      -- ^ Highest value of used_bytes since the start.
    , rMemoryStats_reserved_bytes :: CSize
      -- ^ Bytes taken from the OS: used_bytes, plus what is free in the heap.
    , rMemoryStats_allocation_count :: CSize
      -- ^ Blocks currently allocated. 0 after Rshutdown: anything else is a leak of the library.
    , rMemoryStats_total_allocations :: Word64
      -- ^ Allocations made since the start.
    } deriving (Eq, Show)

-- | The zeroed structure (what @RMemoryStats x = {0};@ is in C).
rMemoryStats :: RMemoryStats
rMemoryStats = RMemoryStats (0) (0) (0) (0) (0)

instance Storable RMemoryStats where
    sizeOf _ = 40
    alignment _ = 8
    peek p = RMemoryStats
        <$> peekByteOff p 0
        <*> peekByteOff p 8
        <*> peekByteOff p 16
        <*> peekByteOff p 24
        <*> peekByteOff p 32
    poke p v = do
        pokeByteOff p 0 (rMemoryStats_used_bytes v)
        pokeByteOff p 8 (rMemoryStats_peak_bytes v)
        pokeByteOff p 16 (rMemoryStats_reserved_bytes v)
        pokeByteOff p 24 (rMemoryStats_allocation_count v)
        pokeByteOff p 32 (rMemoryStats_total_allocations v)

-- | Timings and counters of the last frame produced for a window.
data RFrameStats = RFrameStats
    { rFrameStats_layout_ms :: CDouble
    , rFrameStats_paint_ms :: CDouble
    , rFrameStats_render_ms :: CDouble
    , rFrameStats_widget_count :: Word32
    , rFrameStats_instance_count :: Word32
    , rFrameStats_draw_call_count :: Word32
    , rFrameStats_backdrop_count :: Word32
      -- ^ Glass: times the frame was copied and blurred to be seen through (0 without glass).
    , rFrameStats_paint_job_count :: Word32
    , rFrameStats_frame_index :: Word64
    } deriving (Eq, Show)

-- | The zeroed structure (what @RFrameStats x = {0};@ is in C).
rFrameStats :: RFrameStats
rFrameStats = RFrameStats (0) (0) (0) (0) (0) (0) (0) (0) (0)

instance Storable RFrameStats where
    sizeOf _ = 56
    alignment _ = 8
    peek p = RFrameStats
        <$> peekByteOff p 0
        <*> peekByteOff p 8
        <*> peekByteOff p 16
        <*> peekByteOff p 24
        <*> peekByteOff p 28
        <*> peekByteOff p 32
        <*> peekByteOff p 36
        <*> peekByteOff p 40
        <*> peekByteOff p 48
    poke p v = do
        pokeByteOff p 0 (rFrameStats_layout_ms v)
        pokeByteOff p 8 (rFrameStats_paint_ms v)
        pokeByteOff p 16 (rFrameStats_render_ms v)
        pokeByteOff p 24 (rFrameStats_widget_count v)
        pokeByteOff p 28 (rFrameStats_instance_count v)
        pokeByteOff p 32 (rFrameStats_draw_call_count v)
        pokeByteOff p 36 (rFrameStats_backdrop_count v)
        pokeByteOff p 40 (rFrameStats_paint_job_count v)
        pokeByteOff p 48 (rFrameStats_frame_index v)

-- | Native handles of a window or of a native viewport.
--   Win32 : display = HINSTANCE, window = HWND
--   X11   : display = Display*,  window = Window (XID), visual = VisualID
--   Wayland: display = wl_display*, window = wl_surface* (Linux and BSD choose
--           Wayland or X11 when Rinit runs: check `system`)
--   Cocoa : display = NSWindow*, window = CAMetalLayer* for Vulkan presentation
--   Emscripten: window = opaque browser surface token (no OS window handle)
data RNativeWindow = RNativeWindow
    { rNativeWindow_system :: RWindowSystem
    , rNativeWindow_display :: Ptr ()
    , rNativeWindow_window :: WordPtr
    , rNativeWindow_visual :: WordPtr
    , rNativeWindow_transparent :: CInt
      -- ^ R_TRUE when the window was created with R_WINDOW_TRANSPARENT and the
      -- window system gave it an alpha channel: what is presented in it must
      -- carry alpha (premultiplied), which a rendering backend asks for when
      -- it creates its surface.
    } deriving (Eq, Show)

-- | The zeroed structure (what @RNativeWindow x = {0};@ is in C).
rNativeWindow :: RNativeWindow
rNativeWindow = RNativeWindow (0) (nullPtr) (0) (0) (0)

instance Storable RNativeWindow where
    sizeOf _ = 40
    alignment _ = 8
    peek p = RNativeWindow
        <$> peekByteOff p 0
        <*> peekByteOff p 8
        <*> peekByteOff p 16
        <*> peekByteOff p 24
        <*> peekByteOff p 32
    poke p v = do
        pokeByteOff p 0 (rNativeWindow_system v)
        pokeByteOff p 8 (rNativeWindow_display v)
        pokeByteOff p 16 (rNativeWindow_window v)
        pokeByteOff p 24 (rNativeWindow_visual v)
        pokeByteOff p 32 (rNativeWindow_transparent v)

-- | Input event delivered to a widget. Mouse and key events bubble from the
-- target widget to its parents until one of them handles the event.
data REvent = REvent
    { rEvent_type :: REventType
    , rEvent_window :: Rid
    , rEvent_target :: Rid
      -- ^ Widget the event was first sent to.
    , rEvent_x :: CFloat
      -- ^ Cursor position in the window, in logical units.
    , rEvent_y :: CFloat
      -- ^ Cursor position in the window, in logical units.
    , rEvent_local_x :: CFloat
      -- ^ Cursor position relative to the widget receiving the event.
    , rEvent_local_y :: CFloat
      -- ^ Cursor position relative to the widget receiving the event.
    , rEvent_delta_x :: CFloat
      -- ^ Mouse movement since the previous event (R_EVENT_MOUSE_MOVE).
    , rEvent_delta_y :: CFloat
      -- ^ Mouse movement since the previous event (R_EVENT_MOUSE_MOVE).
    , rEvent_wheel_x :: CFloat
      -- ^ Scroll amount in lines, positive = up / left (R_EVENT_MOUSE_WHEEL).
    , rEvent_wheel_y :: CFloat
      -- ^ Scroll amount in lines, positive = up / left (R_EVENT_MOUSE_WHEEL).
    , rEvent_button :: RMouseButton
    , rEvent_clicks :: CInt
      -- ^ 1 = single click, 2 = double click, ... (R_EVENT_MOUSE_DOWN).
    , rEvent_key :: RKey
    , rEvent_modifiers :: Word32
      -- ^ Bitmask of RModifier.
    , rEvent_repeat :: CInt
      -- ^ R_TRUE when the key event comes from auto-repeat.
    , rEvent_codepoint :: Word32
    , rEvent_wheel_precise :: CInt
      -- ^ R_EVENT_MOUSE_WHEEL: R_TRUE when the wheel comes from a trackpad (or a
      -- mouse that scrolls smoothly). wheel_x, wheel_y are then small fractions of
      -- a line, a line being 48 logical units the fingers moved: what follows the
      -- fingers moves 48 units per line, and a canvas pans rather than zooms.
    } deriving (Eq, Show)

-- | The zeroed structure (what @REvent x = {0};@ is in C).
rEvent :: REvent
rEvent = REvent (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0)

instance Storable REvent where
    sizeOf _ = 72
    alignment _ = 4
    peek p = REvent
        <$> peekByteOff p 0
        <*> peekByteOff p 4
        <*> peekByteOff p 8
        <*> peekByteOff p 12
        <*> peekByteOff p 16
        <*> peekByteOff p 20
        <*> peekByteOff p 24
        <*> peekByteOff p 28
        <*> peekByteOff p 32
        <*> peekByteOff p 36
        <*> peekByteOff p 40
        <*> peekByteOff p 44
        <*> peekByteOff p 48
        <*> peekByteOff p 52
        <*> peekByteOff p 56
        <*> peekByteOff p 60
        <*> peekByteOff p 64
        <*> peekByteOff p 68
    poke p v = do
        pokeByteOff p 0 (rEvent_type v)
        pokeByteOff p 4 (rEvent_window v)
        pokeByteOff p 8 (rEvent_target v)
        pokeByteOff p 12 (rEvent_x v)
        pokeByteOff p 16 (rEvent_y v)
        pokeByteOff p 20 (rEvent_local_x v)
        pokeByteOff p 24 (rEvent_local_y v)
        pokeByteOff p 28 (rEvent_delta_x v)
        pokeByteOff p 32 (rEvent_delta_y v)
        pokeByteOff p 36 (rEvent_wheel_x v)
        pokeByteOff p 40 (rEvent_wheel_y v)
        pokeByteOff p 44 (rEvent_button v)
        pokeByteOff p 48 (rEvent_clicks v)
        pokeByteOff p 52 (rEvent_key v)
        pokeByteOff p 56 (rEvent_modifiers v)
        pokeByteOff p 60 (rEvent_repeat v)
        pokeByteOff p 64 (rEvent_codepoint v)
        pokeByteOff p 68 (rEvent_wheel_precise v)

-- | What a drop target receives. The files and the text are only known when
-- the drop happens (R_DROP_DROPPED), and are valid during the callback.
data RDropEvent = RDropEvent
    { rDropEvent_phase :: RDropPhase
    , rDropEvent_window :: Rid
    , rDropEvent_x :: CFloat
      -- ^ Pointer position in the window, and relative to the target widget.
    , rDropEvent_y :: CFloat
      -- ^ Pointer position in the window, and relative to the target widget.
    , rDropEvent_local_x :: CFloat
    , rDropEvent_local_y :: CFloat
    , rDropEvent_paths :: Ptr CString
      -- ^ Dropped files and folders, as UTF-8 paths.
    , rDropEvent_path_count :: CInt
    , rDropEvent_text :: CString
      -- ^ Dropped text when it is not files, or NULL. For a drag inside the application: its data.
    , rDropEvent_drag_type :: CString
      -- ^ Drag inside the application (Rdrag_begin): its type, else NULL. Set in every phase.
    , rDropEvent_drag_source :: Rid
      -- ^ Widget the drag started from, or R_INVALID_ID.
    } deriving (Eq, Show)

-- | The zeroed structure (what @RDropEvent x = {0};@ is in C).
rDropEvent :: RDropEvent
rDropEvent = RDropEvent (0) (0) (0) (0) (0) (0) (nullPtr) (0) (nullPtr) (nullPtr) (0)

instance Storable RDropEvent where
    sizeOf _ = 64
    alignment _ = 8
    peek p = RDropEvent
        <$> peekByteOff p 0
        <*> peekByteOff p 4
        <*> peekByteOff p 8
        <*> peekByteOff p 12
        <*> peekByteOff p 16
        <*> peekByteOff p 20
        <*> peekByteOff p 24
        <*> peekByteOff p 32
        <*> peekByteOff p 40
        <*> peekByteOff p 48
        <*> peekByteOff p 56
    poke p v = do
        pokeByteOff p 0 (rDropEvent_phase v)
        pokeByteOff p 4 (rDropEvent_window v)
        pokeByteOff p 8 (rDropEvent_x v)
        pokeByteOff p 12 (rDropEvent_y v)
        pokeByteOff p 16 (rDropEvent_local_x v)
        pokeByteOff p 20 (rDropEvent_local_y v)
        pokeByteOff p 24 (rDropEvent_paths v)
        pokeByteOff p 32 (rDropEvent_path_count v)
        pokeByteOff p 40 (rDropEvent_text v)
        pokeByteOff p 48 (rDropEvent_drag_type v)
        pokeByteOff p 56 (rDropEvent_drag_source v)

-- | How the library treats fingers and pens (Rtouch_set_settings, Rwindow_set_touch_settings).
data RTouchSettings = RTouchSettings
    { rTouchSettings_mouse_emulation :: CInt
      -- ^ R_TRUE (default): one finger drives the widgets as the mouse does: a tap clicks, a drag drags.
    , rTouchSettings_slop :: CFloat
      -- ^ How far a finger moves (logical units) before a press becomes a drag or a scroll. Default 8.
    , rTouchSettings_long_press :: CFloat
      -- ^ Seconds a finger rests before the press is a right click (the context menu); 0 turns it off. Default 0.5.
    , rTouchSettings_inertia :: CInt
      -- ^ R_TRUE (default): flicked content keeps going and slows down.
    , rTouchSettings_deceleration :: CFloat
      -- ^ How fast it slows down, in logical units per second per second. Default 2400.
    , rTouchSettings_gestures :: CInt
      -- ^ R_TRUE (default): two fingers pinch to zoom and pan.
    , rTouchSettings_pen_mouse_emulation :: CInt
      -- ^ R_TRUE (default): the pen drives the widgets as the mouse does (its barrel button: the right button).
    , rTouchSettings_pressure_curve :: CFloat
      -- ^ The pressure given to the application is the one of the pen to this power: 1 (default) as it is, below 1 softer, above 1 harder.
    , rTouchSettings_pressure_threshold :: CFloat
      -- ^ Pressure (0..1, after the curve) the tip needs before it presses. Default 0.
    , rTouchSettings_palm_rejection :: CInt
      -- ^ R_TRUE (default): fingers are ignored while a pen is near the screen, and half a second after (the palm resting on it).
    , rTouchSettings_trackpad_gestures :: CInt
      -- ^ R_TRUE (default): the pinch and the rotation of a trackpad zoom what zooms, as two fingers on the screen do.
    , rTouchSettings_swipe_navigation :: CInt
      -- ^ R_TRUE (default): three or more fingers swiped across a trackpad go back or forward (the side buttons of a mouse).
    , rTouchSettings_swipe_distance :: CFloat
      -- ^ How far (logical units) the fingers of a swipe go across before it navigates. Default 120.
    } deriving (Eq, Show)

-- | The zeroed structure (what @RTouchSettings x = {0};@ is in C).
rTouchSettings :: RTouchSettings
rTouchSettings = RTouchSettings (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0)

instance Storable RTouchSettings where
    sizeOf _ = 52
    alignment _ = 4
    peek p = RTouchSettings
        <$> peekByteOff p 0
        <*> peekByteOff p 4
        <*> peekByteOff p 8
        <*> peekByteOff p 12
        <*> peekByteOff p 16
        <*> peekByteOff p 20
        <*> peekByteOff p 24
        <*> peekByteOff p 28
        <*> peekByteOff p 32
        <*> peekByteOff p 36
        <*> peekByteOff p 40
        <*> peekByteOff p 44
        <*> peekByteOff p 48
    poke p v = do
        pokeByteOff p 0 (rTouchSettings_mouse_emulation v)
        pokeByteOff p 4 (rTouchSettings_slop v)
        pokeByteOff p 8 (rTouchSettings_long_press v)
        pokeByteOff p 12 (rTouchSettings_inertia v)
        pokeByteOff p 16 (rTouchSettings_deceleration v)
        pokeByteOff p 20 (rTouchSettings_gestures v)
        pokeByteOff p 24 (rTouchSettings_pen_mouse_emulation v)
        pokeByteOff p 28 (rTouchSettings_pressure_curve v)
        pokeByteOff p 32 (rTouchSettings_pressure_threshold v)
        pokeByteOff p 36 (rTouchSettings_palm_rejection v)
        pokeByteOff p 40 (rTouchSettings_trackpad_gestures v)
        pokeByteOff p 44 (rTouchSettings_swipe_navigation v)
        pokeByteOff p 48 (rTouchSettings_swipe_distance v)

data RTouchEvent = RTouchEvent
    { rTouchEvent_phase :: RTouchPhase
    , rTouchEvent_window :: Rid
    , rTouchEvent_target :: Rid
      -- ^ Widget the finger came down on.
    , rTouchEvent_id :: CInt
      -- ^ Number of the finger, the same from its down to its up.
    , rTouchEvent_x :: CFloat
      -- ^ Position in the window, and relative to the widget receiving the event.
    , rTouchEvent_y :: CFloat
      -- ^ Position in the window, and relative to the widget receiving the event.
    , rTouchEvent_local_x :: CFloat
    , rTouchEvent_local_y :: CFloat
    , rTouchEvent_pressure :: CFloat
      -- ^ 0..1; 1 when the screen does not tell.
    , rTouchEvent_width :: CFloat
      -- ^ Size of the contact, 0 when the screen does not tell.
    , rTouchEvent_height :: CFloat
      -- ^ Size of the contact, 0 when the screen does not tell.
    , rTouchEvent_count :: CInt
      -- ^ Fingers on the window, this one included (0 after the last up).
    , rTouchEvent_modifiers :: Word32
      -- ^ Bitmask of RModifier.
    } deriving (Eq, Show)

-- | The zeroed structure (what @RTouchEvent x = {0};@ is in C).
rTouchEvent :: RTouchEvent
rTouchEvent = RTouchEvent (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0)

instance Storable RTouchEvent where
    sizeOf _ = 52
    alignment _ = 4
    peek p = RTouchEvent
        <$> peekByteOff p 0
        <*> peekByteOff p 4
        <*> peekByteOff p 8
        <*> peekByteOff p 12
        <*> peekByteOff p 16
        <*> peekByteOff p 20
        <*> peekByteOff p 24
        <*> peekByteOff p 28
        <*> peekByteOff p 32
        <*> peekByteOff p 36
        <*> peekByteOff p 40
        <*> peekByteOff p 44
        <*> peekByteOff p 48
    poke p v = do
        pokeByteOff p 0 (rTouchEvent_phase v)
        pokeByteOff p 4 (rTouchEvent_window v)
        pokeByteOff p 8 (rTouchEvent_target v)
        pokeByteOff p 12 (rTouchEvent_id v)
        pokeByteOff p 16 (rTouchEvent_x v)
        pokeByteOff p 20 (rTouchEvent_y v)
        pokeByteOff p 24 (rTouchEvent_local_x v)
        pokeByteOff p 28 (rTouchEvent_local_y v)
        pokeByteOff p 32 (rTouchEvent_pressure v)
        pokeByteOff p 36 (rTouchEvent_width v)
        pokeByteOff p 40 (rTouchEvent_height v)
        pokeByteOff p 44 (rTouchEvent_count v)
        pokeByteOff p 48 (rTouchEvent_modifiers v)

data RPenEvent = RPenEvent
    { rPenEvent_phase :: RPenPhase
    , rPenEvent_window :: Rid
    , rPenEvent_target :: Rid
      -- ^ Widget under the pen; during a stroke, the one it started on.
    , rPenEvent_x :: CFloat
    , rPenEvent_y :: CFloat
    , rPenEvent_local_x :: CFloat
    , rPenEvent_local_y :: CFloat
    , rPenEvent_pressure :: CFloat
      -- ^ 0..1, after the pressure curve; 0 while hovering.
    , rPenEvent_raw_pressure :: CFloat
      -- ^ As the pen gives it, 0..1.
    , rPenEvent_tilt_x :: CFloat
      -- ^ Tilt of the pen in degrees, -90..90, toward the right and toward the user; 0 when upright or unknown.
    , rPenEvent_tilt_y :: CFloat
      -- ^ Tilt of the pen in degrees, -90..90, toward the right and toward the user; 0 when upright or unknown.
    , rPenEvent_rotation :: CFloat
      -- ^ Rotation around its axis in degrees, 0..360 (when the pen tells).
    , rPenEvent_eraser :: CInt
      -- ^ R_TRUE when the eraser end touches (or the eraser button is held).
    , rPenEvent_buttons :: Word32
      -- ^ Bitmask of RPenButton.
    , rPenEvent_modifiers :: Word32
    } deriving (Eq, Show)

-- | The zeroed structure (what @RPenEvent x = {0};@ is in C).
rPenEvent :: RPenEvent
rPenEvent = RPenEvent (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0)

instance Storable RPenEvent where
    sizeOf _ = 60
    alignment _ = 4
    peek p = RPenEvent
        <$> peekByteOff p 0
        <*> peekByteOff p 4
        <*> peekByteOff p 8
        <*> peekByteOff p 12
        <*> peekByteOff p 16
        <*> peekByteOff p 20
        <*> peekByteOff p 24
        <*> peekByteOff p 28
        <*> peekByteOff p 32
        <*> peekByteOff p 36
        <*> peekByteOff p 40
        <*> peekByteOff p 44
        <*> peekByteOff p 48
        <*> peekByteOff p 52
        <*> peekByteOff p 56
    poke p v = do
        pokeByteOff p 0 (rPenEvent_phase v)
        pokeByteOff p 4 (rPenEvent_window v)
        pokeByteOff p 8 (rPenEvent_target v)
        pokeByteOff p 12 (rPenEvent_x v)
        pokeByteOff p 16 (rPenEvent_y v)
        pokeByteOff p 20 (rPenEvent_local_x v)
        pokeByteOff p 24 (rPenEvent_local_y v)
        pokeByteOff p 28 (rPenEvent_pressure v)
        pokeByteOff p 32 (rPenEvent_raw_pressure v)
        pokeByteOff p 36 (rPenEvent_tilt_x v)
        pokeByteOff p 40 (rPenEvent_tilt_y v)
        pokeByteOff p 44 (rPenEvent_rotation v)
        pokeByteOff p 48 (rPenEvent_eraser v)
        pokeByteOff p 52 (rPenEvent_buttons v)
        pokeByteOff p 56 (rPenEvent_modifiers v)

data RGestureEvent = RGestureEvent
    { rGestureEvent_phase :: RGesturePhase
    , rGestureEvent_window :: Rid
    , rGestureEvent_target :: Rid
      -- ^ Widget under the center of the fingers when the gesture began.
    , rGestureEvent_x :: CFloat
      -- ^ Center of the fingers, in the window and relative to the widget receiving the event.
    , rGestureEvent_y :: CFloat
      -- ^ Center of the fingers, in the window and relative to the widget receiving the event.
    , rGestureEvent_local_x :: CFloat
    , rGestureEvent_local_y :: CFloat
    , rGestureEvent_scale :: CFloat
      -- ^ Distance between the fingers relative to when the gesture began (2 = twice as far apart).
    , rGestureEvent_scale_delta :: CFloat
      -- ^ The same since the previous event: what a zoom multiplies by.
    , rGestureEvent_pan_x :: CFloat
      -- ^ Movement of the center since the previous event.
    , rGestureEvent_pan_y :: CFloat
      -- ^ Movement of the center since the previous event.
    , rGestureEvent_rotation :: CFloat
      -- ^ Turn of the fingers since the gesture began, in degrees, clockwise.
    , rGestureEvent_kind :: RGestureKind
      -- ^ R_GESTURE_PINCH or R_GESTURE_SWIPE.
    , rGestureEvent_device :: RGestureDevice
      -- ^ R_GESTURE_DEVICE_TOUCHSCREEN or R_GESTURE_DEVICE_TRACKPAD.
    , rGestureEvent_fingers :: CInt
      -- ^ Fingers of the gesture (2 for a pinch, 3 or more for a swipe; 0 when the system does not tell).
    , rGestureEvent_total_x :: CFloat
      -- ^ Movement of the center since the gesture began.
    , rGestureEvent_total_y :: CFloat
      -- ^ Movement of the center since the gesture began.
    , rGestureEvent_rotation_delta :: CFloat
      -- ^ The same turn as `rotation`, since the previous event.
    , rGestureEvent_cancelled :: CInt
      -- ^ R_GESTURE_END: R_TRUE when the system took the gesture back (nothing must happen as for an end).
    } deriving (Eq, Show)

-- | The zeroed structure (what @RGestureEvent x = {0};@ is in C).
rGestureEvent :: RGestureEvent
rGestureEvent = RGestureEvent (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0)

instance Storable RGestureEvent where
    sizeOf _ = 76
    alignment _ = 4
    peek p = RGestureEvent
        <$> peekByteOff p 0
        <*> peekByteOff p 4
        <*> peekByteOff p 8
        <*> peekByteOff p 12
        <*> peekByteOff p 16
        <*> peekByteOff p 20
        <*> peekByteOff p 24
        <*> peekByteOff p 28
        <*> peekByteOff p 32
        <*> peekByteOff p 36
        <*> peekByteOff p 40
        <*> peekByteOff p 44
        <*> peekByteOff p 48
        <*> peekByteOff p 52
        <*> peekByteOff p 56
        <*> peekByteOff p 60
        <*> peekByteOff p 64
        <*> peekByteOff p 68
        <*> peekByteOff p 72
    poke p v = do
        pokeByteOff p 0 (rGestureEvent_phase v)
        pokeByteOff p 4 (rGestureEvent_window v)
        pokeByteOff p 8 (rGestureEvent_target v)
        pokeByteOff p 12 (rGestureEvent_x v)
        pokeByteOff p 16 (rGestureEvent_y v)
        pokeByteOff p 20 (rGestureEvent_local_x v)
        pokeByteOff p 24 (rGestureEvent_local_y v)
        pokeByteOff p 28 (rGestureEvent_scale v)
        pokeByteOff p 32 (rGestureEvent_scale_delta v)
        pokeByteOff p 36 (rGestureEvent_pan_x v)
        pokeByteOff p 40 (rGestureEvent_pan_y v)
        pokeByteOff p 44 (rGestureEvent_rotation v)
        pokeByteOff p 48 (rGestureEvent_kind v)
        pokeByteOff p 52 (rGestureEvent_device v)
        pokeByteOff p 56 (rGestureEvent_fingers v)
        pokeByteOff p 60 (rGestureEvent_total_x v)
        pokeByteOff p 64 (rGestureEvent_total_y v)
        pokeByteOff p 68 (rGestureEvent_rotation_delta v)
        pokeByteOff p 72 (rGestureEvent_cancelled v)

-- | What a tile shows, filled by the application when the tile is drawn.
data RTileItem = RTileItem
    { rTileItem_text :: CString
      -- ^ Name under the icon (valid until the callback returns).
    , rTileItem_detail :: CString
      -- ^ Second line, dimmer, or NULL.
    , rTileItem_badge :: CString
      -- ^ Small text at the top right of the tile, or NULL.
    , rTileItem_icon :: Rid
      -- ^ Icon texture (R_INVALID_ID: none), the region of it and its tint (white by default).
    , rTileItem_uv :: [CFloat]
    , rTileItem_tint :: [CFloat]
    , rTileItem_icon_radius :: CFloat
    , rTileItem_text_color :: [CFloat]
      -- ^ Color of the name; alpha 0 = the theme.
    } deriving (Eq, Show)

-- | The zeroed structure (what @RTileItem x = {0};@ is in C).
rTileItem :: RTileItem
rTileItem = RTileItem (nullPtr) (nullPtr) (nullPtr) (0) (replicate 4 0) (replicate 4 0) (0) (replicate 4 0)

instance Storable RTileItem where
    sizeOf _ = 80
    alignment _ = 8
    peek p = RTileItem
        <$> peekByteOff p 0
        <*> peekByteOff p 8
        <*> peekByteOff p 16
        <*> peekByteOff p 24
        <*> peekArray 4 (p `plusPtr` 28)
        <*> peekArray 4 (p `plusPtr` 44)
        <*> peekByteOff p 60
        <*> peekArray 4 (p `plusPtr` 64)
    poke p v = do
        pokeByteOff p 0 (rTileItem_text v)
        pokeByteOff p 8 (rTileItem_detail v)
        pokeByteOff p 16 (rTileItem_badge v)
        pokeByteOff p 24 (rTileItem_icon v)
        pokeArray (p `plusPtr` 28) (take 4 (rTileItem_uv v ++ repeat 0))
        pokeArray (p `plusPtr` 44) (take 4 (rTileItem_tint v ++ repeat 0))
        pokeByteOff p 60 (rTileItem_icon_radius v)
        pokeArray (p `plusPtr` 64) (take 4 (rTileItem_text_color v ++ repeat 0))

-- | What the editor knows of a language. Every field but the name may be NULL.
-- Lists of words are separated by spaces.
data RLanguageDesc = RLanguageDesc
    { rLanguageDesc_name :: CString
      -- ^ "Rust". Compared without case.
    , rLanguageDesc_extensions :: CString
      -- ^ Extensions of its files, separated by ';': "rs".
    , rLanguageDesc_keywords :: CString
      -- ^ Words of the language itself: if, while, return...
    , rLanguageDesc_types :: CString
      -- ^ Names of types and of what is always there: int, String, print...
    , rLanguageDesc_constants :: CString
      -- ^ Values with a name: true, false, null...
    , rLanguageDesc_line_comment :: CString
      -- ^ What starts a comment that ends with its line: two slashes, a hash.
    , rLanguageDesc_block_comment_start :: CString
      -- ^ What opens and closes a comment that may span lines.
    , rLanguageDesc_block_comment_end :: CString
    , rLanguageDesc_string_quotes :: CString
      -- ^ Characters that open and close a text on one line: a double quote, a single quote.
    , rLanguageDesc_multiline_strings :: CString
      -- ^ Marks of the texts that span lines, separated by spaces: three double
      -- quotes, a backquote. When what closes such a text is not what opens it,
      -- write both around three dots: R"(...)" . Nothing is escaped in those.
    , rLanguageDesc_directive :: CString
      -- ^ What starts a line for the preprocessor: "#".
    , rLanguageDesc_annotation :: CString
      -- ^ What starts an annotation: "@" (@Override), or "#[" up to its closing bracket.
    , rLanguageDesc_flags :: Word32
      -- ^ Bitmask of RLanguageFlags.
    , rLanguageDesc_script_language :: CString
      -- ^ Markup with R_LANGUAGE_EMBEDS: the language of what is between
      -- <script> and </script> (the name of a language: "C", "Rust"...).
      -- NULL: JavaScript, as in HTML.
    } deriving (Eq, Show)

-- | The zeroed structure (what @RLanguageDesc x = {0};@ is in C).
rLanguageDesc :: RLanguageDesc
rLanguageDesc = RLanguageDesc (nullPtr) (nullPtr) (nullPtr) (nullPtr) (nullPtr) (nullPtr) (nullPtr) (nullPtr) (nullPtr) (nullPtr) (nullPtr) (nullPtr) (0) (nullPtr)

instance Storable RLanguageDesc where
    sizeOf _ = 112
    alignment _ = 8
    peek p = RLanguageDesc
        <$> peekByteOff p 0
        <*> peekByteOff p 8
        <*> peekByteOff p 16
        <*> peekByteOff p 24
        <*> peekByteOff p 32
        <*> peekByteOff p 40
        <*> peekByteOff p 48
        <*> peekByteOff p 56
        <*> peekByteOff p 64
        <*> peekByteOff p 72
        <*> peekByteOff p 80
        <*> peekByteOff p 88
        <*> peekByteOff p 96
        <*> peekByteOff p 104
    poke p v = do
        pokeByteOff p 0 (rLanguageDesc_name v)
        pokeByteOff p 8 (rLanguageDesc_extensions v)
        pokeByteOff p 16 (rLanguageDesc_keywords v)
        pokeByteOff p 24 (rLanguageDesc_types v)
        pokeByteOff p 32 (rLanguageDesc_constants v)
        pokeByteOff p 40 (rLanguageDesc_line_comment v)
        pokeByteOff p 48 (rLanguageDesc_block_comment_start v)
        pokeByteOff p 56 (rLanguageDesc_block_comment_end v)
        pokeByteOff p 64 (rLanguageDesc_string_quotes v)
        pokeByteOff p 72 (rLanguageDesc_multiline_strings v)
        pokeByteOff p 80 (rLanguageDesc_directive v)
        pokeByteOff p 88 (rLanguageDesc_annotation v)
        pokeByteOff p 96 (rLanguageDesc_flags v)
        pokeByteOff p 104 (rLanguageDesc_script_language v)

-- | A task of the jump list of Windows (Rtaskbar_set_jump_list).
data RJumpTask = RJumpTask
    { rJumpTask_title :: CString
      -- ^ What the list shows.
    , rJumpTask_arguments :: CString
      -- ^ The command line given to the application when it is chosen: it is started again with these arguments.
    , rJumpTask_description :: CString
      -- ^ The tooltip; NULL: none.
    } deriving (Eq, Show)

-- | The zeroed structure (what @RJumpTask x = {0};@ is in C).
rJumpTask :: RJumpTask
rJumpTask = RJumpTask (nullPtr) (nullPtr) (nullPtr)

instance Storable RJumpTask where
    sizeOf _ = 24
    alignment _ = 8
    peek p = RJumpTask
        <$> peekByteOff p 0
        <*> peekByteOff p 8
        <*> peekByteOff p 16
    poke p v = do
        pokeByteOff p 0 (rJumpTask_title v)
        pokeByteOff p 8 (rJumpTask_arguments v)
        pokeByteOff p 16 (rJumpTask_description v)

-- | A kind of file in a file dialog.
data RFileFilter = RFileFilter
    { rFileFilter_name :: CString
      -- ^ Shown to the user: "Images".
    , rFileFilter_patterns :: CString
      -- ^ Patterns separated by ';': "*.png;*.jpg". "*" matches everything.
    } deriving (Eq, Show)

-- | The zeroed structure (what @RFileFilter x = {0};@ is in C).
rFileFilter :: RFileFilter
rFileFilter = RFileFilter (nullPtr) (nullPtr)

instance Storable RFileFilter where
    sizeOf _ = 16
    alignment _ = 8
    peek p = RFileFilter
        <$> peekByteOff p 0
        <*> peekByteOff p 8
    poke p v = do
        pokeByteOff p 0 (rFileFilter_name v)
        pokeByteOff p 8 (rFileFilter_patterns v)

-- | Every field may be left to zero.
data RFileDialogDesc = RFileDialogDesc
    { rFileDialogDesc_parent :: Rid
      -- ^ Window the dialog belongs to. R_INVALID_ID: none (a window is opened when Raven needs one).
    , rFileDialogDesc_title :: CString
      -- ^ NULL: "Open", "Save as"...
    , rFileDialogDesc_directory :: CString
      -- ^ Folder shown first. NULL: the one of the previous dialog, else the home of the user.
    , rFileDialogDesc_file_name :: CString
      -- ^ Name proposed to the user (save dialogs).
    , rFileDialogDesc_filters :: Ptr RFileFilter
    , rFileDialogDesc_filter_count :: CInt
    , rFileDialogDesc_default_extension :: CString
      -- ^ Save dialogs: added to a name typed without extension ("xml").
    , rFileDialogDesc_flags :: Word32
      -- ^ Bitmask of RFileDialogFlags.
    } deriving (Eq, Show)

-- | The zeroed structure (what @RFileDialogDesc x = {0};@ is in C).
rFileDialogDesc :: RFileDialogDesc
rFileDialogDesc = RFileDialogDesc (0) (nullPtr) (nullPtr) (nullPtr) (nullPtr) (0) (nullPtr) (0)

instance Storable RFileDialogDesc where
    sizeOf _ = 64
    alignment _ = 8
    peek p = RFileDialogDesc
        <$> peekByteOff p 0
        <*> peekByteOff p 8
        <*> peekByteOff p 16
        <*> peekByteOff p 24
        <*> peekByteOff p 32
        <*> peekByteOff p 40
        <*> peekByteOff p 48
        <*> peekByteOff p 56
    poke p v = do
        pokeByteOff p 0 (rFileDialogDesc_parent v)
        pokeByteOff p 8 (rFileDialogDesc_title v)
        pokeByteOff p 16 (rFileDialogDesc_directory v)
        pokeByteOff p 24 (rFileDialogDesc_file_name v)
        pokeByteOff p 32 (rFileDialogDesc_filters v)
        pokeByteOff p 40 (rFileDialogDesc_filter_count v)
        pokeByteOff p 48 (rFileDialogDesc_default_extension v)
        pokeByteOff p 56 (rFileDialogDesc_flags v)

-- | Passed to the draw callback of a viewport. Sizes are in pixels.
data RViewportDrawInfo = RViewportDrawInfo
    { rViewportDrawInfo_backend :: RBackend
    , rViewportDrawInfo_viewport :: Rid
    , rViewportDrawInfo_window :: Rid
    , rViewportDrawInfo_x :: CInt
      -- ^ Area of the viewport in the window framebuffer.
    , rViewportDrawInfo_y :: CInt
      -- ^ Area of the viewport in the window framebuffer.
    , rViewportDrawInfo_width :: CInt
      -- ^ Area of the viewport in the window framebuffer.
    , rViewportDrawInfo_height :: CInt
      -- ^ Area of the viewport in the window framebuffer.
    , rViewportDrawInfo_clip_x :: CInt
      -- ^ Visible part of that area. Already set as the scissor rectangle.
    , rViewportDrawInfo_clip_y :: CInt
      -- ^ Visible part of that area. Already set as the scissor rectangle.
    , rViewportDrawInfo_clip_width :: CInt
      -- ^ Visible part of that area. Already set as the scissor rectangle.
    , rViewportDrawInfo_clip_height :: CInt
      -- ^ Visible part of that area. Already set as the scissor rectangle.
    , rViewportDrawInfo_framebuffer_width :: CInt
    , rViewportDrawInfo_framebuffer_height :: CInt
    , rViewportDrawInfo_frame_index :: Word64
    , rViewportDrawInfo_command_buffer :: Ptr ()
      -- ^ Vulkan: VkCommandBuffer being recorded, inside the render pass.
    , rViewportDrawInfo_render_pass :: Ptr ()
      -- ^ Vulkan: VkRenderPass in use (one color attachment, no depth).
    , rViewportDrawInfo_color_format :: Word32
      -- ^ Vulkan: VkFormat of the color attachment.
    } deriving (Eq, Show)

-- | The zeroed structure (what @RViewportDrawInfo x = {0};@ is in C).
rViewportDrawInfo :: RViewportDrawInfo
rViewportDrawInfo = RViewportDrawInfo (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (0) (nullPtr) (nullPtr) (0)

instance Storable RViewportDrawInfo where
    sizeOf _ = 88
    alignment _ = 8
    peek p = RViewportDrawInfo
        <$> peekByteOff p 0
        <*> peekByteOff p 4
        <*> peekByteOff p 8
        <*> peekByteOff p 12
        <*> peekByteOff p 16
        <*> peekByteOff p 20
        <*> peekByteOff p 24
        <*> peekByteOff p 28
        <*> peekByteOff p 32
        <*> peekByteOff p 36
        <*> peekByteOff p 40
        <*> peekByteOff p 44
        <*> peekByteOff p 48
        <*> peekByteOff p 56
        <*> peekByteOff p 64
        <*> peekByteOff p 72
        <*> peekByteOff p 80
    poke p v = do
        pokeByteOff p 0 (rViewportDrawInfo_backend v)
        pokeByteOff p 4 (rViewportDrawInfo_viewport v)
        pokeByteOff p 8 (rViewportDrawInfo_window v)
        pokeByteOff p 12 (rViewportDrawInfo_x v)
        pokeByteOff p 16 (rViewportDrawInfo_y v)
        pokeByteOff p 20 (rViewportDrawInfo_width v)
        pokeByteOff p 24 (rViewportDrawInfo_height v)
        pokeByteOff p 28 (rViewportDrawInfo_clip_x v)
        pokeByteOff p 32 (rViewportDrawInfo_clip_y v)
        pokeByteOff p 36 (rViewportDrawInfo_clip_width v)
        pokeByteOff p 40 (rViewportDrawInfo_clip_height v)
        pokeByteOff p 44 (rViewportDrawInfo_framebuffer_width v)
        pokeByteOff p 48 (rViewportDrawInfo_framebuffer_height v)
        pokeByteOff p 56 (rViewportDrawInfo_frame_index v)
        pokeByteOff p 64 (rViewportDrawInfo_command_buffer v)
        pokeByteOff p 72 (rViewportDrawInfo_render_pass v)
        pokeByteOff p 80 (rViewportDrawInfo_color_format v)

-- | State of one animated value. Zero-initialize it and keep it with the widget.
data RSpring = RSpring
    { rSpring_value :: CFloat
    , rSpring_velocity :: CFloat
    , rSpring_started :: CInt
    } deriving (Eq, Show)

-- | The zeroed structure (what @RSpring x = {0};@ is in C).
rSpring :: RSpring
rSpring = RSpring (0) (0) (0)

instance Storable RSpring where
    sizeOf _ = 12
    alignment _ = 4
    peek p = RSpring
        <$> peekByteOff p 0
        <*> peekByteOff p 4
        <*> peekByteOff p 8
    poke p v = do
        pokeByteOff p 0 (rSpring_value v)
        pokeByteOff p 4 (rSpring_velocity v)
        pokeByteOff p 8 (rSpring_started v)

-- | How an effect is drawn. Start from Rshader_look_init.
data RShaderLook = RShaderLook
    { rShaderLook_color :: [CFloat]
      -- ^ First and second color, 0..1, straight alpha.
    , rShaderLook_color2 :: [CFloat]
    , rShaderLook_params :: [CFloat]
      -- ^ Four numbers the effect reads as fx.params (their meaning is the effect's).
    , rShaderLook_radius :: CFloat
      -- ^ Radius of the corners, in logical units.
    , rShaderLook_texture :: Rid
      -- ^ Texture the effect reads with raven_texture(), or R_INVALID_ID.
    , rShaderLook_animate :: CInt
      -- ^ -1: animated when the shader is (R_SHADER_ANIMATED); R_TRUE / R_FALSE forces it.
    , rShaderLook_pointer :: CInt
      -- ^ R_TRUE (the default): the effect knows where the pointer is (fx.pointer, fx.hover, fx.press).
    , rShaderLook_hover :: CFloat
      -- ^ How much the pointer is over it and how much it is pressed, 0..1, as
      -- the effect reads them (fx.hover, fx.press). -1 (the default): 1 while
      -- the pointer is in the rectangle, and while the left button is down
      -- there; a Border eases them itself. The rectangle is clipped to the
      -- visible painting area; overlaid siblings do not interrupt its hover.
    , rShaderLook_press :: CFloat
    } deriving (Eq, Show)

-- | The zeroed structure (what @RShaderLook x = {0};@ is in C).
rShaderLook :: RShaderLook
rShaderLook = RShaderLook (replicate 4 0) (replicate 4 0) (replicate 4 0) (0) (0) (0) (0) (0) (0)

instance Storable RShaderLook where
    sizeOf _ = 72
    alignment _ = 4
    peek p = RShaderLook
        <$> peekArray 4 (p `plusPtr` 0)
        <*> peekArray 4 (p `plusPtr` 16)
        <*> peekArray 4 (p `plusPtr` 32)
        <*> peekByteOff p 48
        <*> peekByteOff p 52
        <*> peekByteOff p 56
        <*> peekByteOff p 60
        <*> peekByteOff p 64
        <*> peekByteOff p 68
    poke p v = do
        pokeArray (p `plusPtr` 0) (take 4 (rShaderLook_color v ++ repeat 0))
        pokeArray (p `plusPtr` 16) (take 4 (rShaderLook_color2 v ++ repeat 0))
        pokeArray (p `plusPtr` 32) (take 4 (rShaderLook_params v ++ repeat 0))
        pokeByteOff p 48 (rShaderLook_radius v)
        pokeByteOff p 52 (rShaderLook_texture v)
        pokeByteOff p 56 (rShaderLook_animate v)
        pokeByteOff p 60 (rShaderLook_pointer v)
        pokeByteOff p 64 (rShaderLook_hover v)
        pokeByteOff p 68 (rShaderLook_press v)

-- | Where and why a text could not be read (Rxml_document_parse).
data RXmlError = RXmlError
    { rXmlError_line :: CInt
      -- ^ Both start at 1.
    , rXmlError_column :: CInt
    , rXmlError_message :: [CChar]
    } deriving (Eq, Show)

-- | The zeroed structure (what @RXmlError x = {0};@ is in C).
rXmlError :: RXmlError
rXmlError = RXmlError (0) (0) (replicate 160 0)

instance Storable RXmlError where
    sizeOf _ = 168
    alignment _ = 4
    peek p = RXmlError
        <$> peekByteOff p 0
        <*> peekByteOff p 4
        <*> peekArray 160 (p `plusPtr` 8)
    poke p v = do
        pokeByteOff p 0 (rXmlError_line v)
        pokeByteOff p 4 (rXmlError_column v)
        pokeArray (p `plusPtr` 8) (take 160 (rXmlError_message v ++ repeat 0))

-- | One attribute of a tag (Rxml_get_attribute_info). The strings belong to the library.
data RXmlAttributeInfo = RXmlAttributeInfo
    { rXmlAttributeInfo_name :: CString
    , rXmlAttributeInfo_type :: RXmlValueType
    , rXmlAttributeInfo_choices :: CString
      -- ^ R_XML_CHOICE: the words, separated by '|'. NULL otherwise.
    , rXmlAttributeInfo_group :: CString
      -- ^ What it belongs to: the tag itself ("Button"), "Widget" for what every widget has, "Slot" for the place in the parent.
    , rXmlAttributeInfo_description :: CString
      -- ^ One line saying what it does.
    } deriving (Eq, Show)

-- | The zeroed structure (what @RXmlAttributeInfo x = {0};@ is in C).
rXmlAttributeInfo :: RXmlAttributeInfo
rXmlAttributeInfo = RXmlAttributeInfo (nullPtr) (0) (nullPtr) (nullPtr) (nullPtr)

instance Storable RXmlAttributeInfo where
    sizeOf _ = 40
    alignment _ = 8
    peek p = RXmlAttributeInfo
        <$> peekByteOff p 0
        <*> peekByteOff p 8
        <*> peekByteOff p 16
        <*> peekByteOff p 24
        <*> peekByteOff p 32
    poke p v = do
        pokeByteOff p 0 (rXmlAttributeInfo_name v)
        pokeByteOff p 8 (rXmlAttributeInfo_type v)
        pokeByteOff p 16 (rXmlAttributeInfo_choices v)
        pokeByteOff p 24 (rXmlAttributeInfo_group v)
        pokeByteOff p 32 (rXmlAttributeInfo_description v)

-- | What the system asks for.
data RAccessibilitySettings = RAccessibilitySettings
    { rAccessibilitySettings_text_scale :: CFloat
      -- ^ Size of the texts asked by the user (1 = normal).
    , rAccessibilitySettings_high_contrast :: CInt
    , rAccessibilitySettings_reduce_motion :: CInt
    , rAccessibilitySettings_screen_reader :: CInt
      -- ^ R_TRUE when a screen reader is running.
    } deriving (Eq, Show)

-- | The zeroed structure (what @RAccessibilitySettings x = {0};@ is in C).
rAccessibilitySettings :: RAccessibilitySettings
rAccessibilitySettings = RAccessibilitySettings (0) (0) (0) (0)

instance Storable RAccessibilitySettings where
    sizeOf _ = 16
    alignment _ = 4
    peek p = RAccessibilitySettings
        <$> peekByteOff p 0
        <*> peekByteOff p 4
        <*> peekByteOff p 8
        <*> peekByteOff p 12
    poke p v = do
        pokeByteOff p 0 (rAccessibilitySettings_text_scale v)
        pokeByteOff p 4 (rAccessibilitySettings_high_contrast v)
        pokeByteOff p 8 (rAccessibilitySettings_reduce_motion v)
        pokeByteOff p 12 (rAccessibilitySettings_screen_reader v)

data RHostPlatformDesc = RHostPlatformDesc
    { rHostPlatformDesc_set_cursor :: FunPtr ()
      -- ^ The pointer over `window` should look like `cursor`. May be NULL.
    , rHostPlatformDesc_clipboard_set :: FunPtr ()
      -- ^ Copies UTF-8 text to the clipboard of the application. NULL: the clipboard stays in the process.
    , rHostPlatformDesc_clipboard_get :: FunPtr ()
      -- ^ The clipboard text, valid until the next call, or NULL. NULL: the clipboard stays in the process.
    , rHostPlatformDesc_wake :: FunPtr ()
      -- ^ Raven has work to do (a timer, a posted task, a redraw): produce a frame soon. Thread-safe. May be NULL.
    , rHostPlatformDesc_user_data :: Ptr ()
    } deriving (Eq, Show)

-- | The zeroed structure (what @RHostPlatformDesc x = {0};@ is in C).
rHostPlatformDesc :: RHostPlatformDesc
rHostPlatformDesc = RHostPlatformDesc (nullFunPtr) (nullFunPtr) (nullFunPtr) (nullFunPtr) (nullPtr)

instance Storable RHostPlatformDesc where
    sizeOf _ = 40
    alignment _ = 8
    peek p = RHostPlatformDesc
        <$> peekByteOff p 0
        <*> peekByteOff p 8
        <*> peekByteOff p 16
        <*> peekByteOff p 24
        <*> peekByteOff p 32
    poke p v = do
        pokeByteOff p 0 (rHostPlatformDesc_set_cursor v)
        pokeByteOff p 8 (rHostPlatformDesc_clipboard_set v)
        pokeByteOff p 16 (rHostPlatformDesc_clipboard_get v)
        pokeByteOff p 24 (rHostPlatformDesc_wake v)
        pokeByteOff p 32 (rHostPlatformDesc_user_data v)

-- | Starts the library with default settings on the given backend.
-- The calling thread becomes the UI thread.
-- Returns: R_TRUE on success. On failure, see Rerror_get_last.
foreign import ccall safe "Rinit" rinit :: RBackend -> IO CInt

-- | Starts the library with explicit settings.
-- desc Settings, or NULL for the defaults.
foreign import ccall safe "Rinit_ex" rinit_ex :: Ptr RInitDesc -> IO CInt

-- | Destroys every window, widget and resource, stops the worker and
-- render threads and releases the backend.
foreign import ccall safe "Rshutdown" rshutdown :: IO ()

foreign import ccall safe "Ris_initialized" ris_initialized :: IO CInt

-- | Returns: The library version, e.g. "0.1.0".
foreign import ccall safe "Rget_version" rget_version :: IO CString

-- | Replaces the allocator of Raven with the functions of the
-- application (an engine allocator, a tracker...). NULL selects the
-- allocator of Raven again. Must be called before Rinit, and before any
-- other function that allocates. The functions are called from several
-- threads; alloc must return memory aligned for any type (16 bytes).
foreign import ccall safe "Rset_allocator" rset_allocator :: Ptr RAllocator -> IO ()

-- | Returns: An allocator that forwards to malloc, realloc and free of the C
-- runtime: Rset_allocator(Rallocator_get_system()) is how memory tools
-- that follow malloc (Valgrind, heap profilers) see the blocks of Raven.
-- The environment variable RAVEN_ALLOCATOR=system does the same without
-- recompiling, and it is the default in a build with AddressSanitizer.
foreign import ccall safe "Rallocator_get_system" rallocator_get_system :: IO (Ptr RAllocator)

-- | Memory from the allocator of Raven (or from the one given to
-- Rset_allocator), for the application: custom widgets, pixel buffers.
-- Aligned on 16 bytes. Never returns NULL: running out of memory is a
-- fatal error. Thread-safe.
foreign import ccall safe "Rmemory_alloc" rmemory_alloc :: CSize -> IO (Ptr ())

-- | Resizes a block, keeping its content. A NULL block is allocated. Thread-safe.
foreign import ccall safe "Rmemory_realloc" rmemory_realloc :: Ptr () -> CSize -> IO (Ptr ())

-- | Frees a block. NULL is ignored. Thread-safe.
foreign import ccall safe "Rmemory_free" rmemory_free :: Ptr () -> IO ()

-- | What the library holds right now. Thread-safe.
foreign import ccall safe "Rmemory_get_stats" rmemory_get_stats :: Ptr RMemoryStats -> IO ()

-- | Walks the whole heap of Raven and verifies its structure.
-- Returns: R_FALSE when it is corrupted (a buffer overrun wrote over a block
-- header). Slow: for tests and for hunting a bug. Thread-safe.
foreign import ccall safe "Rmemory_check" rmemory_check :: IO CInt

-- | Returns to the OS the regions of the heap that hold nothing. Thread-safe.
foreign import ccall safe "Rmemory_trim" rmemory_trim :: IO ()

-- | Produces one frame: processes the pending OS events without
-- waiting, runs the posted tasks and the timers, then lays out, paints and
-- submits every window that changed. A window in which nothing changed is
-- not drawn again, so the call is cheap when the UI is idle.
-- Returns: R_FALSE once Rquit was called or the last window is closed.
foreign import ccall safe "Rdraw_frame" rdraw_frame :: IO CInt

-- | Same as Rdraw_frame, but first sleeps until there is something to
-- do: an OS event, a posted task, a timer, an animation, a pending redraw.
-- timeout_seconds Longest sleep. Negative sleeps without limit, 0
-- does not sleep.
foreign import ccall safe "Rwait_frame" rwait_frame :: CDouble -> IO CInt

-- | Makes Rdraw_frame and Rwait_frame return R_FALSE. Thread-safe.
foreign import ccall safe "Rquit" rquit :: CInt -> IO ()

-- | Returns: The code given to Rquit, 0 when the last window was closed.
foreign import ccall safe "Rget_exit_code" rget_exit_code :: IO CInt

-- | Returns: R_FALSE once Rquit was called or every window is closed.
foreign import ccall safe "Ris_running" ris_running :: IO CInt

-- | Processes the pending OS events without producing a frame.
-- Rdraw_frame does it already: only useful to keep the windows responsive
-- during a long operation.
foreign import ccall safe "Rpoll_events" rpoll_events :: IO ()

-- | Sleeps until an OS event, a posted task, a timer or a pending
-- redraw, and processes the OS events. Rwait_frame does it already.
-- timeout_seconds Maximum wait. Negative waits without limit.
foreign import ccall safe "Rwait_events" rwait_events :: CDouble -> IO ()

foreign import ccall safe "Rset_frame_callback" rset_frame_callback :: RFrameCallback -> Ptr () -> IO ()

-- | Returns: Seconds between the last two frames.
foreign import ccall safe "Rget_delta_time" rget_delta_time :: IO CDouble

-- | Returns: Seconds elapsed since Rinit, from a monotonic clock. Thread-safe.
foreign import ccall safe "Rget_time" rget_time :: IO CDouble

-- | Scale of the whole interface, on top of the one of the monitor
-- (RInitDesc.ui_scale at start): every window is laid out again. 0.25 .. 4.
foreign import ccall safe "Rset_ui_scale" rset_ui_scale :: CFloat -> IO ()

-- | R_TRUE: every frame waits for the vertical synchronization of the
-- screen (no tearing, no wasted frames); R_FALSE: as fast as possible
-- (RInitDesc.disable_vsync at start). Takes effect at the next frame.
foreign import ccall safe "Rset_vsync" rset_vsync :: CInt -> IO ()

foreign import ccall safe "Rget_vsync" rget_vsync :: IO CInt

foreign import ccall safe "Rget_ui_scale" rget_ui_scale :: IO CFloat

-- | R_FALSE makes every change immediate: for users who ask for
-- reduced motion (Raccessibility_set_reduce_motion), and for tests that
-- read the state right after an input.
foreign import ccall safe "Rset_animations_enabled" rset_animations_enabled :: CInt -> IO ()

foreign import ccall safe "Rget_animations_enabled" rget_animations_enabled :: IO CInt

-- | Multiplies the speed of every animation. 1 is the default, 0.25 a slow motion.
foreign import ccall safe "Rset_animation_speed" rset_animation_speed :: CFloat -> IO ()

foreign import ccall safe "Rget_animation_speed" rget_animation_speed :: IO CFloat

-- | When positive, each frame advances the animations by exactly this
-- many seconds instead of the time really elapsed: frames rendered one by
-- one for a video or a screenshot, deterministic tests. 0 follows the clock.
foreign import ccall safe "Rset_animation_time_step" rset_animation_time_step :: CDouble -> IO ()

-- | Queues a function to run on the UI thread during the next frame
-- and wakes Rwait_frame. Thread-safe: this is how worker threads touch the UI.
foreign import ccall safe "Rtask_post" rtask_post :: RTaskCallback -> Ptr () -> IO ()

-- | Runs a function on a worker thread of the job system. Thread-safe.
foreign import ccall safe "Rtask_run_async" rtask_run_async :: RTaskCallback -> Ptr () -> IO ()

-- | Splits [0, count) into ranges of at most `grain` items and runs them
-- on the worker threads and the calling thread. Returns when all are done.
foreign import ccall safe "Rparallel_for" rparallel_for :: Word32 -> Word32 -> RParallelRangeCallback -> Ptr () -> IO ()

-- | Wakes Rwait_frame and Rwait_events from any thread.
foreign import ccall safe "Rwake_up" rwake_up :: IO ()

foreign import ccall safe "Rget_worker_thread_count" rget_worker_thread_count :: IO CInt

foreign import ccall safe "Ris_ui_thread" ris_ui_thread :: IO CInt

-- | Calls a function on the UI thread after `interval_seconds`.
-- repeat R_TRUE to fire repeatedly, R_FALSE to fire once and delete itself.
foreign import ccall safe "Rtimer_create" rtimer_create :: CDouble -> CInt -> RTimerCallback -> Ptr () -> IO Rid

foreign import ccall safe "Rtimer_destroy" rtimer_destroy :: Rid -> IO ()

foreign import ccall safe "Rtimer_is_valid" rtimer_is_valid :: Rid -> IO CInt

foreign import ccall safe "Raction_bind" raction_bind :: CString -> RActionCallback -> Ptr () -> IO ()

foreign import ccall safe "Raction_unbind" raction_unbind :: CString -> IO ()

-- | Fires an action as a widget would: the function bound to the
-- name is called, or the default handler. What a tool bar button, a
-- script or a test uses to do what a menu item does.
-- sender Passed to the function; R_INVALID_ID when there is none.
foreign import ccall safe "Raction_fire" raction_fire :: CString -> Rid -> IO ()

-- | Receives every action that has no bound function and is not one of the built-in actions (e.g. a script bridge).
foreign import ccall safe "Raction_set_default_handler" raction_set_default_handler :: RActionCallback -> Ptr () -> IO ()

-- | The actions Raven knows by itself: an interface written in XML
-- uses them with no C at all (OnClicked="app.quit").
--
--   app.quit                          quits (Rquit(0))
--   window.close, window.minimize,    the window of the widget
--   window.maximize, window.restore   (maximize goes back when it already is)
--   window.picture_in_picture         in and out of picture in picture
--   island.expand, island.fold,       the island of that window
--   island.toggle
--   theme.dark, theme.light,          the look of Raven, dark or light
--   theme.toggle
--   theme:NAME                        Rtheme_use(NAME): a theme loaded or built in
--   language:CODE                     Rlocale_set_language(CODE)
--   open:ADDRESS                      Ropen_url (https:, mailto:, a file)
--   copy:TEXT                         the text to the clipboard
--   show:NAME, hide:NAME, toggle:NAME the widget named NAME in the window (hidden: Collapsed)
--   enable:NAME, disable:NAME
--   focus:NAME
--   page:NAME=2, page:NAME=+1         what a Switcher, a TabView, a ComboBox or a ListView
--                                     shows; +1 and -1 step, and go round
--
-- A function bound to one of these names (Raction_bind) replaces it; the
-- default handler receives the others only.
-- enabled R_FALSE turns them all off: a design tool that shows
-- interfaces without running them. On by default.
foreign import ccall safe "Raction_set_builtins" raction_set_builtins :: CInt -> IO ()

-- | Returns: R_TRUE when the name is one of the built-in actions (Raction_set_builtins), whether they are on or not.
foreign import ccall safe "Raction_is_builtin" raction_is_builtin :: CString -> IO CInt

-- | Opens an address with the program the user has for it: a web
-- page in the browser, a mailto: in the mail program, a file in its
-- application. Returns at once.
-- Returns: R_FALSE when the OS could not be asked (nothing tells whether
-- a program then really opened it).
foreign import ccall safe "Ropen_url" ropen_url :: CString -> IO CInt

foreign import ccall safe "Rclipboard_set_text" rclipboard_set_text :: CString -> IO ()

-- | Copies one content in two formats: `html` for the programs that
-- paste formatting (word processors, chat, mail) and `text` for the
-- others (editors, terminals). Each program takes the format it prefers.
-- Where the window system has no clipboard formats (Win32 and macOS for
-- now), or `html` is NULL, it is Rclipboard_set_text(text).
foreign import ccall safe "Rclipboard_set_html" rclipboard_set_html :: CString -> CString -> IO ()

-- | Returns: The clipboard text, or "" when empty. Valid until the next call.
foreign import ccall safe "Rclipboard_get_text" rclipboard_get_text :: IO CString

-- | Retrieves and clears the last recorded error.
-- detail   Optional output: human-readable description.
-- severity Optional output: severity of the error.
-- Returns: R_NO_ERROR when nothing was recorded.
foreign import ccall safe "Rerror_get_last" rerror_get_last :: Ptr CString -> Ptr RSeverity -> IO RError

foreign import ccall safe "Rerror_to_string" rerror_to_string :: RError -> IO CString

foreign import ccall safe "Rseverity_to_string" rseverity_to_string :: RSeverity -> IO CString

foreign import ccall safe "Rerror_set_callback" rerror_set_callback :: RErrorCallback -> IO ()

foreign import ccall safe "Rset_device_reset_callback" rset_device_reset_callback :: RDeviceResetCallback -> Ptr () -> IO ()

foreign import ccall safe "Rwindow_get_frame_stats" rwindow_get_frame_stats :: Rid -> Ptr RFrameStats -> IO ()

-- | Opens a top-level window.
-- width,height Client size in logical units.
-- flags        Bitmask of RWindowFlags.
foreign import ccall safe "Rwindow_create" rwindow_create :: CString -> CInt -> CInt -> Word32 -> IO Rid

-- | Closes a window and deletes its content widgets.
foreign import ccall safe "Rwindow_destroy" rwindow_destroy :: Rid -> IO ()

foreign import ccall safe "Rwindow_is_valid" rwindow_is_valid :: Rid -> IO CInt

-- | Sets the root widget of the window. The window takes ownership of
-- the widget; the previous content is deleted.
foreign import ccall safe "Rwindow_set_content" rwindow_set_content :: Rid -> Rid -> IO ()

foreign import ccall safe "Rwindow_get_content" rwindow_get_content :: Rid -> IO Rid

foreign import ccall safe "Rwindow_set_title" rwindow_set_title :: Rid -> CString -> IO ()

foreign import ccall safe "Rwindow_get_title" rwindow_get_title :: Rid -> IO CString

foreign import ccall safe "Rwindow_set_size" rwindow_set_size :: Rid -> CInt -> CInt -> IO ()

foreign import ccall safe "Rwindow_get_size" rwindow_get_size :: Rid -> Ptr CInt -> Ptr CInt -> IO ()

foreign import ccall safe "Rwindow_set_min_size" rwindow_set_min_size :: Rid -> CInt -> CInt -> IO ()

foreign import ccall safe "Rwindow_set_position" rwindow_set_position :: Rid -> CInt -> CInt -> IO ()

foreign import ccall safe "Rwindow_get_position" rwindow_get_position :: Rid -> Ptr CInt -> Ptr CInt -> IO ()

-- | Size of the drawable area in pixels.
foreign import ccall safe "Rwindow_get_framebuffer_size" rwindow_get_framebuffer_size :: Rid -> Ptr CInt -> Ptr CInt -> IO ()

-- | Returns: Pixels per logical unit (monitor scale x global UI scale).
foreign import ccall safe "Rwindow_get_scale" rwindow_get_scale :: Rid -> IO CFloat

foreign import ccall safe "Rwindow_show" rwindow_show :: Rid -> IO ()

foreign import ccall safe "Rwindow_hide" rwindow_hide :: Rid -> IO ()

-- | Shows the window in front of the others with the keyboard, and
-- makes the application the active one. Rwindow_show alone does not take
-- the keyboard from another application (a prompt that appears while the
-- user types elsewhere); this is for a window the user just asked for (a
-- card opened from a tray icon). Where the system has no such request,
-- the same as Rwindow_show.
foreign import ccall safe "Rwindow_focus" rwindow_focus :: Rid -> IO ()

foreign import ccall safe "Rwindow_maximize" rwindow_maximize :: Rid -> IO ()

foreign import ccall safe "Rwindow_minimize" rwindow_minimize :: Rid -> IO ()

foreign import ccall safe "Rwindow_restore" rwindow_restore :: Rid -> IO ()

foreign import ccall safe "Rwindow_is_maximized" rwindow_is_maximized :: Rid -> IO CInt

foreign import ccall safe "Rwindow_is_minimized" rwindow_is_minimized :: Rid -> IO CInt

-- | Returns: R_TRUE while the window is the one the user is working in.
foreign import ccall safe "Rwindow_is_active" rwindow_is_active :: Rid -> IO CInt

-- | Asks to close the window, as the close button of its title bar
-- does: the close callback is asked first and may refuse.
foreign import ccall safe "Rwindow_close" rwindow_close :: Rid -> IO ()

-- | Custom headers. Call these from a mouse-down handler, while the left
-- button is held: the window follows the pointer until the button is
-- released (the widget does not receive the release). A
-- R_WIDGET_HEADER_BAR and the widgets given to Rwidget_set_window_drag do
-- it by themselves.
foreign import ccall safe "Rwindow_begin_move" rwindow_begin_move :: Rid -> IO ()

foreign import ccall safe "Rwindow_begin_resize" rwindow_begin_resize :: Rid -> RWindowEdge -> IO ()

-- | Icon of the window (title bar, task bar), from tightly packed
-- RGBA8 pixels; NULL goes back to the default: on Windows the first icon
-- of the executable, when it has one. Windows and X11.
foreign import ccall safe "Rwindow_set_icon" rwindow_set_icon :: Rid -> Ptr Word8 -> CInt -> CInt -> IO ()

foreign import ccall safe "Rwindow_set_clear_color" rwindow_set_clear_color :: Rid -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

-- | Image drawn behind the content, scaled to cover the window:
-- what the glass of a glass theme looks through. R_INVALID_ID removes it.
foreign import ccall safe "Rwindow_set_background" rwindow_set_background :: Rid -> Rid -> IO ()

-- | Chooses what shows behind a window created with
-- R_WINDOW_TRANSPARENT (see RBackdrop). With anything but
-- R_BACKDROP_NONE the window is not cleared with its color and the
-- wallpaper of the theme is not drawn: only what the widgets draw hides
-- the screen. In XML: <Window Backdrop="Blur">.
-- Returns: R_TRUE when the window system does it; R_FALSE (the window
-- then stays as it was) without R_WINDOW_TRANSPARENT, or when it cannot.
foreign import ccall safe "Rwindow_set_backdrop" rwindow_set_backdrop :: Rid -> RBackdrop -> IO CInt

foreign import ccall safe "Rwindow_get_backdrop" rwindow_get_backdrop :: Rid -> IO RBackdrop

-- | Gives the window a theme of its own, by name ("Dark",
-- "WindowsLight", or a theme of Rtheme_load): everything inside it (its
-- content, its island, its menus, lists and dialogs, its background) is
-- drawn with that theme, while the other windows keep the one of the
-- application (Rtheme_use). A widget given Rwidget_set_theme keeps its own.
-- In XML: <Window Theme="...">.
-- name NULL or "": the theme of the application again.
-- Returns: R_FALSE, after reporting an error, when no theme has that name.
foreign import ccall safe "Rwindow_set_theme" rwindow_set_theme :: Rid -> CString -> IO CInt

-- | Returns: Name of the theme the window is drawn with (the one of the application when it has none of its own).
foreign import ccall safe "Rwindow_get_theme" rwindow_get_theme :: Rid -> IO CString

-- | Returns: R_TRUE when the window has a theme of its own (Rwindow_set_theme).
foreign import ccall safe "Rwindow_has_own_theme" rwindow_has_own_theme :: Rid -> IO CInt

-- | Opacity of the whole window, its frame included, from 0 to 1:
-- a window that fades, a tool that lets what is under it show.
-- Returns: R_FALSE when the window system cannot do it.
foreign import ccall safe "Rwindow_set_opacity" rwindow_set_opacity :: Rid -> CFloat -> IO CInt

foreign import ccall safe "Rwindow_get_opacity" rwindow_get_opacity :: Rid -> IO CFloat

-- | Keeps the window above the other windows (splash screens, tools).
-- Returns: R_FALSE when the window system cannot do it.
foreign import ccall safe "Rwindow_set_topmost" rwindow_set_topmost :: Rid -> CInt -> IO CInt

-- | Gives the window an owner (a tool window or an inspector of the
-- main window): it stays above its owner, has no button of its own in the
-- taskbar, is minimized and restored with it (where the window system
-- does it: Windows, X11), and is destroyed with it (everywhere: Raven
-- destroys the windows a window owns before the window). R_INVALID_ID
-- gives the window back its independence.
-- Returns: R_FALSE when the window system cannot keep the window with its
-- owner (it is still destroyed with it), or when the owner is not valid
-- or is owned by the window (nothing is changed then).
foreign import ccall safe "Rwindow_set_owner" rwindow_set_owner :: Rid -> Rid -> IO CInt

-- | Returns: The owner of the window (Rwindow_set_owner), or R_INVALID_ID.
foreign import ccall safe "Rwindow_get_owner" rwindow_get_owner :: Rid -> IO Rid

-- | Picture in picture: the window becomes a small floating window
-- that stays above the other windows, on every desktop (every space of
-- macOS, over the applications in full screen too), in a corner of the
-- work area of its monitor. It loses its title bar where the system
-- allows: the empty parts of its content move it, its edges resize it,
-- and its size keeps the proportions it was given. Leaving it gives back
-- the size, place, frame and state the window had.
--
-- Raven does not add a button to leave it: the content shows one, with
-- the built-in action "window.picture_in_picture" (it goes in and out).
-- Returns: R_FALSE when the window system cannot keep a window above the
-- others (Wayland): the window is left as it is.
foreign import ccall safe "Rwindow_set_picture_in_picture" rwindow_set_picture_in_picture :: Rid -> CInt -> IO CInt

foreign import ccall safe "Rwindow_is_picture_in_picture" rwindow_is_picture_in_picture :: Rid -> IO CInt

-- | Size (logical units) and corner of the window in picture in
-- picture, for the next time it goes in (now, when it is in). 0, 0 (the
-- default): 320 wide, with the proportions the content has when it goes
-- in. The proportions of width / height are kept while the user resizes.
foreign import ccall safe "Rwindow_set_picture_in_picture_layout" rwindow_set_picture_in_picture_layout :: Rid -> CInt -> CInt -> RPictureInPictureCorner -> IO ()

-- | Centers the window on the work area of its monitor (no task bar).
foreign import ccall safe "Rwindow_center" rwindow_center :: Rid -> IO ()

-- | Shows (R_TRUE) or hides (R_FALSE) the inspector of the window: a
-- window beside it that lists its widgets as a tree, says what the
-- selected one is (type, name, classes, place and size in the window,
-- desired size, constraints, slot, visibility, state, style, action,
-- children) and what the last frame cost (layout, paint and render
-- times, widgets, instances, draw calls). The selected widget is
-- outlined in the window, with its type and size. "Pick" selects the
-- next widget clicked in the window (that click goes to the inspector
-- only). It follows the window as it changes; closing it hides it, and
-- it goes with the window. Nothing opens it but this function.
foreign import ccall safe "Rwindow_set_inspector" rwindow_set_inspector :: Rid -> CInt -> IO ()

foreign import ccall safe "Rwindow_is_inspector_shown" rwindow_is_inspector_shown :: Rid -> IO CInt

-- | Returns: The window of the inspector of a window (to place it), or R_INVALID_ID when it is hidden.
foreign import ccall safe "Rwindow_get_inspector" rwindow_get_inspector :: Rid -> IO Rid

-- | Returns: R_TRUE for a window created with R_WINDOW_SPLASH_SCREEN (IsSplashScreen in XML).
foreign import ccall safe "Rwindow_is_splash_screen" rwindow_is_splash_screen :: Rid -> IO CInt

-- | Asks for a new frame even if no widget changed. Thread-safe.
foreign import ccall safe "Rwindow_request_redraw" rwindow_request_redraw :: Rid -> IO ()

-- | R_TRUE redraws the window every frame (games, live viewports).
foreign import ccall safe "Rwindow_set_continuous_redraw" rwindow_set_continuous_redraw :: Rid -> CInt -> IO ()

foreign import ccall safe "Rwindow_set_close_callback" rwindow_set_close_callback :: Rid -> RWindowCloseCallback -> Ptr () -> IO ()

foreign import ccall safe "Rwindow_set_resize_callback" rwindow_set_resize_callback :: Rid -> RWindowResizeCallback -> Ptr () -> IO ()

-- | Takes what is dropped on the window from other applications,
-- where no widget takes it (Rwidget_set_drop_callback). NULL removes it.
foreign import ccall safe "Rwindow_set_drop_callback" rwindow_set_drop_callback :: Rid -> RDropCallback -> Ptr () -> IO ()

-- | During R_DROP_ENTER / R_DROP_MOVE only: choose Raven's highlight
-- in target-local logical units (window-local for a window callback).
-- Clipped to the target; zero width or height hides it without refusing
-- the drop. Each callback starts with the default highlight again.
-- Dimensions must be finite and nonnegative; coordinates must be finite.
foreign import ccall safe "Rdrop_set_highlight_rect" rdrop_set_highlight_rect :: CFloat -> CFloat -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rwindow_set_user_handle" rwindow_set_user_handle :: Rid -> Ptr () -> IO ()

foreign import ccall safe "Rwindow_get_user_handle" rwindow_get_user_handle :: Rid -> IO (Ptr ())

-- | Fills `native` with the OS handles of the window.
foreign import ccall safe "Rwindow_get_native_handle" rwindow_get_native_handle :: Rid -> Ptr RNativeWindow -> IO CInt

-- | Reads back the last rendered frame as tightly packed RGBA8 pixels.
-- Blocks until the frame is rendered. Free the result with Rpixels_free.
-- The alpha is 255, except for a window that is seen through
-- (R_WINDOW_TRANSPARENT): its pixels are then premultiplied by their
-- alpha, as the window system receives them.
-- Returns: NULL when the backend cannot capture.
foreign import ccall safe "Rwindow_capture" rwindow_capture :: Rid -> Ptr CInt -> Ptr CInt -> IO (Ptr Word8)

-- | Widget that has the keyboard focus in the window, or R_INVALID_ID.
foreign import ccall safe "Rwindow_get_focused_widget" rwindow_get_focused_widget :: Rid -> IO Rid

-- | Deepest widget under the cursor, or R_INVALID_ID.
foreign import ccall safe "Rwindow_get_hovered_widget" rwindow_get_hovered_widget :: Rid -> IO Rid

foreign import ccall safe "Rwindow_get_mouse_position" rwindow_get_mouse_position :: Rid -> Ptr CFloat -> Ptr CFloat -> IO ()

-- | Inject input as if it came from the OS. Positions are in logical units.
-- Useful for automated tests, replays and when Raven is driven by a host
-- application.
foreign import ccall safe "Rwindow_inject_mouse_move" rwindow_inject_mouse_move :: Rid -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rwindow_inject_mouse_button" rwindow_inject_mouse_button :: Rid -> RMouseButton -> CInt -> IO ()

foreign import ccall safe "Rwindow_inject_mouse_wheel" rwindow_inject_mouse_wheel :: Rid -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rwindow_inject_key" rwindow_inject_key :: Rid -> RKey -> CInt -> Word32 -> IO ()

foreign import ccall safe "Rwindow_inject_text" rwindow_inject_text :: Rid -> CString -> IO ()

-- | A drag from another application moves over the window
-- (`inside` R_TRUE) or leaves it.
-- Returns: R_TRUE when a drop at this position would be accepted.
foreign import ccall safe "Rwindow_inject_drag" rwindow_inject_drag :: Rid -> CFloat -> CFloat -> CInt -> IO CInt

-- | Drops files and / or text at a position. Returns R_TRUE when a target took them.
foreign import ccall safe "Rwindow_inject_drop" rwindow_inject_drop :: Rid -> CFloat -> CFloat -> Ptr CString -> CInt -> CString -> IO CInt

-- | R_TRUE while the key is held down in the window (cleared when the window loses the keyboard).
foreign import ccall safe "Rwindow_is_key_down" rwindow_is_key_down :: Rid -> RKey -> IO CInt

-- | R_TRUE while the mouse button is held down over (or captured by) the window.
foreign import ccall safe "Rwindow_is_mouse_button_down" rwindow_is_mouse_button_down :: Rid -> RMouseButton -> IO CInt

-- | Modifiers (RModifier) of the last input event.
foreign import ccall safe "Rwindow_get_modifiers" rwindow_get_modifiers :: Rid -> IO Word32

-- | R_TRUE while the pointer is over the client area.
foreign import ccall safe "Rwindow_is_mouse_inside" rwindow_is_mouse_inside :: Rid -> IO CInt

-- | R_TRUE when the keyboard focus is in a text input, spin box or text editor.
foreign import ccall safe "Rwindow_wants_text_input" rwindow_wants_text_input :: Rid -> IO CInt

-- | R_TRUE while a menu, list or dialog is open over the window.
foreign import ccall safe "Rwindow_has_popup" rwindow_has_popup :: Rid -> IO CInt

-- | Last painted caret in window pixels (x, y, width, height), including scrolling and scale.
-- Call after Rdraw_frame. Independent of the blink phase. Returns R_FALSE and zeros the
-- outputs when there is no visible editable caret, the window is inactive, or focus changed
-- since painting. Any output pointer may be NULL.
foreign import ccall safe "Rwindow_get_text_input_rect" rwindow_get_text_input_rect :: Rid -> Ptr CInt -> Ptr CInt -> Ptr CInt -> Ptr CInt -> IO CInt

-- | Returns: Number of open in-window popups (menus, combo lists, pickers), bottom to top.
foreign import ccall safe "Rwindow_get_popup_count" rwindow_get_popup_count :: Rid -> IO CInt

-- | Returns: Root widget of popup `index`, whose rect is in window coordinates; R_INVALID_ID out of range.
foreign import ccall safe "Rwindow_get_popup" rwindow_get_popup :: Rid -> CInt -> IO Rid

-- | R_FALSE: Tab no longer moves the keyboard focus (the application uses it as a shortcut). Default R_TRUE.
foreign import ccall safe "Rwindow_set_tab_navigation" rwindow_set_tab_navigation :: Rid -> CInt -> IO ()

foreign import ccall safe "Rwindow_set_event_hook" rwindow_set_event_hook :: Rid -> RWindowEventHook -> Ptr () -> IO ()

-- | Starts dragging `data` from `source` (call it while a mouse button
-- is held, typically from a mouse-move event once the pointer has moved
-- a few units). Until the button is released, the widgets with a drop
-- callback (Rwidget_set_drop_callback) under the pointer receive
-- R_DROP_ENTER / MOVE / LEAVE and, on release, R_DROP_DROPPED, with
-- `drag_type` set and `text` = `data`. Returning R_FALSE from the callback
-- refuses the drop there. Escape cancels. `label` follows the pointer.
-- Returns: R_FALSE when the widget is not in a window.
foreign import ccall safe "Rdrag_begin" rdrag_begin :: Rid -> CString -> CString -> CString -> IO CInt

-- | R_TRUE while a drag started by Rdrag_begin is in progress in the window.
foreign import ccall safe "Rdrag_is_active" rdrag_is_active :: Rid -> IO CInt

-- | Type of the drag in progress, or NULL.
foreign import ccall safe "Rdrag_get_type" rdrag_get_type :: Rid -> IO CString

-- | Data of the drag in progress, or NULL.
foreign import ccall safe "Rdrag_get_data" rdrag_get_data :: Rid -> IO CString

-- | Ends the drag in progress without dropping (the target receives R_DROP_LEAVE).
foreign import ccall safe "Rdrag_cancel" rdrag_cancel :: Rid -> IO ()

foreign import ccall safe "Rwindow_register_shortcut" rwindow_register_shortcut :: Rid -> RKey -> Word32 -> CString -> IO ()

foreign import ccall safe "Rwindow_unregister_shortcut" rwindow_unregister_shortcut :: Rid -> RKey -> Word32 -> IO ()

-- | The settings every window follows, unless it has its own.
foreign import ccall safe "Rtouch_get_settings" rtouch_get_settings :: Ptr RTouchSettings -> IO ()

-- | Changes them; NULL puts back the defaults.
foreign import ccall safe "Rtouch_set_settings" rtouch_set_settings :: Ptr RTouchSettings -> IO ()

-- | The defaults, to start from.
foreign import ccall safe "Rtouch_get_default_settings" rtouch_get_default_settings :: Ptr RTouchSettings -> IO ()

-- | Settings of one window; NULL: it follows Rtouch_set_settings again.
foreign import ccall safe "Rwindow_set_touch_settings" rwindow_set_touch_settings :: Rid -> Ptr RTouchSettings -> IO ()

-- | The settings the window follows (its own, or those of the application).
foreign import ccall safe "Rwindow_get_touch_settings" rwindow_get_touch_settings :: Rid -> Ptr RTouchSettings -> IO ()

-- | What a finger does on the widget and what it contains (default R_TOUCH_MODE_AUTO: as its parent says).
foreign import ccall safe "Rwidget_set_touch_mode" rwidget_set_touch_mode :: Rid -> RTouchMode -> IO ()

foreign import ccall safe "Rwidget_get_touch_mode" rwidget_get_touch_mode :: Rid -> IO RTouchMode

-- | R_FALSE: two fingers (on the screen or a trackpad) neither zoom nor pan the widget (default R_TRUE).
foreign import ccall safe "Rwidget_set_touch_gestures" rwidget_set_touch_gestures :: Rid -> CInt -> IO ()

foreign import ccall safe "Rwidget_get_touch_gestures" rwidget_get_touch_gestures :: Rid -> IO CInt

-- | R_FALSE: the pen is not a mouse on the widget and what it contains; only its pen callback sees it (default R_TRUE).
foreign import ccall safe "Rwidget_set_pen_mouse" rwidget_set_pen_mouse :: Rid -> CInt -> IO ()

foreign import ccall safe "Rwidget_get_pen_mouse" rwidget_get_pen_mouse :: Rid -> IO CInt

foreign import ccall safe "Rwidget_set_touch_callback" rwidget_set_touch_callback :: Rid -> RTouchCallback -> Ptr () -> IO ()

foreign import ccall safe "Rwidget_set_pen_callback" rwidget_set_pen_callback :: Rid -> RPenCallback -> Ptr () -> IO ()

foreign import ccall safe "Rwidget_set_gesture_callback" rwidget_set_gesture_callback :: Rid -> RGestureCallback -> Ptr () -> IO ()

-- | Fingers on the window now.
foreign import ccall safe "Rwindow_get_touch_count" rwindow_get_touch_count :: Rid -> IO CInt

-- | R_TRUE while a pen is near the screen over the window.
foreign import ccall safe "Rwindow_is_pen_in_range" rwindow_is_pen_in_range :: Rid -> IO CInt

-- | R_TRUE when the pointer last used in the window was a finger (the interface may then be made larger).
foreign import ccall safe "Rwindow_is_touch_active" rwindow_is_touch_active :: Rid -> IO CInt

-- | A finger: R_TOUCH_DOWN, then R_TOUCH_MOVE..., then R_TOUCH_UP, with the same `id`.
foreign import ccall safe "Rwindow_inject_touch" rwindow_inject_touch :: Rid -> CInt -> RTouchPhase -> CFloat -> CFloat -> IO ()

-- | A pen: `pressure` 0..1 as the pen would give it, `buttons` a bitmask of RPenButton.
foreign import ccall safe "Rwindow_inject_pen" rwindow_inject_pen :: Rid -> RPenPhase -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> Word32 -> CInt -> IO ()

-- | A gesture of a trackpad, as the system recognizes it: R_GESTURE_BEGIN,
-- R_GESTURE_UPDATE..., then R_GESTURE_END. (x, y) is the pointer, `scale_delta`
-- and `rotation_delta` (degrees, clockwise) what changed since the previous
-- event, `pan_x`, `pan_y` how far the fingers moved since then.
foreign import ccall safe "Rwindow_inject_trackpad_gesture" rwindow_inject_trackpad_gesture :: Rid -> RGestureKind -> RGesturePhase -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CInt -> IO ()

-- | Two fingers scrolling on a trackpad: (dx, dy) is how far they moved
-- (logical units), the content following them. Sent as a precise wheel
-- (REvent::wheel_precise) at the pointer.
foreign import ccall safe "Rwindow_inject_trackpad_scroll" rwindow_inject_trackpad_scroll :: Rid -> CFloat -> CFloat -> IO ()

-- | The fingers that scrolled left the trackpad, on a system that
-- leaves the inertia to the application (Wayland): flicked content keeps
-- going (Touch.Inertia).
foreign import ccall safe "Rwindow_inject_trackpad_scroll_end" rwindow_inject_trackpad_scroll_end :: Rid -> IO ()

-- | Advances the inertia of flicked content by `seconds` at once,
-- and fires a long press that is due, without waiting (tests).
foreign import ccall safe "Rwindow_advance_touch" rwindow_advance_touch :: Rid -> CDouble -> IO ()

-- | Changes the look of the application. The theme file in use, if
-- it names no look of its own, is laid again over the new one; colors
-- and metrics changed with Rtheme_set_... are lost; what was given to
-- widgets (Rwidget_set_color...) is kept.
--
--   Rlook_set(R_LOOK_NATIVE_LIGHT);   the look of the platform, light
--
-- Theme files that name no look (no Look attribute) follow it too, so a
-- part of a window given one (Theme="paper.xml") changes with it.
foreign import ccall safe "Rlook_set" rlook_set :: RLook -> IO ()

-- | The look given to Rlook_set (R_LOOK_RAVEN_DARK at start).
foreign import ccall safe "Rlook_get" rlook_get :: IO RLook

-- | @deprecated Rlook_set: a built-in theme is a look of Raven.
foreign import ccall safe "Rtheme_apply" rtheme_apply :: RTheme -> IO ()

foreign import ccall safe "Rtheme_set_color" rtheme_set_color :: RColorRole -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rtheme_get_color" rtheme_get_color :: RColorRole -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> IO ()

foreign import ccall safe "Rtheme_set_metric" rtheme_set_metric :: RMetric -> CFloat -> IO ()

foreign import ccall safe "Rtheme_get_metric" rtheme_get_metric :: RMetric -> IO CFloat

-- | Material of every widget that was not given one with
-- Rwidget_set_material. Rtheme_apply sets it: solid for the flat
-- themes, frosted glass for the glass themes.
foreign import ccall safe "Rtheme_set_material" rtheme_set_material :: RMaterial -> IO ()

foreign import ccall safe "Rtheme_get_material" rtheme_get_material :: IO RMaterial

-- | Font used by widgets that have no font of their own.
foreign import ccall safe "Rtheme_set_font" rtheme_set_font :: Rid -> IO ()

foreign import ccall safe "Rtheme_get_font" rtheme_get_font :: IO Rid

-- | Sets the accent color and everything that follows from it: its
-- hovered and pressed shades, the focus outline, the selection, the links.
foreign import ccall safe "Rtheme_set_accent" rtheme_set_accent :: CFloat -> CFloat -> CFloat -> IO ()

-- | Tells whether the theme is a dark one. It decides the tint of
-- the glass, the colors of the code and which wallpaper the glass themes
-- use. Rtheme_apply and the theme files set it.
foreign import ccall safe "Rtheme_set_dark" rtheme_set_dark :: CInt -> IO ()

foreign import ccall safe "Rtheme_is_dark" rtheme_is_dark :: IO CInt

-- | Image drawn behind the content of the windows that have no
-- background of their own, or R_INVALID_ID for none (the window color).
foreign import ccall safe "Rtheme_set_wallpaper" rtheme_set_wallpaper :: Rid -> IO ()

foreign import ccall safe "Rtheme_get_wallpaper" rtheme_get_wallpaper :: IO Rid

-- | Gives a color to the widgets of a type and / or of a class,
-- instead of the color of the theme.
-- widget      Tag of a type of widget ("Button"), or NULL for every type.
-- style_class A class (Rwidget_set_class, Class="..."), or NULL
--                    for the widgets of the type whatever their classes.
--
-- When several styles give the same value to a widget, the one with a
-- type and a class wins over the one with a class, which wins over the
-- one with a type; between equals, the last one set wins.
foreign import ccall safe "Rtheme_set_style_color" rtheme_set_style_color :: CString -> CString -> RColorRole -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rtheme_set_style_metric" rtheme_set_style_metric :: CString -> CString -> RMetric -> CFloat -> IO ()

-- | R_MATERIAL_DEFAULT removes the material of the style.
foreign import ccall safe "Rtheme_set_style_material" rtheme_set_style_material :: CString -> CString -> RMaterial -> IO ()

-- | Removes every style of the theme. Rtheme_apply and Rtheme_use do it too.
foreign import ccall safe "Rtheme_clear_styles" rtheme_clear_styles :: IO ()

-- | Reads a theme file and keeps the theme under its name (the Name
-- attribute, else the name of the file). It does not change the theme in
-- use unless a theme of that name is the one in use.
--
-- The file is watched like the interface files (Rxml_set_live_reload):
-- saving it updates every widget that uses the theme.
-- Returns: The name of the theme, valid until Rshutdown, or NULL after
-- reporting an error.
foreign import ccall safe "Rtheme_load" rtheme_load :: CString -> IO CString

-- | Same, from a text. `size` may be 0 for a null-terminated text.
foreign import ccall safe "Rtheme_load_from_memory" rtheme_load_from_memory :: CString -> CSize -> IO CString

-- | Makes a named theme the theme of the application: a theme
-- given by Rtheme_load (laid over the look in use when it names no
-- look), or the name of a look ("WindowsLight", "RavenDark"; the old
-- "Dark", "Light", "GlassDark"... too), which is then Rlook_set.
-- Values changed with Rtheme_set_... are lost.
-- Returns: R_FALSE, after reporting an error, when no theme has that name.
foreign import ccall safe "Rtheme_use" rtheme_use :: CString -> IO CInt

-- | Name of the theme in use ("Dark" at start).
foreign import ccall safe "Rtheme_get_name" rtheme_get_name :: IO CString

-- | Number of named themes, the four built-in ones included.
foreign import ccall safe "Rtheme_get_count" rtheme_get_count :: IO CInt

-- | Name of the index-th one, or NULL.
foreign import ccall safe "Rtheme_get_name_at" rtheme_get_name_at :: CInt -> IO CString

-- | Writes the theme in use, with every change made since it was
-- applied, as a theme file.
-- name Name given to the theme in the file; NULL keeps the current one.
foreign import ccall safe "Rtheme_save" rtheme_save :: CString -> CString -> IO CInt

-- | Creates a font from the content of a .ttf / .otf file.
-- The buffer is copied. Glyphs are rasterized on demand, at the exact
-- pixel size needed by each window scale, into a shared atlas.
-- Returns: R_INVALID_ID on failure.
foreign import ccall safe "Rfont_create" rfont_create :: CString -> CSize -> IO Rid

foreign import ccall safe "Rfont_create_from_file" rfont_create_from_file :: CString -> IO Rid

foreign import ccall safe "Rfont_destroy" rfont_destroy :: Rid -> IO ()

foreign import ccall safe "Rfont_is_valid" rfont_is_valid :: Rid -> IO CInt

-- | Returns: The font embedded in the library (Roboto, Latin and Latin Extended-A).
foreign import ccall safe "Rfont_get_default" rfont_get_default :: IO Rid

-- | Returns: The fixed-width font embedded in the library (Roboto Mono, same
-- characters), used by default by the text editors. In XML: Font="monospace".
foreign import ccall safe "Rfont_get_monospace" rfont_get_monospace :: IO Rid

-- | Font used for the characters missing from `font` (symbols, icons,
-- other scripts). Fallbacks can be chained.
foreign import ccall safe "Rfont_set_fallback" rfont_set_fallback :: Rid -> Rid -> IO ()

-- | Gives a font the font to use for one of its styles: the bold
-- file of the same family, its italic file. Formatted text (Markdown
-- labels, the Markdown of the text editor) uses them.
--
-- A font that was given none still has the three styles: Raven draws
-- its glyphs thicker for bold and slanted for italic, keeping their
-- advances, so that text does not move when its style changes.
-- variant R_INVALID_ID goes back to the style Raven derives.
foreign import ccall safe "Rfont_set_variant" rfont_set_variant :: Rid -> RFontStyle -> Rid -> IO ()

-- | Returns: The font drawn for a style of `font`: the one given with
-- Rfont_set_variant, else the one Raven derives from it (made on first
-- use; it lives as long as `font` and cannot be deleted by itself).
-- Usable wherever a font is: Rlabel_set_font, Rdraw_text...
foreign import ccall safe "Rfont_get_variant" rfont_get_variant :: Rid -> RFontStyle -> IO Rid

-- | Measures a UTF-8 string, in logical units, at scale 1.
-- font R_INVALID_ID uses the theme font.
-- size Font size; 0 uses R_METRIC_FONT_SIZE.
foreign import ccall safe "Rfont_measure_text" rfont_measure_text :: Rid -> CFloat -> CString -> Ptr CFloat -> Ptr CFloat -> IO ()

-- | Creates a texture from the content of an image file (PNG, JPEG,
-- BMP, TGA, GIF, PSD, SVG). Decoding happens on the calling thread.
-- Returns: R_INVALID_ID on failure.
foreign import ccall safe "Rtexture_create" rtexture_create :: CString -> CSize -> IO Rid

-- | Same as Rtexture_create, decoded on a worker thread. The handle
-- is usable at once; widgets showing it draw nothing until it is ready.
-- The buffer is copied.
foreign import ccall safe "Rtexture_create_async" rtexture_create_async :: CString -> CSize -> IO Rid

-- | Draws an SVG image (an icon) into a texture, at `scale` pixels per
-- unit of the SVG. The widgets show it at the size the SVG gives, whatever
-- the scale (Rtexture_get_density).
--
-- 0 or less: at the size it is shown. It is first drawn at a scale of 2;
-- once painted, it is drawn again (on a worker thread) with as many pixels
-- as the screen shows it with: the scale of the monitor of the window, an
-- Image that stretches it, a zoom. It follows when the window moves to a
-- screen of another density or the image grows, and comes down when it is
-- shown much smaller (sides of at most 4096 pixels). Its width and height
-- in pixels (Rtexture_get_size) change then; its size in logical units
-- does not. Rtexture_create, Rtexture_create_from_file and the asynchronous
-- ones read SVG files this way. Paths, fills, strokes and gradients are
-- drawn; text, filters and masks are not.
foreign import ccall safe "Rtexture_create_from_svg" rtexture_create_from_svg :: CString -> CSize -> CFloat -> IO Rid

-- | Pixels of the texture per logical unit: the scale of an SVG, 1 otherwise.
foreign import ccall safe "Rtexture_get_density" rtexture_get_density :: Rid -> IO CFloat

foreign import ccall safe "Rtexture_create_from_file" rtexture_create_from_file :: CString -> IO Rid

-- | Reads and decodes the file on a worker thread.
foreign import ccall safe "Rtexture_create_from_file_async" rtexture_create_from_file_async :: CString -> IO Rid

-- | Creates a texture from tightly packed RGBA8 pixels (straight alpha). Its filter is R_FILTER_LINEAR.
foreign import ccall safe "Rtexture_create_from_pixels" rtexture_create_from_pixels :: Ptr Word8 -> CInt -> CInt -> IO Rid

-- | Replaces the pixels of a texture. The size may change. Use it to
-- show frames produced on the CPU or read back from another renderer.
foreign import ccall safe "Rtexture_update" rtexture_update :: Rid -> Ptr Word8 -> CInt -> CInt -> IO ()

-- | Decodes an image file held in memory (PNG, JPEG, BMP, TGA) into
-- tightly packed RGBA8 pixels, without making a texture: for pictures
-- that arrive one after the other and go to Rtexture_update. Thread-safe,
-- so that the decoding can be done away from the UI thread.
-- Returns: The pixels, to release with Rpixels_free; NULL when the data
-- is not an image Raven can read.
foreign import ccall safe "Rimage_decode" rimage_decode :: Ptr () -> CSize -> Ptr CInt -> Ptr CInt -> IO (Ptr Word8)

-- | Releases pixels returned by Raven (Rimage_decode, Rwindow_capture). Thread-safe.
foreign import ccall safe "Rpixels_free" rpixels_free :: Ptr Word8 -> IO ()

-- | Returns: R_TRUE once an asynchronous texture is decoded (or failed).
foreign import ccall safe "Rtexture_is_ready" rtexture_is_ready :: Rid -> IO CInt

-- | Blocks until an asynchronous texture is ready.
foreign import ccall safe "Rtexture_wait" rtexture_wait :: Rid -> IO ()

foreign import ccall safe "Rtexture_destroy" rtexture_destroy :: Rid -> IO ()

foreign import ccall safe "Rtexture_is_valid" rtexture_is_valid :: Rid -> IO CInt

foreign import ccall safe "Rtexture_get_size" rtexture_get_size :: Rid -> Ptr CInt -> Ptr CInt -> IO ()

-- | How the texture is smoothed when it is scaled. The textures made
-- from image files start with R_FILTER_MIPMAP, the others with
-- R_FILTER_LINEAR.
foreign import ccall safe "Rtexture_set_filter" rtexture_set_filter :: Rid -> RFilterType -> IO ()

-- | Creates a widget. It is not shown until it is added to a parent
-- that belongs to a window (Rwidget_add_child, Rwindow_set_content).
foreign import ccall safe "Rwidget_create" rwidget_create :: RWidgetType -> IO Rid

-- | Deletes a widget and all its descendants.
foreign import ccall safe "Rwidget_destroy" rwidget_destroy :: Rid -> IO ()

foreign import ccall safe "Rwidget_is_valid" rwidget_is_valid :: Rid -> IO CInt

foreign import ccall safe "Rwidget_get_type" rwidget_get_type :: Rid -> IO RWidgetType

-- | Appends a child to a panel. The parent takes ownership.
-- A widget that already has a parent is moved.
-- Returns: `child`, so calls can be nested:
--         Rid ok = Rwidget_add_child(row, Rbutton_create("OK"));
foreign import ccall safe "Rwidget_add_child" rwidget_add_child :: Rid -> Rid -> IO Rid

foreign import ccall safe "Rwidget_insert_child" rwidget_insert_child :: Rid -> Rid -> CInt -> IO Rid

-- | Detaches a child without deleting it. The caller owns it again.
foreign import ccall safe "Rwidget_remove_child" rwidget_remove_child :: Rid -> Rid -> IO ()

-- | Deletes every child of a panel.
foreign import ccall safe "Rwidget_clear_children" rwidget_clear_children :: Rid -> IO ()

foreign import ccall safe "Rwidget_get_parent" rwidget_get_parent :: Rid -> IO Rid

foreign import ccall safe "Rwidget_get_child_count" rwidget_get_child_count :: Rid -> IO CInt

foreign import ccall safe "Rwidget_get_child" rwidget_get_child :: Rid -> CInt -> IO Rid

-- | Returns: The window showing the widget, or R_INVALID_ID.
foreign import ccall safe "Rwidget_get_window" rwidget_get_window :: Rid -> IO Rid

foreign import ccall safe "Rwidget_set_name" rwidget_set_name :: Rid -> CString -> IO ()

foreign import ccall safe "Rwidget_get_name" rwidget_get_name :: Rid -> IO CString

-- | Finds a widget by name.
-- scope A window or a widget; the search covers its whole subtree.
-- Returns: R_INVALID_ID when no widget has that name.
foreign import ccall safe "Rwidget_find" rwidget_find :: Rid -> CString -> IO Rid

-- | Space kept around the child, inside its slot. All panels.
foreign import ccall safe "Rslot_set_padding" rslot_set_padding :: Rid -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

-- | Alignment of the child inside its slot. Default: fill. All panels.
foreign import ccall safe "Rslot_set_halign" rslot_set_halign :: Rid -> RHAlign -> IO ()

foreign import ccall safe "Rslot_set_valign" rslot_set_valign :: Rid -> RVAlign -> IO ()

-- | The slot takes the desired size of the child along the panel
-- direction (Slate AutoWidth / AutoHeight). This is the default.
-- VBox, HBox.
foreign import ccall safe "Rslot_set_auto" rslot_set_auto :: Rid -> IO ()

-- | The slot shares the space left by the auto slots, in proportion
-- to `coefficient` (Slate FillWidth / FillHeight). VBox, HBox.
-- In a Splitter, the coefficient is the relative size of the pane.
foreign import ccall safe "Rslot_set_fill" rslot_set_fill :: Rid -> CFloat -> IO ()

-- | Upper bound of the slot along the panel direction. 0 = none. VBox, HBox.
foreign import ccall safe "Rslot_set_max_size" rslot_set_max_size :: Rid -> CFloat -> IO ()

-- | Cell of the child. Grid, UniformGrid.
foreign import ccall safe "Rslot_set_grid" rslot_set_grid :: Rid -> CInt -> CInt -> IO ()

foreign import ccall safe "Rslot_set_grid_span" rslot_set_grid_span :: Rid -> CInt -> CInt -> IO ()

-- | Canvas placement. Anchors are fractions of the canvas size (0..1).
--   - anchors equal on an axis: the child is at anchor + position, with
--     the slot size (or its desired size when the slot size is <= 0);
--   - anchors different on an axis: the child stretches between them,
--     position and size are then the near and far margins.
-- Alignment is the pivot of the child (0,0 = top-left, 0.5,0.5 = center).
foreign import ccall safe "Rslot_set_anchors" rslot_set_anchors :: Rid -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rslot_set_position" rslot_set_position :: Rid -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rslot_set_size" rslot_set_size :: Rid -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rslot_set_alignment" rslot_set_alignment :: Rid -> CFloat -> CFloat -> IO ()

-- | Title of the tab showing the child. TabView.
foreign import ccall safe "Rslot_set_title" rslot_set_title :: Rid -> CString -> IO ()

foreign import ccall safe "Rwidget_set_visibility" rwidget_set_visibility :: Rid -> RVisibility -> IO ()

foreign import ccall safe "Rwidget_get_visibility" rwidget_get_visibility :: Rid -> IO RVisibility

-- | A disabled widget ignores input and is drawn dimmed, with its descendants.
foreign import ccall safe "Rwidget_set_enabled" rwidget_set_enabled :: Rid -> CInt -> IO ()

foreign import ccall safe "Rwidget_is_enabled" rwidget_is_enabled :: Rid -> IO CInt

-- | R_FALSE lets mouse events pass through the widget (not its children).
foreign import ccall safe "Rwidget_set_hit_testable" rwidget_set_hit_testable :: Rid -> CInt -> IO ()

-- | Opacity multiplier applied to the widget and its descendants (0..1).
foreign import ccall safe "Rwidget_set_opacity" rwidget_set_opacity :: Rid -> CFloat -> IO ()

-- | Gives the widget a transition that plays each time it appears:
-- when its window is first drawn, when it is added to a window that is
-- shown, and when it becomes visible again (Rwidget_set_visibility, a
-- tab or a switcher showing it). In XML: Appear="FlyUp" AppearDelay="0.1".
-- delay_seconds Time before it starts. Give growing delays to the
-- items of a group to make them arrive one after the other.
foreign import ccall safe "Rwidget_set_transition" rwidget_set_transition :: Rid -> RTransition -> CFloat -> IO ()

-- | Plays a transition once, now, whatever the widget was given with Rwidget_set_transition.
foreign import ccall safe "Rwidget_play_transition" rwidget_play_transition :: Rid -> RTransition -> CFloat -> IO ()

-- | R_TRUE clips the descendants to the bounds of the widget.
foreign import ccall safe "Rwidget_set_clipping" rwidget_set_clipping :: Rid -> CInt -> IO ()

-- | Text shown when the cursor rests on the widget. NULL removes it.
foreign import ccall safe "Rwidget_set_tooltip" rwidget_set_tooltip :: Rid -> CString -> IO ()

-- | Returns: The tooltip text of the widget, "" when it has none.
foreign import ccall safe "Rwidget_get_tooltip" rwidget_get_tooltip :: Rid -> IO CString

foreign import ccall safe "Rwidget_set_cursor" rwidget_set_cursor :: Rid -> RCursor -> IO ()

-- | Size constraints, in logical units. They apply to the desired size the
-- widget reports to its parent. A negative value removes the constraint
-- on that axis.
foreign import ccall safe "Rwidget_set_size" rwidget_set_size :: Rid -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rwidget_set_min_size" rwidget_set_min_size :: Rid -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rwidget_set_max_size" rwidget_set_max_size :: Rid -> CFloat -> CFloat -> IO ()

-- | Bounds of the widget in its window, in logical units, after the last layout.
foreign import ccall safe "Rwidget_get_rect" rwidget_get_rect :: Rid -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> IO ()

foreign import ccall safe "Rwidget_get_desired_size" rwidget_get_desired_size :: Rid -> Ptr CFloat -> Ptr CFloat -> IO ()

foreign import ccall safe "Rwidget_is_hovered" rwidget_is_hovered :: Rid -> IO CInt

foreign import ccall safe "Rwidget_set_focusable" rwidget_set_focusable :: Rid -> CInt -> IO ()

-- | Gives the keyboard focus to the widget.
foreign import ccall safe "Rwidget_focus" rwidget_focus :: Rid -> IO ()

foreign import ccall safe "Rwidget_is_focused" rwidget_is_focused :: Rid -> IO CInt

foreign import ccall safe "Rwidget_set_user_handle" rwidget_set_user_handle :: Rid -> Ptr () -> IO ()

foreign import ccall safe "Rwidget_get_user_handle" rwidget_get_user_handle :: Rid -> IO (Ptr ())

foreign import ccall safe "Rwidget_set_destroy_callback" rwidget_set_destroy_callback :: Rid -> RWidgetDestroyCallback -> Ptr () -> IO ()

foreign import ccall safe "Rwidget_set_event_callback" rwidget_set_event_callback :: Rid -> RWidgetEventCallback -> Ptr () -> IO ()

-- | Name of the action fired by the main event of the widget: button
-- click, value change, text commit, item activation (see Raction_bind).
foreign import ccall safe "Rwidget_set_action" rwidget_set_action :: Rid -> CString -> IO ()

foreign import ccall safe "Rwidget_get_action" rwidget_get_action :: Rid -> IO CString

-- | Makes the widget a target for what is dragged from other
-- applications: files from the file manager, text. It is highlighted
-- while a drop on it would be accepted. NULL removes the callback.
foreign import ccall safe "Rwidget_set_drop_callback" rwidget_set_drop_callback :: Rid -> RDropCallback -> Ptr () -> IO ()

-- | In a window created with R_WINDOW_CUSTOM_HEADER: dragging the
-- widget moves the window and a double click maximizes it, as a title bar
-- does. The buttons, fields and other controls inside it keep working.
-- In XML: WindowDrag="true".
foreign import ccall safe "Rwidget_set_window_drag" rwidget_set_window_drag :: Rid -> CInt -> IO ()

-- | Deepest widget of a subtree under a point of its window, or R_INVALID_ID.
foreign import ccall safe "Rwidget_hit_test" rwidget_hit_test :: Rid -> CFloat -> CFloat -> IO Rid

-- | Deepest visible widget of a subtree under a point of its window,
-- whether it reacts to the pointer or not: a label, a panel, a disabled
-- button. What a design tool selects with a click.
foreign import ccall safe "Rwidget_pick" rwidget_pick :: Rid -> CFloat -> CFloat -> IO Rid

-- | Menu opened by a right click on the widget. The widget does not own it.
foreign import ccall safe "Rwidget_set_context_menu" rwidget_set_context_menu :: Rid -> Rid -> IO ()

-- | Asks for the widget to be laid out and painted again.
foreign import ccall safe "Rwidget_invalidate" rwidget_invalidate :: Rid -> IO ()

-- | Overrides a theme color for one widget. R_COLOR_TEXT is also
-- inherited by its descendants.
foreign import ccall safe "Rwidget_set_color" rwidget_set_color :: Rid -> RColorRole -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rwidget_clear_color" rwidget_clear_color :: Rid -> RColorRole -> IO ()

-- | The color the widget is drawn with for a role: its own, else
-- the one of the styles of its theme that apply to it, else the one of
-- its theme. What a custom widget should draw itself with.
foreign import ccall safe "Rwidget_get_color" rwidget_get_color :: Rid -> RColorRole -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> IO ()

-- | Gives one widget a material of its own, whatever the theme:
-- a solid button in a glass window, a panel of clear glass in a flat one.
-- A widget that draws no surface (a box, a label) gets one of this
-- material behind it: any widget can become a panel of glass.
-- In XML: Material="Glass | FrostedGlass | TintedGlass | Solid".
-- material R_MATERIAL_DEFAULT follows the theme again.
foreign import ccall safe "Rwidget_set_material" rwidget_set_material :: Rid -> RMaterial -> IO ()

-- | Returns: The material the widget is drawn with: its own, or the one of the theme.
foreign import ccall safe "Rwidget_get_material" rwidget_get_material :: Rid -> IO RMaterial

-- | Overrides a theme metric for one widget.
foreign import ccall safe "Rwidget_set_metric" rwidget_set_metric :: Rid -> RMetric -> CFloat -> IO ()

foreign import ccall safe "Rwidget_clear_metric" rwidget_clear_metric :: Rid -> RMetric -> IO ()

-- | Returns: The metric the widget uses: its own, else the one of a style, else the one of its theme.
foreign import ccall safe "Rwidget_get_metric" rwidget_get_metric :: Rid -> RMetric -> IO CFloat

-- | Classes of the widget, separated by spaces ("danger large"):
-- the styles of the theme made for these classes apply to it
-- (Rtheme_set_style_color, <Style Class="danger" .../> in a theme file).
-- In XML: Class="...". NULL or "" removes them.
foreign import ccall safe "Rwidget_set_class" rwidget_set_class :: Rid -> CString -> IO ()

-- | Returns: The classes, "" when it has none. Valid until they change.
foreign import ccall safe "Rwidget_get_class" rwidget_get_class :: Rid -> IO CString

foreign import ccall safe "Rwidget_add_class" rwidget_add_class :: Rid -> CString -> IO ()

foreign import ccall safe "Rwidget_remove_class" rwidget_remove_class :: Rid -> CString -> IO ()

foreign import ccall safe "Rwidget_has_class" rwidget_has_class :: Rid -> CString -> IO CInt

-- | Gives the widget and everything inside it a theme of their own,
-- by name ("Dark", "GlassLight", or a theme of Rtheme_load): a dark side
-- bar in a light window, a preview of another theme. Menus and lists
-- opened from inside follow. In XML: Theme="...".
-- name NULL or "" follows the parent (the application theme) again.
-- Returns: R_FALSE, after reporting an error, when no theme has that name.
foreign import ccall safe "Rwidget_set_theme" rwidget_set_theme :: Rid -> CString -> IO CInt

-- | Returns: Name of the theme the widget is drawn with.
foreign import ccall safe "Rwidget_get_theme" rwidget_get_theme :: Rid -> IO CString

-- | Gives the widget and everything inside it a font, for the texts
-- that have none of their own (a label given Rlabel_set_font keeps its
-- own): a tool whose panels wear one font while the interface it shows
-- keeps the font of the application. Menus and lists opened from inside
-- follow.
-- font R_INVALID_ID: the font of the parent (in the end, the one of the theme, Rtheme_set_font) again.
foreign import ccall safe "Rwidget_set_font" rwidget_set_font :: Rid -> Rid -> IO ()

-- | Returns: The font given to the widget itself, or R_INVALID_ID.
foreign import ccall safe "Rwidget_get_font" rwidget_get_font :: Rid -> IO Rid

-- | Gap inserted between neighbouring children.
foreign import ccall safe "Rpanel_set_spacing" rpanel_set_spacing :: Rid -> CFloat -> IO ()

foreign import ccall safe "Rpanel_get_spacing" rpanel_get_spacing :: Rid -> IO CFloat

-- | Lets a column / row grow to take a share of the free space
-- (Slate FillColumn / FillRow). 0 sizes it to its content (default).
foreign import ccall safe "Rgrid_set_column_fill" rgrid_set_column_fill :: Rid -> CInt -> CFloat -> IO ()

foreign import ccall safe "Rgrid_set_row_fill" rgrid_set_row_fill :: Rid -> CInt -> CFloat -> IO ()

-- | Places children automatically, left to right then top to bottom,
-- in the given number of columns. 0 uses the grid cell of each slot (default).
foreign import ccall safe "Runiform_grid_set_columns" runiform_grid_set_columns :: Rid -> CInt -> IO ()

foreign import ccall safe "Runiform_grid_set_min_cell_size" runiform_grid_set_min_cell_size :: Rid -> CFloat -> CFloat -> IO ()

-- | Space between the outline and the child.
foreign import ccall safe "Rborder_set_padding" rborder_set_padding :: Rid -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

-- | Background color (R_COLOR_PANEL of this widget). Alpha 0 draws no background.
foreign import ccall safe "Rborder_set_background" rborder_set_background :: Rid -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

-- | Outline width and color. Width 0 draws no outline (default).
foreign import ccall safe "Rborder_set_outline" rborder_set_outline :: Rid -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rborder_set_corner_radius" rborder_set_corner_radius :: Rid -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

-- | Soft drop shadow behind the border. Blur 0 disables it (default).
foreign import ccall safe "Rborder_set_shadow" rborder_set_shadow :: Rid -> CFloat -> CFloat -> CFloat -> IO ()

-- | What the border is made of. R_MATERIAL_GLASS turns it into a
-- panel of glass tinted by its background color (the alpha of the color
-- is the strength of the tint): floating tool bars, side bars, cards.
foreign import ccall safe "Rborder_set_material" rborder_set_material :: Rid -> RMaterial -> IO ()

-- | Keeps the child at a fixed width / height ratio, as large as fits.
-- 0 disables it (default). The fixed, minimum and maximum sizes are set
-- with Rwidget_set_size, Rwidget_set_min_size and Rwidget_set_max_size.
foreign import ccall safe "Rsize_box_set_aspect_ratio" rsize_box_set_aspect_ratio :: Rid -> CFloat -> IO ()

foreign import ccall safe "Rspacer_set_size" rspacer_set_size :: Rid -> CFloat -> CFloat -> IO ()

-- | Direction in which children are stacked and scrolled. Default: R_VERTICAL.
foreign import ccall safe "Rscroll_box_set_orientation" rscroll_box_set_orientation :: Rid -> ROrientation -> IO ()

-- | R_TRUE also scrolls across the stacking direction when the content is too large.
foreign import ccall safe "Rscroll_box_set_scroll_both" rscroll_box_set_scroll_both :: Rid -> CInt -> IO ()

foreign import ccall safe "Rscroll_box_set_offset" rscroll_box_set_offset :: Rid -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rscroll_box_get_offset" rscroll_box_get_offset :: Rid -> Ptr CFloat -> Ptr CFloat -> IO ()

foreign import ccall safe "Rscroll_box_scroll_to_end" rscroll_box_scroll_to_end :: Rid -> IO ()

-- | Scrolls every ScrollBox above the widget so that the widget is visible.
foreign import ccall safe "Rwidget_scroll_into_view" rwidget_scroll_into_view :: Rid -> IO ()

-- | R_HORIZONTAL puts the panes side by side (default), R_VERTICAL stacks them.
foreign import ccall safe "Rsplitter_set_orientation" rsplitter_set_orientation :: Rid -> ROrientation -> IO ()

foreign import ccall safe "Rsplitter_set_min_pane_size" rsplitter_set_min_pane_size :: Rid -> CFloat -> IO ()

-- | Creates a panel that shows what is behind it out of focus,
-- with one child over it. In XML:
--
--   <BackgroundBlur Style="Frosted" Radius="24" CornerRadius="18" Padding="16">
--       <Label>Over a blurred view of the window</Label>
--   </BackgroundBlur>
--
-- It works in any theme and with any material: it is not the glass of
-- the glass themes (which bends and reflects), but the looks of
-- RBlurStyle. Unlike the opacity of a widget, it does not let what is
-- behind show sharp.
foreign import ccall safe "Rbackground_blur_create" rbackground_blur_create :: IO Rid

foreign import ccall safe "Rbackground_blur_set_style" rbackground_blur_set_style :: Rid -> RBlurStyle -> IO ()

foreign import ccall safe "Rbackground_blur_get_style" rbackground_blur_get_style :: Rid -> IO RBlurStyle

-- | How far what is behind goes out of focus, in logical units:
-- 4 softens, 20 hides the details, 60 leaves only colors. A negative
-- value (the default) uses R_METRIC_BLUR_RADIUS of the theme.
foreign import ccall safe "Rbackground_blur_set_radius" rbackground_blur_set_radius :: Rid -> CFloat -> IO ()

foreign import ccall safe "Rbackground_blur_get_radius" rbackground_blur_get_radius :: Rid -> IO CFloat

-- | Color laid over the blur; its alpha is how much of it. Without
-- one, R_BLUR_PLAIN has none and the other styles take theirs from the
-- theme (R_COLOR_PANEL).
foreign import ccall safe "Rbackground_blur_set_tint" rbackground_blur_set_tint :: Rid -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

-- | Removes the tint given with Rbackground_blur_set_tint.
foreign import ccall safe "Rbackground_blur_clear_tint" rbackground_blur_clear_tint :: Rid -> IO ()

-- | Saturation of what is seen through: 1 = as it is, 0 = gray, 1.5 = vivid. Negative = the one of the style.
foreign import ccall safe "Rbackground_blur_set_saturation" rbackground_blur_set_saturation :: Rid -> CFloat -> IO ()

-- | Fine grain over the blur, 0 = none .. 1. Negative = the one of the style.
foreign import ccall safe "Rbackground_blur_set_grain" rbackground_blur_set_grain :: Rid -> CFloat -> IO ()

foreign import ccall safe "Rbackground_blur_set_corner_radius" rbackground_blur_set_corner_radius :: Rid -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

-- | Space between the edge and the child.
foreign import ccall safe "Rbackground_blur_set_padding" rbackground_blur_set_padding :: Rid -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

-- | Soft shadow under the panel; blur 0 = none (default).
foreign import ccall safe "Rbackground_blur_set_shadow" rbackground_blur_set_shadow :: Rid -> CFloat -> IO ()

foreign import ccall safe "Rswitcher_set_index" rswitcher_set_index :: Rid -> CInt -> IO ()

foreign import ccall safe "Rswitcher_get_index" rswitcher_get_index :: Rid -> IO CInt

-- | Adds a page. Same as Rwidget_add_child followed by Rslot_set_title.
-- Returns: The index of the new tab, or -1.
foreign import ccall safe "Rtab_view_add_tab" rtab_view_add_tab :: Rid -> CString -> Rid -> IO CInt

foreign import ccall safe "Rtab_view_set_active" rtab_view_set_active :: Rid -> CInt -> IO ()

foreign import ccall safe "Rtab_view_get_active" rtab_view_get_active :: Rid -> IO CInt

foreign import ccall safe "Rtab_view_set_changed_callback" rtab_view_set_changed_callback :: Rid -> RTabChangedCallback -> Ptr () -> IO ()

-- | Documents mode: the major tabs of an application (the level,
-- then one tab per open asset, as in Unreal): tabs on the left with an
-- icon and a close button (a middle click closes too), shrinking when
-- they do not fit, no slide between pages. XML: Documents="true".
foreign import ccall safe "Rtab_view_set_documents" rtab_view_set_documents :: Rid -> CInt -> IO ()

foreign import ccall safe "Rtab_view_get_tab_count" rtab_view_get_tab_count :: Rid -> IO CInt

-- | The page of a tab, R_INVALID_ID if none.
foreign import ccall safe "Rtab_view_get_tab" rtab_view_get_tab :: Rid -> CInt -> IO Rid

-- | Index of the tab of a page, -1 if none.
foreign import ccall safe "Rtab_view_find_tab" rtab_view_find_tab :: Rid -> Rid -> IO CInt

foreign import ccall safe "Rtab_view_set_tab_title" rtab_view_set_tab_title :: Rid -> Rid -> CString -> IO ()

-- | Icon before the title (documents). R_INVALID_ID removes it; tint: a mask in the text color.
foreign import ccall safe "Rtab_view_set_tab_icon" rtab_view_set_tab_icon :: Rid -> Rid -> Rid -> CFloat -> CFloat -> CFloat -> CFloat -> CInt -> IO ()

-- | R_FALSE: no close button (documents ; the level tab). Tabs can be closed by default.
foreign import ccall safe "Rtab_view_set_tab_closable" rtab_view_set_tab_closable :: Rid -> Rid -> CInt -> IO ()

-- | Shows the tab of a page (the changed callback is called).
foreign import ccall safe "Rtab_view_show_tab" rtab_view_show_tab :: Rid -> Rid -> IO ()

foreign import ccall safe "Rtab_view_set_close_callback" rtab_view_set_close_callback :: Rid -> RTabCloseCallback -> Ptr () -> IO ()

foreign import ccall safe "Rexpander_set_title" rexpander_set_title :: Rid -> CString -> IO ()

foreign import ccall safe "Rexpander_get_title" rexpander_get_title :: Rid -> IO CString

foreign import ccall safe "Rexpander_set_expanded" rexpander_set_expanded :: Rid -> CInt -> IO ()

foreign import ccall safe "Rexpander_is_expanded" rexpander_is_expanded :: Rid -> IO CInt

foreign import ccall safe "Rexpander_set_changed_callback" rexpander_set_changed_callback :: Rid -> RExpanderChangedCallback -> Ptr () -> IO ()

-- | Direction of every window: R_DIRECTION_AUTO (the default), _LTR or _RTL.
foreign import ccall safe "Rset_layout_direction" rset_layout_direction :: RDirection -> IO ()

-- | The direction the windows use now: R_DIRECTION_LTR or _RTL.
foreign import ccall safe "Rget_layout_direction" rget_layout_direction :: IO RDirection

-- | Direction of a part of a window: R_DIRECTION_INHERIT, _LTR or _RTL.
foreign import ccall safe "Rwidget_set_direction" rwidget_set_direction :: Rid -> RDirection -> IO ()

-- | The direction the widget is laid out in: R_DIRECTION_LTR or _RTL.
foreign import ccall safe "Rwidget_get_direction" rwidget_get_direction :: Rid -> IO RDirection

-- | Shortcut for Rwidget_create(R_WIDGET_LABEL) + Rlabel_set_text.
foreign import ccall safe "Rlabel_create" rlabel_create :: CString -> IO Rid

-- | Sets the text. '\n' starts a new line.
foreign import ccall safe "Rlabel_set_text" rlabel_set_text :: Rid -> CString -> IO ()

foreign import ccall safe "Rlabel_get_text" rlabel_get_text :: Rid -> IO CString

-- | R_INVALID_ID uses the theme font.
foreign import ccall safe "Rlabel_set_font" rlabel_set_font :: Rid -> Rid -> IO ()

-- | Font size in logical units. 0 uses R_METRIC_FONT_SIZE.
foreign import ccall safe "Rlabel_set_text_size" rlabel_set_text_size :: Rid -> CFloat -> IO ()

foreign import ccall safe "Rlabel_set_text_align" rlabel_set_text_align :: Rid -> RTextAlign -> IO ()

-- | R_TRUE wraps the text on the width given by the parent.
foreign import ccall safe "Rlabel_set_wrap" rlabel_set_wrap :: Rid -> CInt -> IO ()

-- | Text color (R_COLOR_TEXT of this widget).
foreign import ccall safe "Rlabel_set_color" rlabel_set_color :: Rid -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

-- | R_TRUE reads the text as Markdown and shows it formatted
-- (Markdown="true" in XML):
--
--   # titles, **strong**, *emphasis*, `code`, ~~struck~~, [links](url),
--   lists and lists in lists, - [x] task lists, > quotes, --- rules,
--   | tables |, images alone on their line, and ``` blocks of code with
--   the colors of their language (```c, ```python...: the languages of
--   the text editor, raven_text_editor.h).
--
-- The text wraps on the width of the label unless Rlabel_set_wrap said
-- otherwise; put the label in a scroll box for a long document. Strong
-- and emphasized text use the bold and italic of the font
-- (Rfont_set_variant), which Raven derives from the font itself when it
-- was given none. The colors come from the theme: R_COLOR_LINK,
-- R_COLOR_CODE, R_COLOR_SEPARATOR, R_COLOR_ACCENT.
foreign import ccall safe "Rlabel_set_markdown" rlabel_set_markdown :: Rid -> CInt -> IO ()

foreign import ccall safe "Rlabel_get_markdown" rlabel_get_markdown :: Rid -> IO CInt

foreign import ccall safe "Rlabel_set_link_callback" rlabel_set_link_callback :: Rid -> RLabelLinkCallback -> Ptr () -> IO ()

-- | The text of a Markdown label without its marks, as a reader sees it. Valid until the next call.
foreign import ccall safe "Rlabel_get_plain_text" rlabel_get_plain_text :: Rid -> IO CString

-- | R_TRUE lets the user select the text of the label with the
-- mouse, as on a web page (Selectable="true" in XML): drag to select, a
-- double click selects a word, a triple click a paragraph, Shift + click
-- extends the selection; then Ctrl+C (Cmd+C) copies it and Ctrl+A selects
-- everything. A click elsewhere takes the selection away. Plain and
-- Markdown labels alike: a copy of a Markdown label is its text without
-- the marks, and its links are still clicked, not selected. The label
-- takes the mouse (what holds it no longer receives the clicks on it) and
-- the keyboard after a click; Tab passes it by. Default R_FALSE.
foreign import ccall safe "Rlabel_set_selectable" rlabel_set_selectable :: Rid -> CInt -> IO ()

foreign import ccall safe "Rlabel_get_selectable" rlabel_get_selectable :: Rid -> IO CInt

-- | The text selected, as Ctrl+C copies it ("" when nothing is). Valid until the next call.
foreign import ccall safe "Rlabel_get_selected_text" rlabel_get_selected_text :: Rid -> IO CString

-- | Selects the whole text of a selectable label.
foreign import ccall safe "Rlabel_select_all" rlabel_select_all :: Rid -> IO ()

foreign import ccall safe "Rlabel_clear_selection" rlabel_clear_selection :: Rid -> IO ()

-- | Shortcut for Rwidget_create(R_WIDGET_BUTTON) + Rbutton_set_text.
foreign import ccall safe "Rbutton_create" rbutton_create :: CString -> IO Rid

-- | Sets the content of the button to a label. A button can instead
-- hold any single child widget (Rwidget_add_child), e.g. an HBox with an icon.
foreign import ccall safe "Rbutton_set_text" rbutton_set_text :: Rid -> CString -> IO ()

foreign import ccall safe "Rbutton_set_style" rbutton_set_style :: Rid -> RButtonStyle -> IO ()

foreign import ccall safe "Rbutton_set_clicked_callback" rbutton_set_clicked_callback :: Rid -> RButtonClickedCallback -> Ptr () -> IO ()

foreign import ccall safe "Rcheckbox_create" rcheckbox_create :: CString -> IO Rid

foreign import ccall safe "Rcheckbox_set_text" rcheckbox_set_text :: Rid -> CString -> IO ()

foreign import ccall safe "Rcheckbox_get_text" rcheckbox_get_text :: Rid -> IO CString

foreign import ccall safe "Rcheckbox_set_checked" rcheckbox_set_checked :: Rid -> CInt -> IO ()

foreign import ccall safe "Rcheckbox_is_checked" rcheckbox_is_checked :: Rid -> IO CInt

foreign import ccall safe "Rcheckbox_set_style" rcheckbox_set_style :: Rid -> RCheckboxStyle -> IO ()

foreign import ccall safe "Rcheckbox_set_changed_callback" rcheckbox_set_changed_callback :: Rid -> RCheckboxChangedCallback -> Ptr () -> IO ()

foreign import ccall safe "Rslider_set_range" rslider_set_range :: Rid -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rslider_set_value" rslider_set_value :: Rid -> CFloat -> IO ()

foreign import ccall safe "Rslider_get_value" rslider_get_value :: Rid -> IO CFloat

-- | The range given with Rslider_set_range and the step (NULL for what is not wanted).
foreign import ccall safe "Rslider_get_range" rslider_get_range :: Rid -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> IO ()

-- | Snaps the value to multiples of `step`. 0 = continuous (default).
foreign import ccall safe "Rslider_set_step" rslider_set_step :: Rid -> CFloat -> IO ()

foreign import ccall safe "Rslider_set_orientation" rslider_set_orientation :: Rid -> ROrientation -> IO ()

foreign import ccall safe "Rslider_set_changed_callback" rslider_set_changed_callback :: Rid -> RSliderChangedCallback -> Ptr () -> IO ()

-- | Progress between 0 and 1.
foreign import ccall safe "Rprogress_bar_set_value" rprogress_bar_set_value :: Rid -> CFloat -> IO ()

foreign import ccall safe "Rprogress_bar_get_value" rprogress_bar_get_value :: Rid -> IO CFloat

foreign import ccall safe "Rimage_set_texture" rimage_set_texture :: Rid -> Rid -> IO ()

foreign import ccall safe "Rimage_set_tint_color" rimage_set_tint_color :: Rid -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

-- | R_TRUE: the image is tinted with the text color around it (white icons that follow the theme). XML: TintText.
foreign import ccall safe "Rimage_set_tint_text" rimage_set_tint_text :: Rid -> CInt -> IO ()

foreign import ccall safe "Rimage_set_stretch" rimage_set_stretch :: Rid -> RStretch -> IO ()

-- | Part of the texture to show, in UV coordinates. Default: 0, 0, 1, 1.
foreign import ccall safe "Rimage_set_region" rimage_set_region :: Rid -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rimage_set_corner_radius" rimage_set_corner_radius :: Rid -> CFloat -> IO ()

foreign import ccall safe "Rtext_input_set_text" rtext_input_set_text :: Rid -> CString -> IO ()

foreign import ccall safe "Rtext_input_get_text" rtext_input_get_text :: Rid -> IO CString

-- | Dimmed text shown while the input is empty.
foreign import ccall safe "Rtext_input_set_placeholder" rtext_input_set_placeholder :: Rid -> CString -> IO ()

foreign import ccall safe "Rtext_input_set_password" rtext_input_set_password :: Rid -> CInt -> IO ()

foreign import ccall safe "Rtext_input_is_password" rtext_input_is_password :: Rid -> IO CInt

foreign import ccall safe "Rtext_input_is_read_only" rtext_input_is_read_only :: Rid -> IO CInt

foreign import ccall safe "Rtext_input_set_read_only" rtext_input_set_read_only :: Rid -> CInt -> IO ()

-- | Maximum number of characters. 0 = no limit (default).
foreign import ccall safe "Rtext_input_set_max_length" rtext_input_set_max_length :: Rid -> CInt -> IO ()

foreign import ccall safe "Rtext_input_select_all" rtext_input_select_all :: Rid -> IO ()

-- | Called on every edit.
foreign import ccall safe "Rtext_input_set_changed_callback" rtext_input_set_changed_callback :: Rid -> RTextChangedCallback -> Ptr () -> IO ()

-- | Called when Enter is pressed. This also fires the action of the widget.
foreign import ccall safe "Rtext_input_set_committed_callback" rtext_input_set_committed_callback :: Rid -> RTextChangedCallback -> Ptr () -> IO ()

foreign import ccall safe "Rspin_box_set_range" rspin_box_set_range :: Rid -> CDouble -> CDouble -> IO ()

foreign import ccall safe "Rspin_box_set_value" rspin_box_set_value :: Rid -> CDouble -> IO ()

foreign import ccall safe "Rspin_box_get_value" rspin_box_get_value :: Rid -> IO CDouble

-- | The range and the step (NULL for what is not wanted).
foreign import ccall safe "Rspin_box_get_range" rspin_box_get_range :: Rid -> Ptr CDouble -> Ptr CDouble -> Ptr CDouble -> IO ()

foreign import ccall safe "Rspin_box_set_step" rspin_box_set_step :: Rid -> CDouble -> IO ()

-- | Digits shown after the decimal point. Default: 2.
foreign import ccall safe "Rspin_box_set_decimals" rspin_box_set_decimals :: Rid -> CInt -> IO ()

foreign import ccall safe "Rspin_box_set_changed_callback" rspin_box_set_changed_callback :: Rid -> RSpinBoxChangedCallback -> Ptr () -> IO ()

-- | Returns: The index of the new item.
foreign import ccall safe "Rcombo_box_add_item" rcombo_box_add_item :: Rid -> CString -> IO CInt

foreign import ccall safe "Rcombo_box_clear_items" rcombo_box_clear_items :: Rid -> IO ()

foreign import ccall safe "Rcombo_box_get_item_count" rcombo_box_get_item_count :: Rid -> IO CInt

foreign import ccall safe "Rcombo_box_get_item_text" rcombo_box_get_item_text :: Rid -> CInt -> IO CString

-- | Selects an item. -1 selects nothing.
foreign import ccall safe "Rcombo_box_set_selected" rcombo_box_set_selected :: Rid -> CInt -> IO ()

foreign import ccall safe "Rcombo_box_get_selected" rcombo_box_get_selected :: Rid -> IO CInt

foreign import ccall safe "Rcombo_box_set_changed_callback" rcombo_box_set_changed_callback :: Rid -> RComboBoxChangedCallback -> Ptr () -> IO ()

-- | R_HORIZONTAL draws a horizontal line (default).
foreign import ccall safe "Rseparator_set_orientation" rseparator_set_orientation :: Rid -> ROrientation -> IO ()

-- | Returns: The index of the new item.
foreign import ccall safe "Rlist_view_add_item" rlist_view_add_item :: Rid -> CString -> IO CInt

foreign import ccall safe "Rlist_view_set_item_text" rlist_view_set_item_text :: Rid -> CInt -> CString -> IO ()

foreign import ccall safe "Rlist_view_get_item_text" rlist_view_get_item_text :: Rid -> CInt -> IO CString

foreign import ccall safe "Rlist_view_remove_item" rlist_view_remove_item :: Rid -> CInt -> IO ()

foreign import ccall safe "Rlist_view_clear_items" rlist_view_clear_items :: Rid -> IO ()

foreign import ccall safe "Rlist_view_get_item_count" rlist_view_get_item_count :: Rid -> IO CInt

foreign import ccall safe "Rlist_view_set_virtual_items" rlist_view_set_virtual_items :: Rid -> CInt -> RListItemTextCallback -> Ptr () -> IO ()

foreign import ccall safe "Rlist_view_set_item_color_callback" rlist_view_set_item_color_callback :: Rid -> RListItemColorCallback -> Ptr () -> IO ()

-- | Selects a row. -1 selects nothing.
foreign import ccall safe "Rlist_view_set_selected" rlist_view_set_selected :: Rid -> CInt -> IO ()

foreign import ccall safe "Rlist_view_get_selected" rlist_view_get_selected :: Rid -> IO CInt

foreign import ccall safe "Rlist_view_scroll_to_item" rlist_view_scroll_to_item :: Rid -> CInt -> IO ()

foreign import ccall safe "Rlist_view_set_selection_changed_callback" rlist_view_set_selection_changed_callback :: Rid -> RListEventCallback -> Ptr () -> IO ()

-- | Called on double click or Enter. This also fires the action of the widget.
foreign import ccall safe "Rlist_view_set_activated_callback" rlist_view_set_activated_callback :: Rid -> RListEventCallback -> Ptr () -> IO ()

-- | Adds an item.
-- parent_item Item to add under, or R_INVALID_ID for a top-level item.
-- Returns: The handle of the new item.
foreign import ccall safe "Rtree_view_add_item" rtree_view_add_item :: Rid -> Rid -> CString -> IO Rid

-- | Deletes an item and its descendants.
foreign import ccall safe "Rtree_item_destroy" rtree_item_destroy :: Rid -> IO ()

foreign import ccall safe "Rtree_view_clear_items" rtree_view_clear_items :: Rid -> IO ()

foreign import ccall safe "Rtree_item_is_valid" rtree_item_is_valid :: Rid -> IO CInt

foreign import ccall safe "Rtree_item_set_text" rtree_item_set_text :: Rid -> CString -> IO ()

foreign import ccall safe "Rtree_item_get_text" rtree_item_get_text :: Rid -> IO CString

foreign import ccall safe "Rtree_item_set_expanded" rtree_item_set_expanded :: Rid -> CInt -> IO ()

foreign import ccall safe "Rtree_item_is_expanded" rtree_item_is_expanded :: Rid -> IO CInt

foreign import ccall safe "Rtree_item_set_user_handle" rtree_item_set_user_handle :: Rid -> Ptr () -> IO ()

foreign import ccall safe "Rtree_item_get_user_handle" rtree_item_get_user_handle :: Rid -> IO (Ptr ())

-- | Top-level items of a tree.
foreign import ccall safe "Rtree_view_get_item_count" rtree_view_get_item_count :: Rid -> IO CInt

foreign import ccall safe "Rtree_view_get_item" rtree_view_get_item :: Rid -> CInt -> IO Rid

-- | Returns: The parent item, or R_INVALID_ID for a top-level item.
foreign import ccall safe "Rtree_item_get_parent" rtree_item_get_parent :: Rid -> IO Rid

foreign import ccall safe "Rtree_item_get_child_count" rtree_item_get_child_count :: Rid -> IO CInt

foreign import ccall safe "Rtree_item_get_child" rtree_item_get_child :: Rid -> CInt -> IO Rid

-- | Selects an item. R_INVALID_ID selects nothing.
foreign import ccall safe "Rtree_view_set_selected_item" rtree_view_set_selected_item :: Rid -> Rid -> IO ()

foreign import ccall safe "Rtree_view_get_selected_item" rtree_view_get_selected_item :: Rid -> IO Rid

foreign import ccall safe "Rtree_view_set_selection_changed_callback" rtree_view_set_selection_changed_callback :: Rid -> RTreeEventCallback -> Ptr () -> IO ()

-- | Called on double click or Enter. This also fires the action of the widget.
foreign import ccall safe "Rtree_view_set_activated_callback" rtree_view_set_activated_callback :: Rid -> RTreeEventCallback -> Ptr () -> IO ()

-- | Icon drawn before the text of the item (a region u0,v0,u1,v1 of the texture). R_INVALID_ID removes it.
foreign import ccall safe "Rtree_item_set_icon" rtree_item_set_icon :: Rid -> Rid -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

-- | Color of the text of the item; alpha 0 goes back to the theme.
foreign import ccall safe "Rtree_item_set_text_color" rtree_item_set_text_color :: Rid -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

-- | Several items can be selected: Ctrl + click adds or removes one,
-- Shift + click selects a range. The selection callback receives the item
-- clicked last (the primary one, Rtree_view_get_selected_item).
foreign import ccall safe "Rtree_view_set_multi_select" rtree_view_set_multi_select :: Rid -> CInt -> IO ()

foreign import ccall safe "Rtree_item_set_selected" rtree_item_set_selected :: Rid -> CInt -> IO ()

foreign import ccall safe "Rtree_item_is_selected" rtree_item_is_selected :: Rid -> IO CInt

foreign import ccall safe "Rtree_view_clear_selection" rtree_view_clear_selection :: Rid -> IO ()

-- | Selected items, top to bottom.
foreign import ccall safe "Rtree_view_get_selection_count" rtree_view_get_selection_count :: Rid -> IO CInt

foreign import ccall safe "Rtree_view_get_selection" rtree_view_get_selection :: Rid -> CInt -> IO Rid

foreign import ccall safe "Rtree_view_scroll_to_item" rtree_view_scroll_to_item :: Rid -> Rid -> IO ()

foreign import ccall safe "Rtree_view_set_context_callback" rtree_view_set_context_callback :: Rid -> RTreeContextCallback -> Ptr () -> IO ()

-- | Items can be dragged: the selection is given to Rdrag_begin with
-- this type and, as data, the handles of the items in decimal separated
-- by spaces. NULL: no drag (default).
foreign import ccall safe "Rtree_view_set_drag_type" rtree_view_set_drag_type :: Rid -> CString -> IO ()

-- | Item at a point relative to the tree (drop callbacks give local_x, local_y), or R_INVALID_ID.
foreign import ccall safe "Rtree_view_get_item_at" rtree_view_get_item_at :: Rid -> CFloat -> CFloat -> IO Rid

-- | Highlights where a drop would go: an item, or the whole tree (`whole_tree`). R_INVALID_ID, 0 removes it.
foreign import ccall safe "Rtree_view_set_drop_highlight" rtree_view_set_drop_highlight :: Rid -> Rid -> CInt -> IO ()

-- | The menu bar of the system. On macOS, the first MenuBar of a
-- window is shown in the bar at the top of the screen while the window is
-- the active one, after the menu of the application (About, Hide, Quit);
-- it then takes no room in the window. Its shortcuts are those of the
-- system: R_MOD_SUPER is Command, R_MOD_CTRL is Control. Elsewhere it is
-- drawn in the window. On by default; R_FALSE keeps it in the window
-- everywhere. In XML: <MenuBar Native="false">.
foreign import ccall safe "Rmenu_bar_set_native" rmenu_bar_set_native :: Rid -> CInt -> IO ()

-- | Returns: R_TRUE while the menu bar is shown by the system instead of the window.
foreign import ccall safe "Rmenu_bar_is_native" rmenu_bar_is_native :: Rid -> IO CInt

-- | Creates a menu inside a menu bar or as a sub-menu of another menu.
foreign import ccall safe "Rmenu_add_menu" rmenu_add_menu :: Rid -> CString -> IO Rid

foreign import ccall safe "Rmenu_set_title" rmenu_set_title :: Rid -> CString -> IO ()

foreign import ccall safe "Rmenu_add_item" rmenu_add_item :: Rid -> CString -> IO Rid

foreign import ccall safe "Rmenu_add_separator" rmenu_add_separator :: Rid -> IO Rid

foreign import ccall safe "Rmenu_item_set_text" rmenu_item_set_text :: Rid -> CString -> IO ()

foreign import ccall safe "Rmenu_item_get_text" rmenu_item_get_text :: Rid -> IO CString

-- | Shows a shortcut next to the item and triggers the item when the
-- keys are pressed in the window of its menu bar.
foreign import ccall safe "Rmenu_item_set_shortcut" rmenu_item_set_shortcut :: Rid -> RKey -> Word32 -> IO ()

-- | Display-only shortcut hint; copied, never binds a key. Overrides
-- the formatted shortcut without changing its binding. NULL restores the
-- formatted shortcut; an empty string hides it. XML: ShortcutText.
foreign import ccall safe "Rmenu_item_set_shortcut_text" rmenu_item_set_shortcut_text :: Rid -> CString -> IO ()

foreign import ccall safe "Rmenu_item_set_checkable" rmenu_item_set_checkable :: Rid -> CInt -> IO ()

foreign import ccall safe "Rmenu_item_set_checked" rmenu_item_set_checked :: Rid -> CInt -> IO ()

-- | Icon drawn before the text of a menu item (after the check mark column).
-- texture R_INVALID_ID removes it. u0, v0, u1, v1 : region of the texture.
-- tint    R_TRUE: the texture is a mask drawn in the text color (monochrome icons).
foreign import ccall safe "Rmenu_item_set_icon" rmenu_item_set_icon :: Rid -> Rid -> CFloat -> CFloat -> CFloat -> CFloat -> CInt -> IO ()

foreign import ccall safe "Rmenu_item_is_checked" rmenu_item_is_checked :: Rid -> IO CInt

foreign import ccall safe "Rmenu_item_set_clicked_callback" rmenu_item_set_clicked_callback :: Rid -> RMenuItemClickedCallback -> Ptr () -> IO ()

-- | Opens a menu as a popup at a position of the window (logical units).
foreign import ccall safe "Rmenu_open" rmenu_open :: Rid -> Rid -> CFloat -> CFloat -> IO ()

-- | Closes every open menu and popup of the window.
foreign import ccall safe "Rwindow_close_popups" rwindow_close_popups :: Rid -> IO ()

-- | Gives the tiles: `count` of them, described by `callback`. The selection is cleared.
foreign import ccall safe "Rtile_view_set_items" rtile_view_set_items :: Rid -> CInt -> RTileItemCallback -> Ptr () -> IO ()

-- | Draws the tiles again (their description changed, not their number).
foreign import ccall safe "Rtile_view_refresh" rtile_view_refresh :: Rid -> IO ()

foreign import ccall safe "Rtile_view_get_count" rtile_view_get_count :: Rid -> IO CInt

-- | Size of a tile (its name included) and of its icon. Default 112 x 112, icon 64.
foreign import ccall safe "Rtile_view_set_tile_size" rtile_view_set_tile_size :: Rid -> CFloat -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rtile_view_set_empty_text" rtile_view_set_empty_text :: Rid -> CString -> IO ()

foreign import ccall safe "Rtile_view_set_selected" rtile_view_set_selected :: Rid -> CInt -> CInt -> IO ()

foreign import ccall safe "Rtile_view_is_selected" rtile_view_is_selected :: Rid -> CInt -> IO CInt

foreign import ccall safe "Rtile_view_clear_selection" rtile_view_clear_selection :: Rid -> IO ()

foreign import ccall safe "Rtile_view_select_only" rtile_view_select_only :: Rid -> CInt -> IO ()

-- | The tile clicked last, or -1.
foreign import ccall safe "Rtile_view_get_current" rtile_view_get_current :: Rid -> IO CInt

foreign import ccall safe "Rtile_view_get_selection_count" rtile_view_get_selection_count :: Rid -> IO CInt

-- | Index of the nth selected tile, in order.
foreign import ccall safe "Rtile_view_get_selection" rtile_view_get_selection :: Rid -> CInt -> IO CInt

foreign import ccall safe "Rtile_view_scroll_to" rtile_view_scroll_to :: Rid -> CInt -> IO ()

-- | Tile at a point relative to the widget (drop callbacks give local_x, local_y), or -1.
foreign import ccall safe "Rtile_view_get_item_at" rtile_view_get_item_at :: Rid -> CFloat -> CFloat -> IO CInt

-- | Highlights the tile a drop would go to; -1 removes it.
foreign import ccall safe "Rtile_view_set_drop_highlight" rtile_view_set_drop_highlight :: Rid -> CInt -> IO ()

-- | Tiles can be dragged: the selected tiles are given to Rdrag_begin
-- with this type and, as data, their indices separated by spaces.
foreign import ccall safe "Rtile_view_set_drag_type" rtile_view_set_drag_type :: Rid -> CString -> IO ()

foreign import ccall safe "Rtile_view_set_drag_data_callback" rtile_view_set_drag_data_callback :: Rid -> RTileDragDataCallback -> Ptr () -> IO ()

foreign import ccall safe "Rtile_view_set_selection_changed_callback" rtile_view_set_selection_changed_callback :: Rid -> RTileEventCallback -> Ptr () -> IO ()

foreign import ccall safe "Rtile_view_set_activated_callback" rtile_view_set_activated_callback :: Rid -> RTileEventCallback -> Ptr () -> IO ()

foreign import ccall safe "Rtile_view_set_context_callback" rtile_view_set_context_callback :: Rid -> RTileContextCallback -> Ptr () -> IO ()

-- | Adds a column.
-- width Width in logical units; 0 or less gives the default (120).
-- Returns: The index of the column. The stored rows get an empty cell in it.
foreign import ccall safe "Rtable_add_column" rtable_add_column :: Rid -> CString -> CFloat -> IO CInt

foreign import ccall safe "Rtable_get_column_count" rtable_get_column_count :: Rid -> IO CInt

-- | Removes the columns and the stored rows.
foreign import ccall safe "Rtable_clear_columns" rtable_clear_columns :: Rid -> IO ()

foreign import ccall safe "Rtable_set_column_header" rtable_set_column_header :: Rid -> CInt -> CString -> IO ()

foreign import ccall safe "Rtable_get_column_header" rtable_get_column_header :: Rid -> CInt -> IO CString

foreign import ccall safe "Rtable_set_column_width" rtable_set_column_width :: Rid -> CInt -> CFloat -> IO ()

-- | The width the column has on screen (a filling column takes more than its width).
foreign import ccall safe "Rtable_get_column_width" rtable_get_column_width :: Rid -> CInt -> IO CFloat

-- | > 0: the column takes this share of the room the others leave; 0: its width only.
foreign import ccall safe "Rtable_set_column_fill" rtable_set_column_fill :: Rid -> CInt -> CFloat -> IO ()

-- | R_FALSE: a click on the header does not sort (default R_TRUE).
foreign import ccall safe "Rtable_set_column_sortable" rtable_set_column_sortable :: Rid -> CInt -> CInt -> IO ()

-- | Where the texts sit in the cells: 0 left, 0.5 center, 1 right.
foreign import ccall safe "Rtable_set_column_align" rtable_set_column_align :: Rid -> CInt -> CFloat -> IO ()

-- | Adds a row.
-- cells The texts of the cells, separated by '|' ("Ada|36|London"),
--              or NULL for an empty row; Rtable_set_cell gives a text holding a '|'.
-- Returns: The index of the row.
foreign import ccall safe "Rtable_add_row" rtable_add_row :: Rid -> CString -> IO CInt

foreign import ccall safe "Rtable_set_cell" rtable_set_cell :: Rid -> CInt -> CInt -> CString -> IO ()

foreign import ccall safe "Rtable_get_cell" rtable_get_cell :: Rid -> CInt -> CInt -> IO CString

foreign import ccall safe "Rtable_remove_row" rtable_remove_row :: Rid -> CInt -> IO ()

foreign import ccall safe "Rtable_clear_rows" rtable_clear_rows :: Rid -> IO ()

foreign import ccall safe "Rtable_get_row_count" rtable_get_row_count :: Rid -> IO CInt

foreign import ccall safe "Rtable_set_virtual_rows" rtable_set_virtual_rows :: Rid -> CInt -> RTableCellTextCallback -> Ptr () -> IO ()

foreign import ccall safe "Rtable_set_cell_color_callback" rtable_set_cell_color_callback :: Rid -> RTableCellColorCallback -> Ptr () -> IO ()

-- | Selects a row. -1 selects nothing.
foreign import ccall safe "Rtable_set_selected" rtable_set_selected :: Rid -> CInt -> IO ()

foreign import ccall safe "Rtable_get_selected" rtable_get_selected :: Rid -> IO CInt

foreign import ccall safe "Rtable_scroll_to_row" rtable_scroll_to_row :: Rid -> CInt -> IO ()

-- | Sorts by a column (stored rows are moved); -1 shows no sorted column.
foreign import ccall safe "Rtable_sort" rtable_sort :: Rid -> CInt -> CInt -> IO ()

foreign import ccall safe "Rtable_get_sort_column" rtable_get_sort_column :: Rid -> IO CInt

foreign import ccall safe "Rtable_is_sort_descending" rtable_is_sort_descending :: Rid -> IO CInt

foreign import ccall safe "Rtable_set_header_visible" rtable_set_header_visible :: Rid -> CInt -> IO ()

foreign import ccall safe "Rtable_set_grid_lines" rtable_set_grid_lines :: Rid -> CInt -> IO ()

-- | Every other row a little darker.
foreign import ccall safe "Rtable_set_striped" rtable_set_striped :: Rid -> CInt -> IO ()

foreign import ccall safe "Rtable_set_selection_changed_callback" rtable_set_selection_changed_callback :: Rid -> RTableEventCallback -> Ptr () -> IO ()

-- | Called on double click or Enter. This also fires the action of the widget.
foreign import ccall safe "Rtable_set_activated_callback" rtable_set_activated_callback :: Rid -> RTableEventCallback -> Ptr () -> IO ()

foreign import ccall safe "Rtable_set_sort_callback" rtable_set_sort_callback :: Rid -> RTableSortCallback -> Ptr () -> IO ()

-- | Sets the date (today when the picker is created). An invalid date is refused.
foreign import ccall safe "Rdate_picker_set_date" rdate_picker_set_date :: Rid -> CInt -> CInt -> CInt -> IO ()

foreign import ccall safe "Rdate_picker_get_date" rdate_picker_get_date :: Rid -> Ptr CInt -> Ptr CInt -> Ptr CInt -> IO ()

-- | The earliest and the latest dates that can be chosen; 0 for year leaves that side open.
foreign import ccall safe "Rdate_picker_set_range" rdate_picker_set_range :: Rid -> CInt -> CInt -> CInt -> CInt -> CInt -> CInt -> IO ()

-- | 0 = Sunday (default), 1 = Monday...
foreign import ccall safe "Rdate_picker_set_first_day_of_week" rdate_picker_set_first_day_of_week :: Rid -> CInt -> IO ()

-- | How the field writes the date: YYYY, MM, M, DD, D, MMMM (the name
-- of the month), MMM (its first three letters), dddd (the name of the day),
-- ddd; anything else is kept. Default "YYYY-MM-DD"; "D MMMM YYYY" gives "3 November 2026".
foreign import ccall safe "Rdate_picker_set_format" rdate_picker_set_format :: Rid -> CString -> IO ()

-- | The date as the field shows it (valid until the next call).
foreign import ccall safe "Rdate_picker_get_text" rdate_picker_get_text :: Rid -> IO CString

-- | Opens or closes the calendar.
foreign import ccall safe "Rdate_picker_set_open" rdate_picker_set_open :: Rid -> CInt -> IO ()

foreign import ccall safe "Rdate_picker_is_open" rdate_picker_is_open :: Rid -> IO CInt

foreign import ccall safe "Rdate_picker_set_changed_callback" rdate_picker_set_changed_callback :: Rid -> RDateChangedCallback -> Ptr () -> IO ()

foreign import ccall safe "Rtime_picker_set_time" rtime_picker_set_time :: Rid -> CInt -> CInt -> CInt -> IO ()

foreign import ccall safe "Rtime_picker_get_time" rtime_picker_get_time :: Rid -> Ptr CInt -> Ptr CInt -> Ptr CInt -> IO ()

foreign import ccall safe "Rtime_picker_set_seconds_visible" rtime_picker_set_seconds_visible :: Rid -> CInt -> IO ()

-- | Step of the minutes for the arrows and the wheel (default 1).
foreign import ccall safe "Rtime_picker_set_minute_step" rtime_picker_set_minute_step :: Rid -> CInt -> IO ()

foreign import ccall safe "Rtime_picker_set_changed_callback" rtime_picker_set_changed_callback :: Rid -> RTimeChangedCallback -> Ptr () -> IO ()

-- | Shows a message for `seconds` (0 or less: until Rtoast_hide), with the kind and button set now.
foreign import ccall safe "Rtoast_show" rtoast_show :: Rid -> CString -> CDouble -> IO ()

-- | Same, with a kind of its own (the kind set by Rtoast_set_kind is left as it is).
foreign import ccall safe "Rtoast_show_kind" rtoast_show_kind :: Rid -> CString -> RToastKind -> CDouble -> IO ()

-- | Hides the newest message shown; the next one waiting comes.
foreign import ccall safe "Rtoast_hide" rtoast_hide :: Rid -> IO ()

-- | Hides every message, and forgets those waiting.
foreign import ccall safe "Rtoast_clear" rtoast_clear :: Rid -> IO ()

foreign import ccall safe "Rtoast_is_shown" rtoast_is_shown :: Rid -> IO CInt

-- | Returns: The messages shown and waiting.
foreign import ccall safe "Rtoast_get_count" rtoast_get_count :: Rid -> IO CInt

-- | Returns: The newest message shown (the last one, once hidden; the clicked one in the action callback).
foreign import ccall safe "Rtoast_get_text" rtoast_get_text :: Rid -> IO CString

foreign import ccall safe "Rtoast_set_queue" rtoast_set_queue :: Rid -> RToastQueue -> IO ()

foreign import ccall safe "Rtoast_get_queue" rtoast_get_queue :: Rid -> IO RToastQueue

-- | How many messages a R_TOAST_STACK shows at once (default 3).
foreign import ccall safe "Rtoast_set_max_visible" rtoast_set_max_visible :: Rid -> CInt -> IO ()

-- | The kind of the next messages (and of the message shown, with R_TOAST_REPLACE).
foreign import ccall safe "Rtoast_set_kind" rtoast_set_kind :: Rid -> RToastKind -> IO ()

-- | A button at the end of the next messages ("Undo"); NULL or "" removes it.
foreign import ccall safe "Rtoast_set_action_text" rtoast_set_action_text :: Rid -> CString -> IO ()

foreign import ccall safe "Rtoast_set_action_callback" rtoast_set_action_callback :: Rid -> RToastCallback -> Ptr () -> IO ()

-- | Replaces the items: their texts separated by '|'.
foreign import ccall safe "Rbreadcrumb_set_items" rbreadcrumb_set_items :: Rid -> CString -> IO ()

-- | Returns: The index of the new item, at the end.
foreign import ccall safe "Rbreadcrumb_add_item" rbreadcrumb_add_item :: Rid -> CString -> IO CInt

-- | Keeps the first `count` items: Rbreadcrumb_truncate(b, index + 1) goes back to the item clicked.
foreign import ccall safe "Rbreadcrumb_truncate" rbreadcrumb_truncate :: Rid -> CInt -> IO ()

foreign import ccall safe "Rbreadcrumb_get_item_count" rbreadcrumb_get_item_count :: Rid -> IO CInt

foreign import ccall safe "Rbreadcrumb_get_item_text" rbreadcrumb_get_item_text :: Rid -> CInt -> IO CString

foreign import ccall safe "Rbreadcrumb_set_clicked_callback" rbreadcrumb_set_clicked_callback :: Rid -> RBreadcrumbCallback -> Ptr () -> IO ()

-- | The score, from 0 to the number of stars.
foreign import ccall safe "Rrating_set_value" rrating_set_value :: Rid -> CFloat -> IO ()

foreign import ccall safe "Rrating_get_value" rrating_get_value :: Rid -> IO CFloat

-- | Number of stars (default 5, at most 20).
foreign import ccall safe "Rrating_set_max" rrating_set_max :: Rid -> CInt -> IO ()

foreign import ccall safe "Rrating_get_max" rrating_get_max :: Rid -> IO CInt

-- | Half stars can be chosen (a whole star by default).
foreign import ccall safe "Rrating_set_allow_half" rrating_set_allow_half :: Rid -> CInt -> IO ()

-- | Shown only, the user cannot change it.
foreign import ccall safe "Rrating_set_read_only" rrating_set_read_only :: Rid -> CInt -> IO ()

-- | Size of a star (default 18).
foreign import ccall safe "Rrating_set_star_size" rrating_set_star_size :: Rid -> CFloat -> IO ()

foreign import ccall safe "Rrating_set_changed_callback" rrating_set_changed_callback :: Rid -> RRatingChangedCallback -> Ptr () -> IO ()

foreign import ccall safe "Rpagination_set_page_count" rpagination_set_page_count :: Rid -> CInt -> IO ()

foreign import ccall safe "Rpagination_get_page_count" rpagination_get_page_count :: Rid -> IO CInt

foreign import ccall safe "Rpagination_set_page" rpagination_set_page :: Rid -> CInt -> IO ()

foreign import ccall safe "Rpagination_get_page" rpagination_get_page :: Rid -> IO CInt

-- | Page numbers shown around the current one (default 1).
foreign import ccall safe "Rpagination_set_siblings" rpagination_set_siblings :: Rid -> CInt -> IO ()

foreign import ccall safe "Rpagination_set_changed_callback" rpagination_set_changed_callback :: Rid -> RPageChangedCallback -> Ptr () -> IO ()

foreign import ccall safe "Rplot_set_title" rplot_set_title :: Rid -> CString -> IO ()

foreign import ccall safe "Rplot_get_title" rplot_get_title :: Rid -> IO CString

-- | Adds a series (its name in the legend; NULL or "": not in the legend). Returns its index.
foreign import ccall safe "Rplot_add_series" rplot_add_series :: Rid -> CString -> IO CInt

foreign import ccall safe "Rplot_remove_series" rplot_remove_series :: Rid -> CInt -> IO ()

-- | Removes every series and every drag line.
foreign import ccall safe "Rplot_clear" rplot_clear :: Rid -> IO ()

foreign import ccall safe "Rplot_get_series_count" rplot_get_series_count :: Rid -> IO CInt

foreign import ccall safe "Rplot_set_series_name" rplot_set_series_name :: Rid -> CInt -> CString -> IO ()

foreign import ccall safe "Rplot_get_series_name" rplot_get_series_name :: Rid -> CInt -> IO CString

-- | Replaces the points of a series with `count` points, copied.
-- x The X of the points, or NULL: 0, 1, 2...
-- y The Y of the points. NaN leaves a gap in the line.
foreign import ccall safe "Rplot_set_series_data" rplot_set_series_data :: Rid -> CInt -> Ptr CDouble -> Ptr CDouble -> CInt -> IO ()

-- | Adds points after those of a series (x NULL: numbered after the last one).
foreign import ccall safe "Rplot_append_series_data" rplot_append_series_data :: Rid -> CInt -> Ptr CDouble -> Ptr CDouble -> CInt -> IO ()

-- | Adds one point after those of a series.
foreign import ccall safe "Rplot_add_point" rplot_add_point :: Rid -> CInt -> CDouble -> CDouble -> IO ()

foreign import ccall safe "Rplot_clear_series_data" rplot_clear_series_data :: Rid -> CInt -> IO ()

foreign import ccall safe "Rplot_get_point_count" rplot_get_point_count :: Rid -> CInt -> IO CInt

-- | One point of a series. Returns R_FALSE when there is no such point.
foreign import ccall safe "Rplot_get_point" rplot_get_point :: Rid -> CInt -> CInt -> Ptr CDouble -> Ptr CDouble -> IO CInt

-- | Keeps only the last `max` points of a series, for data that streams in (0: all of them, the default).
foreign import ccall safe "Rplot_set_series_max_points" rplot_set_series_max_points :: Rid -> CInt -> CInt -> IO ()

-- | The color of a series (by default the next one of a palette of ten).
foreign import ccall safe "Rplot_set_series_color" rplot_set_series_color :: Rid -> CInt -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

-- | How the line is drawn and how wide (default solid, 1.75; width 0 keeps the width).
foreign import ccall safe "Rplot_set_series_line" rplot_set_series_line :: Rid -> CInt -> RPlotLine -> CFloat -> IO ()

-- | The mark on each point and its radius (default none, 4; size 0 keeps the size).
foreign import ccall safe "Rplot_set_series_marker" rplot_set_series_marker :: Rid -> CInt -> RPlotMarker -> CFloat -> IO ()

-- | Filled markers (the default) or only their outline.
foreign import ccall safe "Rplot_set_series_marker_filled" rplot_set_series_marker_filled :: Rid -> CInt -> CInt -> IO ()

-- | Draws the line in steps: along at the height of a point, then up or down to the next.
foreign import ccall safe "Rplot_set_series_step" rplot_set_series_step :: Rid -> CInt -> CInt -> IO ()

-- | Shades the area between the line and zero, at this opacity (0: none, the default).
foreign import ccall safe "Rplot_set_series_fill" rplot_set_series_fill :: Rid -> CInt -> CFloat -> IO ()

-- | The axes a series is drawn against: R_PLOT_X or R_PLOT_X2, R_PLOT_Y or R_PLOT_Y2.
foreign import ccall safe "Rplot_set_series_axes" rplot_set_series_axes :: Rid -> CInt -> RPlotAxis -> RPlotAxis -> IO ()

-- | Shows or hides a series (the user does it with a click on its name in the legend).
foreign import ccall safe "Rplot_set_series_visible" rplot_set_series_visible :: Rid -> CInt -> CInt -> IO ()

foreign import ccall safe "Rplot_is_series_visible" rplot_is_series_visible :: Rid -> CInt -> IO CInt

-- | The name written along an axis.
foreign import ccall safe "Rplot_set_axis_label" rplot_set_axis_label :: Rid -> RPlotAxis -> CString -> IO ()

foreign import ccall safe "Rplot_get_axis_label" rplot_get_axis_label :: Rid -> RPlotAxis -> IO CString

foreign import ccall safe "Rplot_set_axis_scale" rplot_set_axis_scale :: Rid -> RPlotAxis -> RPlotScale -> IO ()

foreign import ccall safe "Rplot_get_axis_scale" rplot_get_axis_scale :: Rid -> RPlotAxis -> IO RPlotScale

-- | R_PLOT_SCALE_SYMLOG: the axis is linear from -threshold to threshold (default 1).
foreign import ccall safe "Rplot_set_axis_linear_threshold" rplot_set_axis_linear_threshold :: Rid -> RPlotAxis -> CDouble -> IO ()

-- | Shows this range of an axis; the axis then stops following the data.
foreign import ccall safe "Rplot_set_axis_range" rplot_set_axis_range :: Rid -> RPlotAxis -> CDouble -> CDouble -> IO ()

-- | The range an axis shows now (after the user moved or zoomed it).
foreign import ccall safe "Rplot_get_axis_range" rplot_get_axis_range :: Rid -> RPlotAxis -> Ptr CDouble -> Ptr CDouble -> IO ()

-- | The axis is fitted to the data whenever it changes (the default, until the view is moved; a double click turns it on again).
foreign import ccall safe "Rplot_set_axis_auto_fit" rplot_set_axis_auto_fit :: Rid -> RPlotAxis -> CInt -> IO ()

foreign import ccall safe "Rplot_is_axis_auto_fit" rplot_is_axis_auto_fit :: Rid -> RPlotAxis -> IO CInt

-- | How the labels of an axis are written: a printf format for one double ("%.1f s", "%g"), NULL: chosen from the ticks.
foreign import ccall safe "Rplot_set_axis_format" rplot_set_axis_format :: Rid -> RPlotAxis -> CString -> IO ()

-- | 1: shown, 0: hidden, -1: R_PLOT_X and R_PLOT_Y always, the second axes when something uses them (the default).
foreign import ccall safe "Rplot_set_axis_visible" rplot_set_axis_visible :: Rid -> RPlotAxis -> CInt -> IO ()

-- | Lines across the data at the ticks of an axis (on for R_PLOT_X and R_PLOT_Y).
foreign import ccall safe "Rplot_set_axis_grid" rplot_set_axis_grid :: Rid -> RPlotAxis -> CInt -> IO ()

-- | The values grow the other way (to the left, or down).
foreign import ccall safe "Rplot_set_axis_inverted" rplot_set_axis_inverted :: Rid -> RPlotAxis -> CInt -> IO ()

-- | The user cannot move or zoom this axis (the application still can).
foreign import ccall safe "Rplot_set_axis_locked" rplot_set_axis_locked :: Rid -> RPlotAxis -> CInt -> IO ()

-- | Fits every axis that is not locked to the data, and has them follow it again (what a double click does).
foreign import ccall safe "Rplot_fit" rplot_fit :: Rid -> IO ()

-- | Where the legend is (default R_PLOT_LEGEND_TOP_RIGHT).
foreign import ccall safe "Rplot_set_legend" rplot_set_legend :: Rid -> RPlotLegend -> IO ()

-- | Lines across the data through the pointer.
foreign import ccall safe "Rplot_set_crosshair" rplot_set_crosshair :: Rid -> CInt -> IO ()

-- | The coordinates of the pointer in a corner of the data (on by default).
foreign import ccall safe "Rplot_set_mouse_text" rplot_set_mouse_text :: Rid -> CInt -> IO ()

-- | The values of the point under the pointer, written next to it (on by default).
foreign import ccall safe "Rplot_set_tooltips" rplot_set_tooltips :: Rid -> CInt -> IO ()

-- | Where the pointer is, in the units of R_PLOT_X and R_PLOT_Y. Returns R_TRUE when it is over the data.
foreign import ccall safe "Rplot_get_mouse_position" rplot_get_mouse_position :: Rid -> Ptr CDouble -> Ptr CDouble -> IO CInt

-- | Where a point of the data (R_PLOT_X, R_PLOT_Y) is in the widget, in logical units from its corner.
foreign import ccall safe "Rplot_data_to_point" rplot_data_to_point :: Rid -> CDouble -> CDouble -> Ptr CFloat -> Ptr CFloat -> IO ()

-- | The data (R_PLOT_X, R_PLOT_Y) at a point of the widget, in logical units from its corner.
foreign import ccall safe "Rplot_point_to_data" rplot_point_to_data :: Rid -> CFloat -> CFloat -> Ptr CDouble -> Ptr CDouble -> IO ()

-- | Adds a line across the data that the user drags: vertical at an X
-- (axis R_PLOT_X or R_PLOT_X2), horizontal at a Y (R_PLOT_Y or R_PLOT_Y2).
-- Its value is written on it, after `label` (may be NULL). Returns its index.
foreign import ccall safe "Rplot_add_drag_line" rplot_add_drag_line :: Rid -> RPlotAxis -> CDouble -> CString -> IO CInt

foreign import ccall safe "Rplot_remove_drag_line" rplot_remove_drag_line :: Rid -> CInt -> IO ()

foreign import ccall safe "Rplot_get_drag_line_count" rplot_get_drag_line_count :: Rid -> IO CInt

foreign import ccall safe "Rplot_set_drag_line_value" rplot_set_drag_line_value :: Rid -> CInt -> CDouble -> IO ()

foreign import ccall safe "Rplot_get_drag_line_value" rplot_get_drag_line_value :: Rid -> CInt -> IO CDouble

-- | The color of a drag line (by default the accent of the theme).
foreign import ccall safe "Rplot_set_drag_line_color" rplot_set_drag_line_color :: Rid -> CInt -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rplot_set_view_changed_callback" rplot_set_view_changed_callback :: Rid -> RPlotViewCallback -> Ptr () -> IO ()

foreign import ccall safe "Rplot_set_drag_line_callback" rplot_set_drag_line_callback :: Rid -> RPlotDragLineCallback -> Ptr () -> IO ()

foreign import ccall safe "Rplot_set_series_toggled_callback" rplot_set_series_toggled_callback :: Rid -> RPlotSeriesCallback -> Ptr () -> IO ()

-- | text Initial content, or NULL.
foreign import ccall safe "Rtext_editor_create" rtext_editor_create :: CString -> IO Rid

-- | Replaces the whole text. The history of undo is cleared; the changed callback is not called.
foreign import ccall safe "Rtext_editor_set_text" rtext_editor_set_text :: Rid -> CString -> IO ()

-- | Returns: The whole text, lines separated by '\n'. Valid until the next call or the next edit.
foreign import ccall safe "Rtext_editor_get_text" rtext_editor_get_text :: Rid -> IO CString

-- | Returns: The number of bytes of Rtext_editor_get_text.
foreign import ccall safe "Rtext_editor_get_length" rtext_editor_get_length :: Rid -> IO CInt

foreign import ccall safe "Rtext_editor_get_line_count" rtext_editor_get_line_count :: Rid -> IO CInt

-- | Returns: One line, without its line break. Valid until the next edit. NULL when `line` is out of range.
foreign import ccall safe "Rtext_editor_get_line" rtext_editor_get_line :: Rid -> CInt -> IO CString

-- | Types a text at the cursor, replacing the selection, as the user
-- would: it can be undone and the changed callback is called.
foreign import ccall safe "Rtext_editor_insert_text" rtext_editor_insert_text :: Rid -> CString -> IO ()

foreign import ccall safe "Rtext_editor_set_cursor" rtext_editor_set_cursor :: Rid -> CInt -> CInt -> IO ()

foreign import ccall safe "Rtext_editor_get_cursor" rtext_editor_get_cursor :: Rid -> Ptr CInt -> Ptr CInt -> IO ()

-- | Selects from one position to another; the cursor is on the second.
foreign import ccall safe "Rtext_editor_select" rtext_editor_select :: Rid -> CInt -> CInt -> CInt -> CInt -> IO ()

foreign import ccall safe "Rtext_editor_select_all" rtext_editor_select_all :: Rid -> IO ()

-- | Start and end of the selection, in the order of the text. Returns R_FALSE when nothing is selected.
foreign import ccall safe "Rtext_editor_get_selection" rtext_editor_get_selection :: Rid -> Ptr CInt -> Ptr CInt -> Ptr CInt -> Ptr CInt -> IO CInt

-- | Returns: The selected text, "" when there is none. Valid until the next call or the next edit.
foreign import ccall safe "Rtext_editor_get_selected_text" rtext_editor_get_selected_text :: Rid -> IO CString

-- | Scrolls so that a line is visible.
foreign import ccall safe "Rtext_editor_scroll_to_line" rtext_editor_scroll_to_line :: Rid -> CInt -> IO ()

foreign import ccall safe "Rtext_editor_undo" rtext_editor_undo :: Rid -> IO ()

foreign import ccall safe "Rtext_editor_redo" rtext_editor_redo :: Rid -> IO ()

foreign import ccall safe "Rtext_editor_can_undo" rtext_editor_can_undo :: Rid -> IO CInt

foreign import ccall safe "Rtext_editor_can_redo" rtext_editor_can_redo :: Rid -> IO CInt

-- | A read-only editor can still be scrolled, selected and copied.
foreign import ccall safe "Rtext_editor_set_read_only" rtext_editor_set_read_only :: Rid -> CInt -> IO ()

-- | Long lines continue on the next row instead of scrolling sideways. Off by default.
foreign import ccall safe "Rtext_editor_set_word_wrap" rtext_editor_set_word_wrap :: Rid -> CInt -> IO ()

-- | Shows the line numbers in a margin. Off by default.
foreign import ccall safe "Rtext_editor_set_line_numbers" rtext_editor_set_line_numbers :: Rid -> CInt -> IO ()

-- | Width of a tab, in spaces (4 by default).
foreign import ccall safe "Rtext_editor_set_tab_size" rtext_editor_set_tab_size :: Rid -> CInt -> IO ()

-- | R_TRUE makes the Tab key insert spaces instead of a tab character. Off by default.
foreign import ccall safe "Rtext_editor_set_tab_inserts_spaces" rtext_editor_set_tab_inserts_spaces :: Rid -> CInt -> IO ()

-- | The same as Rtext_editor_set_language with "XML", "C" or NULL.
foreign import ccall safe "Rtext_editor_set_syntax" rtext_editor_set_syntax :: Rid -> RSyntax -> IO ()

-- | font R_INVALID_ID uses the font of the theme (or the fixed-width
-- font, with a language).
-- size 0 uses the font size of the theme.
foreign import ccall safe "Rtext_editor_set_font" rtext_editor_set_font :: Rid -> Rid -> CFloat -> IO ()

-- | Text shown, dimmed, while the editor is empty.
foreign import ccall safe "Rtext_editor_set_placeholder" rtext_editor_set_placeholder :: Rid -> CString -> IO ()

-- | Zoom of the text: a factor of its font size, 1 = 100 % (0.5 .. 4).
-- The user zooms with Ctrl + wheel (around the pointer), Ctrl + '+',
-- Ctrl + '-' and Ctrl + 0 (back to 100 %), keypad included. Set by the
-- program, the top of the view stays and the zoom callback is not called.
-- XML: Zoom="1.25".
foreign import ccall safe "Rtext_editor_set_zoom" rtext_editor_set_zoom :: Rid -> CFloat -> IO ()

foreign import ccall safe "Rtext_editor_get_zoom" rtext_editor_get_zoom :: Rid -> IO CFloat

-- | R_FALSE: Ctrl + wheel and the keys do not zoom (the wheel scrolls). On by default. XML: Zoomable="false".
foreign import ccall safe "Rtext_editor_set_zoomable" rtext_editor_set_zoomable :: Rid -> CInt -> IO ()

foreign import ccall safe "Rtext_editor_set_zoom_callback" rtext_editor_set_zoom_callback :: Rid -> RTextEditorZoomCallback -> Ptr () -> IO ()

-- | Declares a language, or replaces the one that has this name: to
-- add words to a built-in language, read it with Rlanguage_get, change
-- what you want and register it again. The strings are copied.
foreign import ccall safe "Rlanguage_register" rlanguage_register :: Ptr RLanguageDesc -> IO CInt

-- | Reads the description of a language. The strings belong to the library. Returns R_FALSE when it is unknown.
foreign import ccall safe "Rlanguage_get" rlanguage_get :: CString -> Ptr RLanguageDesc -> IO CInt

foreign import ccall safe "Rlanguage_get_count" rlanguage_get_count :: IO CInt

foreign import ccall safe "Rlanguage_get_name" rlanguage_get_name :: CInt -> IO CString

-- | Returns: The name of the language of a file, from its extension, or NULL.
foreign import ccall safe "Rlanguage_for_file" rlanguage_for_file :: CString -> IO CString

-- | Gives the editor a language, by name ("C++", "Python"...), by
-- another usual name ("cpp", "js") or by the extension of its files
-- ("rs"). NULL or "" removes it. An editor with a language uses the
-- fixed-width font unless it was given a font.
foreign import ccall safe "Rtext_editor_set_language" rtext_editor_set_language :: Rid -> CString -> IO ()

-- | Returns: The name of the language of the editor, or "".
foreign import ccall safe "Rtext_editor_get_language" rtext_editor_get_language :: Rid -> IO CString

-- | R_TRUE makes the editor an editor of Markdown (the "Markdown"
-- language; Markdown="true" in XML): the text stays as it is written,
-- and what it formats shows while typing, strong text in bold,
-- emphasized text in italic, titles in bold, code, links and marks in
-- their colors. A label with Rlabel_set_markdown shows the same text
-- formatted. R_FALSE removes the language.
foreign import ccall safe "Rtext_editor_set_markdown" rtext_editor_set_markdown :: Rid -> CInt -> IO ()

foreign import ccall safe "Rtext_editor_get_markdown" rtext_editor_get_markdown :: Rid -> IO CInt

-- | What the language of the editor makes of the character at a
-- position: the colors come from it. R_TOKEN_TEXT without a language.
foreign import ccall safe "Rtext_editor_get_token" rtext_editor_get_token :: Rid -> CInt -> CInt -> IO RToken

-- | R_FALSE stops the editor from proposing words by itself; Ctrl+Space still does. On by default.
foreign import ccall safe "Rtext_editor_set_completion" rtext_editor_set_completion :: Rid -> CInt -> IO ()

-- | Proposes words for what is before the cursor, as Ctrl+Space does.
foreign import ccall safe "Rtext_editor_show_completions" rtext_editor_show_completions :: Rid -> IO ()

foreign import ccall safe "Rtext_editor_is_completing" rtext_editor_is_completing :: Rid -> IO CInt

foreign import ccall safe "Rtext_editor_set_completion_callback" rtext_editor_set_completion_callback :: Rid -> RCompletionCallback -> Ptr () -> IO ()

-- | Adds a proposal. Only from a completion callback. Proposals that
-- do not start with the prefix are left out; the others keep the order
-- they were added in.
-- detail A few words shown beside it ("function", "int"), or NULL.
foreign import ccall safe "Rtext_editor_add_completion" rtext_editor_add_completion :: Rid -> CString -> CString -> IO ()

-- | Adds a proposal that writes another text than the one it shows
-- and is matched on: "Threshold", listed and filtered as such, that
-- writes "Threshold(" when chosen. Only from a completion callback.
-- insert What replaces the word being typed; NULL or "" is `text`.
foreign import ccall safe "Rtext_editor_add_completion_insert" rtext_editor_add_completion_insert :: Rid -> CString -> CString -> CString -> IO ()

-- | R_FALSE leaves the words of the language and the names of the
-- text out of the proposals: only the completion callback's are listed
-- (a language server that knows better). On by default.
foreign import ccall safe "Rtext_editor_set_completion_words" rtext_editor_set_completion_words :: Rid -> CInt -> IO ()

-- | R_FALSE stops the editor from closing brackets and quotes and
-- from indenting after them. On by default; it only acts with a language.
foreign import ccall safe "Rtext_editor_set_auto_pairs" rtext_editor_set_auto_pairs :: Rid -> CInt -> IO ()

-- | Underlines a part of a line and attaches a message to it, shown
-- when the pointer rests there: an error of a compiler, a warning.
-- Markers follow their line when text is added or removed above it, and
-- go away when their own line is edited.
-- to_column -1 for the end of the line.
foreign import ccall safe "Rtext_editor_add_marker" rtext_editor_add_marker :: Rid -> CInt -> CInt -> CInt -> RMarkerKind -> CString -> IO ()

foreign import ccall safe "Rtext_editor_clear_markers" rtext_editor_clear_markers :: Rid -> IO ()

foreign import ccall safe "Rtext_editor_get_marker_count" rtext_editor_get_marker_count :: Rid -> IO CInt

foreign import ccall safe "Rtext_editor_set_hover_callback" rtext_editor_set_hover_callback :: Rid -> RTextEditorHoverCallback -> Ptr () -> IO ()

-- | Color of a kind of token in the theme in use, instead of the
-- one Raven picks for dark and for light themes. Theme files set them
-- with <Syntax Keyword="#FF7AB2" String="..."/>.
foreign import ccall safe "Rtheme_set_token_color" rtheme_set_token_color :: RToken -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rtheme_get_token_color" rtheme_get_token_color :: RToken -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> IO ()

-- | Back to the colors Raven picks.
foreign import ccall safe "Rtheme_clear_token_colors" rtheme_clear_token_colors :: IO ()

foreign import ccall safe "Rtext_editor_set_changed_callback" rtext_editor_set_changed_callback :: Rid -> RTextEditorChangedCallback -> Ptr () -> IO ()

-- | title Text shown in the bar; NULL shows the title of the window.
foreign import ccall safe "Rheader_bar_create" rheader_bar_create :: CString -> IO Rid

-- | title NULL shows the title of the window.
foreign import ccall safe "Rheader_bar_set_title" rheader_bar_set_title :: Rid -> CString -> IO ()

-- | buttons Bitmask of RHeaderButtons. All by default; the maximize button only shows on a resizable window.
foreign import ccall safe "Rheader_bar_set_buttons" rheader_bar_set_buttons :: Rid -> Word32 -> IO ()

-- | side R_HALIGN_RIGHT: minimize, maximize, close, as on Windows.
-- R_HALIGN_LEFT: three round colored buttons, as on macOS. Until it is
-- set, the side is the one of the system the program runs on (left on
-- macOS, right elsewhere), whatever the look.
foreign import ccall safe "Rheader_bar_set_button_side" rheader_bar_set_button_side :: Rid -> RHAlign -> IO ()

-- | align R_HALIGN_CENTER (default) or R_HALIGN_LEFT.
foreign import ccall safe "Rheader_bar_set_title_align" rheader_bar_set_title_align :: Rid -> RHAlign -> IO ()

-- | R_TRUE: the title sits in a dark tab hanging from the top edge of
-- the bar (slanted sides, round bottom corners). XML: TitleStyle="Notch".
foreign import ccall safe "Rheader_bar_set_title_notch" rheader_bar_set_title_notch :: Rid -> CInt -> IO ()

-- | R_FALSE removes the background of the bar: for a header that
-- floats above the content (in an Overlay) or sits inside a glass panel.
foreign import ccall safe "Rheader_bar_set_background" rheader_bar_set_background :: Rid -> CInt -> IO ()

foreign import ccall safe "Rdock_area_create" rdock_area_create :: IO Rid

-- | Adds a panel. The dock area takes ownership of the widget.
-- title    Text of its tab.
-- side     Where it goes.
-- relative A panel of the area to split or to share tabs with, or
--                 R_INVALID_ID for the whole area (with R_DOCK_TAB: the
--                 first group of tabs).
-- size     Part of the split space given to the new panel, 0..1
--                 (0 = a half, or a quarter along the whole area).
foreign import ccall safe "Rdock_area_add_panel" rdock_area_add_panel :: Rid -> Rid -> CString -> RDockSide -> Rid -> CFloat -> IO ()

-- | Moves a panel of the area somewhere else in it.
foreign import ccall safe "Rdock_area_move_panel" rdock_area_move_panel :: Rid -> Rid -> RDockSide -> Rid -> CFloat -> IO ()

-- | Removes a panel and deletes it.
foreign import ccall safe "Rdock_area_close_panel" rdock_area_close_panel :: Rid -> Rid -> IO ()

foreign import ccall safe "Rdock_area_set_panel_title" rdock_area_set_panel_title :: Rid -> Rid -> CString -> IO ()

-- | Icon drawn before the title of the tab of a panel (and on the chip while it is dragged).
-- texture Texture of the icon, R_INVALID_ID removes it.
-- u0,v0,u1,v1 Region of the texture (0, 0, 1, 1 : all of it).
-- tint    R_TRUE: the texture is a mask drawn in the text color of the tab (monochrome icons).
foreign import ccall safe "Rdock_area_set_panel_icon" rdock_area_set_panel_icon :: Rid -> Rid -> Rid -> CFloat -> CFloat -> CFloat -> CFloat -> CInt -> IO ()

-- | R_FALSE removes the close button of the tab of a panel. Panels can be closed by default.
foreign import ccall safe "Rdock_area_set_panel_closable" rdock_area_set_panel_closable :: Rid -> Rid -> CInt -> IO ()

-- | Takes a panel out of the arrangement without deleting it (a
-- window of an application closed from its menu). Its tab disappears
-- and its neighbours take its place; the area remembers where it was.
foreign import ccall safe "Rdock_area_hide_panel" rdock_area_hide_panel :: Rid -> Rid -> IO ()

-- | Puts a hidden panel back where it was, and brings its tab to the front (also for a panel that is not hidden).
foreign import ccall safe "Rdock_area_open_panel" rdock_area_open_panel :: Rid -> Rid -> IO ()

-- | Returns: R_TRUE when the panel is in the arrangement (not hidden), shown or behind another tab.
foreign import ccall safe "Rdock_area_is_panel_open" rdock_area_is_panel_open :: Rid -> Rid -> IO CInt

-- | R_TRUE: the close button of a tab hides its panel
-- (Rdock_area_hide_panel) instead of deleting it. XML: CloseHides="true".
-- The close callback is asked first either way.
foreign import ccall safe "Rdock_area_set_close_hides" rdock_area_set_close_hides :: Rid -> CInt -> IO ()

-- | Brings the tab of a panel to the front.
foreign import ccall safe "Rdock_area_show_panel" rdock_area_show_panel :: Rid -> Rid -> IO ()

-- | Returns: R_TRUE when the panel is the one shown in its group of tabs.
foreign import ccall safe "Rdock_area_is_panel_shown" rdock_area_is_panel_shown :: Rid -> Rid -> IO CInt

-- | R_FALSE stops the user from moving the panels (the borders can still be dragged).
foreign import ccall safe "Rdock_area_set_movable" rdock_area_set_movable :: Rid -> CInt -> IO ()

-- | Describes how the panels are arranged, as a short text to keep
-- in a settings file. Panels are named by their widget name
-- (Rwidget_set_name): unnamed panels are not part of the description.
-- Returns: A string valid until the next call.
foreign import ccall safe "Rdock_area_save_layout" rdock_area_save_layout :: Rid -> IO CString

-- | Arranges the panels as a saved description says. Panels of the
-- area that the description does not name keep a place; names it has
-- that match no panel are ignored.
-- Returns: R_FALSE when the text is not a layout (nothing is changed).
foreign import ccall safe "Rdock_area_load_layout" rdock_area_load_layout :: Rid -> CString -> IO CInt

foreign import ccall safe "Rdock_area_set_close_callback" rdock_area_set_close_callback :: Rid -> RDockCloseCallback -> Ptr () -> IO ()

-- | Tearing: a tab dragged out of the area takes its panel to a
-- window of its own (a floating area of the same family), and a tab
-- dragged from such a window over the area docks it back where it is
-- dropped. A double click on the tab of a floating window, or closing
-- that window, puts its panels back where they were. Dragging the only
-- tab of a floating window moves the window. R_TRUE by default; XML:
-- Tearable="false". The functions of this area that take a panel also
-- work on its panels that are in floating windows.
foreign import ccall safe "Rdock_area_set_tearable" rdock_area_set_tearable :: Rid -> CInt -> IO ()

foreign import ccall safe "Rdock_area_is_tearable" rdock_area_is_tearable :: Rid -> IO CInt

-- | Takes a panel to a window of its own, its top left corner at (x, y) on the screen. Returns the window.
foreign import ccall safe "Rdock_area_float_panel" rdock_area_float_panel :: Rid -> Rid -> CInt -> CInt -> IO Rid

-- | A floating panel goes back where it was in the area.
foreign import ccall safe "Rdock_area_dock_panel" rdock_area_dock_panel :: Rid -> Rid -> IO ()

-- | The window a floating panel is in, R_INVALID_ID when it is docked in the area.
foreign import ccall safe "Rdock_area_get_panel_window" rdock_area_get_panel_window :: Rid -> Rid -> IO Rid

foreign import ccall safe "Rdock_area_set_changed_callback" rdock_area_set_changed_callback :: Rid -> RDockChangedCallback -> Ptr () -> IO ()

foreign import ccall safe "Risland_create" risland_create :: IO Rid

-- | Unfolds or folds the island. The changed callback is not called.
foreign import ccall safe "Risland_set_expanded" risland_set_expanded :: Rid -> CInt -> IO ()

foreign import ccall safe "Risland_is_expanded" risland_is_expanded :: Rid -> IO CInt

-- | Unfolds the island, and folds it back by itself after a while:
-- to tell something to the user. It stays open while the pointer is on it.
foreign import ccall safe "Risland_expand_for" risland_expand_for :: Rid -> CDouble -> IO ()

-- | R_TRUE (default): the island unfolds under the pointer and folds back when it leaves.
foreign import ccall safe "Risland_set_expand_on_hover" risland_set_expand_on_hover :: Rid -> CInt -> IO ()

-- | R_TRUE (default): a click on the island unfolds or folds it.
foreign import ccall safe "Risland_set_expand_on_click" risland_set_expand_on_click :: Rid -> CInt -> IO ()

-- | R_TRUE gives the island square upper corners: a notch that hangs
-- from the edge it touches, instead of a capsule that floats under it.
foreign import ccall safe "Risland_set_attached" risland_set_attached :: Rid -> CInt -> IO ()

-- | R_TRUE: on a screen with a camera housing at its top (the notch
-- of a MacBook), the island of a window (Rwindow_set_island) lives in the
-- housing itself, in a window of its own anchored to the screen: it stays
-- there when the window is moved, and takes the mouse and the keyboard as
-- usual. Folded, it is the housing grown into two ears: the first Spacer
-- of its folded content becomes the room of the camera (what is before
-- it shows on the left, what is after on the right). Unfolded, its
-- content starts under the camera.
--
-- It is in the housing while its window is the last one of the
-- application the user worked in, is shown, and is on that screen; it
-- goes back into its window otherwise, and on the window systems and
-- screens without a housing (where nothing changes). XML: Notch="true".
foreign import ccall safe "Risland_set_notch" risland_set_notch :: Rid -> CInt -> IO ()

foreign import ccall safe "Risland_get_notch" risland_get_notch :: Rid -> IO CInt

-- | R_TRUE while the island is in the camera housing of the screen.
foreign import ccall safe "Risland_is_in_notch" risland_is_in_notch :: Rid -> IO CInt

foreign import ccall safe "Risland_set_changed_callback" risland_set_changed_callback :: Rid -> RIslandChangedCallback -> Ptr () -> IO ()

-- | Shows a widget (usually an island) floating at the top of a
-- window, in the middle, above its content. The window takes ownership;
-- the previous one is deleted. R_INVALID_ID removes it. The padding of
-- its slot (Rslot_set_padding) is its distance from the top edge.
foreign import ccall safe "Rwindow_set_island" rwindow_set_island :: Rid -> Rid -> IO ()

foreign import ccall safe "Rwindow_get_island" rwindow_get_island :: Rid -> IO Rid

-- | A progress bar on the icon of the application: on the button of
-- the window in the taskbar (Windows), on the icon in the Dock (macOS),
-- on the launcher (Linux). There is one icon for the whole application in
-- the Dock and the launchers: the last window that set it is shown.
-- value From 0 to 1 (not used by NONE and INDETERMINATE).
-- Returns: R_FALSE where the system shows no progress.
foreign import ccall safe "Rwindow_set_taskbar_progress" rwindow_set_taskbar_progress :: Rid -> RTaskbarProgress -> CFloat -> IO CInt

-- | Asks the user to look at the window when it is not the active
-- one: its taskbar button flashes (Windows), the icon bounces in the
-- Dock (macOS), the window is marked urgent (X11, the launchers of
-- Linux). `critical`: until the user comes to the window; otherwise a few
-- times. Nothing happens when the window is already the active one.
foreign import ccall safe "Rwindow_request_attention" rwindow_request_attention :: Rid -> CInt -> IO CInt

-- | A short text on the icon of the application: a number of unread
-- messages, "!". macOS: the badge of the Dock. Windows: a small round
-- icon on the taskbar buttons of the windows, drawn from the text (one
-- to three characters fit). Linux: a number only (the text is read as
-- one; anything else shows nothing). NULL or "": no badge.
foreign import ccall safe "Rtaskbar_set_badge" rtaskbar_set_badge :: CString -> IO CInt

-- | The menu of the icon of the application in the Dock (macOS),
-- above the entries of the system. A MENU without a parent: its
-- MenuItems fire as when chosen in a window (clicked callback, action),
-- and what changes in it shows the next time the menu opens.
-- R_INVALID_ID: none. R_FALSE where the system has no such menu.
foreign import ccall safe "Rtaskbar_set_menu" rtaskbar_set_menu :: Rid -> IO CInt

-- | The tasks of the jump list of Windows (right click on the button
-- of the application in the taskbar, or on its tile in the Start menu).
-- Choosing one starts the application again with its arguments: the
-- application reads them as any other command line. count 0: none.
-- R_FALSE where there is no jump list.
foreign import ccall safe "Rtaskbar_set_jump_list" rtaskbar_set_jump_list :: Ptr RJumpTask -> CInt -> IO CInt

-- | Returns: R_TRUE when the system shows tray icons: always on Windows and
-- macOS; on Linux, when a host of StatusNotifierItem runs (the panel of
-- the desktop). An icon made where none runs yet appears when one starts.
foreign import ccall safe "Rtray_is_supported" rtray_is_supported :: IO CInt

-- | Puts an icon of the application beside the clock. While one
-- exists, the application goes on running when its last window closes
-- (Ris_running): Rquit (or the action app.quit) ends it.
-- Returns: R_INVALID_ID where the system has none.
foreign import ccall safe "Rtray_create" rtray_create :: IO Rid

foreign import ccall safe "Rtray_destroy" rtray_destroy :: Rid -> IO ()

foreign import ccall safe "Rtray_is_valid" rtray_is_valid :: Rid -> IO CInt

-- | The picture, from tightly packed RGBA8 pixels (straight alpha);
-- the system scales it (16 to 32 pixels square fit everywhere). Copied.
foreign import ccall safe "Rtray_set_icon" rtray_set_icon :: Rid -> Ptr Word8 -> CInt -> CInt -> IO ()

-- | The text shown when the pointer rests on the icon (and its title for the Linux panels).
foreign import ccall safe "Rtray_set_tooltip" rtray_set_tooltip :: Rid -> CString -> IO ()

-- | The menu the system shows for the icon: on a right click
-- (Windows, Linux), on a click (macOS). A MENU without a parent, whose
-- MenuItems fire as when chosen in a window; what changes in it shows
-- the next time it opens. R_INVALID_ID: none.
foreign import ccall safe "Rtray_set_menu" rtray_set_menu :: Rid -> Rid -> IO ()

-- | Called on a click: the left button (activate), the right one
-- when there is no menu, the middle one. On macOS an icon with a menu
-- opens it instead.
foreign import ccall safe "Rtray_set_clicked_callback" rtray_set_clicked_callback :: Rid -> RTrayClickedCallback -> Ptr () -> IO ()

-- | A short text shown beside the icon (macOS menu bar), drawn by the
-- system in its own font and colors; the item widens to fit it. NULL or
-- "": the icon alone. Systems that show no text there ignore it (the
-- tooltip is the place for it).
foreign import ccall safe "Rtray_set_title" rtray_set_title :: Rid -> CString -> IO ()

-- | Where the icon is on the screen, in the units and coordinates of
-- Rwindow_set_position (a window can then open just under it).
-- Returns: R_FALSE when the system does not tell (the outputs are then 0).
foreign import ccall safe "Rtray_get_rect" rtray_get_rect :: Rid -> Ptr CInt -> Ptr CInt -> Ptr CInt -> Ptr CInt -> IO CInt

foreign import ccall safe "Rcolor_picker_create" rcolor_picker_create :: IO Rid

-- | Sets the color (0..1 each). The callback is not called.
foreign import ccall safe "Rcolor_picker_set_color" rcolor_picker_set_color :: Rid -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rcolor_picker_get_color" rcolor_picker_get_color :: Rid -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> IO ()

-- | The same color as hue (0..360), saturation and value (0..1).
-- The picker remembers the hue of a gray and the saturation of black,
-- which red, green and blue cannot tell.
foreign import ccall safe "Rcolor_picker_set_hsv" rcolor_picker_set_hsv :: Rid -> CFloat -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rcolor_picker_get_hsv" rcolor_picker_get_hsv :: Rid -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> IO ()

-- | Sets the color from text: "#RGB", "#RRGGBB", "#RRGGBBAA" (the
-- '#' may be left out) or one of the color names of the XML attributes.
-- A code without opacity leaves the opacity as it is.
-- Returns: R_FALSE when the text is not a color (the color is unchanged).
foreign import ccall safe "Rcolor_picker_set_hex" rcolor_picker_set_hex :: Rid -> CString -> IO CInt

-- | Returns: "#RRGGBB", or "#RRGGBBAA" when the opacity is shown and not full. Valid until the color changes.
foreign import ccall safe "Rcolor_picker_get_hex" rcolor_picker_get_hex :: Rid -> IO CString

-- | R_TRUE (default) shows the opacity strip and the fourth number. R_FALSE: colors are opaque.
foreign import ccall safe "Rcolor_picker_set_alpha_enabled" rcolor_picker_set_alpha_enabled :: Rid -> CInt -> IO ()

-- | The color shown next to the current one, to compare with what
-- was there before: clicking it comes back to it. Rcolor_picker_set_color
-- does not change it; a popup picker takes it when it opens.
foreign import ccall safe "Rcolor_picker_set_previous_color" rcolor_picker_set_previous_color :: Rid -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

-- | Adds a color to the row of swatches: one click takes it.
foreign import ccall safe "Rcolor_picker_add_swatch" rcolor_picker_add_swatch :: Rid -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rcolor_picker_clear_swatches" rcolor_picker_clear_swatches :: Rid -> IO ()

foreign import ccall safe "Rcolor_picker_get_swatch_count" rcolor_picker_get_swatch_count :: Rid -> IO CInt

-- | R_TRUE turns the widget into a swatch that opens the picker in a popup.
foreign import ccall safe "Rcolor_picker_set_popup" rcolor_picker_set_popup :: Rid -> CInt -> IO ()

-- | R_TRUE hides the code and the numbers: only the square, the strips and the swatches.
foreign import ccall safe "Rcolor_picker_set_compact" rcolor_picker_set_compact :: Rid -> CInt -> IO ()

foreign import ccall safe "Rcolor_picker_set_changed_callback" rcolor_picker_set_changed_callback :: Rid -> RColorChangedCallback -> Ptr () -> IO ()

-- | Red, green, blue (0..1) to hue (0..360), saturation, value (0..1).
foreign import ccall safe "Rcolor_rgb_to_hsv" rcolor_rgb_to_hsv :: CFloat -> CFloat -> CFloat -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> IO ()

foreign import ccall safe "Rcolor_hsv_to_rgb" rcolor_hsv_to_rgb :: CFloat -> CFloat -> CFloat -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> IO ()

foreign import ccall safe "Rtimeline_create" rtimeline_create :: IO Rid

-- | Length of the sequence. The view and the playhead stay within 0 .. duration.
foreign import ccall safe "Rtimeline_set_duration" rtimeline_set_duration :: Rid -> CDouble -> IO ()

foreign import ccall safe "Rtimeline_get_duration" rtimeline_get_duration :: Rid -> IO CDouble

-- | Moves the playhead. The time callback is not called.
foreign import ccall safe "Rtimeline_set_time" rtimeline_set_time :: Rid -> CDouble -> IO ()

foreign import ccall safe "Rtimeline_get_time" rtimeline_get_time :: Rid -> IO CDouble

-- | Frames per second. With a frame rate the ruler counts in
-- frames ("2:12" is second 2, frame 12) and everything snaps to frames;
-- 0 (default) counts in seconds and snaps to the marks of the ruler.
foreign import ccall safe "Rtimeline_set_frame_rate" rtimeline_set_frame_rate :: Rid -> CDouble -> IO ()

foreign import ccall safe "Rtimeline_get_frame_rate" rtimeline_get_frame_rate :: Rid -> IO CDouble

-- | R_FALSE lets items be placed anywhere (holding Alt while dragging does it for one move).
foreign import ccall safe "Rtimeline_set_snapping" rtimeline_set_snapping :: Rid -> CInt -> IO ()

-- | The part of the sequence that is shown. Zooming is showing less.
foreign import ccall safe "Rtimeline_set_view" rtimeline_set_view :: Rid -> CDouble -> CDouble -> IO ()

foreign import ccall safe "Rtimeline_get_view" rtimeline_get_view :: Rid -> Ptr CDouble -> Ptr CDouble -> IO ()

-- | Makes the playhead run by itself, at the speed of the clock,
-- calling the time callback at each frame; at the end it stops, or
-- starts again with Rtimeline_set_loop.
foreign import ccall safe "Rtimeline_set_playing" rtimeline_set_playing :: Rid -> CInt -> IO ()

foreign import ccall safe "Rtimeline_is_playing" rtimeline_is_playing :: Rid -> IO CInt

foreign import ccall safe "Rtimeline_set_loop" rtimeline_set_loop :: Rid -> CInt -> IO ()

-- | Width of the column of the names of the tracks. 0 (default): fits the longest name.
foreign import ccall safe "Rtimeline_set_header_width" rtimeline_set_header_width :: Rid -> CFloat -> IO ()

-- | Returns: The width of the column of the names, as laid out now.
foreign import ccall safe "Rtimeline_get_header_width" rtimeline_get_header_width :: Rid -> IO CFloat

-- | Height of the ruler of time (and of the markers' flags band). 0 (default): automatic.
foreign import ccall safe "Rtimeline_set_ruler_height" rtimeline_set_ruler_height :: Rid -> CFloat -> IO ()

foreign import ccall safe "Rtimeline_get_ruler_height" rtimeline_get_ruler_height :: Rid -> IO CFloat

-- | Height of the tracks that have none of their own (Rtimeline_set_track_height). 0 (default): automatic.
foreign import ccall safe "Rtimeline_set_default_track_height" rtimeline_set_default_track_height :: Rid -> CFloat -> IO ()

foreign import ccall safe "Rtimeline_get_default_track_height" rtimeline_get_default_track_height :: Rid -> IO CFloat

-- | Returns: The track, or 0. Tracks are shown in the order they were added.
foreign import ccall safe "Rtimeline_add_track" rtimeline_add_track :: Rid -> CString -> IO CInt

-- | Removes a track and everything on it.
foreign import ccall safe "Rtimeline_remove_track" rtimeline_remove_track :: Rid -> CInt -> IO ()

foreign import ccall safe "Rtimeline_get_track_count" rtimeline_get_track_count :: Rid -> IO CInt

-- | Returns: The index-th track from the top, or 0.
foreign import ccall safe "Rtimeline_get_track" rtimeline_get_track :: Rid -> CInt -> IO CInt

foreign import ccall safe "Rtimeline_set_track_name" rtimeline_set_track_name :: Rid -> CInt -> CString -> IO ()

foreign import ccall safe "Rtimeline_get_track_name" rtimeline_get_track_name :: Rid -> CInt -> IO CString

-- | Color of the clips and keys of the track that have none of their own.
foreign import ccall safe "Rtimeline_set_track_color" rtimeline_set_track_color :: Rid -> CInt -> CFloat -> CFloat -> CFloat -> IO ()

-- | Height of the track in logical units; 0 = the default.
foreign import ccall safe "Rtimeline_set_track_height" rtimeline_set_track_height :: Rid -> CInt -> CFloat -> IO ()

-- | A locked track shows its items but does not let the user change them.
foreign import ccall safe "Rtimeline_set_track_locked" rtimeline_set_track_locked :: Rid -> CInt -> CInt -> IO ()

-- | A muted track is drawn dimmed; what muting means is up to the application.
foreign import ccall safe "Rtimeline_set_track_muted" rtimeline_set_track_muted :: Rid -> CInt -> CInt -> IO ()

-- | A property of the look of a track: "ClipStyle" (Solid,
-- Gradient, Tint, Outline), "KeyShape" (Diamond, Circle, Square...),
-- "KeySize", "ClipRadius", "Background" (its lane), or a part and a
-- property ("Clip.Glow", "Key.BorderColor"). "" or "Default" gives back
-- the one of the timeline.
-- Returns: R_FALSE when the name or the value is not one.
foreign import ccall safe "Rtimeline_set_track_style" rtimeline_set_track_style :: Rid -> CInt -> CString -> CString -> IO CInt

foreign import ccall safe "Rtimeline_is_track_muted" rtimeline_is_track_muted :: Rid -> CInt -> IO CInt

-- | Returns: The clip, or 0 when the track does not exist.
foreign import ccall safe "Rtimeline_add_clip" rtimeline_add_clip :: Rid -> CInt -> CDouble -> CDouble -> CString -> IO CInt

-- | Returns: The key, or 0.
foreign import ccall safe "Rtimeline_add_key" rtimeline_add_key :: Rid -> CInt -> CDouble -> IO CInt

-- | A named moment of the whole sequence, shown on the ruler and across the tracks.
foreign import ccall safe "Rtimeline_add_marker" rtimeline_add_marker :: Rid -> CDouble -> CString -> IO CInt

foreign import ccall safe "Rtimeline_remove_item" rtimeline_remove_item :: Rid -> CInt -> IO ()

-- | Removes every track, clip, key and marker.
foreign import ccall safe "Rtimeline_clear" rtimeline_clear :: Rid -> IO ()

foreign import ccall safe "Rtimeline_get_item_type" rtimeline_get_item_type :: Rid -> CInt -> IO RTimelineItem

-- | Returns: The track of a clip or of a key; 0 for a marker.
foreign import ccall safe "Rtimeline_get_item_track" rtimeline_get_item_track :: Rid -> CInt -> IO CInt

foreign import ccall safe "Rtimeline_get_item_start" rtimeline_get_item_start :: Rid -> CInt -> IO CDouble

-- | Returns: The length of a clip; 0 for a key or a marker.
foreign import ccall safe "Rtimeline_get_item_length" rtimeline_get_item_length :: Rid -> CInt -> IO CDouble

-- | Moves an item: to another track (0 = keep), another start, another length (clips; negative = keep).
foreign import ccall safe "Rtimeline_set_item" rtimeline_set_item :: Rid -> CInt -> CInt -> CDouble -> CDouble -> IO ()

foreign import ccall safe "Rtimeline_set_item_label" rtimeline_set_item_label :: Rid -> CInt -> CString -> IO ()

foreign import ccall safe "Rtimeline_get_item_label" rtimeline_get_item_label :: Rid -> CInt -> IO CString

foreign import ccall safe "Rtimeline_set_item_color" rtimeline_set_item_color :: Rid -> CInt -> CFloat -> CFloat -> CFloat -> IO ()

-- | A property of the look of one clip, key or marker: "Style"
-- (Solid, Gradient, Tint, Outline: clips), "Shape" (keys), or any
-- property of its part ("Background", "BorderColor", "Glow",
-- "CornerRadius", "Foreground" for its label...).
foreign import ccall safe "Rtimeline_set_item_style" rtimeline_set_item_style :: Rid -> CInt -> CString -> CString -> IO CInt

-- | The look the timeline starts from (R_PRESET_AUTO by default; Preset="..." in XML).
foreign import ccall safe "Rtimeline_set_preset" rtimeline_set_preset :: Rid -> RVisualPreset -> IO ()

foreign import ccall safe "Rtimeline_get_preset" rtimeline_get_preset :: Rid -> IO RVisualPreset

foreign import ccall safe "Rtimeline_get_item_count" rtimeline_get_item_count :: Rid -> IO CInt

-- | Returns: The index-th item, in the order they were added, or 0.
foreign import ccall safe "Rtimeline_get_item" rtimeline_get_item :: Rid -> CInt -> IO CInt

-- | Selects an item, alone or (add) with the ones already selected.
foreign import ccall safe "Rtimeline_select" rtimeline_select :: Rid -> CInt -> CInt -> IO ()

foreign import ccall safe "Rtimeline_clear_selection" rtimeline_clear_selection :: Rid -> IO ()

foreign import ccall safe "Rtimeline_is_selected" rtimeline_is_selected :: Rid -> CInt -> IO CInt

foreign import ccall safe "Rtimeline_get_selection_count" rtimeline_get_selection_count :: Rid -> IO CInt

foreign import ccall safe "Rtimeline_get_selected" rtimeline_get_selected :: Rid -> CInt -> IO CInt

foreign import ccall safe "Rtimeline_set_time_callback" rtimeline_set_time_callback :: Rid -> RTimelineTimeCallback -> Ptr () -> IO ()

-- | Called for each item the user moved, stretched or retimed, as
-- it happens. The action of the widget (OnChanged in XML) fires once
-- per gesture, when the button is released.
foreign import ccall safe "Rtimeline_set_changed_callback" rtimeline_set_changed_callback :: Rid -> RTimelineItemCallback -> Ptr () -> IO ()

-- | Called when the selection changed; `item` is the item just clicked, or 0.
foreign import ccall safe "Rtimeline_set_selection_callback" rtimeline_set_selection_callback :: Rid -> RTimelineItemCallback -> Ptr () -> IO ()

-- | Called when an item is double-clicked.
foreign import ccall safe "Rtimeline_set_activated_callback" rtimeline_set_activated_callback :: Rid -> RTimelineItemCallback -> Ptr () -> IO ()

-- | Called when the user deletes an item (the Delete key), before
-- it is removed: its values can still be read. Without this callback the
-- Delete key does nothing: the application decides what can be deleted.
foreign import ccall safe "Rtimeline_set_removed_callback" rtimeline_set_removed_callback :: Rid -> RTimelineItemCallback -> Ptr () -> IO ()

foreign import ccall safe "Rnode_graph_create" rnode_graph_create :: IO Rid

-- | x,y Top-left corner of the node, in the units of the canvas.
foreign import ccall safe "Rnode_graph_add_node" rnode_graph_add_node :: Rid -> CString -> CFloat -> CFloat -> IO CInt

-- | Removes a node, its pins, their links, and the widget it holds.
foreign import ccall safe "Rnode_graph_remove_node" rnode_graph_remove_node :: Rid -> CInt -> IO ()

foreign import ccall safe "Rnode_graph_clear" rnode_graph_clear :: Rid -> IO ()

foreign import ccall safe "Rnode_graph_get_node_count" rnode_graph_get_node_count :: Rid -> IO CInt

foreign import ccall safe "Rnode_graph_get_node" rnode_graph_get_node :: Rid -> CInt -> IO CInt

foreign import ccall safe "Rnode_graph_set_node_title" rnode_graph_set_node_title :: Rid -> CInt -> CString -> IO ()

foreign import ccall safe "Rnode_graph_get_node_title" rnode_graph_get_node_title :: Rid -> CInt -> IO CString

foreign import ccall safe "Rnode_graph_set_node_position" rnode_graph_set_node_position :: Rid -> CInt -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rnode_graph_get_node_position" rnode_graph_get_node_position :: Rid -> CInt -> Ptr CFloat -> Ptr CFloat -> IO ()

-- | Size of a node on the canvas: Raven works it out from its title, its pins and its content.
foreign import ccall safe "Rnode_graph_get_node_size" rnode_graph_get_node_size :: Rid -> CInt -> Ptr CFloat -> Ptr CFloat -> IO ()

-- | Color of the header of the node; without one it takes the accent color of the theme.
foreign import ccall safe "Rnode_graph_set_node_color" rnode_graph_set_node_color :: Rid -> CInt -> CFloat -> CFloat -> CFloat -> IO ()

-- | A property of the look of one node: "HeaderStyle" (Solid,
-- Gradient, Stripe, Tint, None), "HeaderColor", "TitleColor",
-- "Background", "BorderColor", "BorderWidth", "CornerRadius", "Glow",
-- "Icon" (an image file shown before its title), or a part and a property
-- ("Header.Gradient", "Node.ShadowSize"). "" or "Default" gives back the
-- one of the graph.
-- Returns: R_FALSE when the name or the value is not one.
foreign import ccall safe "Rnode_graph_set_node_style" rnode_graph_set_node_style :: Rid -> CInt -> CString -> CString -> IO CInt

-- | Puts a widget inside the node, under its pins: the graph owns
-- it from now on. R_INVALID_ID removes (and deletes) the one it had.
foreign import ccall safe "Rnode_graph_set_node_content" rnode_graph_set_node_content :: Rid -> CInt -> Rid -> IO ()

foreign import ccall safe "Rnode_graph_get_node_content" rnode_graph_get_node_content :: Rid -> CInt -> IO Rid

-- | Name of the node, the `Name` of its XML element: what
-- <Link From="name.pin"> and Rnode_graph_find_node look for. Not shown.
foreign import ccall safe "Rnode_graph_set_node_name" rnode_graph_set_node_name :: Rid -> CInt -> CString -> IO ()

foreign import ccall safe "Rnode_graph_get_node_name" rnode_graph_get_node_name :: Rid -> CInt -> IO CString

-- | Returns: The node of that name, or 0.
foreign import ccall safe "Rnode_graph_find_node" rnode_graph_find_node :: Rid -> CString -> IO CInt

-- | Value the application attaches to a node.
foreign import ccall safe "Rnode_graph_set_node_user_data" rnode_graph_set_node_user_data :: Rid -> CInt -> Ptr () -> IO ()

foreign import ccall safe "Rnode_graph_get_node_user_data" rnode_graph_get_node_user_data :: Rid -> CInt -> IO (Ptr ())

-- | type Free text, or NULL for a pin that links to any type. Returns: The pin, or 0.
foreign import ccall safe "Rnode_graph_add_input" rnode_graph_add_input :: Rid -> CInt -> CString -> CString -> IO CInt

foreign import ccall safe "Rnode_graph_add_output" rnode_graph_add_output :: Rid -> CInt -> CString -> CString -> IO CInt

foreign import ccall safe "Rnode_graph_remove_pin" rnode_graph_remove_pin :: Rid -> CInt -> IO ()

foreign import ccall safe "Rnode_graph_get_pin_node" rnode_graph_get_pin_node :: Rid -> CInt -> IO CInt

foreign import ccall safe "Rnode_graph_is_pin_output" rnode_graph_is_pin_output :: Rid -> CInt -> IO CInt

foreign import ccall safe "Rnode_graph_get_pin_label" rnode_graph_get_pin_label :: Rid -> CInt -> IO CString

foreign import ccall safe "Rnode_graph_get_pin_type" rnode_graph_get_pin_type :: Rid -> CInt -> IO CString

-- | Name of the pin for Rnode_graph_find_pin and for the links of XML; a pin without one answers to its label.
foreign import ccall safe "Rnode_graph_set_pin_name" rnode_graph_set_pin_name :: Rid -> CInt -> CString -> IO ()

-- | Returns: The pin of a node with that name (or label), or 0.
foreign import ccall safe "Rnode_graph_find_pin" rnode_graph_find_pin :: Rid -> CInt -> CString -> IO CInt

-- | Where a pin is on the canvas (the middle of its dot).
foreign import ccall safe "Rnode_graph_get_pin_position" rnode_graph_get_pin_position :: Rid -> CInt -> Ptr CFloat -> Ptr CFloat -> IO ()

-- | Returns: The index-th input (or output) of a node, or 0.
foreign import ccall safe "Rnode_graph_get_input" rnode_graph_get_input :: Rid -> CInt -> CInt -> IO CInt

foreign import ccall safe "Rnode_graph_get_output" rnode_graph_get_output :: Rid -> CInt -> CInt -> IO CInt

-- | Color of the pins of a type and of the links between them.
foreign import ccall safe "Rnode_graph_set_type_color" rnode_graph_set_type_color :: Rid -> CString -> CFloat -> CFloat -> CFloat -> IO ()

-- | A property of the pins of a type and of their links: "Shape"
-- (Circle, Ring, Square, Diamond, Triangle, Arrow), "Size", "Glow",
-- "LinkShape" (Bezier, Straight, Step, Angled), "LinkWidth", "LinkGlow".
foreign import ccall safe "Rnode_graph_set_type_style" rnode_graph_set_type_style :: Rid -> CString -> CString -> CString -> IO CInt

-- | The same for one pin ("Shape", "Size", "Glow", "Color").
foreign import ccall safe "Rnode_graph_set_pin_style" rnode_graph_set_pin_style :: Rid -> CInt -> CString -> CString -> IO CInt

-- | Links an output to an input, as the user does by dragging.
-- The connect callback is not asked.
-- Returns: The link; 0 when the pins cannot be linked (two inputs, the
-- same node, different types).
foreign import ccall safe "Rnode_graph_connect" rnode_graph_connect :: Rid -> CInt -> CInt -> IO CInt

foreign import ccall safe "Rnode_graph_disconnect" rnode_graph_disconnect :: Rid -> CInt -> IO ()

foreign import ccall safe "Rnode_graph_get_link_count" rnode_graph_get_link_count :: Rid -> IO CInt

foreign import ccall safe "Rnode_graph_get_link" rnode_graph_get_link :: Rid -> CInt -> IO CInt

foreign import ccall safe "Rnode_graph_get_link_output" rnode_graph_get_link_output :: Rid -> CInt -> IO CInt

foreign import ccall safe "Rnode_graph_get_link_input" rnode_graph_get_link_input :: Rid -> CInt -> IO CInt

-- | Returns: The link that arrives at an input pin, or 0.
foreign import ccall safe "Rnode_graph_get_input_link" rnode_graph_get_input_link :: Rid -> CInt -> IO CInt

-- | What the middle of the widget shows: a point of the canvas, and how large (1 = one unit per logical unit).
foreign import ccall safe "Rnode_graph_set_view" rnode_graph_set_view :: Rid -> CFloat -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rnode_graph_get_view" rnode_graph_get_view :: Rid -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> IO ()

-- | Converts a point of the widget (from its corner, as in the events) to the canvas, and back, with the view as it is shown now.
foreign import ccall safe "Rnode_graph_to_canvas" rnode_graph_to_canvas :: Rid -> CFloat -> CFloat -> Ptr CFloat -> Ptr CFloat -> IO ()

foreign import ccall safe "Rnode_graph_from_canvas" rnode_graph_from_canvas :: Rid -> CFloat -> CFloat -> Ptr CFloat -> Ptr CFloat -> IO ()

-- | Moves and zooms so that every node shows.
foreign import ccall safe "Rnode_graph_frame_all" rnode_graph_frame_all :: Rid -> IO ()

-- | R_FALSE hides the grid of dots (shown by default).
foreign import ccall safe "Rnode_graph_set_grid_visible" rnode_graph_set_grid_visible :: Rid -> CInt -> IO ()

-- | The pulses of light that run along the links, from output to
-- input. By default they run while the pointer is over the graph, and
-- the window rests the rest of the time. R_TRUE makes them run all the
-- time (the window then draws without pause); R_FALSE stops them.
foreign import ccall safe "Rnode_graph_set_flow_visible" rnode_graph_set_flow_visible :: Rid -> CInt -> IO ()

-- | R_TRUE: the user can look but not move, link or delete.
foreign import ccall safe "Rnode_graph_set_read_only" rnode_graph_set_read_only :: Rid -> CInt -> IO ()

-- | The look the graph starts from (R_PRESET_AUTO by default; Preset="..." in XML).
foreign import ccall safe "Rnode_graph_set_preset" rnode_graph_set_preset :: Rid -> RVisualPreset -> IO ()

foreign import ccall safe "Rnode_graph_get_preset" rnode_graph_get_preset :: Rid -> IO RVisualPreset

foreign import ccall safe "Rnode_graph_select_node" rnode_graph_select_node :: Rid -> CInt -> CInt -> IO ()

foreign import ccall safe "Rnode_graph_clear_selection" rnode_graph_clear_selection :: Rid -> IO ()

foreign import ccall safe "Rnode_graph_is_node_selected" rnode_graph_is_node_selected :: Rid -> CInt -> IO CInt

foreign import ccall safe "Rnode_graph_get_selection_count" rnode_graph_get_selection_count :: Rid -> IO CInt

foreign import ccall safe "Rnode_graph_get_selected_node" rnode_graph_get_selected_node :: Rid -> CInt -> IO CInt

foreign import ccall safe "Rnode_graph_set_connect_callback" rnode_graph_set_connect_callback :: Rid -> RNodeGraphConnectCallback -> Ptr () -> IO ()

-- | Called after the user made a link, and before a link the user removed goes (its pins can still be read).
foreign import ccall safe "Rnode_graph_set_linked_callback" rnode_graph_set_linked_callback :: Rid -> RNodeGraphItemCallback -> Ptr () -> IO ()

foreign import ccall safe "Rnode_graph_set_unlinked_callback" rnode_graph_set_unlinked_callback :: Rid -> RNodeGraphItemCallback -> Ptr () -> IO ()

-- | Called for each node the user moved, when the button is released.
foreign import ccall safe "Rnode_graph_set_moved_callback" rnode_graph_set_moved_callback :: Rid -> RNodeGraphItemCallback -> Ptr () -> IO ()

-- | Called when the selection changed; `item` is the node just clicked, or 0.
foreign import ccall safe "Rnode_graph_set_selection_callback" rnode_graph_set_selection_callback :: Rid -> RNodeGraphItemCallback -> Ptr () -> IO ()

-- | Called when the user deletes a node (the Delete key), before
-- it is removed. Without this callback the Delete key only removes
-- links: the application decides whether nodes can be deleted.
foreign import ccall safe "Rnode_graph_set_removed_callback" rnode_graph_set_removed_callback :: Rid -> RNodeGraphItemCallback -> Ptr () -> IO ()

-- | Called when the user double-clicks a node (to open what it stands for).
foreign import ccall safe "Rnode_graph_set_activated_callback" rnode_graph_set_activated_callback :: Rid -> RNodeGraphItemCallback -> Ptr () -> IO ()

foreign import ccall safe "Rnode_graph_set_context_callback" rnode_graph_set_context_callback :: Rid -> RNodeGraphContextCallback -> Ptr () -> IO ()

-- | Double click on the empty canvas (`item` 0): the application acts
-- there (a search to add a node...). Without this callback it frames all nodes.
foreign import ccall safe "Rnode_graph_set_canvas_activated_callback" rnode_graph_set_canvas_activated_callback :: Rid -> RNodeGraphPointCallback -> Ptr () -> IO ()

-- | A wire the user pulled from a pin was released on nothing: `item` is
-- that pin, x, y where it was dropped (to offer a node to connect it to). The
-- wire draws back as usual.
foreign import ccall safe "Rnode_graph_set_wire_dropped_callback" rnode_graph_set_wire_dropped_callback :: Rid -> RNodeGraphPointCallback -> Ptr () -> IO ()

-- | A pin was clicked, not dragged (`item` is the pin, x, y on the canvas):
-- the application may link in two clicks (Rnode_graph_set_pin_emphasis shows where
-- the second may go). With this callback such a click neither detaches the wire
-- of an input nor counts as a wire dropped on nothing.
foreign import ccall safe "Rnode_graph_set_pin_clicked_callback" rnode_graph_set_pin_clicked_callback :: Rid -> RNodeGraphPointCallback -> Ptr () -> IO ()

-- | The node under a point of the widget (x, y as in the events: Rnode_graph_to_canvas), or 0.
foreign import ccall safe "Rnode_graph_get_node_at" rnode_graph_get_node_at :: Rid -> CFloat -> CFloat -> IO CInt

-- | The pin near a point of the widget (or whose tag is under it: Rnode_graph_set_pin_tag), or 0 (a menu for a pin).
foreign import ccall safe "Rnode_graph_get_pin_at" rnode_graph_get_pin_at :: Rid -> CFloat -> CFloat -> IO CInt

-- | The link under a point of the widget, or 0 (hidden links are never under it).
foreign import ccall safe "Rnode_graph_get_link_at" rnode_graph_get_link_at :: Rid -> CFloat -> CFloat -> IO CInt

-- | R_TRUE: this input keeps every link made to it (a state that several
-- transitions enter) instead of one. Pulling a wire from it starts a new one.
foreign import ccall safe "Rnode_graph_set_pin_multiple" rnode_graph_set_pin_multiple :: Rid -> CInt -> CInt -> IO ()

-- | R_TRUE: the node is running (live debug) : a soft ring breathes around it.
foreign import ccall safe "Rnode_graph_set_node_active" rnode_graph_set_node_active :: Rid -> CInt -> CInt -> IO ()

-- | Color of that ring (the success color of the theme by default).
foreign import ccall safe "Rnode_graph_set_node_active_color" rnode_graph_set_node_active_color :: Rid -> CInt -> CFloat -> CFloat -> CFloat -> IO ()

-- | R_FALSE: the Delete key does not remove this node (the root of a tree, the output of a graph).
foreign import ccall safe "Rnode_graph_set_node_deletable" rnode_graph_set_node_deletable :: Rid -> CInt -> CInt -> IO ()

-- | Shows a node at once, without the animation a node added to a shown graph pops in with (an application that rebuilds its graph).
foreign import ccall safe "Rnode_graph_set_node_shown" rnode_graph_set_node_shown :: Rid -> CInt -> IO ()

-- | The same for a link made with Rnode_graph_connect: drawn whole at once instead of growing from its output.
foreign import ccall safe "Rnode_graph_set_link_shown" rnode_graph_set_link_shown :: Rid -> CInt -> IO ()

-- | An icon before the title: a texture of the application, shared by many nodes and not deleted with them (the "Icon" style loads one per node from a file). R_INVALID_ID removes it.
foreign import ccall safe "Rnode_graph_set_node_icon" rnode_graph_set_node_icon :: Rid -> CInt -> Rid -> IO ()

-- | A short text in a pill on the top right corner of the node ("1", "!"...); NULL or "" removes it.
foreign import ccall safe "Rnode_graph_set_node_badge" rnode_graph_set_node_badge :: Rid -> CInt -> CString -> IO ()

-- | A short text in a chip of the pin's color outside the node, beside the pin
-- (left of an input, right of an output): the name of what goes out or comes in
-- there, as a net label of a schematic. NULL or "" removes it.
foreign import ccall safe "Rnode_graph_set_pin_tag" rnode_graph_set_pin_tag :: Rid -> CInt -> CString -> IO ()

-- | R_TRUE: the link stays (its pins are linked, the application keeps
-- it) but is not drawn and not picked: its two ends are shown otherwise (pin
-- tags naming it, as the labels of a schematic stand for a wire).
foreign import ccall safe "Rnode_graph_set_link_hidden" rnode_graph_set_link_hidden :: Rid -> CInt -> CInt -> IO ()

-- | How much a pin stands out, as while a wire is pulled: 1 shows it as a pin
-- to go to, -1 steps it back, 0 as usual (an application that links in two clicks).
foreign import ccall safe "Rnode_graph_set_pin_emphasis" rnode_graph_set_pin_emphasis :: Rid -> CInt -> CInt -> IO ()

-- | The pin whose tag (Rnode_graph_set_pin_tag) is under a point of the widget, or 0: the chip only, not the pin.
foreign import ccall safe "Rnode_graph_get_tag_at" rnode_graph_get_tag_at :: Rid -> CFloat -> CFloat -> IO CInt

-- | R_TRUE: the pin's tag is drawn as under the pointer (a stronger fill, a
-- thicker edge) — every tag of one net lit while the pointer is on one of them.
foreign import ccall safe "Rnode_graph_set_pin_tag_lit" rnode_graph_set_pin_tag_lit :: Rid -> CInt -> CInt -> IO ()

-- | Pin labels wider than `width` (logical units) are cut short with an
-- ellipsis and the nodes are laid out with them so (compact ports); 0, the
-- default, shows every label whole.
foreign import ccall safe "Rnode_graph_set_pin_label_width" rnode_graph_set_pin_label_width :: Rid -> CFloat -> IO ()

foreign import ccall safe "Rnode_graph_get_pin_label_width" rnode_graph_get_pin_label_width :: Rid -> IO CFloat

-- | Color of the badge pill of one node (Rnode_graph_set_node_badge); its text turns dark or light to suit it.
foreign import ccall safe "Rnode_graph_set_node_badge_color" rnode_graph_set_node_badge_color :: Rid -> CInt -> CFloat -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rblock_editor_create" rblock_editor_create :: IO Rid

-- | A kind of block, and its block in the palette. Returns its index, -1 on an error.
foreign import ccall safe "Rblock_editor_add_kind" rblock_editor_add_kind :: Rid -> CString -> CString -> RBlockShape -> CString -> IO CInt

-- | Color of the blocks of a category (Raven picks one from its name otherwise).
foreign import ccall safe "Rblock_editor_set_category_color" rblock_editor_set_category_color :: Rid -> CString -> CFloat -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rblock_editor_set_palette_visible" rblock_editor_set_palette_visible :: Rid -> CInt -> IO ()

-- | A new block, the top of a script of its own at x, y (canvas units). Returns it, 0 on an error.
foreign import ccall safe "Rblock_editor_add_block" rblock_editor_add_block :: Rid -> CString -> CFloat -> CFloat -> IO CInt

-- | Removes a block and everything that hangs under it.
foreign import ccall safe "Rblock_editor_remove_block" rblock_editor_remove_block :: Rid -> CInt -> IO ()

foreign import ccall safe "Rblock_editor_clear" rblock_editor_clear :: Rid -> IO ()

-- | Puts a block (with what is under it) at a place of another one. R_FALSE when it does not fit there.
foreign import ccall safe "Rblock_editor_attach" rblock_editor_attach :: Rid -> CInt -> CInt -> RBlockPlace -> CInt -> IO CInt

-- | Takes a block (with what is under it) out: a script of its own at x, y.
foreign import ccall safe "Rblock_editor_detach" rblock_editor_detach :: Rid -> CInt -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rblock_editor_get_script_count" rblock_editor_get_script_count :: Rid -> IO CInt

-- | The top block of a script.
foreign import ccall safe "Rblock_editor_get_script" rblock_editor_get_script :: Rid -> CInt -> IO CInt

foreign import ccall safe "Rblock_editor_get_kind" rblock_editor_get_kind :: Rid -> CInt -> IO CString

-- | The block under it, 0 if none.
foreign import ccall safe "Rblock_editor_get_next" rblock_editor_get_next :: Rid -> CInt -> IO CInt

foreign import ccall safe "Rblock_editor_get_mouth_count" rblock_editor_get_mouth_count :: Rid -> CInt -> IO CInt

-- | The first block in a mouth of a C block, 0 if empty.
foreign import ccall safe "Rblock_editor_get_body" rblock_editor_get_body :: Rid -> CInt -> CInt -> IO CInt

foreign import ccall safe "Rblock_editor_get_slot_count" rblock_editor_get_slot_count :: Rid -> CInt -> IO CInt

-- | The value block in a slot, 0 if the slot holds a typed value.
foreign import ccall safe "Rblock_editor_get_input" rblock_editor_get_input :: Rid -> CInt -> CInt -> IO CInt

-- | The typed value of a slot.
foreign import ccall safe "Rblock_editor_get_value" rblock_editor_get_value :: Rid -> CInt -> CInt -> IO CString

foreign import ccall safe "Rblock_editor_set_value" rblock_editor_set_value :: Rid -> CInt -> CInt -> CString -> IO ()

-- | Where a block hangs: the block it hangs from (0: the top of a script).
foreign import ccall safe "Rblock_editor_get_parent" rblock_editor_get_parent :: Rid -> CInt -> Ptr RBlockPlace -> Ptr CInt -> IO CInt

foreign import ccall safe "Rblock_editor_get_position" rblock_editor_get_position :: Rid -> CInt -> Ptr CFloat -> Ptr CFloat -> IO ()

-- | R_TRUE: the block is running: a halo breathes around it.
foreign import ccall safe "Rblock_editor_set_block_glow" rblock_editor_set_block_glow :: Rid -> CInt -> CInt -> IO ()

-- | Every script as text (valid until the next call).
foreign import ccall safe "Rblock_editor_save" rblock_editor_save :: Rid -> IO CString

-- | Replaces the scripts with those of a text from Rblock_editor_save.
foreign import ccall safe "Rblock_editor_load" rblock_editor_load :: Rid -> CString -> IO CInt

foreign import ccall safe "Rblock_editor_set_changed_callback" rblock_editor_set_changed_callback :: Rid -> RBlockEditorCallback -> Ptr () -> IO ()

foreign import ccall safe "Rblock_editor_set_clicked_callback" rblock_editor_set_clicked_callback :: Rid -> RBlockEditorCallback -> Ptr () -> IO ()

foreign import ccall safe "Rblock_editor_set_context_callback" rblock_editor_set_context_callback :: Rid -> RBlockEditorContextCallback -> Ptr () -> IO ()

-- | The choices of a %m slot of a kind, separated by '\n' ("" : the slot is typed in).
foreign import ccall safe "Rblock_editor_set_slot_options" rblock_editor_set_slot_options :: Rid -> CString -> CInt -> CString -> IO ()

-- | R_FALSE: the kind is not in the palette (its blocks stay where they are).
foreign import ccall safe "Rblock_editor_set_kind_visible" rblock_editor_set_kind_visible :: Rid -> CString -> CInt -> IO ()

-- | A new label for a kind (a function that takes one more input...).
-- The blocks of that kind keep the values of the slots that are still there.
foreign import ccall safe "Rblock_editor_set_kind_label" rblock_editor_set_kind_label :: Rid -> CString -> CString -> IO CInt

-- | R_FALSE: the user cannot delete the block (the entry of a function). Not saved.
foreign import ccall safe "Rblock_editor_set_block_deletable" rblock_editor_set_block_deletable :: Rid -> CInt -> CInt -> IO ()

foreign import ccall safe "Rblock_editor_get_selection_count" rblock_editor_get_selection_count :: Rid -> IO CInt

foreign import ccall safe "Rblock_editor_get_selected" rblock_editor_get_selected :: Rid -> CInt -> IO CInt

-- | Selects a block (add: R_FALSE clears the selection first ; block 0 : only clears).
foreign import ccall safe "Rblock_editor_select" rblock_editor_select :: Rid -> CInt -> CInt -> IO ()

-- | Pan (canvas units at the top left) and zoom.
foreign import ccall safe "Rblock_editor_get_view" rblock_editor_get_view :: Rid -> Ptr CFloat -> Ptr CFloat -> Ptr CFloat -> IO ()

foreign import ccall safe "Rblock_editor_set_view" rblock_editor_set_view :: Rid -> CFloat -> CFloat -> CFloat -> IO ()

-- | Shows a widget as a dialog of a window. The window takes
-- ownership of `content`: it is deleted when the dialog closes, so read
-- what you need from it before, or in `closed`.
-- The Escape key dismisses the dialog (result 0).
foreign import ccall safe "Rdialog_open" rdialog_open :: Rid -> Rid -> RDialogClosedCallback -> Ptr () -> IO ()

-- | Closes the dialog that contains a widget (the content itself or
-- anything inside it: the button that was clicked).
-- result Passed to the closed callback and returned by Rdialog_run.
foreign import ccall safe "Rdialog_close" rdialog_close :: Rid -> CInt -> IO ()

-- | Shows a dialog and waits until it closes. Returns its result.
foreign import ccall safe "Rdialog_run" rdialog_run :: Rid -> Rid -> IO CInt

-- | Shows a widget as a popup over the window, its top-left corner at
-- x, y (window coordinates; moved to stay inside the window): a search box,
-- a palette, a picker. Unlike a dialog it does not block the window: a click
-- outside it, or Escape, closes it. The application keeps the content (it is
-- not deleted on close) and can open it again. `closed` (may be NULL) is
-- called when it closes, with the result given to Rpopup_close (0 when dismissed).
-- The content draws its own background (a Border with a Background...).
foreign import ccall safe "Rpopup_open" rpopup_open :: Rid -> Rid -> CFloat -> CFloat -> RDialogClosedCallback -> Ptr () -> IO ()

-- | Closes the popup that contains a widget (the content or anything inside it).
foreign import ccall safe "Rpopup_close" rpopup_close :: Rid -> CInt -> IO ()

-- | Returns: R_TRUE while the widget is shown as a popup.
foreign import ccall safe "Rpopup_is_open" rpopup_is_open :: Rid -> IO CInt

-- | Returns: R_TRUE while a dialog is shown in the window.
foreign import ccall safe "Rwindow_has_dialog" rwindow_has_dialog :: Rid -> IO CInt

-- | Shows a message with buttons and waits for the answer.
-- window R_INVALID_ID: the first window of the application.
-- title  May be NULL.
foreign import ccall safe "Rmessage_box" rmessage_box :: Rid -> CString -> CString -> RMessageButtons -> IO RMessageResult

-- | Asks for a file to open.
-- desc May be NULL.
-- Returns: The path (UTF-8), valid until the next file dialog, or NULL when the user cancelled.
foreign import ccall safe "Rfile_dialog_open" rfile_dialog_open :: Ptr RFileDialogDesc -> IO CString

-- | Asks where to save a file. An existing file was confirmed by the user.
foreign import ccall safe "Rfile_dialog_save" rfile_dialog_save :: Ptr RFileDialogDesc -> IO CString

-- | Asks for a folder.
foreign import ccall safe "Rfile_dialog_select_folder" rfile_dialog_select_folder :: Ptr RFileDialogDesc -> IO CString

-- | What the last file dialog returned: several paths with R_FILE_DIALOG_MULTIPLE.
foreign import ccall safe "Rfile_dialog_get_result_count" rfile_dialog_get_result_count :: IO CInt

foreign import ccall safe "Rfile_dialog_get_result" rfile_dialog_get_result :: CInt -> IO CString

-- | Registers a file held in memory under a path: every file Raven
-- reads (XML documents and their Include, Source and Font attributes,
-- themes, textures, fonts, Rfile_read) is looked up here first, so that
-- resources embedded in the executable need no file beside it. A relative
-- path names the file as it is written in the XML once joined to the
-- folder of the document that names it (Rxml_set_base_path for a
-- document given as text), e.g. "ui/editor.xml". Paths compare with '/'
-- and '\\' alike. Memory files are not watched for live reload. Call it
-- after Rinit; Rshutdown forgets every file.
-- copy R_TRUE: Raven keeps a copy; R_FALSE: `data` must outlive Raven (static data).
foreign import ccall safe "Rfile_register_memory" rfile_register_memory :: CString -> Ptr () -> CSize -> CInt -> IO CInt

foreign import ccall safe "Rfile_unregister_memory" rfile_unregister_memory :: CString -> IO ()

-- | R_TRUE when `path` is a file registered with Rfile_register_memory.
foreign import ccall safe "Rfile_is_memory" rfile_is_memory :: CString -> IO CInt

-- | Whole files, with UTF-8 paths on every platform.
-- Returns: Rfile_read: the content followed by a '\0', to release with
-- Rmemory_free; NULL when the file cannot be read. `size` may be NULL.
foreign import ccall safe "Rfile_read" rfile_read :: CString -> Ptr CSize -> IO CString

foreign import ccall safe "Rfile_write" rfile_write :: CString -> Ptr () -> CSize -> IO CInt

foreign import ccall safe "Rfile_browser_create" rfile_browser_create :: IO Rid

foreign import ccall safe "Rfile_browser_set_mode" rfile_browser_set_mode :: Rid -> RFileBrowserMode -> IO ()

-- | Shows a folder. "~" is the home of the user. Returns R_FALSE when it cannot be read.
foreign import ccall safe "Rfile_browser_set_directory" rfile_browser_set_directory :: Rid -> CString -> IO CInt

foreign import ccall safe "Rfile_browser_get_directory" rfile_browser_get_directory :: Rid -> IO CString

-- | Reads the folder again.
foreign import ccall safe "Rfile_browser_refresh" rfile_browser_refresh :: Rid -> IO ()

-- | Kinds of files the user chooses from; the first one is active. The strings are copied.
foreign import ccall safe "Rfile_browser_set_filters" rfile_browser_set_filters :: Rid -> Ptr RFileFilter -> CInt -> IO ()

foreign import ccall safe "Rfile_browser_set_show_hidden" rfile_browser_set_show_hidden :: Rid -> CInt -> IO ()

-- | R_TRUE lets several files be selected (Ctrl and Shift with a click).
foreign import ccall safe "Rfile_browser_set_multiple" rfile_browser_set_multiple :: Rid -> CInt -> IO ()

-- | R_FALSE hides the name field, the filter list and the buttons:
-- what remains is an explorer to put in a side panel. Shown by default.
foreign import ccall safe "Rfile_browser_set_controls_visible" rfile_browser_set_controls_visible :: Rid -> CInt -> IO ()

-- | R_FALSE hides the list of places on the left.
foreign import ccall safe "Rfile_browser_set_places_visible" rfile_browser_set_places_visible :: Rid -> CInt -> IO ()

-- | Name typed in the name field (save mode).
foreign import ccall safe "Rfile_browser_set_file_name" rfile_browser_set_file_name :: Rid -> CString -> IO ()

-- | Added to a name typed without extension, in save mode.
foreign import ccall safe "Rfile_browser_set_default_extension" rfile_browser_set_default_extension :: Rid -> CString -> IO ()

-- | Paths of the selected entries. The strings are valid until the selection or the folder changes.
foreign import ccall safe "Rfile_browser_get_selection_count" rfile_browser_get_selection_count :: Rid -> IO CInt

foreign import ccall safe "Rfile_browser_get_selection" rfile_browser_get_selection :: Rid -> CInt -> IO CString

-- | Selects an entry of the folder by its name.
foreign import ccall safe "Rfile_browser_select" rfile_browser_select :: Rid -> CString -> IO ()

foreign import ccall safe "Rfile_browser_set_accepted_callback" rfile_browser_set_accepted_callback :: Rid -> RFileBrowserCallback -> Ptr () -> IO ()

-- | Called by the Cancel button.
foreign import ccall safe "Rfile_browser_set_cancelled_callback" rfile_browser_set_cancelled_callback :: Rid -> RFileBrowserCallback -> Ptr () -> IO ()

-- | Called when the selection changes; `path` is the entry under the cursor, or NULL.
foreign import ccall safe "Rfile_browser_set_selection_changed_callback" rfile_browser_set_selection_changed_callback :: Rid -> RFileBrowserCallback -> Ptr () -> IO ()

-- | Returns: R_TRUE when the system took the notification.
foreign import ccall safe "Rnotification_show" rnotification_show :: CString -> CString -> RNotificationUrgency -> IO CInt

-- | R_FALSE when the platform has no notifications (Rnotification_show then always uses the fallback).
foreign import ccall safe "Rnotification_is_supported" rnotification_is_supported :: IO CInt

-- | Called with the notifications the system could not show; NULL: none.
foreign import ccall safe "Rnotification_set_fallback" rnotification_set_fallback :: RNotificationFallback -> Ptr () -> IO ()

-- | Called when the user clicks a notification of Rnotification_show
-- (on the UI thread; the application may then bring its window to the
-- front: the system lets it). It is the last one shown that was clicked:
-- the application remembers what it was about. NULL: none.
-- Windows only for now (elsewhere it is never called).
foreign import ccall safe "Rnotification_set_clicked_callback" rnotification_set_clicked_callback :: RNotificationClickedCallback -> Ptr () -> IO ()

-- | Shows a texture. R_INVALID_ID shows nothing.
foreign import ccall safe "Rviewport_set_texture" rviewport_set_texture :: Rid -> Rid -> IO ()

-- | How the texture is fitted in the widget. Default: R_STRETCH_FILL.
foreign import ccall safe "Rviewport_set_stretch" rviewport_set_stretch :: Rid -> RStretch -> IO ()

-- | R_TRUE flips the texture vertically (OpenGL framebuffers are bottom-up).
foreign import ccall safe "Rviewport_set_flip_y" rviewport_set_flip_y :: Rid -> CInt -> IO ()

foreign import ccall safe "Rviewport_set_draw_callback" rviewport_set_draw_callback :: Rid -> RViewportDrawCallback -> Ptr () -> IO ()

-- | Creates (or destroys) the native child window of the viewport.
-- The viewport must already be in a window.
-- Returns: R_TRUE on success.
foreign import ccall safe "Rviewport_set_native" rviewport_set_native :: Rid -> CInt -> IO CInt

-- | Fills `native` with the handles of the child window.
foreign import ccall safe "Rviewport_get_native_handle" rviewport_get_native_handle :: Rid -> Ptr RNativeWindow -> IO CInt

-- | Size of the viewport in pixels after the last layout.
foreign import ccall safe "Rviewport_get_pixel_size" rviewport_get_pixel_size :: Rid -> Ptr CInt -> Ptr CInt -> IO ()

foreign import ccall safe "Rviewport_set_resize_callback" rviewport_set_resize_callback :: Rid -> RViewportResizeCallback -> Ptr () -> IO ()

-- | R_TRUE redraws the window every frame while the viewport is
-- visible (game view). R_FALSE redraws only on Rwidget_invalidate (default).
foreign import ccall safe "Rviewport_set_realtime" rviewport_set_realtime :: Rid -> CInt -> IO ()

foreign import ccall safe "Rcustom_set_paint_callback" rcustom_set_paint_callback :: Rid -> RPaintCallback -> Ptr () -> IO ()

-- | Reports the desired size of the widget. Without it the widget asks for 0 x 0.
foreign import ccall safe "Rcustom_set_measure_callback" rcustom_set_measure_callback :: Rid -> RMeasureCallback -> Ptr () -> IO ()

-- | Filled rectangle with rounded corners (radius 0 = sharp).
foreign import ccall safe "Rdraw_rect" rdraw_rect :: Ptr RPainter -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

-- | Outline of a rounded rectangle, drawn inside its bounds.
foreign import ccall safe "Rdraw_rect_outline" rdraw_rect_outline :: Ptr RPainter -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rdraw_circle" rdraw_circle :: Ptr RPainter -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

-- | Anti-aliased line with round caps.
foreign import ccall safe "Rdraw_line" rdraw_line :: Ptr RPainter -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

-- | Anti-aliased cubic Bezier curve from (x0, y0) to (x1, y1), pulled by two control points.
foreign import ccall safe "Rdraw_curve" rdraw_curve :: Ptr RPainter -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rdraw_triangle" rdraw_triangle :: Ptr RPainter -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

-- | Texture stretched to a rectangle and multiplied by a tint color.
foreign import ccall safe "Rdraw_image" rdraw_image :: Ptr RPainter -> Rid -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

-- | Draws UTF-8 text; (x, y) is the top-left corner of the first line.
-- font R_INVALID_ID uses the theme font.
-- size 0 uses R_METRIC_FONT_SIZE.
foreign import ccall safe "Rdraw_text" rdraw_text :: Ptr RPainter -> Rid -> CFloat -> CFloat -> CFloat -> CString -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

-- | Rounded rectangle of glass. It shows what was drawn before it,
-- frosted and bent along its rim, under a tint: (r, g, b) is the color of
-- the tint and `a` its strength (0 = clear glass, 1 = opaque color).
-- Works with any theme. Needs a backend that implements glass (the Vulkan
-- backend does).
foreign import ccall safe "Rdraw_glass" rdraw_glass :: Ptr RPainter -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

-- | Animates a value from the paint callback of a custom widget.
-- Moves the spring toward `target` by the time elapsed since the last
-- frame and returns its value. While it moves, the window keeps drawing
-- frames by itself. The first call puts the spring on its target.
--
--     static RSpring size;                                  (zeroed)
--     float s = Rdraw_spring(painter, &size, hovered ? 1.0f : 0.0f, R_MOTION_BOUNCY);
--     Rdraw_circle(painter, cx, cy, 20.0f + 8.0f * s, ...);
foreign import ccall safe "Rdraw_spring" rdraw_spring :: Ptr RPainter -> Ptr RSpring -> CFloat -> RMotion -> IO CFloat

-- | Pushes a spring away from its target (units per second): it bounces back.
foreign import ccall safe "Rspring_kick" rspring_kick :: Ptr RSpring -> CFloat -> IO ()

-- | Puts a spring at rest on a value, without animation.
foreign import ccall safe "Rspring_set" rspring_set :: Ptr RSpring -> CFloat -> IO ()

-- | Restricts the following draws to a rectangle, intersected with the current one.
foreign import ccall safe "Rdraw_push_clip_rect" rdraw_push_clip_rect :: Ptr RPainter -> CFloat -> CFloat -> CFloat -> CFloat -> IO ()

foreign import ccall safe "Rdraw_pop_clip_rect" rdraw_pop_clip_rect :: Ptr RPainter -> IO ()

-- | A shader from SPIR-V code (a fragment shader written with
-- raven_effect.glsl, as in a .spv file): `size` bytes, copied. R_INVALID_ID
-- when the code is not SPIR-V; a backend that refuses it reports an error
-- and the effect is drawn as a rectangle.
foreign import ccall safe "Rshader_create" rshader_create :: Ptr () -> CSize -> Word32 -> IO Rid

-- | The same, from a .spv file (UTF-8 path, relative to the current folder).
foreign import ccall safe "Rshader_create_from_file" rshader_create_from_file :: CString -> Word32 -> IO Rid

foreign import ccall safe "Rshader_destroy" rshader_destroy :: Rid -> IO ()

foreign import ccall safe "Rshader_is_valid" rshader_is_valid :: Rid -> IO CInt

foreign import ccall safe "Rshader_get_flags" rshader_get_flags :: Rid -> IO Word32

-- | A built-in effect, created the first time it is asked for.
foreign import ccall safe "Reffect_get" reffect_get :: REffect -> IO Rid

-- | A built-in effect by its name ("aurora", "shimmer", "neon", "ripple", "plasma", "grain", "spotlight", "mesh"), or R_INVALID_ID.
foreign import ccall safe "Reffect_find" reffect_find :: CString -> IO Rid

foreign import ccall safe "Reffect_get_name" reffect_get_name :: REffect -> IO CString

-- | Its colors and parameters when nothing else is given (Rshader_look_init uses them).
foreign import ccall safe "Reffect_get_defaults" reffect_get_defaults :: REffect -> Ptr RShaderLook -> IO ()

-- | White to white, no parameter, no texture, square corners, animated when the shader is.
foreign import ccall safe "Rshader_look_init" rshader_look_init :: Ptr RShaderLook -> IO ()

-- | Fills a look with the defaults of the shader when it is a built-in effect (else Rshader_look_init).
foreign import ccall safe "Rshader_look_defaults" rshader_look_defaults :: Rid -> Ptr RShaderLook -> IO ()

-- | From a paint callback: draws the effect over a rectangle of the
-- widget (logical units, from its top left corner).
foreign import ccall safe "Rdraw_shader" rdraw_shader :: Ptr RPainter -> Rid -> CFloat -> CFloat -> CFloat -> CFloat -> Ptr RShaderLook -> IO ()

-- | The background of a border becomes an effect (its Background,
-- if any, is drawn under it; its child over it). R_INVALID_ID removes it.
-- XML: Effect="aurora" or Effect="file.spv", EffectColor, EffectColor2,
-- EffectParams="a,b,c,d", EffectAnimate, EffectBackdrop (for a file).
foreign import ccall safe "Rborder_set_effect" rborder_set_effect :: Rid -> Rid -> Ptr RShaderLook -> IO ()

foreign import ccall safe "Rborder_get_effect" rborder_get_effect :: Rid -> IO Rid

-- | Creates a window from a document whose root element is <Window>.
-- xml  UTF-8 text. It does not need to be null-terminated.
-- size Size of the text in bytes.
-- Returns: The window, or R_INVALID_ID. Parse errors are reported with
--         their line and column through the error callback (R_ERROR_XML).
foreign import ccall safe "Rwindow_create_from_xml" rwindow_create_from_xml :: CString -> CSize -> IO Rid

foreign import ccall safe "Rwindow_create_from_xml_file" rwindow_create_from_xml_file :: CString -> IO Rid

-- | Creates a widget tree from a document whose root element is a
-- widget. The caller owns the returned widget until it is added to a parent.
foreign import ccall safe "Rwidget_create_from_xml" rwidget_create_from_xml :: CString -> CSize -> IO Rid

foreign import ccall safe "Rwidget_create_from_xml_file" rwidget_create_from_xml_file :: CString -> IO Rid

-- | A folder looked into after the one of the document (see above),
-- and the first one for the documents given from memory. NULL or "": none.
foreign import ccall safe "Rxml_set_base_path" rxml_set_base_path :: CString -> IO ()

-- | Turns auto-loading on or off (see above). It is on by default;
-- the environment variable RAVEN_AUTO_LOAD=0 turns it off without
-- rebuilding. Turned off, the register functions are the only way in;
-- the places where paths are looked for stay the same.
foreign import ccall safe "Rxml_set_auto_load" rxml_set_auto_load :: CInt -> IO ()

foreign import ccall safe "Rxml_get_auto_load" rxml_get_auto_load :: IO CInt

-- | Turns live reload on or off. It is on by default; the
-- environment variable RAVEN_LIVE_RELOAD=0 turns it off without
-- rebuilding. When off, files are never looked at again.
foreign import ccall safe "Rxml_set_live_reload" rxml_set_live_reload :: CInt -> IO ()

foreign import ccall safe "Rxml_get_live_reload" rxml_get_live_reload :: IO CInt

foreign import ccall safe "Rxml_set_reload_callback" rxml_set_reload_callback :: RXmlReloadCallback -> Ptr () -> IO ()

-- | Looks at the files now, without waiting for the next look
-- (there are five per second), even when live reload is off.
-- Returns: The number of files found changed.
foreign import ccall safe "Rxml_check_files" rxml_check_files :: IO CInt

-- | Reads the <Widget Name="..."> declarations of a document (its
-- root, or the children of a <Widgets> root) so that the tags they
-- declare can be used in the documents loaded afterwards.
-- Returns: The number of tags declared.
foreign import ccall safe "Rxml_register_widgets" rxml_register_widgets :: CString -> CSize -> IO CInt

foreign import ccall safe "Rxml_register_widgets_from_file" rxml_register_widgets_from_file :: CString -> IO CInt

foreign import ccall safe "Rxml_unregister_widget" rxml_unregister_widget :: CString -> IO ()

foreign import ccall safe "Rxml_is_widget_registered" rxml_is_widget_registered :: CString -> IO CInt

-- | The declared tags, for tools that list them.
foreign import ccall safe "Rxml_get_registered_widget_count" rxml_get_registered_widget_count :: IO CInt

foreign import ccall safe "Rxml_get_registered_widget_name" rxml_get_registered_widget_name :: CInt -> IO CString

-- | Registers a tag (or replaces what it was registered with).
-- Returns: R_FALSE when the name is the one of a widget of Raven, or when too many tags are registered (64).
foreign import ccall safe "Rxml_register_tag" rxml_register_tag :: CString -> RXmlTagCreateCallback -> RXmlTagAttributeCallback -> Ptr () -> IO CInt

foreign import ccall safe "Rxml_unregister_tag" rxml_unregister_tag :: CString -> IO ()

foreign import ccall safe "Rxml_is_tag_registered" rxml_is_tag_registered :: CString -> IO CInt

-- | The path of a file named in a document: as the attributes
-- Source of Raven read it, looked for where files are looked for (see
-- Rxml_set_base_path): the folder of the document being loaded (while it
-- loads) and its subfolders, the base path, the current folder, PATH.
-- When no place holds it, the path it would have next to the document
-- (else in the base path). For the attributes of the tags made by code
-- that name files.
-- Returns: The path; valid until the next call.
foreign import ccall safe "Rxml_resolve_path" rxml_resolve_path :: CString -> IO CString

-- | Sets a property from text, with the names and value syntax of the
-- XML attributes. Handy for scripting and data-driven tools.
-- Returns: R_TRUE when the attribute exists for this widget and the value is valid.
foreign import ccall safe "Rwidget_set_attribute" rwidget_set_attribute :: Rid -> CString -> CString -> IO CInt

-- | A new document with one element.
foreign import ccall safe "Rxml_document_create" rxml_document_create :: CString -> IO Rid

-- | Reads a text into a document. Comments are kept.
-- error When not NULL, a mistake in the text is described there
-- and nothing is reported; when NULL it goes to the error callback.
-- Returns: The document, or R_INVALID_ID.
foreign import ccall safe "Rxml_document_parse" rxml_document_parse :: CString -> CSize -> Ptr RXmlError -> IO Rid

-- | Reads a file. The paths written in it are relative to its folder.
foreign import ccall safe "Rxml_document_load" rxml_document_load :: CString -> IO Rid

-- | Releases a document and its nodes. What was built from it lives on.
foreign import ccall safe "Rxml_document_destroy" rxml_document_destroy :: Rid -> IO ()

foreign import ccall safe "Rxml_document_is_valid" rxml_document_is_valid :: Rid -> IO CInt

foreign import ccall safe "Rxml_document_get_root" rxml_document_get_root :: Rid -> IO Rid

-- | Writes the document as text, one element per line, indented
-- with tabs. Rxml_node_get_line then says where each node was written.
-- Returns: The text, valid until the next call for this document or its destruction.
foreign import ccall safe "Rxml_document_to_string" rxml_document_to_string :: Rid -> IO CString

foreign import ccall safe "Rxml_document_save" rxml_document_save :: Rid -> CString -> IO CInt

-- | Builds the widgets the document describes, as
-- Rwidget_create_from_xml does. When the root is a <Window>, builds what
-- the window would show (R_INVALID_ID when it is empty).
foreign import ccall safe "Rxml_document_create_widget" rxml_document_create_widget :: Rid -> IO Rid

-- | Builds the window the document describes, as Rwindow_create_from_xml does.
foreign import ccall safe "Rxml_document_create_window" rxml_document_create_window :: Rid -> IO Rid

foreign import ccall safe "Rxml_node_is_valid" rxml_node_is_valid :: Rid -> IO CInt

foreign import ccall safe "Rxml_node_get_document" rxml_node_get_document :: Rid -> IO Rid

-- | Returns: The tag of an element, "" for a comment.
foreign import ccall safe "Rxml_node_get_tag" rxml_node_get_tag :: Rid -> IO CString

foreign import ccall safe "Rxml_node_set_tag" rxml_node_set_tag :: Rid -> CString -> IO ()

foreign import ccall safe "Rxml_node_is_comment" rxml_node_is_comment :: Rid -> IO CInt

-- | The text inside an element (<Label>this</Label>), or the text of a comment.
foreign import ccall safe "Rxml_node_get_text" rxml_node_get_text :: Rid -> IO CString

foreign import ccall safe "Rxml_node_set_text" rxml_node_set_text :: Rid -> CString -> IO ()

foreign import ccall safe "Rxml_node_get_parent" rxml_node_get_parent :: Rid -> IO Rid

foreign import ccall safe "Rxml_node_get_child_count" rxml_node_get_child_count :: Rid -> IO CInt

foreign import ccall safe "Rxml_node_get_child" rxml_node_get_child :: Rid -> CInt -> IO Rid

-- | Returns: The place of a node among the children of its parent; 0 for the root.
foreign import ccall safe "Rxml_node_get_index" rxml_node_get_index :: Rid -> IO CInt

-- | index Where among the children; -1 for the end.
-- Returns: The new node; R_INVALID_ID (R_ERROR_XML) when it would be nested
--         deeper than a document can be: 256 levels under the root, as
--         when a text is read. The same holds for the functions below.
foreign import ccall safe "Rxml_node_add_child" rxml_node_add_child :: Rid -> CString -> CInt -> IO Rid

foreign import ccall safe "Rxml_node_add_comment" rxml_node_add_comment :: Rid -> CString -> CInt -> IO Rid

-- | Reads a piece of XML (one element and what it contains) and
-- adds it: what a palette, a paste or a template inserts.
foreign import ccall safe "Rxml_node_add_xml" rxml_node_add_xml :: Rid -> CString -> CSize -> CInt -> IO Rid

-- | Removes a node and what it contains. The root stays.
foreign import ccall safe "Rxml_node_remove" rxml_node_remove :: Rid -> IO ()

-- | Moves a node, with what it contains, under another parent of
-- the same document or elsewhere under its own.
-- index Where among the children of the new parent, counted once the node has left its place; -1 for the end.
-- Returns: R_FALSE when it cannot be done (the root, a parent that is inside the node, or a place too deep).
foreign import ccall safe "Rxml_node_move" rxml_node_move :: Rid -> Rid -> CInt -> IO CInt

-- | Copies a node and what it contains, right after it. Returns the copy.
foreign import ccall safe "Rxml_node_duplicate" rxml_node_duplicate :: Rid -> IO Rid

-- | The node as text. Valid until the next call for this document.
foreign import ccall safe "Rxml_node_to_string" rxml_node_to_string :: Rid -> IO CString

foreign import ccall safe "Rxml_node_get_attribute_count" rxml_node_get_attribute_count :: Rid -> IO CInt

foreign import ccall safe "Rxml_node_get_attribute_name" rxml_node_get_attribute_name :: Rid -> CInt -> IO CString

foreign import ccall safe "Rxml_node_get_attribute_value" rxml_node_get_attribute_value :: Rid -> CInt -> IO CString

-- | Returns: The value of an attribute (names are compared without case), or NULL when the node does not have it.
foreign import ccall safe "Rxml_node_get_attribute" rxml_node_get_attribute :: Rid -> CString -> IO CString

-- | Sets an attribute, added after the others when it is new. A NULL value removes it.
foreign import ccall safe "Rxml_node_set_attribute" rxml_node_set_attribute :: Rid -> CString -> CString -> IO ()

-- | Returns: The line of the node, starting at 1, in the text the document
-- was read from or last written to; 0 for a node added since.
foreign import ccall safe "Rxml_node_get_line" rxml_node_get_line :: Rid -> IO CInt

-- | Returns: The widget (the window, for a <Window>) the node became the
-- last time the document was built, or R_INVALID_ID: never built,
-- destroyed since, or not a widget (an <Item>, a comment). A tag declared
-- with <Widget> gives the widget it stands for.
foreign import ccall safe "Rxml_node_get_widget" rxml_node_get_widget :: Rid -> IO Rid

-- | Returns: The node a widget was built from, or R_INVALID_ID: not built
-- from a document, or the document is gone. The widgets inside a tag
-- declared with <Widget> all come from the node where the tag is used.
foreign import ccall safe "Rwidget_get_xml_node" rwidget_get_xml_node :: Rid -> IO Rid

-- | The tags of the library, <Window> included. Those declared with <Widget> are listed by Rxml_get_registered_widget_name.
foreign import ccall safe "Rxml_get_tag_count" rxml_get_tag_count :: IO CInt

foreign import ccall safe "Rxml_get_tag_name" rxml_get_tag_name :: CInt -> IO CString

-- | Returns: One line about a tag, "" when there is nothing to say.
foreign import ccall safe "Rxml_get_tag_description" rxml_get_tag_description :: CString -> IO CString

-- | Returns: 0: no child widget; 1: a single one; 2: any number. Declared and unknown tags give 2.
foreign import ccall safe "Rxml_get_tag_children" rxml_get_tag_children :: CString -> IO CInt

-- | The attributes of a tag: its own first, then those every widget
-- has, then those of its place in its parent. For a tag declared with
-- <Widget>, its own are its parameters.
foreign import ccall safe "Rxml_get_attribute_count" rxml_get_attribute_count :: CString -> IO CInt

foreign import ccall safe "Rxml_get_attribute_info" rxml_get_attribute_info :: CString -> CInt -> Ptr RXmlAttributeInfo -> IO CInt

-- | Loads a CSV table of translations from a file. Returns the number of keys read, -1 on an error.
foreign import ccall safe "Rlocale_load_csv" rlocale_load_csv :: CString -> IO CInt

-- | Same, from text in memory (UTF-8).
foreign import ccall safe "Rlocale_load_csv_from_memory" rlocale_load_csv_from_memory :: CString -> CSize -> IO CInt

-- | Forgets every table.
foreign import ccall safe "Rlocale_clear" rlocale_clear :: IO ()

-- | Adds or replaces one text of one language.
foreign import ccall safe "Rlocale_set_text" rlocale_set_text :: CString -> CString -> CString -> IO ()

-- | Number of languages in the tables, and their codes, in the order of the columns.
foreign import ccall safe "Rlocale_get_language_count" rlocale_get_language_count :: IO CInt

foreign import ccall safe "Rlocale_get_language_at" rlocale_get_language_at :: CInt -> IO CString

-- | Chooses the language: an exact code ("pt-BR"), or its first part
-- ("pt") when the tables have no such column. Every widget that shows a key
-- is updated, and the layout of the windows follows its direction (an
-- Arabic, Hebrew, Persian or Urdu language is laid out from right to left,
-- see Rset_layout_direction). Returns R_FALSE when no column matches (the
-- language is set all the same: the texts fall back).
foreign import ccall safe "Rlocale_set_language" rlocale_set_language :: CString -> IO CInt

foreign import ccall safe "Rlocale_get_language" rlocale_get_language :: IO CString

-- | The language of the user, from the system ("fr-FR", "en-US"...).
foreign import ccall safe "Rlocale_get_system_language" rlocale_get_system_language :: IO CString

-- | The language whose texts are shown when the current one has none (default: the first column).
foreign import ccall safe "Rlocale_set_fallback_language" rlocale_set_fallback_language :: CString -> IO ()

-- | R_TRUE when the language is written from right to left.
foreign import ccall safe "Rlocale_is_rtl" rlocale_is_rtl :: IO CInt

-- | Called after the language or the tables changed (the widgets are already updated).
foreign import ccall safe "Rlocale_set_changed_callback" rlocale_set_changed_callback :: RLocaleChangedCallback -> Ptr () -> IO ()

-- | The text of a key in the current language (see the fallbacks above). Never NULL.
foreign import ccall safe "Rtr" rtr :: CString -> IO CString

-- | R_TRUE when the key has a text in the current language or the fallback one.
foreign import ccall safe "Rtr_exists" rtr_exists :: CString -> IO CInt

-- | The text of a key with its {0} {1} ... replaced by the arguments
-- (translators may put them in any order). "{{" is a brace.
foreign import ccall safe "Rtr_format" rtr_format :: CString -> Ptr CString -> CInt -> IO CString

-- | The plural form of a key for a count: the key followed by the CLDR
-- category of the count in the current language (".zero", ".one", ".two",
-- ".few", ".many", ".other"), else ".other", else the key itself. {n} is
-- replaced by the count, written in the conventions of the language.
foreign import ccall safe "Rtr_plural" rtr_plural :: CString -> CInt -> IO CString

-- | Makes a widget show a key: an attribute ("Text", "Tooltip",
-- "Title", "Placeholder"... any attribute that takes a text) is set to the
-- translation now and every time the language changes. NULL or "" stops it.
-- Same as Rwidget_set_attribute(widget, attribute, "@key").
foreign import ccall safe "Rwidget_set_text_key" rwidget_set_text_key :: Rid -> CString -> CString -> IO ()

-- | A number written in the conventions of the language: decimal separator and digit grouping ("1 234,5" in French).
foreign import ccall safe "Rlocale_format_number" rlocale_format_number :: CDouble -> CInt -> IO CString

-- | Size of every text, 1 = as designed (0.5 to 4). Controls grow with their text.
foreign import ccall safe "Raccessibility_set_text_scale" raccessibility_set_text_scale :: CFloat -> IO ()

foreign import ccall safe "Raccessibility_get_text_scale" raccessibility_get_text_scale :: IO CFloat

-- | Themes of high contrast: black and white, yellow focus, thick
-- borders (the dark one, or the light one when the theme in use is light).
-- R_FALSE goes back to the theme used before.
foreign import ccall safe "Raccessibility_set_high_contrast" raccessibility_set_high_contrast :: CInt -> IO ()

foreign import ccall safe "Raccessibility_get_high_contrast" raccessibility_get_high_contrast :: IO CInt

-- | R_TRUE: the focus ring is always shown (not only after the keyboard was used), and thicker.
foreign import ccall safe "Raccessibility_set_focus_always_visible" raccessibility_set_focus_always_visible :: CInt -> IO ()

foreign import ccall safe "Raccessibility_get_focus_always_visible" raccessibility_get_focus_always_visible :: IO CInt

-- | Reduced motion, for the whole application: R_TRUE and nothing
-- moves by itself. Changes are immediate (no spring, no slide, no bounce,
-- no transition), and the effects (raven_shader.h) stand still; they still
-- follow the pointer. Same as Rset_animations_enabled(R_FALSE).
foreign import ccall safe "Raccessibility_set_reduce_motion" raccessibility_set_reduce_motion :: CInt -> IO ()

foreign import ccall safe "Raccessibility_get_reduce_motion" raccessibility_get_reduce_motion :: IO CInt

-- | A filter on the colors of the interface (pictures and viewports keep theirs).
foreign import ccall safe "Raccessibility_set_color_filter" raccessibility_set_color_filter :: RColorFilter -> IO ()

foreign import ccall safe "Raccessibility_get_color_filter" raccessibility_get_color_filter :: IO RColorFilter

foreign import ccall safe "Raccessibility_get_system_settings" raccessibility_get_system_settings :: Ptr RAccessibilitySettings -> IO ()

-- | Applies the settings of the system (text size, contrast, motion).
foreign import ccall safe "Raccessibility_follow_system" raccessibility_follow_system :: IO ()

-- | What the screen reader says of the widget; NULL: what it shows (or the label before it).
foreign import ccall safe "Rwidget_set_accessible_name" rwidget_set_accessible_name :: Rid -> CString -> IO ()

-- | The name the screen reader uses (given, or found). Valid until the next call.
foreign import ccall safe "Rwidget_get_accessible_name" rwidget_get_accessible_name :: Rid -> IO CString

-- | A longer explanation, read after the name.
foreign import ccall safe "Rwidget_set_accessible_description" rwidget_set_accessible_description :: Rid -> CString -> IO ()

foreign import ccall safe "Rwidget_set_accessible_role" rwidget_set_accessible_role :: Rid -> RAccessibleRole -> IO ()

-- | The role the screen reader sees (given, or the one of the type).
foreign import ccall safe "Rwidget_get_accessible_role" rwidget_get_accessible_role :: Rid -> IO RAccessibleRole

-- | R_TRUE: hidden from screen readers, with what it contains (decoration).
foreign import ccall safe "Rwidget_set_accessible_hidden" rwidget_set_accessible_hidden :: Rid -> CInt -> IO ()

-- | The widget is named by another one (a label): its text is the name.
foreign import ccall safe "Rwidget_set_accessible_labelled_by" rwidget_set_accessible_labelled_by :: Rid -> Rid -> IO ()

-- | Explicit screen-reader press action, including for custom widgets.
-- NULL removes the override. Does not change pointer or keyboard handling.
-- Disabled widgets (including disabled ancestors) cannot be pressed.
foreign import ccall safe "Rwidget_set_accessible_action" rwidget_set_accessible_action :: Rid -> RAccessiblePressCallback -> Ptr () -> IO ()

-- | Override the checked state: 1 checked, 0 unchecked, -1 restores the
-- widget's default (not checkable for a custom widget). The application owns
-- this state; a press callback updates it when appropriate.
foreign import ccall safe "Rwidget_set_accessible_checked" rwidget_set_accessible_checked :: Rid -> CInt -> IO ()

-- | Has a message read (a save that ended, an error). `assertive`: interrupts what is being read.
foreign import ccall safe "Raccessibility_announce" raccessibility_announce :: CString -> CInt -> IO ()

-- | R_TRUE when a screen reader is reading the windows of the application.
foreign import ccall safe "Raccessibility_is_screen_reader_active" raccessibility_is_screen_reader_active :: IO CInt

-- | Makes the next Rinit use the host platform. The structure is copied; NULL restores the default.
foreign import ccall safe "Rhost_platform_register" rhost_platform_register :: Ptr RHostPlatformDesc -> IO ()

-- | Returns: R_TRUE while the running library uses the host platform.
foreign import ccall safe "Rhost_platform_is_active" rhost_platform_is_active :: IO CInt

-- | Size of a host window in pixels, and pixels per logical unit.
foreign import ccall safe "Rhost_window_set_size" rhost_window_set_size :: Rid -> CInt -> CInt -> CFloat -> IO ()

foreign import ccall safe "Rhost_inject_mouse_move" rhost_inject_mouse_move :: Rid -> CInt -> CInt -> Word32 -> IO ()

foreign import ccall safe "Rhost_inject_mouse_button" rhost_inject_mouse_button :: Rid -> CInt -> CInt -> RMouseButton -> CInt -> Word32 -> IO ()

foreign import ccall safe "Rhost_inject_mouse_wheel" rhost_inject_mouse_wheel :: Rid -> CFloat -> CFloat -> Word32 -> IO ()

-- | The pointer left the window: hover states end.
foreign import ccall safe "Rhost_inject_mouse_leave" rhost_inject_mouse_leave :: Rid -> IO ()

-- | repeat R_TRUE for a key held down (auto-repeat).
foreign import ccall safe "Rhost_inject_key" rhost_inject_key :: Rid -> RKey -> CInt -> Word32 -> CInt -> IO ()

-- | One typed Unicode code point.
foreign import ccall safe "Rhost_inject_text" rhost_inject_text :: Rid -> Word32 -> IO ()

-- | The window gained or lost the keyboard. Losing it closes popups and drops captures.
foreign import ccall safe "Rhost_inject_focus" rhost_inject_focus :: Rid -> CInt -> IO ()

-- | The user asked to close the window (its close callback decides).
foreign import ccall safe "Rhost_inject_close" rhost_inject_close :: Rid -> IO ()

// Raven for Java.
//
// Generated from the C headers by tools/bindgen (Raven 0.1): do not edit.
//
// The C API under its own names, in the class raven.R, over the Foreign
// Function & Memory API of Java 22 and later (java.lang.foreign): no
// dependency, no JNI, no code to compile in C.
//
//   - texts are String on both sides (UTF-8 underneath);
//   - enumerations and flags are int constants: R.R_WIDGET_BUTTON;
//   - a parameter the C function writes through (int*, float*, const char**)
//     is an array of one element, or null when C accepts NULL:
//     int[] w = {0}, h = {0}; Rwindow_get_size(window, w, h);
//   - the size that follows a text or bytes (xml, size) is not passed, nor
//     the count that follows a list of texts;
//   - a structure is a class over its memory, with an accessor per field:
//     new RInitDesc().backend(R_BACKEND_NULL).headless(1);
//   - a callback is a functional interface: a lambda passes. It stays alive
//     as long as the program (the library keeps its address);
//   - `user_data` can be left out (null in C); void* and the other addresses
//     are MemorySegment.
//
//   import static raven.R.*;
//
//   public class Hello {
//       public static void main(String[] args) {
//           Rinit(R_BACKEND_VULKAN);
//           Rwindow_create_from_xml("<Window Title='Hi'><Button Text='Quit' OnClicked='quit'/></Window>");
//           Raction_bind("quit", (sender, action, user) -> Rquit(0));
//           while (Rwait_frame(-1.0) != 0) { }
//           Rshutdown();
//       }
//   }
//
//   java --enable-native-access=ALL-UNNAMED -cp raven-gui.jar Hello.java
//
// The library (raven.dll, libraven.so, libraven.dylib: build Raven with
// -DRAVEN_BUILD_SHARED=ON) is looked for in the system property raven.library
// and the RAVEN_LIBRARY environment variable (its path), then in the jar
// (raven/native/<platform>/, which the package of Maven Central fills), then
// where the system looks. Raven belongs to the thread that starts it: call it
// from that thread only.

package raven;

import java.io.IOException;
import java.io.InputStream;
import java.lang.foreign.Arena;
import java.lang.foreign.FunctionDescriptor;
import java.lang.foreign.Linker;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.SymbolLookup;
import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.IdentityHashMap;
import java.util.Map;

import static java.lang.foreign.ValueLayout.ADDRESS;
import static java.lang.foreign.ValueLayout.JAVA_BYTE;
import static java.lang.foreign.ValueLayout.JAVA_DOUBLE;
import static java.lang.foreign.ValueLayout.JAVA_FLOAT;
import static java.lang.foreign.ValueLayout.JAVA_INT;
import static java.lang.foreign.ValueLayout.JAVA_LONG;

public final class R {
    private R() { }

    // ---- the library

    private static final Linker LINKER = Linker.nativeLinker();
    private static final SymbolLookup LIBRARY = load();
    private static final MethodHandles.Lookup LOOKUP = MethodHandles.lookup();
    /** The upcall of each callback given to the library: made once, alive until the program ends. */
    private static final Map<Object, MemorySegment> STUBS = new IdentityHashMap<>();
    /** The last text returned by each callback that returns one: the library reads it after the call. */
    private static final Map<Object, MemorySegment> RETURNED = new IdentityHashMap<>();

    private static SymbolLookup load() {
        String explicit = System.getProperty("raven.library");
        if (explicit == null || explicit.isEmpty()) explicit = System.getenv("RAVEN_LIBRARY");
        if (explicit != null && !explicit.isEmpty()) return SymbolLookup.libraryLookup(Path.of(explicit), Arena.global());
        String name = System.mapLibraryName("raven");
        Path bundled = bundled(name);
        if (bundled != null) return SymbolLookup.libraryLookup(bundled, Arena.global());
        Path here = Path.of(name).toAbsolutePath();
        if (Files.isRegularFile(here)) return SymbolLookup.libraryLookup(here, Arena.global());
        try {
            return SymbolLookup.libraryLookup(name, Arena.global());
        } catch (IllegalArgumentException e) {
            throw new UnsatisfiedLinkError("the Raven library (" + name + ") was not found: set RAVEN_LIBRARY or the "
                + "system property raven.library to its path, or put it where the system looks for libraries");
        }
    }

    /** The library inside the jar, copied once to the temporary folder: null when the jar has none. */
    private static Path bundled(String name) {
        String os = System.getProperty("os.name", "").toLowerCase();
        String arch = System.getProperty("os.arch", "").toLowerCase();
        String cpu = arch.equals("aarch64") || arch.equals("arm64") ? "arm64" : "x64";
        String platform = os.startsWith("windows") ? "windows-" + cpu : os.startsWith("mac") ? "macos" : "linux-" + cpu;
        String resource = "/raven/native/" + platform + "/" + name;
        try (InputStream in = R.class.getResourceAsStream(resource)) {
            if (in == null) return null;
            byte[] library = in.readAllBytes();
            // one folder per content: a library in use (by another program) is never replaced
            String folder = "raven-" + Integer.toHexString(java.util.Arrays.hashCode(library)) + "-" + library.length;
            Path file = Path.of(System.getProperty("java.io.tmpdir"), folder, platform, name);
            if (Files.isRegularFile(file) && Files.size(file) == library.length) return file;
            Files.createDirectories(file.getParent());
            Path part = Files.createTempFile(file.getParent(), name, ".part");
            Files.write(part, library);
            try {
                Files.move(part, file, StandardCopyOption.REPLACE_EXISTING, StandardCopyOption.ATOMIC_MOVE);
            } catch (IOException raced) {
                // (another program copied it at the same time)
                Files.deleteIfExists(part);
                if (!Files.isRegularFile(file)) throw raced;
            }
            return file;
        } catch (IOException e) {
            throw new UnsatisfiedLinkError("cannot copy " + resource + " out of the jar: " + e);
        }
    }

    private static MethodHandle find(String name, FunctionDescriptor descriptor) {
        MemorySegment address = LIBRARY.find(name).orElseThrow(
            () -> new UnsatisfiedLinkError(name + " is not in the Raven library (an older version?)"));
        return LINKER.downcallHandle(address, descriptor);
    }

    private static MemorySegment stub(Object function, Class<?> type, String raw, FunctionDescriptor descriptor) {
        if (function == null) return MemorySegment.NULL;
        synchronized (STUBS) {
            MemorySegment stub = STUBS.get(function);
            if (stub != null) return stub;
            try {
                MethodType t = descriptor.toMethodType().insertParameterTypes(0, type);
                MethodHandle target = MethodHandles.insertArguments(LOOKUP.findStatic(R.class, raw, t), 0, function);
                stub = LINKER.upcallStub(target, descriptor, Arena.global());
            } catch (ReflectiveOperationException e) {
                throw new IllegalStateException(e);
            }
            STUBS.put(function, stub);
            return stub;
        }
    }

    private static RuntimeException fail(Throwable t) {
        if (t instanceof RuntimeException r) return r;
        if (t instanceof Error e) throw e;
        return new RuntimeException(t);
    }

    // ---- texts and memory

    /** A text of the library (an address) as a String; null for a null address. */
    public static String text(MemorySegment address) {
        if (address == null || address.equals(MemorySegment.NULL)) return null;
        return address.reinterpret(Long.MAX_VALUE).getString(0, StandardCharsets.UTF_8);
    }

    /** A text as memory the library can read, in UTF-8 and ended by a zero, allocated in `arena`. */
    public static MemorySegment utf8(Arena arena, String text) {
        return arena.allocateFrom(text, StandardCharsets.UTF_8);
    }

    /** Bytes as memory the library can read, allocated in `arena`. */
    public static MemorySegment bytes(Arena arena, byte[] data) {
        MemorySegment segment = arena.allocate(Math.max(data.length, 1));
        MemorySegment.copy(data, 0, segment, JAVA_BYTE, 0, data.length);
        return segment.asSlice(0, data.length);
    }

    private static MemorySegment texts(Arena arena, String[] texts) {
        if (texts == null) return MemorySegment.NULL;
        MemorySegment array = arena.allocate(ADDRESS, Math.max(texts.length, 1)).asSlice(0, ADDRESS.byteSize() * texts.length);
        for (int i = 0; i < texts.length; i++)
            array.setAtIndex(ADDRESS, i, texts[i] == null ? MemorySegment.NULL : utf8(arena, texts[i]));
        return array;
    }

    private static String fixedText(MemorySegment field) {
        long length = 0;
        while (length < field.byteSize() && field.get(JAVA_BYTE, length) != 0) length++;
        return new String(field.asSlice(0, length).toArray(JAVA_BYTE), StandardCharsets.UTF_8);
    }

    // ---- constants

    public static final int RAVEN_VERSION_MAJOR = 0;
    public static final int RAVEN_VERSION_MINOR = 1;
    public static final int RAVEN_VERSION_PATCH = 0;
    public static final String RAVEN_API_VERSION = "0.1";
    public static final int R_FALSE = 0;
    public static final int R_TRUE = 1;
    public static final int R_INVALID_ID = 0xFFFFFFFF;

    // RBackend
    /**
     * Built-in Vulkan renderer.
     */
    public static final int R_BACKEND_VULKAN = 1;
    /**
     * Renders nothing. Used for headless runs and unit tests.
     */
    public static final int R_BACKEND_NULL = 2;
    /**
     * Backend supplied with Rbackend_register (see raven_backend.h).
     */
    public static final int R_BACKEND_CUSTOM = 3;

    // RError
    public static final int R_NO_ERROR = 0x0070;
    public static final int R_ERROR_INVALID_ID = 0x0071;
    public static final int R_ERROR_INVALID_ENUM = 0x0072;
    public static final int R_ERROR_INVALID_VALUE = 0x0073;
    public static final int R_ERROR_INVALID_OPERATION = 0x0074;
    public static final int R_ERROR_BACKEND = 0x0075;
    public static final int R_ERROR_PLATFORM = 0x0076;
    public static final int R_ERROR_OUT_OF_MEMORY = 0x0077;
    public static final int R_ERROR_INVALID_FILE_FORMAT = 0x0078;
    public static final int R_ERROR_FILE_NOT_FOUND = 0x0079;
    public static final int R_ERROR_XML = 0x007A;
    public static final int R_ERROR_WRONG_THREAD = 0x007B;

    // RSeverity
    public static final int R_SEVERITY_WEAK_WARNING = 0x0080;
    public static final int R_SEVERITY_WARNING = 0x0081;
    public static final int R_SEVERITY_ERROR = 0x0082;
    public static final int R_SEVERITY_FATAL = 0x0083;

    // RWindowFlags
    public static final int R_WINDOW_DEFAULT = 0;
    public static final int R_WINDOW_NO_RESIZE = 1;
    public static final int R_WINDOW_HIDDEN = 2;
    public static final int R_WINDOW_BORDERLESS = 4;
    public static final int R_WINDOW_MAXIMIZED = 8;
    /**
     * No title bar from the OS: the content covers the whole window and
     * draws its own header, with a R_WIDGET_HEADER_BAR (raven_shell.h) or
     * with any widget given to Rwidget_set_window_drag. The window is still
     * moved, resized, snapped and maximized by the OS, as it does for its own
     * title bars.
     */
    public static final int R_WINDOW_CUSTOM_HEADER = 0x0010;
    /**
     * The window can be seen through: where the application draws nothing
     * opaque, what is behind the window on the screen shows (the desktop,
     * the other applications), not a color of the application. Glass then
     * looks at the screen itself. What the OS does to what shows through is
     * chosen with Rwindow_set_backdrop. See RBackdrop for what each window
     * system can do.
     */
    public static final int R_WINDOW_TRANSPARENT = 0x0020;

    // RBackdrop: What is seen behind a window created with R_WINDOW_TRANSPARENT.
    /**
     * Opaque: the window color, or the wallpaper of the theme.
     */
    public static final int R_BACKDROP_NONE = 0x0400;
    /**
     * The screen behind the window, as it is.
     */
    public static final int R_BACKDROP_TRANSPARENT = 0x0401;
    /**
     * The screen behind the window, blurred by the OS.
     */
    public static final int R_BACKDROP_BLUR = 0x0402;
    /**
     * Blurred, tinted and grained by the OS (the acrylic of Windows).
     */
    public static final int R_BACKDROP_ACRYLIC = 0x0403;
    /**
     * A soft tint taken from the wallpaper of the desktop (the mica of Windows 11).
     */
    public static final int R_BACKDROP_MICA = 0x0404;

    // RWindowSystem
    public static final int R_WINDOW_SYSTEM_NONE = 0x0090;
    public static final int R_WINDOW_SYSTEM_WIN32 = 0x0091;
    public static final int R_WINDOW_SYSTEM_X11 = 0x0092;
    public static final int R_WINDOW_SYSTEM_WAYLAND = 0x0093;
    public static final int R_WINDOW_SYSTEM_COCOA = 0x0094;

    // RWindowEdge: Edges and corners of a window, for Rwindow_begin_resize.
    public static final int R_EDGE_LEFT = 0x00D0;
    public static final int R_EDGE_RIGHT = 0x00D1;
    public static final int R_EDGE_TOP = 0x00D2;
    public static final int R_EDGE_BOTTOM = 0x00D3;
    public static final int R_EDGE_TOP_LEFT = 0x00D4;
    public static final int R_EDGE_TOP_RIGHT = 0x00D5;
    public static final int R_EDGE_BOTTOM_LEFT = 0x00D6;
    public static final int R_EDGE_BOTTOM_RIGHT = 0x00D7;

    // RMouseButton
    public static final int R_MOUSE_LEFT = 0;
    public static final int R_MOUSE_RIGHT = 1;
    public static final int R_MOUSE_MIDDLE = 2;
    public static final int R_MOUSE_X1 = 3;
    public static final int R_MOUSE_X2 = 4;

    // RModifier
    public static final int R_MOD_SHIFT = 1;
    public static final int R_MOD_CTRL = 2;
    public static final int R_MOD_ALT = 4;
    public static final int R_MOD_SUPER = 8;

    // RCursor
    public static final int R_CURSOR_ARROW = 0x00B0;
    public static final int R_CURSOR_IBEAM = 0x00B1;
    public static final int R_CURSOR_HAND = 0x00B2;
    public static final int R_CURSOR_CROSSHAIR = 0x00B3;
    public static final int R_CURSOR_RESIZE_H = 0x00B4;
    public static final int R_CURSOR_RESIZE_V = 0x00B5;
    public static final int R_CURSOR_RESIZE_NWSE = 0x00B6;
    public static final int R_CURSOR_RESIZE_NESW = 0x00B7;
    public static final int R_CURSOR_MOVE = 0x00B8;
    public static final int R_CURSOR_NOT_ALLOWED = 0x00B9;
    /**
     * No pointer at all (a game that hides it). Windows; elsewhere the arrow.
     */
    public static final int R_CURSOR_NONE = 0x00BA;

    // RKey: Keyboard keys. Printable keys use their upper-case ASCII code.
    public static final int R_KEY_UNKNOWN = 0;
    public static final int R_KEY_SPACE = 0x0020;
    public static final int R_KEY_APOSTROPHE = 0x0027;
    public static final int R_KEY_COMMA = 0x002C;
    public static final int R_KEY_MINUS = 0x002D;
    public static final int R_KEY_PERIOD = 0x002E;
    public static final int R_KEY_SLASH = 0x002F;
    public static final int R_KEY_0 = 0x0030;
    public static final int R_KEY_1 = 0x0031;
    public static final int R_KEY_2 = 0x0032;
    public static final int R_KEY_3 = 0x0033;
    public static final int R_KEY_4 = 0x0034;
    public static final int R_KEY_5 = 0x0035;
    public static final int R_KEY_6 = 0x0036;
    public static final int R_KEY_7 = 0x0037;
    public static final int R_KEY_8 = 0x0038;
    public static final int R_KEY_9 = 0x0039;
    public static final int R_KEY_SEMICOLON = 0x003B;
    public static final int R_KEY_EQUAL = 0x003D;
    public static final int R_KEY_A = 0x0041;
    public static final int R_KEY_B = 0x0042;
    public static final int R_KEY_C = 0x0043;
    public static final int R_KEY_D = 0x0044;
    public static final int R_KEY_E = 0x0045;
    public static final int R_KEY_F = 0x0046;
    public static final int R_KEY_G = 0x0047;
    public static final int R_KEY_H = 0x0048;
    public static final int R_KEY_I = 0x0049;
    public static final int R_KEY_J = 0x004A;
    public static final int R_KEY_K = 0x004B;
    public static final int R_KEY_L = 0x004C;
    public static final int R_KEY_M = 0x004D;
    public static final int R_KEY_N = 0x004E;
    public static final int R_KEY_O = 0x004F;
    public static final int R_KEY_P = 0x0050;
    public static final int R_KEY_Q = 0x0051;
    public static final int R_KEY_R = 0x0052;
    public static final int R_KEY_S = 0x0053;
    public static final int R_KEY_T = 0x0054;
    public static final int R_KEY_U = 0x0055;
    public static final int R_KEY_V = 0x0056;
    public static final int R_KEY_W = 0x0057;
    public static final int R_KEY_X = 0x0058;
    public static final int R_KEY_Y = 0x0059;
    public static final int R_KEY_Z = 0x005A;
    public static final int R_KEY_LEFT_BRACKET = 0x005B;
    public static final int R_KEY_BACKSLASH = 0x005C;
    public static final int R_KEY_RIGHT_BRACKET = 0x005D;
    public static final int R_KEY_GRAVE = 0x0060;
    public static final int R_KEY_ESCAPE = 0x0100;
    public static final int R_KEY_ENTER = 0x0101;
    public static final int R_KEY_TAB = 0x0102;
    public static final int R_KEY_BACKSPACE = 0x0103;
    public static final int R_KEY_INSERT = 0x0104;
    public static final int R_KEY_DELETE = 0x0105;
    public static final int R_KEY_RIGHT = 0x0106;
    public static final int R_KEY_LEFT = 0x0107;
    public static final int R_KEY_DOWN = 0x0108;
    public static final int R_KEY_UP = 0x0109;
    public static final int R_KEY_PAGE_UP = 0x010A;
    public static final int R_KEY_PAGE_DOWN = 0x010B;
    public static final int R_KEY_HOME = 0x010C;
    public static final int R_KEY_END = 0x010D;
    public static final int R_KEY_CAPS_LOCK = 0x010E;
    public static final int R_KEY_PRINT_SCREEN = 0x010F;
    public static final int R_KEY_PAUSE = 0x0110;
    public static final int R_KEY_F1 = 0x0111;
    public static final int R_KEY_F2 = 0x0112;
    public static final int R_KEY_F3 = 0x0113;
    public static final int R_KEY_F4 = 0x0114;
    public static final int R_KEY_F5 = 0x0115;
    public static final int R_KEY_F6 = 0x0116;
    public static final int R_KEY_F7 = 0x0117;
    public static final int R_KEY_F8 = 0x0118;
    public static final int R_KEY_F9 = 0x0119;
    public static final int R_KEY_F10 = 0x011A;
    public static final int R_KEY_F11 = 0x011B;
    public static final int R_KEY_F12 = 0x011C;
    public static final int R_KEY_KP_0 = 0x011D;
    public static final int R_KEY_KP_1 = 0x011E;
    public static final int R_KEY_KP_2 = 0x011F;
    public static final int R_KEY_KP_3 = 0x0120;
    public static final int R_KEY_KP_4 = 0x0121;
    public static final int R_KEY_KP_5 = 0x0122;
    public static final int R_KEY_KP_6 = 0x0123;
    public static final int R_KEY_KP_7 = 0x0124;
    public static final int R_KEY_KP_8 = 0x0125;
    public static final int R_KEY_KP_9 = 0x0126;
    public static final int R_KEY_KP_DECIMAL = 0x0127;
    public static final int R_KEY_KP_DIVIDE = 0x0128;
    public static final int R_KEY_KP_MULTIPLY = 0x0129;
    public static final int R_KEY_KP_SUBTRACT = 0x012A;
    public static final int R_KEY_KP_ADD = 0x012B;
    public static final int R_KEY_KP_ENTER = 0x012C;
    public static final int R_KEY_LEFT_SHIFT = 0x012D;
    public static final int R_KEY_LEFT_CTRL = 0x012E;
    public static final int R_KEY_LEFT_ALT = 0x012F;
    public static final int R_KEY_LEFT_SUPER = 0x0130;
    public static final int R_KEY_RIGHT_SHIFT = 0x0131;
    public static final int R_KEY_RIGHT_CTRL = 0x0132;
    public static final int R_KEY_RIGHT_ALT = 0x0133;
    public static final int R_KEY_RIGHT_SUPER = 0x0134;
    public static final int R_KEY_MENU = 0x0135;
    public static final int R_KEY_COUNT = 0x0136;

    // REventType
    public static final int R_EVENT_NONE = 0x00C0;
    public static final int R_EVENT_MOUSE_MOVE = 0x00C1;
    public static final int R_EVENT_MOUSE_DOWN = 0x00C2;
    public static final int R_EVENT_MOUSE_UP = 0x00C3;
    public static final int R_EVENT_MOUSE_WHEEL = 0x00C4;
    public static final int R_EVENT_MOUSE_ENTER = 0x00C5;
    public static final int R_EVENT_MOUSE_LEAVE = 0x00C6;
    public static final int R_EVENT_KEY_DOWN = 0x00C7;
    public static final int R_EVENT_KEY_UP = 0x00C8;
    /**
     * A character was typed. `codepoint` holds the Unicode scalar value.
     */
    public static final int R_EVENT_TEXT = 0x00C9;
    public static final int R_EVENT_FOCUS_GAINED = 0x00CA;
    public static final int R_EVENT_FOCUS_LOST = 0x00CB;

    // RDropPhase: Moments of a drag coming from another application (files from the file manager, text...).
    /**
     * The pointer came over the target.
     */
    public static final int R_DROP_ENTER = 0x00E0;
    /**
     * It moves over the target.
     */
    public static final int R_DROP_MOVE = 0x00E1;
    /**
     * It left the target without dropping.
     */
    public static final int R_DROP_LEAVE = 0x00E2;
    /**
     * The user dropped: `paths` and `text` hold what was dropped.
     */
    public static final int R_DROP_DROPPED = 0x00E3;

    // RTouchMode: What a finger does on a widget (Rwidget_set_touch_mode).
    /**
     * The finger scrolls what scrolls under it, and is a mouse elsewhere (default).
     */
    public static final int R_TOUCH_MODE_AUTO = 0x0480;
    /**
     * A finger moving on the widget always scrolls what contains it, even over a control that drags.
     */
    public static final int R_TOUCH_MODE_SCROLL = 0x0481;
    /**
     * The finger is a mouse on the widget: pressing presses at once, moving drags, nothing scrolls.
     */
    public static final int R_TOUCH_MODE_MOUSE = 0x0482;
    /**
     * Fingers are ignored on the widget (only its touch callback sees them): a surface for the pen.
     */
    public static final int R_TOUCH_MODE_NONE = 0x0483;

    // RTouchPhase
    /**
     * A finger came down.
     */
    public static final int R_TOUCH_DOWN = 0x0490;
    public static final int R_TOUCH_MOVE = 0x0491;
    /**
     * The finger was lifted.
     */
    public static final int R_TOUCH_UP = 0x0492;
    /**
     * The system took the finger away (a gesture of its own, the window lost it): nothing must happen as for a lift.
     */
    public static final int R_TOUCH_CANCEL = 0x0493;

    // RPenPhase
    /**
     * The tip touched the screen.
     */
    public static final int R_PEN_DOWN = 0x04A0;
    /**
     * The pen moves while touching.
     */
    public static final int R_PEN_MOVE = 0x04A1;
    /**
     * The tip left the screen.
     */
    public static final int R_PEN_UP = 0x04A2;
    /**
     * The pen moves near the screen without touching it.
     */
    public static final int R_PEN_HOVER = 0x04A3;
    /**
     * The pen went out of reach.
     */
    public static final int R_PEN_LEAVE = 0x04A4;

    // RPenButton: Buttons on the side of a pen (RPenEvent::buttons).
    public static final int R_PEN_BUTTON_BARREL = 1;
    public static final int R_PEN_BUTTON_BARREL2 = 2;

    // RGesturePhase
    /**
     * A second finger came down.
     */
    public static final int R_GESTURE_BEGIN = 0x04B0;
    public static final int R_GESTURE_UPDATE = 0x04B1;
    /**
     * Fewer than two fingers remain.
     */
    public static final int R_GESTURE_END = 0x04B2;

    // RLook: The looks. Each one comes dark and light.
    /**
     * Raven, flat and dense, for tools. The look at start.
     */
    public static final int R_LOOK_RAVEN_DARK = 0x00E0;
    public static final int R_LOOK_RAVEN_LIGHT = 0x00E1;
    /**
     * Raven in glass: controls are capsules of translucent glass that refract
     * and blur what is behind them, catch the light on their rim and react
     * to the pointer with short animations. Windows get a soft colored
     * background unless they have their own (Rwindow_set_background,
     * Rwindow_set_clear_color).
     */
    public static final int R_LOOK_RAVEN_GLASS_DARK = 0x00E2;
    public static final int R_LOOK_RAVEN_GLASS_LIGHT = 0x00E3;
    /**
     * High contrast, for low vision: pure black and white, yellow (or blue)
     * focus and selection, thick borders, larger text (raven_accessibility.h,
     * Raccessibility_set_high_contrast).
     */
    public static final int R_LOOK_RAVEN_HIGH_CONTRAST_DARK = 0x00E4;
    public static final int R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT = 0x00E5;
    /**
     * Windows 11 (Fluent): its controls, its colors, the accent color the
     * user chose (on Windows), the Segoe UI font when the system has it, and
     * motion without bounce.
     */
    public static final int R_LOOK_WINDOWS_DARK = 0x00E6;
    public static final int R_LOOK_WINDOWS_LIGHT = 0x00E7;
    /**
     * macOS (Sequoia, Aqua): its controls, its colors, the accent color the
     * user chose (on macOS), the San Francisco font when the system has it,
     * no change under the pointer, and selections in the accent color.
     */
    public static final int R_LOOK_MACOS_DARK = 0x00E8;
    public static final int R_LOOK_MACOS_LIGHT = 0x00E9;
    /**
     * GNOME (libadwaita), the desktop of most Linux distributions: flat
     * rounded controls, the accent and the font of the desktop (on Linux).
     */
    public static final int R_LOOK_GNOME_DARK = 0x00EA;
    public static final int R_LOOK_GNOME_LIGHT = 0x00EB;
    /**
     * The look of the platform the application runs on: Windows on Windows,
     * macOS on macOS (macOS Glass from macOS 26 on), GNOME on Linux and the
     * other systems.
     */
    public static final int R_LOOK_NATIVE_DARK = 0x00EC;
    public static final int R_LOOK_NATIVE_LIGHT = 0x00ED;
    /**
     * macOS 26 (Tahoe, Liquid Glass): the controls of macOS, larger and
     * rounder, and glass where the system puts it, on what floats above the
     * content: header bars and their buttons, menus, drop-down lists,
     * tooltips, sheets; the knobs of sliders and switches turn into a lens
     * of clear glass while they are held. The content itself (fields, lists,
     * text) stays opaque. The accent color and the font of the system, on
     * macOS.
     */
    public static final int R_LOOK_MACOS_GLASS_DARK = 0x00EE;
    public static final int R_LOOK_MACOS_GLASS_LIGHT = 0x00EF;

    // RTheme: @deprecated The built-in themes of Raven 0.2, now looks: R_THEME_DARK is
    /**
     * Flat, dense themes for tools.
     */
    public static final int R_THEME_DARK = 0x00E0;
    public static final int R_THEME_LIGHT = 0x00E1;
    /**
     * Glass themes: controls are capsules of translucent glass that refract
     * and blur what is behind them, catch the light on their rim and react
     * to the pointer with short animations. Windows get a soft colored
     * background unless they have their own (Rwindow_set_background,
     * Rwindow_set_clear_color).
     */
    public static final int R_THEME_GLASS_DARK = 0x00E2;
    public static final int R_THEME_GLASS_LIGHT = 0x00E3;
    /**
     * High contrast, for low vision: pure black and white, yellow (or blue)
     * focus and selection, thick borders, larger text (raven_accessibility.h,
     * Raccessibility_set_high_contrast).
     */
    public static final int R_THEME_HIGH_CONTRAST_DARK = 0x00E4;
    public static final int R_THEME_HIGH_CONTRAST_LIGHT = 0x00E5;

    // RMaterial: What surfaces are made of, from the clearest to the most opaque. A theme
    /**
     * Opaque: filled with the colors of the theme. The material of R_THEME_DARK and R_THEME_LIGHT.
     */
    public static final int R_MATERIAL_SOLID = 0x02E0;
    /**
     * Clear glass: what is behind stays sharp enough to be recognized, strongly bent at the rim.
     */
    public static final int R_MATERIAL_GLASS = 0x02E1;
    /**
     * Frosted glass: what is behind is blurred into soft colors. The material of the glass themes.
     */
    public static final int R_MATERIAL_FROSTED_GLASS = 0x02E2;
    /**
     * Frosted glass colored by the accent color of the theme (or of the widget): for what must stand out.
     */
    public static final int R_MATERIAL_TINTED_GLASS = 0x02E3;
    /**
     * Not a material: "the one of the theme". Removes the material given to a widget.
     */
    public static final int R_MATERIAL_DEFAULT = 0x02E4;

    // RColorRole: Color roles. The comment lists the widgets reading each role.
    /**
     * Window clear color.
     */
    public static final int R_COLOR_WINDOW = 0x0100;
    /**
     * Border background.
     */
    public static final int R_COLOR_PANEL = 0x0101;
    /**
     * Border outline, list and input outlines.
     */
    public static final int R_COLOR_BORDER = 0x0102;
    /**
     * Text. Inherited by the children of the widget it is set on.
     */
    public static final int R_COLOR_TEXT = 0x0103;
    public static final int R_COLOR_TEXT_DISABLED = 0x0104;
    public static final int R_COLOR_TEXT_PLACEHOLDER = 0x0105;
    /**
     * Primary buttons, checked boxes, slider fill, progress fill.
     */
    public static final int R_COLOR_ACCENT = 0x0106;
    public static final int R_COLOR_ACCENT_HOVERED = 0x0107;
    public static final int R_COLOR_ACCENT_PRESSED = 0x0108;
    /**
     * Text and marks drawn on top of the accent color.
     */
    public static final int R_COLOR_ACCENT_TEXT = 0x0109;
    /**
     * Button, combo box, check box background.
     */
    public static final int R_COLOR_CONTROL = 0x010A;
    public static final int R_COLOR_CONTROL_HOVERED = 0x010B;
    public static final int R_COLOR_CONTROL_PRESSED = 0x010C;
    public static final int R_COLOR_CONTROL_BORDER = 0x010D;
    /**
     * Text input, spin box, list and tree background.
     */
    public static final int R_COLOR_INPUT = 0x010E;
    /**
     * Keyboard focus outline.
     */
    public static final int R_COLOR_FOCUS = 0x010F;
    /**
     * Selected rows and selected text.
     */
    public static final int R_COLOR_SELECTION = 0x0110;
    public static final int R_COLOR_ROW_HOVERED = 0x0111;
    /**
     * Slider track, progress background, scrollbar track.
     */
    public static final int R_COLOR_TRACK = 0x0112;
    /**
     * Scrollbar thumb, slider handle.
     */
    public static final int R_COLOR_HANDLE = 0x0113;
    public static final int R_COLOR_HANDLE_HOVERED = 0x0114;
    public static final int R_COLOR_SEPARATOR = 0x0115;
    /**
     * Menu bar, tab bar, expander header.
     */
    public static final int R_COLOR_HEADER = 0x0116;
    public static final int R_COLOR_HEADER_HOVERED = 0x0117;
    /**
     * Active tab.
     */
    public static final int R_COLOR_TAB_ACTIVE = 0x0118;
    /**
     * Menus, combo box lists.
     */
    public static final int R_COLOR_POPUP = 0x0119;
    public static final int R_COLOR_POPUP_BORDER = 0x011A;
    public static final int R_COLOR_SHADOW = 0x011B;
    public static final int R_COLOR_TOOLTIP = 0x011C;
    public static final int R_COLOR_TOOLTIP_TEXT = 0x011D;
    /**
     * Track of a switch that is on.
     */
    public static final int R_COLOR_SWITCH = 0x011E;
    /**
     * What went well, what needs attention, what is wrong or destroys something (markers, badges, classes).
     */
    public static final int R_COLOR_SUCCESS = 0x011F;
    public static final int R_COLOR_WARNING = 0x0120;
    public static final int R_COLOR_DANGER = 0x0121;
    /**
     * Links of formatted text.
     */
    public static final int R_COLOR_LINK = 0x0122;
    /**
     * Background of code in formatted text.
     */
    public static final int R_COLOR_CODE = 0x0123;

    // RMetric: Sizes, in logical units.
    public static final int R_METRIC_FONT_SIZE = 0x0140;
    public static final int R_METRIC_CORNER_RADIUS = 0x0141;
    public static final int R_METRIC_BORDER_WIDTH = 0x0142;
    /**
     * Height of buttons, inputs, combo boxes, tabs, menu bars.
     */
    public static final int R_METRIC_CONTROL_HEIGHT = 0x0143;
    /**
     * Horizontal / vertical padding inside controls.
     */
    public static final int R_METRIC_PADDING_X = 0x0144;
    public static final int R_METRIC_PADDING_Y = 0x0145;
    /**
     * Gap between an icon or a check mark and its text.
     */
    public static final int R_METRIC_SPACING = 0x0146;
    public static final int R_METRIC_SCROLLBAR_SIZE = 0x0147;
    /**
     * Height of list, tree and menu rows.
     */
    public static final int R_METRIC_ROW_HEIGHT = 0x0148;
    /**
     * Indentation per tree level.
     */
    public static final int R_METRIC_INDENT = 0x0149;
    public static final int R_METRIC_SPLITTER_SIZE = 0x014A;
    public static final int R_METRIC_CHECK_SIZE = 0x014B;
    public static final int R_METRIC_SLIDER_HANDLE_SIZE = 0x014C;
    /**
     * Corner radius of menus, drop-down lists and tooltips.
     */
    public static final int R_METRIC_POPUP_RADIUS = 0x014D;
    /**
     * The glass of the theme: each one multiplies what the material gives,
     * 1 = unchanged. Frost (how much is blurred), refraction (how far the rim
     * bends what is behind), saturation (of what is seen through), rim (the
     * light it catches) and shadow (the one it drops).
     */
    public static final int R_METRIC_GLASS_FROST = 0x014E;
    public static final int R_METRIC_GLASS_REFRACTION = 0x014F;
    public static final int R_METRIC_GLASS_SATURATION = 0x0150;
    public static final int R_METRIC_GLASS_RIM = 0x0151;
    public static final int R_METRIC_GLASS_SHADOW = 0x0152;
    /**
     * Radius of the blur of a BackgroundBlur that was not given one.
     */
    public static final int R_METRIC_BLUR_RADIUS = 0x0153;

    // RFontStyle: The styles of a font (Rfont_get_variant).
    public static final int R_FONT_REGULAR = 0x0420;
    public static final int R_FONT_BOLD = 0x0421;
    public static final int R_FONT_ITALIC = 0x0422;
    public static final int R_FONT_BOLD_ITALIC = 0x0423;

    // RFilterType
    public static final int R_FILTER_LINEAR = 0x02C0;
    /**
     * No smoothing: keeps pixel art sharp.
     */
    public static final int R_FILTER_NEAREST = 0x02C1;
    /**
     * Smooth, with mipmaps: an image shown much smaller than its size stays
     * clean instead of shimmering, for a third more GPU memory. The default
     * of the textures made from image files (Rtexture_create...). Backends
     * without mipmaps treat it as R_FILTER_LINEAR.
     */
    public static final int R_FILTER_MIPMAP = 0x02C2;

    // RWidgetType
    /**
     * Stacks children top to bottom. Slate: SVerticalBox.
     */
    public static final int R_WIDGET_VBOX = 0x0200;
    /**
     * Stacks children left to right. Slate: SHorizontalBox.
     */
    public static final int R_WIDGET_HBOX = 0x0201;
    /**
     * Rows and columns with spans and fill coefficients. Slate: SGridPanel.
     */
    public static final int R_WIDGET_GRID = 0x0202;
    /**
     * Grid whose cells all have the same size. Slate: SUniformGridPanel.
     */
    public static final int R_WIDGET_UNIFORM_GRID = 0x0203;
    /**
     * Draws children on top of each other. Slate: SOverlay.
     */
    public static final int R_WIDGET_OVERLAY = 0x0204;
    /**
     * One child with a background, an outline and inner padding. Slate: SBorder.
     */
    public static final int R_WIDGET_BORDER = 0x0205;
    /**
     * One child with size constraints or a fixed aspect ratio. Slate: SBox.
     */
    public static final int R_WIDGET_SIZE_BOX = 0x0206;
    /**
     * Empty space. Slate: SSpacer.
     */
    public static final int R_WIDGET_SPACER = 0x0207;
    /**
     * Scrollable stack of children. Slate: SScrollBox.
     */
    public static final int R_WIDGET_SCROLL_BOX = 0x0208;
    /**
     * Children separated by draggable handles. Slate: SSplitter.
     */
    public static final int R_WIDGET_SPLITTER = 0x0209;
    /**
     * Flows children and wraps to the next line. Slate: SWrapBox.
     */
    public static final int R_WIDGET_WRAP_BOX = 0x020A;
    /**
     * Children placed with anchors and offsets. Slate: SConstraintCanvas.
     */
    public static final int R_WIDGET_CANVAS = 0x020B;
    /**
     * Shows one child at a time. Slate: SWidgetSwitcher.
     */
    public static final int R_WIDGET_SWITCHER = 0x020C;
    /**
     * Tab bar + one page per child.
     */
    public static final int R_WIDGET_TAB_VIEW = 0x020D;
    /**
     * Collapsible header + one child. Slate: SExpandableArea.
     */
    public static final int R_WIDGET_EXPANDER = 0x020E;
    public static final int R_WIDGET_LABEL = 0x020F;
    public static final int R_WIDGET_BUTTON = 0x0210;
    public static final int R_WIDGET_CHECKBOX = 0x0211;
    public static final int R_WIDGET_SLIDER = 0x0212;
    public static final int R_WIDGET_PROGRESSBAR = 0x0213;
    public static final int R_WIDGET_IMAGE = 0x0214;
    public static final int R_WIDGET_TEXT_INPUT = 0x0215;
    public static final int R_WIDGET_SPIN_BOX = 0x0216;
    public static final int R_WIDGET_COMBO_BOX = 0x0217;
    public static final int R_WIDGET_SEPARATOR = 0x0218;
    public static final int R_WIDGET_LIST_VIEW = 0x0219;
    public static final int R_WIDGET_TREE_VIEW = 0x021A;
    public static final int R_WIDGET_MENU_BAR = 0x021B;
    public static final int R_WIDGET_MENU = 0x021C;
    public static final int R_WIDGET_MENU_ITEM = 0x021D;
    /**
     * Area showing content rendered by the application.
     */
    public static final int R_WIDGET_VIEWPORT = 0x021E;
    /**
     * Widget painted and measured by application callbacks.
     */
    public static final int R_WIDGET_CUSTOM = 0x021F;
    /**
     * Multi-line text editor.
     */
    public static final int R_WIDGET_TEXT_EDITOR = 0x0220;
    /**
     * Title bar of a window with a custom header, holding any widget.
     */
    public static final int R_WIDGET_HEADER_BAR = 0x0221;
    /**
     * Panels docked side by side or behind tabs, that the user rearranges.
     */
    public static final int R_WIDGET_DOCK_AREA = 0x0222;
    /**
     * A capsule that unfolds into a panel, at the top of a window.
     */
    public static final int R_WIDGET_ISLAND = 0x0223;
    /**
     * Explorer of the files of the computer.
     */
    public static final int R_WIDGET_FILE_BROWSER = 0x0224;
    /**
     * One child over a blurred view of what is behind it.
     */
    public static final int R_WIDGET_BACKGROUND_BLUR = 0x0225;
    /**
     * Chooses a color: hue, saturation, value, opacity, by hand or by numbers.
     */
    public static final int R_WIDGET_COLOR_PICKER = 0x0226;
    /**
     * Tracks of clips and keys along a time ruler.
     */
    public static final int R_WIDGET_TIMELINE = 0x0227;
    /**
     * Nodes linked by wires, on a canvas that pans and zooms.
     */
    public static final int R_WIDGET_NODE_GRAPH = 0x0228;
    /**
     * Grid of tiles (an icon over a name), virtualized. Slate: STileView.
     */
    public static final int R_WIDGET_TILE_VIEW = 0x0229;
    /**
     * Blocks that snap into each other, in the style of Scratch.
     */
    public static final int R_WIDGET_BLOCK_EDITOR = 0x022A;

    // RHAlign
    public static final int R_HALIGN_FILL = 0x0240;
    public static final int R_HALIGN_LEFT = 0x0241;
    public static final int R_HALIGN_CENTER = 0x0242;
    public static final int R_HALIGN_RIGHT = 0x0243;

    // RVAlign
    public static final int R_VALIGN_FILL = 0x0250;
    public static final int R_VALIGN_TOP = 0x0251;
    public static final int R_VALIGN_CENTER = 0x0252;
    public static final int R_VALIGN_BOTTOM = 0x0253;

    // RVisibility
    /**
     * Drawn and laid out.
     */
    public static final int R_VISIBLE = 0x0260;
    /**
     * Not drawn, but still takes its space in the layout.
     */
    public static final int R_HIDDEN = 0x0261;
    /**
     * Not drawn and takes no space.
     */
    public static final int R_COLLAPSED = 0x0262;

    // RTransition: How a widget arrives. A transition animates what the widget and its
    public static final int R_TRANSITION_NONE = 0x0300;
    /**
     * Becomes opaque.
     */
    public static final int R_TRANSITION_FADE = 0x0301;
    /**
     * Grows from a smaller size and bounces a little past its own.
     */
    public static final int R_TRANSITION_POP = 0x0302;
    /**
     * Shrinks from a larger size.
     */
    public static final int R_TRANSITION_ZOOM = 0x0303;
    /**
     * Flies in from below / above / the right / the left, and settles with a bounce.
     */
    public static final int R_TRANSITION_FLY_UP = 0x0304;
    public static final int R_TRANSITION_FLY_DOWN = 0x0305;
    public static final int R_TRANSITION_FLY_LEFT = 0x0306;
    public static final int R_TRANSITION_FLY_RIGHT = 0x0307;
    /**
     * Falls from above, squashes on landing and wobbles.
     */
    public static final int R_TRANSITION_DROP = 0x0308;
    /**
     * Short slide from the right / from the left, without bounce: pages that replace one another.
     */
    public static final int R_TRANSITION_SLIDE_LEFT = 0x0309;
    public static final int R_TRANSITION_SLIDE_RIGHT = 0x030A;

    // ROrientation
    public static final int R_HORIZONTAL = 0x0270;
    public static final int R_VERTICAL = 0x0271;

    // RBlurStyle: The looks of a R_WIDGET_BACKGROUND_BLUR. The first three are drawn by
    /**
     * What is behind, out of focus, and nothing else (a Gaussian blur).
     */
    public static final int R_BLUR_PLAIN = 0x0440;
    /**
     * Frosted: blurred, its colors a little stronger, under a veil of the
     * panel color of the theme, a fine grain and a hairline. The default.
     */
    public static final int R_BLUR_FROSTED = 0x0441;
    /**
     * Glass: a slab that bends what is behind at its rim and catches the
     * light, as the glass materials do (RMaterial); the radius says how
     * frosted it is.
     */
    public static final int R_BLUR_GLASS = 0x0442;
    /**
     * Native: the widget opens the window there, and what shows is what the
     * OS puts behind the window (Rwindow_set_backdrop: the blurred desktop,
     * acrylic, mica), under a light veil. It needs a window created with
     * R_WINDOW_TRANSPARENT that the OS could make see-through; anywhere
     * else it is drawn as R_BLUR_FROSTED. The radius is the one of the OS.
     */
    public static final int R_BLUR_NATIVE = 0x0443;

    // RDirection
    /**
     * A widget: the direction of its parent (the default).
     */
    public static final int R_DIRECTION_INHERIT = 0;
    public static final int R_DIRECTION_LTR = 0x0720;
    public static final int R_DIRECTION_RTL = 0x0721;
    /**
     * The windows: the direction of the language.
     */
    public static final int R_DIRECTION_AUTO = 0x0722;

    // RTextAlign
    public static final int R_TEXT_ALIGN_LEFT = 0x0280;
    public static final int R_TEXT_ALIGN_CENTER = 0x0281;
    public static final int R_TEXT_ALIGN_RIGHT = 0x0282;

    // RButtonStyle
    /**
     * Filled with R_COLOR_CONTROL.
     */
    public static final int R_BUTTON_DEFAULT = 0x0290;
    /**
     * Filled with R_COLOR_ACCENT.
     */
    public static final int R_BUTTON_PRIMARY = 0x0291;
    /**
     * No background until hovered (toolbars).
     */
    public static final int R_BUTTON_FLAT = 0x0292;

    // RCheckboxStyle
    public static final int R_CHECKBOX_CHECK = 0x02A0;
    /**
     * Round mark. Checking one unchecks the other radio boxes of the same parent.
     */
    public static final int R_CHECKBOX_RADIO = 0x02A1;
    /**
     * On / off switch.
     */
    public static final int R_CHECKBOX_SWITCH = 0x02A2;

    // RStretch
    /**
     * Stretches the image to the widget, ignoring its aspect ratio.
     */
    public static final int R_STRETCH_FILL = 0x02B0;
    /**
     * Largest size that fits entirely, keeping the aspect ratio.
     */
    public static final int R_STRETCH_FIT = 0x02B1;
    /**
     * Smallest size that covers the widget, keeping the aspect ratio (cropped).
     */
    public static final int R_STRETCH_COVER = 0x02B2;
    /**
     * Original size, centered.
     */
    public static final int R_STRETCH_NONE = 0x02B3;

    // RSyntax: Shorthands of Rtext_editor_set_language for two languages.
    public static final int R_SYNTAX_NONE = 0x0340;
    public static final int R_SYNTAX_XML = 0x0341;
    public static final int R_SYNTAX_C = 0x0342;

    // RLanguageFlags
    /**
     * Tags and attributes instead of code: XML, HTML. The words and the comments of the description are not used.
     */
    public static final int R_LANGUAGE_MARKUP = 1;
    /**
     * Selectors, properties and values: CSS.
     */
    public static final int R_LANGUAGE_STYLE_SHEET = 2;
    /**
     * A block comment may hold another one (Rust).
     */
    public static final int R_LANGUAGE_NESTED_COMMENTS = 4;
    /**
     * Between single quotes there is one character; any other single quote is not a text (C, Java, Rust).
     */
    public static final int R_LANGUAGE_CHAR_LITERALS = 8;
    /**
     * A line that ends with ':' indents the next one (Python).
     */
    public static final int R_LANGUAGE_INDENT_ON_COLON = 0x0010;
    /**
     * The words of the language are recognized whatever their case.
     */
    public static final int R_LANGUAGE_IGNORE_CASE = 0x0020;
    /**
     * Markup that holds JavaScript in script elements and CSS in style elements (HTML).
     */
    public static final int R_LANGUAGE_EMBEDS = 0x0040;
    /**
     * Formatted text written in Markdown: titles, strong and emphasized
     * text, code, links, lists. The editor shows the text as it is written,
     * marks included, with strong text in bold, emphasized text in italic
     * and titles in bold: what will be formatted is seen while typing.
     */
    public static final int R_LANGUAGE_FORMATTED_TEXT = 0x0080;

    // RToken: What a part of the text is, for the language of the editor (Rtext_editor_get_token).
    public static final int R_TOKEN_TEXT = 0x03A0;
    public static final int R_TOKEN_COMMENT = 0x03A1;
    public static final int R_TOKEN_STRING = 0x03A2;
    public static final int R_TOKEN_NUMBER = 0x03A3;
    public static final int R_TOKEN_KEYWORD = 0x03A4;
    /**
     * A type, or a name that is always there.
     */
    public static final int R_TOKEN_TYPE = 0x03A5;
    /**
     * A name followed by an opening parenthesis.
     */
    public static final int R_TOKEN_FUNCTION = 0x03A6;
    /**
     * The name of an element, in markup and in the selectors of a style sheet.
     */
    public static final int R_TOKEN_TAG = 0x03A7;
    /**
     * The name of an attribute (markup), of a class or of a property (style sheets), of a key (JSON).
     */
    public static final int R_TOKEN_ATTRIBUTE = 0x03A8;
    public static final int R_TOKEN_PUNCTUATION = 0x03A9;
    /**
     * A line for the preprocessor, an annotation, a declaration of a document.
     */
    public static final int R_TOKEN_DIRECTIVE = 0x03AA;
    /**
     * Formatted text (the Markdown language): a title, strong text, emphasized text, a link.
     */
    public static final int R_TOKEN_HEADING = 0x03AB;
    public static final int R_TOKEN_STRONG = 0x03AC;
    public static final int R_TOKEN_EMPHASIS = 0x03AD;
    public static final int R_TOKEN_LINK = 0x03AE;

    // RMarkerKind: What a marker says about the text it underlines.
    public static final int R_MARKER_ERROR = 0x0350;
    public static final int R_MARKER_WARNING = 0x0351;
    public static final int R_MARKER_INFO = 0x0352;

    // RHeaderButtons: Buttons of a header bar.
    public static final int R_HEADER_NO_BUTTONS = 0;
    public static final int R_HEADER_CLOSE = 1;
    public static final int R_HEADER_MINIMIZE = 2;
    public static final int R_HEADER_MAXIMIZE = 4;
    public static final int R_HEADER_ALL_BUTTONS = 7;

    // RDockSide: Where a panel goes in a dock area, relative to another panel or to the whole area.
    public static final int R_DOCK_LEFT = 0x0360;
    public static final int R_DOCK_RIGHT = 0x0361;
    public static final int R_DOCK_TOP = 0x0362;
    public static final int R_DOCK_BOTTOM = 0x0363;
    /**
     * Behind the same tabs as the other panel.
     */
    public static final int R_DOCK_TAB = 0x0364;

    // RTimelineItem: What an item of a timeline is.
    public static final int R_TIMELINE_NONE = 0x0460;
    public static final int R_TIMELINE_CLIP = 0x0461;
    public static final int R_TIMELINE_KEY = 0x0462;
    public static final int R_TIMELINE_MARKER = 0x0463;

    // RBlockShape
    /**
     * A step: snaps under another one, others snap under it.
     */
    public static final int R_BLOCK_STATEMENT = 0x0700;
    /**
     * Starts a script ("when the game starts"): nothing goes above it.
     */
    public static final int R_BLOCK_HAT = 0x0701;
    /**
     * Ends a script ("stop"): nothing goes under it.
     */
    public static final int R_BLOCK_CAP = 0x0702;
    /**
     * Wraps stacks in its mouths ("repeat", "if ... else").
     */
    public static final int R_BLOCK_C = 0x0703;
    /**
     * A rounded value that goes into a slot.
     */
    public static final int R_BLOCK_REPORTER = 0x0704;
    /**
     * A hexagonal condition that goes into a %b slot (or a %r one).
     */
    public static final int R_BLOCK_BOOLEAN = 0x0705;

    // RBlockPlace
    public static final int R_BLOCK_NEXT = 0x0710;
    public static final int R_BLOCK_BODY = 0x0711;
    public static final int R_BLOCK_INPUT = 0x0712;

    // RFileDialogFlags
    /**
     * Several files can be chosen (Rfile_dialog_open): see Rfile_dialog_get_result.
     */
    public static final int R_FILE_DIALOG_MULTIPLE = 1;
    /**
     * Never the dialog of the OS: the file browser of Raven, in a dialog of the parent window.
     */
    public static final int R_FILE_DIALOG_RAVEN = 2;
    public static final int R_FILE_DIALOG_SHOW_HIDDEN = 4;

    // RFileBrowserMode
    /**
     * Choose an existing file.
     */
    public static final int R_FILE_BROWSER_OPEN = 0x0380;
    /**
     * Choose where to save: a name is typed; an existing file must be confirmed.
     */
    public static final int R_FILE_BROWSER_SAVE = 0x0381;
    /**
     * Choose a folder.
     */
    public static final int R_FILE_BROWSER_FOLDER = 0x0382;

    // RMessageButtons
    public static final int R_MESSAGE_OK = 0x0390;
    public static final int R_MESSAGE_OK_CANCEL = 0x0391;
    public static final int R_MESSAGE_YES_NO = 0x0392;
    public static final int R_MESSAGE_YES_NO_CANCEL = 0x0393;

    // RMessageResult
    /**
     * Dismissed (Escape), or the Cancel button.
     */
    public static final int R_MESSAGE_RESULT_CANCEL = 0;
    public static final int R_MESSAGE_RESULT_OK = 1;
    public static final int R_MESSAGE_RESULT_YES = 2;
    public static final int R_MESSAGE_RESULT_NO = 3;

    // RMotion: How a spring moves. Animations in Raven are springs, as on iOS: a value is
    /**
     * Fast, no bounce: hover, highlights.
     */
    public static final int R_MOTION_SNAPPY = 0x0320;
    /**
     * Slower, no bounce: scrolling, fades, sizes.
     */
    public static final int R_MOTION_SMOOTH = 0x0321;
    /**
     * Runs a little past the target and comes back: switches, popups, pressed buttons.
     */
    public static final int R_MOTION_BOUNCY = 0x0322;
    /**
     * Swings several times around the target: things that fly in or get dropped.
     */
    public static final int R_MOTION_WOBBLY = 0x0323;

    // REffect: The effects that come with Raven (Reffect_get).
    /**
     * Soft lights that drift slowly: color, color2; params: speed, scale, intensity, -. Animated.
     */
    public static final int R_EFFECT_AURORA = 0x0B40;
    /**
     * A band of light that sweeps across (loading placeholders): color = base, color2 = light; params: speed, width, angle, -. Animated.
     */
    public static final int R_EFFECT_SHIMMER = 0x0B41;
    /**
     * A glowing edge whose colors run around the shape: color, color2; params: speed, width, glow, -. Animated.
     */
    public static final int R_EFFECT_NEON = 0x0B42;
    /**
     * Rings that ripple what is behind, like water (reads the backdrop): color = tint; params: speed, strength, density, center x (0..1). Animated.
     */
    public static final int R_EFFECT_RIPPLE = 0x0B43;
    /**
     * Bands of color that flow into each other: color, color2; params: speed, scale, contrast, -. Animated.
     */
    public static final int R_EFFECT_PLASMA = 0x0B44;
    /**
     * A gradient with the fine grain of film: color (top left) to color2 (bottom right); params: grain, angle, -, -. Animated grain.
     */
    public static final int R_EFFECT_GRAIN = 0x0B45;
    /**
     * A soft light that follows a point: color = light, color2 = background; params: x, y (0..1), radius (0..1), -.
     */
    public static final int R_EFFECT_SPOTLIGHT = 0x0B46;
    /**
     * Four colors blended across the surface, slowly moving: color, color2 and their mix; params: speed, -, -, -. Animated.
     */
    public static final int R_EFFECT_MESH = 0x0B47;

    // RShaderFlags: Flags of Rshader_create.
    /**
     * The effect reads what is drawn behind it (raven_backdrop, raven_backdrop_blur).
     */
    public static final int R_SHADER_BACKDROP = 1;
    /**
     * The effect changes with time: the window keeps drawing frames while it is shown.
     */
    public static final int R_SHADER_ANIMATED = 2;

    // RXmlValueType: The kind of value an attribute takes.
    public static final int R_XML_TEXT = 0x03C0;
    /**
     * "true" or "false".
     */
    public static final int R_XML_BOOL = 0x03C1;
    public static final int R_XML_NUMBER = 0x03C2;
    /**
     * One of the words of `choices`.
     */
    public static final int R_XML_CHOICE = 0x03C3;
    /**
     * "#RGB", "#RRGGBB", "#RRGGBBAA", or a name: white, black, red, transparent...
     */
    public static final int R_XML_COLOR = 0x03C4;
    /**
     * The name of an action (Raction_bind).
     */
    public static final int R_XML_ACTION = 0x03C5;
    /**
     * The path of a file.
     */
    public static final int R_XML_FILE = 0x03C6;
    /**
     * Several numbers separated by commas: "8,4", "0,0,1,1".
     */
    public static final int R_XML_NUMBERS = 0x03C7;
    /**
     * Texts separated by '|': "One|Two|Three".
     */
    public static final int R_XML_LIST = 0x03C8;

    // RColorFilter
    public static final int R_COLOR_FILTER_NONE = 0x0740;
    /**
     * Corrections: the colors that cannot be told apart are moved to colors that can.
     */
    public static final int R_COLOR_FILTER_PROTANOPIA = 0x0741;
    public static final int R_COLOR_FILTER_DEUTERANOPIA = 0x0742;
    public static final int R_COLOR_FILTER_TRITANOPIA = 0x0743;
    public static final int R_COLOR_FILTER_GRAYSCALE = 0x0744;
    public static final int R_COLOR_FILTER_INVERT = 0x0745;
    /**
     * Simulations, to see an interface as the color blind see it.
     */
    public static final int R_COLOR_FILTER_SIMULATE_PROTANOPIA = 0x0746;
    public static final int R_COLOR_FILTER_SIMULATE_DEUTERANOPIA = 0x0747;
    public static final int R_COLOR_FILTER_SIMULATE_TRITANOPIA = 0x0748;
    public static final int R_COLOR_FILTER_SIMULATE_ACHROMATOPSIA = 0x0749;

    // RAccessibleRole
    /**
     * The role of the type of the widget (the default).
     */
    public static final int R_ROLE_AUTO = 0;
    /**
     * Not exposed: its children are seen as the children of its parent.
     */
    public static final int R_ROLE_NONE = 0x0760;
    public static final int R_ROLE_WINDOW = 0x0761;
    public static final int R_ROLE_GROUP = 0x0762;
    public static final int R_ROLE_TEXT = 0x0763;
    public static final int R_ROLE_HEADING = 0x0764;
    public static final int R_ROLE_LINK = 0x0765;
    public static final int R_ROLE_BUTTON = 0x0766;
    public static final int R_ROLE_TOGGLE_BUTTON = 0x0767;
    public static final int R_ROLE_CHECKBOX = 0x0768;
    public static final int R_ROLE_RADIO_BUTTON = 0x0769;
    public static final int R_ROLE_SLIDER = 0x076A;
    public static final int R_ROLE_PROGRESS_BAR = 0x076B;
    public static final int R_ROLE_SPIN_BUTTON = 0x076C;
    public static final int R_ROLE_TEXT_FIELD = 0x076D;
    public static final int R_ROLE_DOCUMENT = 0x076E;
    public static final int R_ROLE_COMBO_BOX = 0x076F;
    public static final int R_ROLE_LIST = 0x0770;
    public static final int R_ROLE_LIST_ITEM = 0x0771;
    public static final int R_ROLE_TREE = 0x0772;
    public static final int R_ROLE_TREE_ITEM = 0x0773;
    public static final int R_ROLE_TABLE = 0x0774;
    public static final int R_ROLE_MENU_BAR = 0x0775;
    public static final int R_ROLE_MENU = 0x0776;
    public static final int R_ROLE_MENU_ITEM = 0x0777;
    public static final int R_ROLE_TAB_LIST = 0x0778;
    public static final int R_ROLE_TAB = 0x0779;
    public static final int R_ROLE_TAB_PANEL = 0x077A;
    public static final int R_ROLE_TOOLBAR = 0x077B;
    public static final int R_ROLE_STATUS_BAR = 0x077C;
    public static final int R_ROLE_DIALOG = 0x077D;
    public static final int R_ROLE_ALERT = 0x077E;
    public static final int R_ROLE_IMAGE = 0x077F;
    public static final int R_ROLE_CANVAS = 0x0780;
    public static final int R_ROLE_SEPARATOR = 0x0781;
    public static final int R_ROLE_SCROLL_BAR = 0x0782;
    public static final int R_ROLE_PANE = 0x0783;

    // ---- structures

    /**
     * Optional settings for Rinit_ex. A zero-initialized structure selects the
     * default for every field, so new fields can be added without breaking code.
     */
    public static final class RInitDesc {
        /** The size of the structure in C, in bytes. */
        public static final long SIZE = 40;
        private final MemorySegment s;
        private Arena arena;

        /** A new structure, zeroed (what `RInitDesc x = {0};` is in C). */
        public RInitDesc() {
            arena = Arena.ofAuto();
            s = arena.allocate(SIZE, 8);
        }

        /** The structure at an address (given by the library, or a slice of an array). */
        public RInitDesc(MemorySegment address) {
            s = address.reinterpret(SIZE);
        }

        private RInitDesc(MemorySegment segment, Arena arena) {
            this.s = segment;
            this.arena = arena;
        }

        /** Its memory: what to give where C takes a pointer to it. */
        public MemorySegment segment() { return s; }

        /** `count` structures side by side, zeroed: an array of C. */
        public static RInitDesc[] array(int count) {
            Arena a = Arena.ofAuto();
            MemorySegment all = a.allocate(SIZE * Math.max(count, 1), 8);
            RInitDesc[] items = new RInitDesc[count];
            for (int i = 0; i < count; i++) items[i] = new RInitDesc(all.asSlice(SIZE * i, SIZE), a);
            return items;
        }

        private Arena arena() {
            if (arena == null) arena = Arena.ofAuto();
            return arena;
        }

        /**
         * Rendering backend. 0 selects R_BACKEND_VULKAN.
         */
        public int backend() { return s.get(JAVA_INT, 0); }
        public RInitDesc backend(int value) { s.set(JAVA_INT, 0, value); return this; }

        /**
         * Application name reported to the graphics driver. May be NULL.
         */
        public String app_name() { return R.text(s.get(ADDRESS, 8)); }
        public RInitDesc app_name(String value) { s.set(ADDRESS, 8, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        /**
         * Worker threads of the job system. 0 = one per spare CPU core, negative = none.
         */
        public int worker_threads() { return s.get(JAVA_INT, 16); }
        public RInitDesc worker_threads(int value) { s.set(JAVA_INT, 16, value); return this; }

        /**
         * R_TRUE renders on the UI thread instead of a dedicated render thread.
         */
        public int disable_render_thread() { return s.get(JAVA_INT, 20); }
        public RInitDesc disable_render_thread(int value) { s.set(JAVA_INT, 20, value); return this; }

        /**
         * R_TRUE presents without waiting for the vertical blank.
         */
        public int disable_vsync() { return s.get(JAVA_INT, 24); }
        public RInitDesc disable_vsync(int value) { s.set(JAVA_INT, 24, value); return this; }

        /**
         * R_TRUE enables the backend debug layer (Vulkan validation) when installed.
         */
        public int enable_validation() { return s.get(JAVA_INT, 28); }
        public RInitDesc enable_validation(int value) { s.set(JAVA_INT, 28, value); return this; }

        /**
         * R_TRUE opens no OS window (null platform). Combine with R_BACKEND_NULL for tests.
         */
        public int headless() { return s.get(JAVA_INT, 32); }
        public RInitDesc headless(int value) { s.set(JAVA_INT, 32, value); return this; }

        /**
         * Global UI scale multiplier applied on top of the monitor scale. 0 = 1.0.
         */
        public float ui_scale() { return s.get(JAVA_FLOAT, 36); }
        public RInitDesc ui_scale(float value) { s.set(JAVA_FLOAT, 36, value); return this; }
    }

    /**
     * Memory functions of the application, to replace the allocator of Raven. See Rset_allocator.
     */
    public static final class RAllocator {
        /** The size of the structure in C, in bytes. */
        public static final long SIZE = 32;
        private final MemorySegment s;
        private Arena arena;

        /** A new structure, zeroed (what `RAllocator x = {0};` is in C). */
        public RAllocator() {
            arena = Arena.ofAuto();
            s = arena.allocate(SIZE, 8);
        }

        /** The structure at an address (given by the library, or a slice of an array). */
        public RAllocator(MemorySegment address) {
            s = address.reinterpret(SIZE);
        }

        private RAllocator(MemorySegment segment, Arena arena) {
            this.s = segment;
            this.arena = arena;
        }

        /** Its memory: what to give where C takes a pointer to it. */
        public MemorySegment segment() { return s; }

        /** `count` structures side by side, zeroed: an array of C. */
        public static RAllocator[] array(int count) {
            Arena a = Arena.ofAuto();
            MemorySegment all = a.allocate(SIZE * Math.max(count, 1), 8);
            RAllocator[] items = new RAllocator[count];
            for (int i = 0; i < count; i++) items[i] = new RAllocator(all.asSlice(SIZE * i, SIZE), a);
            return items;
        }

        private Arena arena() {
            if (arena == null) arena = Arena.ofAuto();
            return arena;
        }

        public MemorySegment alloc() { return s.get(ADDRESS, 0); }
        public RAllocator alloc(MemorySegment value) { s.set(ADDRESS, 0, value == null ? MemorySegment.NULL : value); return this; }

        public MemorySegment realloc() { return s.get(ADDRESS, 8); }
        public RAllocator realloc(MemorySegment value) { s.set(ADDRESS, 8, value == null ? MemorySegment.NULL : value); return this; }

        public MemorySegment free() { return s.get(ADDRESS, 16); }
        public RAllocator free(MemorySegment value) { s.set(ADDRESS, 16, value == null ? MemorySegment.NULL : value); return this; }

        public MemorySegment user_data() { return s.get(ADDRESS, 24); }
        public RAllocator user_data(MemorySegment value) { s.set(ADDRESS, 24, value == null ? MemorySegment.NULL : value); return this; }
    }

    /**
     * What the library holds. See Rmemory_get_stats.
     */
    public static final class RMemoryStats {
        /** The size of the structure in C, in bytes. */
        public static final long SIZE = 40;
        private final MemorySegment s;
        private Arena arena;

        /** A new structure, zeroed (what `RMemoryStats x = {0};` is in C). */
        public RMemoryStats() {
            arena = Arena.ofAuto();
            s = arena.allocate(SIZE, 8);
        }

        /** The structure at an address (given by the library, or a slice of an array). */
        public RMemoryStats(MemorySegment address) {
            s = address.reinterpret(SIZE);
        }

        private RMemoryStats(MemorySegment segment, Arena arena) {
            this.s = segment;
            this.arena = arena;
        }

        /** Its memory: what to give where C takes a pointer to it. */
        public MemorySegment segment() { return s; }

        /** `count` structures side by side, zeroed: an array of C. */
        public static RMemoryStats[] array(int count) {
            Arena a = Arena.ofAuto();
            MemorySegment all = a.allocate(SIZE * Math.max(count, 1), 8);
            RMemoryStats[] items = new RMemoryStats[count];
            for (int i = 0; i < count; i++) items[i] = new RMemoryStats(all.asSlice(SIZE * i, SIZE), a);
            return items;
        }

        private Arena arena() {
            if (arena == null) arena = Arena.ofAuto();
            return arena;
        }

        /**
         * Bytes in the blocks currently allocated. 0 with the allocator of the application.
         */
        public long used_bytes() { return s.get(JAVA_LONG, 0); }
        public RMemoryStats used_bytes(long value) { s.set(JAVA_LONG, 0, value); return this; }

        /**
         * Highest value of used_bytes since the start.
         */
        public long peak_bytes() { return s.get(JAVA_LONG, 8); }
        public RMemoryStats peak_bytes(long value) { s.set(JAVA_LONG, 8, value); return this; }

        /**
         * Bytes taken from the OS: used_bytes, plus what is free in the heap.
         */
        public long reserved_bytes() { return s.get(JAVA_LONG, 16); }
        public RMemoryStats reserved_bytes(long value) { s.set(JAVA_LONG, 16, value); return this; }

        /**
         * Blocks currently allocated. 0 after Rshutdown: anything else is a leak of the library.
         */
        public long allocation_count() { return s.get(JAVA_LONG, 24); }
        public RMemoryStats allocation_count(long value) { s.set(JAVA_LONG, 24, value); return this; }

        /**
         * Allocations made since the start.
         */
        public long total_allocations() { return s.get(JAVA_LONG, 32); }
        public RMemoryStats total_allocations(long value) { s.set(JAVA_LONG, 32, value); return this; }
    }

    /**
     * Timings and counters of the last frame produced for a window.
     */
    public static final class RFrameStats {
        /** The size of the structure in C, in bytes. */
        public static final long SIZE = 56;
        private final MemorySegment s;
        private Arena arena;

        /** A new structure, zeroed (what `RFrameStats x = {0};` is in C). */
        public RFrameStats() {
            arena = Arena.ofAuto();
            s = arena.allocate(SIZE, 8);
        }

        /** The structure at an address (given by the library, or a slice of an array). */
        public RFrameStats(MemorySegment address) {
            s = address.reinterpret(SIZE);
        }

        private RFrameStats(MemorySegment segment, Arena arena) {
            this.s = segment;
            this.arena = arena;
        }

        /** Its memory: what to give where C takes a pointer to it. */
        public MemorySegment segment() { return s; }

        /** `count` structures side by side, zeroed: an array of C. */
        public static RFrameStats[] array(int count) {
            Arena a = Arena.ofAuto();
            MemorySegment all = a.allocate(SIZE * Math.max(count, 1), 8);
            RFrameStats[] items = new RFrameStats[count];
            for (int i = 0; i < count; i++) items[i] = new RFrameStats(all.asSlice(SIZE * i, SIZE), a);
            return items;
        }

        private Arena arena() {
            if (arena == null) arena = Arena.ofAuto();
            return arena;
        }

        public double layout_ms() { return s.get(JAVA_DOUBLE, 0); }
        public RFrameStats layout_ms(double value) { s.set(JAVA_DOUBLE, 0, value); return this; }

        public double paint_ms() { return s.get(JAVA_DOUBLE, 8); }
        public RFrameStats paint_ms(double value) { s.set(JAVA_DOUBLE, 8, value); return this; }

        public double render_ms() { return s.get(JAVA_DOUBLE, 16); }
        public RFrameStats render_ms(double value) { s.set(JAVA_DOUBLE, 16, value); return this; }

        public int widget_count() { return s.get(JAVA_INT, 24); }
        public RFrameStats widget_count(int value) { s.set(JAVA_INT, 24, value); return this; }

        public int instance_count() { return s.get(JAVA_INT, 28); }
        public RFrameStats instance_count(int value) { s.set(JAVA_INT, 28, value); return this; }

        public int draw_call_count() { return s.get(JAVA_INT, 32); }
        public RFrameStats draw_call_count(int value) { s.set(JAVA_INT, 32, value); return this; }

        /**
         * Glass: times the frame was copied and blurred to be seen through (0 without glass).
         */
        public int backdrop_count() { return s.get(JAVA_INT, 36); }
        public RFrameStats backdrop_count(int value) { s.set(JAVA_INT, 36, value); return this; }

        public int paint_job_count() { return s.get(JAVA_INT, 40); }
        public RFrameStats paint_job_count(int value) { s.set(JAVA_INT, 40, value); return this; }

        public long frame_index() { return s.get(JAVA_LONG, 48); }
        public RFrameStats frame_index(long value) { s.set(JAVA_LONG, 48, value); return this; }
    }

    /**
     * Native handles of a window or of a native viewport.
     *   Win32 : display = HINSTANCE, window = HWND
     *   X11   : display = Display*,  window = Window (XID), visual = VisualID
     *   Wayland: display = wl_display*, window = wl_surface* (Linux and BSD choose
     *           Wayland or X11 when Rinit runs: check `system`)
     *   Cocoa : display = NSWindow*, window = CAMetalLayer* for Vulkan presentation
     */
    public static final class RNativeWindow {
        /** The size of the structure in C, in bytes. */
        public static final long SIZE = 40;
        private final MemorySegment s;
        private Arena arena;

        /** A new structure, zeroed (what `RNativeWindow x = {0};` is in C). */
        public RNativeWindow() {
            arena = Arena.ofAuto();
            s = arena.allocate(SIZE, 8);
        }

        /** The structure at an address (given by the library, or a slice of an array). */
        public RNativeWindow(MemorySegment address) {
            s = address.reinterpret(SIZE);
        }

        private RNativeWindow(MemorySegment segment, Arena arena) {
            this.s = segment;
            this.arena = arena;
        }

        /** Its memory: what to give where C takes a pointer to it. */
        public MemorySegment segment() { return s; }

        /** `count` structures side by side, zeroed: an array of C. */
        public static RNativeWindow[] array(int count) {
            Arena a = Arena.ofAuto();
            MemorySegment all = a.allocate(SIZE * Math.max(count, 1), 8);
            RNativeWindow[] items = new RNativeWindow[count];
            for (int i = 0; i < count; i++) items[i] = new RNativeWindow(all.asSlice(SIZE * i, SIZE), a);
            return items;
        }

        private Arena arena() {
            if (arena == null) arena = Arena.ofAuto();
            return arena;
        }

        public int system() { return s.get(JAVA_INT, 0); }
        public RNativeWindow system(int value) { s.set(JAVA_INT, 0, value); return this; }

        public MemorySegment display() { return s.get(ADDRESS, 8); }
        public RNativeWindow display(MemorySegment value) { s.set(ADDRESS, 8, value == null ? MemorySegment.NULL : value); return this; }

        public long window() { return s.get(JAVA_LONG, 16); }
        public RNativeWindow window(long value) { s.set(JAVA_LONG, 16, value); return this; }

        public long visual() { return s.get(JAVA_LONG, 24); }
        public RNativeWindow visual(long value) { s.set(JAVA_LONG, 24, value); return this; }

        /**
         * R_TRUE when the window was created with R_WINDOW_TRANSPARENT and the
         * window system gave it an alpha channel: what is presented in it must
         * carry alpha (premultiplied), which a rendering backend asks for when
         * it creates its surface.
         */
        public int transparent() { return s.get(JAVA_INT, 32); }
        public RNativeWindow transparent(int value) { s.set(JAVA_INT, 32, value); return this; }
    }

    /**
     * Input event delivered to a widget. Mouse and key events bubble from the
     * target widget to its parents until one of them handles the event.
     */
    public static final class REvent {
        /** The size of the structure in C, in bytes. */
        public static final long SIZE = 68;
        private final MemorySegment s;
        private Arena arena;

        /** A new structure, zeroed (what `REvent x = {0};` is in C). */
        public REvent() {
            arena = Arena.ofAuto();
            s = arena.allocate(SIZE, 4);
        }

        /** The structure at an address (given by the library, or a slice of an array). */
        public REvent(MemorySegment address) {
            s = address.reinterpret(SIZE);
        }

        private REvent(MemorySegment segment, Arena arena) {
            this.s = segment;
            this.arena = arena;
        }

        /** Its memory: what to give where C takes a pointer to it. */
        public MemorySegment segment() { return s; }

        /** `count` structures side by side, zeroed: an array of C. */
        public static REvent[] array(int count) {
            Arena a = Arena.ofAuto();
            MemorySegment all = a.allocate(SIZE * Math.max(count, 1), 4);
            REvent[] items = new REvent[count];
            for (int i = 0; i < count; i++) items[i] = new REvent(all.asSlice(SIZE * i, SIZE), a);
            return items;
        }

        private Arena arena() {
            if (arena == null) arena = Arena.ofAuto();
            return arena;
        }

        public int type() { return s.get(JAVA_INT, 0); }
        public REvent type(int value) { s.set(JAVA_INT, 0, value); return this; }

        public int window() { return s.get(JAVA_INT, 4); }
        public REvent window(int value) { s.set(JAVA_INT, 4, value); return this; }

        /**
         * Widget the event was first sent to.
         */
        public int target() { return s.get(JAVA_INT, 8); }
        public REvent target(int value) { s.set(JAVA_INT, 8, value); return this; }

        /**
         * Cursor position in the window, in logical units.
         */
        public float x() { return s.get(JAVA_FLOAT, 12); }
        public REvent x(float value) { s.set(JAVA_FLOAT, 12, value); return this; }

        /**
         * Cursor position in the window, in logical units.
         */
        public float y() { return s.get(JAVA_FLOAT, 16); }
        public REvent y(float value) { s.set(JAVA_FLOAT, 16, value); return this; }

        /**
         * Cursor position relative to the widget receiving the event.
         */
        public float local_x() { return s.get(JAVA_FLOAT, 20); }
        public REvent local_x(float value) { s.set(JAVA_FLOAT, 20, value); return this; }

        /**
         * Cursor position relative to the widget receiving the event.
         */
        public float local_y() { return s.get(JAVA_FLOAT, 24); }
        public REvent local_y(float value) { s.set(JAVA_FLOAT, 24, value); return this; }

        /**
         * Mouse movement since the previous event (R_EVENT_MOUSE_MOVE).
         */
        public float delta_x() { return s.get(JAVA_FLOAT, 28); }
        public REvent delta_x(float value) { s.set(JAVA_FLOAT, 28, value); return this; }

        /**
         * Mouse movement since the previous event (R_EVENT_MOUSE_MOVE).
         */
        public float delta_y() { return s.get(JAVA_FLOAT, 32); }
        public REvent delta_y(float value) { s.set(JAVA_FLOAT, 32, value); return this; }

        /**
         * Scroll amount in lines, positive = up / left (R_EVENT_MOUSE_WHEEL).
         */
        public float wheel_x() { return s.get(JAVA_FLOAT, 36); }
        public REvent wheel_x(float value) { s.set(JAVA_FLOAT, 36, value); return this; }

        /**
         * Scroll amount in lines, positive = up / left (R_EVENT_MOUSE_WHEEL).
         */
        public float wheel_y() { return s.get(JAVA_FLOAT, 40); }
        public REvent wheel_y(float value) { s.set(JAVA_FLOAT, 40, value); return this; }

        public int button() { return s.get(JAVA_INT, 44); }
        public REvent button(int value) { s.set(JAVA_INT, 44, value); return this; }

        /**
         * 1 = single click, 2 = double click, ... (R_EVENT_MOUSE_DOWN).
         */
        public int clicks() { return s.get(JAVA_INT, 48); }
        public REvent clicks(int value) { s.set(JAVA_INT, 48, value); return this; }

        public int key() { return s.get(JAVA_INT, 52); }
        public REvent key(int value) { s.set(JAVA_INT, 52, value); return this; }

        /**
         * Bitmask of RModifier.
         */
        public int modifiers() { return s.get(JAVA_INT, 56); }
        public REvent modifiers(int value) { s.set(JAVA_INT, 56, value); return this; }

        /**
         * R_TRUE when the key event comes from auto-repeat.
         */
        public int repeat() { return s.get(JAVA_INT, 60); }
        public REvent repeat(int value) { s.set(JAVA_INT, 60, value); return this; }

        public int codepoint() { return s.get(JAVA_INT, 64); }
        public REvent codepoint(int value) { s.set(JAVA_INT, 64, value); return this; }
    }

    /**
     * What a drop target receives. The files and the text are only known when
     * the drop happens (R_DROP_DROPPED), and are valid during the callback.
     */
    public static final class RDropEvent {
        /** The size of the structure in C, in bytes. */
        public static final long SIZE = 64;
        private final MemorySegment s;
        private Arena arena;

        /** A new structure, zeroed (what `RDropEvent x = {0};` is in C). */
        public RDropEvent() {
            arena = Arena.ofAuto();
            s = arena.allocate(SIZE, 8);
        }

        /** The structure at an address (given by the library, or a slice of an array). */
        public RDropEvent(MemorySegment address) {
            s = address.reinterpret(SIZE);
        }

        private RDropEvent(MemorySegment segment, Arena arena) {
            this.s = segment;
            this.arena = arena;
        }

        /** Its memory: what to give where C takes a pointer to it. */
        public MemorySegment segment() { return s; }

        /** `count` structures side by side, zeroed: an array of C. */
        public static RDropEvent[] array(int count) {
            Arena a = Arena.ofAuto();
            MemorySegment all = a.allocate(SIZE * Math.max(count, 1), 8);
            RDropEvent[] items = new RDropEvent[count];
            for (int i = 0; i < count; i++) items[i] = new RDropEvent(all.asSlice(SIZE * i, SIZE), a);
            return items;
        }

        private Arena arena() {
            if (arena == null) arena = Arena.ofAuto();
            return arena;
        }

        public int phase() { return s.get(JAVA_INT, 0); }
        public RDropEvent phase(int value) { s.set(JAVA_INT, 0, value); return this; }

        public int window() { return s.get(JAVA_INT, 4); }
        public RDropEvent window(int value) { s.set(JAVA_INT, 4, value); return this; }

        /**
         * Pointer position in the window, and relative to the target widget.
         */
        public float x() { return s.get(JAVA_FLOAT, 8); }
        public RDropEvent x(float value) { s.set(JAVA_FLOAT, 8, value); return this; }

        /**
         * Pointer position in the window, and relative to the target widget.
         */
        public float y() { return s.get(JAVA_FLOAT, 12); }
        public RDropEvent y(float value) { s.set(JAVA_FLOAT, 12, value); return this; }

        public float local_x() { return s.get(JAVA_FLOAT, 16); }
        public RDropEvent local_x(float value) { s.set(JAVA_FLOAT, 16, value); return this; }

        public float local_y() { return s.get(JAVA_FLOAT, 20); }
        public RDropEvent local_y(float value) { s.set(JAVA_FLOAT, 20, value); return this; }

        /**
         * Dropped files and folders, as UTF-8 paths.
         */
        public MemorySegment paths() { return s.get(ADDRESS, 24); }
        public RDropEvent paths(MemorySegment value) { s.set(ADDRESS, 24, value == null ? MemorySegment.NULL : value); return this; }

        public int path_count() { return s.get(JAVA_INT, 32); }
        public RDropEvent path_count(int value) { s.set(JAVA_INT, 32, value); return this; }

        /**
         * Dropped text when it is not files, or NULL. For a drag inside the application: its data.
         */
        public String text() { return R.text(s.get(ADDRESS, 40)); }
        public RDropEvent text(String value) { s.set(ADDRESS, 40, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        /**
         * Drag inside the application (Rdrag_begin): its type, else NULL. Set in every phase.
         */
        public String drag_type() { return R.text(s.get(ADDRESS, 48)); }
        public RDropEvent drag_type(String value) { s.set(ADDRESS, 48, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        /**
         * Widget the drag started from, or R_INVALID_ID.
         */
        public int drag_source() { return s.get(JAVA_INT, 56); }
        public RDropEvent drag_source(int value) { s.set(JAVA_INT, 56, value); return this; }
    }

    /**
     * How the library treats fingers and pens (Rtouch_set_settings, Rwindow_set_touch_settings).
     */
    public static final class RTouchSettings {
        /** The size of the structure in C, in bytes. */
        public static final long SIZE = 40;
        private final MemorySegment s;
        private Arena arena;

        /** A new structure, zeroed (what `RTouchSettings x = {0};` is in C). */
        public RTouchSettings() {
            arena = Arena.ofAuto();
            s = arena.allocate(SIZE, 4);
        }

        /** The structure at an address (given by the library, or a slice of an array). */
        public RTouchSettings(MemorySegment address) {
            s = address.reinterpret(SIZE);
        }

        private RTouchSettings(MemorySegment segment, Arena arena) {
            this.s = segment;
            this.arena = arena;
        }

        /** Its memory: what to give where C takes a pointer to it. */
        public MemorySegment segment() { return s; }

        /** `count` structures side by side, zeroed: an array of C. */
        public static RTouchSettings[] array(int count) {
            Arena a = Arena.ofAuto();
            MemorySegment all = a.allocate(SIZE * Math.max(count, 1), 4);
            RTouchSettings[] items = new RTouchSettings[count];
            for (int i = 0; i < count; i++) items[i] = new RTouchSettings(all.asSlice(SIZE * i, SIZE), a);
            return items;
        }

        private Arena arena() {
            if (arena == null) arena = Arena.ofAuto();
            return arena;
        }

        /**
         * R_TRUE (default): one finger drives the widgets as the mouse does: a tap clicks, a drag drags.
         */
        public int mouse_emulation() { return s.get(JAVA_INT, 0); }
        public RTouchSettings mouse_emulation(int value) { s.set(JAVA_INT, 0, value); return this; }

        /**
         * How far a finger moves (logical units) before a press becomes a drag or a scroll. Default 8.
         */
        public float slop() { return s.get(JAVA_FLOAT, 4); }
        public RTouchSettings slop(float value) { s.set(JAVA_FLOAT, 4, value); return this; }

        /**
         * Seconds a finger rests before the press is a right click (the context menu); 0 turns it off. Default 0.5.
         */
        public float long_press() { return s.get(JAVA_FLOAT, 8); }
        public RTouchSettings long_press(float value) { s.set(JAVA_FLOAT, 8, value); return this; }

        /**
         * R_TRUE (default): flicked content keeps going and slows down.
         */
        public int inertia() { return s.get(JAVA_INT, 12); }
        public RTouchSettings inertia(int value) { s.set(JAVA_INT, 12, value); return this; }

        /**
         * How fast it slows down, in logical units per second per second. Default 2400.
         */
        public float deceleration() { return s.get(JAVA_FLOAT, 16); }
        public RTouchSettings deceleration(float value) { s.set(JAVA_FLOAT, 16, value); return this; }

        /**
         * R_TRUE (default): two fingers pinch to zoom and pan.
         */
        public int gestures() { return s.get(JAVA_INT, 20); }
        public RTouchSettings gestures(int value) { s.set(JAVA_INT, 20, value); return this; }

        /**
         * R_TRUE (default): the pen drives the widgets as the mouse does (its barrel button: the right button).
         */
        public int pen_mouse_emulation() { return s.get(JAVA_INT, 24); }
        public RTouchSettings pen_mouse_emulation(int value) { s.set(JAVA_INT, 24, value); return this; }

        /**
         * The pressure given to the application is the one of the pen to this power: 1 (default) as it is, below 1 softer, above 1 harder.
         */
        public float pressure_curve() { return s.get(JAVA_FLOAT, 28); }
        public RTouchSettings pressure_curve(float value) { s.set(JAVA_FLOAT, 28, value); return this; }

        /**
         * Pressure (0..1, after the curve) the tip needs before it presses. Default 0.
         */
        public float pressure_threshold() { return s.get(JAVA_FLOAT, 32); }
        public RTouchSettings pressure_threshold(float value) { s.set(JAVA_FLOAT, 32, value); return this; }

        /**
         * R_TRUE (default): fingers are ignored while a pen is near the screen, and half a second after (the palm resting on it).
         */
        public int palm_rejection() { return s.get(JAVA_INT, 36); }
        public RTouchSettings palm_rejection(int value) { s.set(JAVA_INT, 36, value); return this; }
    }

    public static final class RTouchEvent {
        /** The size of the structure in C, in bytes. */
        public static final long SIZE = 52;
        private final MemorySegment s;
        private Arena arena;

        /** A new structure, zeroed (what `RTouchEvent x = {0};` is in C). */
        public RTouchEvent() {
            arena = Arena.ofAuto();
            s = arena.allocate(SIZE, 4);
        }

        /** The structure at an address (given by the library, or a slice of an array). */
        public RTouchEvent(MemorySegment address) {
            s = address.reinterpret(SIZE);
        }

        private RTouchEvent(MemorySegment segment, Arena arena) {
            this.s = segment;
            this.arena = arena;
        }

        /** Its memory: what to give where C takes a pointer to it. */
        public MemorySegment segment() { return s; }

        /** `count` structures side by side, zeroed: an array of C. */
        public static RTouchEvent[] array(int count) {
            Arena a = Arena.ofAuto();
            MemorySegment all = a.allocate(SIZE * Math.max(count, 1), 4);
            RTouchEvent[] items = new RTouchEvent[count];
            for (int i = 0; i < count; i++) items[i] = new RTouchEvent(all.asSlice(SIZE * i, SIZE), a);
            return items;
        }

        private Arena arena() {
            if (arena == null) arena = Arena.ofAuto();
            return arena;
        }

        public int phase() { return s.get(JAVA_INT, 0); }
        public RTouchEvent phase(int value) { s.set(JAVA_INT, 0, value); return this; }

        public int window() { return s.get(JAVA_INT, 4); }
        public RTouchEvent window(int value) { s.set(JAVA_INT, 4, value); return this; }

        /**
         * Widget the finger came down on.
         */
        public int target() { return s.get(JAVA_INT, 8); }
        public RTouchEvent target(int value) { s.set(JAVA_INT, 8, value); return this; }

        /**
         * Number of the finger, the same from its down to its up.
         */
        public int id() { return s.get(JAVA_INT, 12); }
        public RTouchEvent id(int value) { s.set(JAVA_INT, 12, value); return this; }

        /**
         * Position in the window, and relative to the widget receiving the event.
         */
        public float x() { return s.get(JAVA_FLOAT, 16); }
        public RTouchEvent x(float value) { s.set(JAVA_FLOAT, 16, value); return this; }

        /**
         * Position in the window, and relative to the widget receiving the event.
         */
        public float y() { return s.get(JAVA_FLOAT, 20); }
        public RTouchEvent y(float value) { s.set(JAVA_FLOAT, 20, value); return this; }

        public float local_x() { return s.get(JAVA_FLOAT, 24); }
        public RTouchEvent local_x(float value) { s.set(JAVA_FLOAT, 24, value); return this; }

        public float local_y() { return s.get(JAVA_FLOAT, 28); }
        public RTouchEvent local_y(float value) { s.set(JAVA_FLOAT, 28, value); return this; }

        /**
         * 0..1; 1 when the screen does not tell.
         */
        public float pressure() { return s.get(JAVA_FLOAT, 32); }
        public RTouchEvent pressure(float value) { s.set(JAVA_FLOAT, 32, value); return this; }

        /**
         * Size of the contact, 0 when the screen does not tell.
         */
        public float width() { return s.get(JAVA_FLOAT, 36); }
        public RTouchEvent width(float value) { s.set(JAVA_FLOAT, 36, value); return this; }

        /**
         * Size of the contact, 0 when the screen does not tell.
         */
        public float height() { return s.get(JAVA_FLOAT, 40); }
        public RTouchEvent height(float value) { s.set(JAVA_FLOAT, 40, value); return this; }

        /**
         * Fingers on the window, this one included (0 after the last up).
         */
        public int count() { return s.get(JAVA_INT, 44); }
        public RTouchEvent count(int value) { s.set(JAVA_INT, 44, value); return this; }

        /**
         * Bitmask of RModifier.
         */
        public int modifiers() { return s.get(JAVA_INT, 48); }
        public RTouchEvent modifiers(int value) { s.set(JAVA_INT, 48, value); return this; }
    }

    public static final class RPenEvent {
        /** The size of the structure in C, in bytes. */
        public static final long SIZE = 60;
        private final MemorySegment s;
        private Arena arena;

        /** A new structure, zeroed (what `RPenEvent x = {0};` is in C). */
        public RPenEvent() {
            arena = Arena.ofAuto();
            s = arena.allocate(SIZE, 4);
        }

        /** The structure at an address (given by the library, or a slice of an array). */
        public RPenEvent(MemorySegment address) {
            s = address.reinterpret(SIZE);
        }

        private RPenEvent(MemorySegment segment, Arena arena) {
            this.s = segment;
            this.arena = arena;
        }

        /** Its memory: what to give where C takes a pointer to it. */
        public MemorySegment segment() { return s; }

        /** `count` structures side by side, zeroed: an array of C. */
        public static RPenEvent[] array(int count) {
            Arena a = Arena.ofAuto();
            MemorySegment all = a.allocate(SIZE * Math.max(count, 1), 4);
            RPenEvent[] items = new RPenEvent[count];
            for (int i = 0; i < count; i++) items[i] = new RPenEvent(all.asSlice(SIZE * i, SIZE), a);
            return items;
        }

        private Arena arena() {
            if (arena == null) arena = Arena.ofAuto();
            return arena;
        }

        public int phase() { return s.get(JAVA_INT, 0); }
        public RPenEvent phase(int value) { s.set(JAVA_INT, 0, value); return this; }

        public int window() { return s.get(JAVA_INT, 4); }
        public RPenEvent window(int value) { s.set(JAVA_INT, 4, value); return this; }

        /**
         * Widget under the pen; during a stroke, the one it started on.
         */
        public int target() { return s.get(JAVA_INT, 8); }
        public RPenEvent target(int value) { s.set(JAVA_INT, 8, value); return this; }

        public float x() { return s.get(JAVA_FLOAT, 12); }
        public RPenEvent x(float value) { s.set(JAVA_FLOAT, 12, value); return this; }

        public float y() { return s.get(JAVA_FLOAT, 16); }
        public RPenEvent y(float value) { s.set(JAVA_FLOAT, 16, value); return this; }

        public float local_x() { return s.get(JAVA_FLOAT, 20); }
        public RPenEvent local_x(float value) { s.set(JAVA_FLOAT, 20, value); return this; }

        public float local_y() { return s.get(JAVA_FLOAT, 24); }
        public RPenEvent local_y(float value) { s.set(JAVA_FLOAT, 24, value); return this; }

        /**
         * 0..1, after the pressure curve; 0 while hovering.
         */
        public float pressure() { return s.get(JAVA_FLOAT, 28); }
        public RPenEvent pressure(float value) { s.set(JAVA_FLOAT, 28, value); return this; }

        /**
         * As the pen gives it, 0..1.
         */
        public float raw_pressure() { return s.get(JAVA_FLOAT, 32); }
        public RPenEvent raw_pressure(float value) { s.set(JAVA_FLOAT, 32, value); return this; }

        /**
         * Tilt of the pen in degrees, -90..90, toward the right and toward the user; 0 when upright or unknown.
         */
        public float tilt_x() { return s.get(JAVA_FLOAT, 36); }
        public RPenEvent tilt_x(float value) { s.set(JAVA_FLOAT, 36, value); return this; }

        /**
         * Tilt of the pen in degrees, -90..90, toward the right and toward the user; 0 when upright or unknown.
         */
        public float tilt_y() { return s.get(JAVA_FLOAT, 40); }
        public RPenEvent tilt_y(float value) { s.set(JAVA_FLOAT, 40, value); return this; }

        /**
         * Rotation around its axis in degrees, 0..360 (when the pen tells).
         */
        public float rotation() { return s.get(JAVA_FLOAT, 44); }
        public RPenEvent rotation(float value) { s.set(JAVA_FLOAT, 44, value); return this; }

        /**
         * R_TRUE when the eraser end touches (or the eraser button is held).
         */
        public int eraser() { return s.get(JAVA_INT, 48); }
        public RPenEvent eraser(int value) { s.set(JAVA_INT, 48, value); return this; }

        /**
         * Bitmask of RPenButton.
         */
        public int buttons() { return s.get(JAVA_INT, 52); }
        public RPenEvent buttons(int value) { s.set(JAVA_INT, 52, value); return this; }

        public int modifiers() { return s.get(JAVA_INT, 56); }
        public RPenEvent modifiers(int value) { s.set(JAVA_INT, 56, value); return this; }
    }

    public static final class RGestureEvent {
        /** The size of the structure in C, in bytes. */
        public static final long SIZE = 48;
        private final MemorySegment s;
        private Arena arena;

        /** A new structure, zeroed (what `RGestureEvent x = {0};` is in C). */
        public RGestureEvent() {
            arena = Arena.ofAuto();
            s = arena.allocate(SIZE, 4);
        }

        /** The structure at an address (given by the library, or a slice of an array). */
        public RGestureEvent(MemorySegment address) {
            s = address.reinterpret(SIZE);
        }

        private RGestureEvent(MemorySegment segment, Arena arena) {
            this.s = segment;
            this.arena = arena;
        }

        /** Its memory: what to give where C takes a pointer to it. */
        public MemorySegment segment() { return s; }

        /** `count` structures side by side, zeroed: an array of C. */
        public static RGestureEvent[] array(int count) {
            Arena a = Arena.ofAuto();
            MemorySegment all = a.allocate(SIZE * Math.max(count, 1), 4);
            RGestureEvent[] items = new RGestureEvent[count];
            for (int i = 0; i < count; i++) items[i] = new RGestureEvent(all.asSlice(SIZE * i, SIZE), a);
            return items;
        }

        private Arena arena() {
            if (arena == null) arena = Arena.ofAuto();
            return arena;
        }

        public int phase() { return s.get(JAVA_INT, 0); }
        public RGestureEvent phase(int value) { s.set(JAVA_INT, 0, value); return this; }

        public int window() { return s.get(JAVA_INT, 4); }
        public RGestureEvent window(int value) { s.set(JAVA_INT, 4, value); return this; }

        /**
         * Widget under the center of the fingers when the gesture began.
         */
        public int target() { return s.get(JAVA_INT, 8); }
        public RGestureEvent target(int value) { s.set(JAVA_INT, 8, value); return this; }

        /**
         * Center of the fingers, in the window and relative to the widget receiving the event.
         */
        public float x() { return s.get(JAVA_FLOAT, 12); }
        public RGestureEvent x(float value) { s.set(JAVA_FLOAT, 12, value); return this; }

        /**
         * Center of the fingers, in the window and relative to the widget receiving the event.
         */
        public float y() { return s.get(JAVA_FLOAT, 16); }
        public RGestureEvent y(float value) { s.set(JAVA_FLOAT, 16, value); return this; }

        public float local_x() { return s.get(JAVA_FLOAT, 20); }
        public RGestureEvent local_x(float value) { s.set(JAVA_FLOAT, 20, value); return this; }

        public float local_y() { return s.get(JAVA_FLOAT, 24); }
        public RGestureEvent local_y(float value) { s.set(JAVA_FLOAT, 24, value); return this; }

        /**
         * Distance between the fingers relative to when the gesture began (2 = twice as far apart).
         */
        public float scale() { return s.get(JAVA_FLOAT, 28); }
        public RGestureEvent scale(float value) { s.set(JAVA_FLOAT, 28, value); return this; }

        /**
         * The same since the previous event: what a zoom multiplies by.
         */
        public float scale_delta() { return s.get(JAVA_FLOAT, 32); }
        public RGestureEvent scale_delta(float value) { s.set(JAVA_FLOAT, 32, value); return this; }

        /**
         * Movement of the center since the previous event.
         */
        public float pan_x() { return s.get(JAVA_FLOAT, 36); }
        public RGestureEvent pan_x(float value) { s.set(JAVA_FLOAT, 36, value); return this; }

        /**
         * Movement of the center since the previous event.
         */
        public float pan_y() { return s.get(JAVA_FLOAT, 40); }
        public RGestureEvent pan_y(float value) { s.set(JAVA_FLOAT, 40, value); return this; }

        /**
         * Turn of the fingers since the gesture began, in degrees, clockwise.
         */
        public float rotation() { return s.get(JAVA_FLOAT, 44); }
        public RGestureEvent rotation(float value) { s.set(JAVA_FLOAT, 44, value); return this; }
    }

    /**
     * What a tile shows, filled by the application when the tile is drawn.
     */
    public static final class RTileItem {
        /** The size of the structure in C, in bytes. */
        public static final long SIZE = 80;
        private final MemorySegment s;
        private Arena arena;

        /** A new structure, zeroed (what `RTileItem x = {0};` is in C). */
        public RTileItem() {
            arena = Arena.ofAuto();
            s = arena.allocate(SIZE, 8);
        }

        /** The structure at an address (given by the library, or a slice of an array). */
        public RTileItem(MemorySegment address) {
            s = address.reinterpret(SIZE);
        }

        private RTileItem(MemorySegment segment, Arena arena) {
            this.s = segment;
            this.arena = arena;
        }

        /** Its memory: what to give where C takes a pointer to it. */
        public MemorySegment segment() { return s; }

        /** `count` structures side by side, zeroed: an array of C. */
        public static RTileItem[] array(int count) {
            Arena a = Arena.ofAuto();
            MemorySegment all = a.allocate(SIZE * Math.max(count, 1), 8);
            RTileItem[] items = new RTileItem[count];
            for (int i = 0; i < count; i++) items[i] = new RTileItem(all.asSlice(SIZE * i, SIZE), a);
            return items;
        }

        private Arena arena() {
            if (arena == null) arena = Arena.ofAuto();
            return arena;
        }

        /**
         * Name under the icon (valid until the callback returns).
         */
        public String text() { return R.text(s.get(ADDRESS, 0)); }
        public RTileItem text(String value) { s.set(ADDRESS, 0, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        /**
         * Second line, dimmer, or NULL.
         */
        public String detail() { return R.text(s.get(ADDRESS, 8)); }
        public RTileItem detail(String value) { s.set(ADDRESS, 8, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        /**
         * Small text at the top right of the tile, or NULL.
         */
        public String badge() { return R.text(s.get(ADDRESS, 16)); }
        public RTileItem badge(String value) { s.set(ADDRESS, 16, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        /**
         * Icon texture (R_INVALID_ID: none), the region of it and its tint (white by default).
         */
        public int icon() { return s.get(JAVA_INT, 24); }
        public RTileItem icon(int value) { s.set(JAVA_INT, 24, value); return this; }

        public float uv(int index) { return s.getAtIndex(JAVA_FLOAT, 28 / 4 + java.util.Objects.checkIndex(index, 4)); }
        public RTileItem uv(int index, float value) { s.setAtIndex(JAVA_FLOAT, 28 / 4 + java.util.Objects.checkIndex(index, 4), value); return this; }

        public float tint(int index) { return s.getAtIndex(JAVA_FLOAT, 44 / 4 + java.util.Objects.checkIndex(index, 4)); }
        public RTileItem tint(int index, float value) { s.setAtIndex(JAVA_FLOAT, 44 / 4 + java.util.Objects.checkIndex(index, 4), value); return this; }

        public float icon_radius() { return s.get(JAVA_FLOAT, 60); }
        public RTileItem icon_radius(float value) { s.set(JAVA_FLOAT, 60, value); return this; }

        /**
         * Color of the name; alpha 0 = the theme.
         */
        public float text_color(int index) { return s.getAtIndex(JAVA_FLOAT, 64 / 4 + java.util.Objects.checkIndex(index, 4)); }
        public RTileItem text_color(int index, float value) { s.setAtIndex(JAVA_FLOAT, 64 / 4 + java.util.Objects.checkIndex(index, 4), value); return this; }
    }

    /**
     * What the editor knows of a language. Every field but the name may be NULL.
     * Lists of words are separated by spaces.
     */
    public static final class RLanguageDesc {
        /** The size of the structure in C, in bytes. */
        public static final long SIZE = 112;
        private final MemorySegment s;
        private Arena arena;

        /** A new structure, zeroed (what `RLanguageDesc x = {0};` is in C). */
        public RLanguageDesc() {
            arena = Arena.ofAuto();
            s = arena.allocate(SIZE, 8);
        }

        /** The structure at an address (given by the library, or a slice of an array). */
        public RLanguageDesc(MemorySegment address) {
            s = address.reinterpret(SIZE);
        }

        private RLanguageDesc(MemorySegment segment, Arena arena) {
            this.s = segment;
            this.arena = arena;
        }

        /** Its memory: what to give where C takes a pointer to it. */
        public MemorySegment segment() { return s; }

        /** `count` structures side by side, zeroed: an array of C. */
        public static RLanguageDesc[] array(int count) {
            Arena a = Arena.ofAuto();
            MemorySegment all = a.allocate(SIZE * Math.max(count, 1), 8);
            RLanguageDesc[] items = new RLanguageDesc[count];
            for (int i = 0; i < count; i++) items[i] = new RLanguageDesc(all.asSlice(SIZE * i, SIZE), a);
            return items;
        }

        private Arena arena() {
            if (arena == null) arena = Arena.ofAuto();
            return arena;
        }

        /**
         * "Rust". Compared without case.
         */
        public String name() { return R.text(s.get(ADDRESS, 0)); }
        public RLanguageDesc name(String value) { s.set(ADDRESS, 0, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        /**
         * Extensions of its files, separated by ';': "rs".
         */
        public String extensions() { return R.text(s.get(ADDRESS, 8)); }
        public RLanguageDesc extensions(String value) { s.set(ADDRESS, 8, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        /**
         * Words of the language itself: if, while, return...
         */
        public String keywords() { return R.text(s.get(ADDRESS, 16)); }
        public RLanguageDesc keywords(String value) { s.set(ADDRESS, 16, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        /**
         * Names of types and of what is always there: int, String, print...
         */
        public String types() { return R.text(s.get(ADDRESS, 24)); }
        public RLanguageDesc types(String value) { s.set(ADDRESS, 24, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        /**
         * Values with a name: true, false, null...
         */
        public String constants() { return R.text(s.get(ADDRESS, 32)); }
        public RLanguageDesc constants(String value) { s.set(ADDRESS, 32, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        /**
         * What starts a comment that ends with its line: two slashes, a hash.
         */
        public String line_comment() { return R.text(s.get(ADDRESS, 40)); }
        public RLanguageDesc line_comment(String value) { s.set(ADDRESS, 40, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        /**
         * What opens and closes a comment that may span lines.
         */
        public String block_comment_start() { return R.text(s.get(ADDRESS, 48)); }
        public RLanguageDesc block_comment_start(String value) { s.set(ADDRESS, 48, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        public String block_comment_end() { return R.text(s.get(ADDRESS, 56)); }
        public RLanguageDesc block_comment_end(String value) { s.set(ADDRESS, 56, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        /**
         * Characters that open and close a text on one line: a double quote, a single quote.
         */
        public String string_quotes() { return R.text(s.get(ADDRESS, 64)); }
        public RLanguageDesc string_quotes(String value) { s.set(ADDRESS, 64, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        /**
         * Marks of the texts that span lines, separated by spaces: three double
         * quotes, a backquote. When what closes such a text is not what opens it,
         * write both around three dots: R"(...)" . Nothing is escaped in those.
         */
        public String multiline_strings() { return R.text(s.get(ADDRESS, 72)); }
        public RLanguageDesc multiline_strings(String value) { s.set(ADDRESS, 72, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        /**
         * What starts a line for the preprocessor: "#".
         */
        public String directive() { return R.text(s.get(ADDRESS, 80)); }
        public RLanguageDesc directive(String value) { s.set(ADDRESS, 80, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        /**
         * What starts an annotation: "&#64;" (&#64;Override), or "#[" up to its closing bracket.
         */
        public String annotation() { return R.text(s.get(ADDRESS, 88)); }
        public RLanguageDesc annotation(String value) { s.set(ADDRESS, 88, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        /**
         * Bitmask of RLanguageFlags.
         */
        public int flags() { return s.get(JAVA_INT, 96); }
        public RLanguageDesc flags(int value) { s.set(JAVA_INT, 96, value); return this; }

        /**
         * Markup with R_LANGUAGE_EMBEDS: the language of what is between
         * &lt;script&gt; and &lt;/script&gt; (the name of a language: "C", "Rust"...).
         * NULL: JavaScript, as in HTML.
         */
        public String script_language() { return R.text(s.get(ADDRESS, 104)); }
        public RLanguageDesc script_language(String value) { s.set(ADDRESS, 104, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }
    }

    /**
     * A kind of file in a file dialog.
     */
    public static final class RFileFilter {
        /** The size of the structure in C, in bytes. */
        public static final long SIZE = 16;
        private final MemorySegment s;
        private Arena arena;

        /** A new structure, zeroed (what `RFileFilter x = {0};` is in C). */
        public RFileFilter() {
            arena = Arena.ofAuto();
            s = arena.allocate(SIZE, 8);
        }

        /** The structure at an address (given by the library, or a slice of an array). */
        public RFileFilter(MemorySegment address) {
            s = address.reinterpret(SIZE);
        }

        private RFileFilter(MemorySegment segment, Arena arena) {
            this.s = segment;
            this.arena = arena;
        }

        /** Its memory: what to give where C takes a pointer to it. */
        public MemorySegment segment() { return s; }

        /** `count` structures side by side, zeroed: an array of C. */
        public static RFileFilter[] array(int count) {
            Arena a = Arena.ofAuto();
            MemorySegment all = a.allocate(SIZE * Math.max(count, 1), 8);
            RFileFilter[] items = new RFileFilter[count];
            for (int i = 0; i < count; i++) items[i] = new RFileFilter(all.asSlice(SIZE * i, SIZE), a);
            return items;
        }

        private Arena arena() {
            if (arena == null) arena = Arena.ofAuto();
            return arena;
        }

        /**
         * Shown to the user: "Images".
         */
        public String name() { return R.text(s.get(ADDRESS, 0)); }
        public RFileFilter name(String value) { s.set(ADDRESS, 0, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        /**
         * Patterns separated by ';': "*.png;*.jpg". "*" matches everything.
         */
        public String patterns() { return R.text(s.get(ADDRESS, 8)); }
        public RFileFilter patterns(String value) { s.set(ADDRESS, 8, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }
    }

    /**
     * Every field may be left to zero.
     */
    public static final class RFileDialogDesc {
        /** The size of the structure in C, in bytes. */
        public static final long SIZE = 64;
        private final MemorySegment s;
        private Arena arena;

        /** A new structure, zeroed (what `RFileDialogDesc x = {0};` is in C). */
        public RFileDialogDesc() {
            arena = Arena.ofAuto();
            s = arena.allocate(SIZE, 8);
        }

        /** The structure at an address (given by the library, or a slice of an array). */
        public RFileDialogDesc(MemorySegment address) {
            s = address.reinterpret(SIZE);
        }

        private RFileDialogDesc(MemorySegment segment, Arena arena) {
            this.s = segment;
            this.arena = arena;
        }

        /** Its memory: what to give where C takes a pointer to it. */
        public MemorySegment segment() { return s; }

        /** `count` structures side by side, zeroed: an array of C. */
        public static RFileDialogDesc[] array(int count) {
            Arena a = Arena.ofAuto();
            MemorySegment all = a.allocate(SIZE * Math.max(count, 1), 8);
            RFileDialogDesc[] items = new RFileDialogDesc[count];
            for (int i = 0; i < count; i++) items[i] = new RFileDialogDesc(all.asSlice(SIZE * i, SIZE), a);
            return items;
        }

        private Arena arena() {
            if (arena == null) arena = Arena.ofAuto();
            return arena;
        }

        /**
         * Window the dialog belongs to. R_INVALID_ID: none (a window is opened when Raven needs one).
         */
        public int parent() { return s.get(JAVA_INT, 0); }
        public RFileDialogDesc parent(int value) { s.set(JAVA_INT, 0, value); return this; }

        /**
         * NULL: "Open", "Save as"...
         */
        public String title() { return R.text(s.get(ADDRESS, 8)); }
        public RFileDialogDesc title(String value) { s.set(ADDRESS, 8, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        /**
         * Folder shown first. NULL: the one of the previous dialog, else the home of the user.
         */
        public String directory() { return R.text(s.get(ADDRESS, 16)); }
        public RFileDialogDesc directory(String value) { s.set(ADDRESS, 16, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        /**
         * Name proposed to the user (save dialogs).
         */
        public String file_name() { return R.text(s.get(ADDRESS, 24)); }
        public RFileDialogDesc file_name(String value) { s.set(ADDRESS, 24, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        public MemorySegment filters() { return s.get(ADDRESS, 32); }
        public RFileDialogDesc filters(MemorySegment value) { s.set(ADDRESS, 32, value == null ? MemorySegment.NULL : value); return this; }

        public int filter_count() { return s.get(JAVA_INT, 40); }
        public RFileDialogDesc filter_count(int value) { s.set(JAVA_INT, 40, value); return this; }

        /**
         * Save dialogs: added to a name typed without extension ("xml").
         */
        public String default_extension() { return R.text(s.get(ADDRESS, 48)); }
        public RFileDialogDesc default_extension(String value) { s.set(ADDRESS, 48, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        /**
         * Bitmask of RFileDialogFlags.
         */
        public int flags() { return s.get(JAVA_INT, 56); }
        public RFileDialogDesc flags(int value) { s.set(JAVA_INT, 56, value); return this; }
    }

    /**
     * Passed to the draw callback of a viewport. Sizes are in pixels.
     */
    public static final class RViewportDrawInfo {
        /** The size of the structure in C, in bytes. */
        public static final long SIZE = 88;
        private final MemorySegment s;
        private Arena arena;

        /** A new structure, zeroed (what `RViewportDrawInfo x = {0};` is in C). */
        public RViewportDrawInfo() {
            arena = Arena.ofAuto();
            s = arena.allocate(SIZE, 8);
        }

        /** The structure at an address (given by the library, or a slice of an array). */
        public RViewportDrawInfo(MemorySegment address) {
            s = address.reinterpret(SIZE);
        }

        private RViewportDrawInfo(MemorySegment segment, Arena arena) {
            this.s = segment;
            this.arena = arena;
        }

        /** Its memory: what to give where C takes a pointer to it. */
        public MemorySegment segment() { return s; }

        /** `count` structures side by side, zeroed: an array of C. */
        public static RViewportDrawInfo[] array(int count) {
            Arena a = Arena.ofAuto();
            MemorySegment all = a.allocate(SIZE * Math.max(count, 1), 8);
            RViewportDrawInfo[] items = new RViewportDrawInfo[count];
            for (int i = 0; i < count; i++) items[i] = new RViewportDrawInfo(all.asSlice(SIZE * i, SIZE), a);
            return items;
        }

        private Arena arena() {
            if (arena == null) arena = Arena.ofAuto();
            return arena;
        }

        public int backend() { return s.get(JAVA_INT, 0); }
        public RViewportDrawInfo backend(int value) { s.set(JAVA_INT, 0, value); return this; }

        public int viewport() { return s.get(JAVA_INT, 4); }
        public RViewportDrawInfo viewport(int value) { s.set(JAVA_INT, 4, value); return this; }

        public int window() { return s.get(JAVA_INT, 8); }
        public RViewportDrawInfo window(int value) { s.set(JAVA_INT, 8, value); return this; }

        /**
         * Area of the viewport in the window framebuffer.
         */
        public int x() { return s.get(JAVA_INT, 12); }
        public RViewportDrawInfo x(int value) { s.set(JAVA_INT, 12, value); return this; }

        /**
         * Area of the viewport in the window framebuffer.
         */
        public int y() { return s.get(JAVA_INT, 16); }
        public RViewportDrawInfo y(int value) { s.set(JAVA_INT, 16, value); return this; }

        /**
         * Area of the viewport in the window framebuffer.
         */
        public int width() { return s.get(JAVA_INT, 20); }
        public RViewportDrawInfo width(int value) { s.set(JAVA_INT, 20, value); return this; }

        /**
         * Area of the viewport in the window framebuffer.
         */
        public int height() { return s.get(JAVA_INT, 24); }
        public RViewportDrawInfo height(int value) { s.set(JAVA_INT, 24, value); return this; }

        /**
         * Visible part of that area. Already set as the scissor rectangle.
         */
        public int clip_x() { return s.get(JAVA_INT, 28); }
        public RViewportDrawInfo clip_x(int value) { s.set(JAVA_INT, 28, value); return this; }

        /**
         * Visible part of that area. Already set as the scissor rectangle.
         */
        public int clip_y() { return s.get(JAVA_INT, 32); }
        public RViewportDrawInfo clip_y(int value) { s.set(JAVA_INT, 32, value); return this; }

        /**
         * Visible part of that area. Already set as the scissor rectangle.
         */
        public int clip_width() { return s.get(JAVA_INT, 36); }
        public RViewportDrawInfo clip_width(int value) { s.set(JAVA_INT, 36, value); return this; }

        /**
         * Visible part of that area. Already set as the scissor rectangle.
         */
        public int clip_height() { return s.get(JAVA_INT, 40); }
        public RViewportDrawInfo clip_height(int value) { s.set(JAVA_INT, 40, value); return this; }

        public int framebuffer_width() { return s.get(JAVA_INT, 44); }
        public RViewportDrawInfo framebuffer_width(int value) { s.set(JAVA_INT, 44, value); return this; }

        public int framebuffer_height() { return s.get(JAVA_INT, 48); }
        public RViewportDrawInfo framebuffer_height(int value) { s.set(JAVA_INT, 48, value); return this; }

        public long frame_index() { return s.get(JAVA_LONG, 56); }
        public RViewportDrawInfo frame_index(long value) { s.set(JAVA_LONG, 56, value); return this; }

        /**
         * Vulkan: VkCommandBuffer being recorded, inside the render pass.
         */
        public MemorySegment command_buffer() { return s.get(ADDRESS, 64); }
        public RViewportDrawInfo command_buffer(MemorySegment value) { s.set(ADDRESS, 64, value == null ? MemorySegment.NULL : value); return this; }

        /**
         * Vulkan: VkRenderPass in use (one color attachment, no depth).
         */
        public MemorySegment render_pass() { return s.get(ADDRESS, 72); }
        public RViewportDrawInfo render_pass(MemorySegment value) { s.set(ADDRESS, 72, value == null ? MemorySegment.NULL : value); return this; }

        /**
         * Vulkan: VkFormat of the color attachment.
         */
        public int color_format() { return s.get(JAVA_INT, 80); }
        public RViewportDrawInfo color_format(int value) { s.set(JAVA_INT, 80, value); return this; }
    }

    /**
     * State of one animated value. Zero-initialize it and keep it with the widget.
     */
    public static final class RSpring {
        /** The size of the structure in C, in bytes. */
        public static final long SIZE = 12;
        private final MemorySegment s;
        private Arena arena;

        /** A new structure, zeroed (what `RSpring x = {0};` is in C). */
        public RSpring() {
            arena = Arena.ofAuto();
            s = arena.allocate(SIZE, 4);
        }

        /** The structure at an address (given by the library, or a slice of an array). */
        public RSpring(MemorySegment address) {
            s = address.reinterpret(SIZE);
        }

        private RSpring(MemorySegment segment, Arena arena) {
            this.s = segment;
            this.arena = arena;
        }

        /** Its memory: what to give where C takes a pointer to it. */
        public MemorySegment segment() { return s; }

        /** `count` structures side by side, zeroed: an array of C. */
        public static RSpring[] array(int count) {
            Arena a = Arena.ofAuto();
            MemorySegment all = a.allocate(SIZE * Math.max(count, 1), 4);
            RSpring[] items = new RSpring[count];
            for (int i = 0; i < count; i++) items[i] = new RSpring(all.asSlice(SIZE * i, SIZE), a);
            return items;
        }

        private Arena arena() {
            if (arena == null) arena = Arena.ofAuto();
            return arena;
        }

        public float value() { return s.get(JAVA_FLOAT, 0); }
        public RSpring value(float value) { s.set(JAVA_FLOAT, 0, value); return this; }

        public float velocity() { return s.get(JAVA_FLOAT, 4); }
        public RSpring velocity(float value) { s.set(JAVA_FLOAT, 4, value); return this; }

        public int started() { return s.get(JAVA_INT, 8); }
        public RSpring started(int value) { s.set(JAVA_INT, 8, value); return this; }
    }

    /**
     * How an effect is drawn. Start from Rshader_look_init.
     */
    public static final class RShaderLook {
        /** The size of the structure in C, in bytes. */
        public static final long SIZE = 72;
        private final MemorySegment s;
        private Arena arena;

        /** A new structure, zeroed (what `RShaderLook x = {0};` is in C). */
        public RShaderLook() {
            arena = Arena.ofAuto();
            s = arena.allocate(SIZE, 4);
        }

        /** The structure at an address (given by the library, or a slice of an array). */
        public RShaderLook(MemorySegment address) {
            s = address.reinterpret(SIZE);
        }

        private RShaderLook(MemorySegment segment, Arena arena) {
            this.s = segment;
            this.arena = arena;
        }

        /** Its memory: what to give where C takes a pointer to it. */
        public MemorySegment segment() { return s; }

        /** `count` structures side by side, zeroed: an array of C. */
        public static RShaderLook[] array(int count) {
            Arena a = Arena.ofAuto();
            MemorySegment all = a.allocate(SIZE * Math.max(count, 1), 4);
            RShaderLook[] items = new RShaderLook[count];
            for (int i = 0; i < count; i++) items[i] = new RShaderLook(all.asSlice(SIZE * i, SIZE), a);
            return items;
        }

        private Arena arena() {
            if (arena == null) arena = Arena.ofAuto();
            return arena;
        }

        /**
         * First and second color, 0..1, straight alpha.
         */
        public float color(int index) { return s.getAtIndex(JAVA_FLOAT, 0 / 4 + java.util.Objects.checkIndex(index, 4)); }
        public RShaderLook color(int index, float value) { s.setAtIndex(JAVA_FLOAT, 0 / 4 + java.util.Objects.checkIndex(index, 4), value); return this; }

        public float color2(int index) { return s.getAtIndex(JAVA_FLOAT, 16 / 4 + java.util.Objects.checkIndex(index, 4)); }
        public RShaderLook color2(int index, float value) { s.setAtIndex(JAVA_FLOAT, 16 / 4 + java.util.Objects.checkIndex(index, 4), value); return this; }

        /**
         * Four numbers the effect reads as fx.params (their meaning is the effect's).
         */
        public float params(int index) { return s.getAtIndex(JAVA_FLOAT, 32 / 4 + java.util.Objects.checkIndex(index, 4)); }
        public RShaderLook params(int index, float value) { s.setAtIndex(JAVA_FLOAT, 32 / 4 + java.util.Objects.checkIndex(index, 4), value); return this; }

        /**
         * Radius of the corners, in logical units.
         */
        public float radius() { return s.get(JAVA_FLOAT, 48); }
        public RShaderLook radius(float value) { s.set(JAVA_FLOAT, 48, value); return this; }

        /**
         * Texture the effect reads with raven_texture(), or R_INVALID_ID.
         */
        public int texture() { return s.get(JAVA_INT, 52); }
        public RShaderLook texture(int value) { s.set(JAVA_INT, 52, value); return this; }

        /**
         * -1: animated when the shader is (R_SHADER_ANIMATED); R_TRUE / R_FALSE forces it.
         */
        public int animate() { return s.get(JAVA_INT, 56); }
        public RShaderLook animate(int value) { s.set(JAVA_INT, 56, value); return this; }

        /**
         * R_TRUE (the default): the effect knows where the pointer is (fx.pointer, fx.hover, fx.press).
         */
        public int pointer() { return s.get(JAVA_INT, 60); }
        public RShaderLook pointer(int value) { s.set(JAVA_INT, 60, value); return this; }

        /**
         * How much the pointer is over it and how much it is pressed, 0..1, as
         * the effect reads them (fx.hover, fx.press). -1 (the default): 1 while
         * the pointer is in the rectangle, and while the left button is down
         * there; a Border eases them itself.
         */
        public float hover() { return s.get(JAVA_FLOAT, 64); }
        public RShaderLook hover(float value) { s.set(JAVA_FLOAT, 64, value); return this; }

        public float press() { return s.get(JAVA_FLOAT, 68); }
        public RShaderLook press(float value) { s.set(JAVA_FLOAT, 68, value); return this; }
    }

    /**
     * Where and why a text could not be read (Rxml_document_parse).
     */
    public static final class RXmlError {
        /** The size of the structure in C, in bytes. */
        public static final long SIZE = 168;
        private final MemorySegment s;
        private Arena arena;

        /** A new structure, zeroed (what `RXmlError x = {0};` is in C). */
        public RXmlError() {
            arena = Arena.ofAuto();
            s = arena.allocate(SIZE, 4);
        }

        /** The structure at an address (given by the library, or a slice of an array). */
        public RXmlError(MemorySegment address) {
            s = address.reinterpret(SIZE);
        }

        private RXmlError(MemorySegment segment, Arena arena) {
            this.s = segment;
            this.arena = arena;
        }

        /** Its memory: what to give where C takes a pointer to it. */
        public MemorySegment segment() { return s; }

        /** `count` structures side by side, zeroed: an array of C. */
        public static RXmlError[] array(int count) {
            Arena a = Arena.ofAuto();
            MemorySegment all = a.allocate(SIZE * Math.max(count, 1), 4);
            RXmlError[] items = new RXmlError[count];
            for (int i = 0; i < count; i++) items[i] = new RXmlError(all.asSlice(SIZE * i, SIZE), a);
            return items;
        }

        private Arena arena() {
            if (arena == null) arena = Arena.ofAuto();
            return arena;
        }

        /**
         * Both start at 1.
         */
        public int line() { return s.get(JAVA_INT, 0); }
        public RXmlError line(int value) { s.set(JAVA_INT, 0, value); return this; }

        public int column() { return s.get(JAVA_INT, 4); }
        public RXmlError column(int value) { s.set(JAVA_INT, 4, value); return this; }

        public String message() { return R.fixedText(s.asSlice(8, 160)); }
    }

    /**
     * One attribute of a tag (Rxml_get_attribute_info). The strings belong to the library.
     */
    public static final class RXmlAttributeInfo {
        /** The size of the structure in C, in bytes. */
        public static final long SIZE = 40;
        private final MemorySegment s;
        private Arena arena;

        /** A new structure, zeroed (what `RXmlAttributeInfo x = {0};` is in C). */
        public RXmlAttributeInfo() {
            arena = Arena.ofAuto();
            s = arena.allocate(SIZE, 8);
        }

        /** The structure at an address (given by the library, or a slice of an array). */
        public RXmlAttributeInfo(MemorySegment address) {
            s = address.reinterpret(SIZE);
        }

        private RXmlAttributeInfo(MemorySegment segment, Arena arena) {
            this.s = segment;
            this.arena = arena;
        }

        /** Its memory: what to give where C takes a pointer to it. */
        public MemorySegment segment() { return s; }

        /** `count` structures side by side, zeroed: an array of C. */
        public static RXmlAttributeInfo[] array(int count) {
            Arena a = Arena.ofAuto();
            MemorySegment all = a.allocate(SIZE * Math.max(count, 1), 8);
            RXmlAttributeInfo[] items = new RXmlAttributeInfo[count];
            for (int i = 0; i < count; i++) items[i] = new RXmlAttributeInfo(all.asSlice(SIZE * i, SIZE), a);
            return items;
        }

        private Arena arena() {
            if (arena == null) arena = Arena.ofAuto();
            return arena;
        }

        public String name() { return R.text(s.get(ADDRESS, 0)); }
        public RXmlAttributeInfo name(String value) { s.set(ADDRESS, 0, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        public int type() { return s.get(JAVA_INT, 8); }
        public RXmlAttributeInfo type(int value) { s.set(JAVA_INT, 8, value); return this; }

        /**
         * R_XML_CHOICE: the words, separated by '|'. NULL otherwise.
         */
        public String choices() { return R.text(s.get(ADDRESS, 16)); }
        public RXmlAttributeInfo choices(String value) { s.set(ADDRESS, 16, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        /**
         * What it belongs to: the tag itself ("Button"), "Widget" for what every widget has, "Slot" for the place in the parent.
         */
        public String group() { return R.text(s.get(ADDRESS, 24)); }
        public RXmlAttributeInfo group(String value) { s.set(ADDRESS, 24, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }

        /**
         * One line saying what it does.
         */
        public String description() { return R.text(s.get(ADDRESS, 32)); }
        public RXmlAttributeInfo description(String value) { s.set(ADDRESS, 32, value == null ? MemorySegment.NULL : R.utf8(arena(), value)); return this; }
    }

    /**
     * What the system asks for.
     */
    public static final class RAccessibilitySettings {
        /** The size of the structure in C, in bytes. */
        public static final long SIZE = 16;
        private final MemorySegment s;
        private Arena arena;

        /** A new structure, zeroed (what `RAccessibilitySettings x = {0};` is in C). */
        public RAccessibilitySettings() {
            arena = Arena.ofAuto();
            s = arena.allocate(SIZE, 4);
        }

        /** The structure at an address (given by the library, or a slice of an array). */
        public RAccessibilitySettings(MemorySegment address) {
            s = address.reinterpret(SIZE);
        }

        private RAccessibilitySettings(MemorySegment segment, Arena arena) {
            this.s = segment;
            this.arena = arena;
        }

        /** Its memory: what to give where C takes a pointer to it. */
        public MemorySegment segment() { return s; }

        /** `count` structures side by side, zeroed: an array of C. */
        public static RAccessibilitySettings[] array(int count) {
            Arena a = Arena.ofAuto();
            MemorySegment all = a.allocate(SIZE * Math.max(count, 1), 4);
            RAccessibilitySettings[] items = new RAccessibilitySettings[count];
            for (int i = 0; i < count; i++) items[i] = new RAccessibilitySettings(all.asSlice(SIZE * i, SIZE), a);
            return items;
        }

        private Arena arena() {
            if (arena == null) arena = Arena.ofAuto();
            return arena;
        }

        /**
         * Size of the texts asked by the user (1 = normal).
         */
        public float text_scale() { return s.get(JAVA_FLOAT, 0); }
        public RAccessibilitySettings text_scale(float value) { s.set(JAVA_FLOAT, 0, value); return this; }

        public int high_contrast() { return s.get(JAVA_INT, 4); }
        public RAccessibilitySettings high_contrast(int value) { s.set(JAVA_INT, 4, value); return this; }

        public int reduce_motion() { return s.get(JAVA_INT, 8); }
        public RAccessibilitySettings reduce_motion(int value) { s.set(JAVA_INT, 8, value); return this; }

        /**
         * R_TRUE when a screen reader is running.
         */
        public int screen_reader() { return s.get(JAVA_INT, 12); }
        public RAccessibilitySettings screen_reader(int value) { s.set(JAVA_INT, 12, value); return this; }
    }

    public static final class RHostPlatformDesc {
        /** The size of the structure in C, in bytes. */
        public static final long SIZE = 40;
        private final MemorySegment s;
        private Arena arena;

        /** A new structure, zeroed (what `RHostPlatformDesc x = {0};` is in C). */
        public RHostPlatformDesc() {
            arena = Arena.ofAuto();
            s = arena.allocate(SIZE, 8);
        }

        /** The structure at an address (given by the library, or a slice of an array). */
        public RHostPlatformDesc(MemorySegment address) {
            s = address.reinterpret(SIZE);
        }

        private RHostPlatformDesc(MemorySegment segment, Arena arena) {
            this.s = segment;
            this.arena = arena;
        }

        /** Its memory: what to give where C takes a pointer to it. */
        public MemorySegment segment() { return s; }

        /** `count` structures side by side, zeroed: an array of C. */
        public static RHostPlatformDesc[] array(int count) {
            Arena a = Arena.ofAuto();
            MemorySegment all = a.allocate(SIZE * Math.max(count, 1), 8);
            RHostPlatformDesc[] items = new RHostPlatformDesc[count];
            for (int i = 0; i < count; i++) items[i] = new RHostPlatformDesc(all.asSlice(SIZE * i, SIZE), a);
            return items;
        }

        private Arena arena() {
            if (arena == null) arena = Arena.ofAuto();
            return arena;
        }

        /**
         * The pointer over `window` should look like `cursor`. May be NULL.
         */
        public MemorySegment set_cursor() { return s.get(ADDRESS, 0); }
        public RHostPlatformDesc set_cursor(MemorySegment value) { s.set(ADDRESS, 0, value == null ? MemorySegment.NULL : value); return this; }

        /**
         * Copies UTF-8 text to the clipboard of the application. NULL: the clipboard stays in the process.
         */
        public MemorySegment clipboard_set() { return s.get(ADDRESS, 8); }
        public RHostPlatformDesc clipboard_set(MemorySegment value) { s.set(ADDRESS, 8, value == null ? MemorySegment.NULL : value); return this; }

        /**
         * The clipboard text, valid until the next call, or NULL. NULL: the clipboard stays in the process.
         */
        public MemorySegment clipboard_get() { return s.get(ADDRESS, 16); }
        public RHostPlatformDesc clipboard_get(MemorySegment value) { s.set(ADDRESS, 16, value == null ? MemorySegment.NULL : value); return this; }

        /**
         * Raven has work to do (a timer, a posted task, a redraw): produce a frame soon. Thread-safe. May be NULL.
         */
        public MemorySegment wake() { return s.get(ADDRESS, 24); }
        public RHostPlatformDesc wake(MemorySegment value) { s.set(ADDRESS, 24, value == null ? MemorySegment.NULL : value); return this; }

        public MemorySegment user_data() { return s.get(ADDRESS, 32); }
        public RHostPlatformDesc user_data(MemorySegment value) { s.set(ADDRESS, 32, value == null ? MemorySegment.NULL : value); return this; }
    }

    // ---- callbacks

    /**
     * Registers a function called once per frame, before the layout.
     * The body of your loop does the same job; the callback is also called
     * while the OS keeps the thread in a loop of its own (on Windows, while
     * the user moves or resizes a window), when your loop is not running.
     */
    @FunctionalInterface
    public interface RFrameCallback {
        void invoke(double delta_seconds, MemorySegment user_data);
    }

    private static void RFrameCallback$raw(RFrameCallback f, double delta_seconds, MemorySegment user_data) {
        try {
            f.invoke(delta_seconds, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RFrameCallback f) {
        return stub(f, RFrameCallback.class, "RFrameCallback$raw", FunctionDescriptor.ofVoid(JAVA_DOUBLE, ADDRESS));
    }

    @FunctionalInterface
    public interface RTaskCallback {
        void invoke(MemorySegment user_data);
    }

    private static void RTaskCallback$raw(RTaskCallback f, MemorySegment user_data) {
        try {
            f.invoke(user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RTaskCallback f) {
        return stub(f, RTaskCallback.class, "RTaskCallback$raw", FunctionDescriptor.ofVoid(ADDRESS));
    }

    @FunctionalInterface
    public interface RParallelRangeCallback {
        void invoke(int begin, int end, MemorySegment user_data);
    }

    private static void RParallelRangeCallback$raw(RParallelRangeCallback f, int begin, int end, MemorySegment user_data) {
        try {
            f.invoke(begin, end, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RParallelRangeCallback f) {
        return stub(f, RParallelRangeCallback.class, "RParallelRangeCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
    }

    @FunctionalInterface
    public interface RTimerCallback {
        void invoke(int timer, MemorySegment user_data);
    }

    private static void RTimerCallback$raw(RTimerCallback f, int timer, MemorySegment user_data) {
        try {
            f.invoke(timer, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RTimerCallback f) {
        return stub(f, RTimerCallback.class, "RTimerCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
    }

    /**
     * Actions connect widgets to application code by name. A widget carries an
     * action name (set from XML with OnClicked="file.save", or from C with
     * Rwidget_set_action); when its main event fires, the function bound to that
     * name is called. Names are resolved when the event fires, so actions can be
     * bound before or after the widgets are created.
     */
    @FunctionalInterface
    public interface RActionCallback {
        void invoke(int sender, String action, MemorySegment user_data);
    }

    private static void RActionCallback$raw(RActionCallback f, int sender, MemorySegment action, MemorySegment user_data) {
        try {
            f.invoke(sender, text(action), user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RActionCallback f) {
        return stub(f, RActionCallback.class, "RActionCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
    }

    /**
     * Registers a function called whenever an error is raised.
     * The function may be registered before Rinit. It is called on the
     * thread that raised the error: errors of the renderer (and the messages
     * of the Vulkan validation layers) come from the render thread.
     */
    @FunctionalInterface
    public interface RErrorCallback {
        void invoke(int code, int severity, String detail);
    }

    private static void RErrorCallback$raw(RErrorCallback f, int code, int severity, MemorySegment detail) {
        try {
            f.invoke(code, severity, text(detail));
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RErrorCallback f) {
        return stub(f, RErrorCallback.class, "RErrorCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
    }

    /**
     * Registers a function called after Raven recovered from the loss
     * of the GPU device (a driver reset or update, a GPU removed, a long sleep).
     *
     * Raven then starts the backend again and remakes what it can by itself:
     * the surfaces of the windows, the fonts, the shaders, the textures made
     * from image files and the small textures (up to 256 x 256). The pixels of
     * the larger textures made from pixels are lost: the textures keep their
     * handle and size but are transparent until Rtexture_update fills them
     * again, which is what this function is for. Textures that wrap objects of
     * the backend (raven_vulkan.h) must be made again, and the Vulkan handles
     * returned by Rvulkan_get_device... have changed.
     *
     * Called on the UI thread, during Rdraw_frame / Rwait_frame. Set it after
     * Rinit; NULL removes it.
     */
    @FunctionalInterface
    public interface RDeviceResetCallback {
        void invoke(MemorySegment user_data);
    }

    private static void RDeviceResetCallback$raw(RDeviceResetCallback f, MemorySegment user_data) {
        try {
            f.invoke(user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RDeviceResetCallback f) {
        return stub(f, RDeviceResetCallback.class, "RDeviceResetCallback$raw", FunctionDescriptor.ofVoid(ADDRESS));
    }

    /**
     * Called for a drop target (a widget, or a window: `target` is its
     * handle). Return R_TRUE to accept: while the pointer is over the target this
     * shows the "can drop here" cursor and highlights the target; for
     * R_DROP_DROPPED it tells the source that the data was taken.
     */
    @FunctionalInterface
    public interface RDropCallback {
        int invoke(int target, RDropEvent event, MemorySegment user_data);
    }

    private static int RDropCallback$raw(RDropCallback f, int target, MemorySegment event, MemorySegment user_data) {
        try {
            return f.invoke(target, event.equals(MemorySegment.NULL) ? null : new RDropEvent(event), user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
            return 0;
        }
    }

    private static MemorySegment upcall(RDropCallback f) {
        return stub(f, RDropCallback.class, "RDropCallback$raw", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS));
    }

    /**
     * Called when the user asks to close the window.
     * Return R_TRUE to let the window close, R_FALSE to keep it open.
     */
    @FunctionalInterface
    public interface RWindowCloseCallback {
        int invoke(int window, MemorySegment user_data);
    }

    private static int RWindowCloseCallback$raw(RWindowCloseCallback f, int window, MemorySegment user_data) {
        try {
            return f.invoke(window, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
            return 0;
        }
    }

    private static MemorySegment upcall(RWindowCloseCallback f) {
        return stub(f, RWindowCloseCallback.class, "RWindowCloseCallback$raw", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS));
    }

    /**
     * Called after the window was resized. Sizes are in logical units.
     */
    @FunctionalInterface
    public interface RWindowResizeCallback {
        void invoke(int window, int width, int height, MemorySegment user_data);
    }

    private static void RWindowResizeCallback$raw(RWindowResizeCallback f, int window, int width, int height, MemorySegment user_data) {
        try {
            f.invoke(window, width, height, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RWindowResizeCallback f) {
        return stub(f, RWindowResizeCallback.class, "RWindowResizeCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT, ADDRESS));
    }

    /**
     * Sees every input event of the window (mouse, wheel, keys, text)
     * before any widget, with `target` set to the widget under the pointer.
     * Returning R_TRUE consumes the event: the widgets do not get it. The
     * state read by Rwindow_is_key_down is updated either way.
     */
    @FunctionalInterface
    public interface RWindowEventHook {
        int invoke(int window, REvent event, MemorySegment user_data);
    }

    private static int RWindowEventHook$raw(RWindowEventHook f, int window, MemorySegment event, MemorySegment user_data) {
        try {
            return f.invoke(window, event.equals(MemorySegment.NULL) ? null : new REvent(event), user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
            return 0;
        }
    }

    private static MemorySegment upcall(RWindowEventHook f) {
        return stub(f, RWindowEventHook.class, "RWindowEventHook$raw", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS));
    }

    /**
     * Return R_TRUE to take the finger: the library then neither clicks nor scrolls with it.
     */
    @FunctionalInterface
    public interface RTouchCallback {
        int invoke(int widget, RTouchEvent event, MemorySegment user_data);
    }

    private static int RTouchCallback$raw(RTouchCallback f, int widget, MemorySegment event, MemorySegment user_data) {
        try {
            return f.invoke(widget, event.equals(MemorySegment.NULL) ? null : new RTouchEvent(event), user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
            return 0;
        }
    }

    private static MemorySegment upcall(RTouchCallback f) {
        return stub(f, RTouchCallback.class, "RTouchCallback$raw", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS));
    }

    /**
     * Return R_TRUE (on R_PEN_DOWN) to take the stroke: the pen does not act as a mouse until it is lifted.
     */
    @FunctionalInterface
    public interface RPenCallback {
        int invoke(int widget, RPenEvent event, MemorySegment user_data);
    }

    private static int RPenCallback$raw(RPenCallback f, int widget, MemorySegment event, MemorySegment user_data) {
        try {
            return f.invoke(widget, event.equals(MemorySegment.NULL) ? null : new RPenEvent(event), user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
            return 0;
        }
    }

    private static MemorySegment upcall(RPenCallback f) {
        return stub(f, RPenCallback.class, "RPenCallback$raw", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS));
    }

    /**
     * Return R_TRUE (on R_GESTURE_BEGIN) to take the gesture: nothing zooms or pans by itself.
     */
    @FunctionalInterface
    public interface RGestureCallback {
        int invoke(int widget, RGestureEvent event, MemorySegment user_data);
    }

    private static int RGestureCallback$raw(RGestureCallback f, int widget, MemorySegment event, MemorySegment user_data) {
        try {
            return f.invoke(widget, event.equals(MemorySegment.NULL) ? null : new RGestureEvent(event), user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
            return 0;
        }
    }

    private static MemorySegment upcall(RGestureCallback f) {
        return stub(f, RGestureCallback.class, "RGestureCallback$raw", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS));
    }

    /**
     * Called once when the widget is gone: deleted by the application,
     * with its parent or its window, or by Rshutdown. The place to release
     * what belonged to it (a texture, a connection). The handle it receives
     * names nothing any more: it is only there to tell widgets apart.
     * Called on the UI thread, shortly after the deletion.
     */
    @FunctionalInterface
    public interface RWidgetDestroyCallback {
        void invoke(int widget, MemorySegment user_data);
    }

    private static void RWidgetDestroyCallback$raw(RWidgetDestroyCallback f, int widget, MemorySegment user_data) {
        try {
            f.invoke(widget, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RWidgetDestroyCallback f) {
        return stub(f, RWidgetDestroyCallback.class, "RWidgetDestroyCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
    }

    /**
     * Receives the input events of a widget before the widget itself.
     * Return R_TRUE to consume the event (the widget and its parents will
     * not see it), R_FALSE to let it continue.
     */
    @FunctionalInterface
    public interface RWidgetEventCallback {
        int invoke(int widget, REvent event, MemorySegment user_data);
    }

    private static int RWidgetEventCallback$raw(RWidgetEventCallback f, int widget, MemorySegment event, MemorySegment user_data) {
        try {
            return f.invoke(widget, event.equals(MemorySegment.NULL) ? null : new REvent(event), user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
            return 0;
        }
    }

    private static MemorySegment upcall(RWidgetEventCallback f) {
        return stub(f, RWidgetEventCallback.class, "RWidgetEventCallback$raw", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS));
    }

    @FunctionalInterface
    public interface RTabChangedCallback {
        void invoke(int tab_view, int index, MemorySegment user_data);
    }

    private static void RTabChangedCallback$raw(RTabChangedCallback f, int tab_view, int index, MemorySegment user_data) {
        try {
            f.invoke(tab_view, index, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RTabChangedCallback f) {
        return stub(f, RTabChangedCallback.class, "RTabChangedCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
    }

    /**
     * The user closes a tab (its button, a middle click). Return R_TRUE
     * to delete its page now, R_FALSE to keep it (the application may ask to
     * save, then delete the page itself with Rwidget_destroy).
     */
    @FunctionalInterface
    public interface RTabCloseCallback {
        int invoke(int tab_view, int index, int content, MemorySegment user_data);
    }

    private static int RTabCloseCallback$raw(RTabCloseCallback f, int tab_view, int index, int content, MemorySegment user_data) {
        try {
            return f.invoke(tab_view, index, content, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
            return 0;
        }
    }

    private static MemorySegment upcall(RTabCloseCallback f) {
        return stub(f, RTabCloseCallback.class, "RTabCloseCallback$raw", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT, ADDRESS));
    }

    @FunctionalInterface
    public interface RExpanderChangedCallback {
        void invoke(int expander, int expanded, MemorySegment user_data);
    }

    private static void RExpanderChangedCallback$raw(RExpanderChangedCallback f, int expander, int expanded, MemorySegment user_data) {
        try {
            f.invoke(expander, expanded, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RExpanderChangedCallback f) {
        return stub(f, RExpanderChangedCallback.class, "RExpanderChangedCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
    }

    /**
     * Called when the user clicks a link of a Markdown label.
     *
     * Without a callback, a link written [Save](action:file.save) fires the
     * action "file.save" (Raction_bind), and http, https and mailto links
     * open in the browser or the mail program of the user (Ropen_url).
     */
    @FunctionalInterface
    public interface RLabelLinkCallback {
        void invoke(int label, String url, MemorySegment user_data);
    }

    private static void RLabelLinkCallback$raw(RLabelLinkCallback f, int label, MemorySegment url, MemorySegment user_data) {
        try {
            f.invoke(label, text(url), user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RLabelLinkCallback f) {
        return stub(f, RLabelLinkCallback.class, "RLabelLinkCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
    }

    @FunctionalInterface
    public interface RButtonClickedCallback {
        void invoke(int button, MemorySegment user_data);
    }

    private static void RButtonClickedCallback$raw(RButtonClickedCallback f, int button, MemorySegment user_data) {
        try {
            f.invoke(button, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RButtonClickedCallback f) {
        return stub(f, RButtonClickedCallback.class, "RButtonClickedCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
    }

    @FunctionalInterface
    public interface RCheckboxChangedCallback {
        void invoke(int checkbox, int checked, MemorySegment user_data);
    }

    private static void RCheckboxChangedCallback$raw(RCheckboxChangedCallback f, int checkbox, int checked, MemorySegment user_data) {
        try {
            f.invoke(checkbox, checked, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RCheckboxChangedCallback f) {
        return stub(f, RCheckboxChangedCallback.class, "RCheckboxChangedCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
    }

    @FunctionalInterface
    public interface RSliderChangedCallback {
        void invoke(int slider, float value, MemorySegment user_data);
    }

    private static void RSliderChangedCallback$raw(RSliderChangedCallback f, int slider, float value, MemorySegment user_data) {
        try {
            f.invoke(slider, value, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RSliderChangedCallback f) {
        return stub(f, RSliderChangedCallback.class, "RSliderChangedCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, ADDRESS));
    }

    @FunctionalInterface
    public interface RTextChangedCallback {
        void invoke(int text_input, String text, MemorySegment user_data);
    }

    private static void RTextChangedCallback$raw(RTextChangedCallback f, int text_input, MemorySegment text, MemorySegment user_data) {
        try {
            f.invoke(text_input, text(text), user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RTextChangedCallback f) {
        return stub(f, RTextChangedCallback.class, "RTextChangedCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
    }

    @FunctionalInterface
    public interface RSpinBoxChangedCallback {
        void invoke(int spin_box, double value, MemorySegment user_data);
    }

    private static void RSpinBoxChangedCallback$raw(RSpinBoxChangedCallback f, int spin_box, double value, MemorySegment user_data) {
        try {
            f.invoke(spin_box, value, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RSpinBoxChangedCallback f) {
        return stub(f, RSpinBoxChangedCallback.class, "RSpinBoxChangedCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_DOUBLE, ADDRESS));
    }

    @FunctionalInterface
    public interface RComboBoxChangedCallback {
        void invoke(int combo_box, int index, MemorySegment user_data);
    }

    private static void RComboBoxChangedCallback$raw(RComboBoxChangedCallback f, int combo_box, int index, MemorySegment user_data) {
        try {
            f.invoke(combo_box, index, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RComboBoxChangedCallback f) {
        return stub(f, RComboBoxChangedCallback.class, "RComboBoxChangedCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
    }

    /**
     * Virtual mode, for large or computed data: the list stores nothing and
     * asks for the text of the rows it draws. The returned string must stay
     * valid until the next call.
     */
    @FunctionalInterface
    public interface RListItemTextCallback {
        String invoke(int list_view, int index, MemorySegment user_data);
    }

    private static MemorySegment RListItemTextCallback$raw(RListItemTextCallback f, int list_view, int index, MemorySegment user_data) {
        try {
            String text = f.invoke(list_view, index, user_data);
            if (text == null) return MemorySegment.NULL;
            MemorySegment segment = utf8(Arena.ofAuto(), text);
            synchronized (RETURNED) { RETURNED.put(f, segment); }
            return segment;
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
            return MemorySegment.NULL;
        }
    }

    private static MemorySegment upcall(RListItemTextCallback f) {
        return stub(f, RListItemTextCallback.class, "RListItemTextCallback$raw", FunctionDescriptor.of(ADDRESS, JAVA_INT, JAVA_INT, ADDRESS));
    }

    @FunctionalInterface
    public interface RListEventCallback {
        void invoke(int list_view, int index, MemorySegment user_data);
    }

    private static void RListEventCallback$raw(RListEventCallback f, int list_view, int index, MemorySegment user_data) {
        try {
            f.invoke(list_view, index, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RListEventCallback f) {
        return stub(f, RListEventCallback.class, "RListEventCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
    }

    @FunctionalInterface
    public interface RTreeEventCallback {
        void invoke(int tree_view, int item, MemorySegment user_data);
    }

    private static void RTreeEventCallback$raw(RTreeEventCallback f, int tree_view, int item, MemorySegment user_data) {
        try {
            f.invoke(tree_view, item, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RTreeEventCallback f) {
        return stub(f, RTreeEventCallback.class, "RTreeEventCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
    }

    /**
     * Right click: `item` is the item under the pointer (selected
     * first), or R_INVALID_ID in the empty space; x, y in the window, for Rmenu_open.
     */
    @FunctionalInterface
    public interface RTreeContextCallback {
        void invoke(int tree_view, int item, float x, float y, MemorySegment user_data);
    }

    private static void RTreeContextCallback$raw(RTreeContextCallback f, int tree_view, int item, float x, float y, MemorySegment user_data) {
        try {
            f.invoke(tree_view, item, x, y, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RTreeContextCallback f) {
        return stub(f, RTreeContextCallback.class, "RTreeContextCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, ADDRESS));
    }

    @FunctionalInterface
    public interface RMenuItemClickedCallback {
        void invoke(int item, MemorySegment user_data);
    }

    private static void RMenuItemClickedCallback$raw(RMenuItemClickedCallback f, int item, MemorySegment user_data) {
        try {
            f.invoke(item, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RMenuItemClickedCallback f) {
        return stub(f, RMenuItemClickedCallback.class, "RMenuItemClickedCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
    }

    /**
     * Fills `item` for tile `index`. Called on the UI thread, only for the tiles in view.
     */
    @FunctionalInterface
    public interface RTileItemCallback {
        void invoke(int tile_view, int index, RTileItem item, MemorySegment user_data);
    }

    private static void RTileItemCallback$raw(RTileItemCallback f, int tile_view, int index, MemorySegment item, MemorySegment user_data) {
        try {
            f.invoke(tile_view, index, item.equals(MemorySegment.NULL) ? null : new RTileItem(item), user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RTileItemCallback f) {
        return stub(f, RTileItemCallback.class, "RTileItemCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS));
    }

    /**
     * Selection change (`index`: the current tile, or -1) and activation (double click, Enter).
     */
    @FunctionalInterface
    public interface RTileEventCallback {
        void invoke(int tile_view, int index, MemorySegment user_data);
    }

    private static void RTileEventCallback$raw(RTileEventCallback f, int tile_view, int index, MemorySegment user_data) {
        try {
            f.invoke(tile_view, index, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RTileEventCallback f) {
        return stub(f, RTileEventCallback.class, "RTileEventCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
    }

    /**
     * Right click: the tile under the pointer (selected first), or -1 in the empty space; x, y in the window.
     */
    @FunctionalInterface
    public interface RTileContextCallback {
        void invoke(int tile_view, int index, float x, float y, MemorySegment user_data);
    }

    private static void RTileContextCallback$raw(RTileContextCallback f, int tile_view, int index, float x, float y, MemorySegment user_data) {
        try {
            f.invoke(tile_view, index, x, y, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RTileContextCallback f) {
        return stub(f, RTileContextCallback.class, "RTileContextCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, ADDRESS));
    }

    /**
     * The data of a drag, given by the application (paths of files...)
     * instead of the indices: the dragged tiles are the selected ones
     * (Rtile_view_get_selection). The text returned is copied; NULL keeps
     * the indices.
     */
    @FunctionalInterface
    public interface RTileDragDataCallback {
        String invoke(int tile_view, MemorySegment user_data);
    }

    private static MemorySegment RTileDragDataCallback$raw(RTileDragDataCallback f, int tile_view, MemorySegment user_data) {
        try {
            String text = f.invoke(tile_view, user_data);
            if (text == null) return MemorySegment.NULL;
            MemorySegment segment = utf8(Arena.ofAuto(), text);
            synchronized (RETURNED) { RETURNED.put(f, segment); }
            return segment;
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
            return MemorySegment.NULL;
        }
    }

    private static MemorySegment upcall(RTileDragDataCallback f) {
        return stub(f, RTileDragDataCallback.class, "RTileDragDataCallback$raw", FunctionDescriptor.of(ADDRESS, JAVA_INT, ADDRESS));
    }

    /**
     * Called when the user zoomed (to zoom other editors the same, or to keep it for the next time).
     */
    @FunctionalInterface
    public interface RTextEditorZoomCallback {
        void invoke(int editor, float zoom, MemorySegment user_data);
    }

    private static void RTextEditorZoomCallback$raw(RTextEditorZoomCallback f, int editor, float zoom, MemorySegment user_data) {
        try {
            f.invoke(editor, zoom, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RTextEditorZoomCallback f) {
        return stub(f, RTextEditorZoomCallback.class, "RTextEditorZoomCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, ADDRESS));
    }

    /**
     * Called each time the editor gathers its proposals, with the
     * start of the word being typed: add your own with
     * Rtext_editor_add_completion (the fields of a structure, the answers of
     * a language server...). They are listed before the others.
     */
    @FunctionalInterface
    public interface RCompletionCallback {
        void invoke(int editor, String prefix, MemorySegment user_data);
    }

    private static void RCompletionCallback$raw(RCompletionCallback f, int editor, MemorySegment prefix, MemorySegment user_data) {
        try {
            f.invoke(editor, text(prefix), user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RCompletionCallback f) {
        return stub(f, RCompletionCallback.class, "RCompletionCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
    }

    /**
     * Called after each change made by the user (typing, pasting,
     * undoing...) or by Rtext_editor_insert_text. The action of the widget
     * (OnChanged in XML) fires at the same time.
     */
    @FunctionalInterface
    public interface RTextEditorChangedCallback {
        void invoke(int editor, MemorySegment user_data);
    }

    private static void RTextEditorChangedCallback$raw(RTextEditorChangedCallback f, int editor, MemorySegment user_data) {
        try {
            f.invoke(editor, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RTextEditorChangedCallback f) {
        return stub(f, RTextEditorChangedCallback.class, "RTextEditorChangedCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
    }

    /**
     * Called when the user closes a panel, before it is deleted.
     * Return R_FALSE to keep the panel.
     */
    @FunctionalInterface
    public interface RDockCloseCallback {
        int invoke(int area, int panel, MemorySegment user_data);
    }

    private static int RDockCloseCallback$raw(RDockCloseCallback f, int area, int panel, MemorySegment user_data) {
        try {
            return f.invoke(area, panel, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
            return 0;
        }
    }

    private static MemorySegment upcall(RDockCloseCallback f) {
        return stub(f, RDockCloseCallback.class, "RDockCloseCallback$raw", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT, ADDRESS));
    }

    /**
     * Called after the user moved, resized, showed or closed a panel.
     */
    @FunctionalInterface
    public interface RDockChangedCallback {
        void invoke(int area, MemorySegment user_data);
    }

    private static void RDockChangedCallback$raw(RDockChangedCallback f, int area, MemorySegment user_data) {
        try {
            f.invoke(area, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RDockChangedCallback f) {
        return stub(f, RDockChangedCallback.class, "RDockChangedCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
    }

    /**
     * Called when the user unfolds or folds the island. The action of the widget (OnChanged) fires too.
     */
    @FunctionalInterface
    public interface RIslandChangedCallback {
        void invoke(int island, int expanded, MemorySegment user_data);
    }

    private static void RIslandChangedCallback$raw(RIslandChangedCallback f, int island, int expanded, MemorySegment user_data) {
        try {
            f.invoke(island, expanded, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RIslandChangedCallback f) {
        return stub(f, RIslandChangedCallback.class, "RIslandChangedCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
    }

    /**
     * Called each time the user changes the color: while dragging,
     * and when a code or a number is entered. The action of the widget
     * (OnChanged in XML) fires too.
     */
    @FunctionalInterface
    public interface RColorChangedCallback {
        void invoke(int picker, float r, float g, float b, float a, MemorySegment user_data);
    }

    private static void RColorChangedCallback$raw(RColorChangedCallback f, int picker, float r, float g, float b, float a, MemorySegment user_data) {
        try {
            f.invoke(picker, r, g, b, a, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RColorChangedCallback f) {
        return stub(f, RColorChangedCallback.class, "RColorChangedCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, ADDRESS));
    }

    /**
     * The playhead moved: scrubbed by the user, or running by itself.
     */
    @FunctionalInterface
    public interface RTimelineTimeCallback {
        void invoke(int timeline, double seconds, MemorySegment user_data);
    }

    private static void RTimelineTimeCallback$raw(RTimelineTimeCallback f, int timeline, double seconds, MemorySegment user_data) {
        try {
            f.invoke(timeline, seconds, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RTimelineTimeCallback f) {
        return stub(f, RTimelineTimeCallback.class, "RTimelineTimeCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_DOUBLE, ADDRESS));
    }

    /**
     * Something happened to an item; which thing depends on the function the callback was given to.
     */
    @FunctionalInterface
    public interface RTimelineItemCallback {
        void invoke(int timeline, int item, MemorySegment user_data);
    }

    private static void RTimelineItemCallback$raw(RTimelineItemCallback f, int timeline, int item, MemorySegment user_data) {
        try {
            f.invoke(timeline, item, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RTimelineItemCallback f) {
        return stub(f, RTimelineItemCallback.class, "RTimelineItemCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
    }

    /**
     * Asked when the user is about to link two pins that may be
     * linked: return R_FALSE to refuse (a loop, a rule of the application).
     */
    @FunctionalInterface
    public interface RNodeGraphConnectCallback {
        int invoke(int graph, int output_pin, int input_pin, MemorySegment user_data);
    }

    private static int RNodeGraphConnectCallback$raw(RNodeGraphConnectCallback f, int graph, int output_pin, int input_pin, MemorySegment user_data) {
        try {
            return f.invoke(graph, output_pin, input_pin, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
            return 0;
        }
    }

    private static MemorySegment upcall(RNodeGraphConnectCallback f) {
        return stub(f, RNodeGraphConnectCallback.class, "RNodeGraphConnectCallback$raw", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT, ADDRESS));
    }

    /**
     * Something happened to a link or to a node; which thing depends on the function the callback was given to.
     */
    @FunctionalInterface
    public interface RNodeGraphItemCallback {
        void invoke(int graph, int item, MemorySegment user_data);
    }

    private static void RNodeGraphItemCallback$raw(RNodeGraphItemCallback f, int graph, int item, MemorySegment user_data) {
        try {
            f.invoke(graph, item, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RNodeGraphItemCallback f) {
        return stub(f, RNodeGraphItemCallback.class, "RNodeGraphItemCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
    }

    /**
     * Right click: `node` is the node under the pointer (selected first),
     * or 0 on the empty canvas; x, y are in the widget (Rnode_graph_to_canvas).
     * Without this callback the right button does nothing.
     */
    @FunctionalInterface
    public interface RNodeGraphContextCallback {
        void invoke(int graph, int node, float x, float y, MemorySegment user_data);
    }

    private static void RNodeGraphContextCallback$raw(RNodeGraphContextCallback f, int graph, int node, float x, float y, MemorySegment user_data) {
        try {
            f.invoke(graph, node, x, y, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RNodeGraphContextCallback f) {
        return stub(f, RNodeGraphContextCallback.class, "RNodeGraphContextCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, ADDRESS));
    }

    /**
     * A block changed (0: several, or one was deleted), or a script was clicked (its top block).
     */
    @FunctionalInterface
    public interface RBlockEditorCallback {
        void invoke(int editor, int block, MemorySegment user_data);
    }

    private static void RBlockEditorCallback$raw(RBlockEditorCallback f, int editor, int block, MemorySegment user_data) {
        try {
            f.invoke(editor, block, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RBlockEditorCallback f) {
        return stub(f, RBlockEditorCallback.class, "RBlockEditorCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
    }

    /**
     * Right click: on a block (0 on the canvas), at x, y in the widget.
     */
    @FunctionalInterface
    public interface RBlockEditorContextCallback {
        void invoke(int editor, int block, float x, float y, MemorySegment user_data);
    }

    private static void RBlockEditorContextCallback$raw(RBlockEditorContextCallback f, int editor, int block, float x, float y, MemorySegment user_data) {
        try {
            f.invoke(editor, block, x, y, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RBlockEditorContextCallback f) {
        return stub(f, RBlockEditorContextCallback.class, "RBlockEditorContextCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, ADDRESS));
    }

    /**
     * Called when a dialog closes. `result` is the value given to Rdialog_close, 0 when dismissed.
     */
    @FunctionalInterface
    public interface RDialogClosedCallback {
        void invoke(int content, int result, MemorySegment user_data);
    }

    private static void RDialogClosedCallback$raw(RDialogClosedCallback f, int content, int result, MemorySegment user_data) {
        try {
            f.invoke(content, result, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RDialogClosedCallback f) {
        return stub(f, RDialogClosedCallback.class, "RDialogClosedCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
    }

    /**
     * Called when the user chooses: a double click or Enter on a file,
     * the Open / Save button. `path` is the first chosen path; with several
     * files, read them with Rfile_browser_get_selection. The action of the
     * widget (OnActivated in XML) fires at the same time.
     */
    @FunctionalInterface
    public interface RFileBrowserCallback {
        void invoke(int browser, String path, MemorySegment user_data);
    }

    private static void RFileBrowserCallback$raw(RFileBrowserCallback f, int browser, MemorySegment path, MemorySegment user_data) {
        try {
            f.invoke(browser, text(path), user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RFileBrowserCallback f) {
        return stub(f, RFileBrowserCallback.class, "RFileBrowserCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
    }

    /**
     * Called on the render thread each time the window is rendered.
     * State you bind (pipeline, descriptor sets, viewport, scissor) does not
     * need to be restored. NULL removes the callback.
     */
    @FunctionalInterface
    public interface RViewportDrawCallback {
        void invoke(RViewportDrawInfo info, MemorySegment user_data);
    }

    private static void RViewportDrawCallback$raw(RViewportDrawCallback f, MemorySegment info, MemorySegment user_data) {
        try {
            f.invoke(info.equals(MemorySegment.NULL) ? null : new RViewportDrawInfo(info), user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RViewportDrawCallback f) {
        return stub(f, RViewportDrawCallback.class, "RViewportDrawCallback$raw", FunctionDescriptor.ofVoid(ADDRESS, ADDRESS));
    }

    /**
     * Called on the UI thread when the pixel size changes: resize your render target here.
     */
    @FunctionalInterface
    public interface RViewportResizeCallback {
        void invoke(int viewport, int width, int height, MemorySegment user_data);
    }

    private static void RViewportResizeCallback$raw(RViewportResizeCallback f, int viewport, int width, int height, MemorySegment user_data) {
        try {
            f.invoke(viewport, width, height, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RViewportResizeCallback f) {
        return stub(f, RViewportResizeCallback.class, "RViewportResizeCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT, ADDRESS));
    }

    @FunctionalInterface
    public interface RPaintCallback {
        void invoke(int widget, MemorySegment painter, float width, float height, MemorySegment user_data);
    }

    private static void RPaintCallback$raw(RPaintCallback f, int widget, MemorySegment painter, float width, float height, MemorySegment user_data) {
        try {
            f.invoke(widget, painter, width, height, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RPaintCallback f) {
        return stub(f, RPaintCallback.class, "RPaintCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, JAVA_FLOAT, JAVA_FLOAT, ADDRESS));
    }

    @FunctionalInterface
    public interface RMeasureCallback {
        void invoke(int widget, MemorySegment width, MemorySegment height, MemorySegment user_data);
    }

    private static void RMeasureCallback$raw(RMeasureCallback f, int widget, MemorySegment width, MemorySegment height, MemorySegment user_data) {
        try {
            f.invoke(widget, width.reinterpret(4), height.reinterpret(4), user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RMeasureCallback f) {
        return stub(f, RMeasureCallback.class, "RMeasureCallback$raw", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS, ADDRESS));
    }

    /**
     * Called each time a file was loaded again.
     * path The file given when the interface was created.
     * ok   R_FALSE when the file could not be used (the interface is unchanged).
     *
     * Widgets that had to be built again have new handles: this is the
     * place to look them up again (Rwidget_find).
     */
    @FunctionalInterface
    public interface RXmlReloadCallback {
        void invoke(String path, int ok, MemorySegment user_data);
    }

    private static void RXmlReloadCallback$raw(RXmlReloadCallback f, MemorySegment path, int ok, MemorySegment user_data) {
        try {
            f.invoke(text(path), ok, user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RXmlReloadCallback f) {
        return stub(f, RXmlReloadCallback.class, "RXmlReloadCallback$raw", FunctionDescriptor.ofVoid(ADDRESS, JAVA_INT, ADDRESS));
    }

    /**
     * Tags made by code. A library or an application that builds a widget of
     * its own (a R_WIDGET_CUSTOM with its callbacks, or an assembly of
     * widgets) can give it a tag: documents then write &lt;MyWidget .../&gt; as
     * they write &lt;Button/&gt;.
     *
     * `create` makes the widget and returns it. `set_attribute` (may be
     * NULL) receives the attributes of the element before Raven looks at
     * them: it returns R_TRUE for those it understands, R_FALSE to leave
     * them to the widget (Name, Width, Slot..., and what its type knows).
     * Rwidget_set_attribute goes through it too.
     */
    @FunctionalInterface
    public interface RXmlTagCreateCallback {
        int invoke(String tag, MemorySegment user_data);
    }

    private static int RXmlTagCreateCallback$raw(RXmlTagCreateCallback f, MemorySegment tag, MemorySegment user_data) {
        try {
            return f.invoke(text(tag), user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
            return 0;
        }
    }

    private static MemorySegment upcall(RXmlTagCreateCallback f) {
        return stub(f, RXmlTagCreateCallback.class, "RXmlTagCreateCallback$raw", FunctionDescriptor.of(JAVA_INT, ADDRESS, ADDRESS));
    }

    @FunctionalInterface
    public interface RXmlTagAttributeCallback {
        int invoke(int widget, String name, String value, MemorySegment user_data);
    }

    private static int RXmlTagAttributeCallback$raw(RXmlTagAttributeCallback f, int widget, MemorySegment name, MemorySegment value, MemorySegment user_data) {
        try {
            return f.invoke(widget, text(name), text(value), user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
            return 0;
        }
    }

    private static MemorySegment upcall(RXmlTagAttributeCallback f) {
        return stub(f, RXmlTagAttributeCallback.class, "RXmlTagAttributeCallback$raw", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS, ADDRESS));
    }

    @FunctionalInterface
    public interface RLocaleChangedCallback {
        void invoke(String language, MemorySegment user_data);
    }

    private static void RLocaleChangedCallback$raw(RLocaleChangedCallback f, MemorySegment language, MemorySegment user_data) {
        try {
            f.invoke(text(language), user_data);
        } catch (Throwable t) {
            // an exception must not cross C: it is reported here
            t.printStackTrace();
        }
    }

    private static MemorySegment upcall(RLocaleChangedCallback f) {
        return stub(f, RLocaleChangedCallback.class, "RLocaleChangedCallback$raw", FunctionDescriptor.ofVoid(ADDRESS, ADDRESS));
    }

    // ---- functions

    private static MethodHandle Rinit$;

    /**
     * Starts the library with default settings on the given backend.
     * The calling thread becomes the UI thread.
     * Returns: R_TRUE on success. On failure, see Rerror_get_last.
     */
    public static int Rinit(int backend) {
        MethodHandle $h = Rinit$;
        if ($h == null) Rinit$ = $h = find("Rinit", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(backend);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rinit_ex$;

    /**
     * Starts the library with explicit settings.
     * desc Settings, or NULL for the defaults.
     */
    public static int Rinit_ex(RInitDesc desc) {
        MethodHandle $h = Rinit_ex$;
        if ($h == null) Rinit_ex$ = $h = find("Rinit_ex", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try {
            int $r = (int) $h.invokeExact((MemorySegment) (desc == null ? MemorySegment.NULL : desc.segment()));
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rshutdown$;

    /**
     * Destroys every window, widget and resource, stops the worker and
     * render threads and releases the backend.
     */
    public static void Rshutdown() {
        MethodHandle $h = Rshutdown$;
        if ($h == null) Rshutdown$ = $h = find("Rshutdown", FunctionDescriptor.ofVoid());
        try {
            $h.invokeExact();
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Ris_initialized$;

    public static int Ris_initialized() {
        MethodHandle $h = Ris_initialized$;
        if ($h == null) Ris_initialized$ = $h = find("Ris_initialized", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rget_version$;

    /**
     * Returns: The library version, e.g. "0.1.0".
     */
    public static String Rget_version() {
        MethodHandle $h = Rget_version$;
        if ($h == null) Rget_version$ = $h = find("Rget_version", FunctionDescriptor.of(ADDRESS));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact();
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rset_allocator$;

    /**
     * Replaces the allocator of Raven with the functions of the
     * application (an engine allocator, a tracker...). NULL selects the
     * allocator of Raven again. Must be called before Rinit, and before any
     * other function that allocates. The functions are called from several
     * threads; alloc must return memory aligned for any type (16 bytes).
     */
    public static void Rset_allocator(RAllocator allocator) {
        MethodHandle $h = Rset_allocator$;
        if ($h == null) Rset_allocator$ = $h = find("Rset_allocator", FunctionDescriptor.ofVoid(ADDRESS));
        try {
            $h.invokeExact((MemorySegment) (allocator == null ? MemorySegment.NULL : allocator.segment()));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rallocator_get_system$;

    /**
     * Returns: An allocator that forwards to malloc, realloc and free of the C
     * runtime: Rset_allocator(Rallocator_get_system()) is how memory tools
     * that follow malloc (Valgrind, heap profilers) see the blocks of Raven.
     * The environment variable RAVEN_ALLOCATOR=system does the same without
     * recompiling, and it is the default in a build with AddressSanitizer.
     */
    public static MemorySegment Rallocator_get_system() {
        MethodHandle $h = Rallocator_get_system$;
        if ($h == null) Rallocator_get_system$ = $h = find("Rallocator_get_system", FunctionDescriptor.of(ADDRESS));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rmemory_alloc$;

    /**
     * Memory from the allocator of Raven (or from the one given to
     * Rset_allocator), for the application: custom widgets, pixel buffers.
     * Aligned on 16 bytes. Never returns NULL: running out of memory is a
     * fatal error. Thread-safe.
     */
    public static MemorySegment Rmemory_alloc(long size) {
        MethodHandle $h = Rmemory_alloc$;
        if ($h == null) Rmemory_alloc$ = $h = find("Rmemory_alloc", FunctionDescriptor.of(ADDRESS, JAVA_LONG));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(size);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rmemory_realloc$;

    /**
     * Resizes a block, keeping its content. A NULL block is allocated. Thread-safe.
     */
    public static MemorySegment Rmemory_realloc(MemorySegment ptr, long size) {
        MethodHandle $h = Rmemory_realloc$;
        if ($h == null) Rmemory_realloc$ = $h = find("Rmemory_realloc", FunctionDescriptor.of(ADDRESS, ADDRESS, JAVA_LONG));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact((MemorySegment) (ptr == null ? MemorySegment.NULL : ptr), size);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rmemory_free$;

    /**
     * Frees a block. NULL is ignored. Thread-safe.
     */
    public static void Rmemory_free(MemorySegment ptr) {
        MethodHandle $h = Rmemory_free$;
        if ($h == null) Rmemory_free$ = $h = find("Rmemory_free", FunctionDescriptor.ofVoid(ADDRESS));
        try {
            $h.invokeExact((MemorySegment) (ptr == null ? MemorySegment.NULL : ptr));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rmemory_get_stats$;

    /**
     * What the library holds right now. Thread-safe.
     */
    public static void Rmemory_get_stats(RMemoryStats stats) {
        MethodHandle $h = Rmemory_get_stats$;
        if ($h == null) Rmemory_get_stats$ = $h = find("Rmemory_get_stats", FunctionDescriptor.ofVoid(ADDRESS));
        try {
            $h.invokeExact((MemorySegment) (stats == null ? MemorySegment.NULL : stats.segment()));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rmemory_check$;

    /**
     * Walks the whole heap of Raven and verifies its structure.
     * Returns: R_FALSE when it is corrupted (a buffer overrun wrote over a block
     * header). Slow: for tests and for hunting a bug. Thread-safe.
     */
    public static int Rmemory_check() {
        MethodHandle $h = Rmemory_check$;
        if ($h == null) Rmemory_check$ = $h = find("Rmemory_check", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rmemory_trim$;

    /**
     * Returns to the OS the regions of the heap that hold nothing. Thread-safe.
     */
    public static void Rmemory_trim() {
        MethodHandle $h = Rmemory_trim$;
        if ($h == null) Rmemory_trim$ = $h = find("Rmemory_trim", FunctionDescriptor.ofVoid());
        try {
            $h.invokeExact();
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdraw_frame$;

    /**
     * Produces one frame: processes the pending OS events without
     * waiting, runs the posted tasks and the timers, then lays out, paints and
     * submits every window that changed. A window in which nothing changed is
     * not drawn again, so the call is cheap when the UI is idle.
     * Returns: R_FALSE once Rquit was called or the last window is closed.
     */
    public static int Rdraw_frame() {
        MethodHandle $h = Rdraw_frame$;
        if ($h == null) Rdraw_frame$ = $h = find("Rdraw_frame", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwait_frame$;

    /**
     * Same as Rdraw_frame, but first sleeps until there is something to
     * do: an OS event, a posted task, a timer, an animation, a pending redraw.
     * timeout_seconds Longest sleep. Negative sleeps without limit, 0
     * does not sleep.
     */
    public static int Rwait_frame(double timeout_seconds) {
        MethodHandle $h = Rwait_frame$;
        if ($h == null) Rwait_frame$ = $h = find("Rwait_frame", FunctionDescriptor.of(JAVA_INT, JAVA_DOUBLE));
        try {
            int $r = (int) $h.invokeExact(timeout_seconds);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rquit$;

    /**
     * Makes Rdraw_frame and Rwait_frame return R_FALSE. Thread-safe.
     */
    public static void Rquit(int exit_code) {
        MethodHandle $h = Rquit$;
        if ($h == null) Rquit$ = $h = find("Rquit", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(exit_code);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rget_exit_code$;

    /**
     * Returns: The code given to Rquit, 0 when the last window was closed.
     */
    public static int Rget_exit_code() {
        MethodHandle $h = Rget_exit_code$;
        if ($h == null) Rget_exit_code$ = $h = find("Rget_exit_code", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Ris_running$;

    /**
     * Returns: R_FALSE once Rquit was called or every window is closed.
     */
    public static int Ris_running() {
        MethodHandle $h = Ris_running$;
        if ($h == null) Ris_running$ = $h = find("Ris_running", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rpoll_events$;

    /**
     * Processes the pending OS events without producing a frame.
     * Rdraw_frame does it already: only useful to keep the windows responsive
     * during a long operation.
     */
    public static void Rpoll_events() {
        MethodHandle $h = Rpoll_events$;
        if ($h == null) Rpoll_events$ = $h = find("Rpoll_events", FunctionDescriptor.ofVoid());
        try {
            $h.invokeExact();
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwait_events$;

    /**
     * Sleeps until an OS event, a posted task, a timer or a pending
     * redraw, and processes the OS events. Rwait_frame does it already.
     * timeout_seconds Maximum wait. Negative waits without limit.
     */
    public static void Rwait_events(double timeout_seconds) {
        MethodHandle $h = Rwait_events$;
        if ($h == null) Rwait_events$ = $h = find("Rwait_events", FunctionDescriptor.ofVoid(JAVA_DOUBLE));
        try {
            $h.invokeExact(timeout_seconds);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rset_frame_callback$;

    public static void Rset_frame_callback(RFrameCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rset_frame_callback$;
        if ($h == null) Rset_frame_callback$ = $h = find("Rset_frame_callback", FunctionDescriptor.ofVoid(ADDRESS, ADDRESS));
        try {
            $h.invokeExact(upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rset_frame_callback(RFrameCallback callback) {
        Rset_frame_callback(callback, (MemorySegment) null);
    }

    private static MethodHandle Rget_delta_time$;

    /**
     * Returns: Seconds between the last two frames.
     */
    public static double Rget_delta_time() {
        MethodHandle $h = Rget_delta_time$;
        if ($h == null) Rget_delta_time$ = $h = find("Rget_delta_time", FunctionDescriptor.of(JAVA_DOUBLE));
        try {
            double $r = (double) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rget_time$;

    /**
     * Returns: Seconds elapsed since Rinit, from a monotonic clock. Thread-safe.
     */
    public static double Rget_time() {
        MethodHandle $h = Rget_time$;
        if ($h == null) Rget_time$ = $h = find("Rget_time", FunctionDescriptor.of(JAVA_DOUBLE));
        try {
            double $r = (double) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rset_ui_scale$;

    /**
     * Scale of the whole interface, on top of the one of the monitor
     * (RInitDesc.ui_scale at start): every window is laid out again. 0.25 .. 4.
     */
    public static void Rset_ui_scale(float scale) {
        MethodHandle $h = Rset_ui_scale$;
        if ($h == null) Rset_ui_scale$ = $h = find("Rset_ui_scale", FunctionDescriptor.ofVoid(JAVA_FLOAT));
        try {
            $h.invokeExact(scale);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rset_vsync$;

    /**
     * R_TRUE: every frame waits for the vertical synchronization of the
     * screen (no tearing, no wasted frames); R_FALSE: as fast as possible
     * (RInitDesc.disable_vsync at start). Takes effect at the next frame.
     */
    public static void Rset_vsync(int enabled) {
        MethodHandle $h = Rset_vsync$;
        if ($h == null) Rset_vsync$ = $h = find("Rset_vsync", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rget_vsync$;

    public static int Rget_vsync() {
        MethodHandle $h = Rget_vsync$;
        if ($h == null) Rget_vsync$ = $h = find("Rget_vsync", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rget_ui_scale$;

    public static float Rget_ui_scale() {
        MethodHandle $h = Rget_ui_scale$;
        if ($h == null) Rget_ui_scale$ = $h = find("Rget_ui_scale", FunctionDescriptor.of(JAVA_FLOAT));
        try {
            float $r = (float) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rset_animations_enabled$;

    /**
     * R_FALSE makes every change immediate: for users who ask for
     * reduced motion (Raccessibility_set_reduce_motion), and for tests that
     * read the state right after an input.
     */
    public static void Rset_animations_enabled(int enabled) {
        MethodHandle $h = Rset_animations_enabled$;
        if ($h == null) Rset_animations_enabled$ = $h = find("Rset_animations_enabled", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rget_animations_enabled$;

    public static int Rget_animations_enabled() {
        MethodHandle $h = Rget_animations_enabled$;
        if ($h == null) Rget_animations_enabled$ = $h = find("Rget_animations_enabled", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rset_animation_speed$;

    /**
     * Multiplies the speed of every animation. 1 is the default, 0.25 a slow motion.
     */
    public static void Rset_animation_speed(float speed) {
        MethodHandle $h = Rset_animation_speed$;
        if ($h == null) Rset_animation_speed$ = $h = find("Rset_animation_speed", FunctionDescriptor.ofVoid(JAVA_FLOAT));
        try {
            $h.invokeExact(speed);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rget_animation_speed$;

    public static float Rget_animation_speed() {
        MethodHandle $h = Rget_animation_speed$;
        if ($h == null) Rget_animation_speed$ = $h = find("Rget_animation_speed", FunctionDescriptor.of(JAVA_FLOAT));
        try {
            float $r = (float) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rset_animation_time_step$;

    /**
     * When positive, each frame advances the animations by exactly this
     * many seconds instead of the time really elapsed: frames rendered one by
     * one for a video or a screenshot, deterministic tests. 0 follows the clock.
     */
    public static void Rset_animation_time_step(double seconds) {
        MethodHandle $h = Rset_animation_time_step$;
        if ($h == null) Rset_animation_time_step$ = $h = find("Rset_animation_time_step", FunctionDescriptor.ofVoid(JAVA_DOUBLE));
        try {
            $h.invokeExact(seconds);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtask_post$;

    /**
     * Queues a function to run on the UI thread during the next frame
     * and wakes Rwait_frame. Thread-safe: this is how worker threads touch the UI.
     */
    public static void Rtask_post(RTaskCallback task, MemorySegment user_data) {
        MethodHandle $h = Rtask_post$;
        if ($h == null) Rtask_post$ = $h = find("Rtask_post", FunctionDescriptor.ofVoid(ADDRESS, ADDRESS));
        try {
            $h.invokeExact(upcall(task), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Queues a function to run on the UI thread during the next frame
     * and wakes Rwait_frame. Thread-safe: this is how worker threads touch the UI.
     */
    public static void Rtask_post(RTaskCallback task) {
        Rtask_post(task, (MemorySegment) null);
    }

    private static MethodHandle Rtask_run_async$;

    /**
     * Runs a function on a worker thread of the job system. Thread-safe.
     */
    public static void Rtask_run_async(RTaskCallback task, MemorySegment user_data) {
        MethodHandle $h = Rtask_run_async$;
        if ($h == null) Rtask_run_async$ = $h = find("Rtask_run_async", FunctionDescriptor.ofVoid(ADDRESS, ADDRESS));
        try {
            $h.invokeExact(upcall(task), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Runs a function on a worker thread of the job system. Thread-safe.
     */
    public static void Rtask_run_async(RTaskCallback task) {
        Rtask_run_async(task, (MemorySegment) null);
    }

    private static MethodHandle Rparallel_for$;

    /**
     * Splits [0, count) into ranges of at most `grain` items and runs them
     * on the worker threads and the calling thread. Returns when all are done.
     */
    public static void Rparallel_for(int count, int grain, RParallelRangeCallback function, MemorySegment user_data) {
        MethodHandle $h = Rparallel_for$;
        if ($h == null) Rparallel_for$ = $h = find("Rparallel_for", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(count, grain, upcall(function), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Splits [0, count) into ranges of at most `grain` items and runs them
     * on the worker threads and the calling thread. Returns when all are done.
     */
    public static void Rparallel_for(int count, int grain, RParallelRangeCallback function) {
        Rparallel_for(count, grain, function, (MemorySegment) null);
    }

    private static MethodHandle Rwake_up$;

    /**
     * Wakes Rwait_frame and Rwait_events from any thread.
     */
    public static void Rwake_up() {
        MethodHandle $h = Rwake_up$;
        if ($h == null) Rwake_up$ = $h = find("Rwake_up", FunctionDescriptor.ofVoid());
        try {
            $h.invokeExact();
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rget_worker_thread_count$;

    public static int Rget_worker_thread_count() {
        MethodHandle $h = Rget_worker_thread_count$;
        if ($h == null) Rget_worker_thread_count$ = $h = find("Rget_worker_thread_count", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Ris_ui_thread$;

    public static int Ris_ui_thread() {
        MethodHandle $h = Ris_ui_thread$;
        if ($h == null) Ris_ui_thread$ = $h = find("Ris_ui_thread", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimer_create$;

    /**
     * Calls a function on the UI thread after `interval_seconds`.
     * repeat R_TRUE to fire repeatedly, R_FALSE to fire once and delete itself.
     */
    public static int Rtimer_create(double interval_seconds, int repeat, RTimerCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rtimer_create$;
        if ($h == null) Rtimer_create$ = $h = find("Rtimer_create", FunctionDescriptor.of(JAVA_INT, JAVA_DOUBLE, JAVA_INT, ADDRESS, ADDRESS));
        try {
            int $r = (int) $h.invokeExact(interval_seconds, repeat, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Calls a function on the UI thread after `interval_seconds`.
     * repeat R_TRUE to fire repeatedly, R_FALSE to fire once and delete itself.
     */
    public static int Rtimer_create(double interval_seconds, int repeat, RTimerCallback callback) {
        return Rtimer_create(interval_seconds, repeat, callback, (MemorySegment) null);
    }

    private static MethodHandle Rtimer_destroy$;

    public static void Rtimer_destroy(int timer) {
        MethodHandle $h = Rtimer_destroy$;
        if ($h == null) Rtimer_destroy$ = $h = find("Rtimer_destroy", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(timer);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimer_is_valid$;

    public static int Rtimer_is_valid(int timer) {
        MethodHandle $h = Rtimer_is_valid$;
        if ($h == null) Rtimer_is_valid$ = $h = find("Rtimer_is_valid", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(timer);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Raction_bind$;

    public static void Raction_bind(String name, RActionCallback callback, MemorySegment user_data) {
        MethodHandle $h = Raction_bind$;
        if ($h == null) Raction_bind$ = $h = find("Raction_bind", FunctionDescriptor.ofVoid(ADDRESS, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            $h.invokeExact(name$, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Raction_bind(String name, RActionCallback callback) {
        Raction_bind(name, callback, (MemorySegment) null);
    }

    private static MethodHandle Raction_unbind$;

    public static void Raction_unbind(String name) {
        MethodHandle $h = Raction_unbind$;
        if ($h == null) Raction_unbind$ = $h = find("Raction_unbind", FunctionDescriptor.ofVoid(ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            $h.invokeExact(name$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Raction_fire$;

    /**
     * Fires an action as a widget would: the function bound to the
     * name is called, or the default handler. What a tool bar button, a
     * script or a test uses to do what a menu item does.
     * sender Passed to the function; R_INVALID_ID when there is none.
     */
    public static void Raction_fire(String name, int sender) {
        MethodHandle $h = Raction_fire$;
        if ($h == null) Raction_fire$ = $h = find("Raction_fire", FunctionDescriptor.ofVoid(ADDRESS, JAVA_INT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            $h.invokeExact(name$, sender);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Raction_set_default_handler$;

    /**
     * Receives every action that has no bound function and is not one of the built-in actions (e.g. a script bridge).
     */
    public static void Raction_set_default_handler(RActionCallback callback, MemorySegment user_data) {
        MethodHandle $h = Raction_set_default_handler$;
        if ($h == null) Raction_set_default_handler$ = $h = find("Raction_set_default_handler", FunctionDescriptor.ofVoid(ADDRESS, ADDRESS));
        try {
            $h.invokeExact(upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Receives every action that has no bound function and is not one of the built-in actions (e.g. a script bridge).
     */
    public static void Raction_set_default_handler(RActionCallback callback) {
        Raction_set_default_handler(callback, (MemorySegment) null);
    }

    private static MethodHandle Raction_set_builtins$;

    /**
     * The actions Raven knows by itself: an interface written in XML
     * uses them with no C at all (OnClicked="app.quit").
     *
     *   app.quit                          quits (Rquit(0))
     *   window.close, window.minimize,    the window of the widget
     *   window.maximize, window.restore   (maximize goes back when it already is)
     *   island.expand, island.fold,       the island of that window
     *   island.toggle
     *   theme.dark, theme.light,          the look of Raven, dark or light
     *   theme.toggle
     *   theme:NAME                        Rtheme_use(NAME): a theme loaded or built in
     *   language:CODE                     Rlocale_set_language(CODE)
     *   open:ADDRESS                      Ropen_url (https:, mailto:, a file)
     *   copy:TEXT                         the text to the clipboard
     *   show:NAME, hide:NAME, toggle:NAME the widget named NAME in the window (hidden: Collapsed)
     *   enable:NAME, disable:NAME
     *   focus:NAME
     *   page:NAME=2, page:NAME=+1         what a Switcher, a TabView, a ComboBox or a ListView
     *                                     shows; +1 and -1 step, and go round
     *
     * A function bound to one of these names (Raction_bind) replaces it; the
     * default handler receives the others only.
     * enabled R_FALSE turns them all off: a design tool that shows
     * interfaces without running them. On by default.
     */
    public static void Raction_set_builtins(int enabled) {
        MethodHandle $h = Raction_set_builtins$;
        if ($h == null) Raction_set_builtins$ = $h = find("Raction_set_builtins", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Raction_is_builtin$;

    /**
     * Returns: R_TRUE when the name is one of the built-in actions (Raction_set_builtins), whether they are on or not.
     */
    public static int Raction_is_builtin(String name) {
        MethodHandle $h = Raction_is_builtin$;
        if ($h == null) Raction_is_builtin$ = $h = find("Raction_is_builtin", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            int $r = (int) $h.invokeExact(name$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Ropen_url$;

    /**
     * Opens an address with the program the user has for it: a web
     * page in the browser, a mailto: in the mail program, a file in its
     * application. Returns at once.
     * Returns: R_FALSE when the OS could not be asked (nothing tells whether
     * a program then really opened it).
     */
    public static int Ropen_url(String url) {
        MethodHandle $h = Ropen_url$;
        if ($h == null) Ropen_url$ = $h = find("Ropen_url", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment url$ = url == null ? MemorySegment.NULL : utf8($a, url);
            int $r = (int) $h.invokeExact(url$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rclipboard_set_text$;

    public static void Rclipboard_set_text(String text) {
        MethodHandle $h = Rclipboard_set_text$;
        if ($h == null) Rclipboard_set_text$ = $h = find("Rclipboard_set_text", FunctionDescriptor.ofVoid(ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            $h.invokeExact(text$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rclipboard_get_text$;

    /**
     * Returns: The clipboard text, or "" when empty. Valid until the next call.
     */
    public static String Rclipboard_get_text() {
        MethodHandle $h = Rclipboard_get_text$;
        if ($h == null) Rclipboard_get_text$ = $h = find("Rclipboard_get_text", FunctionDescriptor.of(ADDRESS));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact();
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rerror_get_last$;

    /**
     * Retrieves and clears the last recorded error.
     * detail   Optional output: human-readable description.
     * severity Optional output: severity of the error.
     * Returns: R_NO_ERROR when nothing was recorded.
     */
    public static int Rerror_get_last(String[] detail, int[] severity) {
        MethodHandle $h = Rerror_get_last$;
        if ($h == null) Rerror_get_last$ = $h = find("Rerror_get_last", FunctionDescriptor.of(JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment detail$ = detail == null ? MemorySegment.NULL : $a.allocate(ADDRESS);
            MemorySegment severity$ = severity == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            int $r = (int) $h.invokeExact(detail$, severity$);
            if (detail != null && detail.length > 0) detail[0] = text(detail$.get(ADDRESS, 0));
            if (severity != null && severity.length > 0) severity[0] = severity$.get(JAVA_INT, 0);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rerror_to_string$;

    public static String Rerror_to_string(int error) {
        MethodHandle $h = Rerror_to_string$;
        if ($h == null) Rerror_to_string$ = $h = find("Rerror_to_string", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(error);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rseverity_to_string$;

    public static String Rseverity_to_string(int severity) {
        MethodHandle $h = Rseverity_to_string$;
        if ($h == null) Rseverity_to_string$ = $h = find("Rseverity_to_string", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(severity);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rerror_set_callback$;

    public static void Rerror_set_callback(RErrorCallback callback) {
        MethodHandle $h = Rerror_set_callback$;
        if ($h == null) Rerror_set_callback$ = $h = find("Rerror_set_callback", FunctionDescriptor.ofVoid(ADDRESS));
        try {
            $h.invokeExact(upcall(callback));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rset_device_reset_callback$;

    public static void Rset_device_reset_callback(RDeviceResetCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rset_device_reset_callback$;
        if ($h == null) Rset_device_reset_callback$ = $h = find("Rset_device_reset_callback", FunctionDescriptor.ofVoid(ADDRESS, ADDRESS));
        try {
            $h.invokeExact(upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rset_device_reset_callback(RDeviceResetCallback callback) {
        Rset_device_reset_callback(callback, (MemorySegment) null);
    }

    private static MethodHandle Rwindow_get_frame_stats$;

    public static void Rwindow_get_frame_stats(int window, RFrameStats stats) {
        MethodHandle $h = Rwindow_get_frame_stats$;
        if ($h == null) Rwindow_get_frame_stats$ = $h = find("Rwindow_get_frame_stats", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try {
            $h.invokeExact(window, (MemorySegment) (stats == null ? MemorySegment.NULL : stats.segment()));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_create$;

    /**
     * Opens a top-level window.
     * width,height Client size in logical units.
     * flags        Bitmask of RWindowFlags.
     */
    public static int Rwindow_create(String title, int width, int height, int flags) {
        MethodHandle $h = Rwindow_create$;
        if ($h == null) Rwindow_create$ = $h = find("Rwindow_create", FunctionDescriptor.of(JAVA_INT, ADDRESS, JAVA_INT, JAVA_INT, JAVA_INT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment title$ = title == null ? MemorySegment.NULL : utf8($a, title);
            int $r = (int) $h.invokeExact(title$, width, height, flags);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_destroy$;

    /**
     * Closes a window and deletes its content widgets.
     */
    public static void Rwindow_destroy(int window) {
        MethodHandle $h = Rwindow_destroy$;
        if ($h == null) Rwindow_destroy$ = $h = find("Rwindow_destroy", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(window);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_is_valid$;

    public static int Rwindow_is_valid(int window) {
        MethodHandle $h = Rwindow_is_valid$;
        if ($h == null) Rwindow_is_valid$ = $h = find("Rwindow_is_valid", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_set_content$;

    /**
     * Sets the root widget of the window. The window takes ownership of
     * the widget; the previous content is deleted.
     */
    public static void Rwindow_set_content(int window, int widget) {
        MethodHandle $h = Rwindow_set_content$;
        if ($h == null) Rwindow_set_content$ = $h = find("Rwindow_set_content", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(window, widget);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_get_content$;

    public static int Rwindow_get_content(int window) {
        MethodHandle $h = Rwindow_get_content$;
        if ($h == null) Rwindow_get_content$ = $h = find("Rwindow_get_content", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_set_title$;

    public static void Rwindow_set_title(int window, String title) {
        MethodHandle $h = Rwindow_set_title$;
        if ($h == null) Rwindow_set_title$ = $h = find("Rwindow_set_title", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment title$ = title == null ? MemorySegment.NULL : utf8($a, title);
            $h.invokeExact(window, title$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_get_title$;

    public static String Rwindow_get_title(int window) {
        MethodHandle $h = Rwindow_get_title$;
        if ($h == null) Rwindow_get_title$ = $h = find("Rwindow_get_title", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(window);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_set_size$;

    public static void Rwindow_set_size(int window, int width, int height) {
        MethodHandle $h = Rwindow_set_size$;
        if ($h == null) Rwindow_set_size$ = $h = find("Rwindow_set_size", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(window, width, height);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_get_size$;

    public static void Rwindow_get_size(int window, int[] width, int[] height) {
        MethodHandle $h = Rwindow_get_size$;
        if ($h == null) Rwindow_get_size$ = $h = find("Rwindow_get_size", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment width$ = width == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            MemorySegment height$ = height == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            $h.invokeExact(window, width$, height$);
            if (width != null && width.length > 0) width[0] = width$.get(JAVA_INT, 0);
            if (height != null && height.length > 0) height[0] = height$.get(JAVA_INT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_set_min_size$;

    public static void Rwindow_set_min_size(int window, int width, int height) {
        MethodHandle $h = Rwindow_set_min_size$;
        if ($h == null) Rwindow_set_min_size$ = $h = find("Rwindow_set_min_size", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(window, width, height);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_set_position$;

    public static void Rwindow_set_position(int window, int x, int y) {
        MethodHandle $h = Rwindow_set_position$;
        if ($h == null) Rwindow_set_position$ = $h = find("Rwindow_set_position", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(window, x, y);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_get_position$;

    public static void Rwindow_get_position(int window, int[] x, int[] y) {
        MethodHandle $h = Rwindow_get_position$;
        if ($h == null) Rwindow_get_position$ = $h = find("Rwindow_get_position", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment x$ = x == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            MemorySegment y$ = y == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            $h.invokeExact(window, x$, y$);
            if (x != null && x.length > 0) x[0] = x$.get(JAVA_INT, 0);
            if (y != null && y.length > 0) y[0] = y$.get(JAVA_INT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_get_framebuffer_size$;

    /**
     * Size of the drawable area in pixels.
     */
    public static void Rwindow_get_framebuffer_size(int window, int[] width, int[] height) {
        MethodHandle $h = Rwindow_get_framebuffer_size$;
        if ($h == null) Rwindow_get_framebuffer_size$ = $h = find("Rwindow_get_framebuffer_size", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment width$ = width == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            MemorySegment height$ = height == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            $h.invokeExact(window, width$, height$);
            if (width != null && width.length > 0) width[0] = width$.get(JAVA_INT, 0);
            if (height != null && height.length > 0) height[0] = height$.get(JAVA_INT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_get_scale$;

    /**
     * Returns: Pixels per logical unit (monitor scale x global UI scale).
     */
    public static float Rwindow_get_scale(int window) {
        MethodHandle $h = Rwindow_get_scale$;
        if ($h == null) Rwindow_get_scale$ = $h = find("Rwindow_get_scale", FunctionDescriptor.of(JAVA_FLOAT, JAVA_INT));
        try {
            float $r = (float) $h.invokeExact(window);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_show$;

    public static void Rwindow_show(int window) {
        MethodHandle $h = Rwindow_show$;
        if ($h == null) Rwindow_show$ = $h = find("Rwindow_show", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(window);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_hide$;

    public static void Rwindow_hide(int window) {
        MethodHandle $h = Rwindow_hide$;
        if ($h == null) Rwindow_hide$ = $h = find("Rwindow_hide", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(window);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_maximize$;

    public static void Rwindow_maximize(int window) {
        MethodHandle $h = Rwindow_maximize$;
        if ($h == null) Rwindow_maximize$ = $h = find("Rwindow_maximize", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(window);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_minimize$;

    public static void Rwindow_minimize(int window) {
        MethodHandle $h = Rwindow_minimize$;
        if ($h == null) Rwindow_minimize$ = $h = find("Rwindow_minimize", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(window);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_restore$;

    public static void Rwindow_restore(int window) {
        MethodHandle $h = Rwindow_restore$;
        if ($h == null) Rwindow_restore$ = $h = find("Rwindow_restore", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(window);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_is_maximized$;

    public static int Rwindow_is_maximized(int window) {
        MethodHandle $h = Rwindow_is_maximized$;
        if ($h == null) Rwindow_is_maximized$ = $h = find("Rwindow_is_maximized", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_is_minimized$;

    public static int Rwindow_is_minimized(int window) {
        MethodHandle $h = Rwindow_is_minimized$;
        if ($h == null) Rwindow_is_minimized$ = $h = find("Rwindow_is_minimized", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_is_active$;

    /**
     * Returns: R_TRUE while the window is the one the user is working in.
     */
    public static int Rwindow_is_active(int window) {
        MethodHandle $h = Rwindow_is_active$;
        if ($h == null) Rwindow_is_active$ = $h = find("Rwindow_is_active", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_close$;

    /**
     * Asks to close the window, as the close button of its title bar
     * does: the close callback is asked first and may refuse.
     */
    public static void Rwindow_close(int window) {
        MethodHandle $h = Rwindow_close$;
        if ($h == null) Rwindow_close$ = $h = find("Rwindow_close", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(window);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_begin_move$;

    /**
     * Custom headers. Call these from a mouse-down handler, while the left
     * button is held: the window follows the pointer until the button is
     * released (the widget does not receive the release). A
     * R_WIDGET_HEADER_BAR and the widgets given to Rwidget_set_window_drag do
     * it by themselves.
     */
    public static void Rwindow_begin_move(int window) {
        MethodHandle $h = Rwindow_begin_move$;
        if ($h == null) Rwindow_begin_move$ = $h = find("Rwindow_begin_move", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(window);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_begin_resize$;

    public static void Rwindow_begin_resize(int window, int edge) {
        MethodHandle $h = Rwindow_begin_resize$;
        if ($h == null) Rwindow_begin_resize$ = $h = find("Rwindow_begin_resize", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(window, edge);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_set_icon$;

    /**
     * Icon of the window (title bar, task bar), from tightly packed
     * RGBA8 pixels; NULL goes back to the default: on Windows the first icon
     * of the executable, when it has one. Windows and X11.
     */
    public static void Rwindow_set_icon(int window, byte[] rgba, int width, int height) {
        MethodHandle $h = Rwindow_set_icon$;
        if ($h == null) Rwindow_set_icon$ = $h = find("Rwindow_set_icon", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, JAVA_INT, JAVA_INT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment rgba$ = rgba == null ? MemorySegment.NULL : bytes($a, rgba);
            $h.invokeExact(window, rgba$, width, height);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_set_clear_color$;

    public static void Rwindow_set_clear_color(int window, float r, float g, float b, float a) {
        MethodHandle $h = Rwindow_set_clear_color$;
        if ($h == null) Rwindow_set_clear_color$ = $h = find("Rwindow_set_clear_color", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(window, r, g, b, a);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_set_background$;

    /**
     * Image drawn behind the content, scaled to cover the window:
     * what the glass of a glass theme looks through. R_INVALID_ID removes it.
     */
    public static void Rwindow_set_background(int window, int texture) {
        MethodHandle $h = Rwindow_set_background$;
        if ($h == null) Rwindow_set_background$ = $h = find("Rwindow_set_background", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(window, texture);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_set_backdrop$;

    /**
     * Chooses what shows behind a window created with
     * R_WINDOW_TRANSPARENT (see RBackdrop). With anything but
     * R_BACKDROP_NONE the window is not cleared with its color and the
     * wallpaper of the theme is not drawn: only what the widgets draw hides
     * the screen. In XML: &lt;Window Backdrop="Blur"&gt;.
     * Returns: R_TRUE when the window system does it; R_FALSE (the window
     * then stays as it was) without R_WINDOW_TRANSPARENT, or when it cannot.
     */
    public static int Rwindow_set_backdrop(int window, int backdrop) {
        MethodHandle $h = Rwindow_set_backdrop$;
        if ($h == null) Rwindow_set_backdrop$ = $h = find("Rwindow_set_backdrop", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window, backdrop);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_get_backdrop$;

    public static int Rwindow_get_backdrop(int window) {
        MethodHandle $h = Rwindow_get_backdrop$;
        if ($h == null) Rwindow_get_backdrop$ = $h = find("Rwindow_get_backdrop", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_set_theme$;

    /**
     * Gives the window a theme of its own, by name ("Dark",
     * "WindowsLight", or a theme of Rtheme_load): everything inside it (its
     * content, its island, its menus, lists and dialogs, its background) is
     * drawn with that theme, while the other windows keep the one of the
     * application (Rtheme_use). A widget given Rwidget_set_theme keeps its own.
     * In XML: &lt;Window Theme="..."&gt;.
     * name NULL or "": the theme of the application again.
     * Returns: R_FALSE, after reporting an error, when no theme has that name.
     */
    public static int Rwindow_set_theme(int window, String name) {
        MethodHandle $h = Rwindow_set_theme$;
        if ($h == null) Rwindow_set_theme$ = $h = find("Rwindow_set_theme", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            int $r = (int) $h.invokeExact(window, name$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_get_theme$;

    /**
     * Returns: Name of the theme the window is drawn with (the one of the application when it has none of its own).
     */
    public static String Rwindow_get_theme(int window) {
        MethodHandle $h = Rwindow_get_theme$;
        if ($h == null) Rwindow_get_theme$ = $h = find("Rwindow_get_theme", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(window);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_has_own_theme$;

    /**
     * Returns: R_TRUE when the window has a theme of its own (Rwindow_set_theme).
     */
    public static int Rwindow_has_own_theme(int window) {
        MethodHandle $h = Rwindow_has_own_theme$;
        if ($h == null) Rwindow_has_own_theme$ = $h = find("Rwindow_has_own_theme", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_set_opacity$;

    /**
     * Opacity of the whole window, its frame included, from 0 to 1:
     * a window that fades, a tool that lets what is under it show.
     * Returns: R_FALSE when the window system cannot do it.
     */
    public static int Rwindow_set_opacity(int window, float opacity) {
        MethodHandle $h = Rwindow_set_opacity$;
        if ($h == null) Rwindow_set_opacity$ = $h = find("Rwindow_set_opacity", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_FLOAT));
        try {
            int $r = (int) $h.invokeExact(window, opacity);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_get_opacity$;

    public static float Rwindow_get_opacity(int window) {
        MethodHandle $h = Rwindow_get_opacity$;
        if ($h == null) Rwindow_get_opacity$ = $h = find("Rwindow_get_opacity", FunctionDescriptor.of(JAVA_FLOAT, JAVA_INT));
        try {
            float $r = (float) $h.invokeExact(window);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_set_topmost$;

    /**
     * Keeps the window above the other windows (splash screens, tools).
     * Returns: R_FALSE when the window system cannot do it.
     */
    public static int Rwindow_set_topmost(int window, int topmost) {
        MethodHandle $h = Rwindow_set_topmost$;
        if ($h == null) Rwindow_set_topmost$ = $h = find("Rwindow_set_topmost", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window, topmost);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_center$;

    /**
     * Centers the window on the work area of its monitor (no task bar).
     */
    public static void Rwindow_center(int window) {
        MethodHandle $h = Rwindow_center$;
        if ($h == null) Rwindow_center$ = $h = find("Rwindow_center", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(window);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_request_redraw$;

    /**
     * Asks for a new frame even if no widget changed. Thread-safe.
     */
    public static void Rwindow_request_redraw(int window) {
        MethodHandle $h = Rwindow_request_redraw$;
        if ($h == null) Rwindow_request_redraw$ = $h = find("Rwindow_request_redraw", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(window);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_set_continuous_redraw$;

    /**
     * R_TRUE redraws the window every frame (games, live viewports).
     */
    public static void Rwindow_set_continuous_redraw(int window, int enabled) {
        MethodHandle $h = Rwindow_set_continuous_redraw$;
        if ($h == null) Rwindow_set_continuous_redraw$ = $h = find("Rwindow_set_continuous_redraw", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(window, enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_set_close_callback$;

    public static void Rwindow_set_close_callback(int window, RWindowCloseCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rwindow_set_close_callback$;
        if ($h == null) Rwindow_set_close_callback$ = $h = find("Rwindow_set_close_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(window, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rwindow_set_close_callback(int window, RWindowCloseCallback callback) {
        Rwindow_set_close_callback(window, callback, (MemorySegment) null);
    }

    private static MethodHandle Rwindow_set_resize_callback$;

    public static void Rwindow_set_resize_callback(int window, RWindowResizeCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rwindow_set_resize_callback$;
        if ($h == null) Rwindow_set_resize_callback$ = $h = find("Rwindow_set_resize_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(window, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rwindow_set_resize_callback(int window, RWindowResizeCallback callback) {
        Rwindow_set_resize_callback(window, callback, (MemorySegment) null);
    }

    private static MethodHandle Rwindow_set_drop_callback$;

    /**
     * Takes what is dropped on the window from other applications,
     * where no widget takes it (Rwidget_set_drop_callback). NULL removes it.
     */
    public static void Rwindow_set_drop_callback(int window, RDropCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rwindow_set_drop_callback$;
        if ($h == null) Rwindow_set_drop_callback$ = $h = find("Rwindow_set_drop_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(window, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Takes what is dropped on the window from other applications,
     * where no widget takes it (Rwidget_set_drop_callback). NULL removes it.
     */
    public static void Rwindow_set_drop_callback(int window, RDropCallback callback) {
        Rwindow_set_drop_callback(window, callback, (MemorySegment) null);
    }

    private static MethodHandle Rwindow_set_user_handle$;

    public static void Rwindow_set_user_handle(int window, MemorySegment handle) {
        MethodHandle $h = Rwindow_set_user_handle$;
        if ($h == null) Rwindow_set_user_handle$ = $h = find("Rwindow_set_user_handle", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try {
            $h.invokeExact(window, (MemorySegment) (handle == null ? MemorySegment.NULL : handle));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_get_user_handle$;

    public static MemorySegment Rwindow_get_user_handle(int window) {
        MethodHandle $h = Rwindow_get_user_handle$;
        if ($h == null) Rwindow_get_user_handle$ = $h = find("Rwindow_get_user_handle", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(window);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_get_native_handle$;

    /**
     * Fills `native` with the OS handles of the window.
     */
    public static int Rwindow_get_native_handle(int window, RNativeWindow native_) {
        MethodHandle $h = Rwindow_get_native_handle$;
        if ($h == null) Rwindow_get_native_handle$ = $h = find("Rwindow_get_native_handle", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS));
        try {
            int $r = (int) $h.invokeExact(window, (MemorySegment) (native_ == null ? MemorySegment.NULL : native_.segment()));
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_capture$;

    /**
     * Reads back the last rendered frame as tightly packed RGBA8 pixels.
     * Blocks until the frame is rendered. Free the result with Rpixels_free.
     * The alpha is 255, except for a window that is seen through
     * (R_WINDOW_TRANSPARENT): its pixels are then premultiplied by their
     * alpha, as the window system receives them.
     * Returns: NULL when the backend cannot capture.
     */
    public static MemorySegment Rwindow_capture(int window, int[] width, int[] height) {
        MethodHandle $h = Rwindow_capture$;
        if ($h == null) Rwindow_capture$ = $h = find("Rwindow_capture", FunctionDescriptor.of(ADDRESS, JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment width$ = width == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            MemorySegment height$ = height == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            MemorySegment $r = (MemorySegment) $h.invokeExact(window, width$, height$);
            if (width != null && width.length > 0) width[0] = width$.get(JAVA_INT, 0);
            if (height != null && height.length > 0) height[0] = height$.get(JAVA_INT, 0);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_get_focused_widget$;

    /**
     * Widget that has the keyboard focus in the window, or R_INVALID_ID.
     */
    public static int Rwindow_get_focused_widget(int window) {
        MethodHandle $h = Rwindow_get_focused_widget$;
        if ($h == null) Rwindow_get_focused_widget$ = $h = find("Rwindow_get_focused_widget", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_get_hovered_widget$;

    /**
     * Deepest widget under the cursor, or R_INVALID_ID.
     */
    public static int Rwindow_get_hovered_widget(int window) {
        MethodHandle $h = Rwindow_get_hovered_widget$;
        if ($h == null) Rwindow_get_hovered_widget$ = $h = find("Rwindow_get_hovered_widget", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_get_mouse_position$;

    public static void Rwindow_get_mouse_position(int window, float[] x, float[] y) {
        MethodHandle $h = Rwindow_get_mouse_position$;
        if ($h == null) Rwindow_get_mouse_position$ = $h = find("Rwindow_get_mouse_position", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment x$ = x == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment y$ = y == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            $h.invokeExact(window, x$, y$);
            if (x != null && x.length > 0) x[0] = x$.get(JAVA_FLOAT, 0);
            if (y != null && y.length > 0) y[0] = y$.get(JAVA_FLOAT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_inject_mouse_move$;

    /**
     * Inject input as if it came from the OS. Positions are in logical units.
     * Useful for automated tests, replays and when Raven is driven by a host
     * application.
     */
    public static void Rwindow_inject_mouse_move(int window, float x, float y) {
        MethodHandle $h = Rwindow_inject_mouse_move$;
        if ($h == null) Rwindow_inject_mouse_move$ = $h = find("Rwindow_inject_mouse_move", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(window, x, y);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_inject_mouse_button$;

    public static void Rwindow_inject_mouse_button(int window, int button, int pressed) {
        MethodHandle $h = Rwindow_inject_mouse_button$;
        if ($h == null) Rwindow_inject_mouse_button$ = $h = find("Rwindow_inject_mouse_button", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(window, button, pressed);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_inject_mouse_wheel$;

    public static void Rwindow_inject_mouse_wheel(int window, float wheel_x, float wheel_y) {
        MethodHandle $h = Rwindow_inject_mouse_wheel$;
        if ($h == null) Rwindow_inject_mouse_wheel$ = $h = find("Rwindow_inject_mouse_wheel", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(window, wheel_x, wheel_y);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_inject_key$;

    public static void Rwindow_inject_key(int window, int key, int pressed, int modifiers) {
        MethodHandle $h = Rwindow_inject_key$;
        if ($h == null) Rwindow_inject_key$ = $h = find("Rwindow_inject_key", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(window, key, pressed, modifiers);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_inject_text$;

    public static void Rwindow_inject_text(int window, String text) {
        MethodHandle $h = Rwindow_inject_text$;
        if ($h == null) Rwindow_inject_text$ = $h = find("Rwindow_inject_text", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            $h.invokeExact(window, text$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_inject_drag$;

    /**
     * A drag from another application moves over the window
     * (`inside` R_TRUE) or leaves it.
     * Returns: R_TRUE when a drop at this position would be accepted.
     */
    public static int Rwindow_inject_drag(int window, float x, float y, int inside) {
        MethodHandle $h = Rwindow_inject_drag$;
        if ($h == null) Rwindow_inject_drag$ = $h = find("Rwindow_inject_drag", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window, x, y, inside);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_inject_drop$;

    /**
     * Drops files and / or text at a position. Returns R_TRUE when a target took them.
     */
    public static int Rwindow_inject_drop(int window, float x, float y, String[] paths, String text) {
        MethodHandle $h = Rwindow_inject_drop$;
        if ($h == null) Rwindow_inject_drop$ = $h = find("Rwindow_inject_drop", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, ADDRESS, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment paths$ = texts($a, paths);
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            int $r = (int) $h.invokeExact(window, x, y, paths$, (int) (paths$.byteSize() / ADDRESS.byteSize()), text$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_is_key_down$;

    /**
     * R_TRUE while the key is held down in the window (cleared when the window loses the keyboard).
     */
    public static int Rwindow_is_key_down(int window, int key) {
        MethodHandle $h = Rwindow_is_key_down$;
        if ($h == null) Rwindow_is_key_down$ = $h = find("Rwindow_is_key_down", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window, key);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_is_mouse_button_down$;

    /**
     * R_TRUE while the mouse button is held down over (or captured by) the window.
     */
    public static int Rwindow_is_mouse_button_down(int window, int button) {
        MethodHandle $h = Rwindow_is_mouse_button_down$;
        if ($h == null) Rwindow_is_mouse_button_down$ = $h = find("Rwindow_is_mouse_button_down", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window, button);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_get_modifiers$;

    /**
     * Modifiers (RModifier) of the last input event.
     */
    public static int Rwindow_get_modifiers(int window) {
        MethodHandle $h = Rwindow_get_modifiers$;
        if ($h == null) Rwindow_get_modifiers$ = $h = find("Rwindow_get_modifiers", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_is_mouse_inside$;

    /**
     * R_TRUE while the pointer is over the client area.
     */
    public static int Rwindow_is_mouse_inside(int window) {
        MethodHandle $h = Rwindow_is_mouse_inside$;
        if ($h == null) Rwindow_is_mouse_inside$ = $h = find("Rwindow_is_mouse_inside", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_wants_text_input$;

    /**
     * R_TRUE when the keyboard focus is in a text input, spin box or text editor.
     */
    public static int Rwindow_wants_text_input(int window) {
        MethodHandle $h = Rwindow_wants_text_input$;
        if ($h == null) Rwindow_wants_text_input$ = $h = find("Rwindow_wants_text_input", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_has_popup$;

    /**
     * R_TRUE while a menu, list or dialog is open over the window.
     */
    public static int Rwindow_has_popup(int window) {
        MethodHandle $h = Rwindow_has_popup$;
        if ($h == null) Rwindow_has_popup$ = $h = find("Rwindow_has_popup", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_get_text_input_rect$;

    /**
     * Last painted caret in window pixels (x, y, width, height), including scrolling and scale.
     * Call after Rdraw_frame. Independent of the blink phase. Returns R_FALSE and zeros the
     * outputs when there is no visible editable caret, the window is inactive, or focus changed
     * since painting. Any output pointer may be NULL.
     */
    public static int Rwindow_get_text_input_rect(int window, int[] x, int[] y, int[] width, int[] height) {
        MethodHandle $h = Rwindow_get_text_input_rect$;
        if ($h == null) Rwindow_get_text_input_rect$ = $h = find("Rwindow_get_text_input_rect", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment x$ = x == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            MemorySegment y$ = y == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            MemorySegment width$ = width == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            MemorySegment height$ = height == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            int $r = (int) $h.invokeExact(window, x$, y$, width$, height$);
            if (x != null && x.length > 0) x[0] = x$.get(JAVA_INT, 0);
            if (y != null && y.length > 0) y[0] = y$.get(JAVA_INT, 0);
            if (width != null && width.length > 0) width[0] = width$.get(JAVA_INT, 0);
            if (height != null && height.length > 0) height[0] = height$.get(JAVA_INT, 0);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_get_popup_count$;

    /**
     * Returns: Number of open in-window popups (menus, combo lists, pickers), bottom to top.
     */
    public static int Rwindow_get_popup_count(int window) {
        MethodHandle $h = Rwindow_get_popup_count$;
        if ($h == null) Rwindow_get_popup_count$ = $h = find("Rwindow_get_popup_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_get_popup$;

    /**
     * Returns: Root widget of popup `index`, whose rect is in window coordinates; R_INVALID_ID out of range.
     */
    public static int Rwindow_get_popup(int window, int index) {
        MethodHandle $h = Rwindow_get_popup$;
        if ($h == null) Rwindow_get_popup$ = $h = find("Rwindow_get_popup", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window, index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_set_tab_navigation$;

    /**
     * R_FALSE: Tab no longer moves the keyboard focus (the application uses it as a shortcut). Default R_TRUE.
     */
    public static void Rwindow_set_tab_navigation(int window, int enabled) {
        MethodHandle $h = Rwindow_set_tab_navigation$;
        if ($h == null) Rwindow_set_tab_navigation$ = $h = find("Rwindow_set_tab_navigation", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(window, enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_set_event_hook$;

    public static void Rwindow_set_event_hook(int window, RWindowEventHook hook, MemorySegment user_data) {
        MethodHandle $h = Rwindow_set_event_hook$;
        if ($h == null) Rwindow_set_event_hook$ = $h = find("Rwindow_set_event_hook", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(window, upcall(hook), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rwindow_set_event_hook(int window, RWindowEventHook hook) {
        Rwindow_set_event_hook(window, hook, (MemorySegment) null);
    }

    private static MethodHandle Rdrag_begin$;

    /**
     * Starts dragging `data` from `source` (call it while a mouse button
     * is held, typically from a mouse-move event once the pointer has moved
     * a few units). Until the button is released, the widgets with a drop
     * callback (Rwidget_set_drop_callback) under the pointer receive
     * R_DROP_ENTER / MOVE / LEAVE and, on release, R_DROP_DROPPED, with
     * `drag_type` set and `text` = `data`. Returning R_FALSE from the callback
     * refuses the drop there. Escape cancels. `label` follows the pointer.
     * Returns: R_FALSE when the widget is not in a window.
     */
    public static int Rdrag_begin(int source, String type, String data, String label) {
        MethodHandle $h = Rdrag_begin$;
        if ($h == null) Rdrag_begin$ = $h = find("Rdrag_begin", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment type$ = type == null ? MemorySegment.NULL : utf8($a, type);
            MemorySegment data$ = data == null ? MemorySegment.NULL : utf8($a, data);
            MemorySegment label$ = label == null ? MemorySegment.NULL : utf8($a, label);
            int $r = (int) $h.invokeExact(source, type$, data$, label$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdrag_is_active$;

    /**
     * R_TRUE while a drag started by Rdrag_begin is in progress in the window.
     */
    public static int Rdrag_is_active(int window) {
        MethodHandle $h = Rdrag_is_active$;
        if ($h == null) Rdrag_is_active$ = $h = find("Rdrag_is_active", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdrag_get_type$;

    /**
     * Type of the drag in progress, or NULL.
     */
    public static String Rdrag_get_type(int window) {
        MethodHandle $h = Rdrag_get_type$;
        if ($h == null) Rdrag_get_type$ = $h = find("Rdrag_get_type", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(window);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdrag_get_data$;

    /**
     * Data of the drag in progress, or NULL.
     */
    public static String Rdrag_get_data(int window) {
        MethodHandle $h = Rdrag_get_data$;
        if ($h == null) Rdrag_get_data$ = $h = find("Rdrag_get_data", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(window);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdrag_cancel$;

    /**
     * Ends the drag in progress without dropping (the target receives R_DROP_LEAVE).
     */
    public static void Rdrag_cancel(int window) {
        MethodHandle $h = Rdrag_cancel$;
        if ($h == null) Rdrag_cancel$ = $h = find("Rdrag_cancel", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(window);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_register_shortcut$;

    public static void Rwindow_register_shortcut(int window, int key, int modifiers, String action) {
        MethodHandle $h = Rwindow_register_shortcut$;
        if ($h == null) Rwindow_register_shortcut$ = $h = find("Rwindow_register_shortcut", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment action$ = action == null ? MemorySegment.NULL : utf8($a, action);
            $h.invokeExact(window, key, modifiers, action$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_unregister_shortcut$;

    public static void Rwindow_unregister_shortcut(int window, int key, int modifiers) {
        MethodHandle $h = Rwindow_unregister_shortcut$;
        if ($h == null) Rwindow_unregister_shortcut$ = $h = find("Rwindow_unregister_shortcut", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(window, key, modifiers);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtouch_get_settings$;

    /**
     * The settings every window follows, unless it has its own.
     */
    public static void Rtouch_get_settings(RTouchSettings settings) {
        MethodHandle $h = Rtouch_get_settings$;
        if ($h == null) Rtouch_get_settings$ = $h = find("Rtouch_get_settings", FunctionDescriptor.ofVoid(ADDRESS));
        try {
            $h.invokeExact((MemorySegment) (settings == null ? MemorySegment.NULL : settings.segment()));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtouch_set_settings$;

    /**
     * Changes them; NULL puts back the defaults.
     */
    public static void Rtouch_set_settings(RTouchSettings settings) {
        MethodHandle $h = Rtouch_set_settings$;
        if ($h == null) Rtouch_set_settings$ = $h = find("Rtouch_set_settings", FunctionDescriptor.ofVoid(ADDRESS));
        try {
            $h.invokeExact((MemorySegment) (settings == null ? MemorySegment.NULL : settings.segment()));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtouch_get_default_settings$;

    /**
     * The defaults, to start from.
     */
    public static void Rtouch_get_default_settings(RTouchSettings settings) {
        MethodHandle $h = Rtouch_get_default_settings$;
        if ($h == null) Rtouch_get_default_settings$ = $h = find("Rtouch_get_default_settings", FunctionDescriptor.ofVoid(ADDRESS));
        try {
            $h.invokeExact((MemorySegment) (settings == null ? MemorySegment.NULL : settings.segment()));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_set_touch_settings$;

    /**
     * Settings of one window; NULL: it follows Rtouch_set_settings again.
     */
    public static void Rwindow_set_touch_settings(int window, RTouchSettings settings) {
        MethodHandle $h = Rwindow_set_touch_settings$;
        if ($h == null) Rwindow_set_touch_settings$ = $h = find("Rwindow_set_touch_settings", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try {
            $h.invokeExact(window, (MemorySegment) (settings == null ? MemorySegment.NULL : settings.segment()));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_get_touch_settings$;

    /**
     * The settings the window follows (its own, or those of the application).
     */
    public static void Rwindow_get_touch_settings(int window, RTouchSettings settings) {
        MethodHandle $h = Rwindow_get_touch_settings$;
        if ($h == null) Rwindow_get_touch_settings$ = $h = find("Rwindow_get_touch_settings", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try {
            $h.invokeExact(window, (MemorySegment) (settings == null ? MemorySegment.NULL : settings.segment()));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_touch_mode$;

    /**
     * What a finger does on the widget and what it contains (default R_TOUCH_MODE_AUTO: as its parent says).
     */
    public static void Rwidget_set_touch_mode(int widget, int mode) {
        MethodHandle $h = Rwidget_set_touch_mode$;
        if ($h == null) Rwidget_set_touch_mode$ = $h = find("Rwidget_set_touch_mode", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(widget, mode);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_touch_mode$;

    public static int Rwidget_get_touch_mode(int widget) {
        MethodHandle $h = Rwidget_get_touch_mode$;
        if ($h == null) Rwidget_get_touch_mode$ = $h = find("Rwidget_get_touch_mode", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(widget);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_touch_gestures$;

    /**
     * R_FALSE: two fingers neither zoom nor pan the widget (default R_TRUE).
     */
    public static void Rwidget_set_touch_gestures(int widget, int enabled) {
        MethodHandle $h = Rwidget_set_touch_gestures$;
        if ($h == null) Rwidget_set_touch_gestures$ = $h = find("Rwidget_set_touch_gestures", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(widget, enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_touch_gestures$;

    public static int Rwidget_get_touch_gestures(int widget) {
        MethodHandle $h = Rwidget_get_touch_gestures$;
        if ($h == null) Rwidget_get_touch_gestures$ = $h = find("Rwidget_get_touch_gestures", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(widget);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_pen_mouse$;

    /**
     * R_FALSE: the pen is not a mouse on the widget and what it contains; only its pen callback sees it (default R_TRUE).
     */
    public static void Rwidget_set_pen_mouse(int widget, int enabled) {
        MethodHandle $h = Rwidget_set_pen_mouse$;
        if ($h == null) Rwidget_set_pen_mouse$ = $h = find("Rwidget_set_pen_mouse", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(widget, enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_pen_mouse$;

    public static int Rwidget_get_pen_mouse(int widget) {
        MethodHandle $h = Rwidget_get_pen_mouse$;
        if ($h == null) Rwidget_get_pen_mouse$ = $h = find("Rwidget_get_pen_mouse", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(widget);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_touch_callback$;

    public static void Rwidget_set_touch_callback(int widget, RTouchCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rwidget_set_touch_callback$;
        if ($h == null) Rwidget_set_touch_callback$ = $h = find("Rwidget_set_touch_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(widget, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rwidget_set_touch_callback(int widget, RTouchCallback callback) {
        Rwidget_set_touch_callback(widget, callback, (MemorySegment) null);
    }

    private static MethodHandle Rwidget_set_pen_callback$;

    public static void Rwidget_set_pen_callback(int widget, RPenCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rwidget_set_pen_callback$;
        if ($h == null) Rwidget_set_pen_callback$ = $h = find("Rwidget_set_pen_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(widget, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rwidget_set_pen_callback(int widget, RPenCallback callback) {
        Rwidget_set_pen_callback(widget, callback, (MemorySegment) null);
    }

    private static MethodHandle Rwidget_set_gesture_callback$;

    public static void Rwidget_set_gesture_callback(int widget, RGestureCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rwidget_set_gesture_callback$;
        if ($h == null) Rwidget_set_gesture_callback$ = $h = find("Rwidget_set_gesture_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(widget, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rwidget_set_gesture_callback(int widget, RGestureCallback callback) {
        Rwidget_set_gesture_callback(widget, callback, (MemorySegment) null);
    }

    private static MethodHandle Rwindow_get_touch_count$;

    /**
     * Fingers on the window now.
     */
    public static int Rwindow_get_touch_count(int window) {
        MethodHandle $h = Rwindow_get_touch_count$;
        if ($h == null) Rwindow_get_touch_count$ = $h = find("Rwindow_get_touch_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_is_pen_in_range$;

    /**
     * R_TRUE while a pen is near the screen over the window.
     */
    public static int Rwindow_is_pen_in_range(int window) {
        MethodHandle $h = Rwindow_is_pen_in_range$;
        if ($h == null) Rwindow_is_pen_in_range$ = $h = find("Rwindow_is_pen_in_range", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_is_touch_active$;

    /**
     * R_TRUE when the pointer last used in the window was a finger (the interface may then be made larger).
     */
    public static int Rwindow_is_touch_active(int window) {
        MethodHandle $h = Rwindow_is_touch_active$;
        if ($h == null) Rwindow_is_touch_active$ = $h = find("Rwindow_is_touch_active", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_inject_touch$;

    /**
     * A finger: R_TOUCH_DOWN, then R_TOUCH_MOVE..., then R_TOUCH_UP, with the same `id`.
     */
    public static void Rwindow_inject_touch(int window, int id, int phase, float x, float y) {
        MethodHandle $h = Rwindow_inject_touch$;
        if ($h == null) Rwindow_inject_touch$ = $h = find("Rwindow_inject_touch", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(window, id, phase, x, y);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_inject_pen$;

    /**
     * A pen: `pressure` 0..1 as the pen would give it, `buttons` a bitmask of RPenButton.
     */
    public static void Rwindow_inject_pen(int window, int phase, float x, float y, float pressure, float tilt_x, float tilt_y, int buttons, int eraser) {
        MethodHandle $h = Rwindow_inject_pen$;
        if ($h == null) Rwindow_inject_pen$ = $h = find("Rwindow_inject_pen", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(window, phase, x, y, pressure, tilt_x, tilt_y, buttons, eraser);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_advance_touch$;

    /**
     * Advances the inertia of flicked content by `seconds` at once,
     * and fires a long press that is due, without waiting (tests).
     */
    public static void Rwindow_advance_touch(int window, double seconds) {
        MethodHandle $h = Rwindow_advance_touch$;
        if ($h == null) Rwindow_advance_touch$ = $h = find("Rwindow_advance_touch", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_DOUBLE));
        try {
            $h.invokeExact(window, seconds);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlook_set$;

    /**
     * Changes the look of the application. The theme file in use, if
     * it names no look of its own, is laid again over the new one; colors
     * and metrics changed with Rtheme_set_... are lost; what was given to
     * widgets (Rwidget_set_color...) is kept.
     *
     *   Rlook_set(R_LOOK_NATIVE_LIGHT);   the look of the platform, light
     *
     * Theme files that name no look (no Look attribute) follow it too, so a
     * part of a window given one (Theme="paper.xml") changes with it.
     */
    public static void Rlook_set(int look) {
        MethodHandle $h = Rlook_set$;
        if ($h == null) Rlook_set$ = $h = find("Rlook_set", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(look);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlook_get$;

    /**
     * The look given to Rlook_set (R_LOOK_RAVEN_DARK at start).
     */
    public static int Rlook_get() {
        MethodHandle $h = Rlook_get$;
        if ($h == null) Rlook_get$ = $h = find("Rlook_get", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_apply$;

    /**
     * &#64;deprecated Rlook_set: a built-in theme is a look of Raven.
     */
    public static void Rtheme_apply(int theme) {
        MethodHandle $h = Rtheme_apply$;
        if ($h == null) Rtheme_apply$ = $h = find("Rtheme_apply", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(theme);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_set_color$;

    public static void Rtheme_set_color(int role, float r, float g, float b, float a) {
        MethodHandle $h = Rtheme_set_color$;
        if ($h == null) Rtheme_set_color$ = $h = find("Rtheme_set_color", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(role, r, g, b, a);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_get_color$;

    public static void Rtheme_get_color(int role, float[] r, float[] g, float[] b, float[] a) {
        MethodHandle $h = Rtheme_get_color$;
        if ($h == null) Rtheme_get_color$ = $h = find("Rtheme_get_color", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment r$ = r == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment g$ = g == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment b$ = b == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment a$ = a == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            $h.invokeExact(role, r$, g$, b$, a$);
            if (r != null && r.length > 0) r[0] = r$.get(JAVA_FLOAT, 0);
            if (g != null && g.length > 0) g[0] = g$.get(JAVA_FLOAT, 0);
            if (b != null && b.length > 0) b[0] = b$.get(JAVA_FLOAT, 0);
            if (a != null && a.length > 0) a[0] = a$.get(JAVA_FLOAT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_set_metric$;

    public static void Rtheme_set_metric(int metric, float value) {
        MethodHandle $h = Rtheme_set_metric$;
        if ($h == null) Rtheme_set_metric$ = $h = find("Rtheme_set_metric", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(metric, value);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_get_metric$;

    public static float Rtheme_get_metric(int metric) {
        MethodHandle $h = Rtheme_get_metric$;
        if ($h == null) Rtheme_get_metric$ = $h = find("Rtheme_get_metric", FunctionDescriptor.of(JAVA_FLOAT, JAVA_INT));
        try {
            float $r = (float) $h.invokeExact(metric);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_set_material$;

    /**
     * Material of every widget that was not given one with
     * Rwidget_set_material. Rtheme_apply sets it: solid for the flat
     * themes, frosted glass for the glass themes.
     */
    public static void Rtheme_set_material(int material) {
        MethodHandle $h = Rtheme_set_material$;
        if ($h == null) Rtheme_set_material$ = $h = find("Rtheme_set_material", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(material);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_get_material$;

    public static int Rtheme_get_material() {
        MethodHandle $h = Rtheme_get_material$;
        if ($h == null) Rtheme_get_material$ = $h = find("Rtheme_get_material", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_set_font$;

    /**
     * Font used by widgets that have no font of their own.
     */
    public static void Rtheme_set_font(int font) {
        MethodHandle $h = Rtheme_set_font$;
        if ($h == null) Rtheme_set_font$ = $h = find("Rtheme_set_font", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(font);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_get_font$;

    public static int Rtheme_get_font() {
        MethodHandle $h = Rtheme_get_font$;
        if ($h == null) Rtheme_get_font$ = $h = find("Rtheme_get_font", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_set_accent$;

    /**
     * Sets the accent color and everything that follows from it: its
     * hovered and pressed shades, the focus outline, the selection, the links.
     */
    public static void Rtheme_set_accent(float r, float g, float b) {
        MethodHandle $h = Rtheme_set_accent$;
        if ($h == null) Rtheme_set_accent$ = $h = find("Rtheme_set_accent", FunctionDescriptor.ofVoid(JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(r, g, b);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_set_dark$;

    /**
     * Tells whether the theme is a dark one. It decides the tint of
     * the glass, the colors of the code and which wallpaper the glass themes
     * use. Rtheme_apply and the theme files set it.
     */
    public static void Rtheme_set_dark(int dark) {
        MethodHandle $h = Rtheme_set_dark$;
        if ($h == null) Rtheme_set_dark$ = $h = find("Rtheme_set_dark", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(dark);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_is_dark$;

    public static int Rtheme_is_dark() {
        MethodHandle $h = Rtheme_is_dark$;
        if ($h == null) Rtheme_is_dark$ = $h = find("Rtheme_is_dark", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_set_wallpaper$;

    /**
     * Image drawn behind the content of the windows that have no
     * background of their own, or R_INVALID_ID for none (the window color).
     */
    public static void Rtheme_set_wallpaper(int texture) {
        MethodHandle $h = Rtheme_set_wallpaper$;
        if ($h == null) Rtheme_set_wallpaper$ = $h = find("Rtheme_set_wallpaper", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(texture);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_get_wallpaper$;

    public static int Rtheme_get_wallpaper() {
        MethodHandle $h = Rtheme_get_wallpaper$;
        if ($h == null) Rtheme_get_wallpaper$ = $h = find("Rtheme_get_wallpaper", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_set_style_color$;

    /**
     * Gives a color to the widgets of a type and / or of a class,
     * instead of the color of the theme.
     * widget      Tag of a type of widget ("Button"), or NULL for every type.
     * style_class A class (Rwidget_set_class, Class="..."), or NULL
     *                    for the widgets of the type whatever their classes.
     *
     * When several styles give the same value to a widget, the one with a
     * type and a class wins over the one with a class, which wins over the
     * one with a type; between equals, the last one set wins.
     */
    public static void Rtheme_set_style_color(String widget, String style_class, int role, float r, float g, float b, float a) {
        MethodHandle $h = Rtheme_set_style_color$;
        if ($h == null) Rtheme_set_style_color$ = $h = find("Rtheme_set_style_color", FunctionDescriptor.ofVoid(ADDRESS, ADDRESS, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment widget$ = widget == null ? MemorySegment.NULL : utf8($a, widget);
            MemorySegment style_class$ = style_class == null ? MemorySegment.NULL : utf8($a, style_class);
            $h.invokeExact(widget$, style_class$, role, r, g, b, a);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_set_style_metric$;

    public static void Rtheme_set_style_metric(String widget, String style_class, int metric, float value) {
        MethodHandle $h = Rtheme_set_style_metric$;
        if ($h == null) Rtheme_set_style_metric$ = $h = find("Rtheme_set_style_metric", FunctionDescriptor.ofVoid(ADDRESS, ADDRESS, JAVA_INT, JAVA_FLOAT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment widget$ = widget == null ? MemorySegment.NULL : utf8($a, widget);
            MemorySegment style_class$ = style_class == null ? MemorySegment.NULL : utf8($a, style_class);
            $h.invokeExact(widget$, style_class$, metric, value);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_set_style_material$;

    /**
     * R_MATERIAL_DEFAULT removes the material of the style.
     */
    public static void Rtheme_set_style_material(String widget, String style_class, int material) {
        MethodHandle $h = Rtheme_set_style_material$;
        if ($h == null) Rtheme_set_style_material$ = $h = find("Rtheme_set_style_material", FunctionDescriptor.ofVoid(ADDRESS, ADDRESS, JAVA_INT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment widget$ = widget == null ? MemorySegment.NULL : utf8($a, widget);
            MemorySegment style_class$ = style_class == null ? MemorySegment.NULL : utf8($a, style_class);
            $h.invokeExact(widget$, style_class$, material);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_clear_styles$;

    /**
     * Removes every style of the theme. Rtheme_apply and Rtheme_use do it too.
     */
    public static void Rtheme_clear_styles() {
        MethodHandle $h = Rtheme_clear_styles$;
        if ($h == null) Rtheme_clear_styles$ = $h = find("Rtheme_clear_styles", FunctionDescriptor.ofVoid());
        try {
            $h.invokeExact();
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_load$;

    /**
     * Reads a theme file and keeps the theme under its name (the Name
     * attribute, else the name of the file). It does not change the theme in
     * use unless a theme of that name is the one in use.
     *
     * The file is watched like the interface files (Rxml_set_live_reload):
     * saving it updates every widget that uses the theme.
     * Returns: The name of the theme, valid until Rshutdown, or NULL after
     * reporting an error.
     */
    public static String Rtheme_load(String path) {
        MethodHandle $h = Rtheme_load$;
        if ($h == null) Rtheme_load$ = $h = find("Rtheme_load", FunctionDescriptor.of(ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment path$ = path == null ? MemorySegment.NULL : utf8($a, path);
            MemorySegment $r = (MemorySegment) $h.invokeExact(path$);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_load_from_memory$;

    /**
     * Same, from a text. `size` may be 0 for a null-terminated text.
     */
    public static String Rtheme_load_from_memory(String xml) {
        MethodHandle $h = Rtheme_load_from_memory$;
        if ($h == null) Rtheme_load_from_memory$ = $h = find("Rtheme_load_from_memory", FunctionDescriptor.of(ADDRESS, ADDRESS, JAVA_LONG));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment xml$ = xml == null ? MemorySegment.NULL : utf8($a, xml);
            MemorySegment $r = (MemorySegment) $h.invokeExact(xml$, (long) (xml$.equals(MemorySegment.NULL) ? 0L : xml$.byteSize() - 1));
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_use$;

    /**
     * Makes a named theme the theme of the application: a theme
     * given by Rtheme_load (laid over the look in use when it names no
     * look), or the name of a look ("WindowsLight", "RavenDark"; the old
     * "Dark", "Light", "GlassDark"... too), which is then Rlook_set.
     * Values changed with Rtheme_set_... are lost.
     * Returns: R_FALSE, after reporting an error, when no theme has that name.
     */
    public static int Rtheme_use(String name) {
        MethodHandle $h = Rtheme_use$;
        if ($h == null) Rtheme_use$ = $h = find("Rtheme_use", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            int $r = (int) $h.invokeExact(name$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_get_name$;

    /**
     * Name of the theme in use ("Dark" at start).
     */
    public static String Rtheme_get_name() {
        MethodHandle $h = Rtheme_get_name$;
        if ($h == null) Rtheme_get_name$ = $h = find("Rtheme_get_name", FunctionDescriptor.of(ADDRESS));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact();
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_get_count$;

    /**
     * Number of named themes, the four built-in ones included.
     */
    public static int Rtheme_get_count() {
        MethodHandle $h = Rtheme_get_count$;
        if ($h == null) Rtheme_get_count$ = $h = find("Rtheme_get_count", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_get_name_at$;

    /**
     * Name of the index-th one, or NULL.
     */
    public static String Rtheme_get_name_at(int index) {
        MethodHandle $h = Rtheme_get_name_at$;
        if ($h == null) Rtheme_get_name_at$ = $h = find("Rtheme_get_name_at", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(index);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_save$;

    /**
     * Writes the theme in use, with every change made since it was
     * applied, as a theme file.
     * name Name given to the theme in the file; NULL keeps the current one.
     */
    public static int Rtheme_save(String path, String name) {
        MethodHandle $h = Rtheme_save$;
        if ($h == null) Rtheme_save$ = $h = find("Rtheme_save", FunctionDescriptor.of(JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment path$ = path == null ? MemorySegment.NULL : utf8($a, path);
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            int $r = (int) $h.invokeExact(path$, name$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfont_create$;

    /**
     * Creates a font from the content of a .ttf / .otf file.
     * The buffer is copied. Glyphs are rasterized on demand, at the exact
     * pixel size needed by each window scale, into a shared atlas.
     * Returns: R_INVALID_ID on failure.
     */
    public static int Rfont_create(byte[] data) {
        MethodHandle $h = Rfont_create$;
        if ($h == null) Rfont_create$ = $h = find("Rfont_create", FunctionDescriptor.of(JAVA_INT, ADDRESS, JAVA_LONG));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment data$ = data == null ? MemorySegment.NULL : bytes($a, data);
            int $r = (int) $h.invokeExact(data$, data$.byteSize());
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfont_create_from_file$;

    public static int Rfont_create_from_file(String path) {
        MethodHandle $h = Rfont_create_from_file$;
        if ($h == null) Rfont_create_from_file$ = $h = find("Rfont_create_from_file", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment path$ = path == null ? MemorySegment.NULL : utf8($a, path);
            int $r = (int) $h.invokeExact(path$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfont_destroy$;

    public static void Rfont_destroy(int font) {
        MethodHandle $h = Rfont_destroy$;
        if ($h == null) Rfont_destroy$ = $h = find("Rfont_destroy", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(font);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfont_is_valid$;

    public static int Rfont_is_valid(int font) {
        MethodHandle $h = Rfont_is_valid$;
        if ($h == null) Rfont_is_valid$ = $h = find("Rfont_is_valid", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(font);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfont_get_default$;

    /**
     * Returns: The font embedded in the library (Roboto, Latin and Latin Extended-A).
     */
    public static int Rfont_get_default() {
        MethodHandle $h = Rfont_get_default$;
        if ($h == null) Rfont_get_default$ = $h = find("Rfont_get_default", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfont_get_monospace$;

    /**
     * Returns: The fixed-width font embedded in the library (Roboto Mono, same
     * characters), used by default by the text editors. In XML: Font="monospace".
     */
    public static int Rfont_get_monospace() {
        MethodHandle $h = Rfont_get_monospace$;
        if ($h == null) Rfont_get_monospace$ = $h = find("Rfont_get_monospace", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfont_set_fallback$;

    /**
     * Font used for the characters missing from `font` (symbols, icons,
     * other scripts). Fallbacks can be chained.
     */
    public static void Rfont_set_fallback(int font, int fallback) {
        MethodHandle $h = Rfont_set_fallback$;
        if ($h == null) Rfont_set_fallback$ = $h = find("Rfont_set_fallback", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(font, fallback);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfont_set_variant$;

    /**
     * Gives a font the font to use for one of its styles: the bold
     * file of the same family, its italic file. Formatted text (Markdown
     * labels, the Markdown of the text editor) uses them.
     *
     * A font that was given none still has the three styles: Raven draws
     * its glyphs thicker for bold and slanted for italic, keeping their
     * advances, so that text does not move when its style changes.
     * variant R_INVALID_ID goes back to the style Raven derives.
     */
    public static void Rfont_set_variant(int font, int style, int variant) {
        MethodHandle $h = Rfont_set_variant$;
        if ($h == null) Rfont_set_variant$ = $h = find("Rfont_set_variant", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(font, style, variant);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfont_get_variant$;

    /**
     * Returns: The font drawn for a style of `font`: the one given with
     * Rfont_set_variant, else the one Raven derives from it (made on first
     * use; it lives as long as `font` and cannot be deleted by itself).
     * Usable wherever a font is: Rlabel_set_font, Rdraw_text...
     */
    public static int Rfont_get_variant(int font, int style) {
        MethodHandle $h = Rfont_get_variant$;
        if ($h == null) Rfont_get_variant$ = $h = find("Rfont_get_variant", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(font, style);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfont_measure_text$;

    /**
     * Measures a UTF-8 string, in logical units, at scale 1.
     * font R_INVALID_ID uses the theme font.
     * size Font size; 0 uses R_METRIC_FONT_SIZE.
     */
    public static void Rfont_measure_text(int font, float size, String text, float[] width, float[] height) {
        MethodHandle $h = Rfont_measure_text$;
        if ($h == null) Rfont_measure_text$ = $h = find("Rfont_measure_text", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, ADDRESS, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            MemorySegment width$ = width == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment height$ = height == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            $h.invokeExact(font, size, text$, width$, height$);
            if (width != null && width.length > 0) width[0] = width$.get(JAVA_FLOAT, 0);
            if (height != null && height.length > 0) height[0] = height$.get(JAVA_FLOAT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtexture_create$;

    /**
     * Creates a texture from the content of an image file (PNG, JPEG,
     * BMP, TGA, GIF, PSD). Decoding happens on the calling thread.
     * Returns: R_INVALID_ID on failure.
     */
    public static int Rtexture_create(byte[] data) {
        MethodHandle $h = Rtexture_create$;
        if ($h == null) Rtexture_create$ = $h = find("Rtexture_create", FunctionDescriptor.of(JAVA_INT, ADDRESS, JAVA_LONG));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment data$ = data == null ? MemorySegment.NULL : bytes($a, data);
            int $r = (int) $h.invokeExact(data$, data$.byteSize());
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtexture_create_async$;

    /**
     * Same as Rtexture_create, decoded on a worker thread. The handle
     * is usable at once; widgets showing it draw nothing until it is ready.
     * The buffer is copied.
     */
    public static int Rtexture_create_async(byte[] data) {
        MethodHandle $h = Rtexture_create_async$;
        if ($h == null) Rtexture_create_async$ = $h = find("Rtexture_create_async", FunctionDescriptor.of(JAVA_INT, ADDRESS, JAVA_LONG));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment data$ = data == null ? MemorySegment.NULL : bytes($a, data);
            int $r = (int) $h.invokeExact(data$, data$.byteSize());
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtexture_create_from_file$;

    public static int Rtexture_create_from_file(String path) {
        MethodHandle $h = Rtexture_create_from_file$;
        if ($h == null) Rtexture_create_from_file$ = $h = find("Rtexture_create_from_file", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment path$ = path == null ? MemorySegment.NULL : utf8($a, path);
            int $r = (int) $h.invokeExact(path$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtexture_create_from_file_async$;

    /**
     * Reads and decodes the file on a worker thread.
     */
    public static int Rtexture_create_from_file_async(String path) {
        MethodHandle $h = Rtexture_create_from_file_async$;
        if ($h == null) Rtexture_create_from_file_async$ = $h = find("Rtexture_create_from_file_async", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment path$ = path == null ? MemorySegment.NULL : utf8($a, path);
            int $r = (int) $h.invokeExact(path$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtexture_create_from_pixels$;

    /**
     * Creates a texture from tightly packed RGBA8 pixels (straight alpha). Its filter is R_FILTER_LINEAR.
     */
    public static int Rtexture_create_from_pixels(byte[] rgba, int width, int height) {
        MethodHandle $h = Rtexture_create_from_pixels$;
        if ($h == null) Rtexture_create_from_pixels$ = $h = find("Rtexture_create_from_pixels", FunctionDescriptor.of(JAVA_INT, ADDRESS, JAVA_INT, JAVA_INT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment rgba$ = rgba == null ? MemorySegment.NULL : bytes($a, rgba);
            int $r = (int) $h.invokeExact(rgba$, width, height);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtexture_update$;

    /**
     * Replaces the pixels of a texture. The size may change. Use it to
     * show frames produced on the CPU or read back from another renderer.
     */
    public static void Rtexture_update(int texture, byte[] rgba, int width, int height) {
        MethodHandle $h = Rtexture_update$;
        if ($h == null) Rtexture_update$ = $h = find("Rtexture_update", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, JAVA_INT, JAVA_INT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment rgba$ = rgba == null ? MemorySegment.NULL : bytes($a, rgba);
            $h.invokeExact(texture, rgba$, width, height);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rimage_decode$;

    /**
     * Decodes an image file held in memory (PNG, JPEG, BMP, TGA) into
     * tightly packed RGBA8 pixels, without making a texture: for pictures
     * that arrive one after the other and go to Rtexture_update. Thread-safe,
     * so that the decoding can be done away from the UI thread.
     * Returns: The pixels, to release with Rpixels_free; NULL when the data
     * is not an image Raven can read.
     */
    public static MemorySegment Rimage_decode(byte[] data, int[] width, int[] height) {
        MethodHandle $h = Rimage_decode$;
        if ($h == null) Rimage_decode$ = $h = find("Rimage_decode", FunctionDescriptor.of(ADDRESS, ADDRESS, JAVA_LONG, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment data$ = data == null ? MemorySegment.NULL : bytes($a, data);
            MemorySegment width$ = width == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            MemorySegment height$ = height == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            MemorySegment $r = (MemorySegment) $h.invokeExact(data$, data$.byteSize(), width$, height$);
            if (width != null && width.length > 0) width[0] = width$.get(JAVA_INT, 0);
            if (height != null && height.length > 0) height[0] = height$.get(JAVA_INT, 0);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rpixels_free$;

    /**
     * Releases pixels returned by Raven (Rimage_decode, Rwindow_capture). Thread-safe.
     */
    public static void Rpixels_free(MemorySegment pixels) {
        MethodHandle $h = Rpixels_free$;
        if ($h == null) Rpixels_free$ = $h = find("Rpixels_free", FunctionDescriptor.ofVoid(ADDRESS));
        try {
            $h.invokeExact((MemorySegment) (pixels == null ? MemorySegment.NULL : pixels));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtexture_is_ready$;

    /**
     * Returns: R_TRUE once an asynchronous texture is decoded (or failed).
     */
    public static int Rtexture_is_ready(int texture) {
        MethodHandle $h = Rtexture_is_ready$;
        if ($h == null) Rtexture_is_ready$ = $h = find("Rtexture_is_ready", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(texture);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtexture_wait$;

    /**
     * Blocks until an asynchronous texture is ready.
     */
    public static void Rtexture_wait(int texture) {
        MethodHandle $h = Rtexture_wait$;
        if ($h == null) Rtexture_wait$ = $h = find("Rtexture_wait", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(texture);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtexture_destroy$;

    public static void Rtexture_destroy(int texture) {
        MethodHandle $h = Rtexture_destroy$;
        if ($h == null) Rtexture_destroy$ = $h = find("Rtexture_destroy", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(texture);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtexture_is_valid$;

    public static int Rtexture_is_valid(int texture) {
        MethodHandle $h = Rtexture_is_valid$;
        if ($h == null) Rtexture_is_valid$ = $h = find("Rtexture_is_valid", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(texture);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtexture_get_size$;

    public static void Rtexture_get_size(int texture, int[] width, int[] height) {
        MethodHandle $h = Rtexture_get_size$;
        if ($h == null) Rtexture_get_size$ = $h = find("Rtexture_get_size", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment width$ = width == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            MemorySegment height$ = height == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            $h.invokeExact(texture, width$, height$);
            if (width != null && width.length > 0) width[0] = width$.get(JAVA_INT, 0);
            if (height != null && height.length > 0) height[0] = height$.get(JAVA_INT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtexture_set_filter$;

    /**
     * How the texture is smoothed when it is scaled. The textures made
     * from image files start with R_FILTER_MIPMAP, the others with
     * R_FILTER_LINEAR.
     */
    public static void Rtexture_set_filter(int texture, int filter) {
        MethodHandle $h = Rtexture_set_filter$;
        if ($h == null) Rtexture_set_filter$ = $h = find("Rtexture_set_filter", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(texture, filter);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_create$;

    /**
     * Creates a widget. It is not shown until it is added to a parent
     * that belongs to a window (Rwidget_add_child, Rwindow_set_content).
     */
    public static int Rwidget_create(int type) {
        MethodHandle $h = Rwidget_create$;
        if ($h == null) Rwidget_create$ = $h = find("Rwidget_create", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(type);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_destroy$;

    /**
     * Deletes a widget and all its descendants.
     */
    public static void Rwidget_destroy(int widget) {
        MethodHandle $h = Rwidget_destroy$;
        if ($h == null) Rwidget_destroy$ = $h = find("Rwidget_destroy", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(widget);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_is_valid$;

    public static int Rwidget_is_valid(int widget) {
        MethodHandle $h = Rwidget_is_valid$;
        if ($h == null) Rwidget_is_valid$ = $h = find("Rwidget_is_valid", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(widget);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_type$;

    public static int Rwidget_get_type(int widget) {
        MethodHandle $h = Rwidget_get_type$;
        if ($h == null) Rwidget_get_type$ = $h = find("Rwidget_get_type", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(widget);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_add_child$;

    /**
     * Appends a child to a panel. The parent takes ownership.
     * A widget that already has a parent is moved.
     * Returns: `child`, so calls can be nested:
     *         Rid ok = Rwidget_add_child(row, Rbutton_create("OK"));
     */
    public static int Rwidget_add_child(int parent, int child) {
        MethodHandle $h = Rwidget_add_child$;
        if ($h == null) Rwidget_add_child$ = $h = find("Rwidget_add_child", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(parent, child);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_insert_child$;

    public static int Rwidget_insert_child(int parent, int child, int index) {
        MethodHandle $h = Rwidget_insert_child$;
        if ($h == null) Rwidget_insert_child$ = $h = find("Rwidget_insert_child", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(parent, child, index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_remove_child$;

    /**
     * Detaches a child without deleting it. The caller owns it again.
     */
    public static void Rwidget_remove_child(int parent, int child) {
        MethodHandle $h = Rwidget_remove_child$;
        if ($h == null) Rwidget_remove_child$ = $h = find("Rwidget_remove_child", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(parent, child);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_clear_children$;

    /**
     * Deletes every child of a panel.
     */
    public static void Rwidget_clear_children(int parent) {
        MethodHandle $h = Rwidget_clear_children$;
        if ($h == null) Rwidget_clear_children$ = $h = find("Rwidget_clear_children", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(parent);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_parent$;

    public static int Rwidget_get_parent(int widget) {
        MethodHandle $h = Rwidget_get_parent$;
        if ($h == null) Rwidget_get_parent$ = $h = find("Rwidget_get_parent", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(widget);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_child_count$;

    public static int Rwidget_get_child_count(int widget) {
        MethodHandle $h = Rwidget_get_child_count$;
        if ($h == null) Rwidget_get_child_count$ = $h = find("Rwidget_get_child_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(widget);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_child$;

    public static int Rwidget_get_child(int widget, int index) {
        MethodHandle $h = Rwidget_get_child$;
        if ($h == null) Rwidget_get_child$ = $h = find("Rwidget_get_child", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(widget, index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_window$;

    /**
     * Returns: The window showing the widget, or R_INVALID_ID.
     */
    public static int Rwidget_get_window(int widget) {
        MethodHandle $h = Rwidget_get_window$;
        if ($h == null) Rwidget_get_window$ = $h = find("Rwidget_get_window", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(widget);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_name$;

    public static void Rwidget_set_name(int widget, String name) {
        MethodHandle $h = Rwidget_set_name$;
        if ($h == null) Rwidget_set_name$ = $h = find("Rwidget_set_name", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            $h.invokeExact(widget, name$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_name$;

    public static String Rwidget_get_name(int widget) {
        MethodHandle $h = Rwidget_get_name$;
        if ($h == null) Rwidget_get_name$ = $h = find("Rwidget_get_name", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(widget);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_find$;

    /**
     * Finds a widget by name.
     * scope A window or a widget; the search covers its whole subtree.
     * Returns: R_INVALID_ID when no widget has that name.
     */
    public static int Rwidget_find(int scope, String name) {
        MethodHandle $h = Rwidget_find$;
        if ($h == null) Rwidget_find$ = $h = find("Rwidget_find", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            int $r = (int) $h.invokeExact(scope, name$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rslot_set_padding$;

    /**
     * Space kept around the child, inside its slot. All panels.
     */
    public static void Rslot_set_padding(int child, float left, float top, float right, float bottom) {
        MethodHandle $h = Rslot_set_padding$;
        if ($h == null) Rslot_set_padding$ = $h = find("Rslot_set_padding", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(child, left, top, right, bottom);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rslot_set_halign$;

    /**
     * Alignment of the child inside its slot. Default: fill. All panels.
     */
    public static void Rslot_set_halign(int child, int align) {
        MethodHandle $h = Rslot_set_halign$;
        if ($h == null) Rslot_set_halign$ = $h = find("Rslot_set_halign", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(child, align);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rslot_set_valign$;

    public static void Rslot_set_valign(int child, int align) {
        MethodHandle $h = Rslot_set_valign$;
        if ($h == null) Rslot_set_valign$ = $h = find("Rslot_set_valign", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(child, align);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rslot_set_auto$;

    /**
     * The slot takes the desired size of the child along the panel
     * direction (Slate AutoWidth / AutoHeight). This is the default.
     * VBox, HBox.
     */
    public static void Rslot_set_auto(int child) {
        MethodHandle $h = Rslot_set_auto$;
        if ($h == null) Rslot_set_auto$ = $h = find("Rslot_set_auto", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(child);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rslot_set_fill$;

    /**
     * The slot shares the space left by the auto slots, in proportion
     * to `coefficient` (Slate FillWidth / FillHeight). VBox, HBox.
     * In a Splitter, the coefficient is the relative size of the pane.
     */
    public static void Rslot_set_fill(int child, float coefficient) {
        MethodHandle $h = Rslot_set_fill$;
        if ($h == null) Rslot_set_fill$ = $h = find("Rslot_set_fill", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(child, coefficient);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rslot_set_max_size$;

    /**
     * Upper bound of the slot along the panel direction. 0 = none. VBox, HBox.
     */
    public static void Rslot_set_max_size(int child, float max_size) {
        MethodHandle $h = Rslot_set_max_size$;
        if ($h == null) Rslot_set_max_size$ = $h = find("Rslot_set_max_size", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(child, max_size);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rslot_set_grid$;

    /**
     * Cell of the child. Grid, UniformGrid.
     */
    public static void Rslot_set_grid(int child, int column, int row) {
        MethodHandle $h = Rslot_set_grid$;
        if ($h == null) Rslot_set_grid$ = $h = find("Rslot_set_grid", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(child, column, row);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rslot_set_grid_span$;

    public static void Rslot_set_grid_span(int child, int column_span, int row_span) {
        MethodHandle $h = Rslot_set_grid_span$;
        if ($h == null) Rslot_set_grid_span$ = $h = find("Rslot_set_grid_span", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(child, column_span, row_span);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rslot_set_anchors$;

    /**
     * Canvas placement. Anchors are fractions of the canvas size (0..1).
     *   - anchors equal on an axis: the child is at anchor + position, with
     *     the slot size (or its desired size when the slot size is &lt;= 0);
     *   - anchors different on an axis: the child stretches between them,
     *     position and size are then the near and far margins.
     * Alignment is the pivot of the child (0,0 = top-left, 0.5,0.5 = center).
     */
    public static void Rslot_set_anchors(int child, float min_x, float min_y, float max_x, float max_y) {
        MethodHandle $h = Rslot_set_anchors$;
        if ($h == null) Rslot_set_anchors$ = $h = find("Rslot_set_anchors", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(child, min_x, min_y, max_x, max_y);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rslot_set_position$;

    public static void Rslot_set_position(int child, float x, float y) {
        MethodHandle $h = Rslot_set_position$;
        if ($h == null) Rslot_set_position$ = $h = find("Rslot_set_position", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(child, x, y);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rslot_set_size$;

    public static void Rslot_set_size(int child, float width, float height) {
        MethodHandle $h = Rslot_set_size$;
        if ($h == null) Rslot_set_size$ = $h = find("Rslot_set_size", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(child, width, height);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rslot_set_alignment$;

    public static void Rslot_set_alignment(int child, float x, float y) {
        MethodHandle $h = Rslot_set_alignment$;
        if ($h == null) Rslot_set_alignment$ = $h = find("Rslot_set_alignment", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(child, x, y);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rslot_set_title$;

    /**
     * Title of the tab showing the child. TabView.
     */
    public static void Rslot_set_title(int child, String title) {
        MethodHandle $h = Rslot_set_title$;
        if ($h == null) Rslot_set_title$ = $h = find("Rslot_set_title", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment title$ = title == null ? MemorySegment.NULL : utf8($a, title);
            $h.invokeExact(child, title$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_visibility$;

    public static void Rwidget_set_visibility(int widget, int visibility) {
        MethodHandle $h = Rwidget_set_visibility$;
        if ($h == null) Rwidget_set_visibility$ = $h = find("Rwidget_set_visibility", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(widget, visibility);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_visibility$;

    public static int Rwidget_get_visibility(int widget) {
        MethodHandle $h = Rwidget_get_visibility$;
        if ($h == null) Rwidget_get_visibility$ = $h = find("Rwidget_get_visibility", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(widget);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_enabled$;

    /**
     * A disabled widget ignores input and is drawn dimmed, with its descendants.
     */
    public static void Rwidget_set_enabled(int widget, int enabled) {
        MethodHandle $h = Rwidget_set_enabled$;
        if ($h == null) Rwidget_set_enabled$ = $h = find("Rwidget_set_enabled", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(widget, enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_is_enabled$;

    public static int Rwidget_is_enabled(int widget) {
        MethodHandle $h = Rwidget_is_enabled$;
        if ($h == null) Rwidget_is_enabled$ = $h = find("Rwidget_is_enabled", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(widget);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_hit_testable$;

    /**
     * R_FALSE lets mouse events pass through the widget (not its children).
     */
    public static void Rwidget_set_hit_testable(int widget, int hit_testable) {
        MethodHandle $h = Rwidget_set_hit_testable$;
        if ($h == null) Rwidget_set_hit_testable$ = $h = find("Rwidget_set_hit_testable", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(widget, hit_testable);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_opacity$;

    /**
     * Opacity multiplier applied to the widget and its descendants (0..1).
     */
    public static void Rwidget_set_opacity(int widget, float opacity) {
        MethodHandle $h = Rwidget_set_opacity$;
        if ($h == null) Rwidget_set_opacity$ = $h = find("Rwidget_set_opacity", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(widget, opacity);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_transition$;

    /**
     * Gives the widget a transition that plays each time it appears:
     * when its window is first drawn, when it is added to a window that is
     * shown, and when it becomes visible again (Rwidget_set_visibility, a
     * tab or a switcher showing it). In XML: Appear="FlyUp" AppearDelay="0.1".
     * delay_seconds Time before it starts. Give growing delays to the
     * items of a group to make them arrive one after the other.
     */
    public static void Rwidget_set_transition(int widget, int transition, float delay_seconds) {
        MethodHandle $h = Rwidget_set_transition$;
        if ($h == null) Rwidget_set_transition$ = $h = find("Rwidget_set_transition", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(widget, transition, delay_seconds);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_play_transition$;

    /**
     * Plays a transition once, now, whatever the widget was given with Rwidget_set_transition.
     */
    public static void Rwidget_play_transition(int widget, int transition, float delay_seconds) {
        MethodHandle $h = Rwidget_play_transition$;
        if ($h == null) Rwidget_play_transition$ = $h = find("Rwidget_play_transition", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(widget, transition, delay_seconds);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_clipping$;

    /**
     * R_TRUE clips the descendants to the bounds of the widget.
     */
    public static void Rwidget_set_clipping(int widget, int clip) {
        MethodHandle $h = Rwidget_set_clipping$;
        if ($h == null) Rwidget_set_clipping$ = $h = find("Rwidget_set_clipping", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(widget, clip);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_tooltip$;

    /**
     * Text shown when the cursor rests on the widget. NULL removes it.
     */
    public static void Rwidget_set_tooltip(int widget, String text) {
        MethodHandle $h = Rwidget_set_tooltip$;
        if ($h == null) Rwidget_set_tooltip$ = $h = find("Rwidget_set_tooltip", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            $h.invokeExact(widget, text$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_tooltip$;

    /**
     * Returns: The tooltip text of the widget, "" when it has none.
     */
    public static String Rwidget_get_tooltip(int widget) {
        MethodHandle $h = Rwidget_get_tooltip$;
        if ($h == null) Rwidget_get_tooltip$ = $h = find("Rwidget_get_tooltip", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(widget);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_cursor$;

    public static void Rwidget_set_cursor(int widget, int cursor) {
        MethodHandle $h = Rwidget_set_cursor$;
        if ($h == null) Rwidget_set_cursor$ = $h = find("Rwidget_set_cursor", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(widget, cursor);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_size$;

    /**
     * Size constraints, in logical units. They apply to the desired size the
     * widget reports to its parent. A negative value removes the constraint
     * on that axis.
     */
    public static void Rwidget_set_size(int widget, float width, float height) {
        MethodHandle $h = Rwidget_set_size$;
        if ($h == null) Rwidget_set_size$ = $h = find("Rwidget_set_size", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(widget, width, height);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_min_size$;

    public static void Rwidget_set_min_size(int widget, float width, float height) {
        MethodHandle $h = Rwidget_set_min_size$;
        if ($h == null) Rwidget_set_min_size$ = $h = find("Rwidget_set_min_size", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(widget, width, height);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_max_size$;

    public static void Rwidget_set_max_size(int widget, float width, float height) {
        MethodHandle $h = Rwidget_set_max_size$;
        if ($h == null) Rwidget_set_max_size$ = $h = find("Rwidget_set_max_size", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(widget, width, height);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_rect$;

    /**
     * Bounds of the widget in its window, in logical units, after the last layout.
     */
    public static void Rwidget_get_rect(int widget, float[] x, float[] y, float[] width, float[] height) {
        MethodHandle $h = Rwidget_get_rect$;
        if ($h == null) Rwidget_get_rect$ = $h = find("Rwidget_get_rect", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment x$ = x == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment y$ = y == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment width$ = width == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment height$ = height == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            $h.invokeExact(widget, x$, y$, width$, height$);
            if (x != null && x.length > 0) x[0] = x$.get(JAVA_FLOAT, 0);
            if (y != null && y.length > 0) y[0] = y$.get(JAVA_FLOAT, 0);
            if (width != null && width.length > 0) width[0] = width$.get(JAVA_FLOAT, 0);
            if (height != null && height.length > 0) height[0] = height$.get(JAVA_FLOAT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_desired_size$;

    public static void Rwidget_get_desired_size(int widget, float[] width, float[] height) {
        MethodHandle $h = Rwidget_get_desired_size$;
        if ($h == null) Rwidget_get_desired_size$ = $h = find("Rwidget_get_desired_size", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment width$ = width == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment height$ = height == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            $h.invokeExact(widget, width$, height$);
            if (width != null && width.length > 0) width[0] = width$.get(JAVA_FLOAT, 0);
            if (height != null && height.length > 0) height[0] = height$.get(JAVA_FLOAT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_is_hovered$;

    public static int Rwidget_is_hovered(int widget) {
        MethodHandle $h = Rwidget_is_hovered$;
        if ($h == null) Rwidget_is_hovered$ = $h = find("Rwidget_is_hovered", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(widget);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_focusable$;

    public static void Rwidget_set_focusable(int widget, int focusable) {
        MethodHandle $h = Rwidget_set_focusable$;
        if ($h == null) Rwidget_set_focusable$ = $h = find("Rwidget_set_focusable", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(widget, focusable);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_focus$;

    /**
     * Gives the keyboard focus to the widget.
     */
    public static void Rwidget_focus(int widget) {
        MethodHandle $h = Rwidget_focus$;
        if ($h == null) Rwidget_focus$ = $h = find("Rwidget_focus", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(widget);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_is_focused$;

    public static int Rwidget_is_focused(int widget) {
        MethodHandle $h = Rwidget_is_focused$;
        if ($h == null) Rwidget_is_focused$ = $h = find("Rwidget_is_focused", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(widget);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_user_handle$;

    public static void Rwidget_set_user_handle(int widget, MemorySegment handle) {
        MethodHandle $h = Rwidget_set_user_handle$;
        if ($h == null) Rwidget_set_user_handle$ = $h = find("Rwidget_set_user_handle", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try {
            $h.invokeExact(widget, (MemorySegment) (handle == null ? MemorySegment.NULL : handle));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_user_handle$;

    public static MemorySegment Rwidget_get_user_handle(int widget) {
        MethodHandle $h = Rwidget_get_user_handle$;
        if ($h == null) Rwidget_get_user_handle$ = $h = find("Rwidget_get_user_handle", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(widget);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_destroy_callback$;

    public static void Rwidget_set_destroy_callback(int widget, RWidgetDestroyCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rwidget_set_destroy_callback$;
        if ($h == null) Rwidget_set_destroy_callback$ = $h = find("Rwidget_set_destroy_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(widget, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rwidget_set_destroy_callback(int widget, RWidgetDestroyCallback callback) {
        Rwidget_set_destroy_callback(widget, callback, (MemorySegment) null);
    }

    private static MethodHandle Rwidget_set_event_callback$;

    public static void Rwidget_set_event_callback(int widget, RWidgetEventCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rwidget_set_event_callback$;
        if ($h == null) Rwidget_set_event_callback$ = $h = find("Rwidget_set_event_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(widget, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rwidget_set_event_callback(int widget, RWidgetEventCallback callback) {
        Rwidget_set_event_callback(widget, callback, (MemorySegment) null);
    }

    private static MethodHandle Rwidget_set_action$;

    /**
     * Name of the action fired by the main event of the widget: button
     * click, value change, text commit, item activation (see Raction_bind).
     */
    public static void Rwidget_set_action(int widget, String action) {
        MethodHandle $h = Rwidget_set_action$;
        if ($h == null) Rwidget_set_action$ = $h = find("Rwidget_set_action", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment action$ = action == null ? MemorySegment.NULL : utf8($a, action);
            $h.invokeExact(widget, action$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_action$;

    public static String Rwidget_get_action(int widget) {
        MethodHandle $h = Rwidget_get_action$;
        if ($h == null) Rwidget_get_action$ = $h = find("Rwidget_get_action", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(widget);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_drop_callback$;

    /**
     * Makes the widget a target for what is dragged from other
     * applications: files from the file manager, text. It is highlighted
     * while a drop on it would be accepted. NULL removes the callback.
     */
    public static void Rwidget_set_drop_callback(int widget, RDropCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rwidget_set_drop_callback$;
        if ($h == null) Rwidget_set_drop_callback$ = $h = find("Rwidget_set_drop_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(widget, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Makes the widget a target for what is dragged from other
     * applications: files from the file manager, text. It is highlighted
     * while a drop on it would be accepted. NULL removes the callback.
     */
    public static void Rwidget_set_drop_callback(int widget, RDropCallback callback) {
        Rwidget_set_drop_callback(widget, callback, (MemorySegment) null);
    }

    private static MethodHandle Rwidget_set_window_drag$;

    /**
     * In a window created with R_WINDOW_CUSTOM_HEADER: dragging the
     * widget moves the window and a double click maximizes it, as a title bar
     * does. The buttons, fields and other controls inside it keep working.
     * In XML: WindowDrag="true".
     */
    public static void Rwidget_set_window_drag(int widget, int enabled) {
        MethodHandle $h = Rwidget_set_window_drag$;
        if ($h == null) Rwidget_set_window_drag$ = $h = find("Rwidget_set_window_drag", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(widget, enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_hit_test$;

    /**
     * Deepest widget of a subtree under a point of its window, or R_INVALID_ID.
     */
    public static int Rwidget_hit_test(int root, float x, float y) {
        MethodHandle $h = Rwidget_hit_test$;
        if ($h == null) Rwidget_hit_test$ = $h = find("Rwidget_hit_test", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            int $r = (int) $h.invokeExact(root, x, y);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_pick$;

    /**
     * Deepest visible widget of a subtree under a point of its window,
     * whether it reacts to the pointer or not: a label, a panel, a disabled
     * button. What a design tool selects with a click.
     */
    public static int Rwidget_pick(int root, float x, float y) {
        MethodHandle $h = Rwidget_pick$;
        if ($h == null) Rwidget_pick$ = $h = find("Rwidget_pick", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            int $r = (int) $h.invokeExact(root, x, y);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_context_menu$;

    /**
     * Menu opened by a right click on the widget. The widget does not own it.
     */
    public static void Rwidget_set_context_menu(int widget, int menu) {
        MethodHandle $h = Rwidget_set_context_menu$;
        if ($h == null) Rwidget_set_context_menu$ = $h = find("Rwidget_set_context_menu", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(widget, menu);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_invalidate$;

    /**
     * Asks for the widget to be laid out and painted again.
     */
    public static void Rwidget_invalidate(int widget) {
        MethodHandle $h = Rwidget_invalidate$;
        if ($h == null) Rwidget_invalidate$ = $h = find("Rwidget_invalidate", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(widget);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_color$;

    /**
     * Overrides a theme color for one widget. R_COLOR_TEXT is also
     * inherited by its descendants.
     */
    public static void Rwidget_set_color(int widget, int role, float r, float g, float b, float a) {
        MethodHandle $h = Rwidget_set_color$;
        if ($h == null) Rwidget_set_color$ = $h = find("Rwidget_set_color", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(widget, role, r, g, b, a);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_clear_color$;

    public static void Rwidget_clear_color(int widget, int role) {
        MethodHandle $h = Rwidget_clear_color$;
        if ($h == null) Rwidget_clear_color$ = $h = find("Rwidget_clear_color", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(widget, role);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_color$;

    /**
     * The color the widget is drawn with for a role: its own, else
     * the one of the styles of its theme that apply to it, else the one of
     * its theme. What a custom widget should draw itself with.
     */
    public static void Rwidget_get_color(int widget, int role, float[] r, float[] g, float[] b, float[] a) {
        MethodHandle $h = Rwidget_get_color$;
        if ($h == null) Rwidget_get_color$ = $h = find("Rwidget_get_color", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment r$ = r == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment g$ = g == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment b$ = b == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment a$ = a == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            $h.invokeExact(widget, role, r$, g$, b$, a$);
            if (r != null && r.length > 0) r[0] = r$.get(JAVA_FLOAT, 0);
            if (g != null && g.length > 0) g[0] = g$.get(JAVA_FLOAT, 0);
            if (b != null && b.length > 0) b[0] = b$.get(JAVA_FLOAT, 0);
            if (a != null && a.length > 0) a[0] = a$.get(JAVA_FLOAT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_material$;

    /**
     * Gives one widget a material of its own, whatever the theme:
     * a solid button in a glass window, a panel of clear glass in a flat one.
     * A widget that draws no surface (a box, a label) gets one of this
     * material behind it: any widget can become a panel of glass.
     * In XML: Material="Glass | FrostedGlass | TintedGlass | Solid".
     * material R_MATERIAL_DEFAULT follows the theme again.
     */
    public static void Rwidget_set_material(int widget, int material) {
        MethodHandle $h = Rwidget_set_material$;
        if ($h == null) Rwidget_set_material$ = $h = find("Rwidget_set_material", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(widget, material);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_material$;

    /**
     * Returns: The material the widget is drawn with: its own, or the one of the theme.
     */
    public static int Rwidget_get_material(int widget) {
        MethodHandle $h = Rwidget_get_material$;
        if ($h == null) Rwidget_get_material$ = $h = find("Rwidget_get_material", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(widget);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_metric$;

    /**
     * Overrides a theme metric for one widget.
     */
    public static void Rwidget_set_metric(int widget, int metric, float value) {
        MethodHandle $h = Rwidget_set_metric$;
        if ($h == null) Rwidget_set_metric$ = $h = find("Rwidget_set_metric", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(widget, metric, value);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_clear_metric$;

    public static void Rwidget_clear_metric(int widget, int metric) {
        MethodHandle $h = Rwidget_clear_metric$;
        if ($h == null) Rwidget_clear_metric$ = $h = find("Rwidget_clear_metric", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(widget, metric);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_metric$;

    /**
     * Returns: The metric the widget uses: its own, else the one of a style, else the one of its theme.
     */
    public static float Rwidget_get_metric(int widget, int metric) {
        MethodHandle $h = Rwidget_get_metric$;
        if ($h == null) Rwidget_get_metric$ = $h = find("Rwidget_get_metric", FunctionDescriptor.of(JAVA_FLOAT, JAVA_INT, JAVA_INT));
        try {
            float $r = (float) $h.invokeExact(widget, metric);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_class$;

    /**
     * Classes of the widget, separated by spaces ("danger large"):
     * the styles of the theme made for these classes apply to it
     * (Rtheme_set_style_color, &lt;Style Class="danger" .../&gt; in a theme file).
     * In XML: Class="...". NULL or "" removes them.
     */
    public static void Rwidget_set_class(int widget, String classes) {
        MethodHandle $h = Rwidget_set_class$;
        if ($h == null) Rwidget_set_class$ = $h = find("Rwidget_set_class", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment classes$ = classes == null ? MemorySegment.NULL : utf8($a, classes);
            $h.invokeExact(widget, classes$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_class$;

    /**
     * Returns: The classes, "" when it has none. Valid until they change.
     */
    public static String Rwidget_get_class(int widget) {
        MethodHandle $h = Rwidget_get_class$;
        if ($h == null) Rwidget_get_class$ = $h = find("Rwidget_get_class", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(widget);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_add_class$;

    public static void Rwidget_add_class(int widget, String style_class) {
        MethodHandle $h = Rwidget_add_class$;
        if ($h == null) Rwidget_add_class$ = $h = find("Rwidget_add_class", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment style_class$ = style_class == null ? MemorySegment.NULL : utf8($a, style_class);
            $h.invokeExact(widget, style_class$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_remove_class$;

    public static void Rwidget_remove_class(int widget, String style_class) {
        MethodHandle $h = Rwidget_remove_class$;
        if ($h == null) Rwidget_remove_class$ = $h = find("Rwidget_remove_class", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment style_class$ = style_class == null ? MemorySegment.NULL : utf8($a, style_class);
            $h.invokeExact(widget, style_class$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_has_class$;

    public static int Rwidget_has_class(int widget, String style_class) {
        MethodHandle $h = Rwidget_has_class$;
        if ($h == null) Rwidget_has_class$ = $h = find("Rwidget_has_class", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment style_class$ = style_class == null ? MemorySegment.NULL : utf8($a, style_class);
            int $r = (int) $h.invokeExact(widget, style_class$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_theme$;

    /**
     * Gives the widget and everything inside it a theme of their own,
     * by name ("Dark", "GlassLight", or a theme of Rtheme_load): a dark side
     * bar in a light window, a preview of another theme. Menus and lists
     * opened from inside follow. In XML: Theme="...".
     * name NULL or "" follows the parent (the application theme) again.
     * Returns: R_FALSE, after reporting an error, when no theme has that name.
     */
    public static int Rwidget_set_theme(int widget, String name) {
        MethodHandle $h = Rwidget_set_theme$;
        if ($h == null) Rwidget_set_theme$ = $h = find("Rwidget_set_theme", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            int $r = (int) $h.invokeExact(widget, name$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_theme$;

    /**
     * Returns: Name of the theme the widget is drawn with.
     */
    public static String Rwidget_get_theme(int widget) {
        MethodHandle $h = Rwidget_get_theme$;
        if ($h == null) Rwidget_get_theme$ = $h = find("Rwidget_get_theme", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(widget);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_font$;

    /**
     * Gives the widget and everything inside it a font, for the texts
     * that have none of their own (a label given Rlabel_set_font keeps its
     * own): a tool whose panels wear one font while the interface it shows
     * keeps the font of the application. Menus and lists opened from inside
     * follow.
     * font R_INVALID_ID: the font of the parent (in the end, the one of the theme, Rtheme_set_font) again.
     */
    public static void Rwidget_set_font(int widget, int font) {
        MethodHandle $h = Rwidget_set_font$;
        if ($h == null) Rwidget_set_font$ = $h = find("Rwidget_set_font", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(widget, font);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_font$;

    /**
     * Returns: The font given to the widget itself, or R_INVALID_ID.
     */
    public static int Rwidget_get_font(int widget) {
        MethodHandle $h = Rwidget_get_font$;
        if ($h == null) Rwidget_get_font$ = $h = find("Rwidget_get_font", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(widget);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rpanel_set_spacing$;

    /**
     * Gap inserted between neighbouring children.
     */
    public static void Rpanel_set_spacing(int panel, float spacing) {
        MethodHandle $h = Rpanel_set_spacing$;
        if ($h == null) Rpanel_set_spacing$ = $h = find("Rpanel_set_spacing", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(panel, spacing);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rpanel_get_spacing$;

    public static float Rpanel_get_spacing(int panel) {
        MethodHandle $h = Rpanel_get_spacing$;
        if ($h == null) Rpanel_get_spacing$ = $h = find("Rpanel_get_spacing", FunctionDescriptor.of(JAVA_FLOAT, JAVA_INT));
        try {
            float $r = (float) $h.invokeExact(panel);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rgrid_set_column_fill$;

    /**
     * Lets a column / row grow to take a share of the free space
     * (Slate FillColumn / FillRow). 0 sizes it to its content (default).
     */
    public static void Rgrid_set_column_fill(int grid, int column, float coefficient) {
        MethodHandle $h = Rgrid_set_column_fill$;
        if ($h == null) Rgrid_set_column_fill$ = $h = find("Rgrid_set_column_fill", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(grid, column, coefficient);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rgrid_set_row_fill$;

    public static void Rgrid_set_row_fill(int grid, int row, float coefficient) {
        MethodHandle $h = Rgrid_set_row_fill$;
        if ($h == null) Rgrid_set_row_fill$ = $h = find("Rgrid_set_row_fill", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(grid, row, coefficient);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Runiform_grid_set_columns$;

    /**
     * Places children automatically, left to right then top to bottom,
     * in the given number of columns. 0 uses the grid cell of each slot (default).
     */
    public static void Runiform_grid_set_columns(int grid, int columns) {
        MethodHandle $h = Runiform_grid_set_columns$;
        if ($h == null) Runiform_grid_set_columns$ = $h = find("Runiform_grid_set_columns", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(grid, columns);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Runiform_grid_set_min_cell_size$;

    public static void Runiform_grid_set_min_cell_size(int grid, float width, float height) {
        MethodHandle $h = Runiform_grid_set_min_cell_size$;
        if ($h == null) Runiform_grid_set_min_cell_size$ = $h = find("Runiform_grid_set_min_cell_size", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(grid, width, height);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rborder_set_padding$;

    /**
     * Space between the outline and the child.
     */
    public static void Rborder_set_padding(int border, float left, float top, float right, float bottom) {
        MethodHandle $h = Rborder_set_padding$;
        if ($h == null) Rborder_set_padding$ = $h = find("Rborder_set_padding", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(border, left, top, right, bottom);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rborder_set_background$;

    /**
     * Background color (R_COLOR_PANEL of this widget). Alpha 0 draws no background.
     */
    public static void Rborder_set_background(int border, float r, float g, float b, float a) {
        MethodHandle $h = Rborder_set_background$;
        if ($h == null) Rborder_set_background$ = $h = find("Rborder_set_background", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(border, r, g, b, a);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rborder_set_outline$;

    /**
     * Outline width and color. Width 0 draws no outline (default).
     */
    public static void Rborder_set_outline(int border, float width, float r, float g, float b, float a) {
        MethodHandle $h = Rborder_set_outline$;
        if ($h == null) Rborder_set_outline$ = $h = find("Rborder_set_outline", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(border, width, r, g, b, a);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rborder_set_corner_radius$;

    public static void Rborder_set_corner_radius(int border, float top_left, float top_right, float bottom_right, float bottom_left) {
        MethodHandle $h = Rborder_set_corner_radius$;
        if ($h == null) Rborder_set_corner_radius$ = $h = find("Rborder_set_corner_radius", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(border, top_left, top_right, bottom_right, bottom_left);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rborder_set_shadow$;

    /**
     * Soft drop shadow behind the border. Blur 0 disables it (default).
     */
    public static void Rborder_set_shadow(int border, float blur, float offset_x, float offset_y) {
        MethodHandle $h = Rborder_set_shadow$;
        if ($h == null) Rborder_set_shadow$ = $h = find("Rborder_set_shadow", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(border, blur, offset_x, offset_y);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rborder_set_material$;

    /**
     * What the border is made of. R_MATERIAL_GLASS turns it into a
     * panel of glass tinted by its background color (the alpha of the color
     * is the strength of the tint): floating tool bars, side bars, cards.
     */
    public static void Rborder_set_material(int border, int material) {
        MethodHandle $h = Rborder_set_material$;
        if ($h == null) Rborder_set_material$ = $h = find("Rborder_set_material", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(border, material);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rsize_box_set_aspect_ratio$;

    /**
     * Keeps the child at a fixed width / height ratio, as large as fits.
     * 0 disables it (default). The fixed, minimum and maximum sizes are set
     * with Rwidget_set_size, Rwidget_set_min_size and Rwidget_set_max_size.
     */
    public static void Rsize_box_set_aspect_ratio(int size_box, float ratio) {
        MethodHandle $h = Rsize_box_set_aspect_ratio$;
        if ($h == null) Rsize_box_set_aspect_ratio$ = $h = find("Rsize_box_set_aspect_ratio", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(size_box, ratio);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rspacer_set_size$;

    public static void Rspacer_set_size(int spacer, float width, float height) {
        MethodHandle $h = Rspacer_set_size$;
        if ($h == null) Rspacer_set_size$ = $h = find("Rspacer_set_size", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(spacer, width, height);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rscroll_box_set_orientation$;

    /**
     * Direction in which children are stacked and scrolled. Default: R_VERTICAL.
     */
    public static void Rscroll_box_set_orientation(int scroll_box, int orientation) {
        MethodHandle $h = Rscroll_box_set_orientation$;
        if ($h == null) Rscroll_box_set_orientation$ = $h = find("Rscroll_box_set_orientation", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(scroll_box, orientation);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rscroll_box_set_scroll_both$;

    /**
     * R_TRUE also scrolls across the stacking direction when the content is too large.
     */
    public static void Rscroll_box_set_scroll_both(int scroll_box, int enabled) {
        MethodHandle $h = Rscroll_box_set_scroll_both$;
        if ($h == null) Rscroll_box_set_scroll_both$ = $h = find("Rscroll_box_set_scroll_both", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(scroll_box, enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rscroll_box_set_offset$;

    public static void Rscroll_box_set_offset(int scroll_box, float x, float y) {
        MethodHandle $h = Rscroll_box_set_offset$;
        if ($h == null) Rscroll_box_set_offset$ = $h = find("Rscroll_box_set_offset", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(scroll_box, x, y);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rscroll_box_get_offset$;

    public static void Rscroll_box_get_offset(int scroll_box, float[] x, float[] y) {
        MethodHandle $h = Rscroll_box_get_offset$;
        if ($h == null) Rscroll_box_get_offset$ = $h = find("Rscroll_box_get_offset", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment x$ = x == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment y$ = y == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            $h.invokeExact(scroll_box, x$, y$);
            if (x != null && x.length > 0) x[0] = x$.get(JAVA_FLOAT, 0);
            if (y != null && y.length > 0) y[0] = y$.get(JAVA_FLOAT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rscroll_box_scroll_to_end$;

    public static void Rscroll_box_scroll_to_end(int scroll_box) {
        MethodHandle $h = Rscroll_box_scroll_to_end$;
        if ($h == null) Rscroll_box_scroll_to_end$ = $h = find("Rscroll_box_scroll_to_end", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(scroll_box);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_scroll_into_view$;

    /**
     * Scrolls every ScrollBox above the widget so that the widget is visible.
     */
    public static void Rwidget_scroll_into_view(int widget) {
        MethodHandle $h = Rwidget_scroll_into_view$;
        if ($h == null) Rwidget_scroll_into_view$ = $h = find("Rwidget_scroll_into_view", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(widget);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rsplitter_set_orientation$;

    /**
     * R_HORIZONTAL puts the panes side by side (default), R_VERTICAL stacks them.
     */
    public static void Rsplitter_set_orientation(int splitter, int orientation) {
        MethodHandle $h = Rsplitter_set_orientation$;
        if ($h == null) Rsplitter_set_orientation$ = $h = find("Rsplitter_set_orientation", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(splitter, orientation);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rsplitter_set_min_pane_size$;

    public static void Rsplitter_set_min_pane_size(int splitter, float size) {
        MethodHandle $h = Rsplitter_set_min_pane_size$;
        if ($h == null) Rsplitter_set_min_pane_size$ = $h = find("Rsplitter_set_min_pane_size", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(splitter, size);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rbackground_blur_create$;

    /**
     * Creates a panel that shows what is behind it out of focus,
     * with one child over it. In XML:
     *
     *   &lt;BackgroundBlur Style="Frosted" Radius="24" CornerRadius="18" Padding="16"&gt;
     *       &lt;Label&gt;Over a blurred view of the window&lt;/Label&gt;
     *   &lt;/BackgroundBlur&gt;
     *
     * It works in any theme and with any material: it is not the glass of
     * the glass themes (which bends and reflects), but the looks of
     * RBlurStyle. Unlike the opacity of a widget, it does not let what is
     * behind show sharp.
     */
    public static int Rbackground_blur_create() {
        MethodHandle $h = Rbackground_blur_create$;
        if ($h == null) Rbackground_blur_create$ = $h = find("Rbackground_blur_create", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rbackground_blur_set_style$;

    public static void Rbackground_blur_set_style(int blur, int style) {
        MethodHandle $h = Rbackground_blur_set_style$;
        if ($h == null) Rbackground_blur_set_style$ = $h = find("Rbackground_blur_set_style", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(blur, style);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rbackground_blur_get_style$;

    public static int Rbackground_blur_get_style(int blur) {
        MethodHandle $h = Rbackground_blur_get_style$;
        if ($h == null) Rbackground_blur_get_style$ = $h = find("Rbackground_blur_get_style", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(blur);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rbackground_blur_set_radius$;

    /**
     * How far what is behind goes out of focus, in logical units:
     * 4 softens, 20 hides the details, 60 leaves only colors. A negative
     * value (the default) uses R_METRIC_BLUR_RADIUS of the theme.
     */
    public static void Rbackground_blur_set_radius(int blur, float radius) {
        MethodHandle $h = Rbackground_blur_set_radius$;
        if ($h == null) Rbackground_blur_set_radius$ = $h = find("Rbackground_blur_set_radius", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(blur, radius);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rbackground_blur_get_radius$;

    public static float Rbackground_blur_get_radius(int blur) {
        MethodHandle $h = Rbackground_blur_get_radius$;
        if ($h == null) Rbackground_blur_get_radius$ = $h = find("Rbackground_blur_get_radius", FunctionDescriptor.of(JAVA_FLOAT, JAVA_INT));
        try {
            float $r = (float) $h.invokeExact(blur);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rbackground_blur_set_tint$;

    /**
     * Color laid over the blur; its alpha is how much of it. Without
     * one, R_BLUR_PLAIN has none and the other styles take theirs from the
     * theme (R_COLOR_PANEL).
     */
    public static void Rbackground_blur_set_tint(int blur, float r, float g, float b, float a) {
        MethodHandle $h = Rbackground_blur_set_tint$;
        if ($h == null) Rbackground_blur_set_tint$ = $h = find("Rbackground_blur_set_tint", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(blur, r, g, b, a);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rbackground_blur_clear_tint$;

    /**
     * Removes the tint given with Rbackground_blur_set_tint.
     */
    public static void Rbackground_blur_clear_tint(int blur) {
        MethodHandle $h = Rbackground_blur_clear_tint$;
        if ($h == null) Rbackground_blur_clear_tint$ = $h = find("Rbackground_blur_clear_tint", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(blur);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rbackground_blur_set_saturation$;

    /**
     * Saturation of what is seen through: 1 = as it is, 0 = gray, 1.5 = vivid. Negative = the one of the style.
     */
    public static void Rbackground_blur_set_saturation(int blur, float saturation) {
        MethodHandle $h = Rbackground_blur_set_saturation$;
        if ($h == null) Rbackground_blur_set_saturation$ = $h = find("Rbackground_blur_set_saturation", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(blur, saturation);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rbackground_blur_set_grain$;

    /**
     * Fine grain over the blur, 0 = none .. 1. Negative = the one of the style.
     */
    public static void Rbackground_blur_set_grain(int blur, float grain) {
        MethodHandle $h = Rbackground_blur_set_grain$;
        if ($h == null) Rbackground_blur_set_grain$ = $h = find("Rbackground_blur_set_grain", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(blur, grain);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rbackground_blur_set_corner_radius$;

    public static void Rbackground_blur_set_corner_radius(int blur, float top_left, float top_right, float bottom_right, float bottom_left) {
        MethodHandle $h = Rbackground_blur_set_corner_radius$;
        if ($h == null) Rbackground_blur_set_corner_radius$ = $h = find("Rbackground_blur_set_corner_radius", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(blur, top_left, top_right, bottom_right, bottom_left);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rbackground_blur_set_padding$;

    /**
     * Space between the edge and the child.
     */
    public static void Rbackground_blur_set_padding(int blur, float left, float top, float right, float bottom) {
        MethodHandle $h = Rbackground_blur_set_padding$;
        if ($h == null) Rbackground_blur_set_padding$ = $h = find("Rbackground_blur_set_padding", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(blur, left, top, right, bottom);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rbackground_blur_set_shadow$;

    /**
     * Soft shadow under the panel; blur 0 = none (default).
     */
    public static void Rbackground_blur_set_shadow(int blur, float shadow_blur) {
        MethodHandle $h = Rbackground_blur_set_shadow$;
        if ($h == null) Rbackground_blur_set_shadow$ = $h = find("Rbackground_blur_set_shadow", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(blur, shadow_blur);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rswitcher_set_index$;

    public static void Rswitcher_set_index(int switcher, int index) {
        MethodHandle $h = Rswitcher_set_index$;
        if ($h == null) Rswitcher_set_index$ = $h = find("Rswitcher_set_index", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(switcher, index);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rswitcher_get_index$;

    public static int Rswitcher_get_index(int switcher) {
        MethodHandle $h = Rswitcher_get_index$;
        if ($h == null) Rswitcher_get_index$ = $h = find("Rswitcher_get_index", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(switcher);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtab_view_add_tab$;

    /**
     * Adds a page. Same as Rwidget_add_child followed by Rslot_set_title.
     * Returns: The index of the new tab, or -1.
     */
    public static int Rtab_view_add_tab(int tab_view, String title, int content) {
        MethodHandle $h = Rtab_view_add_tab$;
        if ($h == null) Rtab_view_add_tab$ = $h = find("Rtab_view_add_tab", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS, JAVA_INT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment title$ = title == null ? MemorySegment.NULL : utf8($a, title);
            int $r = (int) $h.invokeExact(tab_view, title$, content);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtab_view_set_active$;

    public static void Rtab_view_set_active(int tab_view, int index) {
        MethodHandle $h = Rtab_view_set_active$;
        if ($h == null) Rtab_view_set_active$ = $h = find("Rtab_view_set_active", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(tab_view, index);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtab_view_get_active$;

    public static int Rtab_view_get_active(int tab_view) {
        MethodHandle $h = Rtab_view_get_active$;
        if ($h == null) Rtab_view_get_active$ = $h = find("Rtab_view_get_active", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(tab_view);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtab_view_set_changed_callback$;

    public static void Rtab_view_set_changed_callback(int tab_view, RTabChangedCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rtab_view_set_changed_callback$;
        if ($h == null) Rtab_view_set_changed_callback$ = $h = find("Rtab_view_set_changed_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(tab_view, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rtab_view_set_changed_callback(int tab_view, RTabChangedCallback callback) {
        Rtab_view_set_changed_callback(tab_view, callback, (MemorySegment) null);
    }

    private static MethodHandle Rtab_view_set_documents$;

    /**
     * Documents mode: the major tabs of an application (the level,
     * then one tab per open asset, as in Unreal): tabs on the left with an
     * icon and a close button (a middle click closes too), shrinking when
     * they do not fit, no slide between pages. XML: Documents="true".
     */
    public static void Rtab_view_set_documents(int tab_view, int documents) {
        MethodHandle $h = Rtab_view_set_documents$;
        if ($h == null) Rtab_view_set_documents$ = $h = find("Rtab_view_set_documents", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(tab_view, documents);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtab_view_get_tab_count$;

    public static int Rtab_view_get_tab_count(int tab_view) {
        MethodHandle $h = Rtab_view_get_tab_count$;
        if ($h == null) Rtab_view_get_tab_count$ = $h = find("Rtab_view_get_tab_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(tab_view);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtab_view_get_tab$;

    /**
     * The page of a tab, R_INVALID_ID if none.
     */
    public static int Rtab_view_get_tab(int tab_view, int index) {
        MethodHandle $h = Rtab_view_get_tab$;
        if ($h == null) Rtab_view_get_tab$ = $h = find("Rtab_view_get_tab", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(tab_view, index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtab_view_find_tab$;

    /**
     * Index of the tab of a page, -1 if none.
     */
    public static int Rtab_view_find_tab(int tab_view, int content) {
        MethodHandle $h = Rtab_view_find_tab$;
        if ($h == null) Rtab_view_find_tab$ = $h = find("Rtab_view_find_tab", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(tab_view, content);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtab_view_set_tab_title$;

    public static void Rtab_view_set_tab_title(int tab_view, int content, String title) {
        MethodHandle $h = Rtab_view_set_tab_title$;
        if ($h == null) Rtab_view_set_tab_title$ = $h = find("Rtab_view_set_tab_title", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment title$ = title == null ? MemorySegment.NULL : utf8($a, title);
            $h.invokeExact(tab_view, content, title$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtab_view_set_tab_icon$;

    /**
     * Icon before the title (documents). R_INVALID_ID removes it; tint: a mask in the text color.
     */
    public static void Rtab_view_set_tab_icon(int tab_view, int content, int texture, float u0, float v0, float u1, float v1, int tint) {
        MethodHandle $h = Rtab_view_set_tab_icon$;
        if ($h == null) Rtab_view_set_tab_icon$ = $h = find("Rtab_view_set_tab_icon", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_INT));
        try {
            $h.invokeExact(tab_view, content, texture, u0, v0, u1, v1, tint);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtab_view_set_tab_closable$;

    /**
     * R_FALSE: no close button (documents ; the level tab). Tabs can be closed by default.
     */
    public static void Rtab_view_set_tab_closable(int tab_view, int content, int closable) {
        MethodHandle $h = Rtab_view_set_tab_closable$;
        if ($h == null) Rtab_view_set_tab_closable$ = $h = find("Rtab_view_set_tab_closable", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(tab_view, content, closable);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtab_view_show_tab$;

    /**
     * Shows the tab of a page (the changed callback is called).
     */
    public static void Rtab_view_show_tab(int tab_view, int content) {
        MethodHandle $h = Rtab_view_show_tab$;
        if ($h == null) Rtab_view_show_tab$ = $h = find("Rtab_view_show_tab", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(tab_view, content);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtab_view_set_close_callback$;

    public static void Rtab_view_set_close_callback(int tab_view, RTabCloseCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rtab_view_set_close_callback$;
        if ($h == null) Rtab_view_set_close_callback$ = $h = find("Rtab_view_set_close_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(tab_view, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rtab_view_set_close_callback(int tab_view, RTabCloseCallback callback) {
        Rtab_view_set_close_callback(tab_view, callback, (MemorySegment) null);
    }

    private static MethodHandle Rexpander_set_title$;

    public static void Rexpander_set_title(int expander, String title) {
        MethodHandle $h = Rexpander_set_title$;
        if ($h == null) Rexpander_set_title$ = $h = find("Rexpander_set_title", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment title$ = title == null ? MemorySegment.NULL : utf8($a, title);
            $h.invokeExact(expander, title$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rexpander_get_title$;

    public static String Rexpander_get_title(int expander) {
        MethodHandle $h = Rexpander_get_title$;
        if ($h == null) Rexpander_get_title$ = $h = find("Rexpander_get_title", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(expander);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rexpander_set_expanded$;

    public static void Rexpander_set_expanded(int expander, int expanded) {
        MethodHandle $h = Rexpander_set_expanded$;
        if ($h == null) Rexpander_set_expanded$ = $h = find("Rexpander_set_expanded", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(expander, expanded);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rexpander_is_expanded$;

    public static int Rexpander_is_expanded(int expander) {
        MethodHandle $h = Rexpander_is_expanded$;
        if ($h == null) Rexpander_is_expanded$ = $h = find("Rexpander_is_expanded", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(expander);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rexpander_set_changed_callback$;

    public static void Rexpander_set_changed_callback(int expander, RExpanderChangedCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rexpander_set_changed_callback$;
        if ($h == null) Rexpander_set_changed_callback$ = $h = find("Rexpander_set_changed_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(expander, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rexpander_set_changed_callback(int expander, RExpanderChangedCallback callback) {
        Rexpander_set_changed_callback(expander, callback, (MemorySegment) null);
    }

    private static MethodHandle Rset_layout_direction$;

    /**
     * Direction of every window: R_DIRECTION_AUTO (the default), _LTR or _RTL.
     */
    public static void Rset_layout_direction(int direction) {
        MethodHandle $h = Rset_layout_direction$;
        if ($h == null) Rset_layout_direction$ = $h = find("Rset_layout_direction", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(direction);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rget_layout_direction$;

    /**
     * The direction the windows use now: R_DIRECTION_LTR or _RTL.
     */
    public static int Rget_layout_direction() {
        MethodHandle $h = Rget_layout_direction$;
        if ($h == null) Rget_layout_direction$ = $h = find("Rget_layout_direction", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_direction$;

    /**
     * Direction of a part of a window: R_DIRECTION_INHERIT, _LTR or _RTL.
     */
    public static void Rwidget_set_direction(int widget, int direction) {
        MethodHandle $h = Rwidget_set_direction$;
        if ($h == null) Rwidget_set_direction$ = $h = find("Rwidget_set_direction", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(widget, direction);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_direction$;

    /**
     * The direction the widget is laid out in: R_DIRECTION_LTR or _RTL.
     */
    public static int Rwidget_get_direction(int widget) {
        MethodHandle $h = Rwidget_get_direction$;
        if ($h == null) Rwidget_get_direction$ = $h = find("Rwidget_get_direction", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(widget);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlabel_create$;

    /**
     * Shortcut for Rwidget_create(R_WIDGET_LABEL) + Rlabel_set_text.
     */
    public static int Rlabel_create(String text) {
        MethodHandle $h = Rlabel_create$;
        if ($h == null) Rlabel_create$ = $h = find("Rlabel_create", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            int $r = (int) $h.invokeExact(text$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlabel_set_text$;

    /**
     * Sets the text. '\n' starts a new line.
     */
    public static void Rlabel_set_text(int label, String text) {
        MethodHandle $h = Rlabel_set_text$;
        if ($h == null) Rlabel_set_text$ = $h = find("Rlabel_set_text", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            $h.invokeExact(label, text$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlabel_get_text$;

    public static String Rlabel_get_text(int label) {
        MethodHandle $h = Rlabel_get_text$;
        if ($h == null) Rlabel_get_text$ = $h = find("Rlabel_get_text", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(label);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlabel_set_font$;

    /**
     * R_INVALID_ID uses the theme font.
     */
    public static void Rlabel_set_font(int label, int font) {
        MethodHandle $h = Rlabel_set_font$;
        if ($h == null) Rlabel_set_font$ = $h = find("Rlabel_set_font", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(label, font);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlabel_set_text_size$;

    /**
     * Font size in logical units. 0 uses R_METRIC_FONT_SIZE.
     */
    public static void Rlabel_set_text_size(int label, float size) {
        MethodHandle $h = Rlabel_set_text_size$;
        if ($h == null) Rlabel_set_text_size$ = $h = find("Rlabel_set_text_size", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(label, size);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlabel_set_text_align$;

    public static void Rlabel_set_text_align(int label, int align) {
        MethodHandle $h = Rlabel_set_text_align$;
        if ($h == null) Rlabel_set_text_align$ = $h = find("Rlabel_set_text_align", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(label, align);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlabel_set_wrap$;

    /**
     * R_TRUE wraps the text on the width given by the parent.
     */
    public static void Rlabel_set_wrap(int label, int wrap) {
        MethodHandle $h = Rlabel_set_wrap$;
        if ($h == null) Rlabel_set_wrap$ = $h = find("Rlabel_set_wrap", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(label, wrap);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlabel_set_color$;

    /**
     * Text color (R_COLOR_TEXT of this widget).
     */
    public static void Rlabel_set_color(int label, float r, float g, float b, float a) {
        MethodHandle $h = Rlabel_set_color$;
        if ($h == null) Rlabel_set_color$ = $h = find("Rlabel_set_color", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(label, r, g, b, a);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlabel_set_markdown$;

    /**
     * R_TRUE reads the text as Markdown and shows it formatted
     * (Markdown="true" in XML):
     *
     *   # titles, **strong**, *emphasis*, `code`, ~~struck~~, [links](url),
     *   lists and lists in lists, - [x] task lists, &gt; quotes, --- rules,
     *   | tables |, images alone on their line, and ``` blocks of code with
     *   the colors of their language (```c, ```python...: the languages of
     *   the text editor, raven_text_editor.h).
     *
     * The text wraps on the width of the label unless Rlabel_set_wrap said
     * otherwise; put the label in a scroll box for a long document. Strong
     * and emphasized text use the bold and italic of the font
     * (Rfont_set_variant), which Raven derives from the font itself when it
     * was given none. The colors come from the theme: R_COLOR_LINK,
     * R_COLOR_CODE, R_COLOR_SEPARATOR, R_COLOR_ACCENT.
     */
    public static void Rlabel_set_markdown(int label, int enabled) {
        MethodHandle $h = Rlabel_set_markdown$;
        if ($h == null) Rlabel_set_markdown$ = $h = find("Rlabel_set_markdown", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(label, enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlabel_get_markdown$;

    public static int Rlabel_get_markdown(int label) {
        MethodHandle $h = Rlabel_get_markdown$;
        if ($h == null) Rlabel_get_markdown$ = $h = find("Rlabel_get_markdown", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(label);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlabel_set_link_callback$;

    public static void Rlabel_set_link_callback(int label, RLabelLinkCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rlabel_set_link_callback$;
        if ($h == null) Rlabel_set_link_callback$ = $h = find("Rlabel_set_link_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(label, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rlabel_set_link_callback(int label, RLabelLinkCallback callback) {
        Rlabel_set_link_callback(label, callback, (MemorySegment) null);
    }

    private static MethodHandle Rlabel_get_plain_text$;

    /**
     * The text of a Markdown label without its marks, as a reader sees it. Valid until the next call.
     */
    public static String Rlabel_get_plain_text(int label) {
        MethodHandle $h = Rlabel_get_plain_text$;
        if ($h == null) Rlabel_get_plain_text$ = $h = find("Rlabel_get_plain_text", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(label);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlabel_set_selectable$;

    /**
     * R_TRUE lets the user select the text of the label with the
     * mouse, as on a web page (Selectable="true" in XML): drag to select, a
     * double click selects a word, a triple click a paragraph, Shift + click
     * extends the selection; then Ctrl+C (Cmd+C) copies it and Ctrl+A selects
     * everything. A click elsewhere takes the selection away. Plain and
     * Markdown labels alike: a copy of a Markdown label is its text without
     * the marks, and its links are still clicked, not selected. The label
     * takes the mouse (what holds it no longer receives the clicks on it) and
     * the keyboard after a click; Tab passes it by. Default R_FALSE.
     */
    public static void Rlabel_set_selectable(int label, int selectable) {
        MethodHandle $h = Rlabel_set_selectable$;
        if ($h == null) Rlabel_set_selectable$ = $h = find("Rlabel_set_selectable", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(label, selectable);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlabel_get_selectable$;

    public static int Rlabel_get_selectable(int label) {
        MethodHandle $h = Rlabel_get_selectable$;
        if ($h == null) Rlabel_get_selectable$ = $h = find("Rlabel_get_selectable", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(label);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlabel_get_selected_text$;

    /**
     * The text selected, as Ctrl+C copies it ("" when nothing is). Valid until the next call.
     */
    public static String Rlabel_get_selected_text(int label) {
        MethodHandle $h = Rlabel_get_selected_text$;
        if ($h == null) Rlabel_get_selected_text$ = $h = find("Rlabel_get_selected_text", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(label);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlabel_select_all$;

    /**
     * Selects the whole text of a selectable label.
     */
    public static void Rlabel_select_all(int label) {
        MethodHandle $h = Rlabel_select_all$;
        if ($h == null) Rlabel_select_all$ = $h = find("Rlabel_select_all", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(label);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlabel_clear_selection$;

    public static void Rlabel_clear_selection(int label) {
        MethodHandle $h = Rlabel_clear_selection$;
        if ($h == null) Rlabel_clear_selection$ = $h = find("Rlabel_clear_selection", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(label);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rbutton_create$;

    /**
     * Shortcut for Rwidget_create(R_WIDGET_BUTTON) + Rbutton_set_text.
     */
    public static int Rbutton_create(String text) {
        MethodHandle $h = Rbutton_create$;
        if ($h == null) Rbutton_create$ = $h = find("Rbutton_create", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            int $r = (int) $h.invokeExact(text$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rbutton_set_text$;

    /**
     * Sets the content of the button to a label. A button can instead
     * hold any single child widget (Rwidget_add_child), e.g. an HBox with an icon.
     */
    public static void Rbutton_set_text(int button, String text) {
        MethodHandle $h = Rbutton_set_text$;
        if ($h == null) Rbutton_set_text$ = $h = find("Rbutton_set_text", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            $h.invokeExact(button, text$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rbutton_set_style$;

    public static void Rbutton_set_style(int button, int style) {
        MethodHandle $h = Rbutton_set_style$;
        if ($h == null) Rbutton_set_style$ = $h = find("Rbutton_set_style", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(button, style);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rbutton_set_clicked_callback$;

    public static void Rbutton_set_clicked_callback(int button, RButtonClickedCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rbutton_set_clicked_callback$;
        if ($h == null) Rbutton_set_clicked_callback$ = $h = find("Rbutton_set_clicked_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(button, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rbutton_set_clicked_callback(int button, RButtonClickedCallback callback) {
        Rbutton_set_clicked_callback(button, callback, (MemorySegment) null);
    }

    private static MethodHandle Rcheckbox_create$;

    public static int Rcheckbox_create(String text) {
        MethodHandle $h = Rcheckbox_create$;
        if ($h == null) Rcheckbox_create$ = $h = find("Rcheckbox_create", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            int $r = (int) $h.invokeExact(text$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcheckbox_set_text$;

    public static void Rcheckbox_set_text(int checkbox, String text) {
        MethodHandle $h = Rcheckbox_set_text$;
        if ($h == null) Rcheckbox_set_text$ = $h = find("Rcheckbox_set_text", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            $h.invokeExact(checkbox, text$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcheckbox_get_text$;

    public static String Rcheckbox_get_text(int checkbox) {
        MethodHandle $h = Rcheckbox_get_text$;
        if ($h == null) Rcheckbox_get_text$ = $h = find("Rcheckbox_get_text", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(checkbox);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcheckbox_set_checked$;

    public static void Rcheckbox_set_checked(int checkbox, int checked) {
        MethodHandle $h = Rcheckbox_set_checked$;
        if ($h == null) Rcheckbox_set_checked$ = $h = find("Rcheckbox_set_checked", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(checkbox, checked);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcheckbox_is_checked$;

    public static int Rcheckbox_is_checked(int checkbox) {
        MethodHandle $h = Rcheckbox_is_checked$;
        if ($h == null) Rcheckbox_is_checked$ = $h = find("Rcheckbox_is_checked", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(checkbox);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcheckbox_set_style$;

    public static void Rcheckbox_set_style(int checkbox, int style) {
        MethodHandle $h = Rcheckbox_set_style$;
        if ($h == null) Rcheckbox_set_style$ = $h = find("Rcheckbox_set_style", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(checkbox, style);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcheckbox_set_changed_callback$;

    public static void Rcheckbox_set_changed_callback(int checkbox, RCheckboxChangedCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rcheckbox_set_changed_callback$;
        if ($h == null) Rcheckbox_set_changed_callback$ = $h = find("Rcheckbox_set_changed_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(checkbox, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rcheckbox_set_changed_callback(int checkbox, RCheckboxChangedCallback callback) {
        Rcheckbox_set_changed_callback(checkbox, callback, (MemorySegment) null);
    }

    private static MethodHandle Rslider_set_range$;

    public static void Rslider_set_range(int slider, float minimum, float maximum) {
        MethodHandle $h = Rslider_set_range$;
        if ($h == null) Rslider_set_range$ = $h = find("Rslider_set_range", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(slider, minimum, maximum);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rslider_set_value$;

    public static void Rslider_set_value(int slider, float value) {
        MethodHandle $h = Rslider_set_value$;
        if ($h == null) Rslider_set_value$ = $h = find("Rslider_set_value", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(slider, value);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rslider_get_value$;

    public static float Rslider_get_value(int slider) {
        MethodHandle $h = Rslider_get_value$;
        if ($h == null) Rslider_get_value$ = $h = find("Rslider_get_value", FunctionDescriptor.of(JAVA_FLOAT, JAVA_INT));
        try {
            float $r = (float) $h.invokeExact(slider);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rslider_get_range$;

    /**
     * The range given with Rslider_set_range and the step (NULL for what is not wanted).
     */
    public static void Rslider_get_range(int slider, float[] minimum, float[] maximum, float[] step) {
        MethodHandle $h = Rslider_get_range$;
        if ($h == null) Rslider_get_range$ = $h = find("Rslider_get_range", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment minimum$ = minimum == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment maximum$ = maximum == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment step$ = step == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            $h.invokeExact(slider, minimum$, maximum$, step$);
            if (minimum != null && minimum.length > 0) minimum[0] = minimum$.get(JAVA_FLOAT, 0);
            if (maximum != null && maximum.length > 0) maximum[0] = maximum$.get(JAVA_FLOAT, 0);
            if (step != null && step.length > 0) step[0] = step$.get(JAVA_FLOAT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rslider_set_step$;

    /**
     * Snaps the value to multiples of `step`. 0 = continuous (default).
     */
    public static void Rslider_set_step(int slider, float step) {
        MethodHandle $h = Rslider_set_step$;
        if ($h == null) Rslider_set_step$ = $h = find("Rslider_set_step", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(slider, step);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rslider_set_orientation$;

    public static void Rslider_set_orientation(int slider, int orientation) {
        MethodHandle $h = Rslider_set_orientation$;
        if ($h == null) Rslider_set_orientation$ = $h = find("Rslider_set_orientation", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(slider, orientation);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rslider_set_changed_callback$;

    public static void Rslider_set_changed_callback(int slider, RSliderChangedCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rslider_set_changed_callback$;
        if ($h == null) Rslider_set_changed_callback$ = $h = find("Rslider_set_changed_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(slider, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rslider_set_changed_callback(int slider, RSliderChangedCallback callback) {
        Rslider_set_changed_callback(slider, callback, (MemorySegment) null);
    }

    private static MethodHandle Rprogress_bar_set_value$;

    /**
     * Progress between 0 and 1.
     */
    public static void Rprogress_bar_set_value(int progress_bar, float value) {
        MethodHandle $h = Rprogress_bar_set_value$;
        if ($h == null) Rprogress_bar_set_value$ = $h = find("Rprogress_bar_set_value", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(progress_bar, value);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rprogress_bar_get_value$;

    public static float Rprogress_bar_get_value(int progress_bar) {
        MethodHandle $h = Rprogress_bar_get_value$;
        if ($h == null) Rprogress_bar_get_value$ = $h = find("Rprogress_bar_get_value", FunctionDescriptor.of(JAVA_FLOAT, JAVA_INT));
        try {
            float $r = (float) $h.invokeExact(progress_bar);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rimage_set_texture$;

    public static void Rimage_set_texture(int image, int texture) {
        MethodHandle $h = Rimage_set_texture$;
        if ($h == null) Rimage_set_texture$ = $h = find("Rimage_set_texture", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(image, texture);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rimage_set_tint_color$;

    public static void Rimage_set_tint_color(int image, float r, float g, float b, float a) {
        MethodHandle $h = Rimage_set_tint_color$;
        if ($h == null) Rimage_set_tint_color$ = $h = find("Rimage_set_tint_color", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(image, r, g, b, a);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rimage_set_tint_text$;

    /**
     * R_TRUE: the image is tinted with the text color around it (white icons that follow the theme). XML: TintText.
     */
    public static void Rimage_set_tint_text(int image, int enabled) {
        MethodHandle $h = Rimage_set_tint_text$;
        if ($h == null) Rimage_set_tint_text$ = $h = find("Rimage_set_tint_text", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(image, enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rimage_set_stretch$;

    public static void Rimage_set_stretch(int image, int stretch) {
        MethodHandle $h = Rimage_set_stretch$;
        if ($h == null) Rimage_set_stretch$ = $h = find("Rimage_set_stretch", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(image, stretch);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rimage_set_region$;

    /**
     * Part of the texture to show, in UV coordinates. Default: 0, 0, 1, 1.
     */
    public static void Rimage_set_region(int image, float min_u, float min_v, float max_u, float max_v) {
        MethodHandle $h = Rimage_set_region$;
        if ($h == null) Rimage_set_region$ = $h = find("Rimage_set_region", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(image, min_u, min_v, max_u, max_v);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rimage_set_corner_radius$;

    public static void Rimage_set_corner_radius(int image, float radius) {
        MethodHandle $h = Rimage_set_corner_radius$;
        if ($h == null) Rimage_set_corner_radius$ = $h = find("Rimage_set_corner_radius", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(image, radius);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_input_set_text$;

    public static void Rtext_input_set_text(int text_input, String text) {
        MethodHandle $h = Rtext_input_set_text$;
        if ($h == null) Rtext_input_set_text$ = $h = find("Rtext_input_set_text", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            $h.invokeExact(text_input, text$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_input_get_text$;

    public static String Rtext_input_get_text(int text_input) {
        MethodHandle $h = Rtext_input_get_text$;
        if ($h == null) Rtext_input_get_text$ = $h = find("Rtext_input_get_text", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(text_input);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_input_set_placeholder$;

    /**
     * Dimmed text shown while the input is empty.
     */
    public static void Rtext_input_set_placeholder(int text_input, String text) {
        MethodHandle $h = Rtext_input_set_placeholder$;
        if ($h == null) Rtext_input_set_placeholder$ = $h = find("Rtext_input_set_placeholder", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            $h.invokeExact(text_input, text$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_input_set_password$;

    public static void Rtext_input_set_password(int text_input, int password) {
        MethodHandle $h = Rtext_input_set_password$;
        if ($h == null) Rtext_input_set_password$ = $h = find("Rtext_input_set_password", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(text_input, password);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_input_is_password$;

    public static int Rtext_input_is_password(int text_input) {
        MethodHandle $h = Rtext_input_is_password$;
        if ($h == null) Rtext_input_is_password$ = $h = find("Rtext_input_is_password", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(text_input);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_input_is_read_only$;

    public static int Rtext_input_is_read_only(int text_input) {
        MethodHandle $h = Rtext_input_is_read_only$;
        if ($h == null) Rtext_input_is_read_only$ = $h = find("Rtext_input_is_read_only", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(text_input);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_input_set_read_only$;

    public static void Rtext_input_set_read_only(int text_input, int read_only) {
        MethodHandle $h = Rtext_input_set_read_only$;
        if ($h == null) Rtext_input_set_read_only$ = $h = find("Rtext_input_set_read_only", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(text_input, read_only);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_input_set_max_length$;

    /**
     * Maximum number of characters. 0 = no limit (default).
     */
    public static void Rtext_input_set_max_length(int text_input, int max_length) {
        MethodHandle $h = Rtext_input_set_max_length$;
        if ($h == null) Rtext_input_set_max_length$ = $h = find("Rtext_input_set_max_length", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(text_input, max_length);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_input_select_all$;

    public static void Rtext_input_select_all(int text_input) {
        MethodHandle $h = Rtext_input_select_all$;
        if ($h == null) Rtext_input_select_all$ = $h = find("Rtext_input_select_all", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(text_input);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_input_set_changed_callback$;

    /**
     * Called on every edit.
     */
    public static void Rtext_input_set_changed_callback(int text_input, RTextChangedCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rtext_input_set_changed_callback$;
        if ($h == null) Rtext_input_set_changed_callback$ = $h = find("Rtext_input_set_changed_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(text_input, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Called on every edit.
     */
    public static void Rtext_input_set_changed_callback(int text_input, RTextChangedCallback callback) {
        Rtext_input_set_changed_callback(text_input, callback, (MemorySegment) null);
    }

    private static MethodHandle Rtext_input_set_committed_callback$;

    /**
     * Called when Enter is pressed. This also fires the action of the widget.
     */
    public static void Rtext_input_set_committed_callback(int text_input, RTextChangedCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rtext_input_set_committed_callback$;
        if ($h == null) Rtext_input_set_committed_callback$ = $h = find("Rtext_input_set_committed_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(text_input, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Called when Enter is pressed. This also fires the action of the widget.
     */
    public static void Rtext_input_set_committed_callback(int text_input, RTextChangedCallback callback) {
        Rtext_input_set_committed_callback(text_input, callback, (MemorySegment) null);
    }

    private static MethodHandle Rspin_box_set_range$;

    public static void Rspin_box_set_range(int spin_box, double minimum, double maximum) {
        MethodHandle $h = Rspin_box_set_range$;
        if ($h == null) Rspin_box_set_range$ = $h = find("Rspin_box_set_range", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_DOUBLE, JAVA_DOUBLE));
        try {
            $h.invokeExact(spin_box, minimum, maximum);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rspin_box_set_value$;

    public static void Rspin_box_set_value(int spin_box, double value) {
        MethodHandle $h = Rspin_box_set_value$;
        if ($h == null) Rspin_box_set_value$ = $h = find("Rspin_box_set_value", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_DOUBLE));
        try {
            $h.invokeExact(spin_box, value);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rspin_box_get_value$;

    public static double Rspin_box_get_value(int spin_box) {
        MethodHandle $h = Rspin_box_get_value$;
        if ($h == null) Rspin_box_get_value$ = $h = find("Rspin_box_get_value", FunctionDescriptor.of(JAVA_DOUBLE, JAVA_INT));
        try {
            double $r = (double) $h.invokeExact(spin_box);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rspin_box_get_range$;

    /**
     * The range and the step (NULL for what is not wanted).
     */
    public static void Rspin_box_get_range(int spin_box, double[] minimum, double[] maximum, double[] step) {
        MethodHandle $h = Rspin_box_get_range$;
        if ($h == null) Rspin_box_get_range$ = $h = find("Rspin_box_get_range", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment minimum$ = minimum == null ? MemorySegment.NULL : $a.allocate(JAVA_DOUBLE);
            MemorySegment maximum$ = maximum == null ? MemorySegment.NULL : $a.allocate(JAVA_DOUBLE);
            MemorySegment step$ = step == null ? MemorySegment.NULL : $a.allocate(JAVA_DOUBLE);
            $h.invokeExact(spin_box, minimum$, maximum$, step$);
            if (minimum != null && minimum.length > 0) minimum[0] = minimum$.get(JAVA_DOUBLE, 0);
            if (maximum != null && maximum.length > 0) maximum[0] = maximum$.get(JAVA_DOUBLE, 0);
            if (step != null && step.length > 0) step[0] = step$.get(JAVA_DOUBLE, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rspin_box_set_step$;

    public static void Rspin_box_set_step(int spin_box, double step) {
        MethodHandle $h = Rspin_box_set_step$;
        if ($h == null) Rspin_box_set_step$ = $h = find("Rspin_box_set_step", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_DOUBLE));
        try {
            $h.invokeExact(spin_box, step);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rspin_box_set_decimals$;

    /**
     * Digits shown after the decimal point. Default: 2.
     */
    public static void Rspin_box_set_decimals(int spin_box, int decimals) {
        MethodHandle $h = Rspin_box_set_decimals$;
        if ($h == null) Rspin_box_set_decimals$ = $h = find("Rspin_box_set_decimals", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(spin_box, decimals);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rspin_box_set_changed_callback$;

    public static void Rspin_box_set_changed_callback(int spin_box, RSpinBoxChangedCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rspin_box_set_changed_callback$;
        if ($h == null) Rspin_box_set_changed_callback$ = $h = find("Rspin_box_set_changed_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(spin_box, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rspin_box_set_changed_callback(int spin_box, RSpinBoxChangedCallback callback) {
        Rspin_box_set_changed_callback(spin_box, callback, (MemorySegment) null);
    }

    private static MethodHandle Rcombo_box_add_item$;

    /**
     * Returns: The index of the new item.
     */
    public static int Rcombo_box_add_item(int combo_box, String text) {
        MethodHandle $h = Rcombo_box_add_item$;
        if ($h == null) Rcombo_box_add_item$ = $h = find("Rcombo_box_add_item", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            int $r = (int) $h.invokeExact(combo_box, text$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcombo_box_clear_items$;

    public static void Rcombo_box_clear_items(int combo_box) {
        MethodHandle $h = Rcombo_box_clear_items$;
        if ($h == null) Rcombo_box_clear_items$ = $h = find("Rcombo_box_clear_items", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(combo_box);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcombo_box_get_item_count$;

    public static int Rcombo_box_get_item_count(int combo_box) {
        MethodHandle $h = Rcombo_box_get_item_count$;
        if ($h == null) Rcombo_box_get_item_count$ = $h = find("Rcombo_box_get_item_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(combo_box);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcombo_box_get_item_text$;

    public static String Rcombo_box_get_item_text(int combo_box, int index) {
        MethodHandle $h = Rcombo_box_get_item_text$;
        if ($h == null) Rcombo_box_get_item_text$ = $h = find("Rcombo_box_get_item_text", FunctionDescriptor.of(ADDRESS, JAVA_INT, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(combo_box, index);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcombo_box_set_selected$;

    /**
     * Selects an item. -1 selects nothing.
     */
    public static void Rcombo_box_set_selected(int combo_box, int index) {
        MethodHandle $h = Rcombo_box_set_selected$;
        if ($h == null) Rcombo_box_set_selected$ = $h = find("Rcombo_box_set_selected", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(combo_box, index);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcombo_box_get_selected$;

    public static int Rcombo_box_get_selected(int combo_box) {
        MethodHandle $h = Rcombo_box_get_selected$;
        if ($h == null) Rcombo_box_get_selected$ = $h = find("Rcombo_box_get_selected", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(combo_box);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcombo_box_set_changed_callback$;

    public static void Rcombo_box_set_changed_callback(int combo_box, RComboBoxChangedCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rcombo_box_set_changed_callback$;
        if ($h == null) Rcombo_box_set_changed_callback$ = $h = find("Rcombo_box_set_changed_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(combo_box, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rcombo_box_set_changed_callback(int combo_box, RComboBoxChangedCallback callback) {
        Rcombo_box_set_changed_callback(combo_box, callback, (MemorySegment) null);
    }

    private static MethodHandle Rseparator_set_orientation$;

    /**
     * R_HORIZONTAL draws a horizontal line (default).
     */
    public static void Rseparator_set_orientation(int separator, int orientation) {
        MethodHandle $h = Rseparator_set_orientation$;
        if ($h == null) Rseparator_set_orientation$ = $h = find("Rseparator_set_orientation", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(separator, orientation);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlist_view_add_item$;

    /**
     * Returns: The index of the new item.
     */
    public static int Rlist_view_add_item(int list_view, String text) {
        MethodHandle $h = Rlist_view_add_item$;
        if ($h == null) Rlist_view_add_item$ = $h = find("Rlist_view_add_item", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            int $r = (int) $h.invokeExact(list_view, text$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlist_view_set_item_text$;

    public static void Rlist_view_set_item_text(int list_view, int index, String text) {
        MethodHandle $h = Rlist_view_set_item_text$;
        if ($h == null) Rlist_view_set_item_text$ = $h = find("Rlist_view_set_item_text", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            $h.invokeExact(list_view, index, text$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlist_view_get_item_text$;

    public static String Rlist_view_get_item_text(int list_view, int index) {
        MethodHandle $h = Rlist_view_get_item_text$;
        if ($h == null) Rlist_view_get_item_text$ = $h = find("Rlist_view_get_item_text", FunctionDescriptor.of(ADDRESS, JAVA_INT, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(list_view, index);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlist_view_remove_item$;

    public static void Rlist_view_remove_item(int list_view, int index) {
        MethodHandle $h = Rlist_view_remove_item$;
        if ($h == null) Rlist_view_remove_item$ = $h = find("Rlist_view_remove_item", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(list_view, index);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlist_view_clear_items$;

    public static void Rlist_view_clear_items(int list_view) {
        MethodHandle $h = Rlist_view_clear_items$;
        if ($h == null) Rlist_view_clear_items$ = $h = find("Rlist_view_clear_items", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(list_view);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlist_view_get_item_count$;

    public static int Rlist_view_get_item_count(int list_view) {
        MethodHandle $h = Rlist_view_get_item_count$;
        if ($h == null) Rlist_view_get_item_count$ = $h = find("Rlist_view_get_item_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(list_view);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlist_view_set_virtual_items$;

    public static void Rlist_view_set_virtual_items(int list_view, int count, RListItemTextCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rlist_view_set_virtual_items$;
        if ($h == null) Rlist_view_set_virtual_items$ = $h = find("Rlist_view_set_virtual_items", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(list_view, count, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rlist_view_set_virtual_items(int list_view, int count, RListItemTextCallback callback) {
        Rlist_view_set_virtual_items(list_view, count, callback, (MemorySegment) null);
    }

    private static MethodHandle Rlist_view_set_selected$;

    /**
     * Selects a row. -1 selects nothing.
     */
    public static void Rlist_view_set_selected(int list_view, int index) {
        MethodHandle $h = Rlist_view_set_selected$;
        if ($h == null) Rlist_view_set_selected$ = $h = find("Rlist_view_set_selected", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(list_view, index);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlist_view_get_selected$;

    public static int Rlist_view_get_selected(int list_view) {
        MethodHandle $h = Rlist_view_get_selected$;
        if ($h == null) Rlist_view_get_selected$ = $h = find("Rlist_view_get_selected", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(list_view);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlist_view_scroll_to_item$;

    public static void Rlist_view_scroll_to_item(int list_view, int index) {
        MethodHandle $h = Rlist_view_scroll_to_item$;
        if ($h == null) Rlist_view_scroll_to_item$ = $h = find("Rlist_view_scroll_to_item", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(list_view, index);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlist_view_set_selection_changed_callback$;

    public static void Rlist_view_set_selection_changed_callback(int list_view, RListEventCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rlist_view_set_selection_changed_callback$;
        if ($h == null) Rlist_view_set_selection_changed_callback$ = $h = find("Rlist_view_set_selection_changed_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(list_view, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rlist_view_set_selection_changed_callback(int list_view, RListEventCallback callback) {
        Rlist_view_set_selection_changed_callback(list_view, callback, (MemorySegment) null);
    }

    private static MethodHandle Rlist_view_set_activated_callback$;

    /**
     * Called on double click or Enter. This also fires the action of the widget.
     */
    public static void Rlist_view_set_activated_callback(int list_view, RListEventCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rlist_view_set_activated_callback$;
        if ($h == null) Rlist_view_set_activated_callback$ = $h = find("Rlist_view_set_activated_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(list_view, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Called on double click or Enter. This also fires the action of the widget.
     */
    public static void Rlist_view_set_activated_callback(int list_view, RListEventCallback callback) {
        Rlist_view_set_activated_callback(list_view, callback, (MemorySegment) null);
    }

    private static MethodHandle Rtree_view_add_item$;

    /**
     * Adds an item.
     * parent_item Item to add under, or R_INVALID_ID for a top-level item.
     * Returns: The handle of the new item.
     */
    public static int Rtree_view_add_item(int tree_view, int parent_item, String text) {
        MethodHandle $h = Rtree_view_add_item$;
        if ($h == null) Rtree_view_add_item$ = $h = find("Rtree_view_add_item", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            int $r = (int) $h.invokeExact(tree_view, parent_item, text$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_item_destroy$;

    /**
     * Deletes an item and its descendants.
     */
    public static void Rtree_item_destroy(int item) {
        MethodHandle $h = Rtree_item_destroy$;
        if ($h == null) Rtree_item_destroy$ = $h = find("Rtree_item_destroy", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(item);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_view_clear_items$;

    public static void Rtree_view_clear_items(int tree_view) {
        MethodHandle $h = Rtree_view_clear_items$;
        if ($h == null) Rtree_view_clear_items$ = $h = find("Rtree_view_clear_items", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(tree_view);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_item_is_valid$;

    public static int Rtree_item_is_valid(int item) {
        MethodHandle $h = Rtree_item_is_valid$;
        if ($h == null) Rtree_item_is_valid$ = $h = find("Rtree_item_is_valid", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(item);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_item_set_text$;

    public static void Rtree_item_set_text(int item, String text) {
        MethodHandle $h = Rtree_item_set_text$;
        if ($h == null) Rtree_item_set_text$ = $h = find("Rtree_item_set_text", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            $h.invokeExact(item, text$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_item_get_text$;

    public static String Rtree_item_get_text(int item) {
        MethodHandle $h = Rtree_item_get_text$;
        if ($h == null) Rtree_item_get_text$ = $h = find("Rtree_item_get_text", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(item);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_item_set_expanded$;

    public static void Rtree_item_set_expanded(int item, int expanded) {
        MethodHandle $h = Rtree_item_set_expanded$;
        if ($h == null) Rtree_item_set_expanded$ = $h = find("Rtree_item_set_expanded", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(item, expanded);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_item_is_expanded$;

    public static int Rtree_item_is_expanded(int item) {
        MethodHandle $h = Rtree_item_is_expanded$;
        if ($h == null) Rtree_item_is_expanded$ = $h = find("Rtree_item_is_expanded", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(item);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_item_set_user_handle$;

    public static void Rtree_item_set_user_handle(int item, MemorySegment handle) {
        MethodHandle $h = Rtree_item_set_user_handle$;
        if ($h == null) Rtree_item_set_user_handle$ = $h = find("Rtree_item_set_user_handle", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try {
            $h.invokeExact(item, (MemorySegment) (handle == null ? MemorySegment.NULL : handle));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_item_get_user_handle$;

    public static MemorySegment Rtree_item_get_user_handle(int item) {
        MethodHandle $h = Rtree_item_get_user_handle$;
        if ($h == null) Rtree_item_get_user_handle$ = $h = find("Rtree_item_get_user_handle", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(item);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_view_get_item_count$;

    /**
     * Top-level items of a tree.
     */
    public static int Rtree_view_get_item_count(int tree_view) {
        MethodHandle $h = Rtree_view_get_item_count$;
        if ($h == null) Rtree_view_get_item_count$ = $h = find("Rtree_view_get_item_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(tree_view);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_view_get_item$;

    public static int Rtree_view_get_item(int tree_view, int index) {
        MethodHandle $h = Rtree_view_get_item$;
        if ($h == null) Rtree_view_get_item$ = $h = find("Rtree_view_get_item", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(tree_view, index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_item_get_parent$;

    /**
     * Returns: The parent item, or R_INVALID_ID for a top-level item.
     */
    public static int Rtree_item_get_parent(int item) {
        MethodHandle $h = Rtree_item_get_parent$;
        if ($h == null) Rtree_item_get_parent$ = $h = find("Rtree_item_get_parent", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(item);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_item_get_child_count$;

    public static int Rtree_item_get_child_count(int item) {
        MethodHandle $h = Rtree_item_get_child_count$;
        if ($h == null) Rtree_item_get_child_count$ = $h = find("Rtree_item_get_child_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(item);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_item_get_child$;

    public static int Rtree_item_get_child(int item, int index) {
        MethodHandle $h = Rtree_item_get_child$;
        if ($h == null) Rtree_item_get_child$ = $h = find("Rtree_item_get_child", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(item, index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_view_set_selected_item$;

    /**
     * Selects an item. R_INVALID_ID selects nothing.
     */
    public static void Rtree_view_set_selected_item(int tree_view, int item) {
        MethodHandle $h = Rtree_view_set_selected_item$;
        if ($h == null) Rtree_view_set_selected_item$ = $h = find("Rtree_view_set_selected_item", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(tree_view, item);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_view_get_selected_item$;

    public static int Rtree_view_get_selected_item(int tree_view) {
        MethodHandle $h = Rtree_view_get_selected_item$;
        if ($h == null) Rtree_view_get_selected_item$ = $h = find("Rtree_view_get_selected_item", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(tree_view);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_view_set_selection_changed_callback$;

    public static void Rtree_view_set_selection_changed_callback(int tree_view, RTreeEventCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rtree_view_set_selection_changed_callback$;
        if ($h == null) Rtree_view_set_selection_changed_callback$ = $h = find("Rtree_view_set_selection_changed_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(tree_view, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rtree_view_set_selection_changed_callback(int tree_view, RTreeEventCallback callback) {
        Rtree_view_set_selection_changed_callback(tree_view, callback, (MemorySegment) null);
    }

    private static MethodHandle Rtree_view_set_activated_callback$;

    /**
     * Called on double click or Enter. This also fires the action of the widget.
     */
    public static void Rtree_view_set_activated_callback(int tree_view, RTreeEventCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rtree_view_set_activated_callback$;
        if ($h == null) Rtree_view_set_activated_callback$ = $h = find("Rtree_view_set_activated_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(tree_view, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Called on double click or Enter. This also fires the action of the widget.
     */
    public static void Rtree_view_set_activated_callback(int tree_view, RTreeEventCallback callback) {
        Rtree_view_set_activated_callback(tree_view, callback, (MemorySegment) null);
    }

    private static MethodHandle Rtree_item_set_icon$;

    /**
     * Icon drawn before the text of the item (a region u0,v0,u1,v1 of the texture). R_INVALID_ID removes it.
     */
    public static void Rtree_item_set_icon(int item, int texture, float u0, float v0, float u1, float v1) {
        MethodHandle $h = Rtree_item_set_icon$;
        if ($h == null) Rtree_item_set_icon$ = $h = find("Rtree_item_set_icon", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(item, texture, u0, v0, u1, v1);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_item_set_text_color$;

    /**
     * Color of the text of the item; alpha 0 goes back to the theme.
     */
    public static void Rtree_item_set_text_color(int item, float r, float g, float b, float a) {
        MethodHandle $h = Rtree_item_set_text_color$;
        if ($h == null) Rtree_item_set_text_color$ = $h = find("Rtree_item_set_text_color", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(item, r, g, b, a);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_view_set_multi_select$;

    /**
     * Several items can be selected: Ctrl + click adds or removes one,
     * Shift + click selects a range. The selection callback receives the item
     * clicked last (the primary one, Rtree_view_get_selected_item).
     */
    public static void Rtree_view_set_multi_select(int tree_view, int enabled) {
        MethodHandle $h = Rtree_view_set_multi_select$;
        if ($h == null) Rtree_view_set_multi_select$ = $h = find("Rtree_view_set_multi_select", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(tree_view, enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_item_set_selected$;

    public static void Rtree_item_set_selected(int item, int selected) {
        MethodHandle $h = Rtree_item_set_selected$;
        if ($h == null) Rtree_item_set_selected$ = $h = find("Rtree_item_set_selected", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(item, selected);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_item_is_selected$;

    public static int Rtree_item_is_selected(int item) {
        MethodHandle $h = Rtree_item_is_selected$;
        if ($h == null) Rtree_item_is_selected$ = $h = find("Rtree_item_is_selected", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(item);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_view_clear_selection$;

    public static void Rtree_view_clear_selection(int tree_view) {
        MethodHandle $h = Rtree_view_clear_selection$;
        if ($h == null) Rtree_view_clear_selection$ = $h = find("Rtree_view_clear_selection", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(tree_view);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_view_get_selection_count$;

    /**
     * Selected items, top to bottom.
     */
    public static int Rtree_view_get_selection_count(int tree_view) {
        MethodHandle $h = Rtree_view_get_selection_count$;
        if ($h == null) Rtree_view_get_selection_count$ = $h = find("Rtree_view_get_selection_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(tree_view);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_view_get_selection$;

    public static int Rtree_view_get_selection(int tree_view, int index) {
        MethodHandle $h = Rtree_view_get_selection$;
        if ($h == null) Rtree_view_get_selection$ = $h = find("Rtree_view_get_selection", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(tree_view, index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_view_scroll_to_item$;

    public static void Rtree_view_scroll_to_item(int tree_view, int item) {
        MethodHandle $h = Rtree_view_scroll_to_item$;
        if ($h == null) Rtree_view_scroll_to_item$ = $h = find("Rtree_view_scroll_to_item", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(tree_view, item);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_view_set_context_callback$;

    public static void Rtree_view_set_context_callback(int tree_view, RTreeContextCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rtree_view_set_context_callback$;
        if ($h == null) Rtree_view_set_context_callback$ = $h = find("Rtree_view_set_context_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(tree_view, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rtree_view_set_context_callback(int tree_view, RTreeContextCallback callback) {
        Rtree_view_set_context_callback(tree_view, callback, (MemorySegment) null);
    }

    private static MethodHandle Rtree_view_set_drag_type$;

    /**
     * Items can be dragged: the selection is given to Rdrag_begin with
     * this type and, as data, the handles of the items in decimal separated
     * by spaces. NULL: no drag (default).
     */
    public static void Rtree_view_set_drag_type(int tree_view, String type) {
        MethodHandle $h = Rtree_view_set_drag_type$;
        if ($h == null) Rtree_view_set_drag_type$ = $h = find("Rtree_view_set_drag_type", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment type$ = type == null ? MemorySegment.NULL : utf8($a, type);
            $h.invokeExact(tree_view, type$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_view_get_item_at$;

    /**
     * Item at a point relative to the tree (drop callbacks give local_x, local_y), or R_INVALID_ID.
     */
    public static int Rtree_view_get_item_at(int tree_view, float local_x, float local_y) {
        MethodHandle $h = Rtree_view_get_item_at$;
        if ($h == null) Rtree_view_get_item_at$ = $h = find("Rtree_view_get_item_at", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            int $r = (int) $h.invokeExact(tree_view, local_x, local_y);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtree_view_set_drop_highlight$;

    /**
     * Highlights where a drop would go: an item, or the whole tree (`whole_tree`). R_INVALID_ID, 0 removes it.
     */
    public static void Rtree_view_set_drop_highlight(int tree_view, int item, int whole_tree) {
        MethodHandle $h = Rtree_view_set_drop_highlight$;
        if ($h == null) Rtree_view_set_drop_highlight$ = $h = find("Rtree_view_set_drop_highlight", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(tree_view, item, whole_tree);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rmenu_add_menu$;

    /**
     * Creates a menu inside a menu bar or as a sub-menu of another menu.
     */
    public static int Rmenu_add_menu(int menu_bar_or_menu, String title) {
        MethodHandle $h = Rmenu_add_menu$;
        if ($h == null) Rmenu_add_menu$ = $h = find("Rmenu_add_menu", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment title$ = title == null ? MemorySegment.NULL : utf8($a, title);
            int $r = (int) $h.invokeExact(menu_bar_or_menu, title$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rmenu_set_title$;

    public static void Rmenu_set_title(int menu, String title) {
        MethodHandle $h = Rmenu_set_title$;
        if ($h == null) Rmenu_set_title$ = $h = find("Rmenu_set_title", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment title$ = title == null ? MemorySegment.NULL : utf8($a, title);
            $h.invokeExact(menu, title$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rmenu_add_item$;

    public static int Rmenu_add_item(int menu, String text) {
        MethodHandle $h = Rmenu_add_item$;
        if ($h == null) Rmenu_add_item$ = $h = find("Rmenu_add_item", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            int $r = (int) $h.invokeExact(menu, text$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rmenu_add_separator$;

    public static int Rmenu_add_separator(int menu) {
        MethodHandle $h = Rmenu_add_separator$;
        if ($h == null) Rmenu_add_separator$ = $h = find("Rmenu_add_separator", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(menu);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rmenu_item_set_text$;

    public static void Rmenu_item_set_text(int item, String text) {
        MethodHandle $h = Rmenu_item_set_text$;
        if ($h == null) Rmenu_item_set_text$ = $h = find("Rmenu_item_set_text", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            $h.invokeExact(item, text$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rmenu_item_get_text$;

    public static String Rmenu_item_get_text(int item) {
        MethodHandle $h = Rmenu_item_get_text$;
        if ($h == null) Rmenu_item_get_text$ = $h = find("Rmenu_item_get_text", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(item);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rmenu_item_set_shortcut$;

    /**
     * Shows a shortcut next to the item and triggers the item when the
     * keys are pressed in the window of its menu bar.
     */
    public static void Rmenu_item_set_shortcut(int item, int key, int modifiers) {
        MethodHandle $h = Rmenu_item_set_shortcut$;
        if ($h == null) Rmenu_item_set_shortcut$ = $h = find("Rmenu_item_set_shortcut", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(item, key, modifiers);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rmenu_item_set_checkable$;

    public static void Rmenu_item_set_checkable(int item, int checkable) {
        MethodHandle $h = Rmenu_item_set_checkable$;
        if ($h == null) Rmenu_item_set_checkable$ = $h = find("Rmenu_item_set_checkable", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(item, checkable);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rmenu_item_set_checked$;

    public static void Rmenu_item_set_checked(int item, int checked) {
        MethodHandle $h = Rmenu_item_set_checked$;
        if ($h == null) Rmenu_item_set_checked$ = $h = find("Rmenu_item_set_checked", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(item, checked);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rmenu_item_set_icon$;

    /**
     * Icon drawn before the text of a menu item (after the check mark column).
     * texture R_INVALID_ID removes it. u0, v0, u1, v1 : region of the texture.
     * tint    R_TRUE: the texture is a mask drawn in the text color (monochrome icons).
     */
    public static void Rmenu_item_set_icon(int item, int texture, float u0, float v0, float u1, float v1, int tint) {
        MethodHandle $h = Rmenu_item_set_icon$;
        if ($h == null) Rmenu_item_set_icon$ = $h = find("Rmenu_item_set_icon", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_INT));
        try {
            $h.invokeExact(item, texture, u0, v0, u1, v1, tint);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rmenu_item_is_checked$;

    public static int Rmenu_item_is_checked(int item) {
        MethodHandle $h = Rmenu_item_is_checked$;
        if ($h == null) Rmenu_item_is_checked$ = $h = find("Rmenu_item_is_checked", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(item);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rmenu_item_set_clicked_callback$;

    public static void Rmenu_item_set_clicked_callback(int item, RMenuItemClickedCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rmenu_item_set_clicked_callback$;
        if ($h == null) Rmenu_item_set_clicked_callback$ = $h = find("Rmenu_item_set_clicked_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(item, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rmenu_item_set_clicked_callback(int item, RMenuItemClickedCallback callback) {
        Rmenu_item_set_clicked_callback(item, callback, (MemorySegment) null);
    }

    private static MethodHandle Rmenu_open$;

    /**
     * Opens a menu as a popup at a position of the window (logical units).
     */
    public static void Rmenu_open(int menu, int window, float x, float y) {
        MethodHandle $h = Rmenu_open$;
        if ($h == null) Rmenu_open$ = $h = find("Rmenu_open", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(menu, window, x, y);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_close_popups$;

    /**
     * Closes every open menu and popup of the window.
     */
    public static void Rwindow_close_popups(int window) {
        MethodHandle $h = Rwindow_close_popups$;
        if ($h == null) Rwindow_close_popups$ = $h = find("Rwindow_close_popups", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(window);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtile_view_set_items$;

    /**
     * Gives the tiles: `count` of them, described by `callback`. The selection is cleared.
     */
    public static void Rtile_view_set_items(int tile_view, int count, RTileItemCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rtile_view_set_items$;
        if ($h == null) Rtile_view_set_items$ = $h = find("Rtile_view_set_items", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(tile_view, count, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Gives the tiles: `count` of them, described by `callback`. The selection is cleared.
     */
    public static void Rtile_view_set_items(int tile_view, int count, RTileItemCallback callback) {
        Rtile_view_set_items(tile_view, count, callback, (MemorySegment) null);
    }

    private static MethodHandle Rtile_view_refresh$;

    /**
     * Draws the tiles again (their description changed, not their number).
     */
    public static void Rtile_view_refresh(int tile_view) {
        MethodHandle $h = Rtile_view_refresh$;
        if ($h == null) Rtile_view_refresh$ = $h = find("Rtile_view_refresh", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(tile_view);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtile_view_get_count$;

    public static int Rtile_view_get_count(int tile_view) {
        MethodHandle $h = Rtile_view_get_count$;
        if ($h == null) Rtile_view_get_count$ = $h = find("Rtile_view_get_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(tile_view);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtile_view_set_tile_size$;

    /**
     * Size of a tile (its name included) and of its icon. Default 112 x 112, icon 64.
     */
    public static void Rtile_view_set_tile_size(int tile_view, float width, float height, float icon_size) {
        MethodHandle $h = Rtile_view_set_tile_size$;
        if ($h == null) Rtile_view_set_tile_size$ = $h = find("Rtile_view_set_tile_size", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(tile_view, width, height, icon_size);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtile_view_set_empty_text$;

    public static void Rtile_view_set_empty_text(int tile_view, String text) {
        MethodHandle $h = Rtile_view_set_empty_text$;
        if ($h == null) Rtile_view_set_empty_text$ = $h = find("Rtile_view_set_empty_text", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            $h.invokeExact(tile_view, text$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtile_view_set_selected$;

    public static void Rtile_view_set_selected(int tile_view, int index, int selected) {
        MethodHandle $h = Rtile_view_set_selected$;
        if ($h == null) Rtile_view_set_selected$ = $h = find("Rtile_view_set_selected", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(tile_view, index, selected);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtile_view_is_selected$;

    public static int Rtile_view_is_selected(int tile_view, int index) {
        MethodHandle $h = Rtile_view_is_selected$;
        if ($h == null) Rtile_view_is_selected$ = $h = find("Rtile_view_is_selected", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(tile_view, index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtile_view_clear_selection$;

    public static void Rtile_view_clear_selection(int tile_view) {
        MethodHandle $h = Rtile_view_clear_selection$;
        if ($h == null) Rtile_view_clear_selection$ = $h = find("Rtile_view_clear_selection", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(tile_view);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtile_view_select_only$;

    public static void Rtile_view_select_only(int tile_view, int index) {
        MethodHandle $h = Rtile_view_select_only$;
        if ($h == null) Rtile_view_select_only$ = $h = find("Rtile_view_select_only", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(tile_view, index);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtile_view_get_current$;

    /**
     * The tile clicked last, or -1.
     */
    public static int Rtile_view_get_current(int tile_view) {
        MethodHandle $h = Rtile_view_get_current$;
        if ($h == null) Rtile_view_get_current$ = $h = find("Rtile_view_get_current", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(tile_view);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtile_view_get_selection_count$;

    public static int Rtile_view_get_selection_count(int tile_view) {
        MethodHandle $h = Rtile_view_get_selection_count$;
        if ($h == null) Rtile_view_get_selection_count$ = $h = find("Rtile_view_get_selection_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(tile_view);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtile_view_get_selection$;

    /**
     * Index of the nth selected tile, in order.
     */
    public static int Rtile_view_get_selection(int tile_view, int nth) {
        MethodHandle $h = Rtile_view_get_selection$;
        if ($h == null) Rtile_view_get_selection$ = $h = find("Rtile_view_get_selection", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(tile_view, nth);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtile_view_scroll_to$;

    public static void Rtile_view_scroll_to(int tile_view, int index) {
        MethodHandle $h = Rtile_view_scroll_to$;
        if ($h == null) Rtile_view_scroll_to$ = $h = find("Rtile_view_scroll_to", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(tile_view, index);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtile_view_get_item_at$;

    /**
     * Tile at a point relative to the widget (drop callbacks give local_x, local_y), or -1.
     */
    public static int Rtile_view_get_item_at(int tile_view, float local_x, float local_y) {
        MethodHandle $h = Rtile_view_get_item_at$;
        if ($h == null) Rtile_view_get_item_at$ = $h = find("Rtile_view_get_item_at", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            int $r = (int) $h.invokeExact(tile_view, local_x, local_y);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtile_view_set_drop_highlight$;

    /**
     * Highlights the tile a drop would go to; -1 removes it.
     */
    public static void Rtile_view_set_drop_highlight(int tile_view, int index) {
        MethodHandle $h = Rtile_view_set_drop_highlight$;
        if ($h == null) Rtile_view_set_drop_highlight$ = $h = find("Rtile_view_set_drop_highlight", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(tile_view, index);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtile_view_set_drag_type$;

    /**
     * Tiles can be dragged: the selected tiles are given to Rdrag_begin
     * with this type and, as data, their indices separated by spaces.
     */
    public static void Rtile_view_set_drag_type(int tile_view, String type) {
        MethodHandle $h = Rtile_view_set_drag_type$;
        if ($h == null) Rtile_view_set_drag_type$ = $h = find("Rtile_view_set_drag_type", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment type$ = type == null ? MemorySegment.NULL : utf8($a, type);
            $h.invokeExact(tile_view, type$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtile_view_set_drag_data_callback$;

    public static void Rtile_view_set_drag_data_callback(int tile_view, RTileDragDataCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rtile_view_set_drag_data_callback$;
        if ($h == null) Rtile_view_set_drag_data_callback$ = $h = find("Rtile_view_set_drag_data_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(tile_view, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rtile_view_set_drag_data_callback(int tile_view, RTileDragDataCallback callback) {
        Rtile_view_set_drag_data_callback(tile_view, callback, (MemorySegment) null);
    }

    private static MethodHandle Rtile_view_set_selection_changed_callback$;

    public static void Rtile_view_set_selection_changed_callback(int tile_view, RTileEventCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rtile_view_set_selection_changed_callback$;
        if ($h == null) Rtile_view_set_selection_changed_callback$ = $h = find("Rtile_view_set_selection_changed_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(tile_view, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rtile_view_set_selection_changed_callback(int tile_view, RTileEventCallback callback) {
        Rtile_view_set_selection_changed_callback(tile_view, callback, (MemorySegment) null);
    }

    private static MethodHandle Rtile_view_set_activated_callback$;

    public static void Rtile_view_set_activated_callback(int tile_view, RTileEventCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rtile_view_set_activated_callback$;
        if ($h == null) Rtile_view_set_activated_callback$ = $h = find("Rtile_view_set_activated_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(tile_view, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rtile_view_set_activated_callback(int tile_view, RTileEventCallback callback) {
        Rtile_view_set_activated_callback(tile_view, callback, (MemorySegment) null);
    }

    private static MethodHandle Rtile_view_set_context_callback$;

    public static void Rtile_view_set_context_callback(int tile_view, RTileContextCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rtile_view_set_context_callback$;
        if ($h == null) Rtile_view_set_context_callback$ = $h = find("Rtile_view_set_context_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(tile_view, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rtile_view_set_context_callback(int tile_view, RTileContextCallback callback) {
        Rtile_view_set_context_callback(tile_view, callback, (MemorySegment) null);
    }

    private static MethodHandle Rtext_editor_create$;

    /**
     * text Initial content, or NULL.
     */
    public static int Rtext_editor_create(String text) {
        MethodHandle $h = Rtext_editor_create$;
        if ($h == null) Rtext_editor_create$ = $h = find("Rtext_editor_create", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            int $r = (int) $h.invokeExact(text$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_set_text$;

    /**
     * Replaces the whole text. The history of undo is cleared; the changed callback is not called.
     */
    public static void Rtext_editor_set_text(int editor, String text) {
        MethodHandle $h = Rtext_editor_set_text$;
        if ($h == null) Rtext_editor_set_text$ = $h = find("Rtext_editor_set_text", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            $h.invokeExact(editor, text$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_get_text$;

    /**
     * Returns: The whole text, lines separated by '\n'. Valid until the next call or the next edit.
     */
    public static String Rtext_editor_get_text(int editor) {
        MethodHandle $h = Rtext_editor_get_text$;
        if ($h == null) Rtext_editor_get_text$ = $h = find("Rtext_editor_get_text", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(editor);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_get_length$;

    /**
     * Returns: The number of bytes of Rtext_editor_get_text.
     */
    public static int Rtext_editor_get_length(int editor) {
        MethodHandle $h = Rtext_editor_get_length$;
        if ($h == null) Rtext_editor_get_length$ = $h = find("Rtext_editor_get_length", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(editor);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_get_line_count$;

    public static int Rtext_editor_get_line_count(int editor) {
        MethodHandle $h = Rtext_editor_get_line_count$;
        if ($h == null) Rtext_editor_get_line_count$ = $h = find("Rtext_editor_get_line_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(editor);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_get_line$;

    /**
     * Returns: One line, without its line break. Valid until the next edit. NULL when `line` is out of range.
     */
    public static String Rtext_editor_get_line(int editor, int line) {
        MethodHandle $h = Rtext_editor_get_line$;
        if ($h == null) Rtext_editor_get_line$ = $h = find("Rtext_editor_get_line", FunctionDescriptor.of(ADDRESS, JAVA_INT, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(editor, line);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_insert_text$;

    /**
     * Types a text at the cursor, replacing the selection, as the user
     * would: it can be undone and the changed callback is called.
     */
    public static void Rtext_editor_insert_text(int editor, String text) {
        MethodHandle $h = Rtext_editor_insert_text$;
        if ($h == null) Rtext_editor_insert_text$ = $h = find("Rtext_editor_insert_text", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            $h.invokeExact(editor, text$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_set_cursor$;

    public static void Rtext_editor_set_cursor(int editor, int line, int column) {
        MethodHandle $h = Rtext_editor_set_cursor$;
        if ($h == null) Rtext_editor_set_cursor$ = $h = find("Rtext_editor_set_cursor", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(editor, line, column);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_get_cursor$;

    public static void Rtext_editor_get_cursor(int editor, int[] line, int[] column) {
        MethodHandle $h = Rtext_editor_get_cursor$;
        if ($h == null) Rtext_editor_get_cursor$ = $h = find("Rtext_editor_get_cursor", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment line$ = line == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            MemorySegment column$ = column == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            $h.invokeExact(editor, line$, column$);
            if (line != null && line.length > 0) line[0] = line$.get(JAVA_INT, 0);
            if (column != null && column.length > 0) column[0] = column$.get(JAVA_INT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_select$;

    /**
     * Selects from one position to another; the cursor is on the second.
     */
    public static void Rtext_editor_select(int editor, int from_line, int from_column, int to_line, int to_column) {
        MethodHandle $h = Rtext_editor_select$;
        if ($h == null) Rtext_editor_select$ = $h = find("Rtext_editor_select", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(editor, from_line, from_column, to_line, to_column);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_select_all$;

    public static void Rtext_editor_select_all(int editor) {
        MethodHandle $h = Rtext_editor_select_all$;
        if ($h == null) Rtext_editor_select_all$ = $h = find("Rtext_editor_select_all", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(editor);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_get_selection$;

    /**
     * Start and end of the selection, in the order of the text. Returns R_FALSE when nothing is selected.
     */
    public static int Rtext_editor_get_selection(int editor, int[] from_line, int[] from_column, int[] to_line, int[] to_column) {
        MethodHandle $h = Rtext_editor_get_selection$;
        if ($h == null) Rtext_editor_get_selection$ = $h = find("Rtext_editor_get_selection", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment from_line$ = from_line == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            MemorySegment from_column$ = from_column == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            MemorySegment to_line$ = to_line == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            MemorySegment to_column$ = to_column == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            int $r = (int) $h.invokeExact(editor, from_line$, from_column$, to_line$, to_column$);
            if (from_line != null && from_line.length > 0) from_line[0] = from_line$.get(JAVA_INT, 0);
            if (from_column != null && from_column.length > 0) from_column[0] = from_column$.get(JAVA_INT, 0);
            if (to_line != null && to_line.length > 0) to_line[0] = to_line$.get(JAVA_INT, 0);
            if (to_column != null && to_column.length > 0) to_column[0] = to_column$.get(JAVA_INT, 0);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_get_selected_text$;

    /**
     * Returns: The selected text, "" when there is none. Valid until the next call or the next edit.
     */
    public static String Rtext_editor_get_selected_text(int editor) {
        MethodHandle $h = Rtext_editor_get_selected_text$;
        if ($h == null) Rtext_editor_get_selected_text$ = $h = find("Rtext_editor_get_selected_text", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(editor);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_scroll_to_line$;

    /**
     * Scrolls so that a line is visible.
     */
    public static void Rtext_editor_scroll_to_line(int editor, int line) {
        MethodHandle $h = Rtext_editor_scroll_to_line$;
        if ($h == null) Rtext_editor_scroll_to_line$ = $h = find("Rtext_editor_scroll_to_line", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(editor, line);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_undo$;

    public static void Rtext_editor_undo(int editor) {
        MethodHandle $h = Rtext_editor_undo$;
        if ($h == null) Rtext_editor_undo$ = $h = find("Rtext_editor_undo", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(editor);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_redo$;

    public static void Rtext_editor_redo(int editor) {
        MethodHandle $h = Rtext_editor_redo$;
        if ($h == null) Rtext_editor_redo$ = $h = find("Rtext_editor_redo", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(editor);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_can_undo$;

    public static int Rtext_editor_can_undo(int editor) {
        MethodHandle $h = Rtext_editor_can_undo$;
        if ($h == null) Rtext_editor_can_undo$ = $h = find("Rtext_editor_can_undo", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(editor);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_can_redo$;

    public static int Rtext_editor_can_redo(int editor) {
        MethodHandle $h = Rtext_editor_can_redo$;
        if ($h == null) Rtext_editor_can_redo$ = $h = find("Rtext_editor_can_redo", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(editor);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_set_read_only$;

    /**
     * A read-only editor can still be scrolled, selected and copied.
     */
    public static void Rtext_editor_set_read_only(int editor, int read_only) {
        MethodHandle $h = Rtext_editor_set_read_only$;
        if ($h == null) Rtext_editor_set_read_only$ = $h = find("Rtext_editor_set_read_only", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(editor, read_only);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_set_word_wrap$;

    /**
     * Long lines continue on the next row instead of scrolling sideways. Off by default.
     */
    public static void Rtext_editor_set_word_wrap(int editor, int enabled) {
        MethodHandle $h = Rtext_editor_set_word_wrap$;
        if ($h == null) Rtext_editor_set_word_wrap$ = $h = find("Rtext_editor_set_word_wrap", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(editor, enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_set_line_numbers$;

    /**
     * Shows the line numbers in a margin. Off by default.
     */
    public static void Rtext_editor_set_line_numbers(int editor, int enabled) {
        MethodHandle $h = Rtext_editor_set_line_numbers$;
        if ($h == null) Rtext_editor_set_line_numbers$ = $h = find("Rtext_editor_set_line_numbers", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(editor, enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_set_tab_size$;

    /**
     * Width of a tab, in spaces (4 by default).
     */
    public static void Rtext_editor_set_tab_size(int editor, int spaces) {
        MethodHandle $h = Rtext_editor_set_tab_size$;
        if ($h == null) Rtext_editor_set_tab_size$ = $h = find("Rtext_editor_set_tab_size", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(editor, spaces);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_set_tab_inserts_spaces$;

    /**
     * R_TRUE makes the Tab key insert spaces instead of a tab character. Off by default.
     */
    public static void Rtext_editor_set_tab_inserts_spaces(int editor, int enabled) {
        MethodHandle $h = Rtext_editor_set_tab_inserts_spaces$;
        if ($h == null) Rtext_editor_set_tab_inserts_spaces$ = $h = find("Rtext_editor_set_tab_inserts_spaces", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(editor, enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_set_syntax$;

    /**
     * The same as Rtext_editor_set_language with "XML", "C" or NULL.
     */
    public static void Rtext_editor_set_syntax(int editor, int syntax) {
        MethodHandle $h = Rtext_editor_set_syntax$;
        if ($h == null) Rtext_editor_set_syntax$ = $h = find("Rtext_editor_set_syntax", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(editor, syntax);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_set_font$;

    /**
     * font R_INVALID_ID uses the font of the theme (or the fixed-width
     * font, with a language).
     * size 0 uses the font size of the theme.
     */
    public static void Rtext_editor_set_font(int editor, int font, float size) {
        MethodHandle $h = Rtext_editor_set_font$;
        if ($h == null) Rtext_editor_set_font$ = $h = find("Rtext_editor_set_font", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(editor, font, size);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_set_placeholder$;

    /**
     * Text shown, dimmed, while the editor is empty.
     */
    public static void Rtext_editor_set_placeholder(int editor, String text) {
        MethodHandle $h = Rtext_editor_set_placeholder$;
        if ($h == null) Rtext_editor_set_placeholder$ = $h = find("Rtext_editor_set_placeholder", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            $h.invokeExact(editor, text$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_set_zoom$;

    /**
     * Zoom of the text: a factor of its font size, 1 = 100 % (0.5 .. 4).
     * The user zooms with Ctrl + wheel (around the pointer), Ctrl + '+',
     * Ctrl + '-' and Ctrl + 0 (back to 100 %), keypad included. Set by the
     * program, the top of the view stays and the zoom callback is not called.
     * XML: Zoom="1.25".
     */
    public static void Rtext_editor_set_zoom(int editor, float zoom) {
        MethodHandle $h = Rtext_editor_set_zoom$;
        if ($h == null) Rtext_editor_set_zoom$ = $h = find("Rtext_editor_set_zoom", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(editor, zoom);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_get_zoom$;

    public static float Rtext_editor_get_zoom(int editor) {
        MethodHandle $h = Rtext_editor_get_zoom$;
        if ($h == null) Rtext_editor_get_zoom$ = $h = find("Rtext_editor_get_zoom", FunctionDescriptor.of(JAVA_FLOAT, JAVA_INT));
        try {
            float $r = (float) $h.invokeExact(editor);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_set_zoomable$;

    /**
     * R_FALSE: Ctrl + wheel and the keys do not zoom (the wheel scrolls). On by default. XML: Zoomable="false".
     */
    public static void Rtext_editor_set_zoomable(int editor, int zoomable) {
        MethodHandle $h = Rtext_editor_set_zoomable$;
        if ($h == null) Rtext_editor_set_zoomable$ = $h = find("Rtext_editor_set_zoomable", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(editor, zoomable);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_set_zoom_callback$;

    public static void Rtext_editor_set_zoom_callback(int editor, RTextEditorZoomCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rtext_editor_set_zoom_callback$;
        if ($h == null) Rtext_editor_set_zoom_callback$ = $h = find("Rtext_editor_set_zoom_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(editor, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rtext_editor_set_zoom_callback(int editor, RTextEditorZoomCallback callback) {
        Rtext_editor_set_zoom_callback(editor, callback, (MemorySegment) null);
    }

    private static MethodHandle Rlanguage_register$;

    /**
     * Declares a language, or replaces the one that has this name: to
     * add words to a built-in language, read it with Rlanguage_get, change
     * what you want and register it again. The strings are copied.
     */
    public static int Rlanguage_register(RLanguageDesc desc) {
        MethodHandle $h = Rlanguage_register$;
        if ($h == null) Rlanguage_register$ = $h = find("Rlanguage_register", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try {
            int $r = (int) $h.invokeExact((MemorySegment) (desc == null ? MemorySegment.NULL : desc.segment()));
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlanguage_get$;

    /**
     * Reads the description of a language. The strings belong to the library. Returns R_FALSE when it is unknown.
     */
    public static int Rlanguage_get(String name, RLanguageDesc desc) {
        MethodHandle $h = Rlanguage_get$;
        if ($h == null) Rlanguage_get$ = $h = find("Rlanguage_get", FunctionDescriptor.of(JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            int $r = (int) $h.invokeExact(name$, (MemorySegment) (desc == null ? MemorySegment.NULL : desc.segment()));
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlanguage_get_count$;

    public static int Rlanguage_get_count() {
        MethodHandle $h = Rlanguage_get_count$;
        if ($h == null) Rlanguage_get_count$ = $h = find("Rlanguage_get_count", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlanguage_get_name$;

    public static String Rlanguage_get_name(int index) {
        MethodHandle $h = Rlanguage_get_name$;
        if ($h == null) Rlanguage_get_name$ = $h = find("Rlanguage_get_name", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(index);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlanguage_for_file$;

    /**
     * Returns: The name of the language of a file, from its extension, or NULL.
     */
    public static String Rlanguage_for_file(String path) {
        MethodHandle $h = Rlanguage_for_file$;
        if ($h == null) Rlanguage_for_file$ = $h = find("Rlanguage_for_file", FunctionDescriptor.of(ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment path$ = path == null ? MemorySegment.NULL : utf8($a, path);
            MemorySegment $r = (MemorySegment) $h.invokeExact(path$);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_set_language$;

    /**
     * Gives the editor a language, by name ("C++", "Python"...), by
     * another usual name ("cpp", "js") or by the extension of its files
     * ("rs"). NULL or "" removes it. An editor with a language uses the
     * fixed-width font unless it was given a font.
     */
    public static void Rtext_editor_set_language(int editor, String name) {
        MethodHandle $h = Rtext_editor_set_language$;
        if ($h == null) Rtext_editor_set_language$ = $h = find("Rtext_editor_set_language", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            $h.invokeExact(editor, name$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_get_language$;

    /**
     * Returns: The name of the language of the editor, or "".
     */
    public static String Rtext_editor_get_language(int editor) {
        MethodHandle $h = Rtext_editor_get_language$;
        if ($h == null) Rtext_editor_get_language$ = $h = find("Rtext_editor_get_language", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(editor);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_set_markdown$;

    /**
     * R_TRUE makes the editor an editor of Markdown (the "Markdown"
     * language; Markdown="true" in XML): the text stays as it is written,
     * and what it formats shows while typing, strong text in bold,
     * emphasized text in italic, titles in bold, code, links and marks in
     * their colors. A label with Rlabel_set_markdown shows the same text
     * formatted. R_FALSE removes the language.
     */
    public static void Rtext_editor_set_markdown(int editor, int enabled) {
        MethodHandle $h = Rtext_editor_set_markdown$;
        if ($h == null) Rtext_editor_set_markdown$ = $h = find("Rtext_editor_set_markdown", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(editor, enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_get_markdown$;

    public static int Rtext_editor_get_markdown(int editor) {
        MethodHandle $h = Rtext_editor_get_markdown$;
        if ($h == null) Rtext_editor_get_markdown$ = $h = find("Rtext_editor_get_markdown", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(editor);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_get_token$;

    /**
     * What the language of the editor makes of the character at a
     * position: the colors come from it. R_TOKEN_TEXT without a language.
     */
    public static int Rtext_editor_get_token(int editor, int line, int column) {
        MethodHandle $h = Rtext_editor_get_token$;
        if ($h == null) Rtext_editor_get_token$ = $h = find("Rtext_editor_get_token", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(editor, line, column);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_set_completion$;

    /**
     * R_FALSE stops the editor from proposing words by itself; Ctrl+Space still does. On by default.
     */
    public static void Rtext_editor_set_completion(int editor, int enabled) {
        MethodHandle $h = Rtext_editor_set_completion$;
        if ($h == null) Rtext_editor_set_completion$ = $h = find("Rtext_editor_set_completion", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(editor, enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_show_completions$;

    /**
     * Proposes words for what is before the cursor, as Ctrl+Space does.
     */
    public static void Rtext_editor_show_completions(int editor) {
        MethodHandle $h = Rtext_editor_show_completions$;
        if ($h == null) Rtext_editor_show_completions$ = $h = find("Rtext_editor_show_completions", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(editor);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_is_completing$;

    public static int Rtext_editor_is_completing(int editor) {
        MethodHandle $h = Rtext_editor_is_completing$;
        if ($h == null) Rtext_editor_is_completing$ = $h = find("Rtext_editor_is_completing", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(editor);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_set_completion_callback$;

    public static void Rtext_editor_set_completion_callback(int editor, RCompletionCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rtext_editor_set_completion_callback$;
        if ($h == null) Rtext_editor_set_completion_callback$ = $h = find("Rtext_editor_set_completion_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(editor, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rtext_editor_set_completion_callback(int editor, RCompletionCallback callback) {
        Rtext_editor_set_completion_callback(editor, callback, (MemorySegment) null);
    }

    private static MethodHandle Rtext_editor_add_completion$;

    /**
     * Adds a proposal. Only from a completion callback. Proposals that
     * do not start with the prefix are left out; the others keep the order
     * they were added in.
     * detail A few words shown beside it ("function", "int"), or NULL.
     */
    public static void Rtext_editor_add_completion(int editor, String text, String detail) {
        MethodHandle $h = Rtext_editor_add_completion$;
        if ($h == null) Rtext_editor_add_completion$ = $h = find("Rtext_editor_add_completion", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            MemorySegment detail$ = detail == null ? MemorySegment.NULL : utf8($a, detail);
            $h.invokeExact(editor, text$, detail$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_set_auto_pairs$;

    /**
     * R_FALSE stops the editor from closing brackets and quotes and
     * from indenting after them. On by default; it only acts with a language.
     */
    public static void Rtext_editor_set_auto_pairs(int editor, int enabled) {
        MethodHandle $h = Rtext_editor_set_auto_pairs$;
        if ($h == null) Rtext_editor_set_auto_pairs$ = $h = find("Rtext_editor_set_auto_pairs", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(editor, enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_add_marker$;

    /**
     * Underlines a part of a line and attaches a message to it, shown
     * when the pointer rests there: an error of a compiler, a warning.
     * Markers follow their line when text is added or removed above it, and
     * go away when their own line is edited.
     * to_column -1 for the end of the line.
     */
    public static void Rtext_editor_add_marker(int editor, int line, int from_column, int to_column, int kind, String message) {
        MethodHandle $h = Rtext_editor_add_marker$;
        if ($h == null) Rtext_editor_add_marker$ = $h = find("Rtext_editor_add_marker", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment message$ = message == null ? MemorySegment.NULL : utf8($a, message);
            $h.invokeExact(editor, line, from_column, to_column, kind, message$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_clear_markers$;

    public static void Rtext_editor_clear_markers(int editor) {
        MethodHandle $h = Rtext_editor_clear_markers$;
        if ($h == null) Rtext_editor_clear_markers$ = $h = find("Rtext_editor_clear_markers", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(editor);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_get_marker_count$;

    public static int Rtext_editor_get_marker_count(int editor) {
        MethodHandle $h = Rtext_editor_get_marker_count$;
        if ($h == null) Rtext_editor_get_marker_count$ = $h = find("Rtext_editor_get_marker_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(editor);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_set_token_color$;

    /**
     * Color of a kind of token in the theme in use, instead of the
     * one Raven picks for dark and for light themes. Theme files set them
     * with &lt;Syntax Keyword="#FF7AB2" String="..."/&gt;.
     */
    public static void Rtheme_set_token_color(int token, float r, float g, float b, float a) {
        MethodHandle $h = Rtheme_set_token_color$;
        if ($h == null) Rtheme_set_token_color$ = $h = find("Rtheme_set_token_color", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(token, r, g, b, a);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_get_token_color$;

    public static void Rtheme_get_token_color(int token, float[] r, float[] g, float[] b, float[] a) {
        MethodHandle $h = Rtheme_get_token_color$;
        if ($h == null) Rtheme_get_token_color$ = $h = find("Rtheme_get_token_color", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment r$ = r == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment g$ = g == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment b$ = b == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment a$ = a == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            $h.invokeExact(token, r$, g$, b$, a$);
            if (r != null && r.length > 0) r[0] = r$.get(JAVA_FLOAT, 0);
            if (g != null && g.length > 0) g[0] = g$.get(JAVA_FLOAT, 0);
            if (b != null && b.length > 0) b[0] = b$.get(JAVA_FLOAT, 0);
            if (a != null && a.length > 0) a[0] = a$.get(JAVA_FLOAT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtheme_clear_token_colors$;

    /**
     * Back to the colors Raven picks.
     */
    public static void Rtheme_clear_token_colors() {
        MethodHandle $h = Rtheme_clear_token_colors$;
        if ($h == null) Rtheme_clear_token_colors$ = $h = find("Rtheme_clear_token_colors", FunctionDescriptor.ofVoid());
        try {
            $h.invokeExact();
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtext_editor_set_changed_callback$;

    public static void Rtext_editor_set_changed_callback(int editor, RTextEditorChangedCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rtext_editor_set_changed_callback$;
        if ($h == null) Rtext_editor_set_changed_callback$ = $h = find("Rtext_editor_set_changed_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(editor, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rtext_editor_set_changed_callback(int editor, RTextEditorChangedCallback callback) {
        Rtext_editor_set_changed_callback(editor, callback, (MemorySegment) null);
    }

    private static MethodHandle Rheader_bar_create$;

    /**
     * title Text shown in the bar; NULL shows the title of the window.
     */
    public static int Rheader_bar_create(String title) {
        MethodHandle $h = Rheader_bar_create$;
        if ($h == null) Rheader_bar_create$ = $h = find("Rheader_bar_create", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment title$ = title == null ? MemorySegment.NULL : utf8($a, title);
            int $r = (int) $h.invokeExact(title$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rheader_bar_set_title$;

    /**
     * title NULL shows the title of the window.
     */
    public static void Rheader_bar_set_title(int bar, String title) {
        MethodHandle $h = Rheader_bar_set_title$;
        if ($h == null) Rheader_bar_set_title$ = $h = find("Rheader_bar_set_title", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment title$ = title == null ? MemorySegment.NULL : utf8($a, title);
            $h.invokeExact(bar, title$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rheader_bar_set_buttons$;

    /**
     * buttons Bitmask of RHeaderButtons. All by default; the maximize button only shows on a resizable window.
     */
    public static void Rheader_bar_set_buttons(int bar, int buttons) {
        MethodHandle $h = Rheader_bar_set_buttons$;
        if ($h == null) Rheader_bar_set_buttons$ = $h = find("Rheader_bar_set_buttons", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(bar, buttons);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rheader_bar_set_button_side$;

    /**
     * side R_HALIGN_RIGHT (default): minimize, maximize, close, as on
     * Windows. R_HALIGN_LEFT: three round colored buttons, as on macOS.
     */
    public static void Rheader_bar_set_button_side(int bar, int side) {
        MethodHandle $h = Rheader_bar_set_button_side$;
        if ($h == null) Rheader_bar_set_button_side$ = $h = find("Rheader_bar_set_button_side", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(bar, side);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rheader_bar_set_title_align$;

    /**
     * align R_HALIGN_CENTER (default) or R_HALIGN_LEFT.
     */
    public static void Rheader_bar_set_title_align(int bar, int align) {
        MethodHandle $h = Rheader_bar_set_title_align$;
        if ($h == null) Rheader_bar_set_title_align$ = $h = find("Rheader_bar_set_title_align", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(bar, align);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rheader_bar_set_title_notch$;

    /**
     * R_TRUE: the title sits in a dark tab hanging from the top edge of
     * the bar (slanted sides, round bottom corners). XML: TitleStyle="Notch".
     */
    public static void Rheader_bar_set_title_notch(int bar, int notch) {
        MethodHandle $h = Rheader_bar_set_title_notch$;
        if ($h == null) Rheader_bar_set_title_notch$ = $h = find("Rheader_bar_set_title_notch", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(bar, notch);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rheader_bar_set_background$;

    /**
     * R_FALSE removes the background of the bar: for a header that
     * floats above the content (in an Overlay) or sits inside a glass panel.
     */
    public static void Rheader_bar_set_background(int bar, int enabled) {
        MethodHandle $h = Rheader_bar_set_background$;
        if ($h == null) Rheader_bar_set_background$ = $h = find("Rheader_bar_set_background", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(bar, enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdock_area_create$;

    public static int Rdock_area_create() {
        MethodHandle $h = Rdock_area_create$;
        if ($h == null) Rdock_area_create$ = $h = find("Rdock_area_create", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdock_area_add_panel$;

    /**
     * Adds a panel. The dock area takes ownership of the widget.
     * title    Text of its tab.
     * side     Where it goes.
     * relative A panel of the area to split or to share tabs with, or
     *                 R_INVALID_ID for the whole area (with R_DOCK_TAB: the
     *                 first group of tabs).
     * size     Part of the split space given to the new panel, 0..1
     *                 (0 = a half, or a quarter along the whole area).
     */
    public static void Rdock_area_add_panel(int area, int panel, String title, int side, int relative, float size) {
        MethodHandle $h = Rdock_area_add_panel$;
        if ($h == null) Rdock_area_add_panel$ = $h = find("Rdock_area_add_panel", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS, JAVA_INT, JAVA_INT, JAVA_FLOAT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment title$ = title == null ? MemorySegment.NULL : utf8($a, title);
            $h.invokeExact(area, panel, title$, side, relative, size);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdock_area_move_panel$;

    /**
     * Moves a panel of the area somewhere else in it.
     */
    public static void Rdock_area_move_panel(int area, int panel, int side, int relative, float size) {
        MethodHandle $h = Rdock_area_move_panel$;
        if ($h == null) Rdock_area_move_panel$ = $h = find("Rdock_area_move_panel", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(area, panel, side, relative, size);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdock_area_close_panel$;

    /**
     * Removes a panel and deletes it.
     */
    public static void Rdock_area_close_panel(int area, int panel) {
        MethodHandle $h = Rdock_area_close_panel$;
        if ($h == null) Rdock_area_close_panel$ = $h = find("Rdock_area_close_panel", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(area, panel);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdock_area_set_panel_title$;

    public static void Rdock_area_set_panel_title(int area, int panel, String title) {
        MethodHandle $h = Rdock_area_set_panel_title$;
        if ($h == null) Rdock_area_set_panel_title$ = $h = find("Rdock_area_set_panel_title", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment title$ = title == null ? MemorySegment.NULL : utf8($a, title);
            $h.invokeExact(area, panel, title$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdock_area_set_panel_icon$;

    /**
     * Icon drawn before the title of the tab of a panel (and on the chip while it is dragged).
     * texture Texture of the icon, R_INVALID_ID removes it.
     * u0,v0,u1,v1 Region of the texture (0, 0, 1, 1 : all of it).
     * tint    R_TRUE: the texture is a mask drawn in the text color of the tab (monochrome icons).
     */
    public static void Rdock_area_set_panel_icon(int area, int panel, int texture, float u0, float v0, float u1, float v1, int tint) {
        MethodHandle $h = Rdock_area_set_panel_icon$;
        if ($h == null) Rdock_area_set_panel_icon$ = $h = find("Rdock_area_set_panel_icon", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_INT));
        try {
            $h.invokeExact(area, panel, texture, u0, v0, u1, v1, tint);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdock_area_set_panel_closable$;

    /**
     * R_FALSE removes the close button of the tab of a panel. Panels can be closed by default.
     */
    public static void Rdock_area_set_panel_closable(int area, int panel, int closable) {
        MethodHandle $h = Rdock_area_set_panel_closable$;
        if ($h == null) Rdock_area_set_panel_closable$ = $h = find("Rdock_area_set_panel_closable", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(area, panel, closable);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdock_area_hide_panel$;

    /**
     * Takes a panel out of the arrangement without deleting it (a
     * window of an application closed from its menu). Its tab disappears
     * and its neighbours take its place; the area remembers where it was.
     */
    public static void Rdock_area_hide_panel(int area, int panel) {
        MethodHandle $h = Rdock_area_hide_panel$;
        if ($h == null) Rdock_area_hide_panel$ = $h = find("Rdock_area_hide_panel", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(area, panel);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdock_area_open_panel$;

    /**
     * Puts a hidden panel back where it was, and brings its tab to the front (also for a panel that is not hidden).
     */
    public static void Rdock_area_open_panel(int area, int panel) {
        MethodHandle $h = Rdock_area_open_panel$;
        if ($h == null) Rdock_area_open_panel$ = $h = find("Rdock_area_open_panel", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(area, panel);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdock_area_is_panel_open$;

    /**
     * Returns: R_TRUE when the panel is in the arrangement (not hidden), shown or behind another tab.
     */
    public static int Rdock_area_is_panel_open(int area, int panel) {
        MethodHandle $h = Rdock_area_is_panel_open$;
        if ($h == null) Rdock_area_is_panel_open$ = $h = find("Rdock_area_is_panel_open", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(area, panel);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdock_area_set_close_hides$;

    /**
     * R_TRUE: the close button of a tab hides its panel
     * (Rdock_area_hide_panel) instead of deleting it. XML: CloseHides="true".
     * The close callback is asked first either way.
     */
    public static void Rdock_area_set_close_hides(int area, int hides) {
        MethodHandle $h = Rdock_area_set_close_hides$;
        if ($h == null) Rdock_area_set_close_hides$ = $h = find("Rdock_area_set_close_hides", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(area, hides);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdock_area_show_panel$;

    /**
     * Brings the tab of a panel to the front.
     */
    public static void Rdock_area_show_panel(int area, int panel) {
        MethodHandle $h = Rdock_area_show_panel$;
        if ($h == null) Rdock_area_show_panel$ = $h = find("Rdock_area_show_panel", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(area, panel);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdock_area_is_panel_shown$;

    /**
     * Returns: R_TRUE when the panel is the one shown in its group of tabs.
     */
    public static int Rdock_area_is_panel_shown(int area, int panel) {
        MethodHandle $h = Rdock_area_is_panel_shown$;
        if ($h == null) Rdock_area_is_panel_shown$ = $h = find("Rdock_area_is_panel_shown", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(area, panel);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdock_area_set_movable$;

    /**
     * R_FALSE stops the user from moving the panels (the borders can still be dragged).
     */
    public static void Rdock_area_set_movable(int area, int movable) {
        MethodHandle $h = Rdock_area_set_movable$;
        if ($h == null) Rdock_area_set_movable$ = $h = find("Rdock_area_set_movable", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(area, movable);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdock_area_save_layout$;

    /**
     * Describes how the panels are arranged, as a short text to keep
     * in a settings file. Panels are named by their widget name
     * (Rwidget_set_name): unnamed panels are not part of the description.
     * Returns: A string valid until the next call.
     */
    public static String Rdock_area_save_layout(int area) {
        MethodHandle $h = Rdock_area_save_layout$;
        if ($h == null) Rdock_area_save_layout$ = $h = find("Rdock_area_save_layout", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(area);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdock_area_load_layout$;

    /**
     * Arranges the panels as a saved description says. Panels of the
     * area that the description does not name keep a place; names it has
     * that match no panel are ignored.
     * Returns: R_FALSE when the text is not a layout (nothing is changed).
     */
    public static int Rdock_area_load_layout(int area, String layout) {
        MethodHandle $h = Rdock_area_load_layout$;
        if ($h == null) Rdock_area_load_layout$ = $h = find("Rdock_area_load_layout", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment layout$ = layout == null ? MemorySegment.NULL : utf8($a, layout);
            int $r = (int) $h.invokeExact(area, layout$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdock_area_set_close_callback$;

    public static void Rdock_area_set_close_callback(int area, RDockCloseCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rdock_area_set_close_callback$;
        if ($h == null) Rdock_area_set_close_callback$ = $h = find("Rdock_area_set_close_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(area, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rdock_area_set_close_callback(int area, RDockCloseCallback callback) {
        Rdock_area_set_close_callback(area, callback, (MemorySegment) null);
    }

    private static MethodHandle Rdock_area_set_tearable$;

    /**
     * Tearing: a tab dragged out of the area takes its panel to a
     * window of its own (a floating area of the same family), and a tab
     * dragged from such a window over the area docks it back where it is
     * dropped. A double click on the tab of a floating window, or closing
     * that window, puts its panels back where they were. Dragging the only
     * tab of a floating window moves the window. R_TRUE by default; XML:
     * Tearable="false". The functions of this area that take a panel also
     * work on its panels that are in floating windows.
     */
    public static void Rdock_area_set_tearable(int area, int tearable) {
        MethodHandle $h = Rdock_area_set_tearable$;
        if ($h == null) Rdock_area_set_tearable$ = $h = find("Rdock_area_set_tearable", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(area, tearable);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdock_area_is_tearable$;

    public static int Rdock_area_is_tearable(int area) {
        MethodHandle $h = Rdock_area_is_tearable$;
        if ($h == null) Rdock_area_is_tearable$ = $h = find("Rdock_area_is_tearable", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(area);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdock_area_float_panel$;

    /**
     * Takes a panel to a window of its own, its top left corner at (x, y) on the screen. Returns the window.
     */
    public static int Rdock_area_float_panel(int area, int panel, int x, int y) {
        MethodHandle $h = Rdock_area_float_panel$;
        if ($h == null) Rdock_area_float_panel$ = $h = find("Rdock_area_float_panel", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(area, panel, x, y);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdock_area_dock_panel$;

    /**
     * A floating panel goes back where it was in the area.
     */
    public static void Rdock_area_dock_panel(int area, int panel) {
        MethodHandle $h = Rdock_area_dock_panel$;
        if ($h == null) Rdock_area_dock_panel$ = $h = find("Rdock_area_dock_panel", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(area, panel);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdock_area_get_panel_window$;

    /**
     * The window a floating panel is in, R_INVALID_ID when it is docked in the area.
     */
    public static int Rdock_area_get_panel_window(int area, int panel) {
        MethodHandle $h = Rdock_area_get_panel_window$;
        if ($h == null) Rdock_area_get_panel_window$ = $h = find("Rdock_area_get_panel_window", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(area, panel);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdock_area_set_changed_callback$;

    public static void Rdock_area_set_changed_callback(int area, RDockChangedCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rdock_area_set_changed_callback$;
        if ($h == null) Rdock_area_set_changed_callback$ = $h = find("Rdock_area_set_changed_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(area, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rdock_area_set_changed_callback(int area, RDockChangedCallback callback) {
        Rdock_area_set_changed_callback(area, callback, (MemorySegment) null);
    }

    private static MethodHandle Risland_create$;

    public static int Risland_create() {
        MethodHandle $h = Risland_create$;
        if ($h == null) Risland_create$ = $h = find("Risland_create", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Risland_set_expanded$;

    /**
     * Unfolds or folds the island. The changed callback is not called.
     */
    public static void Risland_set_expanded(int island, int expanded) {
        MethodHandle $h = Risland_set_expanded$;
        if ($h == null) Risland_set_expanded$ = $h = find("Risland_set_expanded", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(island, expanded);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Risland_is_expanded$;

    public static int Risland_is_expanded(int island) {
        MethodHandle $h = Risland_is_expanded$;
        if ($h == null) Risland_is_expanded$ = $h = find("Risland_is_expanded", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(island);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Risland_expand_for$;

    /**
     * Unfolds the island, and folds it back by itself after a while:
     * to tell something to the user. It stays open while the pointer is on it.
     */
    public static void Risland_expand_for(int island, double seconds) {
        MethodHandle $h = Risland_expand_for$;
        if ($h == null) Risland_expand_for$ = $h = find("Risland_expand_for", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_DOUBLE));
        try {
            $h.invokeExact(island, seconds);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Risland_set_expand_on_hover$;

    /**
     * R_TRUE (default): the island unfolds under the pointer and folds back when it leaves.
     */
    public static void Risland_set_expand_on_hover(int island, int enabled) {
        MethodHandle $h = Risland_set_expand_on_hover$;
        if ($h == null) Risland_set_expand_on_hover$ = $h = find("Risland_set_expand_on_hover", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(island, enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Risland_set_expand_on_click$;

    /**
     * R_TRUE (default): a click on the island unfolds or folds it.
     */
    public static void Risland_set_expand_on_click(int island, int enabled) {
        MethodHandle $h = Risland_set_expand_on_click$;
        if ($h == null) Risland_set_expand_on_click$ = $h = find("Risland_set_expand_on_click", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(island, enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Risland_set_attached$;

    /**
     * R_TRUE gives the island square upper corners: a notch that hangs
     * from the edge it touches, instead of a capsule that floats under it.
     */
    public static void Risland_set_attached(int island, int attached) {
        MethodHandle $h = Risland_set_attached$;
        if ($h == null) Risland_set_attached$ = $h = find("Risland_set_attached", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(island, attached);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Risland_set_notch$;

    /**
     * R_TRUE: on a screen with a camera housing at its top (the notch
     * of a MacBook), the island of a window (Rwindow_set_island) lives in the
     * housing itself, in a window of its own anchored to the screen: it stays
     * there when the window is moved, and takes the mouse and the keyboard as
     * usual. Folded, it is the housing grown into two ears: the first Spacer
     * of its folded content becomes the room of the camera (what is before
     * it shows on the left, what is after on the right). Unfolded, its
     * content starts under the camera.
     *
     * It is in the housing while its window is the last one of the
     * application the user worked in, is shown, and is on that screen; it
     * goes back into its window otherwise, and on the window systems and
     * screens without a housing (where nothing changes). XML: Notch="true".
     */
    public static void Risland_set_notch(int island, int enabled) {
        MethodHandle $h = Risland_set_notch$;
        if ($h == null) Risland_set_notch$ = $h = find("Risland_set_notch", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(island, enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Risland_get_notch$;

    public static int Risland_get_notch(int island) {
        MethodHandle $h = Risland_get_notch$;
        if ($h == null) Risland_get_notch$ = $h = find("Risland_get_notch", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(island);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Risland_is_in_notch$;

    /**
     * R_TRUE while the island is in the camera housing of the screen.
     */
    public static int Risland_is_in_notch(int island) {
        MethodHandle $h = Risland_is_in_notch$;
        if ($h == null) Risland_is_in_notch$ = $h = find("Risland_is_in_notch", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(island);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Risland_set_changed_callback$;

    public static void Risland_set_changed_callback(int island, RIslandChangedCallback callback, MemorySegment user_data) {
        MethodHandle $h = Risland_set_changed_callback$;
        if ($h == null) Risland_set_changed_callback$ = $h = find("Risland_set_changed_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(island, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Risland_set_changed_callback(int island, RIslandChangedCallback callback) {
        Risland_set_changed_callback(island, callback, (MemorySegment) null);
    }

    private static MethodHandle Rwindow_set_island$;

    /**
     * Shows a widget (usually an island) floating at the top of a
     * window, in the middle, above its content. The window takes ownership;
     * the previous one is deleted. R_INVALID_ID removes it. The padding of
     * its slot (Rslot_set_padding) is its distance from the top edge.
     */
    public static void Rwindow_set_island(int window, int widget) {
        MethodHandle $h = Rwindow_set_island$;
        if ($h == null) Rwindow_set_island$ = $h = find("Rwindow_set_island", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(window, widget);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_get_island$;

    public static int Rwindow_get_island(int window) {
        MethodHandle $h = Rwindow_get_island$;
        if ($h == null) Rwindow_get_island$ = $h = find("Rwindow_get_island", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcolor_picker_create$;

    public static int Rcolor_picker_create() {
        MethodHandle $h = Rcolor_picker_create$;
        if ($h == null) Rcolor_picker_create$ = $h = find("Rcolor_picker_create", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcolor_picker_set_color$;

    /**
     * Sets the color (0..1 each). The callback is not called.
     */
    public static void Rcolor_picker_set_color(int picker, float r, float g, float b, float a) {
        MethodHandle $h = Rcolor_picker_set_color$;
        if ($h == null) Rcolor_picker_set_color$ = $h = find("Rcolor_picker_set_color", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(picker, r, g, b, a);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcolor_picker_get_color$;

    public static void Rcolor_picker_get_color(int picker, float[] r, float[] g, float[] b, float[] a) {
        MethodHandle $h = Rcolor_picker_get_color$;
        if ($h == null) Rcolor_picker_get_color$ = $h = find("Rcolor_picker_get_color", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment r$ = r == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment g$ = g == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment b$ = b == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment a$ = a == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            $h.invokeExact(picker, r$, g$, b$, a$);
            if (r != null && r.length > 0) r[0] = r$.get(JAVA_FLOAT, 0);
            if (g != null && g.length > 0) g[0] = g$.get(JAVA_FLOAT, 0);
            if (b != null && b.length > 0) b[0] = b$.get(JAVA_FLOAT, 0);
            if (a != null && a.length > 0) a[0] = a$.get(JAVA_FLOAT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcolor_picker_set_hsv$;

    /**
     * The same color as hue (0..360), saturation and value (0..1).
     * The picker remembers the hue of a gray and the saturation of black,
     * which red, green and blue cannot tell.
     */
    public static void Rcolor_picker_set_hsv(int picker, float hue, float saturation, float value) {
        MethodHandle $h = Rcolor_picker_set_hsv$;
        if ($h == null) Rcolor_picker_set_hsv$ = $h = find("Rcolor_picker_set_hsv", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(picker, hue, saturation, value);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcolor_picker_get_hsv$;

    public static void Rcolor_picker_get_hsv(int picker, float[] hue, float[] saturation, float[] value) {
        MethodHandle $h = Rcolor_picker_get_hsv$;
        if ($h == null) Rcolor_picker_get_hsv$ = $h = find("Rcolor_picker_get_hsv", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment hue$ = hue == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment saturation$ = saturation == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment value$ = value == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            $h.invokeExact(picker, hue$, saturation$, value$);
            if (hue != null && hue.length > 0) hue[0] = hue$.get(JAVA_FLOAT, 0);
            if (saturation != null && saturation.length > 0) saturation[0] = saturation$.get(JAVA_FLOAT, 0);
            if (value != null && value.length > 0) value[0] = value$.get(JAVA_FLOAT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcolor_picker_set_hex$;

    /**
     * Sets the color from text: "#RGB", "#RRGGBB", "#RRGGBBAA" (the
     * '#' may be left out) or one of the color names of the XML attributes.
     * A code without opacity leaves the opacity as it is.
     * Returns: R_FALSE when the text is not a color (the color is unchanged).
     */
    public static int Rcolor_picker_set_hex(int picker, String text) {
        MethodHandle $h = Rcolor_picker_set_hex$;
        if ($h == null) Rcolor_picker_set_hex$ = $h = find("Rcolor_picker_set_hex", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            int $r = (int) $h.invokeExact(picker, text$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcolor_picker_get_hex$;

    /**
     * Returns: "#RRGGBB", or "#RRGGBBAA" when the opacity is shown and not full. Valid until the color changes.
     */
    public static String Rcolor_picker_get_hex(int picker) {
        MethodHandle $h = Rcolor_picker_get_hex$;
        if ($h == null) Rcolor_picker_get_hex$ = $h = find("Rcolor_picker_get_hex", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(picker);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcolor_picker_set_alpha_enabled$;

    /**
     * R_TRUE (default) shows the opacity strip and the fourth number. R_FALSE: colors are opaque.
     */
    public static void Rcolor_picker_set_alpha_enabled(int picker, int enabled) {
        MethodHandle $h = Rcolor_picker_set_alpha_enabled$;
        if ($h == null) Rcolor_picker_set_alpha_enabled$ = $h = find("Rcolor_picker_set_alpha_enabled", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(picker, enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcolor_picker_set_previous_color$;

    /**
     * The color shown next to the current one, to compare with what
     * was there before: clicking it comes back to it. Rcolor_picker_set_color
     * does not change it; a popup picker takes it when it opens.
     */
    public static void Rcolor_picker_set_previous_color(int picker, float r, float g, float b, float a) {
        MethodHandle $h = Rcolor_picker_set_previous_color$;
        if ($h == null) Rcolor_picker_set_previous_color$ = $h = find("Rcolor_picker_set_previous_color", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(picker, r, g, b, a);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcolor_picker_add_swatch$;

    /**
     * Adds a color to the row of swatches: one click takes it.
     */
    public static void Rcolor_picker_add_swatch(int picker, float r, float g, float b, float a) {
        MethodHandle $h = Rcolor_picker_add_swatch$;
        if ($h == null) Rcolor_picker_add_swatch$ = $h = find("Rcolor_picker_add_swatch", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(picker, r, g, b, a);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcolor_picker_clear_swatches$;

    public static void Rcolor_picker_clear_swatches(int picker) {
        MethodHandle $h = Rcolor_picker_clear_swatches$;
        if ($h == null) Rcolor_picker_clear_swatches$ = $h = find("Rcolor_picker_clear_swatches", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(picker);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcolor_picker_get_swatch_count$;

    public static int Rcolor_picker_get_swatch_count(int picker) {
        MethodHandle $h = Rcolor_picker_get_swatch_count$;
        if ($h == null) Rcolor_picker_get_swatch_count$ = $h = find("Rcolor_picker_get_swatch_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(picker);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcolor_picker_set_popup$;

    /**
     * R_TRUE turns the widget into a swatch that opens the picker in a popup.
     */
    public static void Rcolor_picker_set_popup(int picker, int popup) {
        MethodHandle $h = Rcolor_picker_set_popup$;
        if ($h == null) Rcolor_picker_set_popup$ = $h = find("Rcolor_picker_set_popup", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(picker, popup);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcolor_picker_set_compact$;

    /**
     * R_TRUE hides the code and the numbers: only the square, the strips and the swatches.
     */
    public static void Rcolor_picker_set_compact(int picker, int compact) {
        MethodHandle $h = Rcolor_picker_set_compact$;
        if ($h == null) Rcolor_picker_set_compact$ = $h = find("Rcolor_picker_set_compact", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(picker, compact);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcolor_picker_set_changed_callback$;

    public static void Rcolor_picker_set_changed_callback(int picker, RColorChangedCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rcolor_picker_set_changed_callback$;
        if ($h == null) Rcolor_picker_set_changed_callback$ = $h = find("Rcolor_picker_set_changed_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(picker, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rcolor_picker_set_changed_callback(int picker, RColorChangedCallback callback) {
        Rcolor_picker_set_changed_callback(picker, callback, (MemorySegment) null);
    }

    private static MethodHandle Rcolor_rgb_to_hsv$;

    /**
     * Red, green, blue (0..1) to hue (0..360), saturation, value (0..1).
     */
    public static void Rcolor_rgb_to_hsv(float r, float g, float b, float[] hue, float[] saturation, float[] value) {
        MethodHandle $h = Rcolor_rgb_to_hsv$;
        if ($h == null) Rcolor_rgb_to_hsv$ = $h = find("Rcolor_rgb_to_hsv", FunctionDescriptor.ofVoid(JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, ADDRESS, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment hue$ = hue == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment saturation$ = saturation == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment value$ = value == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            $h.invokeExact(r, g, b, hue$, saturation$, value$);
            if (hue != null && hue.length > 0) hue[0] = hue$.get(JAVA_FLOAT, 0);
            if (saturation != null && saturation.length > 0) saturation[0] = saturation$.get(JAVA_FLOAT, 0);
            if (value != null && value.length > 0) value[0] = value$.get(JAVA_FLOAT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcolor_hsv_to_rgb$;

    public static void Rcolor_hsv_to_rgb(float hue, float saturation, float value, float[] r, float[] g, float[] b) {
        MethodHandle $h = Rcolor_hsv_to_rgb$;
        if ($h == null) Rcolor_hsv_to_rgb$ = $h = find("Rcolor_hsv_to_rgb", FunctionDescriptor.ofVoid(JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, ADDRESS, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment r$ = r == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment g$ = g == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment b$ = b == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            $h.invokeExact(hue, saturation, value, r$, g$, b$);
            if (r != null && r.length > 0) r[0] = r$.get(JAVA_FLOAT, 0);
            if (g != null && g.length > 0) g[0] = g$.get(JAVA_FLOAT, 0);
            if (b != null && b.length > 0) b[0] = b$.get(JAVA_FLOAT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_create$;

    public static int Rtimeline_create() {
        MethodHandle $h = Rtimeline_create$;
        if ($h == null) Rtimeline_create$ = $h = find("Rtimeline_create", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_set_duration$;

    /**
     * Length of the sequence. The view and the playhead stay within 0 .. duration.
     */
    public static void Rtimeline_set_duration(int timeline, double seconds) {
        MethodHandle $h = Rtimeline_set_duration$;
        if ($h == null) Rtimeline_set_duration$ = $h = find("Rtimeline_set_duration", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_DOUBLE));
        try {
            $h.invokeExact(timeline, seconds);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_get_duration$;

    public static double Rtimeline_get_duration(int timeline) {
        MethodHandle $h = Rtimeline_get_duration$;
        if ($h == null) Rtimeline_get_duration$ = $h = find("Rtimeline_get_duration", FunctionDescriptor.of(JAVA_DOUBLE, JAVA_INT));
        try {
            double $r = (double) $h.invokeExact(timeline);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_set_time$;

    /**
     * Moves the playhead. The time callback is not called.
     */
    public static void Rtimeline_set_time(int timeline, double seconds) {
        MethodHandle $h = Rtimeline_set_time$;
        if ($h == null) Rtimeline_set_time$ = $h = find("Rtimeline_set_time", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_DOUBLE));
        try {
            $h.invokeExact(timeline, seconds);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_get_time$;

    public static double Rtimeline_get_time(int timeline) {
        MethodHandle $h = Rtimeline_get_time$;
        if ($h == null) Rtimeline_get_time$ = $h = find("Rtimeline_get_time", FunctionDescriptor.of(JAVA_DOUBLE, JAVA_INT));
        try {
            double $r = (double) $h.invokeExact(timeline);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_set_frame_rate$;

    /**
     * Frames per second. With a frame rate the ruler counts in
     * frames ("2:12" is second 2, frame 12) and everything snaps to frames;
     * 0 (default) counts in seconds and snaps to the marks of the ruler.
     */
    public static void Rtimeline_set_frame_rate(int timeline, double frames_per_second) {
        MethodHandle $h = Rtimeline_set_frame_rate$;
        if ($h == null) Rtimeline_set_frame_rate$ = $h = find("Rtimeline_set_frame_rate", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_DOUBLE));
        try {
            $h.invokeExact(timeline, frames_per_second);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_get_frame_rate$;

    public static double Rtimeline_get_frame_rate(int timeline) {
        MethodHandle $h = Rtimeline_get_frame_rate$;
        if ($h == null) Rtimeline_get_frame_rate$ = $h = find("Rtimeline_get_frame_rate", FunctionDescriptor.of(JAVA_DOUBLE, JAVA_INT));
        try {
            double $r = (double) $h.invokeExact(timeline);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_set_snapping$;

    /**
     * R_FALSE lets items be placed anywhere (holding Alt while dragging does it for one move).
     */
    public static void Rtimeline_set_snapping(int timeline, int enabled) {
        MethodHandle $h = Rtimeline_set_snapping$;
        if ($h == null) Rtimeline_set_snapping$ = $h = find("Rtimeline_set_snapping", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(timeline, enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_set_view$;

    /**
     * The part of the sequence that is shown. Zooming is showing less.
     */
    public static void Rtimeline_set_view(int timeline, double start, double end) {
        MethodHandle $h = Rtimeline_set_view$;
        if ($h == null) Rtimeline_set_view$ = $h = find("Rtimeline_set_view", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_DOUBLE, JAVA_DOUBLE));
        try {
            $h.invokeExact(timeline, start, end);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_get_view$;

    public static void Rtimeline_get_view(int timeline, double[] start, double[] end) {
        MethodHandle $h = Rtimeline_get_view$;
        if ($h == null) Rtimeline_get_view$ = $h = find("Rtimeline_get_view", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment start$ = start == null ? MemorySegment.NULL : $a.allocate(JAVA_DOUBLE);
            MemorySegment end$ = end == null ? MemorySegment.NULL : $a.allocate(JAVA_DOUBLE);
            $h.invokeExact(timeline, start$, end$);
            if (start != null && start.length > 0) start[0] = start$.get(JAVA_DOUBLE, 0);
            if (end != null && end.length > 0) end[0] = end$.get(JAVA_DOUBLE, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_set_playing$;

    /**
     * Makes the playhead run by itself, at the speed of the clock,
     * calling the time callback at each frame; at the end it stops, or
     * starts again with Rtimeline_set_loop.
     */
    public static void Rtimeline_set_playing(int timeline, int playing) {
        MethodHandle $h = Rtimeline_set_playing$;
        if ($h == null) Rtimeline_set_playing$ = $h = find("Rtimeline_set_playing", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(timeline, playing);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_is_playing$;

    public static int Rtimeline_is_playing(int timeline) {
        MethodHandle $h = Rtimeline_is_playing$;
        if ($h == null) Rtimeline_is_playing$ = $h = find("Rtimeline_is_playing", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(timeline);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_set_loop$;

    public static void Rtimeline_set_loop(int timeline, int loop) {
        MethodHandle $h = Rtimeline_set_loop$;
        if ($h == null) Rtimeline_set_loop$ = $h = find("Rtimeline_set_loop", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(timeline, loop);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_set_header_width$;

    /**
     * Width of the column of the names of the tracks. 0 (default): fits the longest name.
     */
    public static void Rtimeline_set_header_width(int timeline, float width) {
        MethodHandle $h = Rtimeline_set_header_width$;
        if ($h == null) Rtimeline_set_header_width$ = $h = find("Rtimeline_set_header_width", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(timeline, width);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_get_header_width$;

    /**
     * Returns: The width of the column of the names, as laid out now.
     */
    public static float Rtimeline_get_header_width(int timeline) {
        MethodHandle $h = Rtimeline_get_header_width$;
        if ($h == null) Rtimeline_get_header_width$ = $h = find("Rtimeline_get_header_width", FunctionDescriptor.of(JAVA_FLOAT, JAVA_INT));
        try {
            float $r = (float) $h.invokeExact(timeline);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_set_ruler_height$;

    /**
     * Height of the ruler of time (and of the markers' flags band). 0 (default): automatic.
     */
    public static void Rtimeline_set_ruler_height(int timeline, float height) {
        MethodHandle $h = Rtimeline_set_ruler_height$;
        if ($h == null) Rtimeline_set_ruler_height$ = $h = find("Rtimeline_set_ruler_height", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(timeline, height);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_get_ruler_height$;

    public static float Rtimeline_get_ruler_height(int timeline) {
        MethodHandle $h = Rtimeline_get_ruler_height$;
        if ($h == null) Rtimeline_get_ruler_height$ = $h = find("Rtimeline_get_ruler_height", FunctionDescriptor.of(JAVA_FLOAT, JAVA_INT));
        try {
            float $r = (float) $h.invokeExact(timeline);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_set_default_track_height$;

    /**
     * Height of the tracks that have none of their own (Rtimeline_set_track_height). 0 (default): automatic.
     */
    public static void Rtimeline_set_default_track_height(int timeline, float height) {
        MethodHandle $h = Rtimeline_set_default_track_height$;
        if ($h == null) Rtimeline_set_default_track_height$ = $h = find("Rtimeline_set_default_track_height", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(timeline, height);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_get_default_track_height$;

    public static float Rtimeline_get_default_track_height(int timeline) {
        MethodHandle $h = Rtimeline_get_default_track_height$;
        if ($h == null) Rtimeline_get_default_track_height$ = $h = find("Rtimeline_get_default_track_height", FunctionDescriptor.of(JAVA_FLOAT, JAVA_INT));
        try {
            float $r = (float) $h.invokeExact(timeline);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_add_track$;

    /**
     * Returns: The track, or 0. Tracks are shown in the order they were added.
     */
    public static int Rtimeline_add_track(int timeline, String name) {
        MethodHandle $h = Rtimeline_add_track$;
        if ($h == null) Rtimeline_add_track$ = $h = find("Rtimeline_add_track", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            int $r = (int) $h.invokeExact(timeline, name$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_remove_track$;

    /**
     * Removes a track and everything on it.
     */
    public static void Rtimeline_remove_track(int timeline, int track) {
        MethodHandle $h = Rtimeline_remove_track$;
        if ($h == null) Rtimeline_remove_track$ = $h = find("Rtimeline_remove_track", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(timeline, track);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_get_track_count$;

    public static int Rtimeline_get_track_count(int timeline) {
        MethodHandle $h = Rtimeline_get_track_count$;
        if ($h == null) Rtimeline_get_track_count$ = $h = find("Rtimeline_get_track_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(timeline);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_get_track$;

    /**
     * Returns: The index-th track from the top, or 0.
     */
    public static int Rtimeline_get_track(int timeline, int index) {
        MethodHandle $h = Rtimeline_get_track$;
        if ($h == null) Rtimeline_get_track$ = $h = find("Rtimeline_get_track", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(timeline, index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_set_track_name$;

    public static void Rtimeline_set_track_name(int timeline, int track, String name) {
        MethodHandle $h = Rtimeline_set_track_name$;
        if ($h == null) Rtimeline_set_track_name$ = $h = find("Rtimeline_set_track_name", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            $h.invokeExact(timeline, track, name$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_get_track_name$;

    public static String Rtimeline_get_track_name(int timeline, int track) {
        MethodHandle $h = Rtimeline_get_track_name$;
        if ($h == null) Rtimeline_get_track_name$ = $h = find("Rtimeline_get_track_name", FunctionDescriptor.of(ADDRESS, JAVA_INT, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(timeline, track);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_set_track_color$;

    /**
     * Color of the clips and keys of the track that have none of their own.
     */
    public static void Rtimeline_set_track_color(int timeline, int track, float r, float g, float b) {
        MethodHandle $h = Rtimeline_set_track_color$;
        if ($h == null) Rtimeline_set_track_color$ = $h = find("Rtimeline_set_track_color", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(timeline, track, r, g, b);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_set_track_height$;

    /**
     * Height of the track in logical units; 0 = the default.
     */
    public static void Rtimeline_set_track_height(int timeline, int track, float height) {
        MethodHandle $h = Rtimeline_set_track_height$;
        if ($h == null) Rtimeline_set_track_height$ = $h = find("Rtimeline_set_track_height", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(timeline, track, height);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_set_track_locked$;

    /**
     * A locked track shows its items but does not let the user change them.
     */
    public static void Rtimeline_set_track_locked(int timeline, int track, int locked) {
        MethodHandle $h = Rtimeline_set_track_locked$;
        if ($h == null) Rtimeline_set_track_locked$ = $h = find("Rtimeline_set_track_locked", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(timeline, track, locked);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_set_track_muted$;

    /**
     * A muted track is drawn dimmed; what muting means is up to the application.
     */
    public static void Rtimeline_set_track_muted(int timeline, int track, int muted) {
        MethodHandle $h = Rtimeline_set_track_muted$;
        if ($h == null) Rtimeline_set_track_muted$ = $h = find("Rtimeline_set_track_muted", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(timeline, track, muted);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_is_track_muted$;

    public static int Rtimeline_is_track_muted(int timeline, int track) {
        MethodHandle $h = Rtimeline_is_track_muted$;
        if ($h == null) Rtimeline_is_track_muted$ = $h = find("Rtimeline_is_track_muted", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(timeline, track);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_add_clip$;

    /**
     * Returns: The clip, or 0 when the track does not exist.
     */
    public static int Rtimeline_add_clip(int timeline, int track, double start, double length, String label) {
        MethodHandle $h = Rtimeline_add_clip$;
        if ($h == null) Rtimeline_add_clip$ = $h = find("Rtimeline_add_clip", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_DOUBLE, JAVA_DOUBLE, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment label$ = label == null ? MemorySegment.NULL : utf8($a, label);
            int $r = (int) $h.invokeExact(timeline, track, start, length, label$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_add_key$;

    /**
     * Returns: The key, or 0.
     */
    public static int Rtimeline_add_key(int timeline, int track, double time) {
        MethodHandle $h = Rtimeline_add_key$;
        if ($h == null) Rtimeline_add_key$ = $h = find("Rtimeline_add_key", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_DOUBLE));
        try {
            int $r = (int) $h.invokeExact(timeline, track, time);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_add_marker$;

    /**
     * A named moment of the whole sequence, shown on the ruler and across the tracks.
     */
    public static int Rtimeline_add_marker(int timeline, double time, String label) {
        MethodHandle $h = Rtimeline_add_marker$;
        if ($h == null) Rtimeline_add_marker$ = $h = find("Rtimeline_add_marker", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_DOUBLE, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment label$ = label == null ? MemorySegment.NULL : utf8($a, label);
            int $r = (int) $h.invokeExact(timeline, time, label$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_remove_item$;

    public static void Rtimeline_remove_item(int timeline, int item) {
        MethodHandle $h = Rtimeline_remove_item$;
        if ($h == null) Rtimeline_remove_item$ = $h = find("Rtimeline_remove_item", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(timeline, item);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_clear$;

    /**
     * Removes every track, clip, key and marker.
     */
    public static void Rtimeline_clear(int timeline) {
        MethodHandle $h = Rtimeline_clear$;
        if ($h == null) Rtimeline_clear$ = $h = find("Rtimeline_clear", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(timeline);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_get_item_type$;

    public static int Rtimeline_get_item_type(int timeline, int item) {
        MethodHandle $h = Rtimeline_get_item_type$;
        if ($h == null) Rtimeline_get_item_type$ = $h = find("Rtimeline_get_item_type", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(timeline, item);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_get_item_track$;

    /**
     * Returns: The track of a clip or of a key; 0 for a marker.
     */
    public static int Rtimeline_get_item_track(int timeline, int item) {
        MethodHandle $h = Rtimeline_get_item_track$;
        if ($h == null) Rtimeline_get_item_track$ = $h = find("Rtimeline_get_item_track", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(timeline, item);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_get_item_start$;

    public static double Rtimeline_get_item_start(int timeline, int item) {
        MethodHandle $h = Rtimeline_get_item_start$;
        if ($h == null) Rtimeline_get_item_start$ = $h = find("Rtimeline_get_item_start", FunctionDescriptor.of(JAVA_DOUBLE, JAVA_INT, JAVA_INT));
        try {
            double $r = (double) $h.invokeExact(timeline, item);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_get_item_length$;

    /**
     * Returns: The length of a clip; 0 for a key or a marker.
     */
    public static double Rtimeline_get_item_length(int timeline, int item) {
        MethodHandle $h = Rtimeline_get_item_length$;
        if ($h == null) Rtimeline_get_item_length$ = $h = find("Rtimeline_get_item_length", FunctionDescriptor.of(JAVA_DOUBLE, JAVA_INT, JAVA_INT));
        try {
            double $r = (double) $h.invokeExact(timeline, item);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_set_item$;

    /**
     * Moves an item: to another track (0 = keep), another start, another length (clips; negative = keep).
     */
    public static void Rtimeline_set_item(int timeline, int item, int track, double start, double length) {
        MethodHandle $h = Rtimeline_set_item$;
        if ($h == null) Rtimeline_set_item$ = $h = find("Rtimeline_set_item", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_DOUBLE, JAVA_DOUBLE));
        try {
            $h.invokeExact(timeline, item, track, start, length);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_set_item_label$;

    public static void Rtimeline_set_item_label(int timeline, int item, String label) {
        MethodHandle $h = Rtimeline_set_item_label$;
        if ($h == null) Rtimeline_set_item_label$ = $h = find("Rtimeline_set_item_label", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment label$ = label == null ? MemorySegment.NULL : utf8($a, label);
            $h.invokeExact(timeline, item, label$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_get_item_label$;

    public static String Rtimeline_get_item_label(int timeline, int item) {
        MethodHandle $h = Rtimeline_get_item_label$;
        if ($h == null) Rtimeline_get_item_label$ = $h = find("Rtimeline_get_item_label", FunctionDescriptor.of(ADDRESS, JAVA_INT, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(timeline, item);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_set_item_color$;

    public static void Rtimeline_set_item_color(int timeline, int item, float r, float g, float b) {
        MethodHandle $h = Rtimeline_set_item_color$;
        if ($h == null) Rtimeline_set_item_color$ = $h = find("Rtimeline_set_item_color", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(timeline, item, r, g, b);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_get_item_count$;

    public static int Rtimeline_get_item_count(int timeline) {
        MethodHandle $h = Rtimeline_get_item_count$;
        if ($h == null) Rtimeline_get_item_count$ = $h = find("Rtimeline_get_item_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(timeline);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_get_item$;

    /**
     * Returns: The index-th item, in the order they were added, or 0.
     */
    public static int Rtimeline_get_item(int timeline, int index) {
        MethodHandle $h = Rtimeline_get_item$;
        if ($h == null) Rtimeline_get_item$ = $h = find("Rtimeline_get_item", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(timeline, index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_select$;

    /**
     * Selects an item, alone or (add) with the ones already selected.
     */
    public static void Rtimeline_select(int timeline, int item, int add) {
        MethodHandle $h = Rtimeline_select$;
        if ($h == null) Rtimeline_select$ = $h = find("Rtimeline_select", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(timeline, item, add);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_clear_selection$;

    public static void Rtimeline_clear_selection(int timeline) {
        MethodHandle $h = Rtimeline_clear_selection$;
        if ($h == null) Rtimeline_clear_selection$ = $h = find("Rtimeline_clear_selection", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(timeline);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_is_selected$;

    public static int Rtimeline_is_selected(int timeline, int item) {
        MethodHandle $h = Rtimeline_is_selected$;
        if ($h == null) Rtimeline_is_selected$ = $h = find("Rtimeline_is_selected", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(timeline, item);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_get_selection_count$;

    public static int Rtimeline_get_selection_count(int timeline) {
        MethodHandle $h = Rtimeline_get_selection_count$;
        if ($h == null) Rtimeline_get_selection_count$ = $h = find("Rtimeline_get_selection_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(timeline);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_get_selected$;

    public static int Rtimeline_get_selected(int timeline, int index) {
        MethodHandle $h = Rtimeline_get_selected$;
        if ($h == null) Rtimeline_get_selected$ = $h = find("Rtimeline_get_selected", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(timeline, index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtimeline_set_time_callback$;

    public static void Rtimeline_set_time_callback(int timeline, RTimelineTimeCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rtimeline_set_time_callback$;
        if ($h == null) Rtimeline_set_time_callback$ = $h = find("Rtimeline_set_time_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(timeline, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rtimeline_set_time_callback(int timeline, RTimelineTimeCallback callback) {
        Rtimeline_set_time_callback(timeline, callback, (MemorySegment) null);
    }

    private static MethodHandle Rtimeline_set_changed_callback$;

    /**
     * Called for each item the user moved, stretched or retimed, as
     * it happens. The action of the widget (OnChanged in XML) fires once
     * per gesture, when the button is released.
     */
    public static void Rtimeline_set_changed_callback(int timeline, RTimelineItemCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rtimeline_set_changed_callback$;
        if ($h == null) Rtimeline_set_changed_callback$ = $h = find("Rtimeline_set_changed_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(timeline, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Called for each item the user moved, stretched or retimed, as
     * it happens. The action of the widget (OnChanged in XML) fires once
     * per gesture, when the button is released.
     */
    public static void Rtimeline_set_changed_callback(int timeline, RTimelineItemCallback callback) {
        Rtimeline_set_changed_callback(timeline, callback, (MemorySegment) null);
    }

    private static MethodHandle Rtimeline_set_selection_callback$;

    /**
     * Called when the selection changed; `item` is the item just clicked, or 0.
     */
    public static void Rtimeline_set_selection_callback(int timeline, RTimelineItemCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rtimeline_set_selection_callback$;
        if ($h == null) Rtimeline_set_selection_callback$ = $h = find("Rtimeline_set_selection_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(timeline, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Called when the selection changed; `item` is the item just clicked, or 0.
     */
    public static void Rtimeline_set_selection_callback(int timeline, RTimelineItemCallback callback) {
        Rtimeline_set_selection_callback(timeline, callback, (MemorySegment) null);
    }

    private static MethodHandle Rtimeline_set_activated_callback$;

    /**
     * Called when an item is double-clicked.
     */
    public static void Rtimeline_set_activated_callback(int timeline, RTimelineItemCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rtimeline_set_activated_callback$;
        if ($h == null) Rtimeline_set_activated_callback$ = $h = find("Rtimeline_set_activated_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(timeline, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Called when an item is double-clicked.
     */
    public static void Rtimeline_set_activated_callback(int timeline, RTimelineItemCallback callback) {
        Rtimeline_set_activated_callback(timeline, callback, (MemorySegment) null);
    }

    private static MethodHandle Rtimeline_set_removed_callback$;

    /**
     * Called when the user deletes an item (the Delete key), before
     * it is removed: its values can still be read. Without this callback the
     * Delete key does nothing: the application decides what can be deleted.
     */
    public static void Rtimeline_set_removed_callback(int timeline, RTimelineItemCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rtimeline_set_removed_callback$;
        if ($h == null) Rtimeline_set_removed_callback$ = $h = find("Rtimeline_set_removed_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(timeline, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Called when the user deletes an item (the Delete key), before
     * it is removed: its values can still be read. Without this callback the
     * Delete key does nothing: the application decides what can be deleted.
     */
    public static void Rtimeline_set_removed_callback(int timeline, RTimelineItemCallback callback) {
        Rtimeline_set_removed_callback(timeline, callback, (MemorySegment) null);
    }

    private static MethodHandle Rnode_graph_create$;

    public static int Rnode_graph_create() {
        MethodHandle $h = Rnode_graph_create$;
        if ($h == null) Rnode_graph_create$ = $h = find("Rnode_graph_create", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_add_node$;

    /**
     * x,y Top-left corner of the node, in the units of the canvas.
     */
    public static int Rnode_graph_add_node(int graph, String title, float x, float y) {
        MethodHandle $h = Rnode_graph_add_node$;
        if ($h == null) Rnode_graph_add_node$ = $h = find("Rnode_graph_add_node", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS, JAVA_FLOAT, JAVA_FLOAT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment title$ = title == null ? MemorySegment.NULL : utf8($a, title);
            int $r = (int) $h.invokeExact(graph, title$, x, y);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_remove_node$;

    /**
     * Removes a node, its pins, their links, and the widget it holds.
     */
    public static void Rnode_graph_remove_node(int graph, int node) {
        MethodHandle $h = Rnode_graph_remove_node$;
        if ($h == null) Rnode_graph_remove_node$ = $h = find("Rnode_graph_remove_node", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(graph, node);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_clear$;

    public static void Rnode_graph_clear(int graph) {
        MethodHandle $h = Rnode_graph_clear$;
        if ($h == null) Rnode_graph_clear$ = $h = find("Rnode_graph_clear", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(graph);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_get_node_count$;

    public static int Rnode_graph_get_node_count(int graph) {
        MethodHandle $h = Rnode_graph_get_node_count$;
        if ($h == null) Rnode_graph_get_node_count$ = $h = find("Rnode_graph_get_node_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(graph);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_get_node$;

    public static int Rnode_graph_get_node(int graph, int index) {
        MethodHandle $h = Rnode_graph_get_node$;
        if ($h == null) Rnode_graph_get_node$ = $h = find("Rnode_graph_get_node", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(graph, index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_set_node_title$;

    public static void Rnode_graph_set_node_title(int graph, int node, String title) {
        MethodHandle $h = Rnode_graph_set_node_title$;
        if ($h == null) Rnode_graph_set_node_title$ = $h = find("Rnode_graph_set_node_title", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment title$ = title == null ? MemorySegment.NULL : utf8($a, title);
            $h.invokeExact(graph, node, title$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_get_node_title$;

    public static String Rnode_graph_get_node_title(int graph, int node) {
        MethodHandle $h = Rnode_graph_get_node_title$;
        if ($h == null) Rnode_graph_get_node_title$ = $h = find("Rnode_graph_get_node_title", FunctionDescriptor.of(ADDRESS, JAVA_INT, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(graph, node);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_set_node_position$;

    public static void Rnode_graph_set_node_position(int graph, int node, float x, float y) {
        MethodHandle $h = Rnode_graph_set_node_position$;
        if ($h == null) Rnode_graph_set_node_position$ = $h = find("Rnode_graph_set_node_position", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(graph, node, x, y);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_get_node_position$;

    public static void Rnode_graph_get_node_position(int graph, int node, float[] x, float[] y) {
        MethodHandle $h = Rnode_graph_get_node_position$;
        if ($h == null) Rnode_graph_get_node_position$ = $h = find("Rnode_graph_get_node_position", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment x$ = x == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment y$ = y == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            $h.invokeExact(graph, node, x$, y$);
            if (x != null && x.length > 0) x[0] = x$.get(JAVA_FLOAT, 0);
            if (y != null && y.length > 0) y[0] = y$.get(JAVA_FLOAT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_get_node_size$;

    /**
     * Size of a node on the canvas: Raven works it out from its title, its pins and its content.
     */
    public static void Rnode_graph_get_node_size(int graph, int node, float[] width, float[] height) {
        MethodHandle $h = Rnode_graph_get_node_size$;
        if ($h == null) Rnode_graph_get_node_size$ = $h = find("Rnode_graph_get_node_size", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment width$ = width == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment height$ = height == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            $h.invokeExact(graph, node, width$, height$);
            if (width != null && width.length > 0) width[0] = width$.get(JAVA_FLOAT, 0);
            if (height != null && height.length > 0) height[0] = height$.get(JAVA_FLOAT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_set_node_color$;

    /**
     * Color of the header of the node; without one it takes the accent color of the theme.
     */
    public static void Rnode_graph_set_node_color(int graph, int node, float r, float g, float b) {
        MethodHandle $h = Rnode_graph_set_node_color$;
        if ($h == null) Rnode_graph_set_node_color$ = $h = find("Rnode_graph_set_node_color", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(graph, node, r, g, b);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_set_node_content$;

    /**
     * Puts a widget inside the node, under its pins: the graph owns
     * it from now on. R_INVALID_ID removes (and deletes) the one it had.
     */
    public static void Rnode_graph_set_node_content(int graph, int node, int widget) {
        MethodHandle $h = Rnode_graph_set_node_content$;
        if ($h == null) Rnode_graph_set_node_content$ = $h = find("Rnode_graph_set_node_content", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(graph, node, widget);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_get_node_content$;

    public static int Rnode_graph_get_node_content(int graph, int node) {
        MethodHandle $h = Rnode_graph_get_node_content$;
        if ($h == null) Rnode_graph_get_node_content$ = $h = find("Rnode_graph_get_node_content", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(graph, node);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_set_node_name$;

    /**
     * Name of the node, the `Name` of its XML element: what
     * &lt;Link From="name.pin"&gt; and Rnode_graph_find_node look for. Not shown.
     */
    public static void Rnode_graph_set_node_name(int graph, int node, String name) {
        MethodHandle $h = Rnode_graph_set_node_name$;
        if ($h == null) Rnode_graph_set_node_name$ = $h = find("Rnode_graph_set_node_name", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            $h.invokeExact(graph, node, name$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_get_node_name$;

    public static String Rnode_graph_get_node_name(int graph, int node) {
        MethodHandle $h = Rnode_graph_get_node_name$;
        if ($h == null) Rnode_graph_get_node_name$ = $h = find("Rnode_graph_get_node_name", FunctionDescriptor.of(ADDRESS, JAVA_INT, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(graph, node);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_find_node$;

    /**
     * Returns: The node of that name, or 0.
     */
    public static int Rnode_graph_find_node(int graph, String name) {
        MethodHandle $h = Rnode_graph_find_node$;
        if ($h == null) Rnode_graph_find_node$ = $h = find("Rnode_graph_find_node", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            int $r = (int) $h.invokeExact(graph, name$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_set_node_user_data$;

    /**
     * Value the application attaches to a node.
     */
    public static void Rnode_graph_set_node_user_data(int graph, int node, MemorySegment user_data) {
        MethodHandle $h = Rnode_graph_set_node_user_data$;
        if ($h == null) Rnode_graph_set_node_user_data$ = $h = find("Rnode_graph_set_node_user_data", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
        try {
            $h.invokeExact(graph, node, (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Value the application attaches to a node.
     */
    public static void Rnode_graph_set_node_user_data(int graph, int node) {
        Rnode_graph_set_node_user_data(graph, node, (MemorySegment) null);
    }

    private static MethodHandle Rnode_graph_get_node_user_data$;

    public static MemorySegment Rnode_graph_get_node_user_data(int graph, int node) {
        MethodHandle $h = Rnode_graph_get_node_user_data$;
        if ($h == null) Rnode_graph_get_node_user_data$ = $h = find("Rnode_graph_get_node_user_data", FunctionDescriptor.of(ADDRESS, JAVA_INT, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(graph, node);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_add_input$;

    /**
     * type Free text, or NULL for a pin that links to any type. Returns: The pin, or 0.
     */
    public static int Rnode_graph_add_input(int graph, int node, String label, String type) {
        MethodHandle $h = Rnode_graph_add_input$;
        if ($h == null) Rnode_graph_add_input$ = $h = find("Rnode_graph_add_input", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment label$ = label == null ? MemorySegment.NULL : utf8($a, label);
            MemorySegment type$ = type == null ? MemorySegment.NULL : utf8($a, type);
            int $r = (int) $h.invokeExact(graph, node, label$, type$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_add_output$;

    public static int Rnode_graph_add_output(int graph, int node, String label, String type) {
        MethodHandle $h = Rnode_graph_add_output$;
        if ($h == null) Rnode_graph_add_output$ = $h = find("Rnode_graph_add_output", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment label$ = label == null ? MemorySegment.NULL : utf8($a, label);
            MemorySegment type$ = type == null ? MemorySegment.NULL : utf8($a, type);
            int $r = (int) $h.invokeExact(graph, node, label$, type$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_remove_pin$;

    public static void Rnode_graph_remove_pin(int graph, int pin) {
        MethodHandle $h = Rnode_graph_remove_pin$;
        if ($h == null) Rnode_graph_remove_pin$ = $h = find("Rnode_graph_remove_pin", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(graph, pin);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_get_pin_node$;

    public static int Rnode_graph_get_pin_node(int graph, int pin) {
        MethodHandle $h = Rnode_graph_get_pin_node$;
        if ($h == null) Rnode_graph_get_pin_node$ = $h = find("Rnode_graph_get_pin_node", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(graph, pin);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_is_pin_output$;

    public static int Rnode_graph_is_pin_output(int graph, int pin) {
        MethodHandle $h = Rnode_graph_is_pin_output$;
        if ($h == null) Rnode_graph_is_pin_output$ = $h = find("Rnode_graph_is_pin_output", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(graph, pin);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_get_pin_label$;

    public static String Rnode_graph_get_pin_label(int graph, int pin) {
        MethodHandle $h = Rnode_graph_get_pin_label$;
        if ($h == null) Rnode_graph_get_pin_label$ = $h = find("Rnode_graph_get_pin_label", FunctionDescriptor.of(ADDRESS, JAVA_INT, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(graph, pin);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_get_pin_type$;

    public static String Rnode_graph_get_pin_type(int graph, int pin) {
        MethodHandle $h = Rnode_graph_get_pin_type$;
        if ($h == null) Rnode_graph_get_pin_type$ = $h = find("Rnode_graph_get_pin_type", FunctionDescriptor.of(ADDRESS, JAVA_INT, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(graph, pin);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_set_pin_name$;

    /**
     * Name of the pin for Rnode_graph_find_pin and for the links of XML; a pin without one answers to its label.
     */
    public static void Rnode_graph_set_pin_name(int graph, int pin, String name) {
        MethodHandle $h = Rnode_graph_set_pin_name$;
        if ($h == null) Rnode_graph_set_pin_name$ = $h = find("Rnode_graph_set_pin_name", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            $h.invokeExact(graph, pin, name$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_find_pin$;

    /**
     * Returns: The pin of a node with that name (or label), or 0.
     */
    public static int Rnode_graph_find_pin(int graph, int node, String name) {
        MethodHandle $h = Rnode_graph_find_pin$;
        if ($h == null) Rnode_graph_find_pin$ = $h = find("Rnode_graph_find_pin", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            int $r = (int) $h.invokeExact(graph, node, name$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_get_pin_position$;

    /**
     * Where a pin is on the canvas (the middle of its dot).
     */
    public static void Rnode_graph_get_pin_position(int graph, int pin, float[] x, float[] y) {
        MethodHandle $h = Rnode_graph_get_pin_position$;
        if ($h == null) Rnode_graph_get_pin_position$ = $h = find("Rnode_graph_get_pin_position", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment x$ = x == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment y$ = y == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            $h.invokeExact(graph, pin, x$, y$);
            if (x != null && x.length > 0) x[0] = x$.get(JAVA_FLOAT, 0);
            if (y != null && y.length > 0) y[0] = y$.get(JAVA_FLOAT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_get_input$;

    /**
     * Returns: The index-th input (or output) of a node, or 0.
     */
    public static int Rnode_graph_get_input(int graph, int node, int index) {
        MethodHandle $h = Rnode_graph_get_input$;
        if ($h == null) Rnode_graph_get_input$ = $h = find("Rnode_graph_get_input", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(graph, node, index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_get_output$;

    public static int Rnode_graph_get_output(int graph, int node, int index) {
        MethodHandle $h = Rnode_graph_get_output$;
        if ($h == null) Rnode_graph_get_output$ = $h = find("Rnode_graph_get_output", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(graph, node, index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_set_type_color$;

    /**
     * Color of the pins of a type and of the links between them.
     */
    public static void Rnode_graph_set_type_color(int graph, String type, float r, float g, float b) {
        MethodHandle $h = Rnode_graph_set_type_color$;
        if ($h == null) Rnode_graph_set_type_color$ = $h = find("Rnode_graph_set_type_color", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment type$ = type == null ? MemorySegment.NULL : utf8($a, type);
            $h.invokeExact(graph, type$, r, g, b);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_connect$;

    /**
     * Links an output to an input, as the user does by dragging.
     * The connect callback is not asked.
     * Returns: The link; 0 when the pins cannot be linked (two inputs, the
     * same node, different types).
     */
    public static int Rnode_graph_connect(int graph, int output_pin, int input_pin) {
        MethodHandle $h = Rnode_graph_connect$;
        if ($h == null) Rnode_graph_connect$ = $h = find("Rnode_graph_connect", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(graph, output_pin, input_pin);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_disconnect$;

    public static void Rnode_graph_disconnect(int graph, int link) {
        MethodHandle $h = Rnode_graph_disconnect$;
        if ($h == null) Rnode_graph_disconnect$ = $h = find("Rnode_graph_disconnect", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(graph, link);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_get_link_count$;

    public static int Rnode_graph_get_link_count(int graph) {
        MethodHandle $h = Rnode_graph_get_link_count$;
        if ($h == null) Rnode_graph_get_link_count$ = $h = find("Rnode_graph_get_link_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(graph);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_get_link$;

    public static int Rnode_graph_get_link(int graph, int index) {
        MethodHandle $h = Rnode_graph_get_link$;
        if ($h == null) Rnode_graph_get_link$ = $h = find("Rnode_graph_get_link", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(graph, index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_get_link_output$;

    public static int Rnode_graph_get_link_output(int graph, int link) {
        MethodHandle $h = Rnode_graph_get_link_output$;
        if ($h == null) Rnode_graph_get_link_output$ = $h = find("Rnode_graph_get_link_output", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(graph, link);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_get_link_input$;

    public static int Rnode_graph_get_link_input(int graph, int link) {
        MethodHandle $h = Rnode_graph_get_link_input$;
        if ($h == null) Rnode_graph_get_link_input$ = $h = find("Rnode_graph_get_link_input", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(graph, link);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_get_input_link$;

    /**
     * Returns: The link that arrives at an input pin, or 0.
     */
    public static int Rnode_graph_get_input_link(int graph, int input_pin) {
        MethodHandle $h = Rnode_graph_get_input_link$;
        if ($h == null) Rnode_graph_get_input_link$ = $h = find("Rnode_graph_get_input_link", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(graph, input_pin);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_set_view$;

    /**
     * What the middle of the widget shows: a point of the canvas, and how large (1 = one unit per logical unit).
     */
    public static void Rnode_graph_set_view(int graph, float center_x, float center_y, float zoom) {
        MethodHandle $h = Rnode_graph_set_view$;
        if ($h == null) Rnode_graph_set_view$ = $h = find("Rnode_graph_set_view", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(graph, center_x, center_y, zoom);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_get_view$;

    public static void Rnode_graph_get_view(int graph, float[] center_x, float[] center_y, float[] zoom) {
        MethodHandle $h = Rnode_graph_get_view$;
        if ($h == null) Rnode_graph_get_view$ = $h = find("Rnode_graph_get_view", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment center_x$ = center_x == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment center_y$ = center_y == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment zoom$ = zoom == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            $h.invokeExact(graph, center_x$, center_y$, zoom$);
            if (center_x != null && center_x.length > 0) center_x[0] = center_x$.get(JAVA_FLOAT, 0);
            if (center_y != null && center_y.length > 0) center_y[0] = center_y$.get(JAVA_FLOAT, 0);
            if (zoom != null && zoom.length > 0) zoom[0] = zoom$.get(JAVA_FLOAT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_to_canvas$;

    /**
     * Converts a point of the widget (from its corner, as in the events) to the canvas, and back, with the view as it is shown now.
     */
    public static void Rnode_graph_to_canvas(int graph, float x, float y, float[] canvas_x, float[] canvas_y) {
        MethodHandle $h = Rnode_graph_to_canvas$;
        if ($h == null) Rnode_graph_to_canvas$ = $h = find("Rnode_graph_to_canvas", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment canvas_x$ = canvas_x == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment canvas_y$ = canvas_y == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            $h.invokeExact(graph, x, y, canvas_x$, canvas_y$);
            if (canvas_x != null && canvas_x.length > 0) canvas_x[0] = canvas_x$.get(JAVA_FLOAT, 0);
            if (canvas_y != null && canvas_y.length > 0) canvas_y[0] = canvas_y$.get(JAVA_FLOAT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_from_canvas$;

    public static void Rnode_graph_from_canvas(int graph, float canvas_x, float canvas_y, float[] x, float[] y) {
        MethodHandle $h = Rnode_graph_from_canvas$;
        if ($h == null) Rnode_graph_from_canvas$ = $h = find("Rnode_graph_from_canvas", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment x$ = x == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment y$ = y == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            $h.invokeExact(graph, canvas_x, canvas_y, x$, y$);
            if (x != null && x.length > 0) x[0] = x$.get(JAVA_FLOAT, 0);
            if (y != null && y.length > 0) y[0] = y$.get(JAVA_FLOAT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_frame_all$;

    /**
     * Moves and zooms so that every node shows.
     */
    public static void Rnode_graph_frame_all(int graph) {
        MethodHandle $h = Rnode_graph_frame_all$;
        if ($h == null) Rnode_graph_frame_all$ = $h = find("Rnode_graph_frame_all", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(graph);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_set_grid_visible$;

    /**
     * R_FALSE hides the grid of dots (shown by default).
     */
    public static void Rnode_graph_set_grid_visible(int graph, int visible) {
        MethodHandle $h = Rnode_graph_set_grid_visible$;
        if ($h == null) Rnode_graph_set_grid_visible$ = $h = find("Rnode_graph_set_grid_visible", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(graph, visible);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_set_flow_visible$;

    /**
     * The pulses of light that run along the links, from output to
     * input. By default they run while the pointer is over the graph, and
     * the window rests the rest of the time. R_TRUE makes them run all the
     * time (the window then draws without pause); R_FALSE stops them.
     */
    public static void Rnode_graph_set_flow_visible(int graph, int visible) {
        MethodHandle $h = Rnode_graph_set_flow_visible$;
        if ($h == null) Rnode_graph_set_flow_visible$ = $h = find("Rnode_graph_set_flow_visible", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(graph, visible);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_set_read_only$;

    /**
     * R_TRUE: the user can look but not move, link or delete.
     */
    public static void Rnode_graph_set_read_only(int graph, int read_only) {
        MethodHandle $h = Rnode_graph_set_read_only$;
        if ($h == null) Rnode_graph_set_read_only$ = $h = find("Rnode_graph_set_read_only", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(graph, read_only);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_select_node$;

    public static void Rnode_graph_select_node(int graph, int node, int add) {
        MethodHandle $h = Rnode_graph_select_node$;
        if ($h == null) Rnode_graph_select_node$ = $h = find("Rnode_graph_select_node", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(graph, node, add);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_clear_selection$;

    public static void Rnode_graph_clear_selection(int graph) {
        MethodHandle $h = Rnode_graph_clear_selection$;
        if ($h == null) Rnode_graph_clear_selection$ = $h = find("Rnode_graph_clear_selection", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(graph);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_is_node_selected$;

    public static int Rnode_graph_is_node_selected(int graph, int node) {
        MethodHandle $h = Rnode_graph_is_node_selected$;
        if ($h == null) Rnode_graph_is_node_selected$ = $h = find("Rnode_graph_is_node_selected", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(graph, node);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_get_selection_count$;

    public static int Rnode_graph_get_selection_count(int graph) {
        MethodHandle $h = Rnode_graph_get_selection_count$;
        if ($h == null) Rnode_graph_get_selection_count$ = $h = find("Rnode_graph_get_selection_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(graph);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_get_selected_node$;

    public static int Rnode_graph_get_selected_node(int graph, int index) {
        MethodHandle $h = Rnode_graph_get_selected_node$;
        if ($h == null) Rnode_graph_get_selected_node$ = $h = find("Rnode_graph_get_selected_node", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(graph, index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_set_connect_callback$;

    public static void Rnode_graph_set_connect_callback(int graph, RNodeGraphConnectCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rnode_graph_set_connect_callback$;
        if ($h == null) Rnode_graph_set_connect_callback$ = $h = find("Rnode_graph_set_connect_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(graph, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rnode_graph_set_connect_callback(int graph, RNodeGraphConnectCallback callback) {
        Rnode_graph_set_connect_callback(graph, callback, (MemorySegment) null);
    }

    private static MethodHandle Rnode_graph_set_linked_callback$;

    /**
     * Called after the user made a link, and before a link the user removed goes (its pins can still be read).
     */
    public static void Rnode_graph_set_linked_callback(int graph, RNodeGraphItemCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rnode_graph_set_linked_callback$;
        if ($h == null) Rnode_graph_set_linked_callback$ = $h = find("Rnode_graph_set_linked_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(graph, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Called after the user made a link, and before a link the user removed goes (its pins can still be read).
     */
    public static void Rnode_graph_set_linked_callback(int graph, RNodeGraphItemCallback callback) {
        Rnode_graph_set_linked_callback(graph, callback, (MemorySegment) null);
    }

    private static MethodHandle Rnode_graph_set_unlinked_callback$;

    public static void Rnode_graph_set_unlinked_callback(int graph, RNodeGraphItemCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rnode_graph_set_unlinked_callback$;
        if ($h == null) Rnode_graph_set_unlinked_callback$ = $h = find("Rnode_graph_set_unlinked_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(graph, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rnode_graph_set_unlinked_callback(int graph, RNodeGraphItemCallback callback) {
        Rnode_graph_set_unlinked_callback(graph, callback, (MemorySegment) null);
    }

    private static MethodHandle Rnode_graph_set_moved_callback$;

    /**
     * Called for each node the user moved, when the button is released.
     */
    public static void Rnode_graph_set_moved_callback(int graph, RNodeGraphItemCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rnode_graph_set_moved_callback$;
        if ($h == null) Rnode_graph_set_moved_callback$ = $h = find("Rnode_graph_set_moved_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(graph, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Called for each node the user moved, when the button is released.
     */
    public static void Rnode_graph_set_moved_callback(int graph, RNodeGraphItemCallback callback) {
        Rnode_graph_set_moved_callback(graph, callback, (MemorySegment) null);
    }

    private static MethodHandle Rnode_graph_set_selection_callback$;

    /**
     * Called when the selection changed; `item` is the node just clicked, or 0.
     */
    public static void Rnode_graph_set_selection_callback(int graph, RNodeGraphItemCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rnode_graph_set_selection_callback$;
        if ($h == null) Rnode_graph_set_selection_callback$ = $h = find("Rnode_graph_set_selection_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(graph, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Called when the selection changed; `item` is the node just clicked, or 0.
     */
    public static void Rnode_graph_set_selection_callback(int graph, RNodeGraphItemCallback callback) {
        Rnode_graph_set_selection_callback(graph, callback, (MemorySegment) null);
    }

    private static MethodHandle Rnode_graph_set_removed_callback$;

    /**
     * Called when the user deletes a node (the Delete key), before
     * it is removed. Without this callback the Delete key only removes
     * links: the application decides whether nodes can be deleted.
     */
    public static void Rnode_graph_set_removed_callback(int graph, RNodeGraphItemCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rnode_graph_set_removed_callback$;
        if ($h == null) Rnode_graph_set_removed_callback$ = $h = find("Rnode_graph_set_removed_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(graph, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Called when the user deletes a node (the Delete key), before
     * it is removed. Without this callback the Delete key only removes
     * links: the application decides whether nodes can be deleted.
     */
    public static void Rnode_graph_set_removed_callback(int graph, RNodeGraphItemCallback callback) {
        Rnode_graph_set_removed_callback(graph, callback, (MemorySegment) null);
    }

    private static MethodHandle Rnode_graph_set_activated_callback$;

    /**
     * Called when the user double-clicks a node (to open what it stands for).
     */
    public static void Rnode_graph_set_activated_callback(int graph, RNodeGraphItemCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rnode_graph_set_activated_callback$;
        if ($h == null) Rnode_graph_set_activated_callback$ = $h = find("Rnode_graph_set_activated_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(graph, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Called when the user double-clicks a node (to open what it stands for).
     */
    public static void Rnode_graph_set_activated_callback(int graph, RNodeGraphItemCallback callback) {
        Rnode_graph_set_activated_callback(graph, callback, (MemorySegment) null);
    }

    private static MethodHandle Rnode_graph_set_context_callback$;

    public static void Rnode_graph_set_context_callback(int graph, RNodeGraphContextCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rnode_graph_set_context_callback$;
        if ($h == null) Rnode_graph_set_context_callback$ = $h = find("Rnode_graph_set_context_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(graph, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rnode_graph_set_context_callback(int graph, RNodeGraphContextCallback callback) {
        Rnode_graph_set_context_callback(graph, callback, (MemorySegment) null);
    }

    private static MethodHandle Rnode_graph_set_pin_multiple$;

    /**
     * R_TRUE: this input keeps every link made to it (a state that several
     * transitions enter) instead of one. Pulling a wire from it starts a new one.
     */
    public static void Rnode_graph_set_pin_multiple(int graph, int pin, int multiple) {
        MethodHandle $h = Rnode_graph_set_pin_multiple$;
        if ($h == null) Rnode_graph_set_pin_multiple$ = $h = find("Rnode_graph_set_pin_multiple", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(graph, pin, multiple);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_set_node_active$;

    /**
     * R_TRUE: the node is running (live debug) : a soft ring breathes around it.
     */
    public static void Rnode_graph_set_node_active(int graph, int node, int active) {
        MethodHandle $h = Rnode_graph_set_node_active$;
        if ($h == null) Rnode_graph_set_node_active$ = $h = find("Rnode_graph_set_node_active", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(graph, node, active);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_set_node_active_color$;

    /**
     * Color of that ring (the success color of the theme by default).
     */
    public static void Rnode_graph_set_node_active_color(int graph, int node, float r, float g, float b) {
        MethodHandle $h = Rnode_graph_set_node_active_color$;
        if ($h == null) Rnode_graph_set_node_active_color$ = $h = find("Rnode_graph_set_node_active_color", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(graph, node, r, g, b);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_set_node_deletable$;

    /**
     * R_FALSE: the Delete key does not remove this node (the root of a tree, the output of a graph).
     */
    public static void Rnode_graph_set_node_deletable(int graph, int node, int deletable) {
        MethodHandle $h = Rnode_graph_set_node_deletable$;
        if ($h == null) Rnode_graph_set_node_deletable$ = $h = find("Rnode_graph_set_node_deletable", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(graph, node, deletable);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rnode_graph_set_node_badge$;

    /**
     * A short text in a pill on the top right corner of the node ("1", "!"...); NULL or "" removes it.
     */
    public static void Rnode_graph_set_node_badge(int graph, int node, String text) {
        MethodHandle $h = Rnode_graph_set_node_badge$;
        if ($h == null) Rnode_graph_set_node_badge$ = $h = find("Rnode_graph_set_node_badge", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            $h.invokeExact(graph, node, text$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_create$;

    public static int Rblock_editor_create() {
        MethodHandle $h = Rblock_editor_create$;
        if ($h == null) Rblock_editor_create$ = $h = find("Rblock_editor_create", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_add_kind$;

    /**
     * A kind of block, and its block in the palette. Returns its index, -1 on an error.
     */
    public static int Rblock_editor_add_kind(int editor, String name, String label, int shape, String category) {
        MethodHandle $h = Rblock_editor_add_kind$;
        if ($h == null) Rblock_editor_add_kind$ = $h = find("Rblock_editor_add_kind", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            MemorySegment label$ = label == null ? MemorySegment.NULL : utf8($a, label);
            MemorySegment category$ = category == null ? MemorySegment.NULL : utf8($a, category);
            int $r = (int) $h.invokeExact(editor, name$, label$, shape, category$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_set_category_color$;

    /**
     * Color of the blocks of a category (Raven picks one from its name otherwise).
     */
    public static void Rblock_editor_set_category_color(int editor, String category, float r, float g, float b) {
        MethodHandle $h = Rblock_editor_set_category_color$;
        if ($h == null) Rblock_editor_set_category_color$ = $h = find("Rblock_editor_set_category_color", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment category$ = category == null ? MemorySegment.NULL : utf8($a, category);
            $h.invokeExact(editor, category$, r, g, b);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_set_palette_visible$;

    public static void Rblock_editor_set_palette_visible(int editor, int visible) {
        MethodHandle $h = Rblock_editor_set_palette_visible$;
        if ($h == null) Rblock_editor_set_palette_visible$ = $h = find("Rblock_editor_set_palette_visible", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(editor, visible);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_add_block$;

    /**
     * A new block, the top of a script of its own at x, y (canvas units). Returns it, 0 on an error.
     */
    public static int Rblock_editor_add_block(int editor, String kind, float x, float y) {
        MethodHandle $h = Rblock_editor_add_block$;
        if ($h == null) Rblock_editor_add_block$ = $h = find("Rblock_editor_add_block", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS, JAVA_FLOAT, JAVA_FLOAT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment kind$ = kind == null ? MemorySegment.NULL : utf8($a, kind);
            int $r = (int) $h.invokeExact(editor, kind$, x, y);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_remove_block$;

    /**
     * Removes a block and everything that hangs under it.
     */
    public static void Rblock_editor_remove_block(int editor, int block) {
        MethodHandle $h = Rblock_editor_remove_block$;
        if ($h == null) Rblock_editor_remove_block$ = $h = find("Rblock_editor_remove_block", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(editor, block);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_clear$;

    public static void Rblock_editor_clear(int editor) {
        MethodHandle $h = Rblock_editor_clear$;
        if ($h == null) Rblock_editor_clear$ = $h = find("Rblock_editor_clear", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(editor);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_attach$;

    /**
     * Puts a block (with what is under it) at a place of another one. R_FALSE when it does not fit there.
     */
    public static int Rblock_editor_attach(int editor, int block, int target, int place, int index) {
        MethodHandle $h = Rblock_editor_attach$;
        if ($h == null) Rblock_editor_attach$ = $h = find("Rblock_editor_attach", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(editor, block, target, place, index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_detach$;

    /**
     * Takes a block (with what is under it) out: a script of its own at x, y.
     */
    public static void Rblock_editor_detach(int editor, int block, float x, float y) {
        MethodHandle $h = Rblock_editor_detach$;
        if ($h == null) Rblock_editor_detach$ = $h = find("Rblock_editor_detach", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(editor, block, x, y);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_get_script_count$;

    public static int Rblock_editor_get_script_count(int editor) {
        MethodHandle $h = Rblock_editor_get_script_count$;
        if ($h == null) Rblock_editor_get_script_count$ = $h = find("Rblock_editor_get_script_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(editor);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_get_script$;

    /**
     * The top block of a script.
     */
    public static int Rblock_editor_get_script(int editor, int index) {
        MethodHandle $h = Rblock_editor_get_script$;
        if ($h == null) Rblock_editor_get_script$ = $h = find("Rblock_editor_get_script", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(editor, index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_get_kind$;

    public static String Rblock_editor_get_kind(int editor, int block) {
        MethodHandle $h = Rblock_editor_get_kind$;
        if ($h == null) Rblock_editor_get_kind$ = $h = find("Rblock_editor_get_kind", FunctionDescriptor.of(ADDRESS, JAVA_INT, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(editor, block);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_get_next$;

    /**
     * The block under it, 0 if none.
     */
    public static int Rblock_editor_get_next(int editor, int block) {
        MethodHandle $h = Rblock_editor_get_next$;
        if ($h == null) Rblock_editor_get_next$ = $h = find("Rblock_editor_get_next", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(editor, block);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_get_mouth_count$;

    public static int Rblock_editor_get_mouth_count(int editor, int block) {
        MethodHandle $h = Rblock_editor_get_mouth_count$;
        if ($h == null) Rblock_editor_get_mouth_count$ = $h = find("Rblock_editor_get_mouth_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(editor, block);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_get_body$;

    /**
     * The first block in a mouth of a C block, 0 if empty.
     */
    public static int Rblock_editor_get_body(int editor, int block, int mouth) {
        MethodHandle $h = Rblock_editor_get_body$;
        if ($h == null) Rblock_editor_get_body$ = $h = find("Rblock_editor_get_body", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(editor, block, mouth);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_get_slot_count$;

    public static int Rblock_editor_get_slot_count(int editor, int block) {
        MethodHandle $h = Rblock_editor_get_slot_count$;
        if ($h == null) Rblock_editor_get_slot_count$ = $h = find("Rblock_editor_get_slot_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(editor, block);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_get_input$;

    /**
     * The value block in a slot, 0 if the slot holds a typed value.
     */
    public static int Rblock_editor_get_input(int editor, int block, int slot) {
        MethodHandle $h = Rblock_editor_get_input$;
        if ($h == null) Rblock_editor_get_input$ = $h = find("Rblock_editor_get_input", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(editor, block, slot);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_get_value$;

    /**
     * The typed value of a slot.
     */
    public static String Rblock_editor_get_value(int editor, int block, int slot) {
        MethodHandle $h = Rblock_editor_get_value$;
        if ($h == null) Rblock_editor_get_value$ = $h = find("Rblock_editor_get_value", FunctionDescriptor.of(ADDRESS, JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(editor, block, slot);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_set_value$;

    public static void Rblock_editor_set_value(int editor, int block, int slot, String value) {
        MethodHandle $h = Rblock_editor_set_value$;
        if ($h == null) Rblock_editor_set_value$ = $h = find("Rblock_editor_set_value", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment value$ = value == null ? MemorySegment.NULL : utf8($a, value);
            $h.invokeExact(editor, block, slot, value$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_get_parent$;

    /**
     * Where a block hangs: the block it hangs from (0: the top of a script).
     */
    public static int Rblock_editor_get_parent(int editor, int block, int[] place, int[] index) {
        MethodHandle $h = Rblock_editor_get_parent$;
        if ($h == null) Rblock_editor_get_parent$ = $h = find("Rblock_editor_get_parent", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment place$ = place == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            MemorySegment index$ = index == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            int $r = (int) $h.invokeExact(editor, block, place$, index$);
            if (place != null && place.length > 0) place[0] = place$.get(JAVA_INT, 0);
            if (index != null && index.length > 0) index[0] = index$.get(JAVA_INT, 0);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_get_position$;

    public static void Rblock_editor_get_position(int editor, int block, float[] x, float[] y) {
        MethodHandle $h = Rblock_editor_get_position$;
        if ($h == null) Rblock_editor_get_position$ = $h = find("Rblock_editor_get_position", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment x$ = x == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment y$ = y == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            $h.invokeExact(editor, block, x$, y$);
            if (x != null && x.length > 0) x[0] = x$.get(JAVA_FLOAT, 0);
            if (y != null && y.length > 0) y[0] = y$.get(JAVA_FLOAT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_set_block_glow$;

    /**
     * R_TRUE: the block is running: a halo breathes around it.
     */
    public static void Rblock_editor_set_block_glow(int editor, int block, int glow) {
        MethodHandle $h = Rblock_editor_set_block_glow$;
        if ($h == null) Rblock_editor_set_block_glow$ = $h = find("Rblock_editor_set_block_glow", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(editor, block, glow);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_save$;

    /**
     * Every script as text (valid until the next call).
     */
    public static String Rblock_editor_save(int editor) {
        MethodHandle $h = Rblock_editor_save$;
        if ($h == null) Rblock_editor_save$ = $h = find("Rblock_editor_save", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(editor);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_load$;

    /**
     * Replaces the scripts with those of a text from Rblock_editor_save.
     */
    public static int Rblock_editor_load(int editor, String text) {
        MethodHandle $h = Rblock_editor_load$;
        if ($h == null) Rblock_editor_load$ = $h = find("Rblock_editor_load", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            int $r = (int) $h.invokeExact(editor, text$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_set_changed_callback$;

    public static void Rblock_editor_set_changed_callback(int editor, RBlockEditorCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rblock_editor_set_changed_callback$;
        if ($h == null) Rblock_editor_set_changed_callback$ = $h = find("Rblock_editor_set_changed_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(editor, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rblock_editor_set_changed_callback(int editor, RBlockEditorCallback callback) {
        Rblock_editor_set_changed_callback(editor, callback, (MemorySegment) null);
    }

    private static MethodHandle Rblock_editor_set_clicked_callback$;

    public static void Rblock_editor_set_clicked_callback(int editor, RBlockEditorCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rblock_editor_set_clicked_callback$;
        if ($h == null) Rblock_editor_set_clicked_callback$ = $h = find("Rblock_editor_set_clicked_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(editor, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rblock_editor_set_clicked_callback(int editor, RBlockEditorCallback callback) {
        Rblock_editor_set_clicked_callback(editor, callback, (MemorySegment) null);
    }

    private static MethodHandle Rblock_editor_set_context_callback$;

    public static void Rblock_editor_set_context_callback(int editor, RBlockEditorContextCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rblock_editor_set_context_callback$;
        if ($h == null) Rblock_editor_set_context_callback$ = $h = find("Rblock_editor_set_context_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(editor, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rblock_editor_set_context_callback(int editor, RBlockEditorContextCallback callback) {
        Rblock_editor_set_context_callback(editor, callback, (MemorySegment) null);
    }

    private static MethodHandle Rblock_editor_set_slot_options$;

    /**
     * The choices of a %m slot of a kind, separated by '\n' ("" : the slot is typed in).
     */
    public static void Rblock_editor_set_slot_options(int editor, String kind, int slot, String options) {
        MethodHandle $h = Rblock_editor_set_slot_options$;
        if ($h == null) Rblock_editor_set_slot_options$ = $h = find("Rblock_editor_set_slot_options", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment kind$ = kind == null ? MemorySegment.NULL : utf8($a, kind);
            MemorySegment options$ = options == null ? MemorySegment.NULL : utf8($a, options);
            $h.invokeExact(editor, kind$, slot, options$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_set_kind_visible$;

    /**
     * R_FALSE: the kind is not in the palette (its blocks stay where they are).
     */
    public static void Rblock_editor_set_kind_visible(int editor, String kind, int visible) {
        MethodHandle $h = Rblock_editor_set_kind_visible$;
        if ($h == null) Rblock_editor_set_kind_visible$ = $h = find("Rblock_editor_set_kind_visible", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, JAVA_INT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment kind$ = kind == null ? MemorySegment.NULL : utf8($a, kind);
            $h.invokeExact(editor, kind$, visible);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_set_kind_label$;

    /**
     * A new label for a kind (a function that takes one more input...).
     * The blocks of that kind keep the values of the slots that are still there.
     */
    public static int Rblock_editor_set_kind_label(int editor, String kind, String label) {
        MethodHandle $h = Rblock_editor_set_kind_label$;
        if ($h == null) Rblock_editor_set_kind_label$ = $h = find("Rblock_editor_set_kind_label", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment kind$ = kind == null ? MemorySegment.NULL : utf8($a, kind);
            MemorySegment label$ = label == null ? MemorySegment.NULL : utf8($a, label);
            int $r = (int) $h.invokeExact(editor, kind$, label$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_set_block_deletable$;

    /**
     * R_FALSE: the user cannot delete the block (the entry of a function). Not saved.
     */
    public static void Rblock_editor_set_block_deletable(int editor, int block, int deletable) {
        MethodHandle $h = Rblock_editor_set_block_deletable$;
        if ($h == null) Rblock_editor_set_block_deletable$ = $h = find("Rblock_editor_set_block_deletable", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(editor, block, deletable);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_get_selection_count$;

    public static int Rblock_editor_get_selection_count(int editor) {
        MethodHandle $h = Rblock_editor_get_selection_count$;
        if ($h == null) Rblock_editor_get_selection_count$ = $h = find("Rblock_editor_get_selection_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(editor);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_get_selected$;

    public static int Rblock_editor_get_selected(int editor, int index) {
        MethodHandle $h = Rblock_editor_get_selected$;
        if ($h == null) Rblock_editor_get_selected$ = $h = find("Rblock_editor_get_selected", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(editor, index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_select$;

    /**
     * Selects a block (add: R_FALSE clears the selection first ; block 0 : only clears).
     */
    public static void Rblock_editor_select(int editor, int block, int add) {
        MethodHandle $h = Rblock_editor_select$;
        if ($h == null) Rblock_editor_select$ = $h = find("Rblock_editor_select", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(editor, block, add);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_get_view$;

    /**
     * Pan (canvas units at the top left) and zoom.
     */
    public static void Rblock_editor_get_view(int editor, float[] x, float[] y, float[] zoom) {
        MethodHandle $h = Rblock_editor_get_view$;
        if ($h == null) Rblock_editor_get_view$ = $h = find("Rblock_editor_get_view", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment x$ = x == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment y$ = y == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            MemorySegment zoom$ = zoom == null ? MemorySegment.NULL : $a.allocate(JAVA_FLOAT);
            $h.invokeExact(editor, x$, y$, zoom$);
            if (x != null && x.length > 0) x[0] = x$.get(JAVA_FLOAT, 0);
            if (y != null && y.length > 0) y[0] = y$.get(JAVA_FLOAT, 0);
            if (zoom != null && zoom.length > 0) zoom[0] = zoom$.get(JAVA_FLOAT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rblock_editor_set_view$;

    public static void Rblock_editor_set_view(int editor, float x, float y, float zoom) {
        MethodHandle $h = Rblock_editor_set_view$;
        if ($h == null) Rblock_editor_set_view$ = $h = find("Rblock_editor_set_view", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact(editor, x, y, zoom);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdialog_open$;

    /**
     * Shows a widget as a dialog of a window. The window takes
     * ownership of `content`: it is deleted when the dialog closes, so read
     * what you need from it before, or in `closed`.
     * The Escape key dismisses the dialog (result 0).
     */
    public static void Rdialog_open(int window, int content, RDialogClosedCallback closed, MemorySegment user_data) {
        MethodHandle $h = Rdialog_open$;
        if ($h == null) Rdialog_open$ = $h = find("Rdialog_open", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(window, content, upcall(closed), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Shows a widget as a dialog of a window. The window takes
     * ownership of `content`: it is deleted when the dialog closes, so read
     * what you need from it before, or in `closed`.
     * The Escape key dismisses the dialog (result 0).
     */
    public static void Rdialog_open(int window, int content, RDialogClosedCallback closed) {
        Rdialog_open(window, content, closed, (MemorySegment) null);
    }

    private static MethodHandle Rdialog_close$;

    /**
     * Closes the dialog that contains a widget (the content itself or
     * anything inside it: the button that was clicked).
     * result Passed to the closed callback and returned by Rdialog_run.
     */
    public static void Rdialog_close(int widget, int result) {
        MethodHandle $h = Rdialog_close$;
        if ($h == null) Rdialog_close$ = $h = find("Rdialog_close", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(widget, result);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdialog_run$;

    /**
     * Shows a dialog and waits until it closes. Returns its result.
     */
    public static int Rdialog_run(int window, int content) {
        MethodHandle $h = Rdialog_run$;
        if ($h == null) Rdialog_run$ = $h = find("Rdialog_run", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window, content);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_has_dialog$;

    /**
     * Returns: R_TRUE while a dialog is shown in the window.
     */
    public static int Rwindow_has_dialog(int window) {
        MethodHandle $h = Rwindow_has_dialog$;
        if ($h == null) Rwindow_has_dialog$ = $h = find("Rwindow_has_dialog", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(window);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rmessage_box$;

    /**
     * Shows a message with buttons and waits for the answer.
     * window R_INVALID_ID: the first window of the application.
     * title  May be NULL.
     */
    public static int Rmessage_box(int window, String title, String text, int buttons) {
        MethodHandle $h = Rmessage_box$;
        if ($h == null) Rmessage_box$ = $h = find("Rmessage_box", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS, JAVA_INT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment title$ = title == null ? MemorySegment.NULL : utf8($a, title);
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            int $r = (int) $h.invokeExact(window, title$, text$, buttons);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_dialog_open$;

    /**
     * Asks for a file to open.
     * desc May be NULL.
     * Returns: The path (UTF-8), valid until the next file dialog, or NULL when the user cancelled.
     */
    public static String Rfile_dialog_open(RFileDialogDesc desc) {
        MethodHandle $h = Rfile_dialog_open$;
        if ($h == null) Rfile_dialog_open$ = $h = find("Rfile_dialog_open", FunctionDescriptor.of(ADDRESS, ADDRESS));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact((MemorySegment) (desc == null ? MemorySegment.NULL : desc.segment()));
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_dialog_save$;

    /**
     * Asks where to save a file. An existing file was confirmed by the user.
     */
    public static String Rfile_dialog_save(RFileDialogDesc desc) {
        MethodHandle $h = Rfile_dialog_save$;
        if ($h == null) Rfile_dialog_save$ = $h = find("Rfile_dialog_save", FunctionDescriptor.of(ADDRESS, ADDRESS));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact((MemorySegment) (desc == null ? MemorySegment.NULL : desc.segment()));
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_dialog_select_folder$;

    /**
     * Asks for a folder.
     */
    public static String Rfile_dialog_select_folder(RFileDialogDesc desc) {
        MethodHandle $h = Rfile_dialog_select_folder$;
        if ($h == null) Rfile_dialog_select_folder$ = $h = find("Rfile_dialog_select_folder", FunctionDescriptor.of(ADDRESS, ADDRESS));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact((MemorySegment) (desc == null ? MemorySegment.NULL : desc.segment()));
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_dialog_get_result_count$;

    /**
     * What the last file dialog returned: several paths with R_FILE_DIALOG_MULTIPLE.
     */
    public static int Rfile_dialog_get_result_count() {
        MethodHandle $h = Rfile_dialog_get_result_count$;
        if ($h == null) Rfile_dialog_get_result_count$ = $h = find("Rfile_dialog_get_result_count", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_dialog_get_result$;

    public static String Rfile_dialog_get_result(int index) {
        MethodHandle $h = Rfile_dialog_get_result$;
        if ($h == null) Rfile_dialog_get_result$ = $h = find("Rfile_dialog_get_result", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(index);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_register_memory$;

    /**
     * Registers a file held in memory under a path: every file Raven
     * reads (XML documents and their Include, Source and Font attributes,
     * themes, textures, fonts, Rfile_read) is looked up here first, so that
     * resources embedded in the executable need no file beside it. A relative
     * path names the file as it is written in the XML once joined to the
     * folder of the document that names it (Rxml_set_base_path for a
     * document given as text), e.g. "ui/editor.xml". Paths compare with '/'
     * and '\\' alike. Memory files are not watched for live reload. Call it
     * after Rinit; Rshutdown forgets every file.
     * copy R_TRUE: Raven keeps a copy; R_FALSE: `data` must outlive Raven (static data).
     */
    public static int Rfile_register_memory(String path, byte[] data, int copy) {
        MethodHandle $h = Rfile_register_memory$;
        if ($h == null) Rfile_register_memory$ = $h = find("Rfile_register_memory", FunctionDescriptor.of(JAVA_INT, ADDRESS, ADDRESS, JAVA_LONG, JAVA_INT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment path$ = path == null ? MemorySegment.NULL : utf8($a, path);
            MemorySegment data$ = data == null ? MemorySegment.NULL : bytes(copy == 0 ? Arena.global() : $a, data);
            int $r = (int) $h.invokeExact(path$, data$, data$.byteSize(), copy);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_unregister_memory$;

    public static void Rfile_unregister_memory(String path) {
        MethodHandle $h = Rfile_unregister_memory$;
        if ($h == null) Rfile_unregister_memory$ = $h = find("Rfile_unregister_memory", FunctionDescriptor.ofVoid(ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment path$ = path == null ? MemorySegment.NULL : utf8($a, path);
            $h.invokeExact(path$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_is_memory$;

    /**
     * R_TRUE when `path` is a file registered with Rfile_register_memory.
     */
    public static int Rfile_is_memory(String path) {
        MethodHandle $h = Rfile_is_memory$;
        if ($h == null) Rfile_is_memory$ = $h = find("Rfile_is_memory", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment path$ = path == null ? MemorySegment.NULL : utf8($a, path);
            int $r = (int) $h.invokeExact(path$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_read$;

    /**
     * Whole files, with UTF-8 paths on every platform.
     * Returns: Rfile_read: the content followed by a '\0', to release with
     * Rmemory_free; NULL when the file cannot be read. `size` may be NULL.
     */
    public static byte[] Rfile_read(String path) {
        MethodHandle $h = Rfile_read$;
        if ($h == null) Rfile_read$ = $h = find("Rfile_read", FunctionDescriptor.of(ADDRESS, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment path$ = path == null ? MemorySegment.NULL : utf8($a, path);
            MemorySegment size$ = $a.allocate(JAVA_LONG);
            MemorySegment $r = (MemorySegment) $h.invokeExact(path$, size$);
            if ($r.equals(MemorySegment.NULL)) return null;
            byte[] $data = $r.reinterpret(size$.get(JAVA_LONG, 0)).toArray(JAVA_BYTE);
            Rmemory_free($r);
            return $data;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_write$;

    public static int Rfile_write(String path, byte[] data) {
        MethodHandle $h = Rfile_write$;
        if ($h == null) Rfile_write$ = $h = find("Rfile_write", FunctionDescriptor.of(JAVA_INT, ADDRESS, ADDRESS, JAVA_LONG));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment path$ = path == null ? MemorySegment.NULL : utf8($a, path);
            MemorySegment data$ = data == null ? MemorySegment.NULL : bytes($a, data);
            int $r = (int) $h.invokeExact(path$, data$, data$.byteSize());
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_browser_create$;

    public static int Rfile_browser_create() {
        MethodHandle $h = Rfile_browser_create$;
        if ($h == null) Rfile_browser_create$ = $h = find("Rfile_browser_create", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_browser_set_mode$;

    public static void Rfile_browser_set_mode(int browser, int mode) {
        MethodHandle $h = Rfile_browser_set_mode$;
        if ($h == null) Rfile_browser_set_mode$ = $h = find("Rfile_browser_set_mode", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(browser, mode);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_browser_set_directory$;

    /**
     * Shows a folder. "~" is the home of the user. Returns R_FALSE when it cannot be read.
     */
    public static int Rfile_browser_set_directory(int browser, String path) {
        MethodHandle $h = Rfile_browser_set_directory$;
        if ($h == null) Rfile_browser_set_directory$ = $h = find("Rfile_browser_set_directory", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment path$ = path == null ? MemorySegment.NULL : utf8($a, path);
            int $r = (int) $h.invokeExact(browser, path$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_browser_get_directory$;

    public static String Rfile_browser_get_directory(int browser) {
        MethodHandle $h = Rfile_browser_get_directory$;
        if ($h == null) Rfile_browser_get_directory$ = $h = find("Rfile_browser_get_directory", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(browser);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_browser_refresh$;

    /**
     * Reads the folder again.
     */
    public static void Rfile_browser_refresh(int browser) {
        MethodHandle $h = Rfile_browser_refresh$;
        if ($h == null) Rfile_browser_refresh$ = $h = find("Rfile_browser_refresh", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(browser);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_browser_set_filters$;

    /**
     * Kinds of files the user chooses from; the first one is active. The strings are copied.
     */
    public static void Rfile_browser_set_filters(int browser, RFileFilter filters, int count) {
        MethodHandle $h = Rfile_browser_set_filters$;
        if ($h == null) Rfile_browser_set_filters$ = $h = find("Rfile_browser_set_filters", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, JAVA_INT));
        try {
            $h.invokeExact(browser, (MemorySegment) (filters == null ? MemorySegment.NULL : filters.segment()), count);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_browser_set_show_hidden$;

    public static void Rfile_browser_set_show_hidden(int browser, int show) {
        MethodHandle $h = Rfile_browser_set_show_hidden$;
        if ($h == null) Rfile_browser_set_show_hidden$ = $h = find("Rfile_browser_set_show_hidden", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(browser, show);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_browser_set_multiple$;

    /**
     * R_TRUE lets several files be selected (Ctrl and Shift with a click).
     */
    public static void Rfile_browser_set_multiple(int browser, int multiple) {
        MethodHandle $h = Rfile_browser_set_multiple$;
        if ($h == null) Rfile_browser_set_multiple$ = $h = find("Rfile_browser_set_multiple", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(browser, multiple);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_browser_set_controls_visible$;

    /**
     * R_FALSE hides the name field, the filter list and the buttons:
     * what remains is an explorer to put in a side panel. Shown by default.
     */
    public static void Rfile_browser_set_controls_visible(int browser, int visible) {
        MethodHandle $h = Rfile_browser_set_controls_visible$;
        if ($h == null) Rfile_browser_set_controls_visible$ = $h = find("Rfile_browser_set_controls_visible", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(browser, visible);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_browser_set_places_visible$;

    /**
     * R_FALSE hides the list of places on the left.
     */
    public static void Rfile_browser_set_places_visible(int browser, int visible) {
        MethodHandle $h = Rfile_browser_set_places_visible$;
        if ($h == null) Rfile_browser_set_places_visible$ = $h = find("Rfile_browser_set_places_visible", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(browser, visible);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_browser_set_file_name$;

    /**
     * Name typed in the name field (save mode).
     */
    public static void Rfile_browser_set_file_name(int browser, String name) {
        MethodHandle $h = Rfile_browser_set_file_name$;
        if ($h == null) Rfile_browser_set_file_name$ = $h = find("Rfile_browser_set_file_name", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            $h.invokeExact(browser, name$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_browser_set_default_extension$;

    /**
     * Added to a name typed without extension, in save mode.
     */
    public static void Rfile_browser_set_default_extension(int browser, String extension) {
        MethodHandle $h = Rfile_browser_set_default_extension$;
        if ($h == null) Rfile_browser_set_default_extension$ = $h = find("Rfile_browser_set_default_extension", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment extension$ = extension == null ? MemorySegment.NULL : utf8($a, extension);
            $h.invokeExact(browser, extension$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_browser_get_selection_count$;

    /**
     * Paths of the selected entries. The strings are valid until the selection or the folder changes.
     */
    public static int Rfile_browser_get_selection_count(int browser) {
        MethodHandle $h = Rfile_browser_get_selection_count$;
        if ($h == null) Rfile_browser_get_selection_count$ = $h = find("Rfile_browser_get_selection_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(browser);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_browser_get_selection$;

    public static String Rfile_browser_get_selection(int browser, int index) {
        MethodHandle $h = Rfile_browser_get_selection$;
        if ($h == null) Rfile_browser_get_selection$ = $h = find("Rfile_browser_get_selection", FunctionDescriptor.of(ADDRESS, JAVA_INT, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(browser, index);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_browser_select$;

    /**
     * Selects an entry of the folder by its name.
     */
    public static void Rfile_browser_select(int browser, String name) {
        MethodHandle $h = Rfile_browser_select$;
        if ($h == null) Rfile_browser_select$ = $h = find("Rfile_browser_select", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            $h.invokeExact(browser, name$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rfile_browser_set_accepted_callback$;

    public static void Rfile_browser_set_accepted_callback(int browser, RFileBrowserCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rfile_browser_set_accepted_callback$;
        if ($h == null) Rfile_browser_set_accepted_callback$ = $h = find("Rfile_browser_set_accepted_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(browser, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rfile_browser_set_accepted_callback(int browser, RFileBrowserCallback callback) {
        Rfile_browser_set_accepted_callback(browser, callback, (MemorySegment) null);
    }

    private static MethodHandle Rfile_browser_set_cancelled_callback$;

    /**
     * Called by the Cancel button.
     */
    public static void Rfile_browser_set_cancelled_callback(int browser, RFileBrowserCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rfile_browser_set_cancelled_callback$;
        if ($h == null) Rfile_browser_set_cancelled_callback$ = $h = find("Rfile_browser_set_cancelled_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(browser, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Called by the Cancel button.
     */
    public static void Rfile_browser_set_cancelled_callback(int browser, RFileBrowserCallback callback) {
        Rfile_browser_set_cancelled_callback(browser, callback, (MemorySegment) null);
    }

    private static MethodHandle Rfile_browser_set_selection_changed_callback$;

    /**
     * Called when the selection changes; `path` is the entry under the cursor, or NULL.
     */
    public static void Rfile_browser_set_selection_changed_callback(int browser, RFileBrowserCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rfile_browser_set_selection_changed_callback$;
        if ($h == null) Rfile_browser_set_selection_changed_callback$ = $h = find("Rfile_browser_set_selection_changed_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(browser, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Called when the selection changes; `path` is the entry under the cursor, or NULL.
     */
    public static void Rfile_browser_set_selection_changed_callback(int browser, RFileBrowserCallback callback) {
        Rfile_browser_set_selection_changed_callback(browser, callback, (MemorySegment) null);
    }

    private static MethodHandle Rviewport_set_texture$;

    /**
     * Shows a texture. R_INVALID_ID shows nothing.
     */
    public static void Rviewport_set_texture(int viewport, int texture) {
        MethodHandle $h = Rviewport_set_texture$;
        if ($h == null) Rviewport_set_texture$ = $h = find("Rviewport_set_texture", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(viewport, texture);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rviewport_set_stretch$;

    /**
     * How the texture is fitted in the widget. Default: R_STRETCH_FILL.
     */
    public static void Rviewport_set_stretch(int viewport, int stretch) {
        MethodHandle $h = Rviewport_set_stretch$;
        if ($h == null) Rviewport_set_stretch$ = $h = find("Rviewport_set_stretch", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(viewport, stretch);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rviewport_set_flip_y$;

    /**
     * R_TRUE flips the texture vertically (OpenGL framebuffers are bottom-up).
     */
    public static void Rviewport_set_flip_y(int viewport, int flip) {
        MethodHandle $h = Rviewport_set_flip_y$;
        if ($h == null) Rviewport_set_flip_y$ = $h = find("Rviewport_set_flip_y", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(viewport, flip);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rviewport_set_draw_callback$;

    public static void Rviewport_set_draw_callback(int viewport, RViewportDrawCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rviewport_set_draw_callback$;
        if ($h == null) Rviewport_set_draw_callback$ = $h = find("Rviewport_set_draw_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(viewport, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rviewport_set_draw_callback(int viewport, RViewportDrawCallback callback) {
        Rviewport_set_draw_callback(viewport, callback, (MemorySegment) null);
    }

    private static MethodHandle Rviewport_set_native$;

    /**
     * Creates (or destroys) the native child window of the viewport.
     * The viewport must already be in a window.
     * Returns: R_TRUE on success.
     */
    public static int Rviewport_set_native(int viewport, int enabled) {
        MethodHandle $h = Rviewport_set_native$;
        if ($h == null) Rviewport_set_native$ = $h = find("Rviewport_set_native", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(viewport, enabled);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rviewport_get_native_handle$;

    /**
     * Fills `native` with the handles of the child window.
     */
    public static int Rviewport_get_native_handle(int viewport, RNativeWindow native_) {
        MethodHandle $h = Rviewport_get_native_handle$;
        if ($h == null) Rviewport_get_native_handle$ = $h = find("Rviewport_get_native_handle", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS));
        try {
            int $r = (int) $h.invokeExact(viewport, (MemorySegment) (native_ == null ? MemorySegment.NULL : native_.segment()));
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rviewport_get_pixel_size$;

    /**
     * Size of the viewport in pixels after the last layout.
     */
    public static void Rviewport_get_pixel_size(int viewport, int[] width, int[] height) {
        MethodHandle $h = Rviewport_get_pixel_size$;
        if ($h == null) Rviewport_get_pixel_size$ = $h = find("Rviewport_get_pixel_size", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment width$ = width == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            MemorySegment height$ = height == null ? MemorySegment.NULL : $a.allocate(JAVA_INT);
            $h.invokeExact(viewport, width$, height$);
            if (width != null && width.length > 0) width[0] = width$.get(JAVA_INT, 0);
            if (height != null && height.length > 0) height[0] = height$.get(JAVA_INT, 0);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rviewport_set_resize_callback$;

    public static void Rviewport_set_resize_callback(int viewport, RViewportResizeCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rviewport_set_resize_callback$;
        if ($h == null) Rviewport_set_resize_callback$ = $h = find("Rviewport_set_resize_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(viewport, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rviewport_set_resize_callback(int viewport, RViewportResizeCallback callback) {
        Rviewport_set_resize_callback(viewport, callback, (MemorySegment) null);
    }

    private static MethodHandle Rviewport_set_realtime$;

    /**
     * R_TRUE redraws the window every frame while the viewport is
     * visible (game view). R_FALSE redraws only on Rwidget_invalidate (default).
     */
    public static void Rviewport_set_realtime(int viewport, int realtime) {
        MethodHandle $h = Rviewport_set_realtime$;
        if ($h == null) Rviewport_set_realtime$ = $h = find("Rviewport_set_realtime", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(viewport, realtime);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rcustom_set_paint_callback$;

    public static void Rcustom_set_paint_callback(int widget, RPaintCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rcustom_set_paint_callback$;
        if ($h == null) Rcustom_set_paint_callback$ = $h = find("Rcustom_set_paint_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(widget, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rcustom_set_paint_callback(int widget, RPaintCallback callback) {
        Rcustom_set_paint_callback(widget, callback, (MemorySegment) null);
    }

    private static MethodHandle Rcustom_set_measure_callback$;

    /**
     * Reports the desired size of the widget. Without it the widget asks for 0 x 0.
     */
    public static void Rcustom_set_measure_callback(int widget, RMeasureCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rcustom_set_measure_callback$;
        if ($h == null) Rcustom_set_measure_callback$ = $h = find("Rcustom_set_measure_callback", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try {
            $h.invokeExact(widget, upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Reports the desired size of the widget. Without it the widget asks for 0 x 0.
     */
    public static void Rcustom_set_measure_callback(int widget, RMeasureCallback callback) {
        Rcustom_set_measure_callback(widget, callback, (MemorySegment) null);
    }

    private static MethodHandle Rdraw_rect$;

    /**
     * Filled rectangle with rounded corners (radius 0 = sharp).
     */
    public static void Rdraw_rect(MemorySegment painter, float x, float y, float width, float height, float radius, float r, float g, float b, float a) {
        MethodHandle $h = Rdraw_rect$;
        if ($h == null) Rdraw_rect$ = $h = find("Rdraw_rect", FunctionDescriptor.ofVoid(ADDRESS, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact((MemorySegment) (painter == null ? MemorySegment.NULL : painter), x, y, width, height, radius, r, g, b, a);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdraw_rect_outline$;

    /**
     * Outline of a rounded rectangle, drawn inside its bounds.
     */
    public static void Rdraw_rect_outline(MemorySegment painter, float x, float y, float width, float height, float radius, float thickness, float r, float g, float b, float a) {
        MethodHandle $h = Rdraw_rect_outline$;
        if ($h == null) Rdraw_rect_outline$ = $h = find("Rdraw_rect_outline", FunctionDescriptor.ofVoid(ADDRESS, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact((MemorySegment) (painter == null ? MemorySegment.NULL : painter), x, y, width, height, radius, thickness, r, g, b, a);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdraw_circle$;

    public static void Rdraw_circle(MemorySegment painter, float center_x, float center_y, float radius, float r, float g, float b, float a) {
        MethodHandle $h = Rdraw_circle$;
        if ($h == null) Rdraw_circle$ = $h = find("Rdraw_circle", FunctionDescriptor.ofVoid(ADDRESS, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact((MemorySegment) (painter == null ? MemorySegment.NULL : painter), center_x, center_y, radius, r, g, b, a);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdraw_line$;

    /**
     * Anti-aliased line with round caps.
     */
    public static void Rdraw_line(MemorySegment painter, float x0, float y0, float x1, float y1, float thickness, float r, float g, float b, float a) {
        MethodHandle $h = Rdraw_line$;
        if ($h == null) Rdraw_line$ = $h = find("Rdraw_line", FunctionDescriptor.ofVoid(ADDRESS, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact((MemorySegment) (painter == null ? MemorySegment.NULL : painter), x0, y0, x1, y1, thickness, r, g, b, a);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdraw_curve$;

    /**
     * Anti-aliased cubic Bezier curve from (x0, y0) to (x1, y1), pulled by two control points.
     */
    public static void Rdraw_curve(MemorySegment painter, float x0, float y0, float cx0, float cy0, float cx1, float cy1, float x1, float y1, float thickness, float r, float g, float b, float a) {
        MethodHandle $h = Rdraw_curve$;
        if ($h == null) Rdraw_curve$ = $h = find("Rdraw_curve", FunctionDescriptor.ofVoid(ADDRESS, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact((MemorySegment) (painter == null ? MemorySegment.NULL : painter), x0, y0, cx0, cy0, cx1, cy1, x1, y1, thickness, r, g, b, a);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdraw_triangle$;

    public static void Rdraw_triangle(MemorySegment painter, float x0, float y0, float x1, float y1, float x2, float y2, float r, float g, float b, float a) {
        MethodHandle $h = Rdraw_triangle$;
        if ($h == null) Rdraw_triangle$ = $h = find("Rdraw_triangle", FunctionDescriptor.ofVoid(ADDRESS, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact((MemorySegment) (painter == null ? MemorySegment.NULL : painter), x0, y0, x1, y1, x2, y2, r, g, b, a);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdraw_image$;

    /**
     * Texture stretched to a rectangle and multiplied by a tint color.
     */
    public static void Rdraw_image(MemorySegment painter, int texture, float x, float y, float width, float height, float r, float g, float b, float a) {
        MethodHandle $h = Rdraw_image$;
        if ($h == null) Rdraw_image$ = $h = find("Rdraw_image", FunctionDescriptor.ofVoid(ADDRESS, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact((MemorySegment) (painter == null ? MemorySegment.NULL : painter), texture, x, y, width, height, r, g, b, a);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdraw_text$;

    /**
     * Draws UTF-8 text; (x, y) is the top-left corner of the first line.
     * font R_INVALID_ID uses the theme font.
     * size 0 uses R_METRIC_FONT_SIZE.
     */
    public static void Rdraw_text(MemorySegment painter, int font, float size, float x, float y, String text, float r, float g, float b, float a) {
        MethodHandle $h = Rdraw_text$;
        if ($h == null) Rdraw_text$ = $h = find("Rdraw_text", FunctionDescriptor.ofVoid(ADDRESS, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, ADDRESS, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            $h.invokeExact((MemorySegment) (painter == null ? MemorySegment.NULL : painter), font, size, x, y, text$, r, g, b, a);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdraw_glass$;

    /**
     * Rounded rectangle of glass. It shows what was drawn before it,
     * frosted and bent along its rim, under a tint: (r, g, b) is the color of
     * the tint and `a` its strength (0 = clear glass, 1 = opaque color).
     * Works with any theme. Needs a backend that implements glass (the Vulkan
     * backend does).
     */
    public static void Rdraw_glass(MemorySegment painter, float x, float y, float width, float height, float radius, float r, float g, float b, float a) {
        MethodHandle $h = Rdraw_glass$;
        if ($h == null) Rdraw_glass$ = $h = find("Rdraw_glass", FunctionDescriptor.ofVoid(ADDRESS, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact((MemorySegment) (painter == null ? MemorySegment.NULL : painter), x, y, width, height, radius, r, g, b, a);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdraw_spring$;

    /**
     * Animates a value from the paint callback of a custom widget.
     * Moves the spring toward `target` by the time elapsed since the last
     * frame and returns its value. While it moves, the window keeps drawing
     * frames by itself. The first call puts the spring on its target.
     *
     *     static RSpring size;                                  (zeroed)
     *     float s = Rdraw_spring(painter, &amp;size, hovered ? 1.0f : 0.0f, R_MOTION_BOUNCY);
     *     Rdraw_circle(painter, cx, cy, 20.0f + 8.0f * s, ...);
     */
    public static float Rdraw_spring(MemorySegment painter, RSpring spring, float target, int motion) {
        MethodHandle $h = Rdraw_spring$;
        if ($h == null) Rdraw_spring$ = $h = find("Rdraw_spring", FunctionDescriptor.of(JAVA_FLOAT, ADDRESS, ADDRESS, JAVA_FLOAT, JAVA_INT));
        try {
            float $r = (float) $h.invokeExact((MemorySegment) (painter == null ? MemorySegment.NULL : painter), (MemorySegment) (spring == null ? MemorySegment.NULL : spring.segment()), target, motion);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rspring_kick$;

    /**
     * Pushes a spring away from its target (units per second): it bounces back.
     */
    public static void Rspring_kick(RSpring spring, float velocity) {
        MethodHandle $h = Rspring_kick$;
        if ($h == null) Rspring_kick$ = $h = find("Rspring_kick", FunctionDescriptor.ofVoid(ADDRESS, JAVA_FLOAT));
        try {
            $h.invokeExact((MemorySegment) (spring == null ? MemorySegment.NULL : spring.segment()), velocity);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rspring_set$;

    /**
     * Puts a spring at rest on a value, without animation.
     */
    public static void Rspring_set(RSpring spring, float value) {
        MethodHandle $h = Rspring_set$;
        if ($h == null) Rspring_set$ = $h = find("Rspring_set", FunctionDescriptor.ofVoid(ADDRESS, JAVA_FLOAT));
        try {
            $h.invokeExact((MemorySegment) (spring == null ? MemorySegment.NULL : spring.segment()), value);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdraw_push_clip_rect$;

    /**
     * Restricts the following draws to a rectangle, intersected with the current one.
     */
    public static void Rdraw_push_clip_rect(MemorySegment painter, float x, float y, float width, float height) {
        MethodHandle $h = Rdraw_push_clip_rect$;
        if ($h == null) Rdraw_push_clip_rect$ = $h = find("Rdraw_push_clip_rect", FunctionDescriptor.ofVoid(ADDRESS, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT));
        try {
            $h.invokeExact((MemorySegment) (painter == null ? MemorySegment.NULL : painter), x, y, width, height);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdraw_pop_clip_rect$;

    public static void Rdraw_pop_clip_rect(MemorySegment painter) {
        MethodHandle $h = Rdraw_pop_clip_rect$;
        if ($h == null) Rdraw_pop_clip_rect$ = $h = find("Rdraw_pop_clip_rect", FunctionDescriptor.ofVoid(ADDRESS));
        try {
            $h.invokeExact((MemorySegment) (painter == null ? MemorySegment.NULL : painter));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rshader_create$;

    /**
     * A shader from SPIR-V code (a fragment shader written with
     * raven_effect.glsl, as in a .spv file): `size` bytes, copied. R_INVALID_ID
     * when the code is not SPIR-V; a backend that refuses it reports an error
     * and the effect is drawn as a rectangle.
     */
    public static int Rshader_create(byte[] spirv, int flags) {
        MethodHandle $h = Rshader_create$;
        if ($h == null) Rshader_create$ = $h = find("Rshader_create", FunctionDescriptor.of(JAVA_INT, ADDRESS, JAVA_LONG, JAVA_INT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment spirv$ = spirv == null ? MemorySegment.NULL : bytes($a, spirv);
            int $r = (int) $h.invokeExact(spirv$, spirv$.byteSize(), flags);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rshader_create_from_file$;

    /**
     * The same, from a .spv file (UTF-8 path, relative to the current folder).
     */
    public static int Rshader_create_from_file(String path, int flags) {
        MethodHandle $h = Rshader_create_from_file$;
        if ($h == null) Rshader_create_from_file$ = $h = find("Rshader_create_from_file", FunctionDescriptor.of(JAVA_INT, ADDRESS, JAVA_INT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment path$ = path == null ? MemorySegment.NULL : utf8($a, path);
            int $r = (int) $h.invokeExact(path$, flags);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rshader_destroy$;

    public static void Rshader_destroy(int shader) {
        MethodHandle $h = Rshader_destroy$;
        if ($h == null) Rshader_destroy$ = $h = find("Rshader_destroy", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(shader);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rshader_is_valid$;

    public static int Rshader_is_valid(int shader) {
        MethodHandle $h = Rshader_is_valid$;
        if ($h == null) Rshader_is_valid$ = $h = find("Rshader_is_valid", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(shader);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rshader_get_flags$;

    public static int Rshader_get_flags(int shader) {
        MethodHandle $h = Rshader_get_flags$;
        if ($h == null) Rshader_get_flags$ = $h = find("Rshader_get_flags", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(shader);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Reffect_get$;

    /**
     * A built-in effect, created the first time it is asked for.
     */
    public static int Reffect_get(int effect) {
        MethodHandle $h = Reffect_get$;
        if ($h == null) Reffect_get$ = $h = find("Reffect_get", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(effect);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Reffect_find$;

    /**
     * A built-in effect by its name ("aurora", "shimmer", "neon", "ripple", "plasma", "grain", "spotlight", "mesh"), or R_INVALID_ID.
     */
    public static int Reffect_find(String name) {
        MethodHandle $h = Reffect_find$;
        if ($h == null) Reffect_find$ = $h = find("Reffect_find", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            int $r = (int) $h.invokeExact(name$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Reffect_get_name$;

    public static String Reffect_get_name(int effect) {
        MethodHandle $h = Reffect_get_name$;
        if ($h == null) Reffect_get_name$ = $h = find("Reffect_get_name", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(effect);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Reffect_get_defaults$;

    /**
     * Its colors and parameters when nothing else is given (Rshader_look_init uses them).
     */
    public static void Reffect_get_defaults(int effect, RShaderLook look) {
        MethodHandle $h = Reffect_get_defaults$;
        if ($h == null) Reffect_get_defaults$ = $h = find("Reffect_get_defaults", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try {
            $h.invokeExact(effect, (MemorySegment) (look == null ? MemorySegment.NULL : look.segment()));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rshader_look_init$;

    /**
     * White to white, no parameter, no texture, square corners, animated when the shader is.
     */
    public static void Rshader_look_init(RShaderLook look) {
        MethodHandle $h = Rshader_look_init$;
        if ($h == null) Rshader_look_init$ = $h = find("Rshader_look_init", FunctionDescriptor.ofVoid(ADDRESS));
        try {
            $h.invokeExact((MemorySegment) (look == null ? MemorySegment.NULL : look.segment()));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rshader_look_defaults$;

    /**
     * Fills a look with the defaults of the shader when it is a built-in effect (else Rshader_look_init).
     */
    public static void Rshader_look_defaults(int shader, RShaderLook look) {
        MethodHandle $h = Rshader_look_defaults$;
        if ($h == null) Rshader_look_defaults$ = $h = find("Rshader_look_defaults", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try {
            $h.invokeExact(shader, (MemorySegment) (look == null ? MemorySegment.NULL : look.segment()));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rdraw_shader$;

    /**
     * From a paint callback: draws the effect over a rectangle of the
     * widget (logical units, from its top left corner).
     */
    public static void Rdraw_shader(MemorySegment painter, int shader, float x, float y, float width, float height, RShaderLook look) {
        MethodHandle $h = Rdraw_shader$;
        if ($h == null) Rdraw_shader$ = $h = find("Rdraw_shader", FunctionDescriptor.ofVoid(ADDRESS, JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, JAVA_FLOAT, ADDRESS));
        try {
            $h.invokeExact((MemorySegment) (painter == null ? MemorySegment.NULL : painter), shader, x, y, width, height, (MemorySegment) (look == null ? MemorySegment.NULL : look.segment()));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rborder_set_effect$;

    /**
     * The background of a border becomes an effect (its Background,
     * if any, is drawn under it; its child over it). R_INVALID_ID removes it.
     * XML: Effect="aurora" or Effect="file.spv", EffectColor, EffectColor2,
     * EffectParams="a,b,c,d", EffectAnimate, EffectBackdrop (for a file).
     */
    public static void Rborder_set_effect(int border, int shader, RShaderLook look) {
        MethodHandle $h = Rborder_set_effect$;
        if ($h == null) Rborder_set_effect$ = $h = find("Rborder_set_effect", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, ADDRESS));
        try {
            $h.invokeExact(border, shader, (MemorySegment) (look == null ? MemorySegment.NULL : look.segment()));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rborder_get_effect$;

    public static int Rborder_get_effect(int border) {
        MethodHandle $h = Rborder_get_effect$;
        if ($h == null) Rborder_get_effect$ = $h = find("Rborder_get_effect", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(border);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_create_from_xml$;

    /**
     * Creates a window from a document whose root element is &lt;Window&gt;.
     * xml  UTF-8 text. It does not need to be null-terminated.
     * size Size of the text in bytes.
     * Returns: The window, or R_INVALID_ID. Parse errors are reported with
     *         their line and column through the error callback (R_ERROR_XML).
     */
    public static int Rwindow_create_from_xml(String xml) {
        MethodHandle $h = Rwindow_create_from_xml$;
        if ($h == null) Rwindow_create_from_xml$ = $h = find("Rwindow_create_from_xml", FunctionDescriptor.of(JAVA_INT, ADDRESS, JAVA_LONG));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment xml$ = xml == null ? MemorySegment.NULL : utf8($a, xml);
            int $r = (int) $h.invokeExact(xml$, (long) (xml$.equals(MemorySegment.NULL) ? 0L : xml$.byteSize() - 1));
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwindow_create_from_xml_file$;

    public static int Rwindow_create_from_xml_file(String path) {
        MethodHandle $h = Rwindow_create_from_xml_file$;
        if ($h == null) Rwindow_create_from_xml_file$ = $h = find("Rwindow_create_from_xml_file", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment path$ = path == null ? MemorySegment.NULL : utf8($a, path);
            int $r = (int) $h.invokeExact(path$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_create_from_xml$;

    /**
     * Creates a widget tree from a document whose root element is a
     * widget. The caller owns the returned widget until it is added to a parent.
     */
    public static int Rwidget_create_from_xml(String xml) {
        MethodHandle $h = Rwidget_create_from_xml$;
        if ($h == null) Rwidget_create_from_xml$ = $h = find("Rwidget_create_from_xml", FunctionDescriptor.of(JAVA_INT, ADDRESS, JAVA_LONG));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment xml$ = xml == null ? MemorySegment.NULL : utf8($a, xml);
            int $r = (int) $h.invokeExact(xml$, (long) (xml$.equals(MemorySegment.NULL) ? 0L : xml$.byteSize() - 1));
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_create_from_xml_file$;

    public static int Rwidget_create_from_xml_file(String path) {
        MethodHandle $h = Rwidget_create_from_xml_file$;
        if ($h == null) Rwidget_create_from_xml_file$ = $h = find("Rwidget_create_from_xml_file", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment path$ = path == null ? MemorySegment.NULL : utf8($a, path);
            int $r = (int) $h.invokeExact(path$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_set_base_path$;

    /**
     * Folder used to resolve relative paths found in XML (Source="...",
     * Font="..."). The *FromXmlFile functions use the folder of the file.
     */
    public static void Rxml_set_base_path(String path) {
        MethodHandle $h = Rxml_set_base_path$;
        if ($h == null) Rxml_set_base_path$ = $h = find("Rxml_set_base_path", FunctionDescriptor.ofVoid(ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment path$ = path == null ? MemorySegment.NULL : utf8($a, path);
            $h.invokeExact(path$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_set_live_reload$;

    /**
     * Turns live reload on or off. It is on by default; the
     * environment variable RAVEN_LIVE_RELOAD=0 turns it off without
     * rebuilding. When off, files are never looked at again.
     */
    public static void Rxml_set_live_reload(int enabled) {
        MethodHandle $h = Rxml_set_live_reload$;
        if ($h == null) Rxml_set_live_reload$ = $h = find("Rxml_set_live_reload", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_get_live_reload$;

    public static int Rxml_get_live_reload() {
        MethodHandle $h = Rxml_get_live_reload$;
        if ($h == null) Rxml_get_live_reload$ = $h = find("Rxml_get_live_reload", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_set_reload_callback$;

    public static void Rxml_set_reload_callback(RXmlReloadCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rxml_set_reload_callback$;
        if ($h == null) Rxml_set_reload_callback$ = $h = find("Rxml_set_reload_callback", FunctionDescriptor.ofVoid(ADDRESS, ADDRESS));
        try {
            $h.invokeExact(upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    public static void Rxml_set_reload_callback(RXmlReloadCallback callback) {
        Rxml_set_reload_callback(callback, (MemorySegment) null);
    }

    private static MethodHandle Rxml_check_files$;

    /**
     * Looks at the files now, without waiting for the next look
     * (there are five per second), even when live reload is off.
     * Returns: The number of files found changed.
     */
    public static int Rxml_check_files() {
        MethodHandle $h = Rxml_check_files$;
        if ($h == null) Rxml_check_files$ = $h = find("Rxml_check_files", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_register_widgets$;

    /**
     * Reads the &lt;Widget Name="..."&gt; declarations of a document (its
     * root, or the children of a &lt;Widgets&gt; root) so that the tags they
     * declare can be used in the documents loaded afterwards.
     * Returns: The number of tags declared.
     */
    public static int Rxml_register_widgets(String xml) {
        MethodHandle $h = Rxml_register_widgets$;
        if ($h == null) Rxml_register_widgets$ = $h = find("Rxml_register_widgets", FunctionDescriptor.of(JAVA_INT, ADDRESS, JAVA_LONG));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment xml$ = xml == null ? MemorySegment.NULL : utf8($a, xml);
            int $r = (int) $h.invokeExact(xml$, (long) (xml$.equals(MemorySegment.NULL) ? 0L : xml$.byteSize() - 1));
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_register_widgets_from_file$;

    public static int Rxml_register_widgets_from_file(String path) {
        MethodHandle $h = Rxml_register_widgets_from_file$;
        if ($h == null) Rxml_register_widgets_from_file$ = $h = find("Rxml_register_widgets_from_file", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment path$ = path == null ? MemorySegment.NULL : utf8($a, path);
            int $r = (int) $h.invokeExact(path$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_unregister_widget$;

    public static void Rxml_unregister_widget(String name) {
        MethodHandle $h = Rxml_unregister_widget$;
        if ($h == null) Rxml_unregister_widget$ = $h = find("Rxml_unregister_widget", FunctionDescriptor.ofVoid(ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            $h.invokeExact(name$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_is_widget_registered$;

    public static int Rxml_is_widget_registered(String name) {
        MethodHandle $h = Rxml_is_widget_registered$;
        if ($h == null) Rxml_is_widget_registered$ = $h = find("Rxml_is_widget_registered", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            int $r = (int) $h.invokeExact(name$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_get_registered_widget_count$;

    /**
     * The declared tags, for tools that list them.
     */
    public static int Rxml_get_registered_widget_count() {
        MethodHandle $h = Rxml_get_registered_widget_count$;
        if ($h == null) Rxml_get_registered_widget_count$ = $h = find("Rxml_get_registered_widget_count", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_get_registered_widget_name$;

    public static String Rxml_get_registered_widget_name(int index) {
        MethodHandle $h = Rxml_get_registered_widget_name$;
        if ($h == null) Rxml_get_registered_widget_name$ = $h = find("Rxml_get_registered_widget_name", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(index);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_register_tag$;

    /**
     * Registers a tag (or replaces what it was registered with).
     * Returns: R_FALSE when the name is the one of a widget of Raven, or when too many tags are registered (64).
     */
    public static int Rxml_register_tag(String tag, RXmlTagCreateCallback create, RXmlTagAttributeCallback set_attribute, MemorySegment user_data) {
        MethodHandle $h = Rxml_register_tag$;
        if ($h == null) Rxml_register_tag$ = $h = find("Rxml_register_tag", FunctionDescriptor.of(JAVA_INT, ADDRESS, ADDRESS, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment tag$ = tag == null ? MemorySegment.NULL : utf8($a, tag);
            int $r = (int) $h.invokeExact(tag$, upcall(create), upcall(set_attribute), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Registers a tag (or replaces what it was registered with).
     * Returns: R_FALSE when the name is the one of a widget of Raven, or when too many tags are registered (64).
     */
    public static int Rxml_register_tag(String tag, RXmlTagCreateCallback create, RXmlTagAttributeCallback set_attribute) {
        return Rxml_register_tag(tag, create, set_attribute, (MemorySegment) null);
    }

    private static MethodHandle Rxml_unregister_tag$;

    public static void Rxml_unregister_tag(String tag) {
        MethodHandle $h = Rxml_unregister_tag$;
        if ($h == null) Rxml_unregister_tag$ = $h = find("Rxml_unregister_tag", FunctionDescriptor.ofVoid(ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment tag$ = tag == null ? MemorySegment.NULL : utf8($a, tag);
            $h.invokeExact(tag$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_is_tag_registered$;

    public static int Rxml_is_tag_registered(String tag) {
        MethodHandle $h = Rxml_is_tag_registered$;
        if ($h == null) Rxml_is_tag_registered$ = $h = find("Rxml_is_tag_registered", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment tag$ = tag == null ? MemorySegment.NULL : utf8($a, tag);
            int $r = (int) $h.invokeExact(tag$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_resolve_path$;

    /**
     * The path of a file named in a document: as the attributes
     * Source of Raven read it, relative to the folder of the document being
     * loaded (while it loads), else to the base path. For the attributes of
     * the tags made by code that name files.
     * Returns: The path; valid until the next call.
     */
    public static String Rxml_resolve_path(String path) {
        MethodHandle $h = Rxml_resolve_path$;
        if ($h == null) Rxml_resolve_path$ = $h = find("Rxml_resolve_path", FunctionDescriptor.of(ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment path$ = path == null ? MemorySegment.NULL : utf8($a, path);
            MemorySegment $r = (MemorySegment) $h.invokeExact(path$);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_attribute$;

    /**
     * Sets a property from text, with the names and value syntax of the
     * XML attributes. Handy for scripting and data-driven tools.
     * Returns: R_TRUE when the attribute exists for this widget and the value is valid.
     */
    public static int Rwidget_set_attribute(int widget, String name, String value) {
        MethodHandle $h = Rwidget_set_attribute$;
        if ($h == null) Rwidget_set_attribute$ = $h = find("Rwidget_set_attribute", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            MemorySegment value$ = value == null ? MemorySegment.NULL : utf8($a, value);
            int $r = (int) $h.invokeExact(widget, name$, value$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_document_create$;

    /**
     * A new document with one element.
     */
    public static int Rxml_document_create(String root_tag) {
        MethodHandle $h = Rxml_document_create$;
        if ($h == null) Rxml_document_create$ = $h = find("Rxml_document_create", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment root_tag$ = root_tag == null ? MemorySegment.NULL : utf8($a, root_tag);
            int $r = (int) $h.invokeExact(root_tag$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_document_parse$;

    /**
     * Reads a text into a document. Comments are kept.
     * error When not NULL, a mistake in the text is described there
     * and nothing is reported; when NULL it goes to the error callback.
     * Returns: The document, or R_INVALID_ID.
     */
    public static int Rxml_document_parse(String xml, RXmlError error) {
        MethodHandle $h = Rxml_document_parse$;
        if ($h == null) Rxml_document_parse$ = $h = find("Rxml_document_parse", FunctionDescriptor.of(JAVA_INT, ADDRESS, JAVA_LONG, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment xml$ = xml == null ? MemorySegment.NULL : utf8($a, xml);
            int $r = (int) $h.invokeExact(xml$, (long) (xml$.equals(MemorySegment.NULL) ? 0L : xml$.byteSize() - 1), (MemorySegment) (error == null ? MemorySegment.NULL : error.segment()));
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_document_load$;

    /**
     * Reads a file. The paths written in it are relative to its folder.
     */
    public static int Rxml_document_load(String path) {
        MethodHandle $h = Rxml_document_load$;
        if ($h == null) Rxml_document_load$ = $h = find("Rxml_document_load", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment path$ = path == null ? MemorySegment.NULL : utf8($a, path);
            int $r = (int) $h.invokeExact(path$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_document_destroy$;

    /**
     * Releases a document and its nodes. What was built from it lives on.
     */
    public static void Rxml_document_destroy(int document) {
        MethodHandle $h = Rxml_document_destroy$;
        if ($h == null) Rxml_document_destroy$ = $h = find("Rxml_document_destroy", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(document);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_document_is_valid$;

    public static int Rxml_document_is_valid(int document) {
        MethodHandle $h = Rxml_document_is_valid$;
        if ($h == null) Rxml_document_is_valid$ = $h = find("Rxml_document_is_valid", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(document);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_document_get_root$;

    public static int Rxml_document_get_root(int document) {
        MethodHandle $h = Rxml_document_get_root$;
        if ($h == null) Rxml_document_get_root$ = $h = find("Rxml_document_get_root", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(document);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_document_to_string$;

    /**
     * Writes the document as text, one element per line, indented
     * with tabs. Rxml_node_get_line then says where each node was written.
     * Returns: The text, valid until the next call for this document or its destruction.
     */
    public static String Rxml_document_to_string(int document) {
        MethodHandle $h = Rxml_document_to_string$;
        if ($h == null) Rxml_document_to_string$ = $h = find("Rxml_document_to_string", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(document);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_document_save$;

    public static int Rxml_document_save(int document, String path) {
        MethodHandle $h = Rxml_document_save$;
        if ($h == null) Rxml_document_save$ = $h = find("Rxml_document_save", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment path$ = path == null ? MemorySegment.NULL : utf8($a, path);
            int $r = (int) $h.invokeExact(document, path$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_document_create_widget$;

    /**
     * Builds the widgets the document describes, as
     * Rwidget_create_from_xml does. When the root is a &lt;Window&gt;, builds what
     * the window would show (R_INVALID_ID when it is empty).
     */
    public static int Rxml_document_create_widget(int document) {
        MethodHandle $h = Rxml_document_create_widget$;
        if ($h == null) Rxml_document_create_widget$ = $h = find("Rxml_document_create_widget", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(document);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_document_create_window$;

    /**
     * Builds the window the document describes, as Rwindow_create_from_xml does.
     */
    public static int Rxml_document_create_window(int document) {
        MethodHandle $h = Rxml_document_create_window$;
        if ($h == null) Rxml_document_create_window$ = $h = find("Rxml_document_create_window", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(document);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_is_valid$;

    public static int Rxml_node_is_valid(int node) {
        MethodHandle $h = Rxml_node_is_valid$;
        if ($h == null) Rxml_node_is_valid$ = $h = find("Rxml_node_is_valid", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(node);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_get_document$;

    public static int Rxml_node_get_document(int node) {
        MethodHandle $h = Rxml_node_get_document$;
        if ($h == null) Rxml_node_get_document$ = $h = find("Rxml_node_get_document", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(node);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_get_tag$;

    /**
     * Returns: The tag of an element, "" for a comment.
     */
    public static String Rxml_node_get_tag(int node) {
        MethodHandle $h = Rxml_node_get_tag$;
        if ($h == null) Rxml_node_get_tag$ = $h = find("Rxml_node_get_tag", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(node);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_set_tag$;

    public static void Rxml_node_set_tag(int node, String tag) {
        MethodHandle $h = Rxml_node_set_tag$;
        if ($h == null) Rxml_node_set_tag$ = $h = find("Rxml_node_set_tag", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment tag$ = tag == null ? MemorySegment.NULL : utf8($a, tag);
            $h.invokeExact(node, tag$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_is_comment$;

    public static int Rxml_node_is_comment(int node) {
        MethodHandle $h = Rxml_node_is_comment$;
        if ($h == null) Rxml_node_is_comment$ = $h = find("Rxml_node_is_comment", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(node);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_get_text$;

    /**
     * The text inside an element (&lt;Label&gt;this&lt;/Label&gt;), or the text of a comment.
     */
    public static String Rxml_node_get_text(int node) {
        MethodHandle $h = Rxml_node_get_text$;
        if ($h == null) Rxml_node_get_text$ = $h = find("Rxml_node_get_text", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(node);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_set_text$;

    public static void Rxml_node_set_text(int node, String text) {
        MethodHandle $h = Rxml_node_set_text$;
        if ($h == null) Rxml_node_set_text$ = $h = find("Rxml_node_set_text", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            $h.invokeExact(node, text$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_get_parent$;

    public static int Rxml_node_get_parent(int node) {
        MethodHandle $h = Rxml_node_get_parent$;
        if ($h == null) Rxml_node_get_parent$ = $h = find("Rxml_node_get_parent", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(node);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_get_child_count$;

    public static int Rxml_node_get_child_count(int node) {
        MethodHandle $h = Rxml_node_get_child_count$;
        if ($h == null) Rxml_node_get_child_count$ = $h = find("Rxml_node_get_child_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(node);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_get_child$;

    public static int Rxml_node_get_child(int node, int index) {
        MethodHandle $h = Rxml_node_get_child$;
        if ($h == null) Rxml_node_get_child$ = $h = find("Rxml_node_get_child", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(node, index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_get_index$;

    /**
     * Returns: The place of a node among the children of its parent; 0 for the root.
     */
    public static int Rxml_node_get_index(int node) {
        MethodHandle $h = Rxml_node_get_index$;
        if ($h == null) Rxml_node_get_index$ = $h = find("Rxml_node_get_index", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(node);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_add_child$;

    /**
     * index Where among the children; -1 for the end.
     */
    public static int Rxml_node_add_child(int parent, String tag, int index) {
        MethodHandle $h = Rxml_node_add_child$;
        if ($h == null) Rxml_node_add_child$ = $h = find("Rxml_node_add_child", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS, JAVA_INT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment tag$ = tag == null ? MemorySegment.NULL : utf8($a, tag);
            int $r = (int) $h.invokeExact(parent, tag$, index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_add_comment$;

    public static int Rxml_node_add_comment(int parent, String text, int index) {
        MethodHandle $h = Rxml_node_add_comment$;
        if ($h == null) Rxml_node_add_comment$ = $h = find("Rxml_node_add_comment", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS, JAVA_INT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            int $r = (int) $h.invokeExact(parent, text$, index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_add_xml$;

    /**
     * Reads a piece of XML (one element and what it contains) and
     * adds it: what a palette, a paste or a template inserts.
     */
    public static int Rxml_node_add_xml(int parent, String xml, int index) {
        MethodHandle $h = Rxml_node_add_xml$;
        if ($h == null) Rxml_node_add_xml$ = $h = find("Rxml_node_add_xml", FunctionDescriptor.of(JAVA_INT, JAVA_INT, ADDRESS, JAVA_LONG, JAVA_INT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment xml$ = xml == null ? MemorySegment.NULL : utf8($a, xml);
            int $r = (int) $h.invokeExact(parent, xml$, (long) (xml$.equals(MemorySegment.NULL) ? 0L : xml$.byteSize() - 1), index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_remove$;

    /**
     * Removes a node and what it contains. The root stays.
     */
    public static void Rxml_node_remove(int node) {
        MethodHandle $h = Rxml_node_remove$;
        if ($h == null) Rxml_node_remove$ = $h = find("Rxml_node_remove", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(node);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_move$;

    /**
     * Moves a node, with what it contains, under another parent of
     * the same document or elsewhere under its own.
     * index Where among the children of the new parent, counted once the node has left its place; -1 for the end.
     * Returns: R_FALSE when it cannot be done (the root, or a parent that is inside the node).
     */
    public static int Rxml_node_move(int node, int new_parent, int index) {
        MethodHandle $h = Rxml_node_move$;
        if ($h == null) Rxml_node_move$ = $h = find("Rxml_node_move", FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(node, new_parent, index);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_duplicate$;

    /**
     * Copies a node and what it contains, right after it. Returns the copy.
     */
    public static int Rxml_node_duplicate(int node) {
        MethodHandle $h = Rxml_node_duplicate$;
        if ($h == null) Rxml_node_duplicate$ = $h = find("Rxml_node_duplicate", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(node);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_to_string$;

    /**
     * The node as text. Valid until the next call for this document.
     */
    public static String Rxml_node_to_string(int node) {
        MethodHandle $h = Rxml_node_to_string$;
        if ($h == null) Rxml_node_to_string$ = $h = find("Rxml_node_to_string", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(node);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_get_attribute_count$;

    public static int Rxml_node_get_attribute_count(int node) {
        MethodHandle $h = Rxml_node_get_attribute_count$;
        if ($h == null) Rxml_node_get_attribute_count$ = $h = find("Rxml_node_get_attribute_count", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(node);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_get_attribute_name$;

    public static String Rxml_node_get_attribute_name(int node, int index) {
        MethodHandle $h = Rxml_node_get_attribute_name$;
        if ($h == null) Rxml_node_get_attribute_name$ = $h = find("Rxml_node_get_attribute_name", FunctionDescriptor.of(ADDRESS, JAVA_INT, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(node, index);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_get_attribute_value$;

    public static String Rxml_node_get_attribute_value(int node, int index) {
        MethodHandle $h = Rxml_node_get_attribute_value$;
        if ($h == null) Rxml_node_get_attribute_value$ = $h = find("Rxml_node_get_attribute_value", FunctionDescriptor.of(ADDRESS, JAVA_INT, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(node, index);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_get_attribute$;

    /**
     * Returns: The value of an attribute (names are compared without case), or NULL when the node does not have it.
     */
    public static String Rxml_node_get_attribute(int node, String name) {
        MethodHandle $h = Rxml_node_get_attribute$;
        if ($h == null) Rxml_node_get_attribute$ = $h = find("Rxml_node_get_attribute", FunctionDescriptor.of(ADDRESS, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            MemorySegment $r = (MemorySegment) $h.invokeExact(node, name$);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_set_attribute$;

    /**
     * Sets an attribute, added after the others when it is new. A NULL value removes it.
     */
    public static void Rxml_node_set_attribute(int node, String name, String value) {
        MethodHandle $h = Rxml_node_set_attribute$;
        if ($h == null) Rxml_node_set_attribute$ = $h = find("Rxml_node_set_attribute", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            MemorySegment value$ = value == null ? MemorySegment.NULL : utf8($a, value);
            $h.invokeExact(node, name$, value$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_get_line$;

    /**
     * Returns: The line of the node, starting at 1, in the text the document
     * was read from or last written to; 0 for a node added since.
     */
    public static int Rxml_node_get_line(int node) {
        MethodHandle $h = Rxml_node_get_line$;
        if ($h == null) Rxml_node_get_line$ = $h = find("Rxml_node_get_line", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(node);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_node_get_widget$;

    /**
     * Returns: The widget (the window, for a &lt;Window&gt;) the node became the
     * last time the document was built, or R_INVALID_ID: never built,
     * destroyed since, or not a widget (an &lt;Item&gt;, a comment). A tag declared
     * with &lt;Widget&gt; gives the widget it stands for.
     */
    public static int Rxml_node_get_widget(int node) {
        MethodHandle $h = Rxml_node_get_widget$;
        if ($h == null) Rxml_node_get_widget$ = $h = find("Rxml_node_get_widget", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(node);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_xml_node$;

    /**
     * Returns: The node a widget was built from, or R_INVALID_ID: not built
     * from a document, or the document is gone. The widgets inside a tag
     * declared with &lt;Widget&gt; all come from the node where the tag is used.
     */
    public static int Rwidget_get_xml_node(int widget) {
        MethodHandle $h = Rwidget_get_xml_node$;
        if ($h == null) Rwidget_get_xml_node$ = $h = find("Rwidget_get_xml_node", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(widget);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_get_tag_count$;

    /**
     * The tags of the library, &lt;Window&gt; included. Those declared with &lt;Widget&gt; are listed by Rxml_get_registered_widget_name.
     */
    public static int Rxml_get_tag_count() {
        MethodHandle $h = Rxml_get_tag_count$;
        if ($h == null) Rxml_get_tag_count$ = $h = find("Rxml_get_tag_count", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_get_tag_name$;

    public static String Rxml_get_tag_name(int index) {
        MethodHandle $h = Rxml_get_tag_name$;
        if ($h == null) Rxml_get_tag_name$ = $h = find("Rxml_get_tag_name", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(index);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_get_tag_description$;

    /**
     * Returns: One line about a tag, "" when there is nothing to say.
     */
    public static String Rxml_get_tag_description(String tag) {
        MethodHandle $h = Rxml_get_tag_description$;
        if ($h == null) Rxml_get_tag_description$ = $h = find("Rxml_get_tag_description", FunctionDescriptor.of(ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment tag$ = tag == null ? MemorySegment.NULL : utf8($a, tag);
            MemorySegment $r = (MemorySegment) $h.invokeExact(tag$);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_get_tag_children$;

    /**
     * Returns: 0: no child widget; 1: a single one; 2: any number. Declared and unknown tags give 2.
     */
    public static int Rxml_get_tag_children(String tag) {
        MethodHandle $h = Rxml_get_tag_children$;
        if ($h == null) Rxml_get_tag_children$ = $h = find("Rxml_get_tag_children", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment tag$ = tag == null ? MemorySegment.NULL : utf8($a, tag);
            int $r = (int) $h.invokeExact(tag$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_get_attribute_count$;

    /**
     * The attributes of a tag: its own first, then those every widget
     * has, then those of its place in its parent. For a tag declared with
     * &lt;Widget&gt;, its own are its parameters.
     */
    public static int Rxml_get_attribute_count(String tag) {
        MethodHandle $h = Rxml_get_attribute_count$;
        if ($h == null) Rxml_get_attribute_count$ = $h = find("Rxml_get_attribute_count", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment tag$ = tag == null ? MemorySegment.NULL : utf8($a, tag);
            int $r = (int) $h.invokeExact(tag$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rxml_get_attribute_info$;

    public static int Rxml_get_attribute_info(String tag, int index, RXmlAttributeInfo info) {
        MethodHandle $h = Rxml_get_attribute_info$;
        if ($h == null) Rxml_get_attribute_info$ = $h = find("Rxml_get_attribute_info", FunctionDescriptor.of(JAVA_INT, ADDRESS, JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment tag$ = tag == null ? MemorySegment.NULL : utf8($a, tag);
            int $r = (int) $h.invokeExact(tag$, index, (MemorySegment) (info == null ? MemorySegment.NULL : info.segment()));
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlocale_load_csv$;

    /**
     * Loads a CSV table of translations from a file. Returns the number of keys read, -1 on an error.
     */
    public static int Rlocale_load_csv(String path) {
        MethodHandle $h = Rlocale_load_csv$;
        if ($h == null) Rlocale_load_csv$ = $h = find("Rlocale_load_csv", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment path$ = path == null ? MemorySegment.NULL : utf8($a, path);
            int $r = (int) $h.invokeExact(path$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlocale_load_csv_from_memory$;

    /**
     * Same, from text in memory (UTF-8).
     */
    public static int Rlocale_load_csv_from_memory(String csv) {
        MethodHandle $h = Rlocale_load_csv_from_memory$;
        if ($h == null) Rlocale_load_csv_from_memory$ = $h = find("Rlocale_load_csv_from_memory", FunctionDescriptor.of(JAVA_INT, ADDRESS, JAVA_LONG));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment csv$ = csv == null ? MemorySegment.NULL : utf8($a, csv);
            int $r = (int) $h.invokeExact(csv$, (long) (csv$.equals(MemorySegment.NULL) ? 0L : csv$.byteSize() - 1));
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlocale_clear$;

    /**
     * Forgets every table.
     */
    public static void Rlocale_clear() {
        MethodHandle $h = Rlocale_clear$;
        if ($h == null) Rlocale_clear$ = $h = find("Rlocale_clear", FunctionDescriptor.ofVoid());
        try {
            $h.invokeExact();
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlocale_set_text$;

    /**
     * Adds or replaces one text of one language.
     */
    public static void Rlocale_set_text(String language, String key, String text) {
        MethodHandle $h = Rlocale_set_text$;
        if ($h == null) Rlocale_set_text$ = $h = find("Rlocale_set_text", FunctionDescriptor.ofVoid(ADDRESS, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment language$ = language == null ? MemorySegment.NULL : utf8($a, language);
            MemorySegment key$ = key == null ? MemorySegment.NULL : utf8($a, key);
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            $h.invokeExact(language$, key$, text$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlocale_get_language_count$;

    /**
     * Number of languages in the tables, and their codes, in the order of the columns.
     */
    public static int Rlocale_get_language_count() {
        MethodHandle $h = Rlocale_get_language_count$;
        if ($h == null) Rlocale_get_language_count$ = $h = find("Rlocale_get_language_count", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlocale_get_language_at$;

    public static String Rlocale_get_language_at(int index) {
        MethodHandle $h = Rlocale_get_language_at$;
        if ($h == null) Rlocale_get_language_at$ = $h = find("Rlocale_get_language_at", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(index);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlocale_set_language$;

    /**
     * Chooses the language: an exact code ("pt-BR"), or its first part
     * ("pt") when the tables have no such column. Every widget that shows a key
     * is updated, and the layout of the windows follows its direction (an
     * Arabic, Hebrew, Persian or Urdu language is laid out from right to left,
     * see Rset_layout_direction). Returns R_FALSE when no column matches (the
     * language is set all the same: the texts fall back).
     */
    public static int Rlocale_set_language(String code) {
        MethodHandle $h = Rlocale_set_language$;
        if ($h == null) Rlocale_set_language$ = $h = find("Rlocale_set_language", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment code$ = code == null ? MemorySegment.NULL : utf8($a, code);
            int $r = (int) $h.invokeExact(code$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlocale_get_language$;

    public static String Rlocale_get_language() {
        MethodHandle $h = Rlocale_get_language$;
        if ($h == null) Rlocale_get_language$ = $h = find("Rlocale_get_language", FunctionDescriptor.of(ADDRESS));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact();
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlocale_get_system_language$;

    /**
     * The language of the user, from the system ("fr-FR", "en-US"...).
     */
    public static String Rlocale_get_system_language() {
        MethodHandle $h = Rlocale_get_system_language$;
        if ($h == null) Rlocale_get_system_language$ = $h = find("Rlocale_get_system_language", FunctionDescriptor.of(ADDRESS));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact();
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlocale_set_fallback_language$;

    /**
     * The language whose texts are shown when the current one has none (default: the first column).
     */
    public static void Rlocale_set_fallback_language(String code) {
        MethodHandle $h = Rlocale_set_fallback_language$;
        if ($h == null) Rlocale_set_fallback_language$ = $h = find("Rlocale_set_fallback_language", FunctionDescriptor.ofVoid(ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment code$ = code == null ? MemorySegment.NULL : utf8($a, code);
            $h.invokeExact(code$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlocale_is_rtl$;

    /**
     * R_TRUE when the language is written from right to left.
     */
    public static int Rlocale_is_rtl() {
        MethodHandle $h = Rlocale_is_rtl$;
        if ($h == null) Rlocale_is_rtl$ = $h = find("Rlocale_is_rtl", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlocale_set_changed_callback$;

    /**
     * Called after the language or the tables changed (the widgets are already updated).
     */
    public static void Rlocale_set_changed_callback(RLocaleChangedCallback callback, MemorySegment user_data) {
        MethodHandle $h = Rlocale_set_changed_callback$;
        if ($h == null) Rlocale_set_changed_callback$ = $h = find("Rlocale_set_changed_callback", FunctionDescriptor.ofVoid(ADDRESS, ADDRESS));
        try {
            $h.invokeExact(upcall(callback), (MemorySegment) (user_data == null ? MemorySegment.NULL : user_data));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    /**
     * Called after the language or the tables changed (the widgets are already updated).
     */
    public static void Rlocale_set_changed_callback(RLocaleChangedCallback callback) {
        Rlocale_set_changed_callback(callback, (MemorySegment) null);
    }

    private static MethodHandle Rtr$;

    /**
     * The text of a key in the current language (see the fallbacks above). Never NULL.
     */
    public static String Rtr(String key) {
        MethodHandle $h = Rtr$;
        if ($h == null) Rtr$ = $h = find("Rtr", FunctionDescriptor.of(ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment key$ = key == null ? MemorySegment.NULL : utf8($a, key);
            MemorySegment $r = (MemorySegment) $h.invokeExact(key$);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtr_exists$;

    /**
     * R_TRUE when the key has a text in the current language or the fallback one.
     */
    public static int Rtr_exists(String key) {
        MethodHandle $h = Rtr_exists$;
        if ($h == null) Rtr_exists$ = $h = find("Rtr_exists", FunctionDescriptor.of(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment key$ = key == null ? MemorySegment.NULL : utf8($a, key);
            int $r = (int) $h.invokeExact(key$);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtr_format$;

    /**
     * The text of a key with its {0} {1} ... replaced by the arguments
     * (translators may put them in any order). "{{" is a brace.
     */
    public static String Rtr_format(String key, String[] args) {
        MethodHandle $h = Rtr_format$;
        if ($h == null) Rtr_format$ = $h = find("Rtr_format", FunctionDescriptor.of(ADDRESS, ADDRESS, ADDRESS, JAVA_INT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment key$ = key == null ? MemorySegment.NULL : utf8($a, key);
            MemorySegment args$ = texts($a, args);
            MemorySegment $r = (MemorySegment) $h.invokeExact(key$, args$, (int) (args$.byteSize() / ADDRESS.byteSize()));
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rtr_plural$;

    /**
     * The plural form of a key for a count: the key followed by the CLDR
     * category of the count in the current language (".zero", ".one", ".two",
     * ".few", ".many", ".other"), else ".other", else the key itself. {n} is
     * replaced by the count, written in the conventions of the language.
     */
    public static String Rtr_plural(String key, int count) {
        MethodHandle $h = Rtr_plural$;
        if ($h == null) Rtr_plural$ = $h = find("Rtr_plural", FunctionDescriptor.of(ADDRESS, ADDRESS, JAVA_INT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment key$ = key == null ? MemorySegment.NULL : utf8($a, key);
            MemorySegment $r = (MemorySegment) $h.invokeExact(key$, count);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_text_key$;

    /**
     * Makes a widget show a key: an attribute ("Text", "Tooltip",
     * "Title", "Placeholder"... any attribute that takes a text) is set to the
     * translation now and every time the language changes. NULL or "" stops it.
     * Same as Rwidget_set_attribute(widget, attribute, "&#64;key").
     */
    public static void Rwidget_set_text_key(int widget, String attribute, String key) {
        MethodHandle $h = Rwidget_set_text_key$;
        if ($h == null) Rwidget_set_text_key$ = $h = find("Rwidget_set_text_key", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment attribute$ = attribute == null ? MemorySegment.NULL : utf8($a, attribute);
            MemorySegment key$ = key == null ? MemorySegment.NULL : utf8($a, key);
            $h.invokeExact(widget, attribute$, key$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rlocale_format_number$;

    /**
     * A number written in the conventions of the language: decimal separator and digit grouping ("1 234,5" in French).
     */
    public static String Rlocale_format_number(double value, int decimals) {
        MethodHandle $h = Rlocale_format_number$;
        if ($h == null) Rlocale_format_number$ = $h = find("Rlocale_format_number", FunctionDescriptor.of(ADDRESS, JAVA_DOUBLE, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(value, decimals);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Raccessibility_set_text_scale$;

    /**
     * Size of every text, 1 = as designed (0.5 to 4). Controls grow with their text.
     */
    public static void Raccessibility_set_text_scale(float scale) {
        MethodHandle $h = Raccessibility_set_text_scale$;
        if ($h == null) Raccessibility_set_text_scale$ = $h = find("Raccessibility_set_text_scale", FunctionDescriptor.ofVoid(JAVA_FLOAT));
        try {
            $h.invokeExact(scale);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Raccessibility_get_text_scale$;

    public static float Raccessibility_get_text_scale() {
        MethodHandle $h = Raccessibility_get_text_scale$;
        if ($h == null) Raccessibility_get_text_scale$ = $h = find("Raccessibility_get_text_scale", FunctionDescriptor.of(JAVA_FLOAT));
        try {
            float $r = (float) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Raccessibility_set_high_contrast$;

    /**
     * Themes of high contrast: black and white, yellow focus, thick
     * borders (the dark one, or the light one when the theme in use is light).
     * R_FALSE goes back to the theme used before.
     */
    public static void Raccessibility_set_high_contrast(int enabled) {
        MethodHandle $h = Raccessibility_set_high_contrast$;
        if ($h == null) Raccessibility_set_high_contrast$ = $h = find("Raccessibility_set_high_contrast", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Raccessibility_get_high_contrast$;

    public static int Raccessibility_get_high_contrast() {
        MethodHandle $h = Raccessibility_get_high_contrast$;
        if ($h == null) Raccessibility_get_high_contrast$ = $h = find("Raccessibility_get_high_contrast", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Raccessibility_set_focus_always_visible$;

    /**
     * R_TRUE: the focus ring is always shown (not only after the keyboard was used), and thicker.
     */
    public static void Raccessibility_set_focus_always_visible(int enabled) {
        MethodHandle $h = Raccessibility_set_focus_always_visible$;
        if ($h == null) Raccessibility_set_focus_always_visible$ = $h = find("Raccessibility_set_focus_always_visible", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Raccessibility_get_focus_always_visible$;

    public static int Raccessibility_get_focus_always_visible() {
        MethodHandle $h = Raccessibility_get_focus_always_visible$;
        if ($h == null) Raccessibility_get_focus_always_visible$ = $h = find("Raccessibility_get_focus_always_visible", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Raccessibility_set_reduce_motion$;

    /**
     * Reduced motion, for the whole application: R_TRUE and nothing
     * moves by itself. Changes are immediate (no spring, no slide, no bounce,
     * no transition), and the effects (raven_shader.h) stand still; they still
     * follow the pointer. Same as Rset_animations_enabled(R_FALSE).
     */
    public static void Raccessibility_set_reduce_motion(int enabled) {
        MethodHandle $h = Raccessibility_set_reduce_motion$;
        if ($h == null) Raccessibility_set_reduce_motion$ = $h = find("Raccessibility_set_reduce_motion", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(enabled);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Raccessibility_get_reduce_motion$;

    public static int Raccessibility_get_reduce_motion() {
        MethodHandle $h = Raccessibility_get_reduce_motion$;
        if ($h == null) Raccessibility_get_reduce_motion$ = $h = find("Raccessibility_get_reduce_motion", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Raccessibility_set_color_filter$;

    /**
     * A filter on the colors of the interface (pictures and viewports keep theirs).
     */
    public static void Raccessibility_set_color_filter(int filter) {
        MethodHandle $h = Raccessibility_set_color_filter$;
        if ($h == null) Raccessibility_set_color_filter$ = $h = find("Raccessibility_set_color_filter", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(filter);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Raccessibility_get_color_filter$;

    public static int Raccessibility_get_color_filter() {
        MethodHandle $h = Raccessibility_get_color_filter$;
        if ($h == null) Raccessibility_get_color_filter$ = $h = find("Raccessibility_get_color_filter", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Raccessibility_get_system_settings$;

    public static void Raccessibility_get_system_settings(RAccessibilitySettings settings) {
        MethodHandle $h = Raccessibility_get_system_settings$;
        if ($h == null) Raccessibility_get_system_settings$ = $h = find("Raccessibility_get_system_settings", FunctionDescriptor.ofVoid(ADDRESS));
        try {
            $h.invokeExact((MemorySegment) (settings == null ? MemorySegment.NULL : settings.segment()));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Raccessibility_follow_system$;

    /**
     * Applies the settings of the system (text size, contrast, motion).
     */
    public static void Raccessibility_follow_system() {
        MethodHandle $h = Raccessibility_follow_system$;
        if ($h == null) Raccessibility_follow_system$ = $h = find("Raccessibility_follow_system", FunctionDescriptor.ofVoid());
        try {
            $h.invokeExact();
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_accessible_name$;

    /**
     * What the screen reader says of the widget; NULL: what it shows (or the label before it).
     */
    public static void Rwidget_set_accessible_name(int widget, String name) {
        MethodHandle $h = Rwidget_set_accessible_name$;
        if ($h == null) Rwidget_set_accessible_name$ = $h = find("Rwidget_set_accessible_name", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment name$ = name == null ? MemorySegment.NULL : utf8($a, name);
            $h.invokeExact(widget, name$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_accessible_name$;

    /**
     * The name the screen reader uses (given, or found). Valid until the next call.
     */
    public static String Rwidget_get_accessible_name(int widget) {
        MethodHandle $h = Rwidget_get_accessible_name$;
        if ($h == null) Rwidget_get_accessible_name$ = $h = find("Rwidget_get_accessible_name", FunctionDescriptor.of(ADDRESS, JAVA_INT));
        try {
            MemorySegment $r = (MemorySegment) $h.invokeExact(widget);
            return text($r);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_accessible_description$;

    /**
     * A longer explanation, read after the name.
     */
    public static void Rwidget_set_accessible_description(int widget, String description) {
        MethodHandle $h = Rwidget_set_accessible_description$;
        if ($h == null) Rwidget_set_accessible_description$ = $h = find("Rwidget_set_accessible_description", FunctionDescriptor.ofVoid(JAVA_INT, ADDRESS));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment description$ = description == null ? MemorySegment.NULL : utf8($a, description);
            $h.invokeExact(widget, description$);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_accessible_role$;

    public static void Rwidget_set_accessible_role(int widget, int role) {
        MethodHandle $h = Rwidget_set_accessible_role$;
        if ($h == null) Rwidget_set_accessible_role$ = $h = find("Rwidget_set_accessible_role", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(widget, role);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_get_accessible_role$;

    /**
     * The role the screen reader sees (given, or the one of the type).
     */
    public static int Rwidget_get_accessible_role(int widget) {
        MethodHandle $h = Rwidget_get_accessible_role$;
        if ($h == null) Rwidget_get_accessible_role$ = $h = find("Rwidget_get_accessible_role", FunctionDescriptor.of(JAVA_INT, JAVA_INT));
        try {
            int $r = (int) $h.invokeExact(widget);
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_accessible_hidden$;

    /**
     * R_TRUE: hidden from screen readers, with what it contains (decoration).
     */
    public static void Rwidget_set_accessible_hidden(int widget, int hidden) {
        MethodHandle $h = Rwidget_set_accessible_hidden$;
        if ($h == null) Rwidget_set_accessible_hidden$ = $h = find("Rwidget_set_accessible_hidden", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(widget, hidden);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rwidget_set_accessible_labelled_by$;

    /**
     * The widget is named by another one (a label): its text is the name.
     */
    public static void Rwidget_set_accessible_labelled_by(int widget, int label) {
        MethodHandle $h = Rwidget_set_accessible_labelled_by$;
        if ($h == null) Rwidget_set_accessible_labelled_by$ = $h = find("Rwidget_set_accessible_labelled_by", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(widget, label);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Raccessibility_announce$;

    /**
     * Has a message read (a save that ended, an error). `assertive`: interrupts what is being read.
     */
    public static void Raccessibility_announce(String text, int assertive) {
        MethodHandle $h = Raccessibility_announce$;
        if ($h == null) Raccessibility_announce$ = $h = find("Raccessibility_announce", FunctionDescriptor.ofVoid(ADDRESS, JAVA_INT));
        try (Arena $a = Arena.ofConfined()) {
            MemorySegment text$ = text == null ? MemorySegment.NULL : utf8($a, text);
            $h.invokeExact(text$, assertive);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Raccessibility_is_screen_reader_active$;

    /**
     * R_TRUE when a screen reader is reading the windows of the application.
     */
    public static int Raccessibility_is_screen_reader_active() {
        MethodHandle $h = Raccessibility_is_screen_reader_active$;
        if ($h == null) Raccessibility_is_screen_reader_active$ = $h = find("Raccessibility_is_screen_reader_active", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rhost_platform_register$;

    /**
     * Makes the next Rinit use the host platform. The structure is copied; NULL restores the default.
     */
    public static void Rhost_platform_register(RHostPlatformDesc desc) {
        MethodHandle $h = Rhost_platform_register$;
        if ($h == null) Rhost_platform_register$ = $h = find("Rhost_platform_register", FunctionDescriptor.ofVoid(ADDRESS));
        try {
            $h.invokeExact((MemorySegment) (desc == null ? MemorySegment.NULL : desc.segment()));
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rhost_platform_is_active$;

    /**
     * Returns: R_TRUE while the running library uses the host platform.
     */
    public static int Rhost_platform_is_active() {
        MethodHandle $h = Rhost_platform_is_active$;
        if ($h == null) Rhost_platform_is_active$ = $h = find("Rhost_platform_is_active", FunctionDescriptor.of(JAVA_INT));
        try {
            int $r = (int) $h.invokeExact();
            return $r;
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rhost_window_set_size$;

    /**
     * Size of a host window in pixels, and pixels per logical unit.
     */
    public static void Rhost_window_set_size(int window, int width, int height, float scale) {
        MethodHandle $h = Rhost_window_set_size$;
        if ($h == null) Rhost_window_set_size$ = $h = find("Rhost_window_set_size", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_FLOAT));
        try {
            $h.invokeExact(window, width, height, scale);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rhost_inject_mouse_move$;

    public static void Rhost_inject_mouse_move(int window, int x, int y, int modifiers) {
        MethodHandle $h = Rhost_inject_mouse_move$;
        if ($h == null) Rhost_inject_mouse_move$ = $h = find("Rhost_inject_mouse_move", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(window, x, y, modifiers);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rhost_inject_mouse_button$;

    public static void Rhost_inject_mouse_button(int window, int x, int y, int button, int pressed, int modifiers) {
        MethodHandle $h = Rhost_inject_mouse_button$;
        if ($h == null) Rhost_inject_mouse_button$ = $h = find("Rhost_inject_mouse_button", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(window, x, y, button, pressed, modifiers);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rhost_inject_mouse_wheel$;

    public static void Rhost_inject_mouse_wheel(int window, float wheel_x, float wheel_y, int modifiers) {
        MethodHandle $h = Rhost_inject_mouse_wheel$;
        if ($h == null) Rhost_inject_mouse_wheel$ = $h = find("Rhost_inject_mouse_wheel", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_FLOAT, JAVA_FLOAT, JAVA_INT));
        try {
            $h.invokeExact(window, wheel_x, wheel_y, modifiers);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rhost_inject_mouse_leave$;

    /**
     * The pointer left the window: hover states end.
     */
    public static void Rhost_inject_mouse_leave(int window) {
        MethodHandle $h = Rhost_inject_mouse_leave$;
        if ($h == null) Rhost_inject_mouse_leave$ = $h = find("Rhost_inject_mouse_leave", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(window);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rhost_inject_key$;

    /**
     * repeat R_TRUE for a key held down (auto-repeat).
     */
    public static void Rhost_inject_key(int window, int key, int pressed, int modifiers, int repeat) {
        MethodHandle $h = Rhost_inject_key$;
        if ($h == null) Rhost_inject_key$ = $h = find("Rhost_inject_key", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(window, key, pressed, modifiers, repeat);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rhost_inject_text$;

    /**
     * One typed Unicode code point.
     */
    public static void Rhost_inject_text(int window, int codepoint) {
        MethodHandle $h = Rhost_inject_text$;
        if ($h == null) Rhost_inject_text$ = $h = find("Rhost_inject_text", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(window, codepoint);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rhost_inject_focus$;

    /**
     * The window gained or lost the keyboard. Losing it closes popups and drops captures.
     */
    public static void Rhost_inject_focus(int window, int focused) {
        MethodHandle $h = Rhost_inject_focus$;
        if ($h == null) Rhost_inject_focus$ = $h = find("Rhost_inject_focus", FunctionDescriptor.ofVoid(JAVA_INT, JAVA_INT));
        try {
            $h.invokeExact(window, focused);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }

    private static MethodHandle Rhost_inject_close$;

    /**
     * The user asked to close the window (its close callback decides).
     */
    public static void Rhost_inject_close(int window) {
        MethodHandle $h = Rhost_inject_close$;
        if ($h == null) Rhost_inject_close$ = $h = find("Rhost_inject_close", FunctionDescriptor.ofVoid(JAVA_INT));
        try {
            $h.invokeExact(window);
        } catch (Throwable $t) {
            throw fail($t);
        }
    }
}

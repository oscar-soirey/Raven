"""Haskell: a cabal package. Raven.C is the C API through the FFI; Raven adds helpers for texts and closures."""

from cparse import classify, doc_lines

# (Haskell type, size in bytes) of the scalars of the headers
SCALAR = {"int": ("CInt", 4), "float": ("CFloat", 4), "double": ("CDouble", 8), "char": ("CChar", 1),
          "size_t": ("CSize", 8), "uint64": ("Word64", 8), "uintptr": ("WordPtr", 8), "uint32": ("Word32", 4),
          "uint8": ("Word8", 1), "id": ("Rid", 4)}


def hs_name(name):
    """A C function as a Haskell one: the first letter lowered (Rwindow_create -> rwindow_create)."""
    return name[0].lower() + name[1:]


def hs_type(api, ctype):
    kind, detail = classify(ctype, api)
    if kind == "void":
        return "()"
    if kind == "scalar":
        return SCALAR[detail][0]
    if kind in ("enum", "callback"):
        return detail
    if kind == "fnptr":
        return "FunPtr ()"
    if kind in ("cstr", "owned_str"):
        return "CString"
    if kind in ("cstr_out", "cstr_array"):
        return "Ptr CString"
    if kind in ("ptr", "cptr"):
        return "Ptr ()"
    if kind in ("bytes", "cbytes"):
        return "Ptr Word8"
    if kind == "out":
        return "Ptr %s" % (SCALAR[detail[1]][0] if detail[0] == "scalar" else detail[1])
    if kind == "carray":
        return "Ptr %s" % SCALAR[detail][0]
    if kind in ("struct_ptr", "cstruct_ptr", "opaque_ptr"):
        return "Ptr %s" % detail
    raise ValueError("no Haskell type for %s" % ctype)


def field_size(api, ctype):
    kind, detail = classify(ctype, api) if ctype != "fnptr" else ("fnptr", None)
    if kind == "scalar":
        return SCALAR[detail][1]
    if kind == "enum":
        return 4
    return 8


def layout(api, s):
    """[(field, offset)], size, alignment: the rules of C for a 64-bit target (as gen_java and gen_bun)."""
    offset = 0
    largest = 1
    fields = []
    for f in s.fields:
        size = field_size(api, f.ctype)
        offset = (offset + size - 1) // size * size
        fields.append((f, offset))
        offset += size * (f.array or 1)
        largest = max(largest, size)
    return fields, (offset + largest - 1) // largest * largest, largest


def field_name(s, f):
    """A field of a structure: the structure, its first letter lowered, then the name of C (rXmlError_line)."""
    return "%s_%s" % (hs_name(s.name), f.name)


def field_type(api, f):
    t = "FunPtr ()" if f.ctype == "fnptr" else hs_type(api, f.ctype)
    return "[%s]" % t if f.array else t


def zero_value(api, f):
    if f.array:
        return "replicate %d 0" % f.array
    if f.ctype == "fnptr":
        return "nullFunPtr"
    kind, _ = classify(f.ctype, api)
    if kind in ("scalar", "enum"):
        return "0"
    return "nullPtr"


def haddock(out, doc, indent="", first="|"):
    lines = doc_lines(doc)
    for i, line in enumerate(lines):
        mark = "-- %s " % first if i == 0 else "-- "
        out.append((indent + mark + line).rstrip())


def signature(api, params, ret):
    types = [hs_type(api, p.ctype) for p in params] + ["IO %s" % wrap(hs_type(api, ret))]
    return " -> ".join(types)


def wrap(t):
    return "(%s)" % t if " " in t else t


def emit_enum(out, e):
    w = out.append
    haddock(out, e.doc)
    w("newtype %s = %s CInt" % (e.name, e.name))
    w("    deriving newtype (Eq, Ord, Show, Num, Bits, Storable)")
    w("")
    for item, value, doc in e.items:
        haddock(out, doc)
        w("pattern %s :: %s" % (item, e.name))
        w("pattern %s = %s %d" % (item, e.name, value))
    w("")


def emit_callback(out, api, c):
    w = out.append
    haddock(out, c.doc)
    fn = signature(api, c.params, c.ret)
    w("type %s = FunPtr (%s)" % (c.name, fn))
    w("")
    w("-- | Makes a '%s' of a Haskell function; free it with 'freeHaskellFunPtr' once the library" % c.name)
    w("-- no longer calls it.")
    w('foreign import ccall "wrapper" mk%s :: (%s) -> IO %s' % (c.name, fn, c.name))
    w("")


def emit_struct(out, api, s):
    w = out.append
    fields, size, align = layout(api, s)
    haddock(out, s.doc)
    w("data %s = %s" % (s.name, s.name))
    for i, (f, _) in enumerate(fields):
        lead = "{" if i == 0 else ","
        w("    %s %s :: %s" % (lead, field_name(s, f), field_type(api, f)))
        haddock(out, f.doc, "      ", "^")
    w("    } deriving (Eq, Show)")
    w("")
    w("-- | The zeroed structure (what @%s x = {0};@ is in C)." % s.name)
    zero = hs_name(s.name)
    w("%s :: %s" % (zero, s.name))
    w("%s = %s %s" % (zero, s.name, " ".join("(%s)" % zero_value(api, f) for f, _ in fields)))
    w("")
    w("instance Storable %s where" % s.name)
    w("    sizeOf _ = %d" % size)
    w("    alignment _ = %d" % align)
    w("    peek p = %s" % s.name)
    for i, (f, offset) in enumerate(fields):
        op = "<$>" if i == 0 else "<*>"
        if f.array:
            w("        %s peekArray %d (p `plusPtr` %d)" % (op, f.array, offset))
        else:
            w("        %s peekByteOff p %d" % (op, offset))
    w("    poke p v = do")
    for f, offset in fields:
        if f.array:
            w("        pokeArray (p `plusPtr` %d) (take %d (%s v ++ repeat 0))" % (offset, f.array, field_name(s, f)))
        else:
            w("        pokeByteOff p %d (%s v)" % (offset, field_name(s, f)))
    w("")


def generate_c(api):
    out = []
    w = out.append
    w("{-# LANGUAGE PatternSynonyms, DerivingStrategies, GeneralizedNewtypeDeriving #-}")
    w("{-# OPTIONS_GHC -Wno-missing-pattern-synonym-signatures -Wno-unused-imports #-}")
    w("")
    w("-- | Raven for Haskell: the C API as it is.")
    w("--")
    w("-- Generated from the C headers by tools/bindgen (Raven %s): do not edit." % api.version)
    w("--")
    w("-- Every function of C is here, its first letter lowered (@Rwindow_create@ is")
    w("-- 'rwindow_create'), with the types of \"Foreign.C\": texts are 'CString', pointers")
    w("-- 'Ptr'. Enumerations are newtypes over 'CInt' whose values keep the names of C")
    w("-- ('R_BACKEND_NULL' is a pattern). Structures are records with a 'Storable'")
    w("-- instance; their fields are named after the structure (@rXmlError_line@), and")
    w("-- @rXmlError@ is the zeroed structure. A callback of type @RXxxCallback@ is made")
    w("-- of a Haskell function by @mkRXxxCallback@.")
    w("--")
    w("-- The module \"Raven\" re-exports this one, with helpers for texts and closures.")
    w("module Raven.C where")
    w("")
    w("import Data.Bits (Bits)")
    w("import Data.Word (Word8, Word32, Word64)")
    w("import Foreign.C.String (CString)")
    w("import Foreign.C.Types")
    w("import Foreign.Marshal.Array (peekArray, pokeArray)")
    w("import Foreign.Ptr")
    w("import Foreign.Storable")
    w("")
    w("-- | A handle: every object of the library is one.")
    w("type Rid = Word32")
    w("")
    w("-- | An unsigned number of the API (flags, codepoints).")
    w("type Ruint = Word32")
    w("")
    for name, kind, value in api.defines:
        if kind == "str":
            w("%s :: String" % hs_name(name))
            w('%s = "%s"' % (hs_name(name), value))
        elif kind == "id":
            w("pattern %s :: Rid" % name)
            w("pattern %s = 0x%08X" % (name, value))
        else:
            w("pattern %s :: CInt" % name)
            w("pattern %s = %d" % (name, value))
    w("")
    for name in api.opaque:
        w("-- | An object of the library seen through a pointer only.")
        w("data %s" % name)
        w("")
    for e in api.enums:
        emit_enum(out, e)
    for c in api.callbacks:
        emit_callback(out, api, c)
    for s in api.structs:
        emit_struct(out, api, s)
    for f in api.functions:
        haddock(out, f.doc)
        # (safe: a function may call Haskell back, through a callback or an action)
        w('foreign import ccall safe "%s" %s :: %s' % (f.name, hs_name(f.name), signature(api, f.params, f.ret)))
        w("")
    return "\n".join(out)


RAVEN_HS = '''-- | Raven for Haskell.
--
-- Generated from the C headers by tools/bindgen (Raven %(version)s): do not edit.
--
-- "Raven.C" is the C API as it is, and is re-exported here. Around it, a few
-- helpers for what Haskell does differently from C:
--
-- * 'text' reads a text of the library, 'withText' makes one for it (UTF-8);
-- * 'actionBind', 'timerStart' and 'errorSetCallback' take Haskell functions;
-- * 'windowFromXml', 'widgetFromXml' and 'widgetFind' take a 'String'.
--
-- > import Raven
-- >
-- > main :: IO ()
-- > main = do
-- >     _ <- rinit R_BACKEND_VULKAN
-- >     _ <- windowFromXml "<Window Title=\\"Hi\\"><Button Text=\\"Quit\\" OnClicked=\\"quit\\"/></Window>"
-- >     actionBind "quit" (\\_ _ -> rquit 0)
-- >     let loop = do running <- rwait_frame (-1); if running /= 0 then loop else pure ()
-- >     loop
-- >     rshutdown
--
-- Raven belongs to the thread that starts it: call it from that thread only
-- (the main thread; with @-threaded@, not from a thread of 'forkIO').
module Raven
    ( module Raven.C
    , text
    , withText
    , fixedText
    , actionBind
    , actionUnbind
    , timerStart
    , timerStop
    , errorSetCallback
    , windowFromXml
    , widgetFromXml
    , widgetFind
    ) where

import Raven.C

import Control.Monad (void, when)
import Data.IORef
import qualified Data.Map.Strict as Map
import Foreign.C.String (CString)
import Foreign.C.Types
import Foreign.Marshal.Array (withArray0)
import Foreign.Ptr
import qualified GHC.Foreign as GHC
import GHC.IO.Encoding (utf8)
import System.IO.Unsafe (unsafePerformIO)

-- | A text of the library as a 'String' (@""@ for a null pointer).
text :: CString -> IO String
text pointer
    | pointer == nullPtr = pure ""
    | otherwise = GHC.peekCString utf8 pointer

-- | Calls a function with a text for the library, valid during the call.
withText :: String -> (CString -> IO a) -> IO a
withText = GHC.withCString utf8

-- | The text held by a @char@ array of a structure (@rXmlError_message@).
fixedText :: [CChar] -> IO String
fixedText chars = withArray0 0 (takeWhile (/= 0) chars) (GHC.peekCString utf8)

-- what the library calls: one function of each kind, which finds the Haskell one
{-# NOINLINE actions #-}
actions :: IORef (Map.Map String (Rid -> String -> IO ()))
actions = unsafePerformIO (newIORef Map.empty)

{-# NOINLINE timers #-}
timers :: IORef (Map.Map Rid (Bool, IO ()))
timers = unsafePerformIO (newIORef Map.empty)

{-# NOINLINE errorHandler #-}
errorHandler :: IORef (Maybe (RError -> RSeverity -> String -> IO ()))
errorHandler = unsafePerformIO (newIORef Nothing)

{-# NOINLINE actionCallback #-}
actionCallback :: RActionCallback
actionCallback = unsafePerformIO $ mkRActionCallback $ \\sender name _ -> do
    action <- text name
    found <- Map.lookup action <$> readIORef actions
    maybe (pure ()) (\\function -> function sender action) found

{-# NOINLINE timerCallback #-}
timerCallback :: RTimerCallback
timerCallback = unsafePerformIO $ mkRTimerCallback $ \\timer _ -> do
    found <- Map.lookup timer <$> readIORef timers
    case found of
        Nothing -> pure ()
        Just (repeating, function) -> do
            function
            -- a timer that fires once is done with its function
            when (not repeating) $ modifyIORef' timers (Map.delete timer)

{-# NOINLINE errorCallback #-}
errorCallback :: RErrorCallback
errorCallback = unsafePerformIO $ mkRErrorCallback $ \\code severity detail -> do
    handler <- readIORef errorHandler
    case handler of
        Nothing -> pure ()
        Just function -> text detail >>= function code severity

-- | Binds an action (@OnClicked="name"@ in XML, 'rwidget_set_action') to a
-- function, which receives the widget that fired it and the name of the action.
-- Binding the name again replaces the function.
actionBind :: String -> (Rid -> String -> IO ()) -> IO ()
actionBind name function = do
    modifyIORef' actions (Map.insert name function)
    withText name $ \\cname -> raction_bind cname actionCallback nullPtr

-- | Removes the function of an action ('raction_unbind').
actionUnbind :: String -> IO ()
actionUnbind name = do
    withText name raction_unbind
    modifyIORef' actions (Map.delete name)

-- | Calls a function after a number of seconds, once or again and again
-- ('rtimer_create'). Stop it with 'timerStop'.
timerStart :: Double -> Bool -> IO () -> IO Rid
timerStart seconds repeating function = do
    timer <- rtimer_create (realToFrac seconds) (if repeating then 1 else 0) timerCallback nullPtr
    when (timer /= R_INVALID_ID) $ modifyIORef' timers (Map.insert timer (repeating, function))
    pure timer

-- | Stops a timer started with 'timerStart'.
timerStop :: Rid -> IO ()
timerStop timer = do
    valid <- rtimer_is_valid timer
    when (valid /= 0) $ rtimer_destroy timer
    modifyIORef' timers (Map.delete timer)

-- | Gives the errors and warnings of the library to a function; 'Nothing'
-- removes it ('rerror_set_callback').
errorSetCallback :: Maybe (RError -> RSeverity -> String -> IO ()) -> IO ()
errorSetCallback handler = do
    writeIORef errorHandler handler
    rerror_set_callback (maybe nullFunPtr (const errorCallback) handler)

-- | Creates a window from XML ('rwindow_create_from_xml').
windowFromXml :: String -> IO Rid
windowFromXml xml = GHC.withCStringLen utf8 xml $ \\(pointer, size) ->
    rwindow_create_from_xml pointer (fromIntegral size)

-- | Creates a widget tree from XML ('rwidget_create_from_xml').
widgetFromXml :: String -> IO Rid
widgetFromXml xml = GHC.withCStringLen utf8 xml $ \\(pointer, size) ->
    rwidget_create_from_xml pointer (fromIntegral size)

-- | Finds a widget by its name ('rwidget_find').
widgetFind :: Rid -> String -> IO Rid
widgetFind scope name = withText name (rwidget_find scope)
'''

EXAMPLE = '''-- A window described in XML, driven from Haskell.
--
--   cabal run raven-hello --extra-lib-dirs=/path/to/lib
--
-- `-- --test` runs without a window system and without a GPU, clicks its own
-- button and checks what happened: it is how the build of Raven checks this
-- binding.

module Main (main) where

import Control.Monad (unless, when)
import Data.IORef
import Data.List (isInfixOf)
import Foreign.Marshal.Alloc (alloca)
import Foreign.Marshal.Utils (with)
import Foreign.Storable (peek)
import System.Environment (getArgs)
import System.Exit (exitFailure)

import Raven

xml :: String
xml = unlines
    [ "<Window Title=\\"Raven from Haskell\\" Width=\\"360\\" Height=\\"200\\">"
    , "    <VBox Spacing=\\"10\\" Slot.Padding=\\"16\\">"
    , "        <Label Name=\\"title\\" Text=\\"Hello from Haskell\\" TextSize=\\"20\\"/>"
    , "        <TextInput Name=\\"name\\" Placeholder=\\"Your name\\"/>"
    , "        <HBox Spacing=\\"8\\">"
    , "            <Button Name=\\"greet\\" Text=\\"Greet\\" Style=\\"Primary\\" OnClicked=\\"greet\\" Slot.Fill=\\"1\\"/>"
    , "            <Button Text=\\"Quit\\" OnClicked=\\"quit\\"/>"
    , "        </HBox>"
    , "    </VBox>"
    , "</Window>"
    ]

check :: Bool -> String -> IO ()
check ok what = unless ok $ do
    putStrLn ("haskell binding: failed: " ++ what)
    exitFailure

runLoop :: IO ()
runLoop = do
    running <- rwait_frame (-1)
    when (running /= 0) runLoop

main :: IO ()
main = do
    test <- elem "--test" <$> getArgs
    let desc = rInitDesc
            { rInitDesc_backend = if test then R_BACKEND_NULL else R_BACKEND_VULKAN
            , rInitDesc_headless = if test then 1 else 0
            }
    started <- with desc rinit_ex
    when (started == 0) $ do
        putStrLn "cannot start Raven"
        exitFailure

    window <- windowFromXml xml
    title <- widgetFind window "title"
    name <- widgetFind window "name"
    clicks <- newIORef (0 :: Int)

    actionBind "greet" $ \\_ _ -> do
        modifyIORef' clicks (+ 1)
        who <- rtext_input_get_text name >>= text
        withText ("Hello, " ++ who ++ "!") (rlabel_set_text title)
    actionBind "quit" $ \\_ _ -> rquit 0

    if not test
        then runLoop >> rshutdown
        else do
            rdraw_frame
            windowTitle <- rwindow_get_title window >>= text
            check (windowTitle == "Raven from Haskell") "the title of the window"
            kind <- rwidget_get_type title
            check (kind == R_WIDGET_LABEL) "the type of a widget"
            withText "Zoé" (rtext_input_set_text name)
            greet <- widgetFind window "greet"
            (x, y, width, height) <- alloca $ \\px -> alloca $ \\py -> alloca $ \\pw -> alloca $ \\ph -> do
                rwidget_get_rect greet px py pw ph
                (,,,) <$> peek px <*> peek py <*> peek pw <*> peek ph
            check (width > 0 && height > 0) "the button is laid out"
            rwindow_inject_mouse_move window (x + width / 2) (y + height / 2)
            rwindow_inject_mouse_button window R_MOUSE_LEFT 1
            rwindow_inject_mouse_button window R_MOUSE_LEFT 0
            count <- readIORef clicks
            check (count == 1) "the click calls the function"
            label <- rlabel_get_text title >>= text
            check (label == "Hello, Zoé!") "the function changed the label"

            -- structures, and a mistake described in one
            stats <- alloca $ \\p -> rmemory_get_stats p >> peek p
            check (rMemoryStats_allocation_count stats > 0) "a structure filled by the library"
            let broken = "<A><B></A>"
            (parsed, err) <- with rXmlError $ \\p -> do
                parsed <- withText broken $ \\b -> rxml_document_parse b (fromIntegral (length broken)) p
                (,) parsed <$> peek p
            check (parsed == R_INVALID_ID) "a broken document"
            check (rXmlError_line err == 1) "the line of the mistake"
            message <- fixedText (rXmlError_message err)
            check ("does not match" `isInfixOf` message) "the message of the mistake"

            -- documents
            document <- withText "VBox" rxml_document_create
            root <- rxml_document_get_root document
            node <- withText "Label" $ \\tag -> rxml_node_add_child root tag (-1)
            withText "Text" $ \\attribute -> withText "a < b" (rxml_node_set_attribute node attribute)
            written <- rxml_document_to_string document >>= text
            check ("Text=\\"a &lt; b\\"" `isInfixOf` written) "a document written as XML"
            built <- rxml_document_create_widget document
            first <- rwidget_get_child built 0 >>= rwidget_get_xml_node
            check (first == node) "the widgets of a document"
            rwidget_destroy built
            rxml_document_destroy document

            -- a timer, and the errors of the library
            fired <- newIORef (0 :: Int)
            _ <- timerStart 0 False (modifyIORef' fired (+ 1))
            errors <- newIORef []
            errorSetCallback (Just (\\_ _ detail -> modifyIORef' errors (detail :)))
            rwidget_destroy R_INVALID_ID
            reported <- readIORef errors
            check (not (null reported)) "the error callback"
            errorSetCallback Nothing
            let frames n = do
                    done <- readIORef fired
                    when (done == 0 && n > (0 :: Int)) (rdraw_frame >> frames (n - 1))
            frames 10
            timerCount <- readIORef fired
            check (timerCount == 1) "the timer"

            withText "quit" $ \\action -> raction_fire action R_INVALID_ID
            running <- ris_running
            check (running == 0) "an action fired by its name"
            rshutdown
            putStrLn "haskell binding: ok"
'''

CABAL = '''cabal-version:      2.4
name:               raven-gui
version:            %(version)s
synopsis:           Raven, a GUI library in C, for Haskell
description:
    The whole C API of Raven through the FFI (Raven.C), and helpers for texts
    and closures (Raven). Generated by tools/bindgen: do not edit.
    Needs the shared library of Raven: --extra-lib-dirs=/path/to/lib.
homepage:           https://github.com/oscar-soirey/Raven
license:            Apache-2.0
license-file:       LICENSE
category:           GUI
build-type:         Simple
extra-doc-files:    README.md

library
    exposed-modules:  Raven
                      Raven.C
    build-depends:    base >= 4.14 && < 5,
                      containers >= 0.6 && < 0.9
    hs-source-dirs:   src
    default-language: Haskell2010
    extra-libraries:  raven

executable raven-hello
    main-is:          Main.hs
    build-depends:    base, raven-gui
    hs-source-dirs:   example
    default-language: Haskell2010
'''

README = '''# Raven for Haskell

A cabal package, `raven-gui`:

- `Raven.C` is the whole C API of Raven through the FFI: every function, its
  first letter lowered (`Rwindow_create` is `rwindow_create`), with the types
  of `Foreign.C` (`CString`, `Ptr`, `CInt`...). Enumerations are newtypes whose
  values keep the names of C (`R_BACKEND_NULL`, `R_WIDGET_LABEL` are patterns);
  structures are records with a `Storable` instance, whose fields are named
  after the structure (`rXmlError_line`), and `rXmlError` is the zeroed
  structure. A callback type `RXxxCallback` is made of a Haskell function by
  `mkRXxxCallback`.
- `Raven` re-exports it, with helpers: `text` reads a text of the library and
  `withText` makes one (UTF-8); `actionBind`, `timerStart`/`timerStop` and
  `errorSetCallback` take Haskell functions; `windowFromXml`, `widgetFromXml`
  and `widgetFind` take a `String`.

Generated by `tools/bindgen` (`gen_haskell.py`): do not edit these files, edit
the generator.

## Use

Build the shared library of Raven (`cmake -B build -DRAVEN_BUILD_SHARED=ON`,
`cmake --build build`), add this folder to your `cabal.project`:

    packages: . <raven>/bindings/haskell

depend on `raven-gui`, and tell cabal where the library is:

    cabal run app --extra-lib-dirs=/path/to/lib

(on macOS also `--ghc-options=-optl-Wl,-rpath,/path/to/lib`, or put the library
where the program finds it; on Linux `LD_LIBRARY_PATH`).

The example of this folder:

    cabal run raven-hello --extra-lib-dirs=/path/to/lib              # a window
    cabal run raven-hello --extra-lib-dirs=/path/to/lib -- --test    # no window system, no GPU: what ctest runs

## From Haskell

```haskell
import Raven

main :: IO ()
main = do
    _ <- rinit R_BACKEND_VULKAN
    window <- windowFromXml "<Window Title=\\"Hello\\"><VBox Slot.Padding=\\"16\\">\\
        \\<TextInput Name=\\"name\\"/><Button Text=\\"Greet\\" OnClicked=\\"greet\\"/></VBox></Window>"
    name <- widgetFind window "name"
    actionBind "greet" $ \\_ _ -> do
        who <- rtext_input_get_text name >>= text
        putStrLn ("Hello, " ++ who)
    let loop = do running <- rwait_frame (-1); if running /= 0 then loop else pure ()
    loop
    rshutdown
```

- **Texts** are `CString`: `withText "Hi" (rlabel_set_text label)` gives one,
  `text` reads one (`""` for a null pointer), `fixedText` reads a `char` array
  of a structure. Both use UTF-8, whatever the locale.
- **Pointers to values** take `alloca` and `peek`; a structure goes in with
  `with rInitDesc { rInitDesc_headless = 1 } rinit_ex`.
- **Callbacks.** `actionBind`, `timerStart` and `errorSetCallback` keep the
  Haskell functions themselves. For the other callbacks, `mkRXxxCallback`
  makes a `FunPtr` that lives until `freeHaskellFunPtr`; or bind an action
  (`OnClicked="name"` in XML, `rwidget_set_action`).
- **Threads.** Raven belongs to the thread that starts it: call it from the
  main thread (with `-threaded`, not from a thread of `forkIO`). Every function
  is a `safe` foreign call, since it may call Haskell back.
- The structures have the layout of C on a 64-bit target.

`raven_backend.h` and `raven_vulkan.h` are left out, as in the other bindings.

Tested with GHC 9.6 or later on macOS and Linux. Windows is not tested.
'''

GITIGNORE = '''dist-newstyle/
'''


def generate(api):
    from gen_rust import package_version
    version = api.version or "0.2"
    return {
        "haskell/src/Raven/C.hs": generate_c(api),
        "haskell/src/Raven.hs": RAVEN_HS % {"version": version},
        "haskell/example/Main.hs": EXAMPLE,
        "haskell/raven-gui.cabal": CABAL % {"version": package_version()},
        "haskell/README.md": README,
        "haskell/.gitignore": GITIGNORE,
    }

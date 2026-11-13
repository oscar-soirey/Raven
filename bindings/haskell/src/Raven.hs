-- | Raven for Haskell.
--
-- Generated from the C headers by tools/bindgen (Raven 0.2): do not edit.
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
-- >     _ <- windowFromXml "<Window Title=\"Hi\"><Button Text=\"Quit\" OnClicked=\"quit\"/></Window>"
-- >     actionBind "quit" (\_ _ -> rquit 0)
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
actionCallback = unsafePerformIO $ mkRActionCallback $ \sender name _ -> do
    action <- text name
    found <- Map.lookup action <$> readIORef actions
    maybe (pure ()) (\function -> function sender action) found

{-# NOINLINE timerCallback #-}
timerCallback :: RTimerCallback
timerCallback = unsafePerformIO $ mkRTimerCallback $ \timer _ -> do
    found <- Map.lookup timer <$> readIORef timers
    case found of
        Nothing -> pure ()
        Just (repeating, function) -> do
            function
            -- a timer that fires once is done with its function
            when (not repeating) $ modifyIORef' timers (Map.delete timer)

{-# NOINLINE errorCallback #-}
errorCallback :: RErrorCallback
errorCallback = unsafePerformIO $ mkRErrorCallback $ \code severity detail -> do
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
    withText name $ \cname -> raction_bind cname actionCallback nullPtr

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
windowFromXml xml = GHC.withCStringLen utf8 xml $ \(pointer, size) ->
    rwindow_create_from_xml pointer (fromIntegral size)

-- | Creates a widget tree from XML ('rwidget_create_from_xml').
widgetFromXml :: String -> IO Rid
widgetFromXml xml = GHC.withCStringLen utf8 xml $ \(pointer, size) ->
    rwidget_create_from_xml pointer (fromIntegral size)

-- | Finds a widget by its name ('rwidget_find').
widgetFind :: Rid -> String -> IO Rid
widgetFind scope name = withText name (rwidget_find scope)

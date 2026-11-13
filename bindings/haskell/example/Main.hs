-- A window described in XML, driven from Haskell.
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
    [ "<Window Title=\"Raven from Haskell\" Width=\"360\" Height=\"200\">"
    , "    <VBox Spacing=\"10\" Slot.Padding=\"16\">"
    , "        <Label Name=\"title\" Text=\"Hello from Haskell\" TextSize=\"20\"/>"
    , "        <TextInput Name=\"name\" Placeholder=\"Your name\"/>"
    , "        <HBox Spacing=\"8\">"
    , "            <Button Name=\"greet\" Text=\"Greet\" Style=\"Primary\" OnClicked=\"greet\" Slot.Fill=\"1\"/>"
    , "            <Button Text=\"Quit\" OnClicked=\"quit\"/>"
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

    actionBind "greet" $ \_ _ -> do
        modifyIORef' clicks (+ 1)
        who <- rtext_input_get_text name >>= text
        withText ("Hello, " ++ who ++ "!") (rlabel_set_text title)
    actionBind "quit" $ \_ _ -> rquit 0

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
            (x, y, width, height) <- alloca $ \px -> alloca $ \py -> alloca $ \pw -> alloca $ \ph -> do
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
            stats <- alloca $ \p -> rmemory_get_stats p >> peek p
            check (rMemoryStats_allocation_count stats > 0) "a structure filled by the library"
            let broken = "<A><B></A>"
            (parsed, err) <- with rXmlError $ \p -> do
                parsed <- withText broken $ \b -> rxml_document_parse b (fromIntegral (length broken)) p
                (,) parsed <$> peek p
            check (parsed == R_INVALID_ID) "a broken document"
            check (rXmlError_line err == 1) "the line of the mistake"
            message <- fixedText (rXmlError_message err)
            check ("does not match" `isInfixOf` message) "the message of the mistake"

            -- documents
            document <- withText "VBox" rxml_document_create
            root <- rxml_document_get_root document
            node <- withText "Label" $ \tag -> rxml_node_add_child root tag (-1)
            withText "Text" $ \attribute -> withText "a < b" (rxml_node_set_attribute node attribute)
            written <- rxml_document_to_string document >>= text
            check ("Text=\"a &lt; b\"" `isInfixOf` written) "a document written as XML"
            built <- rxml_document_create_widget document
            first <- rwidget_get_child built 0 >>= rwidget_get_xml_node
            check (first == node) "the widgets of a document"
            rwidget_destroy built
            rxml_document_destroy document

            -- a timer, and the errors of the library
            fired <- newIORef (0 :: Int)
            _ <- timerStart 0 False (modifyIORef' fired (+ 1))
            errors <- newIORef []
            errorSetCallback (Just (\_ _ detail -> modifyIORef' errors (detail :)))
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

            withText "quit" $ \action -> raction_fire action R_INVALID_ID
            running <- ris_running
            check (running == 0) "an action fired by its name"
            rshutdown
            putStrLn "haskell binding: ok"

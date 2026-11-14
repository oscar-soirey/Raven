## Raven for Nim: the C API as it is.
##
## Generated from the C headers by tools/bindgen (Raven 0.2): do not edit.
##
## The same names as in C. Enumerations are `cint` with constants, as in C;
## structures start zeroed, as every Nim object (`var desc: RInitDesc` is
## `RInitDesc desc = {0};`); texts are `cstring`: a string literal or a `string`
## passes as it is. Callbacks are `{.cdecl.}` procedures.
##
## The module `raven_gui` exports this one, with helpers for texts and closures.
##
## The library is linked (`-lraven`; give its folder with `--passL:-L<dir>`), or,
## with `-d:ravenDynlib`, loaded when the program starts (`-d:ravenLib=<file>`
## names the file).

when defined(ravenDynlib):
  const ravenLib {.strdefine.} =
    when defined(windows): "raven.dll"
    elif defined(macosx): "libraven.dylib"
    else: "libraven.so"
  {.pragma: rapi, importc, cdecl, dynlib: ravenLib.}
else:
  {.passL: "-lraven".}
  {.pragma: rapi, importc, cdecl.}

type
  Rid* = uint32
    ## A handle: every object of the library is one.
  Ruint* = uint32
    ## An unsigned number of the API (flags, codepoints).
  RPainter* = object
    ## An object of the library seen through a pointer only.
  RBackend* = cint
  RError* = cint
  RSeverity* = cint
  RWindowFlags* = cint
  RPictureInPictureCorner* = cint
    ## Where a window in picture in picture goes (Rwindow_set_picture_in_picture_layout).
  RBackdrop* = cint
    ## What is seen behind a window created with R_WINDOW_TRANSPARENT.
    ##
    ##   Windows 11          every value
    ##   Windows 10          TRANSPARENT, BLUR, ACRYLIC (MICA gives ACRYLIC)
    ##   macOS               TRANSPARENT; other backdrops fall back to transparency
    ##   X11                 TRANSPARENT with any compositing manager; BLUR where
    ##                       the compositor blurs on request (KWin; picom and
    ##                       others with their own settings); ACRYLIC and MICA
    ##                       give BLUR
    ##   no compositor       nothing: the window stays opaque
  RWindowSystem* = cint
  RWindowEdge* = cint
    ## Edges and corners of a window, for Rwindow_begin_resize.
  RMouseButton* = cint
  RModifier* = cint
  RCursor* = cint
  RKey* = cint
    ## Keyboard keys. Printable keys use their upper-case ASCII code.
  REventType* = cint
  RDropPhase* = cint
    ## Moments of a drag coming from another application (files from the file manager, text...).
  RTouchMode* = cint
    ## What a finger does on a widget (Rwidget_set_touch_mode).
  RTouchPhase* = cint
  RPenPhase* = cint
  RPenButton* = cint
    ## Buttons on the side of a pen (RPenEvent::buttons).
  RGesturePhase* = cint
  RGestureKind* = cint
    ## What the fingers of a gesture do (RGestureEvent::kind).
  RGestureDevice* = cint
    ## Where the fingers of a gesture are (RGestureEvent::device).
  RLook* = cint
    ## The looks. Each one comes dark and light.
    ##
    ## Raven's looks are the default; the ones of the platforms imitate them
    ## closely (their shapes, colors, sizes, states and motion), without using
    ## their widgets. The looks of the platforms are opaque, except macOS Glass
    ## (macOS 26), whose bars, menus and floating surfaces are glass as on the
    ## system; the glass materials themselves (Rtheme_set_material) belong to
    ## the glass looks of Raven.
  RTheme* = cint
    ## @deprecated The built-in themes of Raven 0.2, now looks: R_THEME_DARK is
    ## R_LOOK_RAVEN_DARK and so on, and Rtheme_apply is Rlook_set.
  RMaterial* = cint
    ## What surfaces are made of, from the clearest to the most opaque. A theme
    ## has one material for everything (Rtheme_set_material); any widget can be
    ## given its own (Rwidget_set_material, Material="..." in XML).
    ##
    ## The three glasses share the same anatomy (a tint over what is behind, a
    ## rim that bends it and catches the light, a shadow) and differ by how much
    ## they let through. Within one material, surfaces are also told apart by
    ## their height above the window: bars lie flat, panels float a little,
    ## controls a little more, menus and dialogs highest; each level has a
    ## brighter rim and a deeper shadow than the one below.
  RColorRole* = cint
    ## Color roles. The comment lists the widgets reading each role.
  RMetric* = cint
    ## Sizes, in logical units.
  RFontStyle* = cint
    ## The styles of a font (Rfont_get_variant).
  RFilterType* = cint
  RWidgetType* = cint
  RHAlign* = cint
  RVAlign* = cint
  RVisibility* = cint
  RTransition* = cint
    ## How a widget arrives. A transition animates what the widget and its
    ## children draw; the layout is not affected, the widget is in place from the
    ## first frame for its neighbours and for the mouse.
  ROrientation* = cint
  RBlurStyle* = cint
    ## The looks of a R_WIDGET_BACKGROUND_BLUR. The first three are drawn by
    ## Raven from what the window shows behind the widget; the last one is the
    ## blur of the OS, which sees what is behind the window itself.
  RDirection* = cint
  RTextAlign* = cint
  RToastKind* = cint
  RToastQueue* = cint
    ## What a Toast does with a message that comes while another one is shown.
  RButtonStyle* = cint
  RCheckboxStyle* = cint
  RStretch* = cint
  RPlotAxis* = cint
    ## The axes of a Plot.
  RPlotScale* = cint
    ## How an axis of a Plot spreads its values.
  RPlotLine* = cint
    ## How the line of a series of a Plot is drawn.
  RPlotMarker* = cint
    ## The mark drawn on each point of a series of a Plot.
  RPlotLegend* = cint
    ## Where the legend of a Plot is.
  RSyntax* = cint
    ## Shorthands of Rtext_editor_set_language for two languages.
  RLanguageFlags* = cint
  RToken* = cint
    ## What a part of the text is, for the language of the editor (Rtext_editor_get_token).
  RMarkerKind* = cint
    ## What a marker says about the text it underlines.
  RHeaderButtons* = cint
    ## Buttons of a header bar.
  RTaskbarProgress* = cint
    ## A progress bar on the icon of the application (Rwindow_set_taskbar_progress).
  RDockSide* = cint
    ## Where a panel goes in a dock area, relative to another panel or to the whole area.
  RVisualPreset* = cint
  RTimelineItem* = cint
    ## What an item of a timeline is.
  RBlockShape* = cint
  RBlockPlace* = cint
  RNotificationUrgency* = cint
    ## How much a notification of the system asks for attention (Rnotification_show).
  RFileDialogFlags* = cint
  RFileBrowserMode* = cint
  RMessageButtons* = cint
  RMessageResult* = cint
  RMotion* = cint
    ## How a spring moves. Animations in Raven are springs, as on iOS: a value is
    ## pulled toward its target and may run past it before settling, which is what
    ## makes a control feel alive rather than mechanical.
  REffect* = cint
    ## The effects that come with Raven (Reffect_get).
  RShaderFlags* = cint
    ## Flags of Rshader_create.
  RXmlValueType* = cint
    ## The kind of value an attribute takes.
  RColorFilter* = cint
  RAccessibleRole* = cint
  RFrameCallback* = proc (delta_seconds: cdouble, user_data: pointer) {.cdecl.}
    ## Registers a function called once per frame, before the layout.
    ## The body of your loop does the same job; the callback is also called
    ## while the OS keeps the thread in a loop of its own (on Windows, while
    ## the user moves or resizes a window), when your loop is not running.
  RTaskCallback* = proc (user_data: pointer) {.cdecl.}
  RParallelRangeCallback* = proc (begin: uint32, `end`: uint32, user_data: pointer) {.cdecl.}
  RTimerCallback* = proc (timer: Rid, user_data: pointer) {.cdecl.}
  RActionCallback* = proc (sender: Rid, action: cstring, user_data: pointer) {.cdecl.}
    ## Actions connect widgets to application code by name. A widget carries an
    ## action name (set from XML with OnClicked="file.save", or from C with
    ## Rwidget_set_action); when its main event fires, the function bound to that
    ## name is called. Names are resolved when the event fires, so actions can be
    ## bound before or after the widgets are created.
  RErrorCallback* = proc (code: RError, severity: RSeverity, detail: cstring) {.cdecl.}
    ## Registers a function called whenever an error is raised.
    ## The function may be registered before Rinit. It is called on the
    ## thread that raised the error: errors of the renderer (and the messages
    ## of the Vulkan validation layers) come from the render thread.
  RDeviceResetCallback* = proc (user_data: pointer) {.cdecl.}
    ## Registers a function called after Raven recovered from the loss
    ## of the GPU device (a driver reset or update, a GPU removed, a long sleep).
    ##
    ## Raven then starts the backend again and remakes what it can by itself:
    ## the surfaces of the windows, the fonts, the shaders, the textures made
    ## from image files and the small textures (up to 256 x 256). The pixels of
    ## the larger textures made from pixels are lost: the textures keep their
    ## handle and size but are transparent until Rtexture_update fills them
    ## again, which is what this function is for. Textures that wrap objects of
    ## the backend (raven_vulkan.h) must be made again, and the Vulkan handles
    ## returned by Rvulkan_get_device... have changed.
    ##
    ## Called on the UI thread, during Rdraw_frame / Rwait_frame. Set it after
    ## Rinit; NULL removes it.
  RDropCallback* = proc (target: Rid, event: ptr RDropEvent, user_data: pointer): cint {.cdecl.}
    ## Called for a drop target (a widget, or a window: `target` is its
    ## handle). Return R_TRUE to accept: while the pointer is over the target this
    ## shows the "can drop here" cursor and highlights the target; for
    ## R_DROP_DROPPED it tells the source that the data was taken.
  RWindowCloseCallback* = proc (window: Rid, user_data: pointer): cint {.cdecl.}
    ## Called when the user asks to close the window.
    ## Return R_TRUE to let the window close, R_FALSE to keep it open.
  RWindowResizeCallback* = proc (window: Rid, width: cint, height: cint, user_data: pointer) {.cdecl.}
    ## Called after the window was resized. Sizes are in logical units.
  RWindowEventHook* = proc (window: Rid, event: ptr REvent, user_data: pointer): cint {.cdecl.}
    ## Sees every input event of the window (mouse, wheel, keys, text)
    ## before any widget, with `target` set to the widget under the pointer.
    ## Returning R_TRUE consumes the event: the widgets do not get it. The
    ## state read by Rwindow_is_key_down is updated either way.
  RTouchCallback* = proc (widget: Rid, event: ptr RTouchEvent, user_data: pointer): cint {.cdecl.}
    ## Return R_TRUE to take the finger: the library then neither clicks nor scrolls with it.
  RPenCallback* = proc (widget: Rid, event: ptr RPenEvent, user_data: pointer): cint {.cdecl.}
    ## Return R_TRUE (on R_PEN_DOWN) to take the stroke: the pen does not act as a mouse until it is lifted.
  RGestureCallback* = proc (widget: Rid, event: ptr RGestureEvent, user_data: pointer): cint {.cdecl.}
    ## Return R_TRUE (on R_GESTURE_BEGIN) to take the gesture: nothing zooms, pans or navigates by itself.
  RWidgetDestroyCallback* = proc (widget: Rid, user_data: pointer) {.cdecl.}
    ## Called once when the widget is gone: deleted by the application,
    ## with its parent or its window, or by Rshutdown. The place to release
    ## what belonged to it (a texture, a connection). The handle it receives
    ## names nothing any more: it is only there to tell widgets apart.
    ## Called on the UI thread, shortly after the deletion.
  RWidgetEventCallback* = proc (widget: Rid, event: ptr REvent, user_data: pointer): cint {.cdecl.}
    ## Receives the input events of a widget before the widget itself.
    ## Return R_TRUE to consume the event (the widget and its parents will
    ## not see it), R_FALSE to let it continue.
  RTabChangedCallback* = proc (tab_view: Rid, index: cint, user_data: pointer) {.cdecl.}
  RTabCloseCallback* = proc (tab_view: Rid, index: cint, content: Rid, user_data: pointer): cint {.cdecl.}
    ## The user closes a tab (its button, a middle click). Return R_TRUE
    ## to delete its page now, R_FALSE to keep it (the application may ask to
    ## save, then delete the page itself with Rwidget_destroy).
  RExpanderChangedCallback* = proc (expander: Rid, expanded: cint, user_data: pointer) {.cdecl.}
  RLabelLinkCallback* = proc (label: Rid, url: cstring, user_data: pointer) {.cdecl.}
    ## Called when the user clicks a link of a Markdown label.
    ##
    ## Without a callback, a link written [Save](action:file.save) fires the
    ## action "file.save" (Raction_bind), and http, https and mailto links
    ## open in the browser or the mail program of the user (Ropen_url).
  RButtonClickedCallback* = proc (button: Rid, user_data: pointer) {.cdecl.}
  RCheckboxChangedCallback* = proc (checkbox: Rid, checked: cint, user_data: pointer) {.cdecl.}
  RSliderChangedCallback* = proc (slider: Rid, value: cfloat, user_data: pointer) {.cdecl.}
  RTextChangedCallback* = proc (text_input: Rid, text: cstring, user_data: pointer) {.cdecl.}
  RSpinBoxChangedCallback* = proc (spin_box: Rid, value: cdouble, user_data: pointer) {.cdecl.}
  RComboBoxChangedCallback* = proc (combo_box: Rid, index: cint, user_data: pointer) {.cdecl.}
  RListItemTextCallback* = proc (list_view: Rid, index: cint, user_data: pointer): cstring {.cdecl.}
    ## Virtual mode, for large or computed data: the list stores nothing and
    ## asks for the text of the rows it draws. The returned string must stay
    ## valid until the next call.
  RListItemColorCallback* = proc (list_view: Rid, index: cint, r: ptr cfloat, g: ptr cfloat, b: ptr cfloat, a: ptr cfloat, user_data: pointer): cint {.cdecl.}
    ## Colors the text of rows (a level, a verdict): the list asks for the color
    ## of each row it draws, stored or virtual. Set r, g, b, a (0..1) and return
    ## R_TRUE; R_FALSE (or alpha 0) keeps the color of the theme. The color is
    ## kept on a selected or hovered row. NULL removes the callback.
  RListEventCallback* = proc (list_view: Rid, index: cint, user_data: pointer) {.cdecl.}
  RTreeEventCallback* = proc (tree_view: Rid, item: Rid, user_data: pointer) {.cdecl.}
  RTreeContextCallback* = proc (tree_view: Rid, item: Rid, x: cfloat, y: cfloat, user_data: pointer) {.cdecl.}
    ## Right click: `item` is the item under the pointer (selected
    ## first), or R_INVALID_ID in the empty space; x, y in the window, for Rmenu_open.
  RMenuItemClickedCallback* = proc (item: Rid, user_data: pointer) {.cdecl.}
  RTileItemCallback* = proc (tile_view: Rid, index: cint, item: ptr RTileItem, user_data: pointer) {.cdecl.}
    ## Fills `item` for tile `index`. Called on the UI thread, only for the tiles in view.
  RTileEventCallback* = proc (tile_view: Rid, index: cint, user_data: pointer) {.cdecl.}
    ## Selection change (`index`: the current tile, or -1) and activation (double click, Enter).
  RTileContextCallback* = proc (tile_view: Rid, index: cint, x: cfloat, y: cfloat, user_data: pointer) {.cdecl.}
    ## Right click: the tile under the pointer (selected first), or -1 in the empty space; x, y in the window.
  RTileDragDataCallback* = proc (tile_view: Rid, user_data: pointer): cstring {.cdecl.}
    ## The data of a drag, given by the application (paths of files...)
    ## instead of the indices: the dragged tiles are the selected ones
    ## (Rtile_view_get_selection). The text returned is copied; NULL keeps
    ## the indices.
  RTableCellTextCallback* = proc (table: Rid, row: cint, column: cint, user_data: pointer): cstring {.cdecl.}
    ## Virtual mode, for large or computed data: the table stores nothing and
    ## asks for the texts of the cells it draws. The returned string must stay
    ## valid until the next call.
  RTableCellColorCallback* = proc (table: Rid, row: cint, column: cint, r: ptr cfloat, g: ptr cfloat, b: ptr cfloat, a: ptr cfloat, user_data: pointer): cint {.cdecl.}
    ## Colors the text of cells (a verdict, a state): the table asks for the
    ## color of each cell it draws, stored or virtual (`row` is the index of the
    ## order shown). Set r, g, b, a (0..1) and return R_TRUE; R_FALSE (or alpha 0)
    ## keeps the color of the theme. The color is kept on a selected or hovered
    ## row. NULL removes the callback.
  RTableEventCallback* = proc (table: Rid, row: cint, user_data: pointer) {.cdecl.}
  RTableSortCallback* = proc (table: Rid, column: cint, descending: cint, user_data: pointer) {.cdecl.}
    ## Called when the user clicks a header; `column` -1: not sorted.
  RDateChangedCallback* = proc (date_picker: Rid, year: cint, month: cint, day: cint, user_data: pointer) {.cdecl.}
    ## Called when the user chooses a date. This also fires the action of the widget.
  RTimeChangedCallback* = proc (time_picker: Rid, hour: cint, minute: cint, second: cint, user_data: pointer) {.cdecl.}
    ## Called when the user changes the time. This also fires the action of the widget.
  RToastCallback* = proc (toast: Rid, user_data: pointer) {.cdecl.}
    ## Called by the button of a message, which then hides. This also fires the action of the widget.
  RBreadcrumbCallback* = proc (breadcrumb: Rid, index: cint, user_data: pointer) {.cdecl.}
    ## Called when the user clicks an item. This also fires the action of the widget.
  RRatingChangedCallback* = proc (rating: Rid, value: cfloat, user_data: pointer) {.cdecl.}
    ## Called when the user changes the score. This also fires the action of the widget.
  RPageChangedCallback* = proc (pagination: Rid, page: cint, user_data: pointer) {.cdecl.}
    ## Called when the user goes to another page. This also fires the action of the widget.
  RPlotViewCallback* = proc (plot: Rid, user_data: pointer) {.cdecl.}
    ## Called when the user moves, zooms or fits the view (Rplot_get_axis_range tells where it is).
  RPlotDragLineCallback* = proc (plot: Rid, line: cint, value: cdouble, user_data: pointer) {.cdecl.}
    ## Called while the user drags a drag line. This also fires the action of the widget.
  RPlotSeriesCallback* = proc (plot: Rid, series: cint, visible: cint, user_data: pointer) {.cdecl.}
    ## Called when the user hides or shows a series from the legend.
  RTextEditorZoomCallback* = proc (editor: Rid, zoom: cfloat, user_data: pointer) {.cdecl.}
    ## Called when the user zoomed (to zoom other editors the same, or to keep it for the next time).
  RCompletionCallback* = proc (editor: Rid, prefix: cstring, user_data: pointer) {.cdecl.}
    ## Called each time the editor gathers its proposals, with the
    ## start of the word being typed: add your own with
    ## Rtext_editor_add_completion (the fields of a structure, the answers of
    ## a language server...). They are listed before the others.
  RTextEditorHoverCallback* = proc (editor: Rid, line: cint, column: cint, user_data: pointer): cstring {.cdecl.}
    ## Called while the pointer moves over the text where no marker
    ## is, with the position under it: the text returned (NULL or "" for
    ## none) is shown as the tooltip, as a marker's message is (what a name
    ## stands for, the value of a variable). The text must stay valid until
    ## the next call.
  RTextEditorChangedCallback* = proc (editor: Rid, user_data: pointer) {.cdecl.}
    ## Called after each change made by the user (typing, pasting,
    ## undoing...) or by Rtext_editor_insert_text. The action of the widget
    ## (OnChanged in XML) fires at the same time.
  RTrayClickedCallback* = proc (tray: Rid, button: RMouseButton, user_data: pointer) {.cdecl.}
    ## Called when the tray icon is clicked (Rtray_set_clicked_callback).
  RDockCloseCallback* = proc (area: Rid, panel: Rid, user_data: pointer): cint {.cdecl.}
    ## Called when the user closes a panel, before it is deleted.
    ## Return R_FALSE to keep the panel.
  RDockChangedCallback* = proc (area: Rid, user_data: pointer) {.cdecl.}
    ## Called after the user moved, resized, showed or closed a panel.
  RIslandChangedCallback* = proc (island: Rid, expanded: cint, user_data: pointer) {.cdecl.}
    ## Called when the user unfolds or folds the island. The action of the widget (OnChanged) fires too.
  RColorChangedCallback* = proc (picker: Rid, r: cfloat, g: cfloat, b: cfloat, a: cfloat, user_data: pointer) {.cdecl.}
    ## Called each time the user changes the color: while dragging,
    ## and when a code or a number is entered. The action of the widget
    ## (OnChanged in XML) fires too.
  RTimelineTimeCallback* = proc (timeline: Rid, seconds: cdouble, user_data: pointer) {.cdecl.}
    ## The playhead moved: scrubbed by the user, or running by itself.
  RTimelineItemCallback* = proc (timeline: Rid, item: cint, user_data: pointer) {.cdecl.}
    ## Something happened to an item; which thing depends on the function the callback was given to.
  RNodeGraphConnectCallback* = proc (graph: Rid, output_pin: cint, input_pin: cint, user_data: pointer): cint {.cdecl.}
    ## Asked when the user is about to link two pins that may be
    ## linked: return R_FALSE to refuse (a loop, a rule of the application).
  RNodeGraphItemCallback* = proc (graph: Rid, item: cint, user_data: pointer) {.cdecl.}
    ## Something happened to a link or to a node; which thing depends on the function the callback was given to.
  RNodeGraphContextCallback* = proc (graph: Rid, node: cint, x: cfloat, y: cfloat, user_data: pointer) {.cdecl.}
    ## Right click: `node` is the node under the pointer (selected first),
    ## or 0 on the empty canvas; x, y are in the widget (Rnode_graph_to_canvas).
    ## Without this callback the right button does nothing.
  RNodeGraphPointCallback* = proc (graph: Rid, item: cint, x: cfloat, y: cfloat, user_data: pointer) {.cdecl.}
    ## Something happened at a point of the canvas: `item` depends on the
    ## function the callback was given to; x, y are in canvas units (as node positions).
  RBlockEditorCallback* = proc (editor: Rid, `block`: cint, user_data: pointer) {.cdecl.}
    ## A block changed (0: several, or one was deleted), or a script was clicked (its top block).
  RBlockEditorContextCallback* = proc (editor: Rid, `block`: cint, x: cfloat, y: cfloat, user_data: pointer) {.cdecl.}
    ## Right click: on a block (0 on the canvas), at x, y in the widget.
  RDialogClosedCallback* = proc (content: Rid, result_value: cint, user_data: pointer) {.cdecl.}
    ## Called when a dialog closes. `result` is the value given to Rdialog_close, 0 when dismissed.
  RFileBrowserCallback* = proc (browser: Rid, path: cstring, user_data: pointer) {.cdecl.}
    ## Called when the user chooses: a double click or Enter on a file,
    ## the Open / Save button. `path` is the first chosen path; with several
    ## files, read them with Rfile_browser_get_selection. The action of the
    ## widget (OnActivated in XML) fires at the same time.
  RNotificationFallback* = proc (title: cstring, body: cstring, urgency: RNotificationUrgency, user_data: pointer) {.cdecl.}
  RNotificationClickedCallback* = proc (user_data: pointer) {.cdecl.}
  RViewportDrawCallback* = proc (info: ptr RViewportDrawInfo, user_data: pointer) {.cdecl.}
    ## Called on the render thread each time the window is rendered.
    ## State you bind (pipeline, descriptor sets, viewport, scissor) does not
    ## need to be restored. NULL removes the callback.
  RViewportResizeCallback* = proc (viewport: Rid, width: cint, height: cint, user_data: pointer) {.cdecl.}
    ## Called on the UI thread when the pixel size changes: resize your render target here.
  RPaintCallback* = proc (widget: Rid, painter: ptr RPainter, width: cfloat, height: cfloat, user_data: pointer) {.cdecl.}
  RMeasureCallback* = proc (widget: Rid, width: ptr cfloat, height: ptr cfloat, user_data: pointer) {.cdecl.}
  RXmlReloadCallback* = proc (path: cstring, ok: cint, user_data: pointer) {.cdecl.}
    ## Called each time a file was loaded again.
    ## path The file given when the interface was created.
    ## ok   R_FALSE when the file could not be used (the interface is unchanged).
    ##
    ## Widgets that had to be built again have new handles: this is the
    ## place to look them up again (Rwidget_find).
  RXmlTagCreateCallback* = proc (tag: cstring, user_data: pointer): Rid {.cdecl.}
    ## Tags made by code. A library or an application that builds a widget of
    ## its own (a R_WIDGET_CUSTOM with its callbacks, or an assembly of
    ## widgets) can give it a tag: documents then write <MyWidget .../> as
    ## they write <Button/>.
    ##
    ## `create` makes the widget and returns it. `set_attribute` (may be
    ## NULL) receives the attributes of the element before Raven looks at
    ## them: it returns R_TRUE for those it understands, R_FALSE to leave
    ## them to the widget (Name, Width, Slot..., and what its type knows).
    ## Rwidget_set_attribute goes through it too.
  RXmlTagAttributeCallback* = proc (widget: Rid, name: cstring, value: cstring, user_data: pointer): cint {.cdecl.}
  RLocaleChangedCallback* = proc (language: cstring, user_data: pointer) {.cdecl.}
  RAccessiblePressCallback* = proc (widget: Rid, user_data: pointer) {.cdecl.}
  RInitDesc* {.bycopy.} = object
    ## Optional settings for Rinit_ex. A zero-initialized structure selects the
    ## default for every field, so new fields can be added without breaking code.
    backend*: RBackend
      ## Rendering backend. 0 selects WebGL in browser builds, Vulkan otherwise.
    app_name*: cstring
      ## Application name reported to the graphics driver. May be NULL.
    worker_threads*: cint
      ## Worker threads of the job system. 0 = one per spare CPU core, negative = none.
    disable_render_thread*: cint
      ## R_TRUE renders on the UI thread instead of a dedicated render thread.
    disable_vsync*: cint
      ## R_TRUE presents without waiting for the vertical blank.
    enable_validation*: cint
      ## R_TRUE enables the backend debug layer (Vulkan validation) when installed.
    headless*: cint
      ## R_TRUE opens no OS window (null platform). Combine with R_BACKEND_NULL for tests.
    ui_scale*: cfloat
      ## Global UI scale multiplier applied on top of the monitor scale. 0 = 1.0.
  RAllocator* {.bycopy.} = object
    ## Memory functions of the application, to replace the allocator of Raven. See Rset_allocator.
    alloc*: pointer
    realloc*: pointer
    free*: pointer
    user_data*: pointer
  RMemoryStats* {.bycopy.} = object
    ## What the library holds. See Rmemory_get_stats.
    used_bytes*: csize_t
      ## Bytes in the blocks currently allocated. 0 with the allocator of the application.
    peak_bytes*: csize_t
      ## Highest value of used_bytes since the start.
    reserved_bytes*: csize_t
      ## Bytes taken from the OS: used_bytes, plus what is free in the heap.
    allocation_count*: csize_t
      ## Blocks currently allocated. 0 after Rshutdown: anything else is a leak of the library.
    total_allocations*: uint64
      ## Allocations made since the start.
  RFrameStats* {.bycopy.} = object
    ## Timings and counters of the last frame produced for a window.
    layout_ms*: cdouble
    paint_ms*: cdouble
    render_ms*: cdouble
    widget_count*: uint32
    instance_count*: uint32
    draw_call_count*: uint32
    backdrop_count*: uint32
      ## Glass: times the frame was copied and blurred to be seen through (0 without glass).
    paint_job_count*: uint32
    frame_index*: uint64
  RNativeWindow* {.bycopy.} = object
    ## Native handles of a window or of a native viewport.
    ##   Win32 : display = HINSTANCE, window = HWND
    ##   X11   : display = Display*,  window = Window (XID), visual = VisualID
    ##   Wayland: display = wl_display*, window = wl_surface* (Linux and BSD choose
    ##           Wayland or X11 when Rinit runs: check `system`)
    ##   Cocoa : display = NSWindow*, window = CAMetalLayer* for Vulkan presentation
    ##   Emscripten: window = opaque browser surface token (no OS window handle)
    system*: RWindowSystem
    display*: pointer
    window*: uint
    visual*: uint
    transparent*: cint
      ## R_TRUE when the window was created with R_WINDOW_TRANSPARENT and the
      ## window system gave it an alpha channel: what is presented in it must
      ## carry alpha (premultiplied), which a rendering backend asks for when
      ## it creates its surface.
  REvent* {.bycopy.} = object
    ## Input event delivered to a widget. Mouse and key events bubble from the
    ## target widget to its parents until one of them handles the event.
    `type`*: REventType
    window*: Rid
    target*: Rid
      ## Widget the event was first sent to.
    x*: cfloat
      ## Cursor position in the window, in logical units.
    y*: cfloat
      ## Cursor position in the window, in logical units.
    local_x*: cfloat
      ## Cursor position relative to the widget receiving the event.
    local_y*: cfloat
      ## Cursor position relative to the widget receiving the event.
    delta_x*: cfloat
      ## Mouse movement since the previous event (R_EVENT_MOUSE_MOVE).
    delta_y*: cfloat
      ## Mouse movement since the previous event (R_EVENT_MOUSE_MOVE).
    wheel_x*: cfloat
      ## Scroll amount in lines, positive = up / left (R_EVENT_MOUSE_WHEEL).
    wheel_y*: cfloat
      ## Scroll amount in lines, positive = up / left (R_EVENT_MOUSE_WHEEL).
    button*: RMouseButton
    clicks*: cint
      ## 1 = single click, 2 = double click, ... (R_EVENT_MOUSE_DOWN).
    key*: RKey
    modifiers*: uint32
      ## Bitmask of RModifier.
    repeat*: cint
      ## R_TRUE when the key event comes from auto-repeat.
    codepoint*: uint32
    wheel_precise*: cint
      ## R_EVENT_MOUSE_WHEEL: R_TRUE when the wheel comes from a trackpad (or a
      ## mouse that scrolls smoothly). wheel_x, wheel_y are then small fractions of
      ## a line, a line being 48 logical units the fingers moved: what follows the
      ## fingers moves 48 units per line, and a canvas pans rather than zooms.
  RDropEvent* {.bycopy.} = object
    ## What a drop target receives. The files and the text are only known when
    ## the drop happens (R_DROP_DROPPED), and are valid during the callback.
    phase*: RDropPhase
    window*: Rid
    x*: cfloat
      ## Pointer position in the window, and relative to the target widget.
    y*: cfloat
      ## Pointer position in the window, and relative to the target widget.
    local_x*: cfloat
    local_y*: cfloat
    paths*: cstringArray
      ## Dropped files and folders, as UTF-8 paths.
    path_count*: cint
    text*: cstring
      ## Dropped text when it is not files, or NULL. For a drag inside the application: its data.
    drag_type*: cstring
      ## Drag inside the application (Rdrag_begin): its type, else NULL. Set in every phase.
    drag_source*: Rid
      ## Widget the drag started from, or R_INVALID_ID.
  RTouchSettings* {.bycopy.} = object
    ## How the library treats fingers and pens (Rtouch_set_settings, Rwindow_set_touch_settings).
    mouse_emulation*: cint
      ## R_TRUE (default): one finger drives the widgets as the mouse does: a tap clicks, a drag drags.
    slop*: cfloat
      ## How far a finger moves (logical units) before a press becomes a drag or a scroll. Default 8.
    long_press*: cfloat
      ## Seconds a finger rests before the press is a right click (the context menu); 0 turns it off. Default 0.5.
    inertia*: cint
      ## R_TRUE (default): flicked content keeps going and slows down.
    deceleration*: cfloat
      ## How fast it slows down, in logical units per second per second. Default 2400.
    gestures*: cint
      ## R_TRUE (default): two fingers pinch to zoom and pan.
    pen_mouse_emulation*: cint
      ## R_TRUE (default): the pen drives the widgets as the mouse does (its barrel button: the right button).
    pressure_curve*: cfloat
      ## The pressure given to the application is the one of the pen to this power: 1 (default) as it is, below 1 softer, above 1 harder.
    pressure_threshold*: cfloat
      ## Pressure (0..1, after the curve) the tip needs before it presses. Default 0.
    palm_rejection*: cint
      ## R_TRUE (default): fingers are ignored while a pen is near the screen, and half a second after (the palm resting on it).
    trackpad_gestures*: cint
      ## R_TRUE (default): the pinch and the rotation of a trackpad zoom what zooms, as two fingers on the screen do.
    swipe_navigation*: cint
      ## R_TRUE (default): three or more fingers swiped across a trackpad go back or forward (the side buttons of a mouse).
    swipe_distance*: cfloat
      ## How far (logical units) the fingers of a swipe go across before it navigates. Default 120.
  RTouchEvent* {.bycopy.} = object
    phase*: RTouchPhase
    window*: Rid
    target*: Rid
      ## Widget the finger came down on.
    id*: cint
      ## Number of the finger, the same from its down to its up.
    x*: cfloat
      ## Position in the window, and relative to the widget receiving the event.
    y*: cfloat
      ## Position in the window, and relative to the widget receiving the event.
    local_x*: cfloat
    local_y*: cfloat
    pressure*: cfloat
      ## 0..1; 1 when the screen does not tell.
    width*: cfloat
      ## Size of the contact, 0 when the screen does not tell.
    height*: cfloat
      ## Size of the contact, 0 when the screen does not tell.
    count*: cint
      ## Fingers on the window, this one included (0 after the last up).
    modifiers*: uint32
      ## Bitmask of RModifier.
  RPenEvent* {.bycopy.} = object
    phase*: RPenPhase
    window*: Rid
    target*: Rid
      ## Widget under the pen; during a stroke, the one it started on.
    x*: cfloat
    y*: cfloat
    local_x*: cfloat
    local_y*: cfloat
    pressure*: cfloat
      ## 0..1, after the pressure curve; 0 while hovering.
    raw_pressure*: cfloat
      ## As the pen gives it, 0..1.
    tilt_x*: cfloat
      ## Tilt of the pen in degrees, -90..90, toward the right and toward the user; 0 when upright or unknown.
    tilt_y*: cfloat
      ## Tilt of the pen in degrees, -90..90, toward the right and toward the user; 0 when upright or unknown.
    rotation*: cfloat
      ## Rotation around its axis in degrees, 0..360 (when the pen tells).
    eraser*: cint
      ## R_TRUE when the eraser end touches (or the eraser button is held).
    buttons*: uint32
      ## Bitmask of RPenButton.
    modifiers*: uint32
  RGestureEvent* {.bycopy.} = object
    phase*: RGesturePhase
    window*: Rid
    target*: Rid
      ## Widget under the center of the fingers when the gesture began.
    x*: cfloat
      ## Center of the fingers, in the window and relative to the widget receiving the event.
    y*: cfloat
      ## Center of the fingers, in the window and relative to the widget receiving the event.
    local_x*: cfloat
    local_y*: cfloat
    scale*: cfloat
      ## Distance between the fingers relative to when the gesture began (2 = twice as far apart).
    scale_delta*: cfloat
      ## The same since the previous event: what a zoom multiplies by.
    pan_x*: cfloat
      ## Movement of the center since the previous event.
    pan_y*: cfloat
      ## Movement of the center since the previous event.
    rotation*: cfloat
      ## Turn of the fingers since the gesture began, in degrees, clockwise.
    kind*: RGestureKind
      ## R_GESTURE_PINCH or R_GESTURE_SWIPE.
    device*: RGestureDevice
      ## R_GESTURE_DEVICE_TOUCHSCREEN or R_GESTURE_DEVICE_TRACKPAD.
    fingers*: cint
      ## Fingers of the gesture (2 for a pinch, 3 or more for a swipe; 0 when the system does not tell).
    total_x*: cfloat
      ## Movement of the center since the gesture began.
    total_y*: cfloat
      ## Movement of the center since the gesture began.
    rotation_delta*: cfloat
      ## The same turn as `rotation`, since the previous event.
    cancelled*: cint
      ## R_GESTURE_END: R_TRUE when the system took the gesture back (nothing must happen as for an end).
  RTileItem* {.bycopy.} = object
    ## What a tile shows, filled by the application when the tile is drawn.
    text*: cstring
      ## Name under the icon (valid until the callback returns).
    detail*: cstring
      ## Second line, dimmer, or NULL.
    badge*: cstring
      ## Small text at the top right of the tile, or NULL.
    icon*: Rid
      ## Icon texture (R_INVALID_ID: none), the region of it and its tint (white by default).
    uv*: array[4, cfloat]
    tint*: array[4, cfloat]
    icon_radius*: cfloat
    text_color*: array[4, cfloat]
      ## Color of the name; alpha 0 = the theme.
  RLanguageDesc* {.bycopy.} = object
    ## What the editor knows of a language. Every field but the name may be NULL.
    ## Lists of words are separated by spaces.
    name*: cstring
      ## "Rust". Compared without case.
    extensions*: cstring
      ## Extensions of its files, separated by ';': "rs".
    keywords*: cstring
      ## Words of the language itself: if, while, return...
    types*: cstring
      ## Names of types and of what is always there: int, String, print...
    constants*: cstring
      ## Values with a name: true, false, null...
    line_comment*: cstring
      ## What starts a comment that ends with its line: two slashes, a hash.
    block_comment_start*: cstring
      ## What opens and closes a comment that may span lines.
    block_comment_end*: cstring
    string_quotes*: cstring
      ## Characters that open and close a text on one line: a double quote, a single quote.
    multiline_strings*: cstring
      ## Marks of the texts that span lines, separated by spaces: three double
      ## quotes, a backquote. When what closes such a text is not what opens it,
      ## write both around three dots: R"(...)" . Nothing is escaped in those.
    directive*: cstring
      ## What starts a line for the preprocessor: "#".
    annotation*: cstring
      ## What starts an annotation: "@" (@Override), or "#[" up to its closing bracket.
    flags*: uint32
      ## Bitmask of RLanguageFlags.
    script_language*: cstring
      ## Markup with R_LANGUAGE_EMBEDS: the language of what is between
      ## <script> and </script> (the name of a language: "C", "Rust"...).
      ## NULL: JavaScript, as in HTML.
  RJumpTask* {.bycopy.} = object
    ## A task of the jump list of Windows (Rtaskbar_set_jump_list).
    title*: cstring
      ## What the list shows.
    arguments*: cstring
      ## The command line given to the application when it is chosen: it is started again with these arguments.
    description*: cstring
      ## The tooltip; NULL: none.
  RFileFilter* {.bycopy.} = object
    ## A kind of file in a file dialog.
    name*: cstring
      ## Shown to the user: "Images".
    patterns*: cstring
      ## Patterns separated by ';': "*.png;*.jpg". "*" matches everything.
  RFileDialogDesc* {.bycopy.} = object
    ## Every field may be left to zero.
    parent*: Rid
      ## Window the dialog belongs to. R_INVALID_ID: none (a window is opened when Raven needs one).
    title*: cstring
      ## NULL: "Open", "Save as"...
    directory*: cstring
      ## Folder shown first. NULL: the one of the previous dialog, else the home of the user.
    file_name*: cstring
      ## Name proposed to the user (save dialogs).
    filters*: ptr RFileFilter
    filter_count*: cint
    default_extension*: cstring
      ## Save dialogs: added to a name typed without extension ("xml").
    flags*: uint32
      ## Bitmask of RFileDialogFlags.
  RViewportDrawInfo* {.bycopy.} = object
    ## Passed to the draw callback of a viewport. Sizes are in pixels.
    backend*: RBackend
    viewport*: Rid
    window*: Rid
    x*: cint
      ## Area of the viewport in the window framebuffer.
    y*: cint
      ## Area of the viewport in the window framebuffer.
    width*: cint
      ## Area of the viewport in the window framebuffer.
    height*: cint
      ## Area of the viewport in the window framebuffer.
    clip_x*: cint
      ## Visible part of that area. Already set as the scissor rectangle.
    clip_y*: cint
      ## Visible part of that area. Already set as the scissor rectangle.
    clip_width*: cint
      ## Visible part of that area. Already set as the scissor rectangle.
    clip_height*: cint
      ## Visible part of that area. Already set as the scissor rectangle.
    framebuffer_width*: cint
    framebuffer_height*: cint
    frame_index*: uint64
    command_buffer*: pointer
      ## Vulkan: VkCommandBuffer being recorded, inside the render pass.
    render_pass*: pointer
      ## Vulkan: VkRenderPass in use (one color attachment, no depth).
    color_format*: uint32
      ## Vulkan: VkFormat of the color attachment.
  RSpring* {.bycopy.} = object
    ## State of one animated value. Zero-initialize it and keep it with the widget.
    value*: cfloat
    velocity*: cfloat
    started*: cint
  RShaderLook* {.bycopy.} = object
    ## How an effect is drawn. Start from Rshader_look_init.
    color*: array[4, cfloat]
      ## First and second color, 0..1, straight alpha.
    color2*: array[4, cfloat]
    params*: array[4, cfloat]
      ## Four numbers the effect reads as fx.params (their meaning is the effect's).
    radius*: cfloat
      ## Radius of the corners, in logical units.
    texture*: Rid
      ## Texture the effect reads with raven_texture(), or R_INVALID_ID.
    animate*: cint
      ## -1: animated when the shader is (R_SHADER_ANIMATED); R_TRUE / R_FALSE forces it.
    pointer*: cint
      ## R_TRUE (the default): the effect knows where the pointer is (fx.pointer, fx.hover, fx.press).
    hover*: cfloat
      ## How much the pointer is over it and how much it is pressed, 0..1, as
      ## the effect reads them (fx.hover, fx.press). -1 (the default): 1 while
      ## the pointer is in the rectangle, and while the left button is down
      ## there; a Border eases them itself. The rectangle is clipped to the
      ## visible painting area; overlaid siblings do not interrupt its hover.
    press*: cfloat
  RXmlError* {.bycopy.} = object
    ## Where and why a text could not be read (Rxml_document_parse).
    line*: cint
      ## Both start at 1.
    column*: cint
    message*: array[160, cchar]
  RXmlAttributeInfo* {.bycopy.} = object
    ## One attribute of a tag (Rxml_get_attribute_info). The strings belong to the library.
    name*: cstring
    `type`*: RXmlValueType
    choices*: cstring
      ## R_XML_CHOICE: the words, separated by '\|'. NULL otherwise.
    group*: cstring
      ## What it belongs to: the tag itself ("Button"), "Widget" for what every widget has, "Slot" for the place in the parent.
    description*: cstring
      ## One line saying what it does.
  RAccessibilitySettings* {.bycopy.} = object
    ## What the system asks for.
    text_scale*: cfloat
      ## Size of the texts asked by the user (1 = normal).
    high_contrast*: cint
    reduce_motion*: cint
    screen_reader*: cint
      ## R_TRUE when a screen reader is running.
  RHostPlatformDesc* {.bycopy.} = object
    set_cursor*: pointer
      ## The pointer over `window` should look like `cursor`. May be NULL.
    clipboard_set*: pointer
      ## Copies UTF-8 text to the clipboard of the application. NULL: the clipboard stays in the process.
    clipboard_get*: pointer
      ## The clipboard text, valid until the next call, or NULL. NULL: the clipboard stays in the process.
    wake*: pointer
      ## Raven has work to do (a timer, a posted task, a redraw): produce a frame soon. Thread-safe. May be NULL.
    user_data*: pointer

const
  RAVEN_VERSION_MAJOR*: cint = 0
  RAVEN_VERSION_MINOR*: cint = 2
  RAVEN_VERSION_PATCH*: cint = 3
  RAVEN_API_VERSION* = "0.2"
  R_FALSE*: cint = 0
  R_TRUE*: cint = 1
  R_INVALID_ID*: Rid = 0xFFFFFFFF'u32

  # RBackend
  R_BACKEND_VULKAN*: RBackend = 1
    ## Built-in Vulkan renderer.
  R_BACKEND_NULL*: RBackend = 2
    ## Renders nothing. Used for headless runs and unit tests.
  R_BACKEND_CUSTOM*: RBackend = 3
    ## Backend supplied with Rbackend_register (see raven_backend.h).
  R_BACKEND_WEBGL*: RBackend = 4
    ## Browser renderer (Emscripten, WebGL 2).

  # RError
  R_NO_ERROR*: RError = 0x0070
  R_ERROR_INVALID_ID*: RError = 0x0071
  R_ERROR_INVALID_ENUM*: RError = 0x0072
  R_ERROR_INVALID_VALUE*: RError = 0x0073
  R_ERROR_INVALID_OPERATION*: RError = 0x0074
  R_ERROR_BACKEND*: RError = 0x0075
  R_ERROR_PLATFORM*: RError = 0x0076
  R_ERROR_OUT_OF_MEMORY*: RError = 0x0077
  R_ERROR_INVALID_FILE_FORMAT*: RError = 0x0078
  R_ERROR_FILE_NOT_FOUND*: RError = 0x0079
  R_ERROR_XML*: RError = 0x007A
  R_ERROR_WRONG_THREAD*: RError = 0x007B

  # RSeverity
  R_SEVERITY_WEAK_WARNING*: RSeverity = 0x0080
  R_SEVERITY_WARNING*: RSeverity = 0x0081
  R_SEVERITY_ERROR*: RSeverity = 0x0082
  R_SEVERITY_FATAL*: RSeverity = 0x0083

  # RWindowFlags
  R_WINDOW_DEFAULT*: RWindowFlags = 0
  R_WINDOW_NO_RESIZE*: RWindowFlags = 1
  R_WINDOW_HIDDEN*: RWindowFlags = 2
  R_WINDOW_BORDERLESS*: RWindowFlags = 4
  R_WINDOW_MAXIMIZED*: RWindowFlags = 8
  R_WINDOW_CUSTOM_HEADER*: RWindowFlags = 0x0010
    ## No title bar from the OS: the content covers the whole window and
    ## draws its own header, with a R_WIDGET_HEADER_BAR (raven_shell.h) or
    ## with any widget given to Rwidget_set_window_drag. The window is still
    ## moved, resized, snapped and maximized by the OS, as it does for its own
    ## title bars.
  R_WINDOW_TRANSPARENT*: RWindowFlags = 0x0020
    ## The window can be seen through: where the application draws nothing
    ## opaque, what is behind the window on the screen shows (the desktop,
    ## the other applications), not a color of the application. Glass then
    ## looks at the screen itself. What the OS does to what shows through is
    ## chosen with Rwindow_set_backdrop. See RBackdrop for what each window
    ## system can do.
  R_WINDOW_SPLASH_SCREEN*: RWindowFlags = 0x0040
    ## A splash screen: shown while the application starts. No frame (it
    ## implies R_WINDOW_BORDERLESS and R_WINDOW_NO_RESIZE, and cancels
    ## R_WINDOW_MAXIMIZED and R_WINDOW_CUSTOM_HEADER), centered on the screen
    ## before it is first shown and again when its size changes, above the
    ## other windows, out of the taskbar and of Alt+Tab (Windows), of the
    ## Window menu and of Cmd+` (macOS), typed _NET_WM_WINDOW_TYPE_SPLASH on
    ## X11. Wayland gives it no place of its own: it is a window without frame
    ## there. The application destroys it once its main window is ready
    ## (creating the main window first keeps Ris_running true).

  # RPictureInPictureCorner
  R_PIP_BOTTOM_RIGHT*: RPictureInPictureCorner = 0x0780
  R_PIP_BOTTOM_LEFT*: RPictureInPictureCorner = 0x0781
  R_PIP_TOP_RIGHT*: RPictureInPictureCorner = 0x0782
  R_PIP_TOP_LEFT*: RPictureInPictureCorner = 0x0783

  # RBackdrop
  R_BACKDROP_NONE*: RBackdrop = 0x0400
    ## Opaque: the window color, or the wallpaper of the theme.
  R_BACKDROP_TRANSPARENT*: RBackdrop = 0x0401
    ## The screen behind the window, as it is.
  R_BACKDROP_BLUR*: RBackdrop = 0x0402
    ## The screen behind the window, blurred by the OS.
  R_BACKDROP_ACRYLIC*: RBackdrop = 0x0403
    ## Blurred, tinted and grained by the OS (the acrylic of Windows).
  R_BACKDROP_MICA*: RBackdrop = 0x0404
    ## A soft tint taken from the wallpaper of the desktop (the mica of Windows 11).

  # RWindowSystem
  R_WINDOW_SYSTEM_NONE*: RWindowSystem = 0x0090
  R_WINDOW_SYSTEM_WIN32*: RWindowSystem = 0x0091
  R_WINDOW_SYSTEM_X11*: RWindowSystem = 0x0092
  R_WINDOW_SYSTEM_WAYLAND*: RWindowSystem = 0x0093
  R_WINDOW_SYSTEM_COCOA*: RWindowSystem = 0x0094
  R_WINDOW_SYSTEM_EMSCRIPTEN*: RWindowSystem = 0x0095

  # RWindowEdge
  R_EDGE_LEFT*: RWindowEdge = 0x00D0
  R_EDGE_RIGHT*: RWindowEdge = 0x00D1
  R_EDGE_TOP*: RWindowEdge = 0x00D2
  R_EDGE_BOTTOM*: RWindowEdge = 0x00D3
  R_EDGE_TOP_LEFT*: RWindowEdge = 0x00D4
  R_EDGE_TOP_RIGHT*: RWindowEdge = 0x00D5
  R_EDGE_BOTTOM_LEFT*: RWindowEdge = 0x00D6
  R_EDGE_BOTTOM_RIGHT*: RWindowEdge = 0x00D7

  # RMouseButton
  R_MOUSE_LEFT*: RMouseButton = 0
  R_MOUSE_RIGHT*: RMouseButton = 1
  R_MOUSE_MIDDLE*: RMouseButton = 2
  R_MOUSE_X1*: RMouseButton = 3
  R_MOUSE_X2*: RMouseButton = 4

  # RModifier
  R_MOD_SHIFT*: RModifier = 1
  R_MOD_CTRL*: RModifier = 2
  R_MOD_ALT*: RModifier = 4
  R_MOD_SUPER*: RModifier = 8

  # RCursor
  R_CURSOR_ARROW*: RCursor = 0x00B0
  R_CURSOR_IBEAM*: RCursor = 0x00B1
  R_CURSOR_HAND*: RCursor = 0x00B2
  R_CURSOR_CROSSHAIR*: RCursor = 0x00B3
  R_CURSOR_RESIZE_H*: RCursor = 0x00B4
  R_CURSOR_RESIZE_V*: RCursor = 0x00B5
  R_CURSOR_RESIZE_NWSE*: RCursor = 0x00B6
  R_CURSOR_RESIZE_NESW*: RCursor = 0x00B7
  R_CURSOR_MOVE*: RCursor = 0x00B8
  R_CURSOR_NOT_ALLOWED*: RCursor = 0x00B9
  R_CURSOR_NONE*: RCursor = 0x00BA
    ## No pointer at all (a game that hides it). Windows; elsewhere the arrow.

  # RKey
  R_KEY_UNKNOWN*: RKey = 0
  R_KEY_SPACE*: RKey = 0x0020
  R_KEY_APOSTROPHE*: RKey = 0x0027
  R_KEY_COMMA*: RKey = 0x002C
  R_KEY_MINUS*: RKey = 0x002D
  R_KEY_PERIOD*: RKey = 0x002E
  R_KEY_SLASH*: RKey = 0x002F
  R_KEY_0*: RKey = 0x0030
  R_KEY_1*: RKey = 0x0031
  R_KEY_2*: RKey = 0x0032
  R_KEY_3*: RKey = 0x0033
  R_KEY_4*: RKey = 0x0034
  R_KEY_5*: RKey = 0x0035
  R_KEY_6*: RKey = 0x0036
  R_KEY_7*: RKey = 0x0037
  R_KEY_8*: RKey = 0x0038
  R_KEY_9*: RKey = 0x0039
  R_KEY_SEMICOLON*: RKey = 0x003B
  R_KEY_EQUAL*: RKey = 0x003D
  R_KEY_A*: RKey = 0x0041
  R_KEY_B*: RKey = 0x0042
  R_KEY_C*: RKey = 0x0043
  R_KEY_D*: RKey = 0x0044
  R_KEY_E*: RKey = 0x0045
  R_KEY_F*: RKey = 0x0046
  R_KEY_G*: RKey = 0x0047
  R_KEY_H*: RKey = 0x0048
  R_KEY_I*: RKey = 0x0049
  R_KEY_J*: RKey = 0x004A
  R_KEY_K*: RKey = 0x004B
  R_KEY_L*: RKey = 0x004C
  R_KEY_M*: RKey = 0x004D
  R_KEY_N*: RKey = 0x004E
  R_KEY_O*: RKey = 0x004F
  R_KEY_P*: RKey = 0x0050
  R_KEY_Q*: RKey = 0x0051
  R_KEY_R*: RKey = 0x0052
  R_KEY_S*: RKey = 0x0053
  R_KEY_T*: RKey = 0x0054
  R_KEY_U*: RKey = 0x0055
  R_KEY_V*: RKey = 0x0056
  R_KEY_W*: RKey = 0x0057
  R_KEY_X*: RKey = 0x0058
  R_KEY_Y*: RKey = 0x0059
  R_KEY_Z*: RKey = 0x005A
  R_KEY_LEFT_BRACKET*: RKey = 0x005B
  R_KEY_BACKSLASH*: RKey = 0x005C
  R_KEY_RIGHT_BRACKET*: RKey = 0x005D
  R_KEY_GRAVE*: RKey = 0x0060
  R_KEY_ESCAPE*: RKey = 0x0100
  R_KEY_ENTER*: RKey = 0x0101
  R_KEY_TAB*: RKey = 0x0102
  R_KEY_BACKSPACE*: RKey = 0x0103
  R_KEY_INSERT*: RKey = 0x0104
  R_KEY_DELETE*: RKey = 0x0105
  R_KEY_RIGHT*: RKey = 0x0106
  R_KEY_LEFT*: RKey = 0x0107
  R_KEY_DOWN*: RKey = 0x0108
  R_KEY_UP*: RKey = 0x0109
  R_KEY_PAGE_UP*: RKey = 0x010A
  R_KEY_PAGE_DOWN*: RKey = 0x010B
  R_KEY_HOME*: RKey = 0x010C
  R_KEY_END*: RKey = 0x010D
  R_KEY_CAPS_LOCK*: RKey = 0x010E
  R_KEY_PRINT_SCREEN*: RKey = 0x010F
  R_KEY_PAUSE*: RKey = 0x0110
  R_KEY_F1*: RKey = 0x0111
  R_KEY_F2*: RKey = 0x0112
  R_KEY_F3*: RKey = 0x0113
  R_KEY_F4*: RKey = 0x0114
  R_KEY_F5*: RKey = 0x0115
  R_KEY_F6*: RKey = 0x0116
  R_KEY_F7*: RKey = 0x0117
  R_KEY_F8*: RKey = 0x0118
  R_KEY_F9*: RKey = 0x0119
  R_KEY_F10*: RKey = 0x011A
  R_KEY_F11*: RKey = 0x011B
  R_KEY_F12*: RKey = 0x011C
  R_KEY_KP_0*: RKey = 0x011D
  R_KEY_KP_1*: RKey = 0x011E
  R_KEY_KP_2*: RKey = 0x011F
  R_KEY_KP_3*: RKey = 0x0120
  R_KEY_KP_4*: RKey = 0x0121
  R_KEY_KP_5*: RKey = 0x0122
  R_KEY_KP_6*: RKey = 0x0123
  R_KEY_KP_7*: RKey = 0x0124
  R_KEY_KP_8*: RKey = 0x0125
  R_KEY_KP_9*: RKey = 0x0126
  R_KEY_KP_DECIMAL*: RKey = 0x0127
  R_KEY_KP_DIVIDE*: RKey = 0x0128
  R_KEY_KP_MULTIPLY*: RKey = 0x0129
  R_KEY_KP_SUBTRACT*: RKey = 0x012A
  R_KEY_KP_ADD*: RKey = 0x012B
  R_KEY_KP_ENTER*: RKey = 0x012C
  R_KEY_LEFT_SHIFT*: RKey = 0x012D
  R_KEY_LEFT_CTRL*: RKey = 0x012E
  R_KEY_LEFT_ALT*: RKey = 0x012F
  R_KEY_LEFT_SUPER*: RKey = 0x0130
  R_KEY_RIGHT_SHIFT*: RKey = 0x0131
  R_KEY_RIGHT_CTRL*: RKey = 0x0132
  R_KEY_RIGHT_ALT*: RKey = 0x0133
  R_KEY_RIGHT_SUPER*: RKey = 0x0134
  R_KEY_MENU*: RKey = 0x0135
  R_KEY_COUNT*: RKey = 0x0136

  # REventType
  R_EVENT_NONE*: REventType = 0x00C0
  R_EVENT_MOUSE_MOVE*: REventType = 0x00C1
  R_EVENT_MOUSE_DOWN*: REventType = 0x00C2
  R_EVENT_MOUSE_UP*: REventType = 0x00C3
  R_EVENT_MOUSE_WHEEL*: REventType = 0x00C4
  R_EVENT_MOUSE_ENTER*: REventType = 0x00C5
  R_EVENT_MOUSE_LEAVE*: REventType = 0x00C6
  R_EVENT_KEY_DOWN*: REventType = 0x00C7
  R_EVENT_KEY_UP*: REventType = 0x00C8
  R_EVENT_TEXT*: REventType = 0x00C9
    ## A character was typed. `codepoint` holds the Unicode scalar value.
  R_EVENT_FOCUS_GAINED*: REventType = 0x00CA
  R_EVENT_FOCUS_LOST*: REventType = 0x00CB

  # RDropPhase
  R_DROP_ENTER*: RDropPhase = 0x00E0
    ## The pointer came over the target.
  R_DROP_MOVE*: RDropPhase = 0x00E1
    ## It moves over the target.
  R_DROP_LEAVE*: RDropPhase = 0x00E2
    ## It left the target without dropping.
  R_DROP_DROPPED*: RDropPhase = 0x00E3
    ## The user dropped: `paths` and `text` hold what was dropped.

  # RTouchMode
  R_TOUCH_MODE_AUTO*: RTouchMode = 0x0480
    ## The finger scrolls what scrolls under it, and is a mouse elsewhere (default).
  R_TOUCH_MODE_SCROLL*: RTouchMode = 0x0481
    ## A finger moving on the widget always scrolls what contains it, even over a control that drags.
  R_TOUCH_MODE_MOUSE*: RTouchMode = 0x0482
    ## The finger is a mouse on the widget: pressing presses at once, moving drags, nothing scrolls.
  R_TOUCH_MODE_NONE*: RTouchMode = 0x0483
    ## Fingers are ignored on the widget (only its touch callback sees them): a surface for the pen.

  # RTouchPhase
  R_TOUCH_DOWN*: RTouchPhase = 0x0490
    ## A finger came down.
  R_TOUCH_MOVE*: RTouchPhase = 0x0491
  R_TOUCH_UP*: RTouchPhase = 0x0492
    ## The finger was lifted.
  R_TOUCH_CANCEL*: RTouchPhase = 0x0493
    ## The system took the finger away (a gesture of its own, the window lost it): nothing must happen as for a lift.

  # RPenPhase
  R_PEN_DOWN*: RPenPhase = 0x04A0
    ## The tip touched the screen.
  R_PEN_MOVE*: RPenPhase = 0x04A1
    ## The pen moves while touching.
  R_PEN_UP*: RPenPhase = 0x04A2
    ## The tip left the screen.
  R_PEN_HOVER*: RPenPhase = 0x04A3
    ## The pen moves near the screen without touching it.
  R_PEN_LEAVE*: RPenPhase = 0x04A4
    ## The pen went out of reach.

  # RPenButton
  R_PEN_BUTTON_BARREL*: RPenButton = 1
  R_PEN_BUTTON_BARREL2*: RPenButton = 2

  # RGesturePhase
  R_GESTURE_BEGIN*: RGesturePhase = 0x04B0
    ## A second finger came down (a touch screen), or the system recognized a gesture (a trackpad).
  R_GESTURE_UPDATE*: RGesturePhase = 0x04B1
  R_GESTURE_END*: RGesturePhase = 0x04B2
    ## Fewer than two fingers remain, or the fingers left the trackpad (RGestureEvent::cancelled: the system took the gesture back).

  # RGestureKind
  R_GESTURE_PINCH*: RGestureKind = 0x04C0
    ## Two fingers that pinch, turn and move together: scale, rotation and pan.
  R_GESTURE_SWIPE*: RGestureKind = 0x04C1
    ## Three or more fingers that move together on a trackpad: only the pan
    ## (scale stays 1). By default, a swipe across goes back or forward.

  # RGestureDevice
  R_GESTURE_DEVICE_TOUCHSCREEN*: RGestureDevice = 0x04D0
    ## On the screen: the library recognized the gesture.
  R_GESTURE_DEVICE_TRACKPAD*: RGestureDevice = 0x04D1
    ## On a trackpad: the system recognized the gesture; the fingers are not on the window (x, y is the pointer).

  # RLook
  R_LOOK_RAVEN_DARK*: RLook = 0x00E0
    ## Raven, flat and dense, for tools. The look at start.
  R_LOOK_RAVEN_LIGHT*: RLook = 0x00E1
  R_LOOK_RAVEN_GLASS_DARK*: RLook = 0x00E2
    ## Raven in glass: controls are capsules of translucent glass that refract
    ## and blur what is behind them, catch the light on their rim and react
    ## to the pointer with short animations. Windows get a soft colored
    ## background unless they have their own (Rwindow_set_background,
    ## Rwindow_set_clear_color).
  R_LOOK_RAVEN_GLASS_LIGHT*: RLook = 0x00E3
  R_LOOK_RAVEN_HIGH_CONTRAST_DARK*: RLook = 0x00E4
    ## High contrast, for low vision: pure black and white, yellow (or blue)
    ## focus and selection, thick borders, larger text (raven_accessibility.h,
    ## Raccessibility_set_high_contrast).
  R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT*: RLook = 0x00E5
  R_LOOK_WINDOWS_DARK*: RLook = 0x00E6
    ## Windows 11 (Fluent): its controls, its colors, the accent color the
    ## user chose (on Windows), the Segoe UI font when the system has it, and
    ## motion without bounce.
  R_LOOK_WINDOWS_LIGHT*: RLook = 0x00E7
  R_LOOK_MACOS_DARK*: RLook = 0x00E8
    ## macOS (Sequoia, Aqua): its controls, its colors, the accent color the
    ## user chose (on macOS), the San Francisco font when the system has it,
    ## no change under the pointer, and selections in the accent color.
  R_LOOK_MACOS_LIGHT*: RLook = 0x00E9
  R_LOOK_GNOME_DARK*: RLook = 0x00EA
    ## GNOME (libadwaita), the desktop of most Linux distributions: flat
    ## rounded controls, the accent and the font of the desktop (on Linux).
  R_LOOK_GNOME_LIGHT*: RLook = 0x00EB
  R_LOOK_NATIVE_DARK*: RLook = 0x00EC
    ## The look of the platform the application runs on: Windows on Windows,
    ## macOS on macOS (macOS Glass from macOS 26 on), GNOME on Linux and the
    ## other systems.
  R_LOOK_NATIVE_LIGHT*: RLook = 0x00ED
  R_LOOK_MACOS_GLASS_DARK*: RLook = 0x00EE
    ## macOS 26 (Tahoe, Liquid Glass): the controls of macOS, larger and
    ## rounder, and glass where the system puts it, on what floats above the
    ## content: header bars and their buttons, menus, drop-down lists,
    ## tooltips, sheets; the knobs of sliders and switches turn into a lens
    ## of clear glass while they are held. The content itself (fields, lists,
    ## text) stays opaque. The accent color and the font of the system, on
    ## macOS.
  R_LOOK_MACOS_GLASS_LIGHT*: RLook = 0x00EF

  # RTheme
  R_THEME_DARK*: RTheme = 0x00E0
    ## Flat, dense themes for tools.
  R_THEME_LIGHT*: RTheme = 0x00E1
  R_THEME_GLASS_DARK*: RTheme = 0x00E2
    ## Glass themes: controls are capsules of translucent glass that refract
    ## and blur what is behind them, catch the light on their rim and react
    ## to the pointer with short animations. Windows get a soft colored
    ## background unless they have their own (Rwindow_set_background,
    ## Rwindow_set_clear_color).
  R_THEME_GLASS_LIGHT*: RTheme = 0x00E3
  R_THEME_HIGH_CONTRAST_DARK*: RTheme = 0x00E4
    ## High contrast, for low vision: pure black and white, yellow (or blue)
    ## focus and selection, thick borders, larger text (raven_accessibility.h,
    ## Raccessibility_set_high_contrast).
  R_THEME_HIGH_CONTRAST_LIGHT*: RTheme = 0x00E5

  # RMaterial
  R_MATERIAL_SOLID*: RMaterial = 0x02E0
    ## Opaque: filled with the colors of the theme. The material of R_THEME_DARK and R_THEME_LIGHT.
  R_MATERIAL_GLASS*: RMaterial = 0x02E1
    ## Clear glass: what is behind stays sharp enough to be recognized, strongly bent at the rim.
  R_MATERIAL_FROSTED_GLASS*: RMaterial = 0x02E2
    ## Frosted glass: what is behind is blurred into soft colors. The material of the glass themes.
  R_MATERIAL_TINTED_GLASS*: RMaterial = 0x02E3
    ## Frosted glass colored by the accent color of the theme (or of the widget): for what must stand out.
  R_MATERIAL_DEFAULT*: RMaterial = 0x02E4
    ## Not a material: "the one of the theme". Removes the material given to a widget.

  # RColorRole
  R_COLOR_WINDOW*: RColorRole = 0x0100
    ## Window clear color.
  R_COLOR_PANEL*: RColorRole = 0x0101
    ## Border background.
  R_COLOR_BORDER*: RColorRole = 0x0102
    ## Border outline, list and input outlines.
  R_COLOR_TEXT*: RColorRole = 0x0103
    ## Text. Inherited by the children of the widget it is set on.
  R_COLOR_TEXT_DISABLED*: RColorRole = 0x0104
  R_COLOR_TEXT_PLACEHOLDER*: RColorRole = 0x0105
  R_COLOR_ACCENT*: RColorRole = 0x0106
    ## Primary buttons, checked boxes, slider fill, progress fill.
  R_COLOR_ACCENT_HOVERED*: RColorRole = 0x0107
  R_COLOR_ACCENT_PRESSED*: RColorRole = 0x0108
  R_COLOR_ACCENT_TEXT*: RColorRole = 0x0109
    ## Text and marks drawn on top of the accent color.
  R_COLOR_CONTROL*: RColorRole = 0x010A
    ## Button, combo box, check box background.
  R_COLOR_CONTROL_HOVERED*: RColorRole = 0x010B
  R_COLOR_CONTROL_PRESSED*: RColorRole = 0x010C
  R_COLOR_CONTROL_BORDER*: RColorRole = 0x010D
  R_COLOR_INPUT*: RColorRole = 0x010E
    ## Text input, spin box, list and tree background.
  R_COLOR_FOCUS*: RColorRole = 0x010F
    ## Keyboard focus outline.
  R_COLOR_SELECTION*: RColorRole = 0x0110
    ## Selected rows and selected text.
  R_COLOR_ROW_HOVERED*: RColorRole = 0x0111
  R_COLOR_TRACK*: RColorRole = 0x0112
    ## Slider track, progress background, scrollbar track.
  R_COLOR_HANDLE*: RColorRole = 0x0113
    ## Scrollbar thumb, slider handle.
  R_COLOR_HANDLE_HOVERED*: RColorRole = 0x0114
  R_COLOR_SEPARATOR*: RColorRole = 0x0115
  R_COLOR_HEADER*: RColorRole = 0x0116
    ## Menu bar, tab bar, expander header.
  R_COLOR_HEADER_HOVERED*: RColorRole = 0x0117
  R_COLOR_TAB_ACTIVE*: RColorRole = 0x0118
    ## Active tab.
  R_COLOR_POPUP*: RColorRole = 0x0119
    ## Menus, combo box lists.
  R_COLOR_POPUP_BORDER*: RColorRole = 0x011A
  R_COLOR_SHADOW*: RColorRole = 0x011B
  R_COLOR_TOOLTIP*: RColorRole = 0x011C
  R_COLOR_TOOLTIP_TEXT*: RColorRole = 0x011D
  R_COLOR_SWITCH*: RColorRole = 0x011E
    ## Track of a switch that is on.
  R_COLOR_SUCCESS*: RColorRole = 0x011F
    ## What went well, what needs attention, what is wrong or destroys something (markers, badges, classes).
  R_COLOR_WARNING*: RColorRole = 0x0120
  R_COLOR_DANGER*: RColorRole = 0x0121
  R_COLOR_LINK*: RColorRole = 0x0122
    ## Links of formatted text.
  R_COLOR_CODE*: RColorRole = 0x0123
    ## Background of code in formatted text.

  # RMetric
  R_METRIC_FONT_SIZE*: RMetric = 0x0140
  R_METRIC_CORNER_RADIUS*: RMetric = 0x0141
  R_METRIC_BORDER_WIDTH*: RMetric = 0x0142
  R_METRIC_CONTROL_HEIGHT*: RMetric = 0x0143
    ## Height of buttons, inputs, combo boxes, tabs, menu bars.
  R_METRIC_PADDING_X*: RMetric = 0x0144
    ## Horizontal / vertical padding inside controls.
  R_METRIC_PADDING_Y*: RMetric = 0x0145
  R_METRIC_SPACING*: RMetric = 0x0146
    ## Gap between an icon or a check mark and its text.
  R_METRIC_SCROLLBAR_SIZE*: RMetric = 0x0147
  R_METRIC_ROW_HEIGHT*: RMetric = 0x0148
    ## Height of list, tree and menu rows.
  R_METRIC_INDENT*: RMetric = 0x0149
    ## Indentation per tree level.
  R_METRIC_SPLITTER_SIZE*: RMetric = 0x014A
  R_METRIC_CHECK_SIZE*: RMetric = 0x014B
  R_METRIC_SLIDER_HANDLE_SIZE*: RMetric = 0x014C
  R_METRIC_POPUP_RADIUS*: RMetric = 0x014D
    ## Corner radius of menus, drop-down lists and tooltips.
  R_METRIC_GLASS_FROST*: RMetric = 0x014E
    ## The glass of the theme: each one multiplies what the material gives,
    ## 1 = unchanged. Frost (how much is blurred), refraction (how far the rim
    ## bends what is behind), saturation (of what is seen through), rim (the
    ## light it catches) and shadow (the one it drops).
  R_METRIC_GLASS_REFRACTION*: RMetric = 0x014F
  R_METRIC_GLASS_SATURATION*: RMetric = 0x0150
  R_METRIC_GLASS_RIM*: RMetric = 0x0151
  R_METRIC_GLASS_SHADOW*: RMetric = 0x0152
  R_METRIC_BLUR_RADIUS*: RMetric = 0x0153
    ## Radius of the blur of a BackgroundBlur that was not given one.

  # RFontStyle
  R_FONT_REGULAR*: RFontStyle = 0x0420
  R_FONT_BOLD*: RFontStyle = 0x0421
  R_FONT_ITALIC*: RFontStyle = 0x0422
  R_FONT_BOLD_ITALIC*: RFontStyle = 0x0423

  # RFilterType
  R_FILTER_LINEAR*: RFilterType = 0x02C0
  R_FILTER_NEAREST*: RFilterType = 0x02C1
    ## No smoothing: keeps pixel art sharp.
  R_FILTER_MIPMAP*: RFilterType = 0x02C2
    ## Smooth, with mipmaps: an image shown much smaller than its size stays
    ## clean instead of shimmering, for a third more GPU memory. The default
    ## of the textures made from image files (Rtexture_create...). Backends
    ## without mipmaps treat it as R_FILTER_LINEAR.

  # RWidgetType
  R_WIDGET_VBOX*: RWidgetType = 0x0200
    ## Stacks children top to bottom. Slate: SVerticalBox.
  R_WIDGET_HBOX*: RWidgetType = 0x0201
    ## Stacks children left to right. Slate: SHorizontalBox.
  R_WIDGET_GRID*: RWidgetType = 0x0202
    ## Rows and columns with spans and fill coefficients. Slate: SGridPanel.
  R_WIDGET_UNIFORM_GRID*: RWidgetType = 0x0203
    ## Grid whose cells all have the same size. Slate: SUniformGridPanel.
  R_WIDGET_OVERLAY*: RWidgetType = 0x0204
    ## Draws children on top of each other. Slate: SOverlay.
  R_WIDGET_BORDER*: RWidgetType = 0x0205
    ## One child with a background, an outline and inner padding. Slate: SBorder.
  R_WIDGET_SIZE_BOX*: RWidgetType = 0x0206
    ## One child with size constraints or a fixed aspect ratio. Slate: SBox.
  R_WIDGET_SPACER*: RWidgetType = 0x0207
    ## Empty space. Slate: SSpacer.
  R_WIDGET_SCROLL_BOX*: RWidgetType = 0x0208
    ## Scrollable stack of children. Slate: SScrollBox.
  R_WIDGET_SPLITTER*: RWidgetType = 0x0209
    ## Children separated by draggable handles. Slate: SSplitter.
  R_WIDGET_WRAP_BOX*: RWidgetType = 0x020A
    ## Flows children and wraps to the next line. Slate: SWrapBox.
  R_WIDGET_CANVAS*: RWidgetType = 0x020B
    ## Children placed with anchors and offsets. Slate: SConstraintCanvas.
  R_WIDGET_SWITCHER*: RWidgetType = 0x020C
    ## Shows one child at a time. Slate: SWidgetSwitcher.
  R_WIDGET_TAB_VIEW*: RWidgetType = 0x020D
    ## Tab bar + one page per child.
  R_WIDGET_EXPANDER*: RWidgetType = 0x020E
    ## Collapsible header + one child. Slate: SExpandableArea.
  R_WIDGET_LABEL*: RWidgetType = 0x020F
  R_WIDGET_BUTTON*: RWidgetType = 0x0210
  R_WIDGET_CHECKBOX*: RWidgetType = 0x0211
  R_WIDGET_SLIDER*: RWidgetType = 0x0212
  R_WIDGET_PROGRESSBAR*: RWidgetType = 0x0213
  R_WIDGET_IMAGE*: RWidgetType = 0x0214
  R_WIDGET_TEXT_INPUT*: RWidgetType = 0x0215
  R_WIDGET_SPIN_BOX*: RWidgetType = 0x0216
  R_WIDGET_COMBO_BOX*: RWidgetType = 0x0217
  R_WIDGET_SEPARATOR*: RWidgetType = 0x0218
  R_WIDGET_LIST_VIEW*: RWidgetType = 0x0219
  R_WIDGET_TREE_VIEW*: RWidgetType = 0x021A
  R_WIDGET_MENU_BAR*: RWidgetType = 0x021B
  R_WIDGET_MENU*: RWidgetType = 0x021C
  R_WIDGET_MENU_ITEM*: RWidgetType = 0x021D
  R_WIDGET_VIEWPORT*: RWidgetType = 0x021E
    ## Area showing content rendered by the application.
  R_WIDGET_CUSTOM*: RWidgetType = 0x021F
    ## Widget painted and measured by application callbacks.
  R_WIDGET_TEXT_EDITOR*: RWidgetType = 0x0220
    ## Multi-line text editor.
  R_WIDGET_HEADER_BAR*: RWidgetType = 0x0221
    ## Title bar of a window with a custom header, holding any widget.
  R_WIDGET_DOCK_AREA*: RWidgetType = 0x0222
    ## Panels docked side by side or behind tabs, that the user rearranges.
  R_WIDGET_ISLAND*: RWidgetType = 0x0223
    ## A capsule that unfolds into a panel, at the top of a window.
  R_WIDGET_FILE_BROWSER*: RWidgetType = 0x0224
    ## Explorer of the files of the computer.
  R_WIDGET_BACKGROUND_BLUR*: RWidgetType = 0x0225
    ## One child over a blurred view of what is behind it.
  R_WIDGET_COLOR_PICKER*: RWidgetType = 0x0226
    ## Chooses a color: hue, saturation, value, opacity, by hand or by numbers.
  R_WIDGET_TIMELINE*: RWidgetType = 0x0227
    ## Tracks of clips and keys along a time ruler.
  R_WIDGET_NODE_GRAPH*: RWidgetType = 0x0228
    ## Nodes linked by wires, on a canvas that pans and zooms.
  R_WIDGET_TILE_VIEW*: RWidgetType = 0x0229
    ## Grid of tiles (an icon over a name), virtualized. Slate: STileView.
  R_WIDGET_BLOCK_EDITOR*: RWidgetType = 0x022A
    ## Blocks that snap into each other, in the style of Scratch.
  R_WIDGET_TABLE*: RWidgetType = 0x022B
    ## Rows of cells under columns that can be resized and sorted, virtualized.
  R_WIDGET_DATE_PICKER*: RWidgetType = 0x022C
    ## A date, chosen in a calendar that drops down.
  R_WIDGET_TIME_PICKER*: RWidgetType = 0x022D
    ## A time of the day: hours, minutes, optionally seconds.
  R_WIDGET_TOAST*: RWidgetType = 0x022E
    ## A short message that rises for a few seconds, with an optional action ("Undo").
  R_WIDGET_BREADCRUMB*: RWidgetType = 0x022F
    ## The path to the current place: links separated by chevrons.
  R_WIDGET_RATING*: RWidgetType = 0x0230
    ## A score shown and chosen with stars.
  R_WIDGET_PAGINATION*: RWidgetType = 0x0231
    ## Page numbers with previous and next.
  R_WIDGET_PLOT*: RWidgetType = 0x0232
    ## Curves of X / Y data: legend, line styles, markers, log axes, a second X and Y axis, pan and zoom.

  # RHAlign
  R_HALIGN_FILL*: RHAlign = 0x0240
  R_HALIGN_LEFT*: RHAlign = 0x0241
  R_HALIGN_CENTER*: RHAlign = 0x0242
  R_HALIGN_RIGHT*: RHAlign = 0x0243

  # RVAlign
  R_VALIGN_FILL*: RVAlign = 0x0250
  R_VALIGN_TOP*: RVAlign = 0x0251
  R_VALIGN_CENTER*: RVAlign = 0x0252
  R_VALIGN_BOTTOM*: RVAlign = 0x0253

  # RVisibility
  R_VISIBLE*: RVisibility = 0x0260
    ## Drawn and laid out.
  R_HIDDEN*: RVisibility = 0x0261
    ## Not drawn, but still takes its space in the layout.
  R_COLLAPSED*: RVisibility = 0x0262
    ## Not drawn and takes no space.

  # RTransition
  R_TRANSITION_NONE*: RTransition = 0x0300
  R_TRANSITION_FADE*: RTransition = 0x0301
    ## Becomes opaque.
  R_TRANSITION_POP*: RTransition = 0x0302
    ## Grows from a smaller size and bounces a little past its own.
  R_TRANSITION_ZOOM*: RTransition = 0x0303
    ## Shrinks from a larger size.
  R_TRANSITION_FLY_UP*: RTransition = 0x0304
    ## Flies in from below / above / the right / the left, and settles with a bounce.
  R_TRANSITION_FLY_DOWN*: RTransition = 0x0305
  R_TRANSITION_FLY_LEFT*: RTransition = 0x0306
  R_TRANSITION_FLY_RIGHT*: RTransition = 0x0307
  R_TRANSITION_DROP*: RTransition = 0x0308
    ## Falls from above, squashes on landing and wobbles.
  R_TRANSITION_SLIDE_LEFT*: RTransition = 0x0309
    ## Short slide from the right / from the left, without bounce: pages that replace one another.
  R_TRANSITION_SLIDE_RIGHT*: RTransition = 0x030A

  # ROrientation
  R_HORIZONTAL*: ROrientation = 0x0270
  R_VERTICAL*: ROrientation = 0x0271

  # RBlurStyle
  R_BLUR_PLAIN*: RBlurStyle = 0x0440
    ## What is behind, out of focus, and nothing else (a Gaussian blur).
  R_BLUR_FROSTED*: RBlurStyle = 0x0441
    ## Frosted: blurred, its colors a little stronger, under a veil of the
    ## panel color of the theme, a fine grain and a hairline. The default.
  R_BLUR_GLASS*: RBlurStyle = 0x0442
    ## Glass: a slab that bends what is behind at its rim and catches the
    ## light, as the glass materials do (RMaterial); the radius says how
    ## frosted it is.
  R_BLUR_NATIVE*: RBlurStyle = 0x0443
    ## Native: the widget opens the window there, and what shows is what the
    ## OS puts behind the window (Rwindow_set_backdrop: the blurred desktop,
    ## acrylic, mica), under a light veil. It needs a window created with
    ## R_WINDOW_TRANSPARENT that the OS could make see-through; anywhere
    ## else it is drawn as R_BLUR_FROSTED. The radius is the one of the OS.

  # RDirection
  R_DIRECTION_INHERIT*: RDirection = 0
    ## A widget: the direction of its parent (the default).
  R_DIRECTION_LTR*: RDirection = 0x0720
  R_DIRECTION_RTL*: RDirection = 0x0721
  R_DIRECTION_AUTO*: RDirection = 0x0722
    ## The windows: the direction of the language.

  # RTextAlign
  R_TEXT_ALIGN_LEFT*: RTextAlign = 0x0280
  R_TEXT_ALIGN_CENTER*: RTextAlign = 0x0281
  R_TEXT_ALIGN_RIGHT*: RTextAlign = 0x0282

  # RToastKind
  R_TOAST_INFO*: RToastKind = 0x04E0
    ## A neutral message (the accent of the theme).
  R_TOAST_SUCCESS*: RToastKind = 0x04E1
  R_TOAST_WARNING*: RToastKind = 0x04E2
  R_TOAST_ERROR*: RToastKind = 0x04E3

  # RToastQueue
  R_TOAST_REPLACE*: RToastQueue = 0x0500
    ## The new message takes the place of the one shown (the default).
  R_TOAST_SEQUENCE*: RToastQueue = 0x0501
    ## The messages are shown one after the other, each for its own time.
  R_TOAST_STACK*: RToastQueue = 0x0502
    ## Up to Rtoast_set_max_visible messages are shown at once, one above the other; the others wait.

  # RButtonStyle
  R_BUTTON_DEFAULT*: RButtonStyle = 0x0290
    ## Filled with R_COLOR_CONTROL.
  R_BUTTON_PRIMARY*: RButtonStyle = 0x0291
    ## Filled with R_COLOR_ACCENT.
  R_BUTTON_FLAT*: RButtonStyle = 0x0292
    ## No background until hovered (toolbars).

  # RCheckboxStyle
  R_CHECKBOX_CHECK*: RCheckboxStyle = 0x02A0
  R_CHECKBOX_RADIO*: RCheckboxStyle = 0x02A1
    ## Round mark. Checking one unchecks the other radio boxes of the same parent.
  R_CHECKBOX_SWITCH*: RCheckboxStyle = 0x02A2
    ## On / off switch.

  # RStretch
  R_STRETCH_FILL*: RStretch = 0x02B0
    ## Stretches the image to the widget, ignoring its aspect ratio.
  R_STRETCH_FIT*: RStretch = 0x02B1
    ## Largest size that fits entirely, keeping the aspect ratio.
  R_STRETCH_COVER*: RStretch = 0x02B2
    ## Smallest size that covers the widget, keeping the aspect ratio (cropped).
  R_STRETCH_NONE*: RStretch = 0x02B3
    ## Original size, centered.

  # RPlotAxis
  R_PLOT_X*: RPlotAxis = 0x0520
    ## The horizontal axis under the data.
  R_PLOT_Y*: RPlotAxis = 0x0521
    ## The vertical axis on the left.
  R_PLOT_X2*: RPlotAxis = 0x0522
    ## A second horizontal axis, over the data: shown when a series or a drag line uses it.
  R_PLOT_Y2*: RPlotAxis = 0x0523
    ## A second vertical axis, on the right: shown when a series or a drag line uses it.

  # RPlotScale
  R_PLOT_SCALE_LINEAR*: RPlotScale = 0x0530
  R_PLOT_SCALE_LOG*: RPlotScale = 0x0531
    ## Powers of ten at even distances; only values above zero are shown.
  R_PLOT_SCALE_SYMLOG*: RPlotScale = 0x0532
    ## Symmetric logarithm: linear between -t and t (Rplot_set_axis_linear_threshold,
    ## 1 by default), logarithmic beyond on both sides: shows zero and the
    ## negative values a logarithm cannot.

  # RPlotLine
  R_PLOT_LINE_SOLID*: RPlotLine = 0x0540
  R_PLOT_LINE_DASHED*: RPlotLine = 0x0541
  R_PLOT_LINE_DOTTED*: RPlotLine = 0x0542
  R_PLOT_LINE_DASH_DOT*: RPlotLine = 0x0543
  R_PLOT_LINE_NONE*: RPlotLine = 0x0544
    ## No line: only the markers (a scatter plot).

  # RPlotMarker
  R_PLOT_MARKER_NONE*: RPlotMarker = 0x0550
  R_PLOT_MARKER_CIRCLE*: RPlotMarker = 0x0551
  R_PLOT_MARKER_SQUARE*: RPlotMarker = 0x0552
  R_PLOT_MARKER_DIAMOND*: RPlotMarker = 0x0553
  R_PLOT_MARKER_TRIANGLE_UP*: RPlotMarker = 0x0554
  R_PLOT_MARKER_TRIANGLE_DOWN*: RPlotMarker = 0x0555
  R_PLOT_MARKER_CROSS*: RPlotMarker = 0x0556
  R_PLOT_MARKER_PLUS*: RPlotMarker = 0x0557
  R_PLOT_MARKER_ASTERISK*: RPlotMarker = 0x0558

  # RPlotLegend
  R_PLOT_LEGEND_HIDDEN*: RPlotLegend = 0x0560
  R_PLOT_LEGEND_TOP_LEFT*: RPlotLegend = 0x0561
  R_PLOT_LEGEND_TOP*: RPlotLegend = 0x0562
  R_PLOT_LEGEND_TOP_RIGHT*: RPlotLegend = 0x0563
  R_PLOT_LEGEND_RIGHT*: RPlotLegend = 0x0564
  R_PLOT_LEGEND_BOTTOM_RIGHT*: RPlotLegend = 0x0565
  R_PLOT_LEGEND_BOTTOM*: RPlotLegend = 0x0566
  R_PLOT_LEGEND_BOTTOM_LEFT*: RPlotLegend = 0x0567
  R_PLOT_LEGEND_LEFT*: RPlotLegend = 0x0568
  R_PLOT_LEGEND_OUTSIDE*: RPlotLegend = 0x0569
    ## To the right of the data, outside it.

  # RSyntax
  R_SYNTAX_NONE*: RSyntax = 0x0340
  R_SYNTAX_XML*: RSyntax = 0x0341
  R_SYNTAX_C*: RSyntax = 0x0342

  # RLanguageFlags
  R_LANGUAGE_MARKUP*: RLanguageFlags = 1
    ## Tags and attributes instead of code: XML, HTML. The words and the comments of the description are not used.
  R_LANGUAGE_STYLE_SHEET*: RLanguageFlags = 2
    ## Selectors, properties and values: CSS.
  R_LANGUAGE_NESTED_COMMENTS*: RLanguageFlags = 4
    ## A block comment may hold another one (Rust).
  R_LANGUAGE_CHAR_LITERALS*: RLanguageFlags = 8
    ## Between single quotes there is one character; any other single quote is not a text (C, Java, Rust).
  R_LANGUAGE_INDENT_ON_COLON*: RLanguageFlags = 0x0010
    ## A line that ends with ':' indents the next one (Python).
  R_LANGUAGE_IGNORE_CASE*: RLanguageFlags = 0x0020
    ## The words of the language are recognized whatever their case.
  R_LANGUAGE_EMBEDS*: RLanguageFlags = 0x0040
    ## Markup that holds JavaScript in script elements and CSS in style elements (HTML).
  R_LANGUAGE_FORMATTED_TEXT*: RLanguageFlags = 0x0080
    ## Formatted text written in Markdown: titles, strong and emphasized
    ## text, code, links, lists. The editor shows the text as it is written,
    ## marks included, with strong text in bold, emphasized text in italic
    ## and titles in bold: what will be formatted is seen while typing.

  # RToken
  R_TOKEN_TEXT*: RToken = 0x03A0
  R_TOKEN_COMMENT*: RToken = 0x03A1
  R_TOKEN_STRING*: RToken = 0x03A2
  R_TOKEN_NUMBER*: RToken = 0x03A3
  R_TOKEN_KEYWORD*: RToken = 0x03A4
  R_TOKEN_TYPE*: RToken = 0x03A5
    ## A type, or a name that is always there.
  R_TOKEN_FUNCTION*: RToken = 0x03A6
    ## A name followed by an opening parenthesis.
  R_TOKEN_TAG*: RToken = 0x03A7
    ## The name of an element, in markup and in the selectors of a style sheet.
  R_TOKEN_ATTRIBUTE*: RToken = 0x03A8
    ## The name of an attribute (markup), of a class or of a property (style sheets), of a key (JSON).
  R_TOKEN_PUNCTUATION*: RToken = 0x03A9
  R_TOKEN_DIRECTIVE*: RToken = 0x03AA
    ## A line for the preprocessor, an annotation, a declaration of a document.
  R_TOKEN_HEADING*: RToken = 0x03AB
    ## Formatted text (the Markdown language): a title, strong text, emphasized text, a link.
  R_TOKEN_STRONG*: RToken = 0x03AC
  R_TOKEN_EMPHASIS*: RToken = 0x03AD
  R_TOKEN_LINK*: RToken = 0x03AE

  # RMarkerKind
  R_MARKER_ERROR*: RMarkerKind = 0x0350
  R_MARKER_WARNING*: RMarkerKind = 0x0351
  R_MARKER_INFO*: RMarkerKind = 0x0352

  # RHeaderButtons
  R_HEADER_NO_BUTTONS*: RHeaderButtons = 0
  R_HEADER_CLOSE*: RHeaderButtons = 1
  R_HEADER_MINIMIZE*: RHeaderButtons = 2
  R_HEADER_MAXIMIZE*: RHeaderButtons = 4
  R_HEADER_ALL_BUTTONS*: RHeaderButtons = 7

  # RTaskbarProgress
  R_TASKBAR_PROGRESS_NONE*: RTaskbarProgress = 0x07A0
  R_TASKBAR_PROGRESS_NORMAL*: RTaskbarProgress = 0x07A1
  R_TASKBAR_PROGRESS_PAUSED*: RTaskbarProgress = 0x07A2
    ## Shown as paused (yellow on Windows).
  R_TASKBAR_PROGRESS_ERROR*: RTaskbarProgress = 0x07A3
    ## Shown as failed (red on Windows).
  R_TASKBAR_PROGRESS_INDETERMINATE*: RTaskbarProgress = 0x07A4
    ## Something goes on, without knowing how far it is.

  # RDockSide
  R_DOCK_LEFT*: RDockSide = 0x0360
  R_DOCK_RIGHT*: RDockSide = 0x0361
  R_DOCK_TOP*: RDockSide = 0x0362
  R_DOCK_BOTTOM*: RDockSide = 0x0363
  R_DOCK_TAB*: RDockSide = 0x0364
    ## Behind the same tabs as the other panel.

  # RVisualPreset
  R_PRESET_AUTO*: RVisualPreset = 0x0510
    ## The look of the theme of the window: its platform, dark or light.
  R_PRESET_MODERN*: RVisualPreset = 0x0511
    ## Cards with soft shadows, few lines (follows dark or light).
  R_PRESET_BLUEPRINT*: RVisualPreset = 0x0512
    ## Headers in gradients, a grid of lines, thick links that glow; pins of the type "exec" are arrows.
  R_PRESET_BLENDER*: RVisualPreset = 0x0513
    ## Compact nodes, flat headers in muted colors, thin links.
  R_PRESET_NEON*: RVisualPreset = 0x0514
    ## Almost black, outlines and links of light.

  # RTimelineItem
  R_TIMELINE_NONE*: RTimelineItem = 0x0460
  R_TIMELINE_CLIP*: RTimelineItem = 0x0461
  R_TIMELINE_KEY*: RTimelineItem = 0x0462
  R_TIMELINE_MARKER*: RTimelineItem = 0x0463

  # RBlockShape
  R_BLOCK_STATEMENT*: RBlockShape = 0x0700
    ## A step: snaps under another one, others snap under it.
  R_BLOCK_HAT*: RBlockShape = 0x0701
    ## Starts a script ("when the game starts"): nothing goes above it.
  R_BLOCK_CAP*: RBlockShape = 0x0702
    ## Ends a script ("stop"): nothing goes under it.
  R_BLOCK_C*: RBlockShape = 0x0703
    ## Wraps stacks in its mouths ("repeat", "if ... else").
  R_BLOCK_REPORTER*: RBlockShape = 0x0704
    ## A rounded value that goes into a slot.
  R_BLOCK_BOOLEAN*: RBlockShape = 0x0705
    ## A hexagonal condition that goes into a %b slot (or a %r one).

  # RBlockPlace
  R_BLOCK_NEXT*: RBlockPlace = 0x0710
  R_BLOCK_BODY*: RBlockPlace = 0x0711
  R_BLOCK_INPUT*: RBlockPlace = 0x0712

  # RNotificationUrgency
  R_NOTIFICATION_LOW*: RNotificationUrgency = 0x04F0
    ## No sound; may only go to the list of notifications.
  R_NOTIFICATION_NORMAL*: RNotificationUrgency = 0x04F1
  R_NOTIFICATION_CRITICAL*: RNotificationUrgency = 0x04F2
    ## Stays until the user dismisses it, where the system allows it.

  # RFileDialogFlags
  R_FILE_DIALOG_MULTIPLE*: RFileDialogFlags = 1
    ## Several files can be chosen (Rfile_dialog_open): see Rfile_dialog_get_result.
  R_FILE_DIALOG_RAVEN*: RFileDialogFlags = 2
    ## Never the dialog of the OS: the file browser of Raven, in a dialog of the parent window.
  R_FILE_DIALOG_SHOW_HIDDEN*: RFileDialogFlags = 4

  # RFileBrowserMode
  R_FILE_BROWSER_OPEN*: RFileBrowserMode = 0x0380
    ## Choose an existing file.
  R_FILE_BROWSER_SAVE*: RFileBrowserMode = 0x0381
    ## Choose where to save: a name is typed; an existing file must be confirmed.
  R_FILE_BROWSER_FOLDER*: RFileBrowserMode = 0x0382
    ## Choose a folder.

  # RMessageButtons
  R_MESSAGE_OK*: RMessageButtons = 0x0390
  R_MESSAGE_OK_CANCEL*: RMessageButtons = 0x0391
  R_MESSAGE_YES_NO*: RMessageButtons = 0x0392
  R_MESSAGE_YES_NO_CANCEL*: RMessageButtons = 0x0393

  # RMessageResult
  R_MESSAGE_RESULT_CANCEL*: RMessageResult = 0
    ## Dismissed (Escape), or the Cancel button.
  R_MESSAGE_RESULT_OK*: RMessageResult = 1
  R_MESSAGE_RESULT_YES*: RMessageResult = 2
  R_MESSAGE_RESULT_NO*: RMessageResult = 3

  # RMotion
  R_MOTION_SNAPPY*: RMotion = 0x0320
    ## Fast, no bounce: hover, highlights.
  R_MOTION_SMOOTH*: RMotion = 0x0321
    ## Slower, no bounce: scrolling, fades, sizes.
  R_MOTION_BOUNCY*: RMotion = 0x0322
    ## Runs a little past the target and comes back: switches, popups, pressed buttons.
  R_MOTION_WOBBLY*: RMotion = 0x0323
    ## Swings several times around the target: things that fly in or get dropped.

  # REffect
  R_EFFECT_AURORA*: REffect = 0x0B40
    ## Soft lights that drift slowly: color, color2; params: speed, scale, intensity, -. Animated.
  R_EFFECT_SHIMMER*: REffect = 0x0B41
    ## A band of light that sweeps across (loading placeholders): color = base, color2 = light; params: speed, width, angle, -. Animated.
  R_EFFECT_NEON*: REffect = 0x0B42
    ## A glowing edge whose colors run around the shape: color, color2; params: speed, width, glow, -. Animated.
  R_EFFECT_RIPPLE*: REffect = 0x0B43
    ## Rings that ripple what is behind, like water (reads the backdrop): color = tint; params: speed, strength, density, center x (0..1). Animated.
  R_EFFECT_PLASMA*: REffect = 0x0B44
    ## Bands of color that flow into each other: color, color2; params: speed, scale, contrast, -. Animated.
  R_EFFECT_GRAIN*: REffect = 0x0B45
    ## A gradient with the fine grain of film: color (top left) to color2 (bottom right); params: grain, angle, -, -. Animated grain.
  R_EFFECT_SPOTLIGHT*: REffect = 0x0B46
    ## A soft light that follows a point: color = light, color2 = background; params: x, y (0..1), radius (0..1), -.
  R_EFFECT_MESH*: REffect = 0x0B47
    ## Four colors blended across the surface, slowly moving: color, color2 and their mix; params: speed, -, -, -. Animated.

  # RShaderFlags
  R_SHADER_BACKDROP*: RShaderFlags = 1
    ## The effect reads what is drawn behind it (raven_backdrop, raven_backdrop_blur).
  R_SHADER_ANIMATED*: RShaderFlags = 2
    ## The effect changes with time: the window keeps drawing frames while it is shown.

  # RXmlValueType
  R_XML_TEXT*: RXmlValueType = 0x03C0
  R_XML_BOOL*: RXmlValueType = 0x03C1
    ## "true" or "false".
  R_XML_NUMBER*: RXmlValueType = 0x03C2
  R_XML_CHOICE*: RXmlValueType = 0x03C3
    ## One of the words of `choices`.
  R_XML_COLOR*: RXmlValueType = 0x03C4
    ## "#RGB", "#RRGGBB", "#RRGGBBAA", or a name: white, black, red, transparent...
  R_XML_ACTION*: RXmlValueType = 0x03C5
    ## The name of an action (Raction_bind).
  R_XML_FILE*: RXmlValueType = 0x03C6
    ## The path of a file.
  R_XML_NUMBERS*: RXmlValueType = 0x03C7
    ## Several numbers separated by commas: "8,4", "0,0,1,1".
  R_XML_LIST*: RXmlValueType = 0x03C8
    ## Texts separated by '\|': "One\|Two\|Three".

  # RColorFilter
  R_COLOR_FILTER_NONE*: RColorFilter = 0x0740
  R_COLOR_FILTER_PROTANOPIA*: RColorFilter = 0x0741
    ## Corrections: the colors that cannot be told apart are moved to colors that can.
  R_COLOR_FILTER_DEUTERANOPIA*: RColorFilter = 0x0742
  R_COLOR_FILTER_TRITANOPIA*: RColorFilter = 0x0743
  R_COLOR_FILTER_GRAYSCALE*: RColorFilter = 0x0744
  R_COLOR_FILTER_INVERT*: RColorFilter = 0x0745
  R_COLOR_FILTER_SIMULATE_PROTANOPIA*: RColorFilter = 0x0746
    ## Simulations, to see an interface as the color blind see it.
  R_COLOR_FILTER_SIMULATE_DEUTERANOPIA*: RColorFilter = 0x0747
  R_COLOR_FILTER_SIMULATE_TRITANOPIA*: RColorFilter = 0x0748
  R_COLOR_FILTER_SIMULATE_ACHROMATOPSIA*: RColorFilter = 0x0749

  # RAccessibleRole
  R_ROLE_AUTO*: RAccessibleRole = 0
    ## The role of the type of the widget (the default).
  R_ROLE_NONE*: RAccessibleRole = 0x0760
    ## Not exposed: its children are seen as the children of its parent.
  R_ROLE_WINDOW*: RAccessibleRole = 0x0761
  R_ROLE_GROUP*: RAccessibleRole = 0x0762
  R_ROLE_TEXT*: RAccessibleRole = 0x0763
  R_ROLE_HEADING*: RAccessibleRole = 0x0764
  R_ROLE_LINK*: RAccessibleRole = 0x0765
  R_ROLE_BUTTON*: RAccessibleRole = 0x0766
  R_ROLE_TOGGLE_BUTTON*: RAccessibleRole = 0x0767
  R_ROLE_CHECKBOX*: RAccessibleRole = 0x0768
  R_ROLE_RADIO_BUTTON*: RAccessibleRole = 0x0769
  R_ROLE_SLIDER*: RAccessibleRole = 0x076A
  R_ROLE_PROGRESS_BAR*: RAccessibleRole = 0x076B
  R_ROLE_SPIN_BUTTON*: RAccessibleRole = 0x076C
  R_ROLE_TEXT_FIELD*: RAccessibleRole = 0x076D
  R_ROLE_DOCUMENT*: RAccessibleRole = 0x076E
  R_ROLE_COMBO_BOX*: RAccessibleRole = 0x076F
  R_ROLE_LIST*: RAccessibleRole = 0x0770
  R_ROLE_LIST_ITEM*: RAccessibleRole = 0x0771
  R_ROLE_TREE*: RAccessibleRole = 0x0772
  R_ROLE_TREE_ITEM*: RAccessibleRole = 0x0773
  R_ROLE_TABLE*: RAccessibleRole = 0x0774
  R_ROLE_MENU_BAR*: RAccessibleRole = 0x0775
  R_ROLE_MENU*: RAccessibleRole = 0x0776
  R_ROLE_MENU_ITEM*: RAccessibleRole = 0x0777
  R_ROLE_TAB_LIST*: RAccessibleRole = 0x0778
  R_ROLE_TAB*: RAccessibleRole = 0x0779
  R_ROLE_TAB_PANEL*: RAccessibleRole = 0x077A
  R_ROLE_TOOLBAR*: RAccessibleRole = 0x077B
  R_ROLE_STATUS_BAR*: RAccessibleRole = 0x077C
  R_ROLE_DIALOG*: RAccessibleRole = 0x077D
  R_ROLE_ALERT*: RAccessibleRole = 0x077E
  R_ROLE_IMAGE*: RAccessibleRole = 0x077F
  R_ROLE_CANVAS*: RAccessibleRole = 0x0780
  R_ROLE_SEPARATOR*: RAccessibleRole = 0x0781
  R_ROLE_SCROLL_BAR*: RAccessibleRole = 0x0782
  R_ROLE_PANE*: RAccessibleRole = 0x0783

proc Rinit*(backend: RBackend): cint {.rapi.}
  ## Starts the library with default settings on the given backend.
  ## The calling thread becomes the UI thread.
  ## Returns: R_TRUE on success. On failure, see Rerror_get_last.
proc Rinit_ex*(desc: ptr RInitDesc): cint {.rapi.}
  ## Starts the library with explicit settings.
  ## desc Settings, or NULL for the defaults.
proc Rshutdown*() {.rapi.}
  ## Destroys every window, widget and resource, stops the worker and
  ## render threads and releases the backend.
proc Ris_initialized*(): cint {.rapi.}
proc Rget_version*(): cstring {.rapi.}
  ## Returns: The library version, e.g. "0.1.0".
proc Rset_allocator*(allocator: ptr RAllocator) {.rapi.}
  ## Replaces the allocator of Raven with the functions of the
  ## application (an engine allocator, a tracker...). NULL selects the
  ## allocator of Raven again. Must be called before Rinit, and before any
  ## other function that allocates. The functions are called from several
  ## threads; alloc must return memory aligned for any type (16 bytes).
proc Rallocator_get_system*(): ptr RAllocator {.rapi.}
  ## Returns: An allocator that forwards to malloc, realloc and free of the C
  ## runtime: Rset_allocator(Rallocator_get_system()) is how memory tools
  ## that follow malloc (Valgrind, heap profilers) see the blocks of Raven.
  ## The environment variable RAVEN_ALLOCATOR=system does the same without
  ## recompiling, and it is the default in a build with AddressSanitizer.
proc Rmemory_alloc*(size: csize_t): pointer {.rapi.}
  ## Memory from the allocator of Raven (or from the one given to
  ## Rset_allocator), for the application: custom widgets, pixel buffers.
  ## Aligned on 16 bytes. Never returns NULL: running out of memory is a
  ## fatal error. Thread-safe.
proc Rmemory_realloc*(`ptr`: pointer, size: csize_t): pointer {.rapi.}
  ## Resizes a block, keeping its content. A NULL block is allocated. Thread-safe.
proc Rmemory_free*(`ptr`: pointer) {.rapi.}
  ## Frees a block. NULL is ignored. Thread-safe.
proc Rmemory_get_stats*(stats: ptr RMemoryStats) {.rapi.}
  ## What the library holds right now. Thread-safe.
proc Rmemory_check*(): cint {.rapi.}
  ## Walks the whole heap of Raven and verifies its structure.
  ## Returns: R_FALSE when it is corrupted (a buffer overrun wrote over a block
  ## header). Slow: for tests and for hunting a bug. Thread-safe.
proc Rmemory_trim*() {.rapi.}
  ## Returns to the OS the regions of the heap that hold nothing. Thread-safe.
proc Rdraw_frame*(): cint {.rapi.}
  ## Produces one frame: processes the pending OS events without
  ## waiting, runs the posted tasks and the timers, then lays out, paints and
  ## submits every window that changed. A window in which nothing changed is
  ## not drawn again, so the call is cheap when the UI is idle.
  ## Returns: R_FALSE once Rquit was called or the last window is closed.
proc Rwait_frame*(timeout_seconds: cdouble): cint {.rapi.}
  ## Same as Rdraw_frame, but first sleeps until there is something to
  ## do: an OS event, a posted task, a timer, an animation, a pending redraw.
  ## timeout_seconds Longest sleep. Negative sleeps without limit, 0
  ## does not sleep.
proc Rquit*(exit_code: cint) {.rapi.}
  ## Makes Rdraw_frame and Rwait_frame return R_FALSE. Thread-safe.
proc Rget_exit_code*(): cint {.rapi.}
  ## Returns: The code given to Rquit, 0 when the last window was closed.
proc Ris_running*(): cint {.rapi.}
  ## Returns: R_FALSE once Rquit was called or every window is closed.
proc Rpoll_events*() {.rapi.}
  ## Processes the pending OS events without producing a frame.
  ## Rdraw_frame does it already: only useful to keep the windows responsive
  ## during a long operation.
proc Rwait_events*(timeout_seconds: cdouble) {.rapi.}
  ## Sleeps until an OS event, a posted task, a timer or a pending
  ## redraw, and processes the OS events. Rwait_frame does it already.
  ## timeout_seconds Maximum wait. Negative waits without limit.
proc Rset_frame_callback*(callback: RFrameCallback, user_data: pointer) {.rapi.}
proc Rget_delta_time*(): cdouble {.rapi.}
  ## Returns: Seconds between the last two frames.
proc Rget_time*(): cdouble {.rapi.}
  ## Returns: Seconds elapsed since Rinit, from a monotonic clock. Thread-safe.
proc Rset_ui_scale*(scale: cfloat) {.rapi.}
  ## Scale of the whole interface, on top of the one of the monitor
  ## (RInitDesc.ui_scale at start): every window is laid out again. 0.25 .. 4.
proc Rset_vsync*(enabled: cint) {.rapi.}
  ## R_TRUE: every frame waits for the vertical synchronization of the
  ## screen (no tearing, no wasted frames); R_FALSE: as fast as possible
  ## (RInitDesc.disable_vsync at start). Takes effect at the next frame.
proc Rget_vsync*(): cint {.rapi.}
proc Rget_ui_scale*(): cfloat {.rapi.}
proc Rset_animations_enabled*(enabled: cint) {.rapi.}
  ## R_FALSE makes every change immediate: for users who ask for
  ## reduced motion (Raccessibility_set_reduce_motion), and for tests that
  ## read the state right after an input.
proc Rget_animations_enabled*(): cint {.rapi.}
proc Rset_animation_speed*(speed: cfloat) {.rapi.}
  ## Multiplies the speed of every animation. 1 is the default, 0.25 a slow motion.
proc Rget_animation_speed*(): cfloat {.rapi.}
proc Rset_animation_time_step*(seconds: cdouble) {.rapi.}
  ## When positive, each frame advances the animations by exactly this
  ## many seconds instead of the time really elapsed: frames rendered one by
  ## one for a video or a screenshot, deterministic tests. 0 follows the clock.
proc Rtask_post*(task: RTaskCallback, user_data: pointer) {.rapi.}
  ## Queues a function to run on the UI thread during the next frame
  ## and wakes Rwait_frame. Thread-safe: this is how worker threads touch the UI.
proc Rtask_run_async*(task: RTaskCallback, user_data: pointer) {.rapi.}
  ## Runs a function on a worker thread of the job system. Thread-safe.
proc Rparallel_for*(count: uint32, grain: uint32, function: RParallelRangeCallback, user_data: pointer) {.rapi.}
  ## Splits [0, count) into ranges of at most `grain` items and runs them
  ## on the worker threads and the calling thread. Returns when all are done.
proc Rwake_up*() {.rapi.}
  ## Wakes Rwait_frame and Rwait_events from any thread.
proc Rget_worker_thread_count*(): cint {.rapi.}
proc Ris_ui_thread*(): cint {.rapi.}
proc Rtimer_create*(interval_seconds: cdouble, repeat: cint, callback: RTimerCallback, user_data: pointer): Rid {.rapi.}
  ## Calls a function on the UI thread after `interval_seconds`.
  ## repeat R_TRUE to fire repeatedly, R_FALSE to fire once and delete itself.
proc Rtimer_destroy*(timer: Rid) {.rapi.}
proc Rtimer_is_valid*(timer: Rid): cint {.rapi.}
proc Raction_bind*(name: cstring, callback: RActionCallback, user_data: pointer) {.rapi.}
proc Raction_unbind*(name: cstring) {.rapi.}
proc Raction_fire*(name: cstring, sender: Rid) {.rapi.}
  ## Fires an action as a widget would: the function bound to the
  ## name is called, or the default handler. What a tool bar button, a
  ## script or a test uses to do what a menu item does.
  ## sender Passed to the function; R_INVALID_ID when there is none.
proc Raction_set_default_handler*(callback: RActionCallback, user_data: pointer) {.rapi.}
  ## Receives every action that has no bound function and is not one of the built-in actions (e.g. a script bridge).
proc Raction_set_builtins*(enabled: cint) {.rapi.}
  ## The actions Raven knows by itself: an interface written in XML
  ## uses them with no C at all (OnClicked="app.quit").
  ##
  ##   app.quit                          quits (Rquit(0))
  ##   window.close, window.minimize,    the window of the widget
  ##   window.maximize, window.restore   (maximize goes back when it already is)
  ##   window.picture_in_picture         in and out of picture in picture
  ##   island.expand, island.fold,       the island of that window
  ##   island.toggle
  ##   theme.dark, theme.light,          the look of Raven, dark or light
  ##   theme.toggle
  ##   theme:NAME                        Rtheme_use(NAME): a theme loaded or built in
  ##   language:CODE                     Rlocale_set_language(CODE)
  ##   open:ADDRESS                      Ropen_url (https:, mailto:, a file)
  ##   copy:TEXT                         the text to the clipboard
  ##   show:NAME, hide:NAME, toggle:NAME the widget named NAME in the window (hidden: Collapsed)
  ##   enable:NAME, disable:NAME
  ##   focus:NAME
  ##   page:NAME=2, page:NAME=+1         what a Switcher, a TabView, a ComboBox or a ListView
  ##                                     shows; +1 and -1 step, and go round
  ##
  ## A function bound to one of these names (Raction_bind) replaces it; the
  ## default handler receives the others only.
  ## enabled R_FALSE turns them all off: a design tool that shows
  ## interfaces without running them. On by default.
proc Raction_is_builtin*(name: cstring): cint {.rapi.}
  ## Returns: R_TRUE when the name is one of the built-in actions (Raction_set_builtins), whether they are on or not.
proc Ropen_url*(url: cstring): cint {.rapi.}
  ## Opens an address with the program the user has for it: a web
  ## page in the browser, a mailto: in the mail program, a file in its
  ## application. Returns at once.
  ## Returns: R_FALSE when the OS could not be asked (nothing tells whether
  ## a program then really opened it).
proc Rclipboard_set_text*(text: cstring) {.rapi.}
proc Rclipboard_set_html*(text: cstring, html: cstring) {.rapi.}
  ## Copies one content in two formats: `html` for the programs that
  ## paste formatting (word processors, chat, mail) and `text` for the
  ## others (editors, terminals). Each program takes the format it prefers.
  ## Where the window system has no clipboard formats (Win32 and macOS for
  ## now), or `html` is NULL, it is Rclipboard_set_text(text).
proc Rclipboard_get_text*(): cstring {.rapi.}
  ## Returns: The clipboard text, or "" when empty. Valid until the next call.
proc Rerror_get_last*(detail: ptr cstring, severity: ptr RSeverity): RError {.rapi.}
  ## Retrieves and clears the last recorded error.
  ## detail   Optional output: human-readable description.
  ## severity Optional output: severity of the error.
  ## Returns: R_NO_ERROR when nothing was recorded.
proc Rerror_to_string*(error: RError): cstring {.rapi.}
proc Rseverity_to_string*(severity: RSeverity): cstring {.rapi.}
proc Rerror_set_callback*(callback: RErrorCallback) {.rapi.}
proc Rset_device_reset_callback*(callback: RDeviceResetCallback, user_data: pointer) {.rapi.}
proc Rwindow_get_frame_stats*(window: Rid, stats: ptr RFrameStats) {.rapi.}
proc Rwindow_create*(title: cstring, width: cint, height: cint, flags: uint32): Rid {.rapi.}
  ## Opens a top-level window.
  ## width,height Client size in logical units.
  ## flags        Bitmask of RWindowFlags.
proc Rwindow_destroy*(window: Rid) {.rapi.}
  ## Closes a window and deletes its content widgets.
proc Rwindow_is_valid*(window: Rid): cint {.rapi.}
proc Rwindow_set_content*(window: Rid, widget: Rid) {.rapi.}
  ## Sets the root widget of the window. The window takes ownership of
  ## the widget; the previous content is deleted.
proc Rwindow_get_content*(window: Rid): Rid {.rapi.}
proc Rwindow_set_title*(window: Rid, title: cstring) {.rapi.}
proc Rwindow_get_title*(window: Rid): cstring {.rapi.}
proc Rwindow_set_size*(window: Rid, width: cint, height: cint) {.rapi.}
proc Rwindow_get_size*(window: Rid, width: ptr cint, height: ptr cint) {.rapi.}
proc Rwindow_set_min_size*(window: Rid, width: cint, height: cint) {.rapi.}
proc Rwindow_set_position*(window: Rid, x: cint, y: cint) {.rapi.}
proc Rwindow_get_position*(window: Rid, x: ptr cint, y: ptr cint) {.rapi.}
proc Rwindow_get_framebuffer_size*(window: Rid, width: ptr cint, height: ptr cint) {.rapi.}
  ## Size of the drawable area in pixels.
proc Rwindow_get_scale*(window: Rid): cfloat {.rapi.}
  ## Returns: Pixels per logical unit (monitor scale x global UI scale).
proc Rwindow_show*(window: Rid) {.rapi.}
proc Rwindow_hide*(window: Rid) {.rapi.}
proc Rwindow_focus*(window: Rid) {.rapi.}
  ## Shows the window in front of the others with the keyboard, and
  ## makes the application the active one. Rwindow_show alone does not take
  ## the keyboard from another application (a prompt that appears while the
  ## user types elsewhere); this is for a window the user just asked for (a
  ## card opened from a tray icon). Where the system has no such request,
  ## the same as Rwindow_show.
proc Rwindow_maximize*(window: Rid) {.rapi.}
proc Rwindow_minimize*(window: Rid) {.rapi.}
proc Rwindow_restore*(window: Rid) {.rapi.}
proc Rwindow_is_maximized*(window: Rid): cint {.rapi.}
proc Rwindow_is_minimized*(window: Rid): cint {.rapi.}
proc Rwindow_is_active*(window: Rid): cint {.rapi.}
  ## Returns: R_TRUE while the window is the one the user is working in.
proc Rwindow_close*(window: Rid) {.rapi.}
  ## Asks to close the window, as the close button of its title bar
  ## does: the close callback is asked first and may refuse.
proc Rwindow_begin_move*(window: Rid) {.rapi.}
  ## Custom headers. Call these from a mouse-down handler, while the left
  ## button is held: the window follows the pointer until the button is
  ## released (the widget does not receive the release). A
  ## R_WIDGET_HEADER_BAR and the widgets given to Rwidget_set_window_drag do
  ## it by themselves.
proc Rwindow_begin_resize*(window: Rid, edge: RWindowEdge) {.rapi.}
proc Rwindow_set_icon*(window: Rid, rgba: ptr uint8, width: cint, height: cint) {.rapi.}
  ## Icon of the window (title bar, task bar), from tightly packed
  ## RGBA8 pixels; NULL goes back to the default: on Windows the first icon
  ## of the executable, when it has one. Windows and X11.
proc Rwindow_set_clear_color*(window: Rid, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
proc Rwindow_set_background*(window: Rid, texture: Rid) {.rapi.}
  ## Image drawn behind the content, scaled to cover the window:
  ## what the glass of a glass theme looks through. R_INVALID_ID removes it.
proc Rwindow_set_backdrop*(window: Rid, backdrop: RBackdrop): cint {.rapi.}
  ## Chooses what shows behind a window created with
  ## R_WINDOW_TRANSPARENT (see RBackdrop). With anything but
  ## R_BACKDROP_NONE the window is not cleared with its color and the
  ## wallpaper of the theme is not drawn: only what the widgets draw hides
  ## the screen. In XML: <Window Backdrop="Blur">.
  ## Returns: R_TRUE when the window system does it; R_FALSE (the window
  ## then stays as it was) without R_WINDOW_TRANSPARENT, or when it cannot.
proc Rwindow_get_backdrop*(window: Rid): RBackdrop {.rapi.}
proc Rwindow_set_theme*(window: Rid, name: cstring): cint {.rapi.}
  ## Gives the window a theme of its own, by name ("Dark",
  ## "WindowsLight", or a theme of Rtheme_load): everything inside it (its
  ## content, its island, its menus, lists and dialogs, its background) is
  ## drawn with that theme, while the other windows keep the one of the
  ## application (Rtheme_use). A widget given Rwidget_set_theme keeps its own.
  ## In XML: <Window Theme="...">.
  ## name NULL or "": the theme of the application again.
  ## Returns: R_FALSE, after reporting an error, when no theme has that name.
proc Rwindow_get_theme*(window: Rid): cstring {.rapi.}
  ## Returns: Name of the theme the window is drawn with (the one of the application when it has none of its own).
proc Rwindow_has_own_theme*(window: Rid): cint {.rapi.}
  ## Returns: R_TRUE when the window has a theme of its own (Rwindow_set_theme).
proc Rwindow_set_opacity*(window: Rid, opacity: cfloat): cint {.rapi.}
  ## Opacity of the whole window, its frame included, from 0 to 1:
  ## a window that fades, a tool that lets what is under it show.
  ## Returns: R_FALSE when the window system cannot do it.
proc Rwindow_get_opacity*(window: Rid): cfloat {.rapi.}
proc Rwindow_set_topmost*(window: Rid, topmost: cint): cint {.rapi.}
  ## Keeps the window above the other windows (splash screens, tools).
  ## Returns: R_FALSE when the window system cannot do it.
proc Rwindow_set_owner*(window: Rid, owner: Rid): cint {.rapi.}
  ## Gives the window an owner (a tool window or an inspector of the
  ## main window): it stays above its owner, has no button of its own in the
  ## taskbar, is minimized and restored with it (where the window system
  ## does it: Windows, X11), and is destroyed with it (everywhere: Raven
  ## destroys the windows a window owns before the window). R_INVALID_ID
  ## gives the window back its independence.
  ## Returns: R_FALSE when the window system cannot keep the window with its
  ## owner (it is still destroyed with it), or when the owner is not valid
  ## or is owned by the window (nothing is changed then).
proc Rwindow_get_owner*(window: Rid): Rid {.rapi.}
  ## Returns: The owner of the window (Rwindow_set_owner), or R_INVALID_ID.
proc Rwindow_set_picture_in_picture*(window: Rid, enabled: cint): cint {.rapi.}
  ## Picture in picture: the window becomes a small floating window
  ## that stays above the other windows, on every desktop (every space of
  ## macOS, over the applications in full screen too), in a corner of the
  ## work area of its monitor. It loses its title bar where the system
  ## allows: the empty parts of its content move it, its edges resize it,
  ## and its size keeps the proportions it was given. Leaving it gives back
  ## the size, place, frame and state the window had.
  ##
  ## Raven does not add a button to leave it: the content shows one, with
  ## the built-in action "window.picture_in_picture" (it goes in and out).
  ## Returns: R_FALSE when the window system cannot keep a window above the
  ## others (Wayland): the window is left as it is.
proc Rwindow_is_picture_in_picture*(window: Rid): cint {.rapi.}
proc Rwindow_set_picture_in_picture_layout*(window: Rid, width: cint, height: cint, corner: RPictureInPictureCorner) {.rapi.}
  ## Size (logical units) and corner of the window in picture in
  ## picture, for the next time it goes in (now, when it is in). 0, 0 (the
  ## default): 320 wide, with the proportions the content has when it goes
  ## in. The proportions of width / height are kept while the user resizes.
proc Rwindow_center*(window: Rid) {.rapi.}
  ## Centers the window on the work area of its monitor (no task bar).
proc Rwindow_set_inspector*(window: Rid, shown: cint) {.rapi.}
  ## Shows (R_TRUE) or hides (R_FALSE) the inspector of the window: a
  ## window beside it that lists its widgets as a tree, says what the
  ## selected one is (type, name, classes, place and size in the window,
  ## desired size, constraints, slot, visibility, state, style, action,
  ## children) and what the last frame cost (layout, paint and render
  ## times, widgets, instances, draw calls). The selected widget is
  ## outlined in the window, with its type and size. "Pick" selects the
  ## next widget clicked in the window (that click goes to the inspector
  ## only). It follows the window as it changes; closing it hides it, and
  ## it goes with the window. Nothing opens it but this function.
proc Rwindow_is_inspector_shown*(window: Rid): cint {.rapi.}
proc Rwindow_get_inspector*(window: Rid): Rid {.rapi.}
  ## Returns: The window of the inspector of a window (to place it), or R_INVALID_ID when it is hidden.
proc Rwindow_is_splash_screen*(window: Rid): cint {.rapi.}
  ## Returns: R_TRUE for a window created with R_WINDOW_SPLASH_SCREEN (IsSplashScreen in XML).
proc Rwindow_request_redraw*(window: Rid) {.rapi.}
  ## Asks for a new frame even if no widget changed. Thread-safe.
proc Rwindow_set_continuous_redraw*(window: Rid, enabled: cint) {.rapi.}
  ## R_TRUE redraws the window every frame (games, live viewports).
proc Rwindow_set_close_callback*(window: Rid, callback: RWindowCloseCallback, user_data: pointer) {.rapi.}
proc Rwindow_set_resize_callback*(window: Rid, callback: RWindowResizeCallback, user_data: pointer) {.rapi.}
proc Rwindow_set_drop_callback*(window: Rid, callback: RDropCallback, user_data: pointer) {.rapi.}
  ## Takes what is dropped on the window from other applications,
  ## where no widget takes it (Rwidget_set_drop_callback). NULL removes it.
proc Rdrop_set_highlight_rect*(x: cfloat, y: cfloat, width: cfloat, height: cfloat) {.rapi.}
  ## During R_DROP_ENTER / R_DROP_MOVE only: choose Raven's highlight
  ## in target-local logical units (window-local for a window callback).
  ## Clipped to the target; zero width or height hides it without refusing
  ## the drop. Each callback starts with the default highlight again.
  ## Dimensions must be finite and nonnegative; coordinates must be finite.
proc Rwindow_set_user_handle*(window: Rid, handle: pointer) {.rapi.}
proc Rwindow_get_user_handle*(window: Rid): pointer {.rapi.}
proc Rwindow_get_native_handle*(window: Rid, native: ptr RNativeWindow): cint {.rapi.}
  ## Fills `native` with the OS handles of the window.
proc Rwindow_capture*(window: Rid, width: ptr cint, height: ptr cint): ptr uint8 {.rapi.}
  ## Reads back the last rendered frame as tightly packed RGBA8 pixels.
  ## Blocks until the frame is rendered. Free the result with Rpixels_free.
  ## The alpha is 255, except for a window that is seen through
  ## (R_WINDOW_TRANSPARENT): its pixels are then premultiplied by their
  ## alpha, as the window system receives them.
  ## Returns: NULL when the backend cannot capture.
proc Rwindow_get_focused_widget*(window: Rid): Rid {.rapi.}
  ## Widget that has the keyboard focus in the window, or R_INVALID_ID.
proc Rwindow_get_hovered_widget*(window: Rid): Rid {.rapi.}
  ## Deepest widget under the cursor, or R_INVALID_ID.
proc Rwindow_get_mouse_position*(window: Rid, x: ptr cfloat, y: ptr cfloat) {.rapi.}
proc Rwindow_inject_mouse_move*(window: Rid, x: cfloat, y: cfloat) {.rapi.}
  ## Inject input as if it came from the OS. Positions are in logical units.
  ## Useful for automated tests, replays and when Raven is driven by a host
  ## application.
proc Rwindow_inject_mouse_button*(window: Rid, button: RMouseButton, pressed: cint) {.rapi.}
proc Rwindow_inject_mouse_wheel*(window: Rid, wheel_x: cfloat, wheel_y: cfloat) {.rapi.}
proc Rwindow_inject_key*(window: Rid, key: RKey, pressed: cint, modifiers: uint32) {.rapi.}
proc Rwindow_inject_text*(window: Rid, text: cstring) {.rapi.}
proc Rwindow_inject_drag*(window: Rid, x: cfloat, y: cfloat, inside: cint): cint {.rapi.}
  ## A drag from another application moves over the window
  ## (`inside` R_TRUE) or leaves it.
  ## Returns: R_TRUE when a drop at this position would be accepted.
proc Rwindow_inject_drop*(window: Rid, x: cfloat, y: cfloat, paths: cstringArray, path_count: cint, text: cstring): cint {.rapi.}
  ## Drops files and / or text at a position. Returns R_TRUE when a target took them.
proc Rwindow_is_key_down*(window: Rid, key: RKey): cint {.rapi.}
  ## R_TRUE while the key is held down in the window (cleared when the window loses the keyboard).
proc Rwindow_is_mouse_button_down*(window: Rid, button: RMouseButton): cint {.rapi.}
  ## R_TRUE while the mouse button is held down over (or captured by) the window.
proc Rwindow_get_modifiers*(window: Rid): uint32 {.rapi.}
  ## Modifiers (RModifier) of the last input event.
proc Rwindow_is_mouse_inside*(window: Rid): cint {.rapi.}
  ## R_TRUE while the pointer is over the client area.
proc Rwindow_wants_text_input*(window: Rid): cint {.rapi.}
  ## R_TRUE when the keyboard focus is in a text input, spin box or text editor.
proc Rwindow_has_popup*(window: Rid): cint {.rapi.}
  ## R_TRUE while a menu, list or dialog is open over the window.
proc Rwindow_get_text_input_rect*(window: Rid, x: ptr cint, y: ptr cint, width: ptr cint, height: ptr cint): cint {.rapi.}
  ## Last painted caret in window pixels (x, y, width, height), including scrolling and scale.
  ## Call after Rdraw_frame. Independent of the blink phase. Returns R_FALSE and zeros the
  ## outputs when there is no visible editable caret, the window is inactive, or focus changed
  ## since painting. Any output pointer may be NULL.
proc Rwindow_get_popup_count*(window: Rid): cint {.rapi.}
  ## Returns: Number of open in-window popups (menus, combo lists, pickers), bottom to top.
proc Rwindow_get_popup*(window: Rid, index: cint): Rid {.rapi.}
  ## Returns: Root widget of popup `index`, whose rect is in window coordinates; R_INVALID_ID out of range.
proc Rwindow_set_tab_navigation*(window: Rid, enabled: cint) {.rapi.}
  ## R_FALSE: Tab no longer moves the keyboard focus (the application uses it as a shortcut). Default R_TRUE.
proc Rwindow_set_event_hook*(window: Rid, hook: RWindowEventHook, user_data: pointer) {.rapi.}
proc Rdrag_begin*(source: Rid, `type`: cstring, data: cstring, label: cstring): cint {.rapi.}
  ## Starts dragging `data` from `source` (call it while a mouse button
  ## is held, typically from a mouse-move event once the pointer has moved
  ## a few units). Until the button is released, the widgets with a drop
  ## callback (Rwidget_set_drop_callback) under the pointer receive
  ## R_DROP_ENTER / MOVE / LEAVE and, on release, R_DROP_DROPPED, with
  ## `drag_type` set and `text` = `data`. Returning R_FALSE from the callback
  ## refuses the drop there. Escape cancels. `label` follows the pointer.
  ## Returns: R_FALSE when the widget is not in a window.
proc Rdrag_is_active*(window: Rid): cint {.rapi.}
  ## R_TRUE while a drag started by Rdrag_begin is in progress in the window.
proc Rdrag_get_type*(window: Rid): cstring {.rapi.}
  ## Type of the drag in progress, or NULL.
proc Rdrag_get_data*(window: Rid): cstring {.rapi.}
  ## Data of the drag in progress, or NULL.
proc Rdrag_cancel*(window: Rid) {.rapi.}
  ## Ends the drag in progress without dropping (the target receives R_DROP_LEAVE).
proc Rwindow_register_shortcut*(window: Rid, key: RKey, modifiers: uint32, action: cstring) {.rapi.}
proc Rwindow_unregister_shortcut*(window: Rid, key: RKey, modifiers: uint32) {.rapi.}
proc Rtouch_get_settings*(settings: ptr RTouchSettings) {.rapi.}
  ## The settings every window follows, unless it has its own.
proc Rtouch_set_settings*(settings: ptr RTouchSettings) {.rapi.}
  ## Changes them; NULL puts back the defaults.
proc Rtouch_get_default_settings*(settings: ptr RTouchSettings) {.rapi.}
  ## The defaults, to start from.
proc Rwindow_set_touch_settings*(window: Rid, settings: ptr RTouchSettings) {.rapi.}
  ## Settings of one window; NULL: it follows Rtouch_set_settings again.
proc Rwindow_get_touch_settings*(window: Rid, settings: ptr RTouchSettings) {.rapi.}
  ## The settings the window follows (its own, or those of the application).
proc Rwidget_set_touch_mode*(widget: Rid, mode: RTouchMode) {.rapi.}
  ## What a finger does on the widget and what it contains (default R_TOUCH_MODE_AUTO: as its parent says).
proc Rwidget_get_touch_mode*(widget: Rid): RTouchMode {.rapi.}
proc Rwidget_set_touch_gestures*(widget: Rid, enabled: cint) {.rapi.}
  ## R_FALSE: two fingers (on the screen or a trackpad) neither zoom nor pan the widget (default R_TRUE).
proc Rwidget_get_touch_gestures*(widget: Rid): cint {.rapi.}
proc Rwidget_set_pen_mouse*(widget: Rid, enabled: cint) {.rapi.}
  ## R_FALSE: the pen is not a mouse on the widget and what it contains; only its pen callback sees it (default R_TRUE).
proc Rwidget_get_pen_mouse*(widget: Rid): cint {.rapi.}
proc Rwidget_set_touch_callback*(widget: Rid, callback: RTouchCallback, user_data: pointer) {.rapi.}
proc Rwidget_set_pen_callback*(widget: Rid, callback: RPenCallback, user_data: pointer) {.rapi.}
proc Rwidget_set_gesture_callback*(widget: Rid, callback: RGestureCallback, user_data: pointer) {.rapi.}
proc Rwindow_get_touch_count*(window: Rid): cint {.rapi.}
  ## Fingers on the window now.
proc Rwindow_is_pen_in_range*(window: Rid): cint {.rapi.}
  ## R_TRUE while a pen is near the screen over the window.
proc Rwindow_is_touch_active*(window: Rid): cint {.rapi.}
  ## R_TRUE when the pointer last used in the window was a finger (the interface may then be made larger).
proc Rwindow_inject_touch*(window: Rid, id: cint, phase: RTouchPhase, x: cfloat, y: cfloat) {.rapi.}
  ## A finger: R_TOUCH_DOWN, then R_TOUCH_MOVE..., then R_TOUCH_UP, with the same `id`.
proc Rwindow_inject_pen*(window: Rid, phase: RPenPhase, x: cfloat, y: cfloat, pressure: cfloat, tilt_x: cfloat, tilt_y: cfloat, buttons: uint32, eraser: cint) {.rapi.}
  ## A pen: `pressure` 0..1 as the pen would give it, `buttons` a bitmask of RPenButton.
proc Rwindow_inject_trackpad_gesture*(window: Rid, kind: RGestureKind, phase: RGesturePhase, x: cfloat, y: cfloat, scale_delta: cfloat, rotation_delta: cfloat, pan_x: cfloat, pan_y: cfloat, fingers: cint) {.rapi.}
  ## A gesture of a trackpad, as the system recognizes it: R_GESTURE_BEGIN,
  ## R_GESTURE_UPDATE..., then R_GESTURE_END. (x, y) is the pointer, `scale_delta`
  ## and `rotation_delta` (degrees, clockwise) what changed since the previous
  ## event, `pan_x`, `pan_y` how far the fingers moved since then.
proc Rwindow_inject_trackpad_scroll*(window: Rid, dx: cfloat, dy: cfloat) {.rapi.}
  ## Two fingers scrolling on a trackpad: (dx, dy) is how far they moved
  ## (logical units), the content following them. Sent as a precise wheel
  ## (REvent::wheel_precise) at the pointer.
proc Rwindow_inject_trackpad_scroll_end*(window: Rid) {.rapi.}
  ## The fingers that scrolled left the trackpad, on a system that
  ## leaves the inertia to the application (Wayland): flicked content keeps
  ## going (Touch.Inertia).
proc Rwindow_advance_touch*(window: Rid, seconds: cdouble) {.rapi.}
  ## Advances the inertia of flicked content by `seconds` at once,
  ## and fires a long press that is due, without waiting (tests).
proc Rlook_set*(look: RLook) {.rapi.}
  ## Changes the look of the application. The theme file in use, if
  ## it names no look of its own, is laid again over the new one; colors
  ## and metrics changed with Rtheme_set_... are lost; what was given to
  ## widgets (Rwidget_set_color...) is kept.
  ##
  ##   Rlook_set(R_LOOK_NATIVE_LIGHT);   the look of the platform, light
  ##
  ## Theme files that name no look (no Look attribute) follow it too, so a
  ## part of a window given one (Theme="paper.xml") changes with it.
proc Rlook_get*(): RLook {.rapi.}
  ## The look given to Rlook_set (R_LOOK_RAVEN_DARK at start).
proc Rtheme_apply*(theme: RTheme) {.rapi.}
  ## @deprecated Rlook_set: a built-in theme is a look of Raven.
proc Rtheme_set_color*(role: RColorRole, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
proc Rtheme_get_color*(role: RColorRole, r: ptr cfloat, g: ptr cfloat, b: ptr cfloat, a: ptr cfloat) {.rapi.}
proc Rtheme_set_metric*(metric: RMetric, value: cfloat) {.rapi.}
proc Rtheme_get_metric*(metric: RMetric): cfloat {.rapi.}
proc Rtheme_set_material*(material: RMaterial) {.rapi.}
  ## Material of every widget that was not given one with
  ## Rwidget_set_material. Rtheme_apply sets it: solid for the flat
  ## themes, frosted glass for the glass themes.
proc Rtheme_get_material*(): RMaterial {.rapi.}
proc Rtheme_set_font*(font: Rid) {.rapi.}
  ## Font used by widgets that have no font of their own.
proc Rtheme_get_font*(): Rid {.rapi.}
proc Rtheme_set_accent*(r: cfloat, g: cfloat, b: cfloat) {.rapi.}
  ## Sets the accent color and everything that follows from it: its
  ## hovered and pressed shades, the focus outline, the selection, the links.
proc Rtheme_set_dark*(dark: cint) {.rapi.}
  ## Tells whether the theme is a dark one. It decides the tint of
  ## the glass, the colors of the code and which wallpaper the glass themes
  ## use. Rtheme_apply and the theme files set it.
proc Rtheme_is_dark*(): cint {.rapi.}
proc Rtheme_set_wallpaper*(texture: Rid) {.rapi.}
  ## Image drawn behind the content of the windows that have no
  ## background of their own, or R_INVALID_ID for none (the window color).
proc Rtheme_get_wallpaper*(): Rid {.rapi.}
proc Rtheme_set_style_color*(widget: cstring, style_class: cstring, role: RColorRole, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
  ## Gives a color to the widgets of a type and / or of a class,
  ## instead of the color of the theme.
  ## widget      Tag of a type of widget ("Button"), or NULL for every type.
  ## style_class A class (Rwidget_set_class, Class="..."), or NULL
  ##                    for the widgets of the type whatever their classes.
  ##
  ## When several styles give the same value to a widget, the one with a
  ## type and a class wins over the one with a class, which wins over the
  ## one with a type; between equals, the last one set wins.
proc Rtheme_set_style_metric*(widget: cstring, style_class: cstring, metric: RMetric, value: cfloat) {.rapi.}
proc Rtheme_set_style_material*(widget: cstring, style_class: cstring, material: RMaterial) {.rapi.}
  ## R_MATERIAL_DEFAULT removes the material of the style.
proc Rtheme_clear_styles*() {.rapi.}
  ## Removes every style of the theme. Rtheme_apply and Rtheme_use do it too.
proc Rtheme_load*(path: cstring): cstring {.rapi.}
  ## Reads a theme file and keeps the theme under its name (the Name
  ## attribute, else the name of the file). It does not change the theme in
  ## use unless a theme of that name is the one in use.
  ##
  ## The file is watched like the interface files (Rxml_set_live_reload):
  ## saving it updates every widget that uses the theme.
  ## Returns: The name of the theme, valid until Rshutdown, or NULL after
  ## reporting an error.
proc Rtheme_load_from_memory*(xml: cstring, size: csize_t): cstring {.rapi.}
  ## Same, from a text. `size` may be 0 for a null-terminated text.
proc Rtheme_use*(name: cstring): cint {.rapi.}
  ## Makes a named theme the theme of the application: a theme
  ## given by Rtheme_load (laid over the look in use when it names no
  ## look), or the name of a look ("WindowsLight", "RavenDark"; the old
  ## "Dark", "Light", "GlassDark"... too), which is then Rlook_set.
  ## Values changed with Rtheme_set_... are lost.
  ## Returns: R_FALSE, after reporting an error, when no theme has that name.
proc Rtheme_get_name*(): cstring {.rapi.}
  ## Name of the theme in use ("Dark" at start).
proc Rtheme_get_count*(): cint {.rapi.}
  ## Number of named themes, the four built-in ones included.
proc Rtheme_get_name_at*(index: cint): cstring {.rapi.}
  ## Name of the index-th one, or NULL.
proc Rtheme_save*(path: cstring, name: cstring): cint {.rapi.}
  ## Writes the theme in use, with every change made since it was
  ## applied, as a theme file.
  ## name Name given to the theme in the file; NULL keeps the current one.
proc Rfont_create*(data: cstring, data_size: csize_t): Rid {.rapi.}
  ## Creates a font from the content of a .ttf / .otf file.
  ## The buffer is copied. Glyphs are rasterized on demand, at the exact
  ## pixel size needed by each window scale, into a shared atlas.
  ## Returns: R_INVALID_ID on failure.
proc Rfont_create_from_file*(path: cstring): Rid {.rapi.}
proc Rfont_destroy*(font: Rid) {.rapi.}
proc Rfont_is_valid*(font: Rid): cint {.rapi.}
proc Rfont_get_default*(): Rid {.rapi.}
  ## Returns: The font embedded in the library (Roboto, Latin and Latin Extended-A).
proc Rfont_get_monospace*(): Rid {.rapi.}
  ## Returns: The fixed-width font embedded in the library (Roboto Mono, same
  ## characters), used by default by the text editors. In XML: Font="monospace".
proc Rfont_set_fallback*(font: Rid, fallback: Rid) {.rapi.}
  ## Font used for the characters missing from `font` (symbols, icons,
  ## other scripts). Fallbacks can be chained.
proc Rfont_set_variant*(font: Rid, style: RFontStyle, variant: Rid) {.rapi.}
  ## Gives a font the font to use for one of its styles: the bold
  ## file of the same family, its italic file. Formatted text (Markdown
  ## labels, the Markdown of the text editor) uses them.
  ##
  ## A font that was given none still has the three styles: Raven draws
  ## its glyphs thicker for bold and slanted for italic, keeping their
  ## advances, so that text does not move when its style changes.
  ## variant R_INVALID_ID goes back to the style Raven derives.
proc Rfont_get_variant*(font: Rid, style: RFontStyle): Rid {.rapi.}
  ## Returns: The font drawn for a style of `font`: the one given with
  ## Rfont_set_variant, else the one Raven derives from it (made on first
  ## use; it lives as long as `font` and cannot be deleted by itself).
  ## Usable wherever a font is: Rlabel_set_font, Rdraw_text...
proc Rfont_measure_text*(font: Rid, size: cfloat, text: cstring, width: ptr cfloat, height: ptr cfloat) {.rapi.}
  ## Measures a UTF-8 string, in logical units, at scale 1.
  ## font R_INVALID_ID uses the theme font.
  ## size Font size; 0 uses R_METRIC_FONT_SIZE.
proc Rtexture_create*(data: cstring, data_size: csize_t): Rid {.rapi.}
  ## Creates a texture from the content of an image file (PNG, JPEG,
  ## BMP, TGA, GIF, PSD, SVG). Decoding happens on the calling thread.
  ## Returns: R_INVALID_ID on failure.
proc Rtexture_create_async*(data: cstring, data_size: csize_t): Rid {.rapi.}
  ## Same as Rtexture_create, decoded on a worker thread. The handle
  ## is usable at once; widgets showing it draw nothing until it is ready.
  ## The buffer is copied.
proc Rtexture_create_from_svg*(svg: cstring, svg_size: csize_t, scale: cfloat): Rid {.rapi.}
  ## Draws an SVG image (an icon) into a texture, at `scale` pixels per
  ## unit of the SVG. The widgets show it at the size the SVG gives, whatever
  ## the scale (Rtexture_get_density).
  ##
  ## 0 or less: at the size it is shown. It is first drawn at a scale of 2;
  ## once painted, it is drawn again (on a worker thread) with as many pixels
  ## as the screen shows it with: the scale of the monitor of the window, an
  ## Image that stretches it, a zoom. It follows when the window moves to a
  ## screen of another density or the image grows, and comes down when it is
  ## shown much smaller (sides of at most 4096 pixels). Its width and height
  ## in pixels (Rtexture_get_size) change then; its size in logical units
  ## does not. Rtexture_create, Rtexture_create_from_file and the asynchronous
  ## ones read SVG files this way. Paths, fills, strokes and gradients are
  ## drawn; text, filters and masks are not.
proc Rtexture_get_density*(texture: Rid): cfloat {.rapi.}
  ## Pixels of the texture per logical unit: the scale of an SVG, 1 otherwise.
proc Rtexture_create_from_file*(path: cstring): Rid {.rapi.}
proc Rtexture_create_from_file_async*(path: cstring): Rid {.rapi.}
  ## Reads and decodes the file on a worker thread.
proc Rtexture_create_from_pixels*(rgba: ptr uint8, width: cint, height: cint): Rid {.rapi.}
  ## Creates a texture from tightly packed RGBA8 pixels (straight alpha). Its filter is R_FILTER_LINEAR.
proc Rtexture_update*(texture: Rid, rgba: ptr uint8, width: cint, height: cint) {.rapi.}
  ## Replaces the pixels of a texture. The size may change. Use it to
  ## show frames produced on the CPU or read back from another renderer.
proc Rimage_decode*(data: pointer, data_size: csize_t, width: ptr cint, height: ptr cint): ptr uint8 {.rapi.}
  ## Decodes an image file held in memory (PNG, JPEG, BMP, TGA) into
  ## tightly packed RGBA8 pixels, without making a texture: for pictures
  ## that arrive one after the other and go to Rtexture_update. Thread-safe,
  ## so that the decoding can be done away from the UI thread.
  ## Returns: The pixels, to release with Rpixels_free; NULL when the data
  ## is not an image Raven can read.
proc Rpixels_free*(pixels: ptr uint8) {.rapi.}
  ## Releases pixels returned by Raven (Rimage_decode, Rwindow_capture). Thread-safe.
proc Rtexture_is_ready*(texture: Rid): cint {.rapi.}
  ## Returns: R_TRUE once an asynchronous texture is decoded (or failed).
proc Rtexture_wait*(texture: Rid) {.rapi.}
  ## Blocks until an asynchronous texture is ready.
proc Rtexture_destroy*(texture: Rid) {.rapi.}
proc Rtexture_is_valid*(texture: Rid): cint {.rapi.}
proc Rtexture_get_size*(texture: Rid, width: ptr cint, height: ptr cint) {.rapi.}
proc Rtexture_set_filter*(texture: Rid, filter: RFilterType) {.rapi.}
  ## How the texture is smoothed when it is scaled. The textures made
  ## from image files start with R_FILTER_MIPMAP, the others with
  ## R_FILTER_LINEAR.
proc Rwidget_create*(`type`: RWidgetType): Rid {.rapi.}
  ## Creates a widget. It is not shown until it is added to a parent
  ## that belongs to a window (Rwidget_add_child, Rwindow_set_content).
proc Rwidget_destroy*(widget: Rid) {.rapi.}
  ## Deletes a widget and all its descendants.
proc Rwidget_is_valid*(widget: Rid): cint {.rapi.}
proc Rwidget_get_type*(widget: Rid): RWidgetType {.rapi.}
proc Rwidget_add_child*(parent: Rid, child: Rid): Rid {.rapi.}
  ## Appends a child to a panel. The parent takes ownership.
  ## A widget that already has a parent is moved.
  ## Returns: `child`, so calls can be nested:
  ##         Rid ok = Rwidget_add_child(row, Rbutton_create("OK"));
proc Rwidget_insert_child*(parent: Rid, child: Rid, index: cint): Rid {.rapi.}
proc Rwidget_remove_child*(parent: Rid, child: Rid) {.rapi.}
  ## Detaches a child without deleting it. The caller owns it again.
proc Rwidget_clear_children*(parent: Rid) {.rapi.}
  ## Deletes every child of a panel.
proc Rwidget_get_parent*(widget: Rid): Rid {.rapi.}
proc Rwidget_get_child_count*(widget: Rid): cint {.rapi.}
proc Rwidget_get_child*(widget: Rid, index: cint): Rid {.rapi.}
proc Rwidget_get_window*(widget: Rid): Rid {.rapi.}
  ## Returns: The window showing the widget, or R_INVALID_ID.
proc Rwidget_set_name*(widget: Rid, name: cstring) {.rapi.}
proc Rwidget_get_name*(widget: Rid): cstring {.rapi.}
proc Rwidget_find*(scope: Rid, name: cstring): Rid {.rapi.}
  ## Finds a widget by name.
  ## scope A window or a widget; the search covers its whole subtree.
  ## Returns: R_INVALID_ID when no widget has that name.
proc Rslot_set_padding*(child: Rid, left: cfloat, top: cfloat, right: cfloat, bottom: cfloat) {.rapi.}
  ## Space kept around the child, inside its slot. All panels.
proc Rslot_set_halign*(child: Rid, align: RHAlign) {.rapi.}
  ## Alignment of the child inside its slot. Default: fill. All panels.
proc Rslot_set_valign*(child: Rid, align: RVAlign) {.rapi.}
proc Rslot_set_auto*(child: Rid) {.rapi.}
  ## The slot takes the desired size of the child along the panel
  ## direction (Slate AutoWidth / AutoHeight). This is the default.
  ## VBox, HBox.
proc Rslot_set_fill*(child: Rid, coefficient: cfloat) {.rapi.}
  ## The slot shares the space left by the auto slots, in proportion
  ## to `coefficient` (Slate FillWidth / FillHeight). VBox, HBox.
  ## In a Splitter, the coefficient is the relative size of the pane.
proc Rslot_set_max_size*(child: Rid, max_size: cfloat) {.rapi.}
  ## Upper bound of the slot along the panel direction. 0 = none. VBox, HBox.
proc Rslot_set_grid*(child: Rid, column: cint, row: cint) {.rapi.}
  ## Cell of the child. Grid, UniformGrid.
proc Rslot_set_grid_span*(child: Rid, column_span: cint, row_span: cint) {.rapi.}
proc Rslot_set_anchors*(child: Rid, min_x: cfloat, min_y: cfloat, max_x: cfloat, max_y: cfloat) {.rapi.}
  ## Canvas placement. Anchors are fractions of the canvas size (0..1).
  ##   - anchors equal on an axis: the child is at anchor + position, with
  ##     the slot size (or its desired size when the slot size is <= 0);
  ##   - anchors different on an axis: the child stretches between them,
  ##     position and size are then the near and far margins.
  ## Alignment is the pivot of the child (0,0 = top-left, 0.5,0.5 = center).
proc Rslot_set_position*(child: Rid, x: cfloat, y: cfloat) {.rapi.}
proc Rslot_set_size*(child: Rid, width: cfloat, height: cfloat) {.rapi.}
proc Rslot_set_alignment*(child: Rid, x: cfloat, y: cfloat) {.rapi.}
proc Rslot_set_title*(child: Rid, title: cstring) {.rapi.}
  ## Title of the tab showing the child. TabView.
proc Rwidget_set_visibility*(widget: Rid, visibility: RVisibility) {.rapi.}
proc Rwidget_get_visibility*(widget: Rid): RVisibility {.rapi.}
proc Rwidget_set_enabled*(widget: Rid, enabled: cint) {.rapi.}
  ## A disabled widget ignores input and is drawn dimmed, with its descendants.
proc Rwidget_is_enabled*(widget: Rid): cint {.rapi.}
proc Rwidget_set_hit_testable*(widget: Rid, hit_testable: cint) {.rapi.}
  ## R_FALSE lets mouse events pass through the widget (not its children).
proc Rwidget_set_opacity*(widget: Rid, opacity: cfloat) {.rapi.}
  ## Opacity multiplier applied to the widget and its descendants (0..1).
proc Rwidget_set_transition*(widget: Rid, transition: RTransition, delay_seconds: cfloat) {.rapi.}
  ## Gives the widget a transition that plays each time it appears:
  ## when its window is first drawn, when it is added to a window that is
  ## shown, and when it becomes visible again (Rwidget_set_visibility, a
  ## tab or a switcher showing it). In XML: Appear="FlyUp" AppearDelay="0.1".
  ## delay_seconds Time before it starts. Give growing delays to the
  ## items of a group to make them arrive one after the other.
proc Rwidget_play_transition*(widget: Rid, transition: RTransition, delay_seconds: cfloat) {.rapi.}
  ## Plays a transition once, now, whatever the widget was given with Rwidget_set_transition.
proc Rwidget_set_clipping*(widget: Rid, clip: cint) {.rapi.}
  ## R_TRUE clips the descendants to the bounds of the widget.
proc Rwidget_set_tooltip*(widget: Rid, text: cstring) {.rapi.}
  ## Text shown when the cursor rests on the widget. NULL removes it.
proc Rwidget_get_tooltip*(widget: Rid): cstring {.rapi.}
  ## Returns: The tooltip text of the widget, "" when it has none.
proc Rwidget_set_cursor*(widget: Rid, cursor: RCursor) {.rapi.}
proc Rwidget_set_size*(widget: Rid, width: cfloat, height: cfloat) {.rapi.}
  ## Size constraints, in logical units. They apply to the desired size the
  ## widget reports to its parent. A negative value removes the constraint
  ## on that axis.
proc Rwidget_set_min_size*(widget: Rid, width: cfloat, height: cfloat) {.rapi.}
proc Rwidget_set_max_size*(widget: Rid, width: cfloat, height: cfloat) {.rapi.}
proc Rwidget_get_rect*(widget: Rid, x: ptr cfloat, y: ptr cfloat, width: ptr cfloat, height: ptr cfloat) {.rapi.}
  ## Bounds of the widget in its window, in logical units, after the last layout.
proc Rwidget_get_desired_size*(widget: Rid, width: ptr cfloat, height: ptr cfloat) {.rapi.}
proc Rwidget_is_hovered*(widget: Rid): cint {.rapi.}
proc Rwidget_set_focusable*(widget: Rid, focusable: cint) {.rapi.}
proc Rwidget_focus*(widget: Rid) {.rapi.}
  ## Gives the keyboard focus to the widget.
proc Rwidget_is_focused*(widget: Rid): cint {.rapi.}
proc Rwidget_set_user_handle*(widget: Rid, handle: pointer) {.rapi.}
proc Rwidget_get_user_handle*(widget: Rid): pointer {.rapi.}
proc Rwidget_set_destroy_callback*(widget: Rid, callback: RWidgetDestroyCallback, user_data: pointer) {.rapi.}
proc Rwidget_set_event_callback*(widget: Rid, callback: RWidgetEventCallback, user_data: pointer) {.rapi.}
proc Rwidget_set_action*(widget: Rid, action: cstring) {.rapi.}
  ## Name of the action fired by the main event of the widget: button
  ## click, value change, text commit, item activation (see Raction_bind).
proc Rwidget_get_action*(widget: Rid): cstring {.rapi.}
proc Rwidget_set_drop_callback*(widget: Rid, callback: RDropCallback, user_data: pointer) {.rapi.}
  ## Makes the widget a target for what is dragged from other
  ## applications: files from the file manager, text. It is highlighted
  ## while a drop on it would be accepted. NULL removes the callback.
proc Rwidget_set_window_drag*(widget: Rid, enabled: cint) {.rapi.}
  ## In a window created with R_WINDOW_CUSTOM_HEADER: dragging the
  ## widget moves the window and a double click maximizes it, as a title bar
  ## does. The buttons, fields and other controls inside it keep working.
  ## In XML: WindowDrag="true".
proc Rwidget_hit_test*(root: Rid, x: cfloat, y: cfloat): Rid {.rapi.}
  ## Deepest widget of a subtree under a point of its window, or R_INVALID_ID.
proc Rwidget_pick*(root: Rid, x: cfloat, y: cfloat): Rid {.rapi.}
  ## Deepest visible widget of a subtree under a point of its window,
  ## whether it reacts to the pointer or not: a label, a panel, a disabled
  ## button. What a design tool selects with a click.
proc Rwidget_set_context_menu*(widget: Rid, menu: Rid) {.rapi.}
  ## Menu opened by a right click on the widget. The widget does not own it.
proc Rwidget_invalidate*(widget: Rid) {.rapi.}
  ## Asks for the widget to be laid out and painted again.
proc Rwidget_set_color*(widget: Rid, role: RColorRole, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
  ## Overrides a theme color for one widget. R_COLOR_TEXT is also
  ## inherited by its descendants.
proc Rwidget_clear_color*(widget: Rid, role: RColorRole) {.rapi.}
proc Rwidget_get_color*(widget: Rid, role: RColorRole, r: ptr cfloat, g: ptr cfloat, b: ptr cfloat, a: ptr cfloat) {.rapi.}
  ## The color the widget is drawn with for a role: its own, else
  ## the one of the styles of its theme that apply to it, else the one of
  ## its theme. What a custom widget should draw itself with.
proc Rwidget_set_material*(widget: Rid, material: RMaterial) {.rapi.}
  ## Gives one widget a material of its own, whatever the theme:
  ## a solid button in a glass window, a panel of clear glass in a flat one.
  ## A widget that draws no surface (a box, a label) gets one of this
  ## material behind it: any widget can become a panel of glass.
  ## In XML: Material="Glass \| FrostedGlass \| TintedGlass \| Solid".
  ## material R_MATERIAL_DEFAULT follows the theme again.
proc Rwidget_get_material*(widget: Rid): RMaterial {.rapi.}
  ## Returns: The material the widget is drawn with: its own, or the one of the theme.
proc Rwidget_set_metric*(widget: Rid, metric: RMetric, value: cfloat) {.rapi.}
  ## Overrides a theme metric for one widget.
proc Rwidget_clear_metric*(widget: Rid, metric: RMetric) {.rapi.}
proc Rwidget_get_metric*(widget: Rid, metric: RMetric): cfloat {.rapi.}
  ## Returns: The metric the widget uses: its own, else the one of a style, else the one of its theme.
proc Rwidget_set_class*(widget: Rid, classes: cstring) {.rapi.}
  ## Classes of the widget, separated by spaces ("danger large"):
  ## the styles of the theme made for these classes apply to it
  ## (Rtheme_set_style_color, <Style Class="danger" .../> in a theme file).
  ## In XML: Class="...". NULL or "" removes them.
proc Rwidget_get_class*(widget: Rid): cstring {.rapi.}
  ## Returns: The classes, "" when it has none. Valid until they change.
proc Rwidget_add_class*(widget: Rid, style_class: cstring) {.rapi.}
proc Rwidget_remove_class*(widget: Rid, style_class: cstring) {.rapi.}
proc Rwidget_has_class*(widget: Rid, style_class: cstring): cint {.rapi.}
proc Rwidget_set_theme*(widget: Rid, name: cstring): cint {.rapi.}
  ## Gives the widget and everything inside it a theme of their own,
  ## by name ("Dark", "GlassLight", or a theme of Rtheme_load): a dark side
  ## bar in a light window, a preview of another theme. Menus and lists
  ## opened from inside follow. In XML: Theme="...".
  ## name NULL or "" follows the parent (the application theme) again.
  ## Returns: R_FALSE, after reporting an error, when no theme has that name.
proc Rwidget_get_theme*(widget: Rid): cstring {.rapi.}
  ## Returns: Name of the theme the widget is drawn with.
proc Rwidget_set_font*(widget: Rid, font: Rid) {.rapi.}
  ## Gives the widget and everything inside it a font, for the texts
  ## that have none of their own (a label given Rlabel_set_font keeps its
  ## own): a tool whose panels wear one font while the interface it shows
  ## keeps the font of the application. Menus and lists opened from inside
  ## follow.
  ## font R_INVALID_ID: the font of the parent (in the end, the one of the theme, Rtheme_set_font) again.
proc Rwidget_get_font*(widget: Rid): Rid {.rapi.}
  ## Returns: The font given to the widget itself, or R_INVALID_ID.
proc Rpanel_set_spacing*(panel: Rid, spacing: cfloat) {.rapi.}
  ## Gap inserted between neighbouring children.
proc Rpanel_get_spacing*(panel: Rid): cfloat {.rapi.}
proc Rgrid_set_column_fill*(grid: Rid, column: cint, coefficient: cfloat) {.rapi.}
  ## Lets a column / row grow to take a share of the free space
  ## (Slate FillColumn / FillRow). 0 sizes it to its content (default).
proc Rgrid_set_row_fill*(grid: Rid, row: cint, coefficient: cfloat) {.rapi.}
proc Runiform_grid_set_columns*(grid: Rid, columns: cint) {.rapi.}
  ## Places children automatically, left to right then top to bottom,
  ## in the given number of columns. 0 uses the grid cell of each slot (default).
proc Runiform_grid_set_min_cell_size*(grid: Rid, width: cfloat, height: cfloat) {.rapi.}
proc Rborder_set_padding*(border: Rid, left: cfloat, top: cfloat, right: cfloat, bottom: cfloat) {.rapi.}
  ## Space between the outline and the child.
proc Rborder_set_background*(border: Rid, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
  ## Background color (R_COLOR_PANEL of this widget). Alpha 0 draws no background.
proc Rborder_set_outline*(border: Rid, width: cfloat, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
  ## Outline width and color. Width 0 draws no outline (default).
proc Rborder_set_corner_radius*(border: Rid, top_left: cfloat, top_right: cfloat, bottom_right: cfloat, bottom_left: cfloat) {.rapi.}
proc Rborder_set_shadow*(border: Rid, blur: cfloat, offset_x: cfloat, offset_y: cfloat) {.rapi.}
  ## Soft drop shadow behind the border. Blur 0 disables it (default).
proc Rborder_set_material*(border: Rid, material: RMaterial) {.rapi.}
  ## What the border is made of. R_MATERIAL_GLASS turns it into a
  ## panel of glass tinted by its background color (the alpha of the color
  ## is the strength of the tint): floating tool bars, side bars, cards.
proc Rsize_box_set_aspect_ratio*(size_box: Rid, ratio: cfloat) {.rapi.}
  ## Keeps the child at a fixed width / height ratio, as large as fits.
  ## 0 disables it (default). The fixed, minimum and maximum sizes are set
  ## with Rwidget_set_size, Rwidget_set_min_size and Rwidget_set_max_size.
proc Rspacer_set_size*(spacer: Rid, width: cfloat, height: cfloat) {.rapi.}
proc Rscroll_box_set_orientation*(scroll_box: Rid, orientation: ROrientation) {.rapi.}
  ## Direction in which children are stacked and scrolled. Default: R_VERTICAL.
proc Rscroll_box_set_scroll_both*(scroll_box: Rid, enabled: cint) {.rapi.}
  ## R_TRUE also scrolls across the stacking direction when the content is too large.
proc Rscroll_box_set_offset*(scroll_box: Rid, x: cfloat, y: cfloat) {.rapi.}
proc Rscroll_box_get_offset*(scroll_box: Rid, x: ptr cfloat, y: ptr cfloat) {.rapi.}
proc Rscroll_box_scroll_to_end*(scroll_box: Rid) {.rapi.}
proc Rwidget_scroll_into_view*(widget: Rid) {.rapi.}
  ## Scrolls every ScrollBox above the widget so that the widget is visible.
proc Rsplitter_set_orientation*(splitter: Rid, orientation: ROrientation) {.rapi.}
  ## R_HORIZONTAL puts the panes side by side (default), R_VERTICAL stacks them.
proc Rsplitter_set_min_pane_size*(splitter: Rid, size: cfloat) {.rapi.}
proc Rbackground_blur_create*(): Rid {.rapi.}
  ## Creates a panel that shows what is behind it out of focus,
  ## with one child over it. In XML:
  ##
  ##   <BackgroundBlur Style="Frosted" Radius="24" CornerRadius="18" Padding="16">
  ##       <Label>Over a blurred view of the window</Label>
  ##   </BackgroundBlur>
  ##
  ## It works in any theme and with any material: it is not the glass of
  ## the glass themes (which bends and reflects), but the looks of
  ## RBlurStyle. Unlike the opacity of a widget, it does not let what is
  ## behind show sharp.
proc Rbackground_blur_set_style*(blur: Rid, style: RBlurStyle) {.rapi.}
proc Rbackground_blur_get_style*(blur: Rid): RBlurStyle {.rapi.}
proc Rbackground_blur_set_radius*(blur: Rid, radius: cfloat) {.rapi.}
  ## How far what is behind goes out of focus, in logical units:
  ## 4 softens, 20 hides the details, 60 leaves only colors. A negative
  ## value (the default) uses R_METRIC_BLUR_RADIUS of the theme.
proc Rbackground_blur_get_radius*(blur: Rid): cfloat {.rapi.}
proc Rbackground_blur_set_tint*(blur: Rid, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
  ## Color laid over the blur; its alpha is how much of it. Without
  ## one, R_BLUR_PLAIN has none and the other styles take theirs from the
  ## theme (R_COLOR_PANEL).
proc Rbackground_blur_clear_tint*(blur: Rid) {.rapi.}
  ## Removes the tint given with Rbackground_blur_set_tint.
proc Rbackground_blur_set_saturation*(blur: Rid, saturation: cfloat) {.rapi.}
  ## Saturation of what is seen through: 1 = as it is, 0 = gray, 1.5 = vivid. Negative = the one of the style.
proc Rbackground_blur_set_grain*(blur: Rid, grain: cfloat) {.rapi.}
  ## Fine grain over the blur, 0 = none .. 1. Negative = the one of the style.
proc Rbackground_blur_set_corner_radius*(blur: Rid, top_left: cfloat, top_right: cfloat, bottom_right: cfloat, bottom_left: cfloat) {.rapi.}
proc Rbackground_blur_set_padding*(blur: Rid, left: cfloat, top: cfloat, right: cfloat, bottom: cfloat) {.rapi.}
  ## Space between the edge and the child.
proc Rbackground_blur_set_shadow*(blur: Rid, shadow_blur: cfloat) {.rapi.}
  ## Soft shadow under the panel; blur 0 = none (default).
proc Rswitcher_set_index*(switcher: Rid, index: cint) {.rapi.}
proc Rswitcher_get_index*(switcher: Rid): cint {.rapi.}
proc Rtab_view_add_tab*(tab_view: Rid, title: cstring, content: Rid): cint {.rapi.}
  ## Adds a page. Same as Rwidget_add_child followed by Rslot_set_title.
  ## Returns: The index of the new tab, or -1.
proc Rtab_view_set_active*(tab_view: Rid, index: cint) {.rapi.}
proc Rtab_view_get_active*(tab_view: Rid): cint {.rapi.}
proc Rtab_view_set_changed_callback*(tab_view: Rid, callback: RTabChangedCallback, user_data: pointer) {.rapi.}
proc Rtab_view_set_documents*(tab_view: Rid, documents: cint) {.rapi.}
  ## Documents mode: the major tabs of an application (the level,
  ## then one tab per open asset, as in Unreal): tabs on the left with an
  ## icon and a close button (a middle click closes too), shrinking when
  ## they do not fit, no slide between pages. XML: Documents="true".
proc Rtab_view_get_tab_count*(tab_view: Rid): cint {.rapi.}
proc Rtab_view_get_tab*(tab_view: Rid, index: cint): Rid {.rapi.}
  ## The page of a tab, R_INVALID_ID if none.
proc Rtab_view_find_tab*(tab_view: Rid, content: Rid): cint {.rapi.}
  ## Index of the tab of a page, -1 if none.
proc Rtab_view_set_tab_title*(tab_view: Rid, content: Rid, title: cstring) {.rapi.}
proc Rtab_view_set_tab_icon*(tab_view: Rid, content: Rid, texture: Rid, u0: cfloat, v0: cfloat, u1: cfloat, v1: cfloat, tint: cint) {.rapi.}
  ## Icon before the title (documents). R_INVALID_ID removes it; tint: a mask in the text color.
proc Rtab_view_set_tab_closable*(tab_view: Rid, content: Rid, closable: cint) {.rapi.}
  ## R_FALSE: no close button (documents ; the level tab). Tabs can be closed by default.
proc Rtab_view_show_tab*(tab_view: Rid, content: Rid) {.rapi.}
  ## Shows the tab of a page (the changed callback is called).
proc Rtab_view_set_close_callback*(tab_view: Rid, callback: RTabCloseCallback, user_data: pointer) {.rapi.}
proc Rexpander_set_title*(expander: Rid, title: cstring) {.rapi.}
proc Rexpander_get_title*(expander: Rid): cstring {.rapi.}
proc Rexpander_set_expanded*(expander: Rid, expanded: cint) {.rapi.}
proc Rexpander_is_expanded*(expander: Rid): cint {.rapi.}
proc Rexpander_set_changed_callback*(expander: Rid, callback: RExpanderChangedCallback, user_data: pointer) {.rapi.}
proc Rset_layout_direction*(direction: RDirection) {.rapi.}
  ## Direction of every window: R_DIRECTION_AUTO (the default), _LTR or _RTL.
proc Rget_layout_direction*(): RDirection {.rapi.}
  ## The direction the windows use now: R_DIRECTION_LTR or _RTL.
proc Rwidget_set_direction*(widget: Rid, direction: RDirection) {.rapi.}
  ## Direction of a part of a window: R_DIRECTION_INHERIT, _LTR or _RTL.
proc Rwidget_get_direction*(widget: Rid): RDirection {.rapi.}
  ## The direction the widget is laid out in: R_DIRECTION_LTR or _RTL.
proc Rlabel_create*(text: cstring): Rid {.rapi.}
  ## Shortcut for Rwidget_create(R_WIDGET_LABEL) + Rlabel_set_text.
proc Rlabel_set_text*(label: Rid, text: cstring) {.rapi.}
  ## Sets the text. '\n' starts a new line.
proc Rlabel_get_text*(label: Rid): cstring {.rapi.}
proc Rlabel_set_font*(label: Rid, font: Rid) {.rapi.}
  ## R_INVALID_ID uses the theme font.
proc Rlabel_set_text_size*(label: Rid, size: cfloat) {.rapi.}
  ## Font size in logical units. 0 uses R_METRIC_FONT_SIZE.
proc Rlabel_set_text_align*(label: Rid, align: RTextAlign) {.rapi.}
proc Rlabel_set_wrap*(label: Rid, wrap: cint) {.rapi.}
  ## R_TRUE wraps the text on the width given by the parent.
proc Rlabel_set_color*(label: Rid, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
  ## Text color (R_COLOR_TEXT of this widget).
proc Rlabel_set_markdown*(label: Rid, enabled: cint) {.rapi.}
  ## R_TRUE reads the text as Markdown and shows it formatted
  ## (Markdown="true" in XML):
  ##
  ##   # titles, **strong**, *emphasis*, `code`, ~~struck~~, [links](url),
  ##   lists and lists in lists, - [x] task lists, > quotes, --- rules,
  ##   \| tables \|, images alone on their line, and \`\`\` blocks of code with
  ##   the colors of their language (\`\`\`c, \`\`\`python...: the languages of
  ##   the text editor, raven_text_editor.h).
  ##
  ## The text wraps on the width of the label unless Rlabel_set_wrap said
  ## otherwise; put the label in a scroll box for a long document. Strong
  ## and emphasized text use the bold and italic of the font
  ## (Rfont_set_variant), which Raven derives from the font itself when it
  ## was given none. The colors come from the theme: R_COLOR_LINK,
  ## R_COLOR_CODE, R_COLOR_SEPARATOR, R_COLOR_ACCENT.
proc Rlabel_get_markdown*(label: Rid): cint {.rapi.}
proc Rlabel_set_link_callback*(label: Rid, callback: RLabelLinkCallback, user_data: pointer) {.rapi.}
proc Rlabel_get_plain_text*(label: Rid): cstring {.rapi.}
  ## The text of a Markdown label without its marks, as a reader sees it. Valid until the next call.
proc Rlabel_set_selectable*(label: Rid, selectable: cint) {.rapi.}
  ## R_TRUE lets the user select the text of the label with the
  ## mouse, as on a web page (Selectable="true" in XML): drag to select, a
  ## double click selects a word, a triple click a paragraph, Shift + click
  ## extends the selection; then Ctrl+C (Cmd+C) copies it and Ctrl+A selects
  ## everything. A click elsewhere takes the selection away. Plain and
  ## Markdown labels alike: a copy of a Markdown label is its text without
  ## the marks, and its links are still clicked, not selected. The label
  ## takes the mouse (what holds it no longer receives the clicks on it) and
  ## the keyboard after a click; Tab passes it by. Default R_FALSE.
proc Rlabel_get_selectable*(label: Rid): cint {.rapi.}
proc Rlabel_get_selected_text*(label: Rid): cstring {.rapi.}
  ## The text selected, as Ctrl+C copies it ("" when nothing is). Valid until the next call.
proc Rlabel_select_all*(label: Rid) {.rapi.}
  ## Selects the whole text of a selectable label.
proc Rlabel_clear_selection*(label: Rid) {.rapi.}
proc Rbutton_create*(text: cstring): Rid {.rapi.}
  ## Shortcut for Rwidget_create(R_WIDGET_BUTTON) + Rbutton_set_text.
proc Rbutton_set_text*(button: Rid, text: cstring) {.rapi.}
  ## Sets the content of the button to a label. A button can instead
  ## hold any single child widget (Rwidget_add_child), e.g. an HBox with an icon.
proc Rbutton_set_style*(button: Rid, style: RButtonStyle) {.rapi.}
proc Rbutton_set_clicked_callback*(button: Rid, callback: RButtonClickedCallback, user_data: pointer) {.rapi.}
proc Rcheckbox_create*(text: cstring): Rid {.rapi.}
proc Rcheckbox_set_text*(checkbox: Rid, text: cstring) {.rapi.}
proc Rcheckbox_get_text*(checkbox: Rid): cstring {.rapi.}
proc Rcheckbox_set_checked*(checkbox: Rid, checked: cint) {.rapi.}
proc Rcheckbox_is_checked*(checkbox: Rid): cint {.rapi.}
proc Rcheckbox_set_style*(checkbox: Rid, style: RCheckboxStyle) {.rapi.}
proc Rcheckbox_set_changed_callback*(checkbox: Rid, callback: RCheckboxChangedCallback, user_data: pointer) {.rapi.}
proc Rslider_set_range*(slider: Rid, minimum: cfloat, maximum: cfloat) {.rapi.}
proc Rslider_set_value*(slider: Rid, value: cfloat) {.rapi.}
proc Rslider_get_value*(slider: Rid): cfloat {.rapi.}
proc Rslider_get_range*(slider: Rid, minimum: ptr cfloat, maximum: ptr cfloat, step: ptr cfloat) {.rapi.}
  ## The range given with Rslider_set_range and the step (NULL for what is not wanted).
proc Rslider_set_step*(slider: Rid, step: cfloat) {.rapi.}
  ## Snaps the value to multiples of `step`. 0 = continuous (default).
proc Rslider_set_orientation*(slider: Rid, orientation: ROrientation) {.rapi.}
proc Rslider_set_changed_callback*(slider: Rid, callback: RSliderChangedCallback, user_data: pointer) {.rapi.}
proc Rprogress_bar_set_value*(progress_bar: Rid, value: cfloat) {.rapi.}
  ## Progress between 0 and 1.
proc Rprogress_bar_get_value*(progress_bar: Rid): cfloat {.rapi.}
proc Rimage_set_texture*(image: Rid, texture: Rid) {.rapi.}
proc Rimage_set_tint_color*(image: Rid, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
proc Rimage_set_tint_text*(image: Rid, enabled: cint) {.rapi.}
  ## R_TRUE: the image is tinted with the text color around it (white icons that follow the theme). XML: TintText.
proc Rimage_set_stretch*(image: Rid, stretch: RStretch) {.rapi.}
proc Rimage_set_region*(image: Rid, min_u: cfloat, min_v: cfloat, max_u: cfloat, max_v: cfloat) {.rapi.}
  ## Part of the texture to show, in UV coordinates. Default: 0, 0, 1, 1.
proc Rimage_set_corner_radius*(image: Rid, radius: cfloat) {.rapi.}
proc Rtext_input_set_text*(text_input: Rid, text: cstring) {.rapi.}
proc Rtext_input_get_text*(text_input: Rid): cstring {.rapi.}
proc Rtext_input_set_placeholder*(text_input: Rid, text: cstring) {.rapi.}
  ## Dimmed text shown while the input is empty.
proc Rtext_input_set_password*(text_input: Rid, password: cint) {.rapi.}
proc Rtext_input_is_password*(text_input: Rid): cint {.rapi.}
proc Rtext_input_is_read_only*(text_input: Rid): cint {.rapi.}
proc Rtext_input_set_read_only*(text_input: Rid, read_only: cint) {.rapi.}
proc Rtext_input_set_max_length*(text_input: Rid, max_length: cint) {.rapi.}
  ## Maximum number of characters. 0 = no limit (default).
proc Rtext_input_select_all*(text_input: Rid) {.rapi.}
proc Rtext_input_set_changed_callback*(text_input: Rid, callback: RTextChangedCallback, user_data: pointer) {.rapi.}
  ## Called on every edit.
proc Rtext_input_set_committed_callback*(text_input: Rid, callback: RTextChangedCallback, user_data: pointer) {.rapi.}
  ## Called when Enter is pressed. This also fires the action of the widget.
proc Rspin_box_set_range*(spin_box: Rid, minimum: cdouble, maximum: cdouble) {.rapi.}
proc Rspin_box_set_value*(spin_box: Rid, value: cdouble) {.rapi.}
proc Rspin_box_get_value*(spin_box: Rid): cdouble {.rapi.}
proc Rspin_box_get_range*(spin_box: Rid, minimum: ptr cdouble, maximum: ptr cdouble, step: ptr cdouble) {.rapi.}
  ## The range and the step (NULL for what is not wanted).
proc Rspin_box_set_step*(spin_box: Rid, step: cdouble) {.rapi.}
proc Rspin_box_set_decimals*(spin_box: Rid, decimals: cint) {.rapi.}
  ## Digits shown after the decimal point. Default: 2.
proc Rspin_box_set_changed_callback*(spin_box: Rid, callback: RSpinBoxChangedCallback, user_data: pointer) {.rapi.}
proc Rcombo_box_add_item*(combo_box: Rid, text: cstring): cint {.rapi.}
  ## Returns: The index of the new item.
proc Rcombo_box_clear_items*(combo_box: Rid) {.rapi.}
proc Rcombo_box_get_item_count*(combo_box: Rid): cint {.rapi.}
proc Rcombo_box_get_item_text*(combo_box: Rid, index: cint): cstring {.rapi.}
proc Rcombo_box_set_selected*(combo_box: Rid, index: cint) {.rapi.}
  ## Selects an item. -1 selects nothing.
proc Rcombo_box_get_selected*(combo_box: Rid): cint {.rapi.}
proc Rcombo_box_set_changed_callback*(combo_box: Rid, callback: RComboBoxChangedCallback, user_data: pointer) {.rapi.}
proc Rseparator_set_orientation*(separator: Rid, orientation: ROrientation) {.rapi.}
  ## R_HORIZONTAL draws a horizontal line (default).
proc Rlist_view_add_item*(list_view: Rid, text: cstring): cint {.rapi.}
  ## Returns: The index of the new item.
proc Rlist_view_set_item_text*(list_view: Rid, index: cint, text: cstring) {.rapi.}
proc Rlist_view_get_item_text*(list_view: Rid, index: cint): cstring {.rapi.}
proc Rlist_view_remove_item*(list_view: Rid, index: cint) {.rapi.}
proc Rlist_view_clear_items*(list_view: Rid) {.rapi.}
proc Rlist_view_get_item_count*(list_view: Rid): cint {.rapi.}
proc Rlist_view_set_virtual_items*(list_view: Rid, count: cint, callback: RListItemTextCallback, user_data: pointer) {.rapi.}
proc Rlist_view_set_item_color_callback*(list_view: Rid, callback: RListItemColorCallback, user_data: pointer) {.rapi.}
proc Rlist_view_set_selected*(list_view: Rid, index: cint) {.rapi.}
  ## Selects a row. -1 selects nothing.
proc Rlist_view_get_selected*(list_view: Rid): cint {.rapi.}
proc Rlist_view_scroll_to_item*(list_view: Rid, index: cint) {.rapi.}
proc Rlist_view_set_selection_changed_callback*(list_view: Rid, callback: RListEventCallback, user_data: pointer) {.rapi.}
proc Rlist_view_set_activated_callback*(list_view: Rid, callback: RListEventCallback, user_data: pointer) {.rapi.}
  ## Called on double click or Enter. This also fires the action of the widget.
proc Rtree_view_add_item*(tree_view: Rid, parent_item: Rid, text: cstring): Rid {.rapi.}
  ## Adds an item.
  ## parent_item Item to add under, or R_INVALID_ID for a top-level item.
  ## Returns: The handle of the new item.
proc Rtree_item_destroy*(item: Rid) {.rapi.}
  ## Deletes an item and its descendants.
proc Rtree_view_clear_items*(tree_view: Rid) {.rapi.}
proc Rtree_item_is_valid*(item: Rid): cint {.rapi.}
proc Rtree_item_set_text*(item: Rid, text: cstring) {.rapi.}
proc Rtree_item_get_text*(item: Rid): cstring {.rapi.}
proc Rtree_item_set_expanded*(item: Rid, expanded: cint) {.rapi.}
proc Rtree_item_is_expanded*(item: Rid): cint {.rapi.}
proc Rtree_item_set_user_handle*(item: Rid, handle: pointer) {.rapi.}
proc Rtree_item_get_user_handle*(item: Rid): pointer {.rapi.}
proc Rtree_view_get_item_count*(tree_view: Rid): cint {.rapi.}
  ## Top-level items of a tree.
proc Rtree_view_get_item*(tree_view: Rid, index: cint): Rid {.rapi.}
proc Rtree_item_get_parent*(item: Rid): Rid {.rapi.}
  ## Returns: The parent item, or R_INVALID_ID for a top-level item.
proc Rtree_item_get_child_count*(item: Rid): cint {.rapi.}
proc Rtree_item_get_child*(item: Rid, index: cint): Rid {.rapi.}
proc Rtree_view_set_selected_item*(tree_view: Rid, item: Rid) {.rapi.}
  ## Selects an item. R_INVALID_ID selects nothing.
proc Rtree_view_get_selected_item*(tree_view: Rid): Rid {.rapi.}
proc Rtree_view_set_selection_changed_callback*(tree_view: Rid, callback: RTreeEventCallback, user_data: pointer) {.rapi.}
proc Rtree_view_set_activated_callback*(tree_view: Rid, callback: RTreeEventCallback, user_data: pointer) {.rapi.}
  ## Called on double click or Enter. This also fires the action of the widget.
proc Rtree_item_set_icon*(item: Rid, texture: Rid, u0: cfloat, v0: cfloat, u1: cfloat, v1: cfloat) {.rapi.}
  ## Icon drawn before the text of the item (a region u0,v0,u1,v1 of the texture). R_INVALID_ID removes it.
proc Rtree_item_set_text_color*(item: Rid, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
  ## Color of the text of the item; alpha 0 goes back to the theme.
proc Rtree_view_set_multi_select*(tree_view: Rid, enabled: cint) {.rapi.}
  ## Several items can be selected: Ctrl + click adds or removes one,
  ## Shift + click selects a range. The selection callback receives the item
  ## clicked last (the primary one, Rtree_view_get_selected_item).
proc Rtree_item_set_selected*(item: Rid, selected: cint) {.rapi.}
proc Rtree_item_is_selected*(item: Rid): cint {.rapi.}
proc Rtree_view_clear_selection*(tree_view: Rid) {.rapi.}
proc Rtree_view_get_selection_count*(tree_view: Rid): cint {.rapi.}
  ## Selected items, top to bottom.
proc Rtree_view_get_selection*(tree_view: Rid, index: cint): Rid {.rapi.}
proc Rtree_view_scroll_to_item*(tree_view: Rid, item: Rid) {.rapi.}
proc Rtree_view_set_context_callback*(tree_view: Rid, callback: RTreeContextCallback, user_data: pointer) {.rapi.}
proc Rtree_view_set_drag_type*(tree_view: Rid, `type`: cstring) {.rapi.}
  ## Items can be dragged: the selection is given to Rdrag_begin with
  ## this type and, as data, the handles of the items in decimal separated
  ## by spaces. NULL: no drag (default).
proc Rtree_view_get_item_at*(tree_view: Rid, local_x: cfloat, local_y: cfloat): Rid {.rapi.}
  ## Item at a point relative to the tree (drop callbacks give local_x, local_y), or R_INVALID_ID.
proc Rtree_view_set_drop_highlight*(tree_view: Rid, item: Rid, whole_tree: cint) {.rapi.}
  ## Highlights where a drop would go: an item, or the whole tree (`whole_tree`). R_INVALID_ID, 0 removes it.
proc Rmenu_bar_set_native*(menu_bar: Rid, native: cint) {.rapi.}
  ## The menu bar of the system. On macOS, the first MenuBar of a
  ## window is shown in the bar at the top of the screen while the window is
  ## the active one, after the menu of the application (About, Hide, Quit);
  ## it then takes no room in the window. Its shortcuts are those of the
  ## system: R_MOD_SUPER is Command, R_MOD_CTRL is Control. Elsewhere it is
  ## drawn in the window. On by default; R_FALSE keeps it in the window
  ## everywhere. In XML: <MenuBar Native="false">.
proc Rmenu_bar_is_native*(menu_bar: Rid): cint {.rapi.}
  ## Returns: R_TRUE while the menu bar is shown by the system instead of the window.
proc Rmenu_add_menu*(menu_bar_or_menu: Rid, title: cstring): Rid {.rapi.}
  ## Creates a menu inside a menu bar or as a sub-menu of another menu.
proc Rmenu_set_title*(menu: Rid, title: cstring) {.rapi.}
proc Rmenu_add_item*(menu: Rid, text: cstring): Rid {.rapi.}
proc Rmenu_add_separator*(menu: Rid): Rid {.rapi.}
proc Rmenu_item_set_text*(item: Rid, text: cstring) {.rapi.}
proc Rmenu_item_get_text*(item: Rid): cstring {.rapi.}
proc Rmenu_item_set_shortcut*(item: Rid, key: RKey, modifiers: uint32) {.rapi.}
  ## Shows a shortcut next to the item and triggers the item when the
  ## keys are pressed in the window of its menu bar.
proc Rmenu_item_set_shortcut_text*(item: Rid, text: cstring) {.rapi.}
  ## Display-only shortcut hint; copied, never binds a key. Overrides
  ## the formatted shortcut without changing its binding. NULL restores the
  ## formatted shortcut; an empty string hides it. XML: ShortcutText.
proc Rmenu_item_set_checkable*(item: Rid, checkable: cint) {.rapi.}
proc Rmenu_item_set_checked*(item: Rid, checked: cint) {.rapi.}
proc Rmenu_item_set_icon*(item: Rid, texture: Rid, u0: cfloat, v0: cfloat, u1: cfloat, v1: cfloat, tint: cint) {.rapi.}
  ## Icon drawn before the text of a menu item (after the check mark column).
  ## texture R_INVALID_ID removes it. u0, v0, u1, v1 : region of the texture.
  ## tint    R_TRUE: the texture is a mask drawn in the text color (monochrome icons).
proc Rmenu_item_is_checked*(item: Rid): cint {.rapi.}
proc Rmenu_item_set_clicked_callback*(item: Rid, callback: RMenuItemClickedCallback, user_data: pointer) {.rapi.}
proc Rmenu_open*(menu: Rid, window: Rid, x: cfloat, y: cfloat) {.rapi.}
  ## Opens a menu as a popup at a position of the window (logical units).
proc Rwindow_close_popups*(window: Rid) {.rapi.}
  ## Closes every open menu and popup of the window.
proc Rtile_view_set_items*(tile_view: Rid, count: cint, callback: RTileItemCallback, user_data: pointer) {.rapi.}
  ## Gives the tiles: `count` of them, described by `callback`. The selection is cleared.
proc Rtile_view_refresh*(tile_view: Rid) {.rapi.}
  ## Draws the tiles again (their description changed, not their number).
proc Rtile_view_get_count*(tile_view: Rid): cint {.rapi.}
proc Rtile_view_set_tile_size*(tile_view: Rid, width: cfloat, height: cfloat, icon_size: cfloat) {.rapi.}
  ## Size of a tile (its name included) and of its icon. Default 112 x 112, icon 64.
proc Rtile_view_set_empty_text*(tile_view: Rid, text: cstring) {.rapi.}
proc Rtile_view_set_selected*(tile_view: Rid, index: cint, selected: cint) {.rapi.}
proc Rtile_view_is_selected*(tile_view: Rid, index: cint): cint {.rapi.}
proc Rtile_view_clear_selection*(tile_view: Rid) {.rapi.}
proc Rtile_view_select_only*(tile_view: Rid, index: cint) {.rapi.}
proc Rtile_view_get_current*(tile_view: Rid): cint {.rapi.}
  ## The tile clicked last, or -1.
proc Rtile_view_get_selection_count*(tile_view: Rid): cint {.rapi.}
proc Rtile_view_get_selection*(tile_view: Rid, nth: cint): cint {.rapi.}
  ## Index of the nth selected tile, in order.
proc Rtile_view_scroll_to*(tile_view: Rid, index: cint) {.rapi.}
proc Rtile_view_get_item_at*(tile_view: Rid, local_x: cfloat, local_y: cfloat): cint {.rapi.}
  ## Tile at a point relative to the widget (drop callbacks give local_x, local_y), or -1.
proc Rtile_view_set_drop_highlight*(tile_view: Rid, index: cint) {.rapi.}
  ## Highlights the tile a drop would go to; -1 removes it.
proc Rtile_view_set_drag_type*(tile_view: Rid, `type`: cstring) {.rapi.}
  ## Tiles can be dragged: the selected tiles are given to Rdrag_begin
  ## with this type and, as data, their indices separated by spaces.
proc Rtile_view_set_drag_data_callback*(tile_view: Rid, callback: RTileDragDataCallback, user_data: pointer) {.rapi.}
proc Rtile_view_set_selection_changed_callback*(tile_view: Rid, callback: RTileEventCallback, user_data: pointer) {.rapi.}
proc Rtile_view_set_activated_callback*(tile_view: Rid, callback: RTileEventCallback, user_data: pointer) {.rapi.}
proc Rtile_view_set_context_callback*(tile_view: Rid, callback: RTileContextCallback, user_data: pointer) {.rapi.}
proc Rtable_add_column*(table: Rid, header: cstring, width: cfloat): cint {.rapi.}
  ## Adds a column.
  ## width Width in logical units; 0 or less gives the default (120).
  ## Returns: The index of the column. The stored rows get an empty cell in it.
proc Rtable_get_column_count*(table: Rid): cint {.rapi.}
proc Rtable_clear_columns*(table: Rid) {.rapi.}
  ## Removes the columns and the stored rows.
proc Rtable_set_column_header*(table: Rid, column: cint, header: cstring) {.rapi.}
proc Rtable_get_column_header*(table: Rid, column: cint): cstring {.rapi.}
proc Rtable_set_column_width*(table: Rid, column: cint, width: cfloat) {.rapi.}
proc Rtable_get_column_width*(table: Rid, column: cint): cfloat {.rapi.}
  ## The width the column has on screen (a filling column takes more than its width).
proc Rtable_set_column_fill*(table: Rid, column: cint, fill: cfloat) {.rapi.}
  ## > 0: the column takes this share of the room the others leave; 0: its width only.
proc Rtable_set_column_sortable*(table: Rid, column: cint, sortable: cint) {.rapi.}
  ## R_FALSE: a click on the header does not sort (default R_TRUE).
proc Rtable_set_column_align*(table: Rid, column: cint, align: cfloat) {.rapi.}
  ## Where the texts sit in the cells: 0 left, 0.5 center, 1 right.
proc Rtable_add_row*(table: Rid, cells: cstring): cint {.rapi.}
  ## Adds a row.
  ## cells The texts of the cells, separated by '\|' ("Ada\|36\|London"),
  ##              or NULL for an empty row; Rtable_set_cell gives a text holding a '\|'.
  ## Returns: The index of the row.
proc Rtable_set_cell*(table: Rid, row: cint, column: cint, text: cstring) {.rapi.}
proc Rtable_get_cell*(table: Rid, row: cint, column: cint): cstring {.rapi.}
proc Rtable_remove_row*(table: Rid, row: cint) {.rapi.}
proc Rtable_clear_rows*(table: Rid) {.rapi.}
proc Rtable_get_row_count*(table: Rid): cint {.rapi.}
proc Rtable_set_virtual_rows*(table: Rid, count: cint, callback: RTableCellTextCallback, user_data: pointer) {.rapi.}
proc Rtable_set_cell_color_callback*(table: Rid, callback: RTableCellColorCallback, user_data: pointer) {.rapi.}
proc Rtable_set_selected*(table: Rid, row: cint) {.rapi.}
  ## Selects a row. -1 selects nothing.
proc Rtable_get_selected*(table: Rid): cint {.rapi.}
proc Rtable_scroll_to_row*(table: Rid, row: cint) {.rapi.}
proc Rtable_sort*(table: Rid, column: cint, descending: cint) {.rapi.}
  ## Sorts by a column (stored rows are moved); -1 shows no sorted column.
proc Rtable_get_sort_column*(table: Rid): cint {.rapi.}
proc Rtable_is_sort_descending*(table: Rid): cint {.rapi.}
proc Rtable_set_header_visible*(table: Rid, visible: cint) {.rapi.}
proc Rtable_set_grid_lines*(table: Rid, visible: cint) {.rapi.}
proc Rtable_set_striped*(table: Rid, striped: cint) {.rapi.}
  ## Every other row a little darker.
proc Rtable_set_selection_changed_callback*(table: Rid, callback: RTableEventCallback, user_data: pointer) {.rapi.}
proc Rtable_set_activated_callback*(table: Rid, callback: RTableEventCallback, user_data: pointer) {.rapi.}
  ## Called on double click or Enter. This also fires the action of the widget.
proc Rtable_set_sort_callback*(table: Rid, callback: RTableSortCallback, user_data: pointer) {.rapi.}
proc Rdate_picker_set_date*(date_picker: Rid, year: cint, month: cint, day: cint) {.rapi.}
  ## Sets the date (today when the picker is created). An invalid date is refused.
proc Rdate_picker_get_date*(date_picker: Rid, year: ptr cint, month: ptr cint, day: ptr cint) {.rapi.}
proc Rdate_picker_set_range*(date_picker: Rid, min_year: cint, min_month: cint, min_day: cint, max_year: cint, max_month: cint, max_day: cint) {.rapi.}
  ## The earliest and the latest dates that can be chosen; 0 for year leaves that side open.
proc Rdate_picker_set_first_day_of_week*(date_picker: Rid, day: cint) {.rapi.}
  ## 0 = Sunday (default), 1 = Monday...
proc Rdate_picker_set_format*(date_picker: Rid, format: cstring) {.rapi.}
  ## How the field writes the date: YYYY, MM, M, DD, D, MMMM (the name
  ## of the month), MMM (its first three letters), dddd (the name of the day),
  ## ddd; anything else is kept. Default "YYYY-MM-DD"; "D MMMM YYYY" gives "3 November 2026".
proc Rdate_picker_get_text*(date_picker: Rid): cstring {.rapi.}
  ## The date as the field shows it (valid until the next call).
proc Rdate_picker_set_open*(date_picker: Rid, open: cint) {.rapi.}
  ## Opens or closes the calendar.
proc Rdate_picker_is_open*(date_picker: Rid): cint {.rapi.}
proc Rdate_picker_set_changed_callback*(date_picker: Rid, callback: RDateChangedCallback, user_data: pointer) {.rapi.}
proc Rtime_picker_set_time*(time_picker: Rid, hour: cint, minute: cint, second: cint) {.rapi.}
proc Rtime_picker_get_time*(time_picker: Rid, hour: ptr cint, minute: ptr cint, second: ptr cint) {.rapi.}
proc Rtime_picker_set_seconds_visible*(time_picker: Rid, visible: cint) {.rapi.}
proc Rtime_picker_set_minute_step*(time_picker: Rid, step: cint) {.rapi.}
  ## Step of the minutes for the arrows and the wheel (default 1).
proc Rtime_picker_set_changed_callback*(time_picker: Rid, callback: RTimeChangedCallback, user_data: pointer) {.rapi.}
proc Rtoast_show*(toast: Rid, text: cstring, seconds: cdouble) {.rapi.}
  ## Shows a message for `seconds` (0 or less: until Rtoast_hide), with the kind and button set now.
proc Rtoast_show_kind*(toast: Rid, text: cstring, kind: RToastKind, seconds: cdouble) {.rapi.}
  ## Same, with a kind of its own (the kind set by Rtoast_set_kind is left as it is).
proc Rtoast_hide*(toast: Rid) {.rapi.}
  ## Hides the newest message shown; the next one waiting comes.
proc Rtoast_clear*(toast: Rid) {.rapi.}
  ## Hides every message, and forgets those waiting.
proc Rtoast_is_shown*(toast: Rid): cint {.rapi.}
proc Rtoast_get_count*(toast: Rid): cint {.rapi.}
  ## Returns: The messages shown and waiting.
proc Rtoast_get_text*(toast: Rid): cstring {.rapi.}
  ## Returns: The newest message shown (the last one, once hidden; the clicked one in the action callback).
proc Rtoast_set_queue*(toast: Rid, queue: RToastQueue) {.rapi.}
proc Rtoast_get_queue*(toast: Rid): RToastQueue {.rapi.}
proc Rtoast_set_max_visible*(toast: Rid, count: cint) {.rapi.}
  ## How many messages a R_TOAST_STACK shows at once (default 3).
proc Rtoast_set_kind*(toast: Rid, kind: RToastKind) {.rapi.}
  ## The kind of the next messages (and of the message shown, with R_TOAST_REPLACE).
proc Rtoast_set_action_text*(toast: Rid, text: cstring) {.rapi.}
  ## A button at the end of the next messages ("Undo"); NULL or "" removes it.
proc Rtoast_set_action_callback*(toast: Rid, callback: RToastCallback, user_data: pointer) {.rapi.}
proc Rbreadcrumb_set_items*(breadcrumb: Rid, items: cstring) {.rapi.}
  ## Replaces the items: their texts separated by '\|'.
proc Rbreadcrumb_add_item*(breadcrumb: Rid, text: cstring): cint {.rapi.}
  ## Returns: The index of the new item, at the end.
proc Rbreadcrumb_truncate*(breadcrumb: Rid, count: cint) {.rapi.}
  ## Keeps the first `count` items: Rbreadcrumb_truncate(b, index + 1) goes back to the item clicked.
proc Rbreadcrumb_get_item_count*(breadcrumb: Rid): cint {.rapi.}
proc Rbreadcrumb_get_item_text*(breadcrumb: Rid, index: cint): cstring {.rapi.}
proc Rbreadcrumb_set_clicked_callback*(breadcrumb: Rid, callback: RBreadcrumbCallback, user_data: pointer) {.rapi.}
proc Rrating_set_value*(rating: Rid, value: cfloat) {.rapi.}
  ## The score, from 0 to the number of stars.
proc Rrating_get_value*(rating: Rid): cfloat {.rapi.}
proc Rrating_set_max*(rating: Rid, max: cint) {.rapi.}
  ## Number of stars (default 5, at most 20).
proc Rrating_get_max*(rating: Rid): cint {.rapi.}
proc Rrating_set_allow_half*(rating: Rid, allow: cint) {.rapi.}
  ## Half stars can be chosen (a whole star by default).
proc Rrating_set_read_only*(rating: Rid, read_only: cint) {.rapi.}
  ## Shown only, the user cannot change it.
proc Rrating_set_star_size*(rating: Rid, size: cfloat) {.rapi.}
  ## Size of a star (default 18).
proc Rrating_set_changed_callback*(rating: Rid, callback: RRatingChangedCallback, user_data: pointer) {.rapi.}
proc Rpagination_set_page_count*(pagination: Rid, count: cint) {.rapi.}
proc Rpagination_get_page_count*(pagination: Rid): cint {.rapi.}
proc Rpagination_set_page*(pagination: Rid, page: cint) {.rapi.}
proc Rpagination_get_page*(pagination: Rid): cint {.rapi.}
proc Rpagination_set_siblings*(pagination: Rid, siblings: cint) {.rapi.}
  ## Page numbers shown around the current one (default 1).
proc Rpagination_set_changed_callback*(pagination: Rid, callback: RPageChangedCallback, user_data: pointer) {.rapi.}
proc Rplot_set_title*(plot: Rid, title: cstring) {.rapi.}
proc Rplot_get_title*(plot: Rid): cstring {.rapi.}
proc Rplot_add_series*(plot: Rid, name: cstring): cint {.rapi.}
  ## Adds a series (its name in the legend; NULL or "": not in the legend). Returns its index.
proc Rplot_remove_series*(plot: Rid, series: cint) {.rapi.}
proc Rplot_clear*(plot: Rid) {.rapi.}
  ## Removes every series and every drag line.
proc Rplot_get_series_count*(plot: Rid): cint {.rapi.}
proc Rplot_set_series_name*(plot: Rid, series: cint, name: cstring) {.rapi.}
proc Rplot_get_series_name*(plot: Rid, series: cint): cstring {.rapi.}
proc Rplot_set_series_data*(plot: Rid, series: cint, x: ptr cdouble, y: ptr cdouble, count: cint) {.rapi.}
  ## Replaces the points of a series with `count` points, copied.
  ## x The X of the points, or NULL: 0, 1, 2...
  ## y The Y of the points. NaN leaves a gap in the line.
proc Rplot_append_series_data*(plot: Rid, series: cint, x: ptr cdouble, y: ptr cdouble, count: cint) {.rapi.}
  ## Adds points after those of a series (x NULL: numbered after the last one).
proc Rplot_add_point*(plot: Rid, series: cint, x: cdouble, y: cdouble) {.rapi.}
  ## Adds one point after those of a series.
proc Rplot_clear_series_data*(plot: Rid, series: cint) {.rapi.}
proc Rplot_get_point_count*(plot: Rid, series: cint): cint {.rapi.}
proc Rplot_get_point*(plot: Rid, series: cint, index: cint, x: ptr cdouble, y: ptr cdouble): cint {.rapi.}
  ## One point of a series. Returns R_FALSE when there is no such point.
proc Rplot_set_series_max_points*(plot: Rid, series: cint, max: cint) {.rapi.}
  ## Keeps only the last `max` points of a series, for data that streams in (0: all of them, the default).
proc Rplot_set_series_color*(plot: Rid, series: cint, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
  ## The color of a series (by default the next one of a palette of ten).
proc Rplot_set_series_line*(plot: Rid, series: cint, line: RPlotLine, width: cfloat) {.rapi.}
  ## How the line is drawn and how wide (default solid, 1.75; width 0 keeps the width).
proc Rplot_set_series_marker*(plot: Rid, series: cint, marker: RPlotMarker, size: cfloat) {.rapi.}
  ## The mark on each point and its radius (default none, 4; size 0 keeps the size).
proc Rplot_set_series_marker_filled*(plot: Rid, series: cint, filled: cint) {.rapi.}
  ## Filled markers (the default) or only their outline.
proc Rplot_set_series_step*(plot: Rid, series: cint, step: cint) {.rapi.}
  ## Draws the line in steps: along at the height of a point, then up or down to the next.
proc Rplot_set_series_fill*(plot: Rid, series: cint, opacity: cfloat) {.rapi.}
  ## Shades the area between the line and zero, at this opacity (0: none, the default).
proc Rplot_set_series_axes*(plot: Rid, series: cint, x_axis: RPlotAxis, y_axis: RPlotAxis) {.rapi.}
  ## The axes a series is drawn against: R_PLOT_X or R_PLOT_X2, R_PLOT_Y or R_PLOT_Y2.
proc Rplot_set_series_visible*(plot: Rid, series: cint, visible: cint) {.rapi.}
  ## Shows or hides a series (the user does it with a click on its name in the legend).
proc Rplot_is_series_visible*(plot: Rid, series: cint): cint {.rapi.}
proc Rplot_set_axis_label*(plot: Rid, axis: RPlotAxis, label: cstring) {.rapi.}
  ## The name written along an axis.
proc Rplot_get_axis_label*(plot: Rid, axis: RPlotAxis): cstring {.rapi.}
proc Rplot_set_axis_scale*(plot: Rid, axis: RPlotAxis, scale: RPlotScale) {.rapi.}
proc Rplot_get_axis_scale*(plot: Rid, axis: RPlotAxis): RPlotScale {.rapi.}
proc Rplot_set_axis_linear_threshold*(plot: Rid, axis: RPlotAxis, threshold: cdouble) {.rapi.}
  ## R_PLOT_SCALE_SYMLOG: the axis is linear from -threshold to threshold (default 1).
proc Rplot_set_axis_range*(plot: Rid, axis: RPlotAxis, min: cdouble, max: cdouble) {.rapi.}
  ## Shows this range of an axis; the axis then stops following the data.
proc Rplot_get_axis_range*(plot: Rid, axis: RPlotAxis, min: ptr cdouble, max: ptr cdouble) {.rapi.}
  ## The range an axis shows now (after the user moved or zoomed it).
proc Rplot_set_axis_auto_fit*(plot: Rid, axis: RPlotAxis, auto_fit: cint) {.rapi.}
  ## The axis is fitted to the data whenever it changes (the default, until the view is moved; a double click turns it on again).
proc Rplot_is_axis_auto_fit*(plot: Rid, axis: RPlotAxis): cint {.rapi.}
proc Rplot_set_axis_format*(plot: Rid, axis: RPlotAxis, format: cstring) {.rapi.}
  ## How the labels of an axis are written: a printf format for one double ("%.1f s", "%g"), NULL: chosen from the ticks.
proc Rplot_set_axis_visible*(plot: Rid, axis: RPlotAxis, visible: cint) {.rapi.}
  ## 1: shown, 0: hidden, -1: R_PLOT_X and R_PLOT_Y always, the second axes when something uses them (the default).
proc Rplot_set_axis_grid*(plot: Rid, axis: RPlotAxis, grid: cint) {.rapi.}
  ## Lines across the data at the ticks of an axis (on for R_PLOT_X and R_PLOT_Y).
proc Rplot_set_axis_inverted*(plot: Rid, axis: RPlotAxis, inverted: cint) {.rapi.}
  ## The values grow the other way (to the left, or down).
proc Rplot_set_axis_locked*(plot: Rid, axis: RPlotAxis, locked: cint) {.rapi.}
  ## The user cannot move or zoom this axis (the application still can).
proc Rplot_fit*(plot: Rid) {.rapi.}
  ## Fits every axis that is not locked to the data, and has them follow it again (what a double click does).
proc Rplot_set_legend*(plot: Rid, legend: RPlotLegend) {.rapi.}
  ## Where the legend is (default R_PLOT_LEGEND_TOP_RIGHT).
proc Rplot_set_crosshair*(plot: Rid, crosshair: cint) {.rapi.}
  ## Lines across the data through the pointer.
proc Rplot_set_mouse_text*(plot: Rid, shown: cint) {.rapi.}
  ## The coordinates of the pointer in a corner of the data (on by default).
proc Rplot_set_tooltips*(plot: Rid, shown: cint) {.rapi.}
  ## The values of the point under the pointer, written next to it (on by default).
proc Rplot_get_mouse_position*(plot: Rid, x: ptr cdouble, y: ptr cdouble): cint {.rapi.}
  ## Where the pointer is, in the units of R_PLOT_X and R_PLOT_Y. Returns R_TRUE when it is over the data.
proc Rplot_data_to_point*(plot: Rid, x: cdouble, y: cdouble, point_x: ptr cfloat, point_y: ptr cfloat) {.rapi.}
  ## Where a point of the data (R_PLOT_X, R_PLOT_Y) is in the widget, in logical units from its corner.
proc Rplot_point_to_data*(plot: Rid, point_x: cfloat, point_y: cfloat, x: ptr cdouble, y: ptr cdouble) {.rapi.}
  ## The data (R_PLOT_X, R_PLOT_Y) at a point of the widget, in logical units from its corner.
proc Rplot_add_drag_line*(plot: Rid, axis: RPlotAxis, value: cdouble, label: cstring): cint {.rapi.}
  ## Adds a line across the data that the user drags: vertical at an X
  ## (axis R_PLOT_X or R_PLOT_X2), horizontal at a Y (R_PLOT_Y or R_PLOT_Y2).
  ## Its value is written on it, after `label` (may be NULL). Returns its index.
proc Rplot_remove_drag_line*(plot: Rid, line: cint) {.rapi.}
proc Rplot_get_drag_line_count*(plot: Rid): cint {.rapi.}
proc Rplot_set_drag_line_value*(plot: Rid, line: cint, value: cdouble) {.rapi.}
proc Rplot_get_drag_line_value*(plot: Rid, line: cint): cdouble {.rapi.}
proc Rplot_set_drag_line_color*(plot: Rid, line: cint, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
  ## The color of a drag line (by default the accent of the theme).
proc Rplot_set_view_changed_callback*(plot: Rid, callback: RPlotViewCallback, user_data: pointer) {.rapi.}
proc Rplot_set_drag_line_callback*(plot: Rid, callback: RPlotDragLineCallback, user_data: pointer) {.rapi.}
proc Rplot_set_series_toggled_callback*(plot: Rid, callback: RPlotSeriesCallback, user_data: pointer) {.rapi.}
proc Rtext_editor_create*(text: cstring): Rid {.rapi.}
  ## text Initial content, or NULL.
proc Rtext_editor_set_text*(editor: Rid, text: cstring) {.rapi.}
  ## Replaces the whole text. The history of undo is cleared; the changed callback is not called.
proc Rtext_editor_get_text*(editor: Rid): cstring {.rapi.}
  ## Returns: The whole text, lines separated by '\n'. Valid until the next call or the next edit.
proc Rtext_editor_get_length*(editor: Rid): cint {.rapi.}
  ## Returns: The number of bytes of Rtext_editor_get_text.
proc Rtext_editor_get_line_count*(editor: Rid): cint {.rapi.}
proc Rtext_editor_get_line*(editor: Rid, line: cint): cstring {.rapi.}
  ## Returns: One line, without its line break. Valid until the next edit. NULL when `line` is out of range.
proc Rtext_editor_insert_text*(editor: Rid, text: cstring) {.rapi.}
  ## Types a text at the cursor, replacing the selection, as the user
  ## would: it can be undone and the changed callback is called.
proc Rtext_editor_set_cursor*(editor: Rid, line: cint, column: cint) {.rapi.}
proc Rtext_editor_get_cursor*(editor: Rid, line: ptr cint, column: ptr cint) {.rapi.}
proc Rtext_editor_select*(editor: Rid, from_line: cint, from_column: cint, to_line: cint, to_column: cint) {.rapi.}
  ## Selects from one position to another; the cursor is on the second.
proc Rtext_editor_select_all*(editor: Rid) {.rapi.}
proc Rtext_editor_get_selection*(editor: Rid, from_line: ptr cint, from_column: ptr cint, to_line: ptr cint, to_column: ptr cint): cint {.rapi.}
  ## Start and end of the selection, in the order of the text. Returns R_FALSE when nothing is selected.
proc Rtext_editor_get_selected_text*(editor: Rid): cstring {.rapi.}
  ## Returns: The selected text, "" when there is none. Valid until the next call or the next edit.
proc Rtext_editor_scroll_to_line*(editor: Rid, line: cint) {.rapi.}
  ## Scrolls so that a line is visible.
proc Rtext_editor_undo*(editor: Rid) {.rapi.}
proc Rtext_editor_redo*(editor: Rid) {.rapi.}
proc Rtext_editor_can_undo*(editor: Rid): cint {.rapi.}
proc Rtext_editor_can_redo*(editor: Rid): cint {.rapi.}
proc Rtext_editor_set_read_only*(editor: Rid, read_only: cint) {.rapi.}
  ## A read-only editor can still be scrolled, selected and copied.
proc Rtext_editor_set_word_wrap*(editor: Rid, enabled: cint) {.rapi.}
  ## Long lines continue on the next row instead of scrolling sideways. Off by default.
proc Rtext_editor_set_line_numbers*(editor: Rid, enabled: cint) {.rapi.}
  ## Shows the line numbers in a margin. Off by default.
proc Rtext_editor_set_tab_size*(editor: Rid, spaces: cint) {.rapi.}
  ## Width of a tab, in spaces (4 by default).
proc Rtext_editor_set_tab_inserts_spaces*(editor: Rid, enabled: cint) {.rapi.}
  ## R_TRUE makes the Tab key insert spaces instead of a tab character. Off by default.
proc Rtext_editor_set_syntax*(editor: Rid, syntax: RSyntax) {.rapi.}
  ## The same as Rtext_editor_set_language with "XML", "C" or NULL.
proc Rtext_editor_set_font*(editor: Rid, font: Rid, size: cfloat) {.rapi.}
  ## font R_INVALID_ID uses the font of the theme (or the fixed-width
  ## font, with a language).
  ## size 0 uses the font size of the theme.
proc Rtext_editor_set_placeholder*(editor: Rid, text: cstring) {.rapi.}
  ## Text shown, dimmed, while the editor is empty.
proc Rtext_editor_set_zoom*(editor: Rid, zoom: cfloat) {.rapi.}
  ## Zoom of the text: a factor of its font size, 1 = 100 % (0.5 .. 4).
  ## The user zooms with Ctrl + wheel (around the pointer), Ctrl + '+',
  ## Ctrl + '-' and Ctrl + 0 (back to 100 %), keypad included. Set by the
  ## program, the top of the view stays and the zoom callback is not called.
  ## XML: Zoom="1.25".
proc Rtext_editor_get_zoom*(editor: Rid): cfloat {.rapi.}
proc Rtext_editor_set_zoomable*(editor: Rid, zoomable: cint) {.rapi.}
  ## R_FALSE: Ctrl + wheel and the keys do not zoom (the wheel scrolls). On by default. XML: Zoomable="false".
proc Rtext_editor_set_zoom_callback*(editor: Rid, callback: RTextEditorZoomCallback, user_data: pointer) {.rapi.}
proc Rlanguage_register*(desc: ptr RLanguageDesc): cint {.rapi.}
  ## Declares a language, or replaces the one that has this name: to
  ## add words to a built-in language, read it with Rlanguage_get, change
  ## what you want and register it again. The strings are copied.
proc Rlanguage_get*(name: cstring, desc: ptr RLanguageDesc): cint {.rapi.}
  ## Reads the description of a language. The strings belong to the library. Returns R_FALSE when it is unknown.
proc Rlanguage_get_count*(): cint {.rapi.}
proc Rlanguage_get_name*(index: cint): cstring {.rapi.}
proc Rlanguage_for_file*(path: cstring): cstring {.rapi.}
  ## Returns: The name of the language of a file, from its extension, or NULL.
proc Rtext_editor_set_language*(editor: Rid, name: cstring) {.rapi.}
  ## Gives the editor a language, by name ("C++", "Python"...), by
  ## another usual name ("cpp", "js") or by the extension of its files
  ## ("rs"). NULL or "" removes it. An editor with a language uses the
  ## fixed-width font unless it was given a font.
proc Rtext_editor_get_language*(editor: Rid): cstring {.rapi.}
  ## Returns: The name of the language of the editor, or "".
proc Rtext_editor_set_markdown*(editor: Rid, enabled: cint) {.rapi.}
  ## R_TRUE makes the editor an editor of Markdown (the "Markdown"
  ## language; Markdown="true" in XML): the text stays as it is written,
  ## and what it formats shows while typing, strong text in bold,
  ## emphasized text in italic, titles in bold, code, links and marks in
  ## their colors. A label with Rlabel_set_markdown shows the same text
  ## formatted. R_FALSE removes the language.
proc Rtext_editor_get_markdown*(editor: Rid): cint {.rapi.}
proc Rtext_editor_get_token*(editor: Rid, line: cint, column: cint): RToken {.rapi.}
  ## What the language of the editor makes of the character at a
  ## position: the colors come from it. R_TOKEN_TEXT without a language.
proc Rtext_editor_set_completion*(editor: Rid, enabled: cint) {.rapi.}
  ## R_FALSE stops the editor from proposing words by itself; Ctrl+Space still does. On by default.
proc Rtext_editor_show_completions*(editor: Rid) {.rapi.}
  ## Proposes words for what is before the cursor, as Ctrl+Space does.
proc Rtext_editor_is_completing*(editor: Rid): cint {.rapi.}
proc Rtext_editor_set_completion_callback*(editor: Rid, callback: RCompletionCallback, user_data: pointer) {.rapi.}
proc Rtext_editor_add_completion*(editor: Rid, text: cstring, detail: cstring) {.rapi.}
  ## Adds a proposal. Only from a completion callback. Proposals that
  ## do not start with the prefix are left out; the others keep the order
  ## they were added in.
  ## detail A few words shown beside it ("function", "int"), or NULL.
proc Rtext_editor_add_completion_insert*(editor: Rid, text: cstring, detail: cstring, insert: cstring) {.rapi.}
  ## Adds a proposal that writes another text than the one it shows
  ## and is matched on: "Threshold", listed and filtered as such, that
  ## writes "Threshold(" when chosen. Only from a completion callback.
  ## insert What replaces the word being typed; NULL or "" is `text`.
proc Rtext_editor_set_completion_words*(editor: Rid, enabled: cint) {.rapi.}
  ## R_FALSE leaves the words of the language and the names of the
  ## text out of the proposals: only the completion callback's are listed
  ## (a language server that knows better). On by default.
proc Rtext_editor_set_auto_pairs*(editor: Rid, enabled: cint) {.rapi.}
  ## R_FALSE stops the editor from closing brackets and quotes and
  ## from indenting after them. On by default; it only acts with a language.
proc Rtext_editor_add_marker*(editor: Rid, line: cint, from_column: cint, to_column: cint, kind: RMarkerKind, message: cstring) {.rapi.}
  ## Underlines a part of a line and attaches a message to it, shown
  ## when the pointer rests there: an error of a compiler, a warning.
  ## Markers follow their line when text is added or removed above it, and
  ## go away when their own line is edited.
  ## to_column -1 for the end of the line.
proc Rtext_editor_clear_markers*(editor: Rid) {.rapi.}
proc Rtext_editor_get_marker_count*(editor: Rid): cint {.rapi.}
proc Rtext_editor_set_hover_callback*(editor: Rid, callback: RTextEditorHoverCallback, user_data: pointer) {.rapi.}
proc Rtheme_set_token_color*(token: RToken, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
  ## Color of a kind of token in the theme in use, instead of the
  ## one Raven picks for dark and for light themes. Theme files set them
  ## with <Syntax Keyword="#FF7AB2" String="..."/>.
proc Rtheme_get_token_color*(token: RToken, r: ptr cfloat, g: ptr cfloat, b: ptr cfloat, a: ptr cfloat) {.rapi.}
proc Rtheme_clear_token_colors*() {.rapi.}
  ## Back to the colors Raven picks.
proc Rtext_editor_set_changed_callback*(editor: Rid, callback: RTextEditorChangedCallback, user_data: pointer) {.rapi.}
proc Rheader_bar_create*(title: cstring): Rid {.rapi.}
  ## title Text shown in the bar; NULL shows the title of the window.
proc Rheader_bar_set_title*(bar: Rid, title: cstring) {.rapi.}
  ## title NULL shows the title of the window.
proc Rheader_bar_set_buttons*(bar: Rid, buttons: uint32) {.rapi.}
  ## buttons Bitmask of RHeaderButtons. All by default; the maximize button only shows on a resizable window.
proc Rheader_bar_set_button_side*(bar: Rid, side: RHAlign) {.rapi.}
  ## side R_HALIGN_RIGHT: minimize, maximize, close, as on Windows.
  ## R_HALIGN_LEFT: three round colored buttons, as on macOS. Until it is
  ## set, the side is the one of the system the program runs on (left on
  ## macOS, right elsewhere), whatever the look.
proc Rheader_bar_set_title_align*(bar: Rid, align: RHAlign) {.rapi.}
  ## align R_HALIGN_CENTER (default) or R_HALIGN_LEFT.
proc Rheader_bar_set_title_notch*(bar: Rid, notch: cint) {.rapi.}
  ## R_TRUE: the title sits in a dark tab hanging from the top edge of
  ## the bar (slanted sides, round bottom corners). XML: TitleStyle="Notch".
proc Rheader_bar_set_background*(bar: Rid, enabled: cint) {.rapi.}
  ## R_FALSE removes the background of the bar: for a header that
  ## floats above the content (in an Overlay) or sits inside a glass panel.
proc Rdock_area_create*(): Rid {.rapi.}
proc Rdock_area_add_panel*(area: Rid, panel: Rid, title: cstring, side: RDockSide, relative: Rid, size: cfloat) {.rapi.}
  ## Adds a panel. The dock area takes ownership of the widget.
  ## title    Text of its tab.
  ## side     Where it goes.
  ## relative A panel of the area to split or to share tabs with, or
  ##                 R_INVALID_ID for the whole area (with R_DOCK_TAB: the
  ##                 first group of tabs).
  ## size     Part of the split space given to the new panel, 0..1
  ##                 (0 = a half, or a quarter along the whole area).
proc Rdock_area_move_panel*(area: Rid, panel: Rid, side: RDockSide, relative: Rid, size: cfloat) {.rapi.}
  ## Moves a panel of the area somewhere else in it.
proc Rdock_area_close_panel*(area: Rid, panel: Rid) {.rapi.}
  ## Removes a panel and deletes it.
proc Rdock_area_set_panel_title*(area: Rid, panel: Rid, title: cstring) {.rapi.}
proc Rdock_area_set_panel_icon*(area: Rid, panel: Rid, texture: Rid, u0: cfloat, v0: cfloat, u1: cfloat, v1: cfloat, tint: cint) {.rapi.}
  ## Icon drawn before the title of the tab of a panel (and on the chip while it is dragged).
  ## texture Texture of the icon, R_INVALID_ID removes it.
  ## u0,v0,u1,v1 Region of the texture (0, 0, 1, 1 : all of it).
  ## tint    R_TRUE: the texture is a mask drawn in the text color of the tab (monochrome icons).
proc Rdock_area_set_panel_closable*(area: Rid, panel: Rid, closable: cint) {.rapi.}
  ## R_FALSE removes the close button of the tab of a panel. Panels can be closed by default.
proc Rdock_area_hide_panel*(area: Rid, panel: Rid) {.rapi.}
  ## Takes a panel out of the arrangement without deleting it (a
  ## window of an application closed from its menu). Its tab disappears
  ## and its neighbours take its place; the area remembers where it was.
proc Rdock_area_open_panel*(area: Rid, panel: Rid) {.rapi.}
  ## Puts a hidden panel back where it was, and brings its tab to the front (also for a panel that is not hidden).
proc Rdock_area_is_panel_open*(area: Rid, panel: Rid): cint {.rapi.}
  ## Returns: R_TRUE when the panel is in the arrangement (not hidden), shown or behind another tab.
proc Rdock_area_set_close_hides*(area: Rid, hides: cint) {.rapi.}
  ## R_TRUE: the close button of a tab hides its panel
  ## (Rdock_area_hide_panel) instead of deleting it. XML: CloseHides="true".
  ## The close callback is asked first either way.
proc Rdock_area_show_panel*(area: Rid, panel: Rid) {.rapi.}
  ## Brings the tab of a panel to the front.
proc Rdock_area_is_panel_shown*(area: Rid, panel: Rid): cint {.rapi.}
  ## Returns: R_TRUE when the panel is the one shown in its group of tabs.
proc Rdock_area_set_movable*(area: Rid, movable: cint) {.rapi.}
  ## R_FALSE stops the user from moving the panels (the borders can still be dragged).
proc Rdock_area_save_layout*(area: Rid): cstring {.rapi.}
  ## Describes how the panels are arranged, as a short text to keep
  ## in a settings file. Panels are named by their widget name
  ## (Rwidget_set_name): unnamed panels are not part of the description.
  ## Returns: A string valid until the next call.
proc Rdock_area_load_layout*(area: Rid, layout: cstring): cint {.rapi.}
  ## Arranges the panels as a saved description says. Panels of the
  ## area that the description does not name keep a place; names it has
  ## that match no panel are ignored.
  ## Returns: R_FALSE when the text is not a layout (nothing is changed).
proc Rdock_area_set_close_callback*(area: Rid, callback: RDockCloseCallback, user_data: pointer) {.rapi.}
proc Rdock_area_set_tearable*(area: Rid, tearable: cint) {.rapi.}
  ## Tearing: a tab dragged out of the area takes its panel to a
  ## window of its own (a floating area of the same family), and a tab
  ## dragged from such a window over the area docks it back where it is
  ## dropped. A double click on the tab of a floating window, or closing
  ## that window, puts its panels back where they were. Dragging the only
  ## tab of a floating window moves the window. R_TRUE by default; XML:
  ## Tearable="false". The functions of this area that take a panel also
  ## work on its panels that are in floating windows.
proc Rdock_area_is_tearable*(area: Rid): cint {.rapi.}
proc Rdock_area_float_panel*(area: Rid, panel: Rid, x: cint, y: cint): Rid {.rapi.}
  ## Takes a panel to a window of its own, its top left corner at (x, y) on the screen. Returns the window.
proc Rdock_area_dock_panel*(area: Rid, panel: Rid) {.rapi.}
  ## A floating panel goes back where it was in the area.
proc Rdock_area_get_panel_window*(area: Rid, panel: Rid): Rid {.rapi.}
  ## The window a floating panel is in, R_INVALID_ID when it is docked in the area.
proc Rdock_area_set_changed_callback*(area: Rid, callback: RDockChangedCallback, user_data: pointer) {.rapi.}
proc Risland_create*(): Rid {.rapi.}
proc Risland_set_expanded*(island: Rid, expanded: cint) {.rapi.}
  ## Unfolds or folds the island. The changed callback is not called.
proc Risland_is_expanded*(island: Rid): cint {.rapi.}
proc Risland_expand_for*(island: Rid, seconds: cdouble) {.rapi.}
  ## Unfolds the island, and folds it back by itself after a while:
  ## to tell something to the user. It stays open while the pointer is on it.
proc Risland_set_expand_on_hover*(island: Rid, enabled: cint) {.rapi.}
  ## R_TRUE (default): the island unfolds under the pointer and folds back when it leaves.
proc Risland_set_expand_on_click*(island: Rid, enabled: cint) {.rapi.}
  ## R_TRUE (default): a click on the island unfolds or folds it.
proc Risland_set_attached*(island: Rid, attached: cint) {.rapi.}
  ## R_TRUE gives the island square upper corners: a notch that hangs
  ## from the edge it touches, instead of a capsule that floats under it.
proc Risland_set_notch*(island: Rid, enabled: cint) {.rapi.}
  ## R_TRUE: on a screen with a camera housing at its top (the notch
  ## of a MacBook), the island of a window (Rwindow_set_island) lives in the
  ## housing itself, in a window of its own anchored to the screen: it stays
  ## there when the window is moved, and takes the mouse and the keyboard as
  ## usual. Folded, it is the housing grown into two ears: the first Spacer
  ## of its folded content becomes the room of the camera (what is before
  ## it shows on the left, what is after on the right). Unfolded, its
  ## content starts under the camera.
  ##
  ## It is in the housing while its window is the last one of the
  ## application the user worked in, is shown, and is on that screen; it
  ## goes back into its window otherwise, and on the window systems and
  ## screens without a housing (where nothing changes). XML: Notch="true".
proc Risland_get_notch*(island: Rid): cint {.rapi.}
proc Risland_is_in_notch*(island: Rid): cint {.rapi.}
  ## R_TRUE while the island is in the camera housing of the screen.
proc Risland_set_changed_callback*(island: Rid, callback: RIslandChangedCallback, user_data: pointer) {.rapi.}
proc Rwindow_set_island*(window: Rid, widget: Rid) {.rapi.}
  ## Shows a widget (usually an island) floating at the top of a
  ## window, in the middle, above its content. The window takes ownership;
  ## the previous one is deleted. R_INVALID_ID removes it. The padding of
  ## its slot (Rslot_set_padding) is its distance from the top edge.
proc Rwindow_get_island*(window: Rid): Rid {.rapi.}
proc Rwindow_set_taskbar_progress*(window: Rid, state: RTaskbarProgress, value: cfloat): cint {.rapi.}
  ## A progress bar on the icon of the application: on the button of
  ## the window in the taskbar (Windows), on the icon in the Dock (macOS),
  ## on the launcher (Linux). There is one icon for the whole application in
  ## the Dock and the launchers: the last window that set it is shown.
  ## value From 0 to 1 (not used by NONE and INDETERMINATE).
  ## Returns: R_FALSE where the system shows no progress.
proc Rwindow_request_attention*(window: Rid, critical: cint): cint {.rapi.}
  ## Asks the user to look at the window when it is not the active
  ## one: its taskbar button flashes (Windows), the icon bounces in the
  ## Dock (macOS), the window is marked urgent (X11, the launchers of
  ## Linux). `critical`: until the user comes to the window; otherwise a few
  ## times. Nothing happens when the window is already the active one.
proc Rtaskbar_set_badge*(text: cstring): cint {.rapi.}
  ## A short text on the icon of the application: a number of unread
  ## messages, "!". macOS: the badge of the Dock. Windows: a small round
  ## icon on the taskbar buttons of the windows, drawn from the text (one
  ## to three characters fit). Linux: a number only (the text is read as
  ## one; anything else shows nothing). NULL or "": no badge.
proc Rtaskbar_set_menu*(menu: Rid): cint {.rapi.}
  ## The menu of the icon of the application in the Dock (macOS),
  ## above the entries of the system. A MENU without a parent: its
  ## MenuItems fire as when chosen in a window (clicked callback, action),
  ## and what changes in it shows the next time the menu opens.
  ## R_INVALID_ID: none. R_FALSE where the system has no such menu.
proc Rtaskbar_set_jump_list*(tasks: ptr RJumpTask, count: cint): cint {.rapi.}
  ## The tasks of the jump list of Windows (right click on the button
  ## of the application in the taskbar, or on its tile in the Start menu).
  ## Choosing one starts the application again with its arguments: the
  ## application reads them as any other command line. count 0: none.
  ## R_FALSE where there is no jump list.
proc Rtray_is_supported*(): cint {.rapi.}
  ## Returns: R_TRUE when the system shows tray icons: always on Windows and
  ## macOS; on Linux, when a host of StatusNotifierItem runs (the panel of
  ## the desktop). An icon made where none runs yet appears when one starts.
proc Rtray_create*(): Rid {.rapi.}
  ## Puts an icon of the application beside the clock. While one
  ## exists, the application goes on running when its last window closes
  ## (Ris_running): Rquit (or the action app.quit) ends it.
  ## Returns: R_INVALID_ID where the system has none.
proc Rtray_destroy*(tray: Rid) {.rapi.}
proc Rtray_is_valid*(tray: Rid): cint {.rapi.}
proc Rtray_set_icon*(tray: Rid, rgba: ptr uint8, width: cint, height: cint) {.rapi.}
  ## The picture, from tightly packed RGBA8 pixels (straight alpha);
  ## the system scales it (16 to 32 pixels square fit everywhere). Copied.
proc Rtray_set_tooltip*(tray: Rid, text: cstring) {.rapi.}
  ## The text shown when the pointer rests on the icon (and its title for the Linux panels).
proc Rtray_set_menu*(tray: Rid, menu: Rid) {.rapi.}
  ## The menu the system shows for the icon: on a right click
  ## (Windows, Linux), on a click (macOS). A MENU without a parent, whose
  ## MenuItems fire as when chosen in a window; what changes in it shows
  ## the next time it opens. R_INVALID_ID: none.
proc Rtray_set_clicked_callback*(tray: Rid, callback: RTrayClickedCallback, user_data: pointer) {.rapi.}
  ## Called on a click: the left button (activate), the right one
  ## when there is no menu, the middle one. On macOS an icon with a menu
  ## opens it instead.
proc Rtray_set_title*(tray: Rid, text: cstring) {.rapi.}
  ## A short text shown beside the icon (macOS menu bar), drawn by the
  ## system in its own font and colors; the item widens to fit it. NULL or
  ## "": the icon alone. Systems that show no text there ignore it (the
  ## tooltip is the place for it).
proc Rtray_get_rect*(tray: Rid, x: ptr cint, y: ptr cint, width: ptr cint, height: ptr cint): cint {.rapi.}
  ## Where the icon is on the screen, in the units and coordinates of
  ## Rwindow_set_position (a window can then open just under it).
  ## Returns: R_FALSE when the system does not tell (the outputs are then 0).
proc Rcolor_picker_create*(): Rid {.rapi.}
proc Rcolor_picker_set_color*(picker: Rid, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
  ## Sets the color (0..1 each). The callback is not called.
proc Rcolor_picker_get_color*(picker: Rid, r: ptr cfloat, g: ptr cfloat, b: ptr cfloat, a: ptr cfloat) {.rapi.}
proc Rcolor_picker_set_hsv*(picker: Rid, hue: cfloat, saturation: cfloat, value: cfloat) {.rapi.}
  ## The same color as hue (0..360), saturation and value (0..1).
  ## The picker remembers the hue of a gray and the saturation of black,
  ## which red, green and blue cannot tell.
proc Rcolor_picker_get_hsv*(picker: Rid, hue: ptr cfloat, saturation: ptr cfloat, value: ptr cfloat) {.rapi.}
proc Rcolor_picker_set_hex*(picker: Rid, text: cstring): cint {.rapi.}
  ## Sets the color from text: "#RGB", "#RRGGBB", "#RRGGBBAA" (the
  ## '#' may be left out) or one of the color names of the XML attributes.
  ## A code without opacity leaves the opacity as it is.
  ## Returns: R_FALSE when the text is not a color (the color is unchanged).
proc Rcolor_picker_get_hex*(picker: Rid): cstring {.rapi.}
  ## Returns: "#RRGGBB", or "#RRGGBBAA" when the opacity is shown and not full. Valid until the color changes.
proc Rcolor_picker_set_alpha_enabled*(picker: Rid, enabled: cint) {.rapi.}
  ## R_TRUE (default) shows the opacity strip and the fourth number. R_FALSE: colors are opaque.
proc Rcolor_picker_set_previous_color*(picker: Rid, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
  ## The color shown next to the current one, to compare with what
  ## was there before: clicking it comes back to it. Rcolor_picker_set_color
  ## does not change it; a popup picker takes it when it opens.
proc Rcolor_picker_add_swatch*(picker: Rid, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
  ## Adds a color to the row of swatches: one click takes it.
proc Rcolor_picker_clear_swatches*(picker: Rid) {.rapi.}
proc Rcolor_picker_get_swatch_count*(picker: Rid): cint {.rapi.}
proc Rcolor_picker_set_popup*(picker: Rid, popup: cint) {.rapi.}
  ## R_TRUE turns the widget into a swatch that opens the picker in a popup.
proc Rcolor_picker_set_compact*(picker: Rid, compact: cint) {.rapi.}
  ## R_TRUE hides the code and the numbers: only the square, the strips and the swatches.
proc Rcolor_picker_set_changed_callback*(picker: Rid, callback: RColorChangedCallback, user_data: pointer) {.rapi.}
proc Rcolor_rgb_to_hsv*(r: cfloat, g: cfloat, b: cfloat, hue: ptr cfloat, saturation: ptr cfloat, value: ptr cfloat) {.rapi.}
  ## Red, green, blue (0..1) to hue (0..360), saturation, value (0..1).
proc Rcolor_hsv_to_rgb*(hue: cfloat, saturation: cfloat, value: cfloat, r: ptr cfloat, g: ptr cfloat, b: ptr cfloat) {.rapi.}
proc Rtimeline_create*(): Rid {.rapi.}
proc Rtimeline_set_duration*(timeline: Rid, seconds: cdouble) {.rapi.}
  ## Length of the sequence. The view and the playhead stay within 0 .. duration.
proc Rtimeline_get_duration*(timeline: Rid): cdouble {.rapi.}
proc Rtimeline_set_time*(timeline: Rid, seconds: cdouble) {.rapi.}
  ## Moves the playhead. The time callback is not called.
proc Rtimeline_get_time*(timeline: Rid): cdouble {.rapi.}
proc Rtimeline_set_frame_rate*(timeline: Rid, frames_per_second: cdouble) {.rapi.}
  ## Frames per second. With a frame rate the ruler counts in
  ## frames ("2:12" is second 2, frame 12) and everything snaps to frames;
  ## 0 (default) counts in seconds and snaps to the marks of the ruler.
proc Rtimeline_get_frame_rate*(timeline: Rid): cdouble {.rapi.}
proc Rtimeline_set_snapping*(timeline: Rid, enabled: cint) {.rapi.}
  ## R_FALSE lets items be placed anywhere (holding Alt while dragging does it for one move).
proc Rtimeline_set_view*(timeline: Rid, start: cdouble, `end`: cdouble) {.rapi.}
  ## The part of the sequence that is shown. Zooming is showing less.
proc Rtimeline_get_view*(timeline: Rid, start: ptr cdouble, `end`: ptr cdouble) {.rapi.}
proc Rtimeline_set_playing*(timeline: Rid, playing: cint) {.rapi.}
  ## Makes the playhead run by itself, at the speed of the clock,
  ## calling the time callback at each frame; at the end it stops, or
  ## starts again with Rtimeline_set_loop.
proc Rtimeline_is_playing*(timeline: Rid): cint {.rapi.}
proc Rtimeline_set_loop*(timeline: Rid, loop: cint) {.rapi.}
proc Rtimeline_set_header_width*(timeline: Rid, width: cfloat) {.rapi.}
  ## Width of the column of the names of the tracks. 0 (default): fits the longest name.
proc Rtimeline_get_header_width*(timeline: Rid): cfloat {.rapi.}
  ## Returns: The width of the column of the names, as laid out now.
proc Rtimeline_set_ruler_height*(timeline: Rid, height: cfloat) {.rapi.}
  ## Height of the ruler of time (and of the markers' flags band). 0 (default): automatic.
proc Rtimeline_get_ruler_height*(timeline: Rid): cfloat {.rapi.}
proc Rtimeline_set_default_track_height*(timeline: Rid, height: cfloat) {.rapi.}
  ## Height of the tracks that have none of their own (Rtimeline_set_track_height). 0 (default): automatic.
proc Rtimeline_get_default_track_height*(timeline: Rid): cfloat {.rapi.}
proc Rtimeline_add_track*(timeline: Rid, name: cstring): cint {.rapi.}
  ## Returns: The track, or 0. Tracks are shown in the order they were added.
proc Rtimeline_remove_track*(timeline: Rid, track: cint) {.rapi.}
  ## Removes a track and everything on it.
proc Rtimeline_get_track_count*(timeline: Rid): cint {.rapi.}
proc Rtimeline_get_track*(timeline: Rid, index: cint): cint {.rapi.}
  ## Returns: The index-th track from the top, or 0.
proc Rtimeline_set_track_name*(timeline: Rid, track: cint, name: cstring) {.rapi.}
proc Rtimeline_get_track_name*(timeline: Rid, track: cint): cstring {.rapi.}
proc Rtimeline_set_track_color*(timeline: Rid, track: cint, r: cfloat, g: cfloat, b: cfloat) {.rapi.}
  ## Color of the clips and keys of the track that have none of their own.
proc Rtimeline_set_track_height*(timeline: Rid, track: cint, height: cfloat) {.rapi.}
  ## Height of the track in logical units; 0 = the default.
proc Rtimeline_set_track_locked*(timeline: Rid, track: cint, locked: cint) {.rapi.}
  ## A locked track shows its items but does not let the user change them.
proc Rtimeline_set_track_muted*(timeline: Rid, track: cint, muted: cint) {.rapi.}
  ## A muted track is drawn dimmed; what muting means is up to the application.
proc Rtimeline_set_track_style*(timeline: Rid, track: cint, property: cstring, value: cstring): cint {.rapi.}
  ## A property of the look of a track: "ClipStyle" (Solid,
  ## Gradient, Tint, Outline), "KeyShape" (Diamond, Circle, Square...),
  ## "KeySize", "ClipRadius", "Background" (its lane), or a part and a
  ## property ("Clip.Glow", "Key.BorderColor"). "" or "Default" gives back
  ## the one of the timeline.
  ## Returns: R_FALSE when the name or the value is not one.
proc Rtimeline_is_track_muted*(timeline: Rid, track: cint): cint {.rapi.}
proc Rtimeline_add_clip*(timeline: Rid, track: cint, start: cdouble, length: cdouble, label: cstring): cint {.rapi.}
  ## Returns: The clip, or 0 when the track does not exist.
proc Rtimeline_add_key*(timeline: Rid, track: cint, time: cdouble): cint {.rapi.}
  ## Returns: The key, or 0.
proc Rtimeline_add_marker*(timeline: Rid, time: cdouble, label: cstring): cint {.rapi.}
  ## A named moment of the whole sequence, shown on the ruler and across the tracks.
proc Rtimeline_remove_item*(timeline: Rid, item: cint) {.rapi.}
proc Rtimeline_clear*(timeline: Rid) {.rapi.}
  ## Removes every track, clip, key and marker.
proc Rtimeline_get_item_type*(timeline: Rid, item: cint): RTimelineItem {.rapi.}
proc Rtimeline_get_item_track*(timeline: Rid, item: cint): cint {.rapi.}
  ## Returns: The track of a clip or of a key; 0 for a marker.
proc Rtimeline_get_item_start*(timeline: Rid, item: cint): cdouble {.rapi.}
proc Rtimeline_get_item_length*(timeline: Rid, item: cint): cdouble {.rapi.}
  ## Returns: The length of a clip; 0 for a key or a marker.
proc Rtimeline_set_item*(timeline: Rid, item: cint, track: cint, start: cdouble, length: cdouble) {.rapi.}
  ## Moves an item: to another track (0 = keep), another start, another length (clips; negative = keep).
proc Rtimeline_set_item_label*(timeline: Rid, item: cint, label: cstring) {.rapi.}
proc Rtimeline_get_item_label*(timeline: Rid, item: cint): cstring {.rapi.}
proc Rtimeline_set_item_color*(timeline: Rid, item: cint, r: cfloat, g: cfloat, b: cfloat) {.rapi.}
proc Rtimeline_set_item_style*(timeline: Rid, item: cint, property: cstring, value: cstring): cint {.rapi.}
  ## A property of the look of one clip, key or marker: "Style"
  ## (Solid, Gradient, Tint, Outline: clips), "Shape" (keys), or any
  ## property of its part ("Background", "BorderColor", "Glow",
  ## "CornerRadius", "Foreground" for its label...).
proc Rtimeline_set_preset*(timeline: Rid, preset: RVisualPreset) {.rapi.}
  ## The look the timeline starts from (R_PRESET_AUTO by default; Preset="..." in XML).
proc Rtimeline_get_preset*(timeline: Rid): RVisualPreset {.rapi.}
proc Rtimeline_get_item_count*(timeline: Rid): cint {.rapi.}
proc Rtimeline_get_item*(timeline: Rid, index: cint): cint {.rapi.}
  ## Returns: The index-th item, in the order they were added, or 0.
proc Rtimeline_select*(timeline: Rid, item: cint, add: cint) {.rapi.}
  ## Selects an item, alone or (add) with the ones already selected.
proc Rtimeline_clear_selection*(timeline: Rid) {.rapi.}
proc Rtimeline_is_selected*(timeline: Rid, item: cint): cint {.rapi.}
proc Rtimeline_get_selection_count*(timeline: Rid): cint {.rapi.}
proc Rtimeline_get_selected*(timeline: Rid, index: cint): cint {.rapi.}
proc Rtimeline_set_time_callback*(timeline: Rid, callback: RTimelineTimeCallback, user_data: pointer) {.rapi.}
proc Rtimeline_set_changed_callback*(timeline: Rid, callback: RTimelineItemCallback, user_data: pointer) {.rapi.}
  ## Called for each item the user moved, stretched or retimed, as
  ## it happens. The action of the widget (OnChanged in XML) fires once
  ## per gesture, when the button is released.
proc Rtimeline_set_selection_callback*(timeline: Rid, callback: RTimelineItemCallback, user_data: pointer) {.rapi.}
  ## Called when the selection changed; `item` is the item just clicked, or 0.
proc Rtimeline_set_activated_callback*(timeline: Rid, callback: RTimelineItemCallback, user_data: pointer) {.rapi.}
  ## Called when an item is double-clicked.
proc Rtimeline_set_removed_callback*(timeline: Rid, callback: RTimelineItemCallback, user_data: pointer) {.rapi.}
  ## Called when the user deletes an item (the Delete key), before
  ## it is removed: its values can still be read. Without this callback the
  ## Delete key does nothing: the application decides what can be deleted.
proc Rnode_graph_create*(): Rid {.rapi.}
proc Rnode_graph_add_node*(graph: Rid, title: cstring, x: cfloat, y: cfloat): cint {.rapi.}
  ## x,y Top-left corner of the node, in the units of the canvas.
proc Rnode_graph_remove_node*(graph: Rid, node: cint) {.rapi.}
  ## Removes a node, its pins, their links, and the widget it holds.
proc Rnode_graph_clear*(graph: Rid) {.rapi.}
proc Rnode_graph_get_node_count*(graph: Rid): cint {.rapi.}
proc Rnode_graph_get_node*(graph: Rid, index: cint): cint {.rapi.}
proc Rnode_graph_set_node_title*(graph: Rid, node: cint, title: cstring) {.rapi.}
proc Rnode_graph_get_node_title*(graph: Rid, node: cint): cstring {.rapi.}
proc Rnode_graph_set_node_position*(graph: Rid, node: cint, x: cfloat, y: cfloat) {.rapi.}
proc Rnode_graph_get_node_position*(graph: Rid, node: cint, x: ptr cfloat, y: ptr cfloat) {.rapi.}
proc Rnode_graph_get_node_size*(graph: Rid, node: cint, width: ptr cfloat, height: ptr cfloat) {.rapi.}
  ## Size of a node on the canvas: Raven works it out from its title, its pins and its content.
proc Rnode_graph_set_node_color*(graph: Rid, node: cint, r: cfloat, g: cfloat, b: cfloat) {.rapi.}
  ## Color of the header of the node; without one it takes the accent color of the theme.
proc Rnode_graph_set_node_style*(graph: Rid, node: cint, property: cstring, value: cstring): cint {.rapi.}
  ## A property of the look of one node: "HeaderStyle" (Solid,
  ## Gradient, Stripe, Tint, None), "HeaderColor", "TitleColor",
  ## "Background", "BorderColor", "BorderWidth", "CornerRadius", "Glow",
  ## "Icon" (an image file shown before its title), or a part and a property
  ## ("Header.Gradient", "Node.ShadowSize"). "" or "Default" gives back the
  ## one of the graph.
  ## Returns: R_FALSE when the name or the value is not one.
proc Rnode_graph_set_node_content*(graph: Rid, node: cint, widget: Rid) {.rapi.}
  ## Puts a widget inside the node, under its pins: the graph owns
  ## it from now on. R_INVALID_ID removes (and deletes) the one it had.
proc Rnode_graph_get_node_content*(graph: Rid, node: cint): Rid {.rapi.}
proc Rnode_graph_set_node_name*(graph: Rid, node: cint, name: cstring) {.rapi.}
  ## Name of the node, the `Name` of its XML element: what
  ## <Link From="name.pin"> and Rnode_graph_find_node look for. Not shown.
proc Rnode_graph_get_node_name*(graph: Rid, node: cint): cstring {.rapi.}
proc Rnode_graph_find_node*(graph: Rid, name: cstring): cint {.rapi.}
  ## Returns: The node of that name, or 0.
proc Rnode_graph_set_node_user_data*(graph: Rid, node: cint, user_data: pointer) {.rapi.}
  ## Value the application attaches to a node.
proc Rnode_graph_get_node_user_data*(graph: Rid, node: cint): pointer {.rapi.}
proc Rnode_graph_add_input*(graph: Rid, node: cint, label: cstring, `type`: cstring): cint {.rapi.}
  ## type Free text, or NULL for a pin that links to any type. Returns: The pin, or 0.
proc Rnode_graph_add_output*(graph: Rid, node: cint, label: cstring, `type`: cstring): cint {.rapi.}
proc Rnode_graph_remove_pin*(graph: Rid, pin: cint) {.rapi.}
proc Rnode_graph_get_pin_node*(graph: Rid, pin: cint): cint {.rapi.}
proc Rnode_graph_is_pin_output*(graph: Rid, pin: cint): cint {.rapi.}
proc Rnode_graph_get_pin_label*(graph: Rid, pin: cint): cstring {.rapi.}
proc Rnode_graph_get_pin_type*(graph: Rid, pin: cint): cstring {.rapi.}
proc Rnode_graph_set_pin_name*(graph: Rid, pin: cint, name: cstring) {.rapi.}
  ## Name of the pin for Rnode_graph_find_pin and for the links of XML; a pin without one answers to its label.
proc Rnode_graph_find_pin*(graph: Rid, node: cint, name: cstring): cint {.rapi.}
  ## Returns: The pin of a node with that name (or label), or 0.
proc Rnode_graph_get_pin_position*(graph: Rid, pin: cint, x: ptr cfloat, y: ptr cfloat) {.rapi.}
  ## Where a pin is on the canvas (the middle of its dot).
proc Rnode_graph_get_input*(graph: Rid, node: cint, index: cint): cint {.rapi.}
  ## Returns: The index-th input (or output) of a node, or 0.
proc Rnode_graph_get_output*(graph: Rid, node: cint, index: cint): cint {.rapi.}
proc Rnode_graph_set_type_color*(graph: Rid, `type`: cstring, r: cfloat, g: cfloat, b: cfloat) {.rapi.}
  ## Color of the pins of a type and of the links between them.
proc Rnode_graph_set_type_style*(graph: Rid, `type`: cstring, property: cstring, value: cstring): cint {.rapi.}
  ## A property of the pins of a type and of their links: "Shape"
  ## (Circle, Ring, Square, Diamond, Triangle, Arrow), "Size", "Glow",
  ## "LinkShape" (Bezier, Straight, Step, Angled), "LinkWidth", "LinkGlow".
proc Rnode_graph_set_pin_style*(graph: Rid, pin: cint, property: cstring, value: cstring): cint {.rapi.}
  ## The same for one pin ("Shape", "Size", "Glow", "Color").
proc Rnode_graph_connect*(graph: Rid, output_pin: cint, input_pin: cint): cint {.rapi.}
  ## Links an output to an input, as the user does by dragging.
  ## The connect callback is not asked.
  ## Returns: The link; 0 when the pins cannot be linked (two inputs, the
  ## same node, different types).
proc Rnode_graph_disconnect*(graph: Rid, link: cint) {.rapi.}
proc Rnode_graph_get_link_count*(graph: Rid): cint {.rapi.}
proc Rnode_graph_get_link*(graph: Rid, index: cint): cint {.rapi.}
proc Rnode_graph_get_link_output*(graph: Rid, link: cint): cint {.rapi.}
proc Rnode_graph_get_link_input*(graph: Rid, link: cint): cint {.rapi.}
proc Rnode_graph_get_input_link*(graph: Rid, input_pin: cint): cint {.rapi.}
  ## Returns: The link that arrives at an input pin, or 0.
proc Rnode_graph_set_view*(graph: Rid, center_x: cfloat, center_y: cfloat, zoom: cfloat) {.rapi.}
  ## What the middle of the widget shows: a point of the canvas, and how large (1 = one unit per logical unit).
proc Rnode_graph_get_view*(graph: Rid, center_x: ptr cfloat, center_y: ptr cfloat, zoom: ptr cfloat) {.rapi.}
proc Rnode_graph_to_canvas*(graph: Rid, x: cfloat, y: cfloat, canvas_x: ptr cfloat, canvas_y: ptr cfloat) {.rapi.}
  ## Converts a point of the widget (from its corner, as in the events) to the canvas, and back, with the view as it is shown now.
proc Rnode_graph_from_canvas*(graph: Rid, canvas_x: cfloat, canvas_y: cfloat, x: ptr cfloat, y: ptr cfloat) {.rapi.}
proc Rnode_graph_frame_all*(graph: Rid) {.rapi.}
  ## Moves and zooms so that every node shows.
proc Rnode_graph_set_grid_visible*(graph: Rid, visible: cint) {.rapi.}
  ## R_FALSE hides the grid of dots (shown by default).
proc Rnode_graph_set_flow_visible*(graph: Rid, visible: cint) {.rapi.}
  ## The pulses of light that run along the links, from output to
  ## input. By default they run while the pointer is over the graph, and
  ## the window rests the rest of the time. R_TRUE makes them run all the
  ## time (the window then draws without pause); R_FALSE stops them.
proc Rnode_graph_set_read_only*(graph: Rid, read_only: cint) {.rapi.}
  ## R_TRUE: the user can look but not move, link or delete.
proc Rnode_graph_set_preset*(graph: Rid, preset: RVisualPreset) {.rapi.}
  ## The look the graph starts from (R_PRESET_AUTO by default; Preset="..." in XML).
proc Rnode_graph_get_preset*(graph: Rid): RVisualPreset {.rapi.}
proc Rnode_graph_select_node*(graph: Rid, node: cint, add: cint) {.rapi.}
proc Rnode_graph_clear_selection*(graph: Rid) {.rapi.}
proc Rnode_graph_is_node_selected*(graph: Rid, node: cint): cint {.rapi.}
proc Rnode_graph_get_selection_count*(graph: Rid): cint {.rapi.}
proc Rnode_graph_get_selected_node*(graph: Rid, index: cint): cint {.rapi.}
proc Rnode_graph_set_connect_callback*(graph: Rid, callback: RNodeGraphConnectCallback, user_data: pointer) {.rapi.}
proc Rnode_graph_set_linked_callback*(graph: Rid, callback: RNodeGraphItemCallback, user_data: pointer) {.rapi.}
  ## Called after the user made a link, and before a link the user removed goes (its pins can still be read).
proc Rnode_graph_set_unlinked_callback*(graph: Rid, callback: RNodeGraphItemCallback, user_data: pointer) {.rapi.}
proc Rnode_graph_set_moved_callback*(graph: Rid, callback: RNodeGraphItemCallback, user_data: pointer) {.rapi.}
  ## Called for each node the user moved, when the button is released.
proc Rnode_graph_set_selection_callback*(graph: Rid, callback: RNodeGraphItemCallback, user_data: pointer) {.rapi.}
  ## Called when the selection changed; `item` is the node just clicked, or 0.
proc Rnode_graph_set_removed_callback*(graph: Rid, callback: RNodeGraphItemCallback, user_data: pointer) {.rapi.}
  ## Called when the user deletes a node (the Delete key), before
  ## it is removed. Without this callback the Delete key only removes
  ## links: the application decides whether nodes can be deleted.
proc Rnode_graph_set_activated_callback*(graph: Rid, callback: RNodeGraphItemCallback, user_data: pointer) {.rapi.}
  ## Called when the user double-clicks a node (to open what it stands for).
proc Rnode_graph_set_context_callback*(graph: Rid, callback: RNodeGraphContextCallback, user_data: pointer) {.rapi.}
proc Rnode_graph_set_canvas_activated_callback*(graph: Rid, callback: RNodeGraphPointCallback, user_data: pointer) {.rapi.}
  ## Double click on the empty canvas (`item` 0): the application acts
  ## there (a search to add a node...). Without this callback it frames all nodes.
proc Rnode_graph_set_wire_dropped_callback*(graph: Rid, callback: RNodeGraphPointCallback, user_data: pointer) {.rapi.}
  ## A wire the user pulled from a pin was released on nothing: `item` is
  ## that pin, x, y where it was dropped (to offer a node to connect it to). The
  ## wire draws back as usual.
proc Rnode_graph_set_pin_clicked_callback*(graph: Rid, callback: RNodeGraphPointCallback, user_data: pointer) {.rapi.}
  ## A pin was clicked, not dragged (`item` is the pin, x, y on the canvas):
  ## the application may link in two clicks (Rnode_graph_set_pin_emphasis shows where
  ## the second may go). With this callback such a click neither detaches the wire
  ## of an input nor counts as a wire dropped on nothing.
proc Rnode_graph_get_node_at*(graph: Rid, x: cfloat, y: cfloat): cint {.rapi.}
  ## The node under a point of the widget (x, y as in the events: Rnode_graph_to_canvas), or 0.
proc Rnode_graph_get_pin_at*(graph: Rid, x: cfloat, y: cfloat): cint {.rapi.}
  ## The pin near a point of the widget (or whose tag is under it: Rnode_graph_set_pin_tag), or 0 (a menu for a pin).
proc Rnode_graph_get_link_at*(graph: Rid, x: cfloat, y: cfloat): cint {.rapi.}
  ## The link under a point of the widget, or 0 (hidden links are never under it).
proc Rnode_graph_set_pin_multiple*(graph: Rid, pin: cint, multiple: cint) {.rapi.}
  ## R_TRUE: this input keeps every link made to it (a state that several
  ## transitions enter) instead of one. Pulling a wire from it starts a new one.
proc Rnode_graph_set_node_active*(graph: Rid, node: cint, active: cint) {.rapi.}
  ## R_TRUE: the node is running (live debug) : a soft ring breathes around it.
proc Rnode_graph_set_node_active_color*(graph: Rid, node: cint, r: cfloat, g: cfloat, b: cfloat) {.rapi.}
  ## Color of that ring (the success color of the theme by default).
proc Rnode_graph_set_node_deletable*(graph: Rid, node: cint, deletable: cint) {.rapi.}
  ## R_FALSE: the Delete key does not remove this node (the root of a tree, the output of a graph).
proc Rnode_graph_set_node_shown*(graph: Rid, node: cint) {.rapi.}
  ## Shows a node at once, without the animation a node added to a shown graph pops in with (an application that rebuilds its graph).
proc Rnode_graph_set_link_shown*(graph: Rid, link: cint) {.rapi.}
  ## The same for a link made with Rnode_graph_connect: drawn whole at once instead of growing from its output.
proc Rnode_graph_set_node_icon*(graph: Rid, node: cint, texture: Rid) {.rapi.}
  ## An icon before the title: a texture of the application, shared by many nodes and not deleted with them (the "Icon" style loads one per node from a file). R_INVALID_ID removes it.
proc Rnode_graph_set_node_badge*(graph: Rid, node: cint, text: cstring) {.rapi.}
  ## A short text in a pill on the top right corner of the node ("1", "!"...); NULL or "" removes it.
proc Rnode_graph_set_pin_tag*(graph: Rid, pin: cint, text: cstring) {.rapi.}
  ## A short text in a chip of the pin's color outside the node, beside the pin
  ## (left of an input, right of an output): the name of what goes out or comes in
  ## there, as a net label of a schematic. NULL or "" removes it.
proc Rnode_graph_set_link_hidden*(graph: Rid, link: cint, hidden: cint) {.rapi.}
  ## R_TRUE: the link stays (its pins are linked, the application keeps
  ## it) but is not drawn and not picked: its two ends are shown otherwise (pin
  ## tags naming it, as the labels of a schematic stand for a wire).
proc Rnode_graph_set_pin_emphasis*(graph: Rid, pin: cint, emphasis: cint) {.rapi.}
  ## How much a pin stands out, as while a wire is pulled: 1 shows it as a pin
  ## to go to, -1 steps it back, 0 as usual (an application that links in two clicks).
proc Rnode_graph_get_tag_at*(graph: Rid, x: cfloat, y: cfloat): cint {.rapi.}
  ## The pin whose tag (Rnode_graph_set_pin_tag) is under a point of the widget, or 0: the chip only, not the pin.
proc Rnode_graph_set_pin_tag_lit*(graph: Rid, pin: cint, lit: cint) {.rapi.}
  ## R_TRUE: the pin's tag is drawn as under the pointer (a stronger fill, a
  ## thicker edge) — every tag of one net lit while the pointer is on one of them.
proc Rnode_graph_set_pin_label_width*(graph: Rid, width: cfloat) {.rapi.}
  ## Pin labels wider than `width` (logical units) are cut short with an
  ## ellipsis and the nodes are laid out with them so (compact ports); 0, the
  ## default, shows every label whole.
proc Rnode_graph_get_pin_label_width*(graph: Rid): cfloat {.rapi.}
proc Rnode_graph_set_node_badge_color*(graph: Rid, node: cint, r: cfloat, g: cfloat, b: cfloat) {.rapi.}
  ## Color of the badge pill of one node (Rnode_graph_set_node_badge); its text turns dark or light to suit it.
proc Rblock_editor_create*(): Rid {.rapi.}
proc Rblock_editor_add_kind*(editor: Rid, name: cstring, label: cstring, shape: RBlockShape, category: cstring): cint {.rapi.}
  ## A kind of block, and its block in the palette. Returns its index, -1 on an error.
proc Rblock_editor_set_category_color*(editor: Rid, category: cstring, r: cfloat, g: cfloat, b: cfloat) {.rapi.}
  ## Color of the blocks of a category (Raven picks one from its name otherwise).
proc Rblock_editor_set_palette_visible*(editor: Rid, visible: cint) {.rapi.}
proc Rblock_editor_add_block*(editor: Rid, kind: cstring, x: cfloat, y: cfloat): cint {.rapi.}
  ## A new block, the top of a script of its own at x, y (canvas units). Returns it, 0 on an error.
proc Rblock_editor_remove_block*(editor: Rid, `block`: cint) {.rapi.}
  ## Removes a block and everything that hangs under it.
proc Rblock_editor_clear*(editor: Rid) {.rapi.}
proc Rblock_editor_attach*(editor: Rid, `block`: cint, target: cint, place: RBlockPlace, index: cint): cint {.rapi.}
  ## Puts a block (with what is under it) at a place of another one. R_FALSE when it does not fit there.
proc Rblock_editor_detach*(editor: Rid, `block`: cint, x: cfloat, y: cfloat) {.rapi.}
  ## Takes a block (with what is under it) out: a script of its own at x, y.
proc Rblock_editor_get_script_count*(editor: Rid): cint {.rapi.}
proc Rblock_editor_get_script*(editor: Rid, index: cint): cint {.rapi.}
  ## The top block of a script.
proc Rblock_editor_get_kind*(editor: Rid, `block`: cint): cstring {.rapi.}
proc Rblock_editor_get_next*(editor: Rid, `block`: cint): cint {.rapi.}
  ## The block under it, 0 if none.
proc Rblock_editor_get_mouth_count*(editor: Rid, `block`: cint): cint {.rapi.}
proc Rblock_editor_get_body*(editor: Rid, `block`: cint, mouth: cint): cint {.rapi.}
  ## The first block in a mouth of a C block, 0 if empty.
proc Rblock_editor_get_slot_count*(editor: Rid, `block`: cint): cint {.rapi.}
proc Rblock_editor_get_input*(editor: Rid, `block`: cint, slot: cint): cint {.rapi.}
  ## The value block in a slot, 0 if the slot holds a typed value.
proc Rblock_editor_get_value*(editor: Rid, `block`: cint, slot: cint): cstring {.rapi.}
  ## The typed value of a slot.
proc Rblock_editor_set_value*(editor: Rid, `block`: cint, slot: cint, value: cstring) {.rapi.}
proc Rblock_editor_get_parent*(editor: Rid, `block`: cint, place: ptr RBlockPlace, index: ptr cint): cint {.rapi.}
  ## Where a block hangs: the block it hangs from (0: the top of a script).
proc Rblock_editor_get_position*(editor: Rid, `block`: cint, x: ptr cfloat, y: ptr cfloat) {.rapi.}
proc Rblock_editor_set_block_glow*(editor: Rid, `block`: cint, glow: cint) {.rapi.}
  ## R_TRUE: the block is running: a halo breathes around it.
proc Rblock_editor_save*(editor: Rid): cstring {.rapi.}
  ## Every script as text (valid until the next call).
proc Rblock_editor_load*(editor: Rid, text: cstring): cint {.rapi.}
  ## Replaces the scripts with those of a text from Rblock_editor_save.
proc Rblock_editor_set_changed_callback*(editor: Rid, callback: RBlockEditorCallback, user_data: pointer) {.rapi.}
proc Rblock_editor_set_clicked_callback*(editor: Rid, callback: RBlockEditorCallback, user_data: pointer) {.rapi.}
proc Rblock_editor_set_context_callback*(editor: Rid, callback: RBlockEditorContextCallback, user_data: pointer) {.rapi.}
proc Rblock_editor_set_slot_options*(editor: Rid, kind: cstring, slot: cint, options: cstring) {.rapi.}
  ## The choices of a %m slot of a kind, separated by '\n' ("" : the slot is typed in).
proc Rblock_editor_set_kind_visible*(editor: Rid, kind: cstring, visible: cint) {.rapi.}
  ## R_FALSE: the kind is not in the palette (its blocks stay where they are).
proc Rblock_editor_set_kind_label*(editor: Rid, kind: cstring, label: cstring): cint {.rapi.}
  ## A new label for a kind (a function that takes one more input...).
  ## The blocks of that kind keep the values of the slots that are still there.
proc Rblock_editor_set_block_deletable*(editor: Rid, `block`: cint, deletable: cint) {.rapi.}
  ## R_FALSE: the user cannot delete the block (the entry of a function). Not saved.
proc Rblock_editor_get_selection_count*(editor: Rid): cint {.rapi.}
proc Rblock_editor_get_selected*(editor: Rid, index: cint): cint {.rapi.}
proc Rblock_editor_select*(editor: Rid, `block`: cint, add: cint) {.rapi.}
  ## Selects a block (add: R_FALSE clears the selection first ; block 0 : only clears).
proc Rblock_editor_get_view*(editor: Rid, x: ptr cfloat, y: ptr cfloat, zoom: ptr cfloat) {.rapi.}
  ## Pan (canvas units at the top left) and zoom.
proc Rblock_editor_set_view*(editor: Rid, x: cfloat, y: cfloat, zoom: cfloat) {.rapi.}
proc Rdialog_open*(window: Rid, content: Rid, closed: RDialogClosedCallback, user_data: pointer) {.rapi.}
  ## Shows a widget as a dialog of a window. The window takes
  ## ownership of `content`: it is deleted when the dialog closes, so read
  ## what you need from it before, or in `closed`.
  ## The Escape key dismisses the dialog (result 0).
proc Rdialog_close*(widget: Rid, result_value: cint) {.rapi.}
  ## Closes the dialog that contains a widget (the content itself or
  ## anything inside it: the button that was clicked).
  ## result Passed to the closed callback and returned by Rdialog_run.
proc Rdialog_run*(window: Rid, content: Rid): cint {.rapi.}
  ## Shows a dialog and waits until it closes. Returns its result.
proc Rpopup_open*(window: Rid, content: Rid, x: cfloat, y: cfloat, closed: RDialogClosedCallback, user_data: pointer) {.rapi.}
  ## Shows a widget as a popup over the window, its top-left corner at
  ## x, y (window coordinates; moved to stay inside the window): a search box,
  ## a palette, a picker. Unlike a dialog it does not block the window: a click
  ## outside it, or Escape, closes it. The application keeps the content (it is
  ## not deleted on close) and can open it again. `closed` (may be NULL) is
  ## called when it closes, with the result given to Rpopup_close (0 when dismissed).
  ## The content draws its own background (a Border with a Background...).
proc Rpopup_close*(widget: Rid, result_value: cint) {.rapi.}
  ## Closes the popup that contains a widget (the content or anything inside it).
proc Rpopup_is_open*(content: Rid): cint {.rapi.}
  ## Returns: R_TRUE while the widget is shown as a popup.
proc Rwindow_has_dialog*(window: Rid): cint {.rapi.}
  ## Returns: R_TRUE while a dialog is shown in the window.
proc Rmessage_box*(window: Rid, title: cstring, text: cstring, buttons: RMessageButtons): RMessageResult {.rapi.}
  ## Shows a message with buttons and waits for the answer.
  ## window R_INVALID_ID: the first window of the application.
  ## title  May be NULL.
proc Rfile_dialog_open*(desc: ptr RFileDialogDesc): cstring {.rapi.}
  ## Asks for a file to open.
  ## desc May be NULL.
  ## Returns: The path (UTF-8), valid until the next file dialog, or NULL when the user cancelled.
proc Rfile_dialog_save*(desc: ptr RFileDialogDesc): cstring {.rapi.}
  ## Asks where to save a file. An existing file was confirmed by the user.
proc Rfile_dialog_select_folder*(desc: ptr RFileDialogDesc): cstring {.rapi.}
  ## Asks for a folder.
proc Rfile_dialog_get_result_count*(): cint {.rapi.}
  ## What the last file dialog returned: several paths with R_FILE_DIALOG_MULTIPLE.
proc Rfile_dialog_get_result*(index: cint): cstring {.rapi.}
proc Rfile_register_memory*(path: cstring, data: pointer, size: csize_t, copy: cint): cint {.rapi.}
  ## Registers a file held in memory under a path: every file Raven
  ## reads (XML documents and their Include, Source and Font attributes,
  ## themes, textures, fonts, Rfile_read) is looked up here first, so that
  ## resources embedded in the executable need no file beside it. A relative
  ## path names the file as it is written in the XML once joined to the
  ## folder of the document that names it (Rxml_set_base_path for a
  ## document given as text), e.g. "ui/editor.xml". Paths compare with '/'
  ## and '\\' alike. Memory files are not watched for live reload. Call it
  ## after Rinit; Rshutdown forgets every file.
  ## copy R_TRUE: Raven keeps a copy; R_FALSE: `data` must outlive Raven (static data).
proc Rfile_unregister_memory*(path: cstring) {.rapi.}
proc Rfile_is_memory*(path: cstring): cint {.rapi.}
  ## R_TRUE when `path` is a file registered with Rfile_register_memory.
proc Rfile_read*(path: cstring, size: ptr csize_t): cstring {.rapi.}
  ## Whole files, with UTF-8 paths on every platform.
  ## Returns: Rfile_read: the content followed by a '\0', to release with
  ## Rmemory_free; NULL when the file cannot be read. `size` may be NULL.
proc Rfile_write*(path: cstring, data: pointer, size: csize_t): cint {.rapi.}
proc Rfile_browser_create*(): Rid {.rapi.}
proc Rfile_browser_set_mode*(browser: Rid, mode: RFileBrowserMode) {.rapi.}
proc Rfile_browser_set_directory*(browser: Rid, path: cstring): cint {.rapi.}
  ## Shows a folder. "~" is the home of the user. Returns R_FALSE when it cannot be read.
proc Rfile_browser_get_directory*(browser: Rid): cstring {.rapi.}
proc Rfile_browser_refresh*(browser: Rid) {.rapi.}
  ## Reads the folder again.
proc Rfile_browser_set_filters*(browser: Rid, filters: ptr RFileFilter, count: cint) {.rapi.}
  ## Kinds of files the user chooses from; the first one is active. The strings are copied.
proc Rfile_browser_set_show_hidden*(browser: Rid, show: cint) {.rapi.}
proc Rfile_browser_set_multiple*(browser: Rid, multiple: cint) {.rapi.}
  ## R_TRUE lets several files be selected (Ctrl and Shift with a click).
proc Rfile_browser_set_controls_visible*(browser: Rid, visible: cint) {.rapi.}
  ## R_FALSE hides the name field, the filter list and the buttons:
  ## what remains is an explorer to put in a side panel. Shown by default.
proc Rfile_browser_set_places_visible*(browser: Rid, visible: cint) {.rapi.}
  ## R_FALSE hides the list of places on the left.
proc Rfile_browser_set_file_name*(browser: Rid, name: cstring) {.rapi.}
  ## Name typed in the name field (save mode).
proc Rfile_browser_set_default_extension*(browser: Rid, extension: cstring) {.rapi.}
  ## Added to a name typed without extension, in save mode.
proc Rfile_browser_get_selection_count*(browser: Rid): cint {.rapi.}
  ## Paths of the selected entries. The strings are valid until the selection or the folder changes.
proc Rfile_browser_get_selection*(browser: Rid, index: cint): cstring {.rapi.}
proc Rfile_browser_select*(browser: Rid, name: cstring) {.rapi.}
  ## Selects an entry of the folder by its name.
proc Rfile_browser_set_accepted_callback*(browser: Rid, callback: RFileBrowserCallback, user_data: pointer) {.rapi.}
proc Rfile_browser_set_cancelled_callback*(browser: Rid, callback: RFileBrowserCallback, user_data: pointer) {.rapi.}
  ## Called by the Cancel button.
proc Rfile_browser_set_selection_changed_callback*(browser: Rid, callback: RFileBrowserCallback, user_data: pointer) {.rapi.}
  ## Called when the selection changes; `path` is the entry under the cursor, or NULL.
proc Rnotification_show*(title: cstring, body: cstring, urgency: RNotificationUrgency): cint {.rapi.}
  ## Returns: R_TRUE when the system took the notification.
proc Rnotification_is_supported*(): cint {.rapi.}
  ## R_FALSE when the platform has no notifications (Rnotification_show then always uses the fallback).
proc Rnotification_set_fallback*(callback: RNotificationFallback, user_data: pointer) {.rapi.}
  ## Called with the notifications the system could not show; NULL: none.
proc Rnotification_set_clicked_callback*(callback: RNotificationClickedCallback, user_data: pointer) {.rapi.}
  ## Called when the user clicks a notification of Rnotification_show
  ## (on the UI thread; the application may then bring its window to the
  ## front: the system lets it). It is the last one shown that was clicked:
  ## the application remembers what it was about. NULL: none.
  ## Windows only for now (elsewhere it is never called).
proc Rviewport_set_texture*(viewport: Rid, texture: Rid) {.rapi.}
  ## Shows a texture. R_INVALID_ID shows nothing.
proc Rviewport_set_stretch*(viewport: Rid, stretch: RStretch) {.rapi.}
  ## How the texture is fitted in the widget. Default: R_STRETCH_FILL.
proc Rviewport_set_flip_y*(viewport: Rid, flip: cint) {.rapi.}
  ## R_TRUE flips the texture vertically (OpenGL framebuffers are bottom-up).
proc Rviewport_set_draw_callback*(viewport: Rid, callback: RViewportDrawCallback, user_data: pointer) {.rapi.}
proc Rviewport_set_native*(viewport: Rid, enabled: cint): cint {.rapi.}
  ## Creates (or destroys) the native child window of the viewport.
  ## The viewport must already be in a window.
  ## Returns: R_TRUE on success.
proc Rviewport_get_native_handle*(viewport: Rid, native: ptr RNativeWindow): cint {.rapi.}
  ## Fills `native` with the handles of the child window.
proc Rviewport_get_pixel_size*(viewport: Rid, width: ptr cint, height: ptr cint) {.rapi.}
  ## Size of the viewport in pixels after the last layout.
proc Rviewport_set_resize_callback*(viewport: Rid, callback: RViewportResizeCallback, user_data: pointer) {.rapi.}
proc Rviewport_set_realtime*(viewport: Rid, realtime: cint) {.rapi.}
  ## R_TRUE redraws the window every frame while the viewport is
  ## visible (game view). R_FALSE redraws only on Rwidget_invalidate (default).
proc Rcustom_set_paint_callback*(widget: Rid, callback: RPaintCallback, user_data: pointer) {.rapi.}
proc Rcustom_set_measure_callback*(widget: Rid, callback: RMeasureCallback, user_data: pointer) {.rapi.}
  ## Reports the desired size of the widget. Without it the widget asks for 0 x 0.
proc Rdraw_rect*(painter: ptr RPainter, x: cfloat, y: cfloat, width: cfloat, height: cfloat, radius: cfloat, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
  ## Filled rectangle with rounded corners (radius 0 = sharp).
proc Rdraw_rect_outline*(painter: ptr RPainter, x: cfloat, y: cfloat, width: cfloat, height: cfloat, radius: cfloat, thickness: cfloat, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
  ## Outline of a rounded rectangle, drawn inside its bounds.
proc Rdraw_circle*(painter: ptr RPainter, center_x: cfloat, center_y: cfloat, radius: cfloat, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
proc Rdraw_line*(painter: ptr RPainter, x0: cfloat, y0: cfloat, x1: cfloat, y1: cfloat, thickness: cfloat, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
  ## Anti-aliased line with round caps.
proc Rdraw_curve*(painter: ptr RPainter, x0: cfloat, y0: cfloat, cx0: cfloat, cy0: cfloat, cx1: cfloat, cy1: cfloat, x1: cfloat, y1: cfloat, thickness: cfloat, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
  ## Anti-aliased cubic Bezier curve from (x0, y0) to (x1, y1), pulled by two control points.
proc Rdraw_triangle*(painter: ptr RPainter, x0: cfloat, y0: cfloat, x1: cfloat, y1: cfloat, x2: cfloat, y2: cfloat, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
proc Rdraw_image*(painter: ptr RPainter, texture: Rid, x: cfloat, y: cfloat, width: cfloat, height: cfloat, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
  ## Texture stretched to a rectangle and multiplied by a tint color.
proc Rdraw_text*(painter: ptr RPainter, font: Rid, size: cfloat, x: cfloat, y: cfloat, text: cstring, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
  ## Draws UTF-8 text; (x, y) is the top-left corner of the first line.
  ## font R_INVALID_ID uses the theme font.
  ## size 0 uses R_METRIC_FONT_SIZE.
proc Rdraw_glass*(painter: ptr RPainter, x: cfloat, y: cfloat, width: cfloat, height: cfloat, radius: cfloat, r: cfloat, g: cfloat, b: cfloat, a: cfloat) {.rapi.}
  ## Rounded rectangle of glass. It shows what was drawn before it,
  ## frosted and bent along its rim, under a tint: (r, g, b) is the color of
  ## the tint and `a` its strength (0 = clear glass, 1 = opaque color).
  ## Works with any theme. Needs a backend that implements glass (the Vulkan
  ## backend does).
proc Rdraw_spring*(painter: ptr RPainter, spring: ptr RSpring, target: cfloat, motion: RMotion): cfloat {.rapi.}
  ## Animates a value from the paint callback of a custom widget.
  ## Moves the spring toward `target` by the time elapsed since the last
  ## frame and returns its value. While it moves, the window keeps drawing
  ## frames by itself. The first call puts the spring on its target.
  ##
  ##     static RSpring size;                                  (zeroed)
  ##     float s = Rdraw_spring(painter, &size, hovered ? 1.0f : 0.0f, R_MOTION_BOUNCY);
  ##     Rdraw_circle(painter, cx, cy, 20.0f + 8.0f * s, ...);
proc Rspring_kick*(spring: ptr RSpring, velocity: cfloat) {.rapi.}
  ## Pushes a spring away from its target (units per second): it bounces back.
proc Rspring_set*(spring: ptr RSpring, value: cfloat) {.rapi.}
  ## Puts a spring at rest on a value, without animation.
proc Rdraw_push_clip_rect*(painter: ptr RPainter, x: cfloat, y: cfloat, width: cfloat, height: cfloat) {.rapi.}
  ## Restricts the following draws to a rectangle, intersected with the current one.
proc Rdraw_pop_clip_rect*(painter: ptr RPainter) {.rapi.}
proc Rshader_create*(spirv: pointer, size: csize_t, flags: uint32): Rid {.rapi.}
  ## A shader from SPIR-V code (a fragment shader written with
  ## raven_effect.glsl, as in a .spv file): `size` bytes, copied. R_INVALID_ID
  ## when the code is not SPIR-V; a backend that refuses it reports an error
  ## and the effect is drawn as a rectangle.
proc Rshader_create_from_file*(path: cstring, flags: uint32): Rid {.rapi.}
  ## The same, from a .spv file (UTF-8 path, relative to the current folder).
proc Rshader_destroy*(shader: Rid) {.rapi.}
proc Rshader_is_valid*(shader: Rid): cint {.rapi.}
proc Rshader_get_flags*(shader: Rid): uint32 {.rapi.}
proc Reffect_get*(effect: REffect): Rid {.rapi.}
  ## A built-in effect, created the first time it is asked for.
proc Reffect_find*(name: cstring): Rid {.rapi.}
  ## A built-in effect by its name ("aurora", "shimmer", "neon", "ripple", "plasma", "grain", "spotlight", "mesh"), or R_INVALID_ID.
proc Reffect_get_name*(effect: REffect): cstring {.rapi.}
proc Reffect_get_defaults*(effect: REffect, look: ptr RShaderLook) {.rapi.}
  ## Its colors and parameters when nothing else is given (Rshader_look_init uses them).
proc Rshader_look_init*(look: ptr RShaderLook) {.rapi.}
  ## White to white, no parameter, no texture, square corners, animated when the shader is.
proc Rshader_look_defaults*(shader: Rid, look: ptr RShaderLook) {.rapi.}
  ## Fills a look with the defaults of the shader when it is a built-in effect (else Rshader_look_init).
proc Rdraw_shader*(painter: ptr RPainter, shader: Rid, x: cfloat, y: cfloat, width: cfloat, height: cfloat, look: ptr RShaderLook) {.rapi.}
  ## From a paint callback: draws the effect over a rectangle of the
  ## widget (logical units, from its top left corner).
proc Rborder_set_effect*(border: Rid, shader: Rid, look: ptr RShaderLook) {.rapi.}
  ## The background of a border becomes an effect (its Background,
  ## if any, is drawn under it; its child over it). R_INVALID_ID removes it.
  ## XML: Effect="aurora" or Effect="file.spv", EffectColor, EffectColor2,
  ## EffectParams="a,b,c,d", EffectAnimate, EffectBackdrop (for a file).
proc Rborder_get_effect*(border: Rid): Rid {.rapi.}
proc Rwindow_create_from_xml*(xml: cstring, size: csize_t): Rid {.rapi.}
  ## Creates a window from a document whose root element is <Window>.
  ## xml  UTF-8 text. It does not need to be null-terminated.
  ## size Size of the text in bytes.
  ## Returns: The window, or R_INVALID_ID. Parse errors are reported with
  ##         their line and column through the error callback (R_ERROR_XML).
proc Rwindow_create_from_xml_file*(path: cstring): Rid {.rapi.}
proc Rwidget_create_from_xml*(xml: cstring, size: csize_t): Rid {.rapi.}
  ## Creates a widget tree from a document whose root element is a
  ## widget. The caller owns the returned widget until it is added to a parent.
proc Rwidget_create_from_xml_file*(path: cstring): Rid {.rapi.}
proc Rxml_set_base_path*(path: cstring) {.rapi.}
  ## A folder looked into after the one of the document (see above),
  ## and the first one for the documents given from memory. NULL or "": none.
proc Rxml_set_auto_load*(enabled: cint) {.rapi.}
  ## Turns auto-loading on or off (see above). It is on by default;
  ## the environment variable RAVEN_AUTO_LOAD=0 turns it off without
  ## rebuilding. Turned off, the register functions are the only way in;
  ## the places where paths are looked for stay the same.
proc Rxml_get_auto_load*(): cint {.rapi.}
proc Rxml_set_live_reload*(enabled: cint) {.rapi.}
  ## Turns live reload on or off. It is on by default; the
  ## environment variable RAVEN_LIVE_RELOAD=0 turns it off without
  ## rebuilding. When off, files are never looked at again.
proc Rxml_get_live_reload*(): cint {.rapi.}
proc Rxml_set_reload_callback*(callback: RXmlReloadCallback, user_data: pointer) {.rapi.}
proc Rxml_check_files*(): cint {.rapi.}
  ## Looks at the files now, without waiting for the next look
  ## (there are five per second), even when live reload is off.
  ## Returns: The number of files found changed.
proc Rxml_register_widgets*(xml: cstring, size: csize_t): cint {.rapi.}
  ## Reads the <Widget Name="..."> declarations of a document (its
  ## root, or the children of a <Widgets> root) so that the tags they
  ## declare can be used in the documents loaded afterwards.
  ## Returns: The number of tags declared.
proc Rxml_register_widgets_from_file*(path: cstring): cint {.rapi.}
proc Rxml_unregister_widget*(name: cstring) {.rapi.}
proc Rxml_is_widget_registered*(name: cstring): cint {.rapi.}
proc Rxml_get_registered_widget_count*(): cint {.rapi.}
  ## The declared tags, for tools that list them.
proc Rxml_get_registered_widget_name*(index: cint): cstring {.rapi.}
proc Rxml_register_tag*(tag: cstring, create: RXmlTagCreateCallback, set_attribute: RXmlTagAttributeCallback, user_data: pointer): cint {.rapi.}
  ## Registers a tag (or replaces what it was registered with).
  ## Returns: R_FALSE when the name is the one of a widget of Raven, or when too many tags are registered (64).
proc Rxml_unregister_tag*(tag: cstring) {.rapi.}
proc Rxml_is_tag_registered*(tag: cstring): cint {.rapi.}
proc Rxml_resolve_path*(path: cstring): cstring {.rapi.}
  ## The path of a file named in a document: as the attributes
  ## Source of Raven read it, looked for where files are looked for (see
  ## Rxml_set_base_path): the folder of the document being loaded (while it
  ## loads) and its subfolders, the base path, the current folder, PATH.
  ## When no place holds it, the path it would have next to the document
  ## (else in the base path). For the attributes of the tags made by code
  ## that name files.
  ## Returns: The path; valid until the next call.
proc Rwidget_set_attribute*(widget: Rid, name: cstring, value: cstring): cint {.rapi.}
  ## Sets a property from text, with the names and value syntax of the
  ## XML attributes. Handy for scripting and data-driven tools.
  ## Returns: R_TRUE when the attribute exists for this widget and the value is valid.
proc Rxml_document_create*(root_tag: cstring): Rid {.rapi.}
  ## A new document with one element.
proc Rxml_document_parse*(xml: cstring, size: csize_t, error: ptr RXmlError): Rid {.rapi.}
  ## Reads a text into a document. Comments are kept.
  ## error When not NULL, a mistake in the text is described there
  ## and nothing is reported; when NULL it goes to the error callback.
  ## Returns: The document, or R_INVALID_ID.
proc Rxml_document_load*(path: cstring): Rid {.rapi.}
  ## Reads a file. The paths written in it are relative to its folder.
proc Rxml_document_destroy*(document: Rid) {.rapi.}
  ## Releases a document and its nodes. What was built from it lives on.
proc Rxml_document_is_valid*(document: Rid): cint {.rapi.}
proc Rxml_document_get_root*(document: Rid): Rid {.rapi.}
proc Rxml_document_to_string*(document: Rid): cstring {.rapi.}
  ## Writes the document as text, one element per line, indented
  ## with tabs. Rxml_node_get_line then says where each node was written.
  ## Returns: The text, valid until the next call for this document or its destruction.
proc Rxml_document_save*(document: Rid, path: cstring): cint {.rapi.}
proc Rxml_document_create_widget*(document: Rid): Rid {.rapi.}
  ## Builds the widgets the document describes, as
  ## Rwidget_create_from_xml does. When the root is a <Window>, builds what
  ## the window would show (R_INVALID_ID when it is empty).
proc Rxml_document_create_window*(document: Rid): Rid {.rapi.}
  ## Builds the window the document describes, as Rwindow_create_from_xml does.
proc Rxml_node_is_valid*(node: Rid): cint {.rapi.}
proc Rxml_node_get_document*(node: Rid): Rid {.rapi.}
proc Rxml_node_get_tag*(node: Rid): cstring {.rapi.}
  ## Returns: The tag of an element, "" for a comment.
proc Rxml_node_set_tag*(node: Rid, tag: cstring) {.rapi.}
proc Rxml_node_is_comment*(node: Rid): cint {.rapi.}
proc Rxml_node_get_text*(node: Rid): cstring {.rapi.}
  ## The text inside an element (<Label>this</Label>), or the text of a comment.
proc Rxml_node_set_text*(node: Rid, text: cstring) {.rapi.}
proc Rxml_node_get_parent*(node: Rid): Rid {.rapi.}
proc Rxml_node_get_child_count*(node: Rid): cint {.rapi.}
proc Rxml_node_get_child*(node: Rid, index: cint): Rid {.rapi.}
proc Rxml_node_get_index*(node: Rid): cint {.rapi.}
  ## Returns: The place of a node among the children of its parent; 0 for the root.
proc Rxml_node_add_child*(parent: Rid, tag: cstring, index: cint): Rid {.rapi.}
  ## index Where among the children; -1 for the end.
  ## Returns: The new node; R_INVALID_ID (R_ERROR_XML) when it would be nested
  ##         deeper than a document can be: 256 levels under the root, as
  ##         when a text is read. The same holds for the functions below.
proc Rxml_node_add_comment*(parent: Rid, text: cstring, index: cint): Rid {.rapi.}
proc Rxml_node_add_xml*(parent: Rid, xml: cstring, size: csize_t, index: cint): Rid {.rapi.}
  ## Reads a piece of XML (one element and what it contains) and
  ## adds it: what a palette, a paste or a template inserts.
proc Rxml_node_remove*(node: Rid) {.rapi.}
  ## Removes a node and what it contains. The root stays.
proc Rxml_node_move*(node: Rid, new_parent: Rid, index: cint): cint {.rapi.}
  ## Moves a node, with what it contains, under another parent of
  ## the same document or elsewhere under its own.
  ## index Where among the children of the new parent, counted once the node has left its place; -1 for the end.
  ## Returns: R_FALSE when it cannot be done (the root, a parent that is inside the node, or a place too deep).
proc Rxml_node_duplicate*(node: Rid): Rid {.rapi.}
  ## Copies a node and what it contains, right after it. Returns the copy.
proc Rxml_node_to_string*(node: Rid): cstring {.rapi.}
  ## The node as text. Valid until the next call for this document.
proc Rxml_node_get_attribute_count*(node: Rid): cint {.rapi.}
proc Rxml_node_get_attribute_name*(node: Rid, index: cint): cstring {.rapi.}
proc Rxml_node_get_attribute_value*(node: Rid, index: cint): cstring {.rapi.}
proc Rxml_node_get_attribute*(node: Rid, name: cstring): cstring {.rapi.}
  ## Returns: The value of an attribute (names are compared without case), or NULL when the node does not have it.
proc Rxml_node_set_attribute*(node: Rid, name: cstring, value: cstring) {.rapi.}
  ## Sets an attribute, added after the others when it is new. A NULL value removes it.
proc Rxml_node_get_line*(node: Rid): cint {.rapi.}
  ## Returns: The line of the node, starting at 1, in the text the document
  ## was read from or last written to; 0 for a node added since.
proc Rxml_node_get_widget*(node: Rid): Rid {.rapi.}
  ## Returns: The widget (the window, for a <Window>) the node became the
  ## last time the document was built, or R_INVALID_ID: never built,
  ## destroyed since, or not a widget (an <Item>, a comment). A tag declared
  ## with <Widget> gives the widget it stands for.
proc Rwidget_get_xml_node*(widget: Rid): Rid {.rapi.}
  ## Returns: The node a widget was built from, or R_INVALID_ID: not built
  ## from a document, or the document is gone. The widgets inside a tag
  ## declared with <Widget> all come from the node where the tag is used.
proc Rxml_get_tag_count*(): cint {.rapi.}
  ## The tags of the library, <Window> included. Those declared with <Widget> are listed by Rxml_get_registered_widget_name.
proc Rxml_get_tag_name*(index: cint): cstring {.rapi.}
proc Rxml_get_tag_description*(tag: cstring): cstring {.rapi.}
  ## Returns: One line about a tag, "" when there is nothing to say.
proc Rxml_get_tag_children*(tag: cstring): cint {.rapi.}
  ## Returns: 0: no child widget; 1: a single one; 2: any number. Declared and unknown tags give 2.
proc Rxml_get_attribute_count*(tag: cstring): cint {.rapi.}
  ## The attributes of a tag: its own first, then those every widget
  ## has, then those of its place in its parent. For a tag declared with
  ## <Widget>, its own are its parameters.
proc Rxml_get_attribute_info*(tag: cstring, index: cint, info: ptr RXmlAttributeInfo): cint {.rapi.}
proc Rlocale_load_csv*(path: cstring): cint {.rapi.}
  ## Loads a CSV table of translations from a file. Returns the number of keys read, -1 on an error.
proc Rlocale_load_csv_from_memory*(csv: cstring, size: csize_t): cint {.rapi.}
  ## Same, from text in memory (UTF-8).
proc Rlocale_clear*() {.rapi.}
  ## Forgets every table.
proc Rlocale_set_text*(language: cstring, key: cstring, text: cstring) {.rapi.}
  ## Adds or replaces one text of one language.
proc Rlocale_get_language_count*(): cint {.rapi.}
  ## Number of languages in the tables, and their codes, in the order of the columns.
proc Rlocale_get_language_at*(index: cint): cstring {.rapi.}
proc Rlocale_set_language*(code: cstring): cint {.rapi.}
  ## Chooses the language: an exact code ("pt-BR"), or its first part
  ## ("pt") when the tables have no such column. Every widget that shows a key
  ## is updated, and the layout of the windows follows its direction (an
  ## Arabic, Hebrew, Persian or Urdu language is laid out from right to left,
  ## see Rset_layout_direction). Returns R_FALSE when no column matches (the
  ## language is set all the same: the texts fall back).
proc Rlocale_get_language*(): cstring {.rapi.}
proc Rlocale_get_system_language*(): cstring {.rapi.}
  ## The language of the user, from the system ("fr-FR", "en-US"...).
proc Rlocale_set_fallback_language*(code: cstring) {.rapi.}
  ## The language whose texts are shown when the current one has none (default: the first column).
proc Rlocale_is_rtl*(): cint {.rapi.}
  ## R_TRUE when the language is written from right to left.
proc Rlocale_set_changed_callback*(callback: RLocaleChangedCallback, user_data: pointer) {.rapi.}
  ## Called after the language or the tables changed (the widgets are already updated).
proc Rtr*(key: cstring): cstring {.rapi.}
  ## The text of a key in the current language (see the fallbacks above). Never NULL.
proc Rtr_exists*(key: cstring): cint {.rapi.}
  ## R_TRUE when the key has a text in the current language or the fallback one.
proc Rtr_format*(key: cstring, args: cstringArray, arg_count: cint): cstring {.rapi.}
  ## The text of a key with its {0} {1} ... replaced by the arguments
  ## (translators may put them in any order). "{{" is a brace.
proc Rtr_plural*(key: cstring, count: cint): cstring {.rapi.}
  ## The plural form of a key for a count: the key followed by the CLDR
  ## category of the count in the current language (".zero", ".one", ".two",
  ## ".few", ".many", ".other"), else ".other", else the key itself. {n} is
  ## replaced by the count, written in the conventions of the language.
proc Rwidget_set_text_key*(widget: Rid, attribute: cstring, key: cstring) {.rapi.}
  ## Makes a widget show a key: an attribute ("Text", "Tooltip",
  ## "Title", "Placeholder"... any attribute that takes a text) is set to the
  ## translation now and every time the language changes. NULL or "" stops it.
  ## Same as Rwidget_set_attribute(widget, attribute, "@key").
proc Rlocale_format_number*(value: cdouble, decimals: cint): cstring {.rapi.}
  ## A number written in the conventions of the language: decimal separator and digit grouping ("1 234,5" in French).
proc Raccessibility_set_text_scale*(scale: cfloat) {.rapi.}
  ## Size of every text, 1 = as designed (0.5 to 4). Controls grow with their text.
proc Raccessibility_get_text_scale*(): cfloat {.rapi.}
proc Raccessibility_set_high_contrast*(enabled: cint) {.rapi.}
  ## Themes of high contrast: black and white, yellow focus, thick
  ## borders (the dark one, or the light one when the theme in use is light).
  ## R_FALSE goes back to the theme used before.
proc Raccessibility_get_high_contrast*(): cint {.rapi.}
proc Raccessibility_set_focus_always_visible*(enabled: cint) {.rapi.}
  ## R_TRUE: the focus ring is always shown (not only after the keyboard was used), and thicker.
proc Raccessibility_get_focus_always_visible*(): cint {.rapi.}
proc Raccessibility_set_reduce_motion*(enabled: cint) {.rapi.}
  ## Reduced motion, for the whole application: R_TRUE and nothing
  ## moves by itself. Changes are immediate (no spring, no slide, no bounce,
  ## no transition), and the effects (raven_shader.h) stand still; they still
  ## follow the pointer. Same as Rset_animations_enabled(R_FALSE).
proc Raccessibility_get_reduce_motion*(): cint {.rapi.}
proc Raccessibility_set_color_filter*(filter: RColorFilter) {.rapi.}
  ## A filter on the colors of the interface (pictures and viewports keep theirs).
proc Raccessibility_get_color_filter*(): RColorFilter {.rapi.}
proc Raccessibility_get_system_settings*(settings: ptr RAccessibilitySettings) {.rapi.}
proc Raccessibility_follow_system*() {.rapi.}
  ## Applies the settings of the system (text size, contrast, motion).
proc Rwidget_set_accessible_name*(widget: Rid, name: cstring) {.rapi.}
  ## What the screen reader says of the widget; NULL: what it shows (or the label before it).
proc Rwidget_get_accessible_name*(widget: Rid): cstring {.rapi.}
  ## The name the screen reader uses (given, or found). Valid until the next call.
proc Rwidget_set_accessible_description*(widget: Rid, description: cstring) {.rapi.}
  ## A longer explanation, read after the name.
proc Rwidget_set_accessible_role*(widget: Rid, role: RAccessibleRole) {.rapi.}
proc Rwidget_get_accessible_role*(widget: Rid): RAccessibleRole {.rapi.}
  ## The role the screen reader sees (given, or the one of the type).
proc Rwidget_set_accessible_hidden*(widget: Rid, hidden: cint) {.rapi.}
  ## R_TRUE: hidden from screen readers, with what it contains (decoration).
proc Rwidget_set_accessible_labelled_by*(widget: Rid, label: Rid) {.rapi.}
  ## The widget is named by another one (a label): its text is the name.
proc Rwidget_set_accessible_action*(widget: Rid, press: RAccessiblePressCallback, user_data: pointer) {.rapi.}
  ## Explicit screen-reader press action, including for custom widgets.
  ## NULL removes the override. Does not change pointer or keyboard handling.
  ## Disabled widgets (including disabled ancestors) cannot be pressed.
proc Rwidget_set_accessible_checked*(widget: Rid, checked: cint) {.rapi.}
  ## Override the checked state: 1 checked, 0 unchecked, -1 restores the
  ## widget's default (not checkable for a custom widget). The application owns
  ## this state; a press callback updates it when appropriate.
proc Raccessibility_announce*(text: cstring, assertive: cint) {.rapi.}
  ## Has a message read (a save that ended, an error). `assertive`: interrupts what is being read.
proc Raccessibility_is_screen_reader_active*(): cint {.rapi.}
  ## R_TRUE when a screen reader is reading the windows of the application.
proc Rhost_platform_register*(desc: ptr RHostPlatformDesc) {.rapi.}
  ## Makes the next Rinit use the host platform. The structure is copied; NULL restores the default.
proc Rhost_platform_is_active*(): cint {.rapi.}
  ## Returns: R_TRUE while the running library uses the host platform.
proc Rhost_window_set_size*(window: Rid, width: cint, height: cint, scale: cfloat) {.rapi.}
  ## Size of a host window in pixels, and pixels per logical unit.
proc Rhost_inject_mouse_move*(window: Rid, x: cint, y: cint, modifiers: uint32) {.rapi.}
proc Rhost_inject_mouse_button*(window: Rid, x: cint, y: cint, button: RMouseButton, pressed: cint, modifiers: uint32) {.rapi.}
proc Rhost_inject_mouse_wheel*(window: Rid, wheel_x: cfloat, wheel_y: cfloat, modifiers: uint32) {.rapi.}
proc Rhost_inject_mouse_leave*(window: Rid) {.rapi.}
  ## The pointer left the window: hover states end.
proc Rhost_inject_key*(window: Rid, key: RKey, pressed: cint, modifiers: uint32, repeat: cint) {.rapi.}
  ## repeat R_TRUE for a key held down (auto-repeat).
proc Rhost_inject_text*(window: Rid, codepoint: uint32) {.rapi.}
  ## One typed Unicode code point.
proc Rhost_inject_focus*(window: Rid, focused: cint) {.rapi.}
  ## The window gained or lost the keyboard. Losing it closes popups and drops captures.
proc Rhost_inject_close*(window: Rid) {.rapi.}
  ## The user asked to close the window (its close callback decides).

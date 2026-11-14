## Raven for Nim.
##
## Generated from the C headers by tools/bindgen (Raven 0.2): do not edit.
##
## `raven_gui/c` is the C API as it is, and is exported here. Around it, a few
## helpers for what Nim does differently from C:
##
## - `text` reads a text of the library as a `string`;
## - `actionBind`, `timerStart` and `errorSetCallback` take closures;
## - `windowFromXml`, `widgetFromXml` and `widgetFind` take a `string`;
## - `fileRead` reads a file as a `string` and frees the copy of the library.
##
## .. code-block:: nim
##   import raven_gui
##
##   discard Rinit(R_BACKEND_VULKAN)
##   discard windowFromXml("""<Window Title="Hi"><Button Text="Quit" OnClicked="quit"/></Window>""")
##   actionBind("quit", proc (sender: Rid, action: string) = Rquit(0))
##   while Rwait_frame(-1) != 0: discard
##   Rshutdown()
##
## Raven belongs to the thread that starts it: call it, and these helpers,
## from that thread only.

import std/[options, tables]
import raven_gui/c
export c

type
  ActionProc* = proc (sender: Rid, action: string) {.closure.}
    ## What `actionBind` calls.
  TimerProc* = proc () {.closure.}
    ## What `timerStart` calls.
  ErrorProc* = proc (code: RError, severity: RSeverity, detail: string) {.closure.}
    ## What `errorSetCallback` calls.

proc text*(pointer: cstring): string =
  ## A text of the library as a `string` ("" for nil).
  if pointer.isNil: "" else: $pointer

proc text*[I](chars: array[I, cchar]): string =
  ## The text held by a `cchar` array of a structure (`RXmlError.message`).
  for ch in chars:
    if ch == '\0':
      break
    result.add ch

proc fileRead*(path: string): Option[string] =
  ## The content of a file (`Rfile_read`), none when it cannot be read. The
  ## copy of the library is freed (`Rmemory_free`).
  var size: csize_t
  let data = Rfile_read(path.cstring, addr size)
  if data.isNil:
    return none(string)
  var content = newString(int(size))
  if size > 0:
    copyMem(addr content[0], data, int(size))
  Rmemory_free(data)
  some(content)

# what the library calls: one procedure of each kind, which finds the closure
var actions: Table[string, ActionProc]
var timers: Table[Rid, tuple[call: TimerProc, repeating: bool]]
var errorHandler: ErrorProc

# (an exception must not cross C: it is told on stderr)
proc report(error: ref Exception) {.raises: [].} =
  try:
    stderr.writeLine("raven: " & error.msg)
  except Exception:
    discard

proc actionCallback(sender: Rid, name: cstring, userData: pointer) {.cdecl, raises: [].} =
  try:
    let action = text(name)
    let call = actions.getOrDefault(action)
    if call != nil:
      call(sender, action)
  except Exception as error:
    report(error)

proc timerCallback(timer: Rid, userData: pointer) {.cdecl, raises: [].} =
  try:
    if timer in timers:
      let entry = timers[timer]
      entry.call()
      # a timer that fires once is done with its closure
      if not entry.repeating:
        timers.del(timer)
  except Exception as error:
    report(error)

proc errorCallback(code: RError, severity: RSeverity, detail: cstring) {.cdecl, raises: [].} =
  try:
    if errorHandler != nil:
      errorHandler(code, severity, text(detail))
  except Exception as error:
    report(error)

proc actionBind*(name: string, call: ActionProc) =
  ## Binds an action (`OnClicked="name"` in XML, `Rwidget_set_action`) to a
  ## closure, which receives the widget that fired it and the name of the action.
  ## Binding the name again replaces the closure.
  actions[name] = call
  Raction_bind(name.cstring, actionCallback, nil)

proc actionUnbind*(name: string) =
  ## Removes the closure of an action (`Raction_unbind`).
  Raction_unbind(name.cstring)
  actions.del(name)

proc timerStart*(seconds: float, repeating: bool, call: TimerProc): Rid {.discardable.} =
  ## Calls a closure after `seconds`, once or again and again (`Rtimer_create`).
  ## Stop it with `timerStop`.
  result = Rtimer_create(seconds, cint(repeating), timerCallback, nil)
  if result != R_INVALID_ID:
    timers[result] = (call, repeating)

proc timerStop*(timer: Rid) =
  ## Stops a timer started with `timerStart`.
  if Rtimer_is_valid(timer) != 0:
    Rtimer_destroy(timer)
  timers.del(timer)

proc errorSetCallback*(handler: ErrorProc) =
  ## Gives the errors and warnings of the library to a closure; nil removes it
  ## (`Rerror_set_callback`).
  errorHandler = handler
  Rerror_set_callback(if handler == nil: nil else: errorCallback)

proc windowFromXml*(xml: string): Rid {.discardable.} =
  ## Creates a window from XML (`Rwindow_create_from_xml`).
  Rwindow_create_from_xml(xml.cstring, csize_t(xml.len))

proc widgetFromXml*(xml: string): Rid {.discardable.} =
  ## Creates a widget tree from XML (`Rwidget_create_from_xml`).
  Rwidget_create_from_xml(xml.cstring, csize_t(xml.len))

proc widgetFind*(scope: Rid, name: string): Rid =
  ## Finds a widget by its name (`Rwidget_find`).
  Rwidget_find(scope, name.cstring)

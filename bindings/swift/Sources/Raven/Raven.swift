// Raven for Swift.
//
// Generated from the C headers by tools/bindgen (Raven 0.2): do not edit.
//
// `import Raven` gives the C API as it is, read by Swift from the headers: the
// same names, the same documentation, Swift strings where C takes a text
// (`Rlabel_set_text(label, "Hi")`), `inout` where it takes a pointer to a value.
// Around it, a few helpers for what Swift does differently from C:
//
// - `text` reads a text of the library;
// - `actionBind`, `timerStart` and `errorSetCallback` take closures;
// - `windowFromXml` and `widgetFromXml` take a `String`.
//
//     import Raven
//
//     Rinit(R_BACKEND_VULKAN)
//     let window = windowFromXml(#"<Window Title="Hi"><Button Text="Quit" OnClicked="quit"/></Window>"#)
//     actionBind("quit") { _, _ in Rquit(0) }
//     while Rwait_frame(-1) != 0 {}
//     Rshutdown()
//
// Raven belongs to the thread that starts it: call it, and these helpers, from
// that thread only.

@_exported import CRaven

/// A text of the library as a `String` ("" for a null pointer).
public func text(_ pointer: UnsafePointer<CChar>?) -> String {
    guard let pointer = pointer else { return "" }
    return String(cString: pointer)
}

/// A text of the library as a `String` ("" for a null pointer).
public func text(_ pointer: UnsafeMutablePointer<CChar>?) -> String {
    return text(UnsafePointer(pointer))
}

/// The text held by a fixed array of a structure (`char message[256]`, a tuple in Swift).
public func text<T>(fixed array: T) -> String {
    return withUnsafeBytes(of: array) { bytes in
        let end = bytes.firstIndex(of: 0) ?? bytes.count
        return String(decoding: bytes[..<end], as: UTF8.self)
    }
}

private final class RavenAction {
    let function: (Rid, String) -> Void
    init(_ function: @escaping (Rid, String) -> Void) { self.function = function }
}

private final class RavenTimer {
    let function: () -> Void
    let repeating: Bool
    init(_ function: @escaping () -> Void, _ repeating: Bool) {
        self.function = function
        self.repeating = repeating
    }
}

// what the closures given to the library are kept alive by
private var ravenActions: [String: RavenAction] = [:]
private var ravenTimers: [Rid: RavenTimer] = [:]
private var ravenErrorHandler: ((RError, RSeverity, String) -> Void)?

/// Binds an action (`OnClicked="name"` in XML, `Rwidget_set_action`) to a
/// closure, which receives the widget that fired it and the name of the action.
/// Binding the name again replaces the closure.
public func actionBind(_ name: String, _ function: @escaping (_ sender: Rid, _ action: String) -> Void) {
    let action = RavenAction(function)
    ravenActions[name] = action
    Raction_bind(name, { sender, name, userData in
        guard let userData = userData else { return }
        Unmanaged<RavenAction>.fromOpaque(userData).takeUnretainedValue().function(sender, text(name))
    }, Unmanaged.passUnretained(action).toOpaque())
}

/// Removes the closure of an action (`Raction_unbind`).
public func actionUnbind(_ name: String) {
    Raction_unbind(name)
    ravenActions[name] = nil
}

/// Calls a closure after `seconds`, once or again and again (`Rtimer_create`).
/// Stop it with `timerStop`.
@discardableResult
public func timerStart(_ seconds: Double, repeating: Bool = false, _ function: @escaping () -> Void) -> Rid {
    let timer = RavenTimer(function, repeating)
    let id = Rtimer_create(seconds, repeating ? 1 : 0, { id, userData in
        guard let userData = userData else { return }
        let timer = Unmanaged<RavenTimer>.fromOpaque(userData).takeUnretainedValue()
        timer.function()
        // a timer that fires once is done with its closure
        if !timer.repeating && ravenTimers[id] === timer { ravenTimers[id] = nil }
    }, Unmanaged.passUnretained(timer).toOpaque())
    if id != R_INVALID_ID { ravenTimers[id] = timer }
    return id
}

/// Stops a timer started with `timerStart`.
public func timerStop(_ timer: Rid) {
    if Rtimer_is_valid(timer) != 0 { Rtimer_destroy(timer) }
    ravenTimers[timer] = nil
}

/// Gives the errors and warnings of the library to a closure; `nil` removes it
/// (`Rerror_set_callback`).
public func errorSetCallback(_ function: ((_ code: RError, _ severity: RSeverity, _ detail: String) -> Void)?) {
    ravenErrorHandler = function
    if function == nil {
        Rerror_set_callback(nil)
    } else {
        Rerror_set_callback({ code, severity, detail in
            ravenErrorHandler?(code, severity, text(detail))
        })
    }
}

/// Creates a window from XML (`Rwindow_create_from_xml`).
public func windowFromXml(_ xml: String) -> Rid {
    return Rwindow_create_from_xml(xml, xml.utf8.count)
}

/// Creates a widget tree from XML (`Rwidget_create_from_xml`).
public func widgetFromXml(_ xml: String) -> Rid {
    return Rwidget_create_from_xml(xml, xml.utf8.count)
}

/**
 * Raven for D.
 *
 * Generated from the C headers by tools/bindgen (Raven 0.2): do not edit.
 *
 * `raven.c` is the C API as it is, and is imported here. Around it, a few
 * helpers for what D does differently from C:
 *
 * - `text` reads a text of the library as a `string`;
 * - `actionBind`, `timerStart` and `errorSetCallback` take delegates;
 * - `windowFromXml`, `widgetFromXml` and `widgetFind` take a `string`.
 *
 * ---
 * import raven;
 *
 * void main()
 * {
 *     Rinit(R_BACKEND_VULKAN);
 *     windowFromXml(`<Window Title="Hi"><Button Text="Quit" OnClicked="quit"/></Window>`);
 *     actionBind("quit", (Rid, string) { Rquit(0); });
 *     while (Rwait_frame(-1)) {}
 *     Rshutdown();
 * }
 * ---
 *
 * Raven belongs to the thread that starts it: call it, and these helpers,
 * from that thread only.
 */
module raven;

public import raven.c;

import core.stdc.stdio : fprintf, stderr;
import std.string : fromStringz, toStringz;

/// A text of the library as a `string` ("" for a null pointer).
string text(const(char)* pointer)
{
    return pointer is null ? "" : fromStringz(pointer).idup;
}

/// The text held by a `char` array of a structure (`RXmlError.message`).
string text(size_t n)(const ref char[n] array)
{
    size_t end = 0;
    while (end < n && array[end] != 0)
        end++;
    return array[0 .. end].idup;
}

// what the library calls: one function of each kind, which finds the delegate
private void delegate(Rid sender, string action)[string] actions;
private struct Timer
{
    void delegate() function_;
    bool repeating;
}
private Timer[Rid] timers;
private void delegate(RError code, RSeverity severity, string detail) errorHandler;

// (an exception must not cross C: it is told on stderr)
private void report(Throwable error) nothrow
{
    try
        fprintf(stderr, "raven: %.*s\n", cast(int) error.msg.length, error.msg.ptr);
    catch (Throwable) {}
}

private extern (C) void actionCallback(Rid sender, const(char)* name, void*) nothrow
{
    try
    {
        auto action = text(name);
        if (auto function_ = action in actions)
            (*function_)(sender, action);
    }
    catch (Throwable error)
        report(error);
}

private extern (C) void timerCallback(Rid timer, void*) nothrow
{
    try
    {
        if (auto found = timer in timers)
        {
            auto entry = *found;
            entry.function_();
            // a timer that fires once is done with its delegate
            if (!entry.repeating)
                timers.remove(timer);
        }
    }
    catch (Throwable error)
        report(error);
}

private extern (C) void errorCallback(RError code, RSeverity severity, const(char)* detail) nothrow
{
    try
    {
        if (errorHandler !is null)
            errorHandler(code, severity, text(detail));
    }
    catch (Throwable error)
        report(error);
}

/**
 * Binds an action (`OnClicked="name"` in XML, `Rwidget_set_action`) to a
 * delegate, which receives the widget that fired it and the name of the action.
 * Binding the name again replaces the delegate.
 */
void actionBind(string name, void delegate(Rid sender, string action) function_)
{
    actions[name] = function_;
    Raction_bind(toStringz(name), &actionCallback, null);
}

/// ditto
void actionBind(string name, void function(Rid sender, string action) function_)
{
    import std.functional : toDelegate;
    actionBind(name, toDelegate(function_));
}

/// Removes the delegate of an action (`Raction_unbind`).
void actionUnbind(string name)
{
    Raction_unbind(toStringz(name));
    actions.remove(name);
}

/// Calls a delegate after `seconds`, once or again and again (`Rtimer_create`). Stop it with `timerStop`.
Rid timerStart(double seconds, bool repeating, void delegate() function_)
{
    Rid timer = Rtimer_create(seconds, repeating ? 1 : 0, &timerCallback, null);
    if (timer != R_INVALID_ID)
        timers[timer] = Timer(function_, repeating);
    return timer;
}

/// Stops a timer started with `timerStart`.
void timerStop(Rid timer)
{
    if (Rtimer_is_valid(timer))
        Rtimer_destroy(timer);
    timers.remove(timer);
}

/// Gives the errors and warnings of the library to a delegate; `null` removes it (`Rerror_set_callback`).
void errorSetCallback(void delegate(RError code, RSeverity severity, string detail) handler)
{
    errorHandler = handler;
    Rerror_set_callback(handler is null ? null : &errorCallback);
}

/// Creates a window from XML (`Rwindow_create_from_xml`).
Rid windowFromXml(string xml)
{
    return Rwindow_create_from_xml(xml.ptr, xml.length);
}

/// Creates a widget tree from XML (`Rwidget_create_from_xml`).
Rid widgetFromXml(string xml)
{
    return Rwidget_create_from_xml(xml.ptr, xml.length);
}

/// Finds a widget by its name (`Rwidget_find`).
Rid widgetFind(Rid scope_, string name)
{
    return Rwidget_find(scope_, toStringz(name));
}

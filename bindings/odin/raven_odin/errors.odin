package raven_odin

import "base:runtime"
import "core:log"
import "core:sync"

import raw "../raven"

// Errors of Raven: the last one, and a procedure called for each.
//
// When the application sets nothing, init sends them to the logger of its
// `context` (`context.logger`, core:log). A program with a logger finds the
// errors of Raven in it; a program without one sees nothing, as in C.

Error_Proc :: #type proc(code: Error, severity: Severity, detail: string)

@(private)
Error_Binding :: struct {
	call:       Error_Proc,
	ctx:        runtime.Context,
	thread:     int,  // the thread that registered it
	by_default: bool, // set up by init, removed by shutdown
}

// Read by the trampoline on any thread, so behind a lock.
@(private)
error_lock: sync.Mutex
@(private)
error_binding: Error_Binding
// True once the application called error_set_callback: init then sets up nothing.
@(private)
error_chosen: bool

// The last error, which this call clears. code is .No_Error when there was none.
// detail is valid until the next call of error_get_last.
error_get_last :: proc() -> (code: Error, severity: Severity, detail: string) {
	text: cstring
	level: raw.RSeverity
	code = Error(raw.Rerror_get_last(&text, &level))
	return code, Severity(level), string(text)
}

// Calls `call` for every error Raven raises, with the `context` that is current
// now. nil removes it, and the logging of init with it. May be called before init.
// `detail` is valid during the call only.
//
// Raven calls it on the thread that raised the error, which can be its render
// thread: what `call` uses of the context (the logger, the allocator) must
// allow that. On a thread other than this one, `context.temp_allocator` is the
// default one of that thread.
error_set_callback :: proc(call: Error_Proc) {
	error_chosen = true
	error_binding_set({call = call, ctx = context, thread = sync.current_thread_id()})
}

// An Error_Proc that writes to `context.logger`: what init sets up.
error_log :: proc(code: Error, severity: Severity, detail: string) {
	level := log.Level.Error
	switch severity {
	case .Weak_Warning:
		level = .Info
	case .Warning:
		level = .Warning
	case .Error:
		level = .Error
	case .Fatal:
		level = .Fatal
	}
	log.logf(level, "raven: %v: %s", code, detail)
}

@(private)
error_binding_set :: proc(binding: Error_Binding) {
	sync.mutex_lock(&error_lock)
	error_binding = binding
	sync.mutex_unlock(&error_lock)
	if binding.call != nil {
		raw.Rerror_set_callback(error_trampoline)
	} else {
		raw.Rerror_set_callback(nil)
	}
}

// Called by init, before Raven starts, so that the errors of the start are logged too.
@(private)
error_log_by_default :: proc() {
	if !error_chosen {
		error_binding_set({call = error_log, ctx = context, thread = sync.current_thread_id(), by_default = true})
	}
}

// Called by shutdown: the logger of the context of init may not live longer.
@(private)
error_default_remove :: proc() {
	sync.mutex_lock(&error_lock)
	by_default := error_binding.by_default
	sync.mutex_unlock(&error_lock)
	if by_default {
		error_binding_set({})
	}
}

@(private)
error_trampoline :: proc "c" (code: raw.RError, severity: raw.RSeverity, detail: cstring) {
	sync.mutex_lock(&error_lock)
	binding := error_binding
	sync.mutex_unlock(&error_lock)
	if binding.call == nil {
		return
	}
	// (built in a variable: an assignment to `context` inside the `if` would
	// only hold for that block)
	ctx := binding.ctx
	if sync.current_thread_id() != binding.thread {
		// the temporary allocator of the registering thread belongs to that thread
		ctx.temp_allocator = runtime.default_context().temp_allocator
	}
	context = ctx
	binding.call(Error(code), Severity(severity), string(detail))
}

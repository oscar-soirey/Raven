package raven_odin

import "base:runtime"
import "core:strings"

import raw "../raven"

// Callbacks of the C API are `proc "c"` and have no Odin `context`. Here the
// application gives a normal Odin procedure; one trampoline per callback type
// restores the `context` of the call that registered it and calls the procedure.
//
// The C side gets no pointer to Odin memory: it only gets the trampoline. The
// trampoline finds the registration by the name of the action or the id of the
// timer, so a registration that is replaced, unbound or destroyed while its own
// callback runs can never be a dangling pointer.
//
// All of it runs on the UI thread of Raven (the thread that called init).

Action_Proc :: #type proc(sender: Widget, action: string, user_data: rawptr)
Timer_Proc  :: #type proc(timer: Timer, user_data: rawptr)
Frame_Proc  :: #type proc(delta_seconds: f64, user_data: rawptr)

@(private)
Action_Binding :: struct {
	call:      Action_Proc,
	user_data: rawptr,
	ctx:       runtime.Context, // of the caller of action_bind; also owns the key
}

@(private)
Timer_Binding :: struct {
	call:      Timer_Proc,
	user_data: rawptr,
	ctx:       runtime.Context,
}

@(private)
Frame_Binding :: struct {
	call:      Frame_Proc,
	user_data: rawptr,
	ctx:       runtime.Context,
}

@(private)
action_bindings: map[string]Action_Binding
@(private)
timer_bindings: map[Timer]Timer_Binding
@(private)
frame_binding: Frame_Binding

// Binds an action name (OnClicked="name" in XML) to a procedure. A second bind of
// the same name replaces the first. The procedure runs with the `context` that
// was current when action_bind was called. `action` is valid during the call only.
action_bind :: proc(name: string, call: Action_Proc, user_data: rawptr = nil) {
	assert(call != nil, "action_bind: call is nil")
	cname := strings.clone_to_cstring(name, context.temp_allocator)
	action_binding_remove(name)
	action_bindings[strings.clone(name)] = {call, user_data, context}
	raw.Raction_bind(cname, action_trampoline, nil)
}

action_unbind :: proc(name: string) {
	cname := strings.clone_to_cstring(name, context.temp_allocator)
	action_binding_remove(name)
	raw.Raction_unbind(cname)
}

@(private)
action_binding_remove :: proc(name: string) {
	if _, found := action_bindings[name]; found {
		key, old := delete_key(&action_bindings, name)
		delete(key, old.ctx.allocator)
	}
}

// Fires an action as a widget would.
action_fire :: proc(name: string, sender: Widget = INVALID) {
	cname := strings.clone_to_cstring(name, context.temp_allocator)
	raw.Raction_fire(cname, raw.Rid(sender))
}

// Calls `call` on the UI thread after `interval` seconds; every `interval` seconds
// when `repeat`. A one-shot timer deletes itself. The procedure runs with the
// `context` that was current when timer_create was called.
timer_create :: proc(interval: f64, call: Timer_Proc, user_data: rawptr = nil, repeat := false) -> (timer: Timer, ok: bool) #optional_ok {
	assert(call != nil, "timer_create: call is nil")
	id := raw.Rtimer_create(interval, 1 if repeat else 0, timer_trampoline, nil)
	if id == raw.R_INVALID_ID {
		return Timer(INVALID), false
	}
	timer = Timer(id)
	timer_bindings[timer] = {call, user_data, context}
	return timer, true
}

timer_destroy :: proc(timer: Timer) {
	delete_key(&timer_bindings, timer)
	raw.Rtimer_destroy(raw.Rid(timer))
}

// Calls `call` once per frame, before the layout, with the `context` that is
// current now. There is one frame procedure: a second call replaces the first,
// nil removes it. To be called after init.
set_frame_callback :: proc(call: Frame_Proc, user_data: rawptr = nil) {
	if call == nil {
		frame_binding = {}
		raw.Rset_frame_callback(nil, nil)
		return
	}
	frame_binding = {call, user_data, context}
	raw.Rset_frame_callback(frame_trampoline, nil)
}

@(private)
frame_trampoline :: proc "c" (delta_seconds: f64, user_data: rawptr) {
	binding := frame_binding
	if binding.call == nil {
		return
	}
	context = binding.ctx
	binding.call(delta_seconds, binding.user_data)
}

@(private)
action_trampoline :: proc "c" (sender: raw.Rid, action: cstring, user_data: rawptr) {
	context = runtime.default_context()
	name := string(action)
	binding, found := action_bindings[name]
	if !found {
		return
	}
	context = binding.ctx
	binding.call(Widget(sender), name, binding.user_data)
}

@(private)
timer_trampoline :: proc "c" (timer: raw.Rid, user_data: rawptr) {
	context = runtime.default_context()
	id := Timer(timer)
	binding, found := timer_bindings[id]
	if !found {
		return
	}
	context = binding.ctx
	binding.call(id, binding.user_data)
	// A one-shot timer is gone after its call: release the registration.
	// (A repeating timer is released by timer_destroy or shutdown.)
	if raw.Rtimer_is_valid(timer) == 0 {
		delete_key(&timer_bindings, id)
	}
}

@(private)
callbacks_release_all :: proc() {
	for key, binding in action_bindings {
		delete(key, binding.ctx.allocator)
	}
	delete(action_bindings)
	delete(timer_bindings)
	action_bindings = nil
	timer_bindings = nil
	frame_binding = {}
}

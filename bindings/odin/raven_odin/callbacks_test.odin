package raven_odin

import "core:mem"
import "core:testing"

// The trampolines of callbacks.odin, headless. A tracking allocator checks that
// every registration is released (R3 of the design): the test runner itself only
// logs a leak as a warning.

@(private = "file")
State :: struct {
	fired:         int,
	context_ok:    bool,
	name_ok:       bool,
	sender:        Widget,
	replaced:      int,
	timer_fired:   int,
	timer_context: bool,
	repeat_fired:  int,
	repeat_timer:  Timer,
}

@(test)
callbacks_use_the_context_of_the_caller :: proc(t: ^testing.T) {
	track: mem.Tracking_Allocator
	mem.tracking_allocator_init(&track, context.allocator)
	defer mem.tracking_allocator_destroy(&track)
	context.allocator = mem.tracking_allocator(&track)

	testing.expect(t, init(headless = true), "init")

	state: State

	// ---- actions
	context.user_index = 42
	action_bind("greet", proc(sender: Widget, action: string, data: rawptr) {
		s := (^State)(data)
		s.fired += 1
		s.context_ok = context.user_index == 42
		s.name_ok = action == "greet"
		s.sender = sender
	}, rawptr(&state))

	context.user_index = 0 // fired from another context than the one that bound it
	action_fire("greet", Widget(7))
	testing.expect_value(t, state.fired, 1)
	testing.expect(t, state.context_ok, "the context of action_bind is restored")
	testing.expect(t, state.name_ok, "the action name arrives as a string")
	testing.expect_value(t, state.sender, Widget(7))

	// a second bind replaces the first
	action_bind("greet", proc(sender: Widget, action: string, data: rawptr) {
		(^State)(data).replaced += 1
	}, rawptr(&state))
	action_fire("greet")
	testing.expect_value(t, state.fired, 1)
	testing.expect_value(t, state.replaced, 1)

	// an unbound action calls nothing
	action_unbind("greet")
	action_fire("greet")
	testing.expect_value(t, state.replaced, 1)
	testing.expect_value(t, len(action_bindings), 0)

	// ---- a one-shot timer fires once and releases its registration
	context.user_index = 7
	one_shot := timer_create(0.01, proc(timer: Timer, data: rawptr) {
		s := (^State)(data)
		s.timer_fired += 1
		s.timer_context = context.user_index == 7
	}, rawptr(&state))
	testing.expect(t, one_shot != Timer(INVALID), "timer created")
	for _ in 0 ..< 100 {
		wait_frame(0.02)
		if state.timer_fired > 0 {
			break
		}
	}
	for _ in 0 ..< 5 {
		wait_frame(0.02) // it must not fire again
	}
	testing.expect_value(t, state.timer_fired, 1)
	testing.expect(t, state.timer_context, "the context of timer_create is restored")
	testing.expect_value(t, len(timer_bindings), 0)

	// ---- a repeating timer that destroys itself from its own callback
	state.repeat_timer = timer_create(0.005, proc(timer: Timer, data: rawptr) {
		s := (^State)(data)
		s.repeat_fired += 1
		if s.repeat_fired == 3 {
			timer_destroy(timer)
		}
	}, rawptr(&state), repeat = true)
	for _ in 0 ..< 200 {
		wait_frame(0.01)
		if state.repeat_fired >= 3 {
			break
		}
	}
	for _ in 0 ..< 5 {
		wait_frame(0.02)
	}
	testing.expect_value(t, state.repeat_fired, 3)
	testing.expect_value(t, len(timer_bindings), 0)

	// ---- what is still bound at shutdown is released by shutdown
	action_bind("left_over", proc(sender: Widget, action: string, data: rawptr) {})
	timer_create(60, proc(timer: Timer, data: rawptr) {}, repeat = true)
	shutdown()
	testing.expect_value(t, len(action_bindings), 0)
	testing.expect_value(t, len(timer_bindings), 0)
	testing.expect_value(t, len(track.allocation_map), 0)
}

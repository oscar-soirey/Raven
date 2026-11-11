package raven_odin

import raw "../raven"

// Starts Raven. The calling thread becomes the UI thread.
// headless: no window system and no GPU (the null backend), for tests.
init :: proc(headless := false) -> bool {
	desc: raw.RInitDesc
	if headless {
		desc.backend = raw.R_BACKEND_NULL
		desc.headless = 1
	} else {
		desc.backend = raw.R_BACKEND_VULKAN
	}
	return raw.Rinit_ex(&desc) != 0
}

// Destroys every window and widget, and releases what the Odin side holds for
// callbacks (action_bind, timer_create).
shutdown :: proc() {
	raw.Rshutdown()
	callbacks_release_all()
}

quit :: proc(exit_code: i32 = 0) {
	raw.Rquit(exit_code)
}

draw_frame :: proc() {
	raw.Rdraw_frame()
}

// Waits up to `timeout` seconds (negative: until something happens), runs timers
// and tasks, draws. Returns false once the application quit.
wait_frame :: proc(timeout: f64 = -1) -> bool {
	return raw.Rwait_frame(timeout) != 0
}

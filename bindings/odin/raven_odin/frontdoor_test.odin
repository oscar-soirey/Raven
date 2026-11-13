package raven_odin

import "base:runtime"
import "core:log"
import "core:mem"
import "core:strings"
import "core:sync"
import "core:testing"
import "core:thread"

import raw "../raven"

// Errors, the frame callback, run, capture and the allocator, headless.

// An XML text with a mistake: Raven raises an error for it.
@(private = "file")
BAD_XML :: `<Window><Oops`

@(private = "file")
Logged :: struct {
	count:    int,
	level:    log.Level,
	prefixed: bool,
}

// An Error_Proc has no user data: what the callbacks of the test see is here.
@(private = "file")
Seen :: struct {
	errors:     int,
	code:       Error,
	detail_ok:  bool,
	context_ok: bool,
	thread:     int,
	temp_data:  rawptr,
}

@(private = "file")
seen: Seen

@(private = "file")
counting_logger_proc :: proc(data: rawptr, level: log.Level, text: string, options: log.Options, location := #caller_location) {
	logged := (^Logged)(data)
	logged.count += 1
	logged.level = level
	logged.prefixed = strings.has_prefix(text, "raven: ")
}

@(private = "file")
record_error :: proc(code: Error, severity: Severity, detail: string) {
	seen.errors += 1
	seen.code = code
	seen.detail_ok = len(detail) > 0
	seen.context_ok = context.user_index == 11
	seen.thread = sync.current_thread_id()
	seen.temp_data = context.temp_allocator.data
}

@(test)
errors_reach_the_logger_and_the_callback :: proc(t: ^testing.T) {
	seen = {}
	logged: Logged

	// ---- nothing set: init sends the errors to the logger of its context.
	// Only init gets the counting logger: the checks of the test report through
	// `context.logger` too, and must reach the test runner.
	started: bool
	{
		context.logger = runtime.Logger{procedure = counting_logger_proc, data = &logged, lowest_level = .Debug}
		started = init(headless = true)
	}
	testing.expect(t, started, "init")
	_, built := window_create_from_xml(BAD_XML)
	testing.expect(t, !built, "a window from a wrong XML text")
	testing.expect_value(t, logged.count, 1)
	testing.expect(t, logged.prefixed, "the message starts with raven:")
	testing.expect(t, logged.level >= log.Level.Warning, "an error is logged as a warning or more")

	// ---- the last error, which reading clears
	code, _, detail := error_get_last()
	testing.expect_value(t, code, Error.Error_Xml)
	testing.expect(t, len(detail) > 0, "the last error has a detail")
	code, _, _ = error_get_last()
	testing.expect_value(t, code, Error.No_Error)

	// ---- a procedure of the application replaces the logging
	context.user_index = 11
	error_set_callback(record_error)
	context.user_index = 0 // raised from another context than the one that registered
	window_create_from_xml(BAD_XML)
	testing.expect_value(t, seen.errors, 1)
	testing.expect_value(t, seen.code, Error.Error_Xml)
	testing.expect(t, seen.detail_ok, "the detail arrives as a string")
	testing.expect(t, seen.context_ok, "the context of error_set_callback is restored")
	testing.expect_value(t, seen.temp_data, context.temp_allocator.data)
	testing.expect_value(t, logged.count, 1)

	// ---- an error raised on another thread (as the render thread of Raven does)
	// gets the temporary allocator of that thread
	other := thread.create_and_start(proc() {
		error_trampoline(raw.RError(Error.Error_Backend), raw.RSeverity(Severity.Warning), "from another thread")
	})
	thread.join(other)
	thread.destroy(other)
	testing.expect_value(t, seen.errors, 2)
	testing.expect_value(t, seen.code, Error.Error_Backend)
	testing.expect(t, seen.context_ok, "the context is restored on the other thread too")
	testing.expect(t, seen.thread != sync.current_thread_id(), "it ran on the other thread")
	testing.expect(t, seen.temp_data != context.temp_allocator.data, "not the temporary allocator of this thread")

	// ---- nil removes it: no procedure, no logging
	error_set_callback(nil)
	window_create_from_xml(BAD_XML)
	testing.expect_value(t, seen.errors, 2)
	testing.expect_value(t, logged.count, 1)
	error_get_last()

	shutdown()
	error_chosen = false // as a new program: the next init logs again
}

@(private = "file")
Frames :: struct {
	count:      int,
	context_ok: bool,
	delta_ok:   bool,
}

@(test)
frame_callback_run_capture_and_allocator :: proc(t: ^testing.T) {
	testing.expect(t, init(headless = true), "init")
	window, built := window_create_from_xml(`<Window Title="Test" Width="64" Height="48"><Label Text="x"/></Window>`)
	testing.expect(t, built, "window")

	// ---- capture: the null backend gives the clear color in the size of the frame
	wait_frame(0)
	pixels, width, height, captured := window_capture(window)
	testing.expect(t, captured, "capture")
	testing.expect(t, width > 0 && height > 0, "the capture has a size")
	testing.expect_value(t, len(pixels), int(width) * int(height) * 4)
	pixels_free(pixels)

	// ---- the allocator of Raven behind the allocator of Odin
	a := allocator()
	before := memory_stats().total_allocations
	numbers := make([dynamic]int, a)
	for i in 0 ..< 1000 {
		append(&numbers, i)
	}
	testing.expect_value(t, len(numbers), 1000)
	testing.expect_value(t, numbers[999], 999)
	testing.expect(t, memory_stats().total_allocations > before, "the blocks come from Raven")
	delete(numbers)

	Block :: struct {
		bytes: [64]u8,
	}
	block := new(Block, a) // Raven fills new memory with 0xCD in a debug build
	zeroed := true
	for b in block.bytes {
		zeroed &&= b == 0
	}
	testing.expect(t, zeroed, "new gives zeroed memory")
	free(block, a)

	_, alloc_error := mem.alloc_bytes(8, 64, a)
	testing.expect_value(t, alloc_error, mem.Allocator_Error.Invalid_Argument)
	testing.expect(t, memory_check(), "the heap of Raven is intact")

	// ---- the frame callback, with the context of the caller
	frames: Frames
	context.user_index = 5
	set_frame_callback(proc(delta_seconds: f64, data: rawptr) {
		f := (^Frames)(data)
		f.count += 1
		f.context_ok = context.user_index == 5
		f.delta_ok = delta_seconds >= 0
	}, rawptr(&frames))
	context.user_index = 0
	wait_frame(0)
	wait_frame(0)
	testing.expect(t, frames.count >= 1, "the frame callback runs")
	testing.expect(t, frames.context_ok, "the context of set_frame_callback is restored")
	testing.expect(t, frames.delta_ok, "the time of the frame arrives")

	counted := frames.count
	set_frame_callback(nil)
	wait_frame(0)
	testing.expect_value(t, frames.count, counted)

	// ---- run returns the code given to quit
	timer_create(0.01, proc(timer: Timer, data: rawptr) {
		quit(3)
	})
	testing.expect_value(t, run(), i32(3))
	testing.expect_value(t, exit_code(), i32(3))
	testing.expect(t, !is_running(), "not running after quit")

	shutdown()
}

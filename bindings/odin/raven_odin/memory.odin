package raven_odin

import "base:runtime"

import raw "../raven"

// The heap of Raven as an Odin allocator, and its checks.

// Alignment of every block of Raven. A larger alignment is refused.
MEMORY_ALIGNMENT :: 16

// The allocator of Raven (Rmemory_alloc, Rmemory_realloc, Rmemory_free), or the
// one the application gave to Rset_allocator. Thread-safe. Its blocks count in
// memory_stats. Running out of memory is a fatal error of Raven: it never
// returns .Out_Of_Memory.
//
//     pixels := make([]u8, width * height * 4, raven.allocator())
allocator :: proc "contextless" () -> runtime.Allocator {
	return {procedure = allocator_proc, data = nil}
}

// Walks the whole heap of Raven and verifies its structure. False when it is
// corrupted. Slow: for tests and for hunting a bug.
memory_check :: proc() -> bool {
	return raw.Rmemory_check() != 0
}

// Returns to the OS the regions of the heap that hold nothing.
memory_trim :: proc() {
	raw.Rmemory_trim()
}

@(private)
allocator_proc :: proc(
	allocator_data: rawptr,
	mode: runtime.Allocator_Mode,
	size, alignment: int,
	old_memory: rawptr,
	old_size: int,
	location := #caller_location,
) -> ([]byte, runtime.Allocator_Error) {
	switch mode {
	case .Alloc, .Alloc_Non_Zeroed:
		if alignment > MEMORY_ALIGNMENT {
			return nil, .Invalid_Argument
		}
		if size <= 0 {
			return nil, nil
		}
		ptr := raw.Rmemory_alloc(uint(size))
		if mode == .Alloc {
			runtime.mem_zero(ptr, size)
		}
		return ([^]byte)(ptr)[:size], nil

	case .Resize, .Resize_Non_Zeroed:
		if alignment > MEMORY_ALIGNMENT {
			return nil, .Invalid_Argument
		}
		if size <= 0 {
			raw.Rmemory_free(old_memory)
			return nil, nil
		}
		ptr := raw.Rmemory_realloc(old_memory, uint(size))
		if mode == .Resize && size > old_size {
			runtime.mem_zero(([^]byte)(ptr)[old_size:], size - old_size)
		}
		return ([^]byte)(ptr)[:size], nil

	case .Free:
		raw.Rmemory_free(old_memory)
		return nil, nil

	case .Query_Features:
		set := (^runtime.Allocator_Mode_Set)(old_memory)
		if set != nil {
			set^ = {.Alloc, .Alloc_Non_Zeroed, .Free, .Resize, .Resize_Non_Zeroed, .Query_Features}
		}
		return nil, nil

	case .Free_All, .Query_Info:
		return nil, .Mode_Not_Implemented
	}
	return nil, .Mode_Not_Implemented
}

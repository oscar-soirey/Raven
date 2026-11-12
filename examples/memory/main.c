/**
 * Raven example - memory
 * Where the memory of the library comes from, and how to watch it.
 *
 * Raven never calls malloc: it has its own allocator (regions of 4 MB from
 * the system, handed out by a TLSF heap). An application can give its own
 * instead with Rset_allocator, before Rinit: an engine that tracks every
 * block, a memory budget, a debugging tool. This example gives one that
 * counts, and shows next to it what the library reports about itself
 * (Rmemory_get_stats).
 *
 *   raven_memory            the counting allocator of this file
 *   raven_memory --raven    the allocator of Raven
 */

#include "../common/example.h"

/* ---------------------------------------------------------------------------
 *  An allocator of the application: malloc, counted
 *
 *  The functions are called from several threads (the render thread, the
 *  worker threads): the counters are only touched under a lock. Each block
 *  remembers its size in a header of 16 bytes, which also keeps the
 *  alignment the library asks for.
 * ------------------------------------------------------------------------- */

typedef struct counter_t {
	size_t blocks;
	size_t bytes;
	size_t calls;
	volatile int lock;
} counter_t;

static counter_t g_counter;

#if defined(_MSC_VER)
#include <intrin.h>
#define LOCK(counter) while (_InterlockedExchange((volatile long*)&(counter)->lock, 1)) {}
#define UNLOCK(counter) _InterlockedExchange((volatile long*)&(counter)->lock, 0)
#else
#define LOCK(counter) while (__sync_lock_test_and_set(&(counter)->lock, 1)) {}
#define UNLOCK(counter) __sync_lock_release(&(counter)->lock)
#endif

#define HEADER 16

static void* counted_alloc(size_t size, void* user_data) {
	counter_t* counter = (counter_t*)user_data;
	size_t* block = (size_t*)malloc(size + HEADER);
	if (!block) return NULL;
	block[0] = size;
	LOCK(counter);
	counter->blocks++;
	counter->bytes += size;
	counter->calls++;
	UNLOCK(counter);
	return (char*)block + HEADER;
}

static void counted_free(void* ptr, void* user_data) {
	counter_t* counter = (counter_t*)user_data;
	size_t* block;
	if (!ptr) return;
	block = (size_t*)((char*)ptr - HEADER);
	LOCK(counter);
	counter->blocks--;
	counter->bytes -= block[0];
	UNLOCK(counter);
	free(block);
}

static void* counted_realloc(void* ptr, size_t size, void* user_data) {
	counter_t* counter = (counter_t*)user_data;
	size_t* block;
	size_t old_size;
	if (!ptr) return counted_alloc(size, user_data);
	block = (size_t*)((char*)ptr - HEADER);
	old_size = block[0];
	block = (size_t*)realloc(block, size + HEADER);
	if (!block) return NULL;
	block[0] = size;
	LOCK(counter);
	counter->bytes += size;
	counter->bytes -= old_size;
	counter->calls++;
	UNLOCK(counter);
	return (char*)block + HEADER;
}

/* ---------------------------------------------------------------------------
 *  The window
 * ------------------------------------------------------------------------- */

static struct {
	Rid window, grid, ours, theirs, check;
	int custom;
} app;

static void refresh(Rid timer, void* user_data) {
	RMemoryStats stats;
	char text[256];
	(void)timer;
	(void)user_data;
	if (!Rwidget_is_valid(app.theirs)) return;
	Rmemory_get_stats(&stats);
	snprintf(text, sizeof(text), "Rmemory_get_stats: %u blocks, %.1f KB in use, peak %.1f KB, %.1f MB taken from the system",
		(unsigned)stats.allocation_count, (double)stats.used_bytes / 1024.0, (double)stats.peak_bytes / 1024.0,
		(double)stats.reserved_bytes / (1024.0 * 1024.0));
	Rlabel_set_text(app.theirs, text);
	if (app.custom) {
		size_t blocks, bytes, calls;
		LOCK(&g_counter);
		blocks = g_counter.blocks;
		bytes = g_counter.bytes;
		calls = g_counter.calls;
		UNLOCK(&g_counter);
		snprintf(text, sizeof(text), "Our allocator: %u blocks, %.1f KB, %u allocations so far",
			(unsigned)blocks, (double)bytes / 1024.0, (unsigned)calls);
	} else {
		snprintf(text, sizeof(text), "The allocator of Raven is in use (started with --raven).");
	}
	Rlabel_set_text(app.ours, text);
}

static void on_add(Rid button, void* user_data) {
	int i;
	(void)button;
	(void)user_data;
	for (i = 0; i < 200; i++) {
		char text[16];
		snprintf(text, sizeof(text), "%d", Rwidget_get_child_count(app.grid) + 1);
		Rwidget_add_child(app.grid, Rbutton_create(text));
	}
	refresh(R_INVALID_ID, NULL);
}

static void on_clear(Rid button, void* user_data) {
	(void)button;
	(void)user_data;
	Rwidget_clear_children(app.grid);
	/* gives back to the system the regions that are now empty */
	Rmemory_trim();
	refresh(R_INVALID_ID, NULL);
}

static void on_check(Rid button, void* user_data) {
	(void)button;
	(void)user_data;
	/* walks the heap of Raven and verifies every block; with another allocator there is nothing to walk */
	Rlabel_set_text(app.check, Rmemory_check() ? "Rmemory_check: the heap is consistent." : "Rmemory_check: the heap is damaged!");
}

static void example_shutdown(void) {
	Rshutdown();
	if (app.custom)
		printf("blocks still allocated after Rshutdown: %u\n", (unsigned)g_counter.blocks);
}

int main(int argc, char** argv) {
	RAllocator allocator;
	Rid box, row, scroll, button;

	app.custom = !(argc > 1 && strcmp(argv[1], "--raven") == 0);
	if (app.custom) {
		/* before Rinit, and before anything else of the library that allocates */
		allocator.alloc = counted_alloc;
		allocator.realloc = counted_realloc;
		allocator.free = counted_free;
		allocator.user_data = &g_counter;
		Rset_allocator(&allocator);
	}
	if (!example_init("memory")) return 1;

	app.window = Rwindow_create("Raven - memory", 720, 460, R_WINDOW_DEFAULT);
	box = Rwidget_create(R_WIDGET_VBOX);
	Rpanel_set_spacing(box, 10.0f);
	Rwindow_set_content(app.window, box);
	Rslot_set_padding(box, 16.0f, 16.0f, 16.0f, 16.0f);

	app.ours = Rwidget_add_child(box, Rlabel_create(""));
	app.theirs = Rwidget_add_child(box, Rlabel_create(""));
	app.check = Rwidget_add_child(box, Rlabel_create(""));
	Rwidget_set_opacity(app.check, 0.7f);

	row = Rwidget_add_child(box, Rwidget_create(R_WIDGET_HBOX));
	Rpanel_set_spacing(row, 8.0f);
	button = Rwidget_add_child(row, Rbutton_create("Add 200 buttons"));
	Rbutton_set_style(button, R_BUTTON_PRIMARY);
	Rbutton_set_clicked_callback(button, on_add, NULL);
	Rbutton_set_clicked_callback(Rwidget_add_child(row, Rbutton_create("Remove them")), on_clear, NULL);
	Rbutton_set_clicked_callback(Rwidget_add_child(row, Rbutton_create("Check the heap")), on_check, NULL);

	scroll = Rwidget_add_child(box, Rwidget_create(R_WIDGET_SCROLL_BOX));
	Rslot_set_fill(scroll, 1.0f);
	app.grid = Rwidget_add_child(scroll, Rwidget_create(R_WIDGET_WRAP_BOX));
	Rpanel_set_spacing(app.grid, 4.0f);

	on_add(R_INVALID_ID, NULL);
	Rtimer_create(0.5, R_TRUE, refresh, NULL);

	example_run(app.window, NULL, example_shutdown);
#ifdef __EMSCRIPTEN__
	return 0;
#else
	return app.custom && g_counter.blocks != 0 ? 1 : 0;
#endif
}

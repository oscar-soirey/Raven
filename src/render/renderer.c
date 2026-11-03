/**
 * Raven - render/renderer.c
 * Render thread and its command queue.
 */

#include "renderer.h"
#include "../core/context.h"

typedef enum rvn_rcmd_type_t {
	RVN_RCMD_INIT,
	RVN_RCMD_SHUTDOWN,
	RVN_RCMD_CREATE_SURFACE,
	RVN_RCMD_DESTROY_SURFACE,
	RVN_RCMD_CREATE_TEXTURE,
	RVN_RCMD_UPDATE_TEXTURE,
	RVN_RCMD_SET_TEXTURE_FILTER,
	RVN_RCMD_DESTROY_TEXTURE,
	RVN_RCMD_CREATE_SHADER,
	RVN_RCMD_DESTROY_SHADER,
	RVN_RCMD_FRAME,
	RVN_RCMD_CALL,
	RVN_RCMD_STOP
} rvn_rcmd_type_t;

/** Filled by the render thread for commands the UI thread waits for. */
typedef struct rvn_rcmd_wait_t {
	int done;
	int result;
} rvn_rcmd_wait_t;

typedef struct rvn_rcmd_t {
	rvn_rcmd_type_t type;
	Rid id;
	RNativeWindow native;
	RTextureDesc desc;
	RBackendInitInfo init;
	RFilterType filter;
	uint32_t x, y, width, height;
	void* pixels;
	rvn_frame_t* frame;
	rvn_render_fn function;
	void* arg;
	rvn_rcmd_wait_t* wait;
	struct rvn_rcmd_t* next;
} rvn_rcmd_t;

#define RVN_FRAME_POOL_MAX 6

static struct {
	RBackendVTable backend;
	int threaded;
	int running;
	rvn_thread_t thread;
	rvn_mutex_t mutex;
	/** Signalled when a command is queued. */
	rvn_cond_t queue_cv;
	/** Broadcast when a waited command is done. */
	rvn_cond_t done_cv;
	/** Broadcast when a frame has been rendered. */
	rvn_cond_t frame_cv;
	rvn_rcmd_t* head;
	rvn_rcmd_t* tail;
	rvn_frame_t* free_frames;
	int free_frame_count;
	/** Called while the UI thread waits for the render thread, or NULL. */
	void (*wait_service)(void);
	/** Set by the backend (any thread) when its device is lost; handled by the UI thread. */
	rvn_atomic_t device_lost;
} g_renderer;

Ruint Rbackend_get_caps(void) {
	return g_renderer.running ? g_renderer.backend.caps : 0;
}

/* ---------------------------------------------------------------------------
 *  Execution (render thread, or the UI thread when not threaded)
 * ------------------------------------------------------------------------- */

static void rvn_frame_release(rvn_frame_t* frame) {
	rvn_mutex_lock(&g_renderer.mutex);
	if (g_renderer.free_frame_count < RVN_FRAME_POOL_MAX) {
		frame->next = g_renderer.free_frames;
		g_renderer.free_frames = frame;
		g_renderer.free_frame_count++;
		frame = NULL;
	}
	rvn_mutex_unlock(&g_renderer.mutex);
	if (frame) {
		rvn_drawlist_free(&frame->list);
		rvn_free(frame);
	}
}

static int rvn_rcmd_execute(rvn_rcmd_t* cmd) {
	const RBackendVTable* backend = &g_renderer.backend;
	int result = R_TRUE;

	switch (cmd->type) {
		case RVN_RCMD_INIT:
			result = backend->Init(&cmd->init);
			break;
		case RVN_RCMD_SHUTDOWN:
			backend->WaitIdle();
			backend->Shutdown();
			break;
		case RVN_RCMD_CREATE_SURFACE:
			result = backend->CreateSurface(cmd->id, &cmd->native, cmd->width, cmd->height);
			break;
		case RVN_RCMD_DESTROY_SURFACE:
			backend->DestroySurface(cmd->id);
			break;
		case RVN_RCMD_CREATE_TEXTURE:
			result = backend->CreateTexture(cmd->id, &cmd->desc, cmd->pixels);
			rvn_free(cmd->pixels);
			break;
		case RVN_RCMD_UPDATE_TEXTURE:
			backend->UpdateTexture(cmd->id, cmd->x, cmd->y, cmd->width, cmd->height, cmd->pixels);
			rvn_free(cmd->pixels);
			break;
		case RVN_RCMD_SET_TEXTURE_FILTER:
			backend->SetTextureFilter(cmd->id, cmd->filter);
			break;
		case RVN_RCMD_CREATE_SHADER:
			result = backend->CreateShader ? backend->CreateShader(cmd->id, (const uint32_t*)cmd->pixels, (size_t)cmd->width) : R_FALSE;
			rvn_free(cmd->pixels);
			cmd->pixels = NULL;
			break;
		case RVN_RCMD_DESTROY_SHADER:
			if (backend->DestroyShader) backend->DestroyShader(cmd->id);
			break;
		case RVN_RCMD_DESTROY_TEXTURE:
			backend->DestroyTexture(cmd->id);
			break;
		case RVN_RCMD_FRAME: {
			rvn_frame_t* frame = cmd->frame;
			double start = rvn_clock_seconds();
			backend->RenderFrame(&frame->data, frame->capture);
			if (frame->render_us) {
				rvn_atomic_store(frame->render_us, (int)((rvn_clock_seconds() - start) * 1e6));
			}
			if (frame->frames_in_flight) {
				rvn_atomic_add(frame->frames_in_flight, -1);
				rvn_mutex_lock(&g_renderer.mutex);
				rvn_cond_broadcast(&g_renderer.frame_cv);
				rvn_mutex_unlock(&g_renderer.mutex);
			}
			rvn_frame_release(frame);
			/* a continuously redrawing window is paced by this wake-up */
			rvn_wake();
			break;
		}
		case RVN_RCMD_CALL:
			if (cmd->function) cmd->function(cmd->arg);
			break;
		case RVN_RCMD_STOP:
			break;
	}
	return result;
}

static void rvn_render_thread(void* arg) {
	RVN_UNUSED(arg);
	for (;;) {
		rvn_rcmd_t* cmd;
		int stop, result;

		rvn_mutex_lock(&g_renderer.mutex);
		while (!g_renderer.head) rvn_cond_wait(&g_renderer.queue_cv, &g_renderer.mutex);
		cmd = g_renderer.head;
		g_renderer.head = cmd->next;
		if (!g_renderer.head) g_renderer.tail = NULL;
		rvn_mutex_unlock(&g_renderer.mutex);

		stop = cmd->type == RVN_RCMD_STOP;
		result = rvn_rcmd_execute(cmd);
		if (cmd->wait) {
			rvn_mutex_lock(&g_renderer.mutex);
			cmd->wait->result = result;
			cmd->wait->done = R_TRUE;
			rvn_cond_broadcast(&g_renderer.done_cv);
			rvn_mutex_unlock(&g_renderer.mutex);
		}
		rvn_free(cmd);
		if (stop) break;
	}
}

/*
 * Waits on a condition of the renderer (mutex held). On some window systems
 * the UI thread must keep answering the window system while it is blocked on
 * the render thread (see rvn_renderer_set_wait_service): the wait is then cut
 * into short slices with a call to the service in between. The caller loops
 * on its predicate, so returning early is harmless.
 */
static void rvn_renderer_wait(rvn_cond_t* cond) {
	if (!g_renderer.wait_service) {
		rvn_cond_wait(cond, &g_renderer.mutex);
		return;
	}
	if (rvn_cond_wait_timeout(cond, &g_renderer.mutex, 0.002)) return;
	rvn_mutex_unlock(&g_renderer.mutex);
	g_renderer.wait_service();
	rvn_mutex_lock(&g_renderer.mutex);
}

void rvn_renderer_set_wait_service(void (*service)(void)) {
	g_renderer.wait_service = service;
}

/**
 * Queues a command. With `wait`, blocks until it was executed and returns
 * its result. Takes ownership of `cmd`.
 */
static int rvn_rcmd_submit(rvn_rcmd_t* cmd, int wait) {
	rvn_rcmd_wait_t waiter;
	int result;

	if (!g_renderer.running) {
		/* renderer not started or already stopped: drop the command */
		if (cmd->type == RVN_RCMD_CREATE_TEXTURE || cmd->type == RVN_RCMD_UPDATE_TEXTURE || cmd->type == RVN_RCMD_CREATE_SHADER) rvn_free(cmd->pixels);
		if (cmd->type == RVN_RCMD_FRAME) {
			if (cmd->frame->frames_in_flight) rvn_atomic_add(cmd->frame->frames_in_flight, -1);
			rvn_drawlist_free(&cmd->frame->list);
			rvn_free(cmd->frame);
		}
		rvn_free(cmd);
		return R_FALSE;
	}

	if (!g_renderer.threaded) {
		result = rvn_rcmd_execute(cmd);
		rvn_free(cmd);
		return result;
	}

	waiter.done = R_FALSE;
	waiter.result = R_TRUE;
	cmd->wait = wait ? &waiter : NULL;
	cmd->next = NULL;

	rvn_mutex_lock(&g_renderer.mutex);
	if (g_renderer.tail) g_renderer.tail->next = cmd;
	else g_renderer.head = cmd;
	g_renderer.tail = cmd;
	rvn_cond_signal(&g_renderer.queue_cv);
	if (wait) {
		while (!waiter.done) rvn_renderer_wait(&g_renderer.done_cv);
	}
	rvn_mutex_unlock(&g_renderer.mutex);
	return waiter.result;
}

static rvn_rcmd_t* rvn_rcmd_new(rvn_rcmd_type_t type, Rid id) {
	rvn_rcmd_t* cmd = (rvn_rcmd_t*)rvn_calloc(sizeof(rvn_rcmd_t));
	cmd->type = type;
	cmd->id = id;
	return cmd;
}

/* ---------------------------------------------------------------------------
 *  Lifecycle
 * ------------------------------------------------------------------------- */

int rvn_renderer_init(const RBackendVTable* backend, const RBackendInitInfo* info, int threaded) {
	rvn_rcmd_t* cmd;
	int ok;

	memset(&g_renderer, 0, sizeof(g_renderer));
	g_renderer.backend = *backend;
	g_renderer.threaded = threaded;
	rvn_mutex_init(&g_renderer.mutex);
	rvn_cond_init(&g_renderer.queue_cv);
	rvn_cond_init(&g_renderer.done_cv);
	rvn_cond_init(&g_renderer.frame_cv);

	if (threaded && !rvn_thread_create(&g_renderer.thread, rvn_render_thread, NULL, "raven-render")) {
		rvn_error(R_ERROR_PLATFORM, R_SEVERITY_WARNING, "cannot start the render thread, rendering on the UI thread");
		g_renderer.threaded = R_FALSE;
	}
	g_renderer.running = R_TRUE;

	/* the backend is initialized on the thread that will use it */
	cmd = rvn_rcmd_new(RVN_RCMD_INIT, R_INVALID_ID);
	cmd->init = *info;
	ok = rvn_rcmd_submit(cmd, R_TRUE);
	if (!ok) {
		if (g_renderer.threaded) {
			rvn_rcmd_submit(rvn_rcmd_new(RVN_RCMD_STOP, R_INVALID_ID), R_FALSE);
			rvn_thread_join(&g_renderer.thread);
		}
		g_renderer.running = R_FALSE;
		rvn_cond_destroy(&g_renderer.frame_cv);
		rvn_cond_destroy(&g_renderer.done_cv);
		rvn_cond_destroy(&g_renderer.queue_cv);
		rvn_mutex_destroy(&g_renderer.mutex);
	}
	return ok;
}

void rvn_renderer_shutdown(void) {
	rvn_frame_t* frame;
	if (!g_renderer.running) return;
	rvn_rcmd_submit(rvn_rcmd_new(RVN_RCMD_SHUTDOWN, R_INVALID_ID), R_TRUE);
	if (g_renderer.threaded) {
		rvn_rcmd_submit(rvn_rcmd_new(RVN_RCMD_STOP, R_INVALID_ID), R_FALSE);
		rvn_thread_join(&g_renderer.thread);
	}
	g_renderer.running = R_FALSE;
	frame = g_renderer.free_frames;
	while (frame) {
		rvn_frame_t* next = frame->next;
		rvn_drawlist_free(&frame->list);
		rvn_free(frame);
		frame = next;
	}
	rvn_cond_destroy(&g_renderer.frame_cv);
	rvn_cond_destroy(&g_renderer.done_cv);
	rvn_cond_destroy(&g_renderer.queue_cv);
	rvn_mutex_destroy(&g_renderer.mutex);
	memset(&g_renderer, 0, sizeof(g_renderer));
}

int rvn_renderer_is_threaded(void) {
	return g_renderer.threaded;
}

/* ---------------------------------------------------------------------------
 *  Commands
 * ------------------------------------------------------------------------- */

int rvn_renderer_create_surface(Rid window, const RNativeWindow* native, uint32_t width, uint32_t height) {
	rvn_rcmd_t* cmd = rvn_rcmd_new(RVN_RCMD_CREATE_SURFACE, window);
	cmd->native = *native;
	cmd->width = width;
	cmd->height = height;
	return rvn_rcmd_submit(cmd, R_TRUE);
}

void rvn_renderer_destroy_surface(Rid window) {
	rvn_rcmd_submit(rvn_rcmd_new(RVN_RCMD_DESTROY_SURFACE, window), R_TRUE);
}

void rvn_renderer_create_texture(Rid texture, const RTextureDesc* desc, void* pixels) {
	rvn_rcmd_t* cmd = rvn_rcmd_new(RVN_RCMD_CREATE_TEXTURE, texture);
	cmd->desc = *desc;
	cmd->pixels = pixels;
	rvn_rcmd_submit(cmd, R_FALSE);
}

void rvn_renderer_update_texture(Rid texture, uint32_t x, uint32_t y, uint32_t width, uint32_t height, void* pixels) {
	rvn_rcmd_t* cmd = rvn_rcmd_new(RVN_RCMD_UPDATE_TEXTURE, texture);
	cmd->x = x;
	cmd->y = y;
	cmd->width = width;
	cmd->height = height;
	cmd->pixels = pixels;
	rvn_rcmd_submit(cmd, R_FALSE);
}

void rvn_renderer_set_texture_filter(Rid texture, RFilterType filter) {
	rvn_rcmd_t* cmd = rvn_rcmd_new(RVN_RCMD_SET_TEXTURE_FILTER, texture);
	cmd->filter = filter;
	rvn_rcmd_submit(cmd, R_FALSE);
}

void rvn_renderer_destroy_texture(Rid texture) {
	rvn_rcmd_submit(rvn_rcmd_new(RVN_RCMD_DESTROY_TEXTURE, texture), R_FALSE);
}

void rvn_renderer_create_shader(Rid shader, uint32_t* spirv, size_t size) {
	rvn_rcmd_t* cmd = rvn_rcmd_new(RVN_RCMD_CREATE_SHADER, shader);
	cmd->pixels = spirv;
	cmd->width = (uint32_t)size;
	rvn_rcmd_submit(cmd, R_FALSE);
}

void rvn_renderer_destroy_shader(Rid shader) {
	rvn_rcmd_submit(rvn_rcmd_new(RVN_RCMD_DESTROY_SHADER, shader), R_FALSE);
}

/* ---------------------------------------------------------------------------
 *  Loss of the device
 * ------------------------------------------------------------------------- */

void Rbackend_device_lost(void) {
	if (!g_renderer.running) return;
	if (rvn_atomic_exchange(&g_renderer.device_lost, 1) == 0) {
		/* the UI thread may be waiting for events: it recovers at its next update */
		rvn_wake();
	}
}

int rvn_renderer_device_lost(void) {
	return g_renderer.running && rvn_atomic_load(&g_renderer.device_lost) != 0;
}

int rvn_renderer_restart(const RBackendInitInfo* info) {
	rvn_rcmd_t* cmd;
	if (!g_renderer.running) return R_FALSE;
	/* (a loss during the restart is seen again afterwards) */
	rvn_atomic_store(&g_renderer.device_lost, 0);
	rvn_rcmd_submit(rvn_rcmd_new(RVN_RCMD_SHUTDOWN, R_INVALID_ID), R_TRUE);
	cmd = rvn_rcmd_new(RVN_RCMD_INIT, R_INVALID_ID);
	cmd->init = *info;
	return rvn_rcmd_submit(cmd, R_TRUE);
}

void rvn_renderer_call(rvn_render_fn function, void* arg, int wait) {
	rvn_rcmd_t* cmd = rvn_rcmd_new(RVN_RCMD_CALL, R_INVALID_ID);
	cmd->function = function;
	cmd->arg = arg;
	rvn_rcmd_submit(cmd, wait);
}

void rvn_renderer_sync(void) {
	if (g_renderer.running && g_renderer.threaded) rvn_renderer_call(NULL, NULL, R_TRUE);
}

/* ---------------------------------------------------------------------------
 *  Frames
 * ------------------------------------------------------------------------- */

rvn_frame_t* rvn_renderer_acquire_frame(void) {
	rvn_frame_t* frame = NULL;
	if (g_renderer.running) {
		rvn_mutex_lock(&g_renderer.mutex);
		frame = g_renderer.free_frames;
		if (frame) {
			g_renderer.free_frames = frame->next;
			g_renderer.free_frame_count--;
		}
		rvn_mutex_unlock(&g_renderer.mutex);
	}
	if (!frame) {
		frame = (rvn_frame_t*)rvn_calloc(sizeof(rvn_frame_t));
		rvn_drawlist_init(&frame->list);
	}
	rvn_drawlist_reset(&frame->list);
	memset(&frame->data, 0, sizeof(frame->data));
	frame->capture = NULL;
	frame->frames_in_flight = NULL;
	frame->render_us = NULL;
	frame->next = NULL;
	return frame;
}

void rvn_renderer_throttle(rvn_atomic_t* frames_in_flight, int max_frames) {
	if (!g_renderer.running || !g_renderer.threaded) return;
	rvn_mutex_lock(&g_renderer.mutex);
	while (rvn_atomic_load(frames_in_flight) >= max_frames) rvn_renderer_wait(&g_renderer.frame_cv);
	rvn_mutex_unlock(&g_renderer.mutex);
}

void rvn_renderer_submit_frame(rvn_frame_t* frame) {
	rvn_rcmd_t* cmd = rvn_rcmd_new(RVN_RCMD_FRAME, frame->data.window);
	frame->data.instances = frame->list.instances;
	frame->data.instance_count = frame->list.instance_count;
	frame->data.commands = frame->list.commands;
	frame->data.command_count = frame->list.command_count;
	if (frame->frames_in_flight) rvn_atomic_add(frame->frames_in_flight, 1);
	cmd->frame = frame;
	rvn_rcmd_submit(cmd, R_FALSE);
}

/**
 * Raven - core/context.c
 * Library lifecycle, main loop, posted tasks, timers, actions, clipboard.
 */

#include "context.h"
#include "heap.h"
#include "fs_watch.h"
#include "../widgets/common.h"
#include "../render/renderer.h"

rvn_context_t g_rvn;

static RBackendVTable g_custom_backend;
static int g_custom_backend_set;

void rvn_tasks_run(void);

/* ---------------------------------------------------------------------------
 *  Guards
 * ------------------------------------------------------------------------- */

int rvn_api_check(const char* function) {
	if (!g_rvn.initialized) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "%s: Raven is not initialized", function);
		return R_FALSE;
	}
	if (rvn_thread_id() != g_rvn.ui_thread) {
		rvn_error(R_ERROR_WRONG_THREAD, R_SEVERITY_ERROR,
			"%s must be called from the UI thread (use Rtask_post from other threads)", function);
		return R_FALSE;
	}
	return R_TRUE;
}

void rvn_dispatch_begin(void) {
	g_rvn.dispatch_depth++;
}

void rvn_dispatch_end(void) {
	if (--g_rvn.dispatch_depth > 0) return;
	g_rvn.dispatch_depth = 0;
	/* nobody up the stack holds pointers any more: release what was deleted */
	if (g_rvn.dead_widget_count) rvn_widgets_collect();
	if (g_rvn.dead_window_count) rvn_windows_collect();
}

void rvn_wake(void) {
	if (g_rvn.initialized && g_rvn.platform) g_rvn.platform->wake();
}

/* ---------------------------------------------------------------------------
 *  Initialization
 * ------------------------------------------------------------------------- */

void Rbackend_register(const RBackendVTable* vtable) {
	if (g_rvn.initialized) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rbackend_register must be called before Rinit");
		return;
	}
	if (!vtable || !vtable->Init || !vtable->Shutdown || !vtable->CreateSurface || !vtable->DestroySurface ||
		!vtable->CreateTexture || !vtable->UpdateTexture || !vtable->SetTextureFilter || !vtable->DestroyTexture ||
		!vtable->RenderFrame || !vtable->WaitIdle) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rbackend_register: every function of the table is required");
		return;
	}
	g_custom_backend = *vtable;
	g_custom_backend_set = R_TRUE;
}

static const RBackendVTable* rvn_select_backend(RBackend backend) {
	switch (backend) {
		case R_BACKEND_VULKAN:
#if defined(RAVEN_BACKEND_VULKAN_ENABLED)
			return rvn_backend_vulkan();
#else
			rvn_error(R_ERROR_BACKEND, R_SEVERITY_FATAL, "Raven was built without the Vulkan backend");
			return NULL;
#endif
		case R_BACKEND_NULL:
			return rvn_backend_null();
		case R_BACKEND_CUSTOM:
			if (!g_custom_backend_set) {
				rvn_error(R_ERROR_BACKEND, R_SEVERITY_FATAL, "R_BACKEND_CUSTOM needs Rbackend_register first");
				return NULL;
			}
			return &g_custom_backend;
	}
	rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_FATAL, "Rinit: unknown backend");
	return NULL;
}

int Rinit(RBackend backend) {
	RInitDesc desc;
	memset(&desc, 0, sizeof(desc));
	desc.backend = backend;
	return Rinit_ex(&desc);
}

int Rinit_ex(const RInitDesc* user_desc) {
	RInitDesc desc;
	RBackendInitInfo info;
	const RBackendVTable* backend;
	int workers, threaded;

	if (g_rvn.initialized) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_WARNING, "Rinit: already initialized");
		return R_FALSE;
	}
	memset(&desc, 0, sizeof(desc));
	if (user_desc) desc = *user_desc;
	if (desc.backend == 0) desc.backend = R_BACKEND_VULKAN;

	backend = rvn_select_backend(desc.backend);
	if (!backend) return R_FALSE;

	memset(&g_rvn, 0, sizeof(g_rvn));
	g_rvn.ui_thread = rvn_thread_id();
	g_rvn.start_time = rvn_clock_seconds();
	g_rvn.ui_scale = desc.ui_scale > 0.0f ? desc.ui_scale : 1.0f;
	g_rvn.vsync = !desc.disable_vsync;
	g_rvn.backend = desc.backend;
	g_rvn.default_font = R_INVALID_ID;
	rvn_pool_init(&g_rvn.windows, RVN_HANDLE_WINDOW);
	rvn_pool_init(&g_rvn.widgets, RVN_HANDLE_WIDGET);
	rvn_pool_init(&g_rvn.fonts, RVN_HANDLE_FONT);
	rvn_pool_init(&g_rvn.textures, RVN_HANDLE_TEXTURE);
	rvn_pool_init(&g_rvn.shaders, RVN_HANDLE_SHADER);
	rvn_pool_init(&g_rvn.timers, RVN_HANDLE_TIMER);
	rvn_pool_init(&g_rvn.tree_items, RVN_HANDLE_TREE_ITEM);
	rvn_pool_init(&g_rvn.xml_documents, RVN_HANDLE_XML_DOCUMENT);
	rvn_pool_init(&g_rvn.xml_nodes, RVN_HANDLE_XML_NODE);
	rvn_mutex_init(&g_rvn.task_mutex);

	/* windowing */
	g_rvn.platform = rvn_platform_select(desc.headless);
	if (!g_rvn.platform || !g_rvn.platform->init(rvn_window_on_platform_event, rvn_window_region_at)) {
		rvn_error(R_ERROR_PLATFORM, R_SEVERITY_FATAL, "Rinit: cannot initialize the %s window system",
			g_rvn.platform ? g_rvn.platform->name : "native");
		goto fail_platform;
	}

	/* job system: leave one core to the UI thread and one to the render thread */
	threaded = !desc.disable_render_thread;
	if (desc.worker_threads < 0) {
		workers = 0;
	} else if (desc.worker_threads > 0) {
		workers = desc.worker_threads;
	} else {
		workers = rvn_cpu_count() - 1 - (threaded ? 1 : 0);
		workers = rvn_clampi(workers, 1, 16);
	}
	rvn_jobs_init(workers);

	/* rendering */
	memset(&info, 0, sizeof(info));
	g_rvn.app_name = rvn_strdup(desc.app_name ? desc.app_name : "Raven application");
	g_rvn.enable_validation = desc.enable_validation;
	info.app_name = g_rvn.app_name;
	info.enable_validation = desc.enable_validation;
	info.vsync = g_rvn.vsync;
	info.window_system = g_rvn.platform->system;
	info.native_display = g_rvn.platform->native_display();
	if (!rvn_renderer_init(backend, &info, threaded)) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_FATAL, "Rinit: cannot initialize the %s backend", backend->name);
		goto fail_renderer;
	}
	rvn_renderer_set_wait_service(g_rvn.platform->wait_service);

	g_rvn.initialized = R_TRUE;
	rvn_paint_init();
	rvn_themes_init();
	rvn_theme_apply(R_THEME_DARK);
	rvn_fonts_init();
	g_rvn.theme.font = g_rvn.default_font;
	g_rvn.last_update_time = rvn_clock_seconds();
	return R_TRUE;

fail_renderer:
	rvn_free(g_rvn.app_name);
	g_rvn.app_name = NULL;
	rvn_jobs_shutdown();
	g_rvn.platform->shutdown();
fail_platform:
	rvn_mutex_destroy(&g_rvn.task_mutex);
	rvn_pool_free(&g_rvn.windows);
	rvn_pool_free(&g_rvn.widgets);
	rvn_pool_free(&g_rvn.fonts);
	rvn_pool_free(&g_rvn.textures);
	rvn_pool_free(&g_rvn.shaders);
	rvn_pool_free(&g_rvn.timers);
	rvn_pool_free(&g_rvn.tree_items);
	rvn_pool_free(&g_rvn.xml_documents);
	rvn_pool_free(&g_rvn.xml_nodes);
	memset(&g_rvn, 0, sizeof(g_rvn));
	return R_FALSE;
}

void Rshutdown(void) {
	uint32_t i;
	if (!rvn_api_check(__func__)) return;

	/* Stop native filesystem notifications before draining posted UI tasks. */
	rvn_fs_watch_stop();
	/* windows first: this deletes every widget that is shown */
	rvn_windows_destroy_all();
	/* then the widgets the application created but never attached */
	for (i = 0; i < g_rvn.widgets.used; i++) {
		rvn_widget_t* widget = (rvn_widget_t*)rvn_pool_at(&g_rvn.widgets, i);
		/* internal widgets are destroyed by the widget that owns them */
		if (widget && !widget->parent && !(widget->flags & RVN_WF_INTERNAL)) rvn_widget_destroy(widget);
	}
	rvn_widgets_collect();

	/* let the running jobs finish and deliver their results */
	rvn_jobs_shutdown();
	rvn_tasks_run();

	for (i = 0; i < g_rvn.timers.used; i++) {
		rvn_timer_t* timer = (rvn_timer_t*)rvn_pool_at(&g_rvn.timers, i);
		if (timer) {
			rvn_pool_remove(&g_rvn.timers, timer->id);
			rvn_free(timer);
		}
	}

	rvn_xml_shutdown();
	rvn_locale_shutdown();
	rvn_a11y_shutdown();
	rvn_themes_shutdown();
	rvn_look_shutdown();
	rvn_dialogs_shutdown();
	rvn_islands_shutdown();
	rvn_languages_shutdown();
	rvn_fonts_shutdown();
	rvn_shaders_shutdown();
	rvn_textures_shutdown();
	rvn_paint_shutdown();
	rvn_memory_files_shutdown();
	rvn_renderer_shutdown();
	g_rvn.platform->shutdown();

	for (i = 0; i < g_rvn.action_count; i++) rvn_free(g_rvn.actions[i].name);
	rvn_free(g_rvn.actions);
	rvn_free(g_rvn.tasks);
	rvn_free(g_rvn.dead_widgets);
	rvn_free(g_rvn.dead_windows);
	rvn_free(g_rvn.clipboard);
	rvn_free(g_rvn.xml_base_path);
	rvn_free(g_rvn.app_name);
	rvn_mutex_destroy(&g_rvn.task_mutex);
	rvn_pool_free(&g_rvn.windows);
	rvn_pool_free(&g_rvn.widgets);
	rvn_pool_free(&g_rvn.fonts);
	rvn_pool_free(&g_rvn.textures);
	rvn_pool_free(&g_rvn.shaders);
	rvn_pool_free(&g_rvn.timers);
	rvn_pool_free(&g_rvn.tree_items);
	rvn_pool_free(&g_rvn.xml_documents);
	rvn_pool_free(&g_rvn.xml_nodes);
	memset(&g_rvn, 0, sizeof(g_rvn));
	/* nothing is allocated any more: the regions of the heap return to the OS */
	rvn_heap_trim();
}

int Ris_initialized(void) {
	return g_rvn.initialized;
}

const char* Rget_version(void) {
	return RVN_VERSION_STRING;
}

/* ---------------------------------------------------------------------------
 *  Tasks
 * ------------------------------------------------------------------------- */

void Rtask_post(RTaskCallback task, void* user_data) {
	if (!task) return;
	if (!g_rvn.initialized) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rtask_post: Raven is not initialized");
		return;
	}
	rvn_mutex_lock(&g_rvn.task_mutex);
	RVN_ARRAY_RESERVE(g_rvn.tasks, g_rvn.task_cap, g_rvn.task_count + 1);
	g_rvn.tasks[g_rvn.task_count].function = task;
	g_rvn.tasks[g_rvn.task_count].user_data = user_data;
	g_rvn.task_count++;
	rvn_mutex_unlock(&g_rvn.task_mutex);
	rvn_wake();
}

void rvn_tasks_run(void) {
	rvn_task_t* tasks;
	uint32_t count, i;

	/* take the whole batch: tasks posted while running wait for the next update */
	rvn_mutex_lock(&g_rvn.task_mutex);
	count = g_rvn.task_count;
	tasks = NULL;
	if (count) {
		tasks = (rvn_task_t*)rvn_alloc(sizeof(rvn_task_t) * count);
		memcpy(tasks, g_rvn.tasks, sizeof(rvn_task_t) * count);
		g_rvn.task_count = 0;
	}
	rvn_mutex_unlock(&g_rvn.task_mutex);

	if (!count) return;
	rvn_dispatch_begin();
	for (i = 0; i < count; i++) tasks[i].function(tasks[i].user_data);
	rvn_dispatch_end();
	rvn_free(tasks);
}

static int rvn_tasks_pending(void) {
	int pending;
	rvn_mutex_lock(&g_rvn.task_mutex);
	pending = g_rvn.task_count > 0;
	rvn_mutex_unlock(&g_rvn.task_mutex);
	return pending;
}

void Rtask_run_async(RTaskCallback task, void* user_data) {
	if (!task) return;
	if (!g_rvn.initialized) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rtask_run_async: Raven is not initialized");
		return;
	}
	rvn_jobs_submit(NULL, task, user_data);
}

void Rparallel_for(Ruint count, Ruint grain, RParallelRangeCallback function, void* user_data) {
	if (!function) return;
	if (!g_rvn.initialized) {
		function(0, count, user_data);
		return;
	}
	rvn_jobs_parallel_for(count, grain, function, user_data);
}

void Rwake_up(void) {
	rvn_wake();
}

int Rget_worker_thread_count(void) {
	return g_rvn.initialized ? rvn_jobs_worker_count() : 0;
}

int Ris_ui_thread(void) {
	return g_rvn.initialized && rvn_thread_id() == g_rvn.ui_thread;
}

/* ---------------------------------------------------------------------------
 *  Timers
 * ------------------------------------------------------------------------- */

Rid Rtimer_create(double interval_seconds, int repeat, RTimerCallback callback, void* user_data) {
	rvn_timer_t* timer;
	RVN_API_GUARD(R_INVALID_ID);
	if (!callback || interval_seconds < 0.0) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rtimer_create: invalid arguments");
		return R_INVALID_ID;
	}
	timer = (rvn_timer_t*)rvn_calloc(sizeof(rvn_timer_t));
	/* a repeating timer of zero seconds would spin the loop */
	timer->interval = repeat && interval_seconds < 0.001 ? 0.001 : interval_seconds;
	timer->deadline = rvn_clock_seconds() + timer->interval;
	timer->repeat = repeat != 0;
	timer->callback = callback;
	timer->user_data = user_data;
	timer->id = rvn_pool_add(&g_rvn.timers, timer);
	if (timer->id == R_INVALID_ID) {
		rvn_free(timer);
		return R_INVALID_ID;
	}
	return timer->id;
}

void Rtimer_destroy(Rid id) {
	rvn_timer_t* timer;
	RVN_API_GUARD_VOID();
	timer = (rvn_timer_t*)rvn_pool_get(&g_rvn.timers, id);
	if (!timer) {
		rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "Rtimer_destroy: invalid timer");
		return;
	}
	rvn_pool_remove(&g_rvn.timers, id);
	rvn_free(timer);
}

int Rtimer_is_valid(Rid timer) {
	return g_rvn.initialized && rvn_pool_get(&g_rvn.timers, timer) != NULL;
}

double rvn_timers_next_delay(void) {
	double now = rvn_clock_seconds(), best = -1.0;
	uint32_t i;
	for (i = 0; i < g_rvn.timers.used; i++) {
		rvn_timer_t* timer = (rvn_timer_t*)rvn_pool_at(&g_rvn.timers, i);
		double delay;
		if (!timer) continue;
		delay = timer->deadline - now;
		if (delay < 0.0) delay = 0.0;
		if (best < 0.0 || delay < best) best = delay;
	}
	return best;
}

static void rvn_timers_run(void) {
	double now = rvn_clock_seconds();
	uint32_t i, used = g_rvn.timers.used;

	rvn_dispatch_begin();
	for (i = 0; i < used; i++) {
		rvn_timer_t* timer = (rvn_timer_t*)rvn_pool_at(&g_rvn.timers, i);
		Rid id;
		RTimerCallback callback;
		void* user_data;
		if (!timer || timer->deadline > now) continue;
		id = timer->id;
		callback = timer->callback;
		user_data = timer->user_data;
		if (timer->repeat) {
			timer->deadline += timer->interval;
			/* do not try to catch up after a long stall */
			if (timer->deadline < now) timer->deadline = now + timer->interval;
		} else {
			rvn_pool_remove(&g_rvn.timers, id);
			rvn_free(timer);
		}
		callback(id, user_data);
	}
	rvn_dispatch_end();
}

/* ---------------------------------------------------------------------------
 *  Actions
 * ------------------------------------------------------------------------- */

static rvn_action_t* rvn_action_find(const char* name) {
	uint32_t i;
	for (i = 0; i < g_rvn.action_count; i++) {
		if (strcmp(g_rvn.actions[i].name, name) == 0) return &g_rvn.actions[i];
	}
	return NULL;
}

void Raction_bind(const char* name, RActionCallback callback, void* user_data) {
	rvn_action_t* action;
	RVN_API_GUARD_VOID();
	if (!name || !name[0]) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Raction_bind: empty action name");
		return;
	}
	if (!callback) {
		Raction_unbind(name);
		return;
	}
	action = rvn_action_find(name);
	if (!action) {
		RVN_ARRAY_RESERVE(g_rvn.actions, g_rvn.action_cap, g_rvn.action_count + 1);
		action = &g_rvn.actions[g_rvn.action_count++];
		action->name = rvn_strdup(name);
	}
	action->callback = callback;
	action->user_data = user_data;
}

void Raction_unbind(const char* name) {
	rvn_action_t* action;
	RVN_API_GUARD_VOID();
	if (!name) return;
	action = rvn_action_find(name);
	if (!action) return;
	rvn_free(action->name);
	*action = g_rvn.actions[--g_rvn.action_count];
}

void Raction_fire(const char* name, Rid sender) {
	char* copy;
	RVN_API_GUARD_VOID();
	if (!name || !name[0]) return;
	/* (the function may unbind the action, or free what the name was read from) */
	copy = rvn_strdup(name);
	rvn_fire_action(sender, copy);
	rvn_free(copy);
}

void Raction_set_default_handler(RActionCallback callback, void* user_data) {
	RVN_API_GUARD_VOID();
	g_rvn.default_action = callback;
	g_rvn.default_action_user = user_data;
}

void rvn_fire_action(Rid sender, const char* name) {
	rvn_action_t* action;
	if (!name || !name[0]) return;
	action = rvn_action_find(name);
	if (action) {
		/* copy: the callback may bind or unbind actions */
		RActionCallback callback = action->callback;
		void* user_data = action->user_data;
		callback(sender, name, user_data);
	} else if (g_rvn.default_action) {
		g_rvn.default_action(sender, name, g_rvn.default_action_user);
	}
}

/* ---------------------------------------------------------------------------
 *  Clipboard
 * ------------------------------------------------------------------------- */

void Rclipboard_set_text(const char* text) {
	RVN_API_GUARD_VOID();
	g_rvn.platform->clipboard_set(text ? text : "");
}

const char* Rclipboard_get_text(void) {
	RVN_API_GUARD("");
	rvn_free(g_rvn.clipboard);
	g_rvn.clipboard = g_rvn.platform->clipboard_get();
	return g_rvn.clipboard ? g_rvn.clipboard : "";
}

/* ---------------------------------------------------------------------------
 *  Main loop
 * ------------------------------------------------------------------------- */

void Rquit(int exit_code) {
	if (!g_rvn.initialized) return;
	rvn_atomic_store(&g_rvn.exit_code, exit_code);
	rvn_atomic_store(&g_rvn.quit_requested, 1);
	rvn_wake();
}

int Ris_running(void) {
	uint32_t i;
	if (!g_rvn.initialized || rvn_atomic_load(&g_rvn.quit_requested)) return R_FALSE;
	if (!g_rvn.had_window) return R_TRUE;
	for (i = 0; i < g_rvn.windows.used; i++) {
		rvn_window_t* window = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
		if (window && !window->dead) return R_TRUE;
	}
	return R_FALSE;
}

void Rset_device_reset_callback(RDeviceResetCallback callback, void* user_data) {
	RVN_API_GUARD_VOID();
	g_rvn.device_reset_callback = callback;
	g_rvn.device_reset_user = user_data;
}

void Rset_frame_callback(RFrameCallback callback, void* user_data) {
	RVN_API_GUARD_VOID();
	g_rvn.frame_callback = callback;
	g_rvn.frame_user = user_data;
}

double Rget_time(void) {
	return g_rvn.initialized ? rvn_clock_seconds() - g_rvn.start_time : 0.0;
}

void Rpoll_events(void) {
	RVN_API_GUARD_VOID();
	g_rvn.platform->pump(0.0);
}

void Rwait_events(double timeout_seconds) {
	uint32_t i;
	double timer_delay;
	RVN_API_GUARD_VOID();

	/* do not sleep when there is already something to do */
	if (rvn_tasks_pending() || rvn_atomic_load(&g_rvn.redraw_all) || rvn_atomic_load(&g_rvn.quit_requested)) {
		timeout_seconds = 0.0;
	}
	for (i = 0; i < g_rvn.windows.used && timeout_seconds != 0.0; i++) {
		rvn_window_t* window = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
		if (window && rvn_window_wants_frame(window)) timeout_seconds = 0.0;
	}
	timer_delay = rvn_timers_next_delay();
	if (timer_delay >= 0.0 && (timeout_seconds < 0.0 || timer_delay < timeout_seconds)) timeout_seconds = timer_delay;
	g_rvn.platform->pump(timeout_seconds);
}

void rvn_update(void) {
	double now;
	uint32_t i;
	/* a platform modal loop may call back while an update is already running */
	if (g_rvn.in_update) return;
	g_rvn.in_update = R_TRUE;

	rvn_tasks_run();
	rvn_timers_run();
	/* the GPU device was lost: everything of the backend is made again first */
	rvn_device_check();

	now = rvn_clock_seconds();
	g_rvn.delta_time = now - g_rvn.last_update_time;
	if (g_rvn.frame_callback) {
		rvn_dispatch_begin();
		g_rvn.frame_callback(g_rvn.delta_time, g_rvn.frame_user);
		rvn_dispatch_end();
	}
	g_rvn.last_update_time = now;

	if (rvn_atomic_exchange(&g_rvn.redraw_all, 0)) rvn_repaint_all_windows();
	for (i = 0; i < g_rvn.windows.used; i++) {
		rvn_window_t* window = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
		if (window) rvn_window_update(window);
	}
	g_rvn.in_update = R_FALSE;
}

int Rdraw_frame(void) {
	RVN_API_GUARD(R_FALSE);
	rvn_windows_throttle();
	g_rvn.platform->pump(0.0);
	rvn_update();
	return Ris_running();
}

int Rwait_frame(double timeout_seconds) {
	RVN_API_GUARD(R_FALSE);
	Rwait_events(timeout_seconds);
	/* a frame is coming: wait for the renderer first, then read the input that came meanwhile */
	rvn_windows_throttle();
	g_rvn.platform->pump(0.0);
	rvn_update();
	return Ris_running();
}

int Rget_exit_code(void) {
	return g_rvn.initialized ? rvn_atomic_load(&g_rvn.exit_code) : 0;
}

double Rget_delta_time(void) {
	return g_rvn.initialized ? g_rvn.delta_time : 0.0;
}

/**
 * Raven - core/context.c
 * Library lifecycle, main loop, posted tasks, timers, actions, clipboard.
 */

#include "context.h"
#include "heap.h"
#include "fs_watch.h"
#include "transfer.h"
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
#if defined(RAVEN_PLATFORM_EMSCRIPTEN)
	rvn_web_loop_poke();
#endif
	return R_TRUE;
}

void rvn_dispatch_begin(void) {
	g_rvn.dispatch_depth++;
}

void rvn_dispatch_end(void) {
	if (--g_rvn.dispatch_depth > 0) return;
	g_rvn.dispatch_depth = 0;
	/* nobody up the stack holds pointers any more: release what was deleted */
	rvn_collect_dead();
}

void rvn_collect_dead(void) {
	/* (freeing a widget can close a window, and a window going takes widgets with it) */
	while (g_rvn.dead_widget_count || g_rvn.dead_window_count) {
		rvn_widgets_collect();
		rvn_windows_collect();
	}
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
		        case R_BACKEND_WEBGL:
#if defined(RAVEN_BACKEND_WEBGL_ENABLED)
            return rvn_backend_webgl();
#else
            rvn_error(R_ERROR_BACKEND, R_SEVERITY_FATAL,
                      "Raven was built without the WebGL backend");
            return NULL;
#endif
		case R_BACKEND_METAL:
#if defined(RAVEN_BACKEND_METAL_ENABLED)
			return rvn_backend_metal();
#else
			rvn_error(R_ERROR_BACKEND, R_SEVERITY_FATAL, "Raven was built without the Metal backend");
			return NULL;
#endif
        case R_BACKEND_NULL:
			return rvn_backend_null();
		case R_BACKEND_DEFAULT:
			/* (resolved by Rinit_ex before) */
			break;
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

/* A run driven by the environment without a GPU that works (a Linux machine without a Vulkan driver): the tree still can be read. */
static int rvn_init_null_fallback(const RBackendVTable** backend, RBackendInitInfo* info, int threaded) {
	fprintf(stderr, "raven: the %s backend cannot start: the run goes on with the null backend (the tree, no pictures)\n", (*backend)->name);
	*backend = rvn_backend_null();
	g_rvn.backend = R_BACKEND_NULL;
	info->vsync = rvn_present_vsync();
	return rvn_renderer_init(*backend, info, threaded);
}

int Rinit_ex(const RInitDesc* user_desc) {
	RInitDesc desc;
	RBackendInitInfo info;
	const RBackendVTable* backend;
	int workers, threaded, driven;

	if (g_rvn.initialized) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_WARNING, "Rinit: already initialized");
		return R_FALSE;
	}
	memset(&desc, 0, sizeof(desc));
	if (user_desc) desc = *user_desc;
	/* RAVEN_HEADLESS, RAVEN_SCRIPT...: the application is driven from outside (widget/headless.c) */
	driven = rvn_headless_configure(&desc);
	if (desc.backend == R_BACKEND_DEFAULT) {
#if defined(RAVEN_BACKEND_WEBGL_ENABLED)
        desc.backend = R_BACKEND_WEBGL;
#elif defined(RAVEN_BACKEND_METAL_ENABLED)
		desc.backend = R_BACKEND_METAL;
#else
        desc.backend = R_BACKEND_VULKAN;
#endif
    }
#if defined(__EMSCRIPTEN__)
    /* DOM and WebGL contexts belong to the browser main thread. */
    desc.disable_render_thread = R_TRUE;
#if !defined(__EMSCRIPTEN_PTHREADS__)
    desc.worker_threads = -1;
#endif
#endif

	backend = rvn_select_backend(desc.backend);
	if (!backend && driven && desc.backend != R_BACKEND_NULL && rvn_headless_allows_null_fallback()) {
		fprintf(stderr, "raven: no GPU backend: the run goes on with the null backend (the tree, no pictures)\n");
		desc.backend = R_BACKEND_NULL;
		backend = rvn_select_backend(desc.backend);
	}
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
	info.vsync = rvn_present_vsync();
	info.window_system = g_rvn.platform->system;
	info.native_display = g_rvn.platform->native_display();
	if (!rvn_renderer_init(backend, &info, threaded) &&
		!(driven && desc.backend != R_BACKEND_NULL && rvn_headless_allows_null_fallback() && rvn_init_null_fallback(&backend, &info, threaded))) {
		rvn_error(R_ERROR_BACKEND, R_SEVERITY_FATAL, "Rinit: cannot initialize the %s backend", backend->name);
		goto fail_renderer;
	}
	rvn_renderer_set_wait_service(g_rvn.platform->wait_service);

	g_rvn.initialized = R_TRUE;
	rvn_services_init();
	rvn_paint_init();
	rvn_themes_init();
	rvn_theme_apply(R_THEME_DARK);
	rvn_fonts_init();
	g_rvn.theme.font = g_rvn.default_font;
	g_rvn.last_update_time = rvn_clock_seconds();
	if (driven) rvn_headless_start();
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

#if defined(__EMSCRIPTEN__)
void rvn_web_images_shutdown(void);
#endif
void Rshutdown(void) {
	uint32_t i;
	int headless_exit;
	if (!rvn_api_check(__func__)) return;

	/* a run driven by the environment ends here when the application ends first: its pictures and its tree, while the windows are there */
	headless_exit = rvn_headless_shutdown();

	#if defined(__EMSCRIPTEN__)
    rvn_web_images_shutdown();
#endif
    /* Stop native filesystem notifications before draining posted UI tasks. */
	rvn_fs_watch_stop();
	/* the answers of the system still coming are dropped: no callback of the application while everything goes */
	rvn_services_shutdown();
	/* windows first: this deletes every widget that is shown */
	/* (the inspectors first: they are windows that follow other windows) */
	rvn_inspector_shutdown();
	rvn_windows_destroy_all();
	/* then the widgets the application created but never attached */
	for (i = 0; i < g_rvn.widgets.used; i++) {
		rvn_widget_t* widget = (rvn_widget_t*)rvn_pool_at(&g_rvn.widgets, i);
		/* internal widgets are destroyed by the widget that owns them */
		if (widget && !widget->parent && !(widget->flags & RVN_WF_INTERNAL)) rvn_widget_destroy(widget);
	}
	rvn_collect_dead();

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

	/* (the scripts before the XML: their actions are unbound, their memory goes) */
	rvn_script_shutdown();
	rvn_xml_shutdown();
	rvn_taskbar_shutdown();
	rvn_tray_shutdown();
	rvn_notify_shutdown();
	rvn_locale_shutdown();
	rvn_timezone_shutdown();
	rvn_a11y_shutdown();
	rvn_themes_shutdown();
	rvn_look_shutdown();
	rvn_dialogs_shutdown();
	rvn_islands_shutdown();
	rvn_pdf_shutdown();
	rvn_spell_shutdown();
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
	rvn_free(g_rvn.icons);
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
	/* a script that failed fails the process, whatever main returns after */
	if (headless_exit != 0) {
		fflush(stdout);
		fflush(stderr);
		exit(headless_exit);
	}
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

int rvn_tasks_pending(void) {
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

int rvn_action_lookup(const char* name, RActionCallback* callback, void** user_data) {
	rvn_action_t* action = name ? rvn_action_find(name) : NULL;
	if (!action) return R_FALSE;
	if (callback) *callback = action->callback;
	if (user_data) *user_data = action->user_data;
	return R_TRUE;
}

void Raction_set_default_handler(RActionCallback callback, void* user_data) {
	RVN_API_GUARD_VOID();
	g_rvn.default_action = callback;
	g_rvn.default_action_user = user_data;
}

void Raction_set_builtins(int enabled) {
	RVN_API_GUARD_VOID();
	g_rvn.builtins_off = !enabled;
}

/* (widget/actions.c: "app.quit", "show:name"...) */
int rvn_builtin_action(Rid sender, const char* name);

void rvn_fire_action(Rid sender, const char* name) {
	rvn_action_t* action;
	if (!name || !name[0]) return;
	action = rvn_action_find(name);
	if (action) {
		/* copy: the callback may bind or unbind actions */
		RActionCallback callback = action->callback;
		void* user_data = action->user_data;
		callback(sender, name, user_data);
	} else if (!g_rvn.builtins_off && rvn_builtin_action(sender, name)) {
		/* one of the actions Raven knows by itself */
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

void Rclipboard_set_html(const char* text, const char* html) {
	RVN_API_GUARD_VOID();
	if (html && g_rvn.platform->clipboard_set_data) {
		RDataItem items[2];
		items[0].mime = "text/plain";
		items[0].data = text ? text : "";
		items[0].size = strlen((const char*)items[0].data);
		items[1].mime = "text/html";
		items[1].data = html;
		items[1].size = strlen(html);
		g_rvn.platform->clipboard_set_data(items, 2);
	} else if (html && g_rvn.platform->clipboard_set_html) {
		g_rvn.platform->clipboard_set_html(text ? text : "", html);
	} else {
		g_rvn.platform->clipboard_set(text ? text : "");
	}
}

const char* Rclipboard_get_text(void) {
	RVN_API_GUARD("");
	rvn_free(g_rvn.clipboard);
	g_rvn.clipboard = g_rvn.platform->clipboard_get();
	return g_rvn.clipboard ? g_rvn.clipboard : "";
}

static int rvn_data_check(const RDataItem* items, int count, const char* function) {
	int i;
	if (count < 0 || (count > 0 && !items)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: no items", function);
		return R_FALSE;
	}
	for (i = 0; i < count; i++) {
		if (!items[i].mime || !items[i].mime[0] || (!items[i].data && items[i].size)) {
			rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: item %d has no MIME type or no data", function, i);
			return R_FALSE;
		}
	}
	return R_TRUE;
}

int Rclipboard_set_data(const RDataItem* items, int count) {
	const RDataItem* text;
	const RDataItem* html;
	char* plain;
	int i, all = R_TRUE;
	RVN_API_GUARD(R_FALSE);
	if (!rvn_data_check(items, count, __func__)) return R_FALSE;
	if (g_rvn.platform->clipboard_set_data) return g_rvn.platform->clipboard_set_data(items, count) ? R_TRUE : R_FALSE;
	/* a window system that only takes text (and HTML) */
	text = rvn_data_find(items, count, "text/plain");
	html = g_rvn.platform->clipboard_set_html ? rvn_data_find(items, count, "text/html") : NULL;
	if (!text) return R_FALSE;
	for (i = 0; i < count; i++) {
		if (&items[i] != text && &items[i] != html) all = R_FALSE;
	}
	plain = rvn_bytes_dup(text->data, text->size);
	if (html) {
		char* markup = rvn_bytes_dup(html->data, html->size);
		g_rvn.platform->clipboard_set_html(plain, markup);
		rvn_free(markup);
	} else {
		g_rvn.platform->clipboard_set(plain);
	}
	rvn_free(plain);
	return all;
}

int Rclipboard_has_data(const char* mime) {
	char* text;
	int has;
	RVN_API_GUARD(R_FALSE);
	if (!mime || !mime[0]) return R_FALSE;
	if (g_rvn.platform->clipboard_has_data) return g_rvn.platform->clipboard_has_data(mime) ? R_TRUE : R_FALSE;
	if (!rvn_mime_is(mime, "text/plain")) return R_FALSE;
	text = g_rvn.platform->clipboard_get();
	has = text && text[0];
	rvn_free(text);
	return has ? R_TRUE : R_FALSE;
}

char* Rclipboard_get_data(const char* mime, size_t* size) {
	char* data;
	size_t length = 0;
	if (size) *size = 0;
	RVN_API_GUARD(NULL);
	if (!mime || !mime[0]) return NULL;
	if (g_rvn.platform->clipboard_get_data) {
		data = g_rvn.platform->clipboard_get_data(mime, &length);
	} else if (rvn_mime_is(mime, "text/plain")) {
		data = g_rvn.platform->clipboard_get();
		length = data ? strlen(data) : 0;
		if (data && !length) {
			rvn_free(data);
			data = NULL;
		}
	} else {
		data = NULL;
	}
	if (data && size) *size = length;
	return data;
}

int Rclipboard_set_image(const uint8_t* rgba, int width, int height, int stride) {
	RDataItem item;
	uint8_t* png;
	uint8_t* packed = NULL;
	size_t size = 0;
	int done;
	RVN_API_GUARD(R_FALSE);
	if (!rgba || width <= 0 || height <= 0 || (stride != 0 && stride < width * 4)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rclipboard_set_image: invalid pixels (%dx%d, stride %d)", width, height, stride);
		return R_FALSE;
	}
	if (!g_rvn.platform->clipboard_set_data) return R_FALSE;
	/* (the PNG writer takes rows one after the other) */
	if (stride != 0 && stride != width * 4) {
		int y;
		packed = (uint8_t*)rvn_alloc((size_t)width * (size_t)height * 4u);
		for (y = 0; y < height; y++) {
			memcpy(packed + (size_t)y * (size_t)width * 4u, rgba + (size_t)y * (size_t)stride, (size_t)width * 4u);
		}
	}
	png = Rimage_encode_png(packed ? packed : rgba, width, height, &size);
	rvn_free(packed);
	if (!png) return R_FALSE;
	item.mime = "image/png";
	item.data = png;
	item.size = size;
	done = g_rvn.platform->clipboard_set_data(&item, 1);
	Rpixels_free(png);
	return done ? R_TRUE : R_FALSE;
}

int Rclipboard_has_image(void) {
	RVN_API_GUARD(R_FALSE);
	return Rclipboard_has_data("image/png") || Rclipboard_has_data("image/jpeg");
}

uint8_t* Rclipboard_get_image(int* width, int* height) {
	static const char* const formats[] = { "image/png", "image/jpeg", "image/bmp" };
	size_t i;
	if (width) *width = 0;
	if (height) *height = 0;
	RVN_API_GUARD(NULL);
	for (i = 0; i < sizeof(formats) / sizeof(formats[0]); i++) {
		size_t size = 0;
		char* data = Rclipboard_get_data(formats[i], &size);
		uint8_t* pixels;
		if (!data) continue;
		pixels = Rimage_decode(data, size, width, height);
		rvn_free(data);
		if (pixels) return pixels;
	}
	return NULL;
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
	/* an application with a tray icon lives on without windows, until Rquit */
	if (rvn_tray_count() > 0) return R_TRUE;
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

/* ---------------------------------------------------------------------------
 *  Application lifecycle
 * ------------------------------------------------------------------------- */

void rvn_app_event(RAppEvent event) {
	switch (event) {
		case R_APP_DID_PAUSE:
			/* nothing is drawn from now on (rvn_window_can_paint); what is queued is finished before the system takes the GPU away */
			g_rvn.app_paused = R_TRUE;
			rvn_renderer_wait_idle();
			break;
		case R_APP_WILL_RESUME:
		case R_APP_DID_RESUME:
			/* (DID_RESUME alone too: a platform may miss the first) */
			if (g_rvn.app_paused) {
				g_rvn.app_paused = R_FALSE;
				rvn_repaint_all_windows();
				rvn_wake();
			}
			break;
		default:
			break;
	}
	if (g_rvn.app_event_callback) {
		rvn_dispatch_begin();
		g_rvn.app_event_callback(event, g_rvn.app_event_user);
		rvn_dispatch_end();
	}
	if (event == R_APP_LOW_MEMORY) {
		/* after the application freed its own: Raven's caches that are made again on use, then the empty pages of the heap */
		rvn_renderer_trim();
		Rmemory_trim();
	}
}

void Rset_app_event_callback(RAppEventCallback callback, void* user_data) {
	RVN_API_GUARD_VOID();
	g_rvn.app_event_callback = callback;
	g_rvn.app_event_user = user_data;
}

int Ris_app_paused(void) {
	return g_rvn.initialized && g_rvn.app_paused ? R_TRUE : R_FALSE;
}

void Rinject_app_event(RAppEvent event) {
	RVN_API_GUARD_VOID();
	if ((int)event < (int)R_APP_WILL_PAUSE || (int)event > (int)R_APP_WILL_TERMINATE) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rinject_app_event: %d is not an RAppEvent", (int)event);
		return;
	}
	rvn_app_event(event);
}

void Rpoll_events(void) {
	RVN_API_GUARD_VOID();
	g_rvn.platform->pump(0.0);
}

double rvn_wait_timeout(double timeout_seconds) {
	uint32_t i;
	double timer_delay;
	/* do not sleep when there is already something to do */
	if (rvn_tasks_pending() || rvn_atomic_load(&g_rvn.redraw_all) || rvn_atomic_load(&g_rvn.quit_requested) || rvn_headless_busy()) {
		timeout_seconds = 0.0;
	}
	for (i = 0; i < g_rvn.windows.used && timeout_seconds != 0.0; i++) {
		rvn_window_t* window = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
		/* (a window waiting for the compositor sleeps until it answers, or until its wait runs out) */
		double frame_in = window ? rvn_window_frame_in(window) : -1.0;
		if (frame_in >= 0.0 && (timeout_seconds < 0.0 || frame_in < timeout_seconds)) timeout_seconds = frame_in;
	}
	timer_delay = rvn_timers_next_delay();
	if (timer_delay >= 0.0 && (timeout_seconds < 0.0 || timer_delay < timeout_seconds)) timeout_seconds = timer_delay;
	return timeout_seconds;
}

void Rwait_events(double timeout_seconds) {
	RVN_API_GUARD_VOID();
	g_rvn.platform->pump(rvn_wait_timeout(timeout_seconds));
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
	/* the menu of the icon of the application follows its Menu */
	rvn_taskbar_update();
	rvn_tray_update();
	rvn_textures_update_svg();
	rvn_inspector_update();
	/* (a callback of the layout may close a window: it is freed once they all are done) */
	rvn_dispatch_begin();
	for (i = 0; i < g_rvn.windows.used; i++) {
		rvn_window_t* window = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
		if (window) rvn_window_update(window);
	}
	rvn_dispatch_end();
	g_rvn.in_update = R_FALSE;
	/* the next steps of a script (widget/headless.c) */
	rvn_headless_frame();
}

int rvn_frame_pacing(void) {
	/* (only the Vulkan and Metal backends present to the window: a frame of the null backend would never be answered) */
	return (g_rvn.backend == R_BACKEND_VULKAN || g_rvn.backend == R_BACKEND_METAL) && g_rvn.platform &&
		g_rvn.platform->window_frame_wait && g_rvn.platform->window_frame_begin;
}

int rvn_present_vsync(void) {
	return g_rvn.vsync && !rvn_frame_pacing();
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

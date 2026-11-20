/**
 * Raven - platform/services.c
 * The services of the device (raven_device.h): permissions, the pickers and
 * the camera of the system, haptics, sharing, opening URLs and downloads.
 *
 * The platform does the work (rvn_platform_t: permission_request, file_open,
 * camera_take_photo...) and answers later, from any thread, through
 * rvn_permission_answer and rvn_service_complete. Those answers are posted as
 * tasks: the application gets them on the UI thread, at the next frame. The
 * answers the core gives itself (unsupported, busy, a status already known)
 * take the same way, so that no callback ever runs inside the call that asked.
 *
 * Without the platform's entry, the desktops answer with the file dialogs of
 * widgets/dialogs.c (which wait for the user, run from a task); the phones
 * answer R_DEVICE_UNSUPPORTED (their window system cannot run a dialog that
 * waits).
 */

#include "platform.h"
#include "../core/context.h"
#include "../core/fs.h"
#include "../core/thread.h"
#include "../widget/widget.h"

#include <ctype.h>

#define RVN_PERMISSION_COUNT ((int)R_PERMISSION_LOCATION - (int)R_PERMISSION_CAMERA + 1)

typedef enum rvn_service_kind_t {
	RVN_SERVICE_OPEN,
	RVN_SERVICE_SAVE,
	RVN_SERVICE_FOLDER,
	RVN_SERVICE_PHOTO,
	RVN_SERVICE_PICK_PHOTOS,
	RVN_SERVICE_SHARE
} rvn_service_kind_t;

/* A request of the application waiting for its answer, with its own copy of what was asked. */
typedef struct rvn_service_t {
	uint32_t id;
	rvn_service_kind_t kind;
	RDeviceCallback callback;
	void* user_data;
	Rid window;
	rvn_service_request_t request;
	/* (copies the request points into) */
	RFileFilter* filters;
	char* strings[8];
	void* data;
	struct rvn_service_t* next;
} rvn_service_t;

typedef struct rvn_permission_wait_t {
	RPermissionCallback callback;
	void* user_data;
} rvn_permission_wait_t;

/* Built on the thread that answers, delivered on the UI thread (a task). */
typedef struct rvn_service_answer_t {
	/* a request (id != 0), or a permission */
	uint32_t id;
	int result;
	RPickedFile* files;
	int count;
	char* error;
	int permission;
	int status;
	/* a status given to one callback only (Rpermission_request answered at once) */
	RPermissionCallback callback;
	void* user_data;
} rvn_service_answer_t;

static struct {
	/* requests waiting for rvn_service_complete (UI thread only) */
	rvn_service_t* pending;
	/* the screen of the system open now (a request id), 0: none */
	uint32_t sheet;
	struct {
		rvn_permission_wait_t* waits;
		uint32_t count, cap;
		/* the platform was asked and has not answered yet */
		int asking;
	} permissions[RVN_PERMISSION_COUNT];
} g_services;

/*
 * What the answering threads look at: kept across Rinit / Rshutdown (an
 * answer of the system may come after Raven stopped), the lock made once and
 * never destroyed.
 */
static rvn_mutex_t g_services_lock;
static int g_services_lock_made;
static int g_services_alive;
static uint32_t g_services_next_id;

void rvn_services_init(void) {
	if (!g_services_lock_made) {
		rvn_mutex_init(&g_services_lock);
		g_services_lock_made = R_TRUE;
	}
	memset(&g_services, 0, sizeof(g_services));
	rvn_mutex_lock(&g_services_lock);
	g_services_alive = R_TRUE;
	rvn_mutex_unlock(&g_services_lock);
}

static void rvn_service_free(rvn_service_t* service) {
	int i;
	for (i = 0; i < (int)(sizeof(service->strings) / sizeof(service->strings[0])); i++) rvn_free(service->strings[i]);
	if (service->filters) {
		for (i = 0; i < service->request.filter_count; i++) {
			rvn_free((char*)service->filters[i].name);
			rvn_free((char*)service->filters[i].patterns);
		}
		rvn_free(service->filters);
	}
	rvn_free(service->data);
	rvn_free(service);
}

/* Before the windows go: the requests still open are forgotten, their callbacks never called. */
void rvn_services_shutdown(void) {
	int i;
	if (!g_services_lock_made) return;
	rvn_mutex_lock(&g_services_lock);
	g_services_alive = R_FALSE;
	rvn_mutex_unlock(&g_services_lock);
	while (g_services.pending) {
		rvn_service_t* next = g_services.pending->next;
		rvn_service_free(g_services.pending);
		g_services.pending = next;
	}
	for (i = 0; i < RVN_PERMISSION_COUNT; i++) rvn_free(g_services.permissions[i].waits);
	memset(&g_services, 0, sizeof(g_services));
}

static void rvn_service_answer_free(rvn_service_answer_t* answer) {
	int i;
	for (i = 0; i < answer->count; i++) {
		rvn_free((char*)answer->files[i].path);
		rvn_free((char*)answer->files[i].name);
		rvn_free((char*)answer->files[i].mime_type);
	}
	rvn_free(answer->files);
	rvn_free(answer->error);
	rvn_free(answer);
}

static int rvn_permission_index(int permission) {
	int index = permission - (int)R_PERMISSION_CAMERA;
	return index >= 0 && index < RVN_PERMISSION_COUNT ? index : -1;
}

static void rvn_service_start(rvn_service_t* service);

static rvn_service_t* rvn_service_take(uint32_t id) {
	rvn_service_t** link = &g_services.pending;
	while (*link && (*link)->id != id) link = &(*link)->next;
	if (*link) {
		rvn_service_t* service = *link;
		*link = service->next;
		service->next = NULL;
		return service;
	}
	return NULL;
}

/* UI thread (a task): hands an answer to the application. */
static void rvn_service_deliver(void* arg) {
	rvn_service_answer_t* answer = (rvn_service_answer_t*)arg;
	if (!g_services_alive) {
		rvn_service_answer_free(answer);
		return;
	}
	if (answer->id) {
		rvn_service_t* service = rvn_service_take(answer->id);
		if (!service) {
			rvn_error(R_ERROR_PLATFORM, R_SEVERITY_WARNING, "device services: an answer to request %u, which is not open", (unsigned)answer->id);
		} else {
			if (g_services.sheet == service->id) g_services.sheet = 0;
			if (answer->error && *answer->error)
				rvn_error(R_ERROR_PLATFORM, R_SEVERITY_WARNING, "device services: %s", answer->error);
			if (service->callback) {
				int done = answer->result == (int)R_DEVICE_DONE;
				service->callback((RDeviceResult)answer->result, done && answer->count ? answer->files : NULL,
					done ? answer->count : 0, service->user_data);
			}
			rvn_service_free(service);
		}
	} else if (answer->callback) {
		answer->callback((RPermission)answer->permission, (RPermissionStatus)answer->status, answer->user_data);
	} else {
		int index = rvn_permission_index(answer->permission);
		if (index >= 0) {
			rvn_permission_wait_t* waits = g_services.permissions[index].waits;
			uint32_t count = g_services.permissions[index].count, i;
			/* (a callback may ask again: it starts a new list) */
			g_services.permissions[index].waits = NULL;
			g_services.permissions[index].count = 0;
			g_services.permissions[index].cap = 0;
			g_services.permissions[index].asking = R_FALSE;
			for (i = 0; i < count; i++)
				waits[i].callback((RPermission)answer->permission, (RPermissionStatus)answer->status, waits[i].user_data);
			rvn_free(waits);
		}
	}
	rvn_service_answer_free(answer);
}

/* Any thread. R_FALSE when Raven is not running: the answer is dropped. */
static int rvn_service_post(rvn_service_answer_t* answer) {
	int posted = R_FALSE;
	if (g_services_lock_made) {
		rvn_mutex_lock(&g_services_lock);
		if (g_services_alive) {
			Rtask_post(rvn_service_deliver, answer);
			posted = R_TRUE;
		}
		rvn_mutex_unlock(&g_services_lock);
	}
	if (!posted) rvn_service_answer_free(answer);
	return posted;
}

void rvn_permission_answer(int permission, int status) {
	rvn_service_answer_t* answer;
	if (rvn_permission_index(permission) < 0) return;
	if (status < (int)R_PERMISSION_UNSUPPORTED || status > (int)R_PERMISSION_BLOCKED) status = (int)R_PERMISSION_UNSUPPORTED;
	answer = (rvn_service_answer_t*)rvn_calloc(sizeof(*answer));
	answer->permission = permission;
	answer->status = status;
	rvn_service_post(answer);
}

void rvn_service_complete(uint32_t id, int result, const RPickedFile* files, int count, const char* error) {
	rvn_service_answer_t* answer;
	int i;
	if (!id) return;
	if (result < (int)R_DEVICE_DONE || result > (int)R_DEVICE_FAILED) result = (int)R_DEVICE_FAILED;
	answer = (rvn_service_answer_t*)rvn_calloc(sizeof(*answer));
	answer->id = id;
	answer->result = result;
	answer->error = error && *error ? rvn_strdup(error) : NULL;
	if (files && count > 0) {
		answer->files = (RPickedFile*)rvn_calloc(sizeof(RPickedFile) * (size_t)count);
		for (i = 0; i < count; i++) {
			RPickedFile* file = &answer->files[answer->count];
			uint64_t modified = 0, size = 0;
			file->path = files[i].path ? rvn_strdup(files[i].path) : NULL;
			file->name = rvn_strdup(files[i].name ? files[i].name : files[i].path ? rvn_path_name(files[i].path) : "");
			file->mime_type = files[i].mime_type ? rvn_strdup(files[i].mime_type) : NULL;
			file->size = files[i].size;
			if (!file->size && file->path && rvn_file_stamp(file->path, &modified, &size)) file->size = size;
			file->width = files[i].width > 0 ? files[i].width : 0;
			file->height = files[i].height > 0 ? files[i].height : 0;
			answer->count++;
		}
	}
	rvn_service_post(answer);
}

/* UI thread: the core answers itself (not supported, busy, refused), at the next frame. */
static void rvn_service_answer_now(uint32_t id, RDeviceResult result) {
	rvn_service_complete(id, (int)result, NULL, 0, NULL);
}

char** rvn_service_extensions(const rvn_service_request_t* request, int* count) {
	char** list = NULL;
	uint32_t used = 0, cap = 0;
	int i;
	*count = 0;
	if (!request || !request->filters) return NULL;
	for (i = 0; i < request->filter_count; i++) {
		const char* p = request->filters[i].patterns;
		while (p && *p) {
			size_t length = strcspn(p, ";");
			const char* dot;
			/* "*", "*.*": everything */
			if ((length == 1 && p[0] == '*') || (length == 3 && strncmp(p, "*.*", 3) == 0)) {
				rvn_service_extensions_free(list, (int)used);
				return NULL;
			}
			dot = memchr(p, '.', length);
			if (dot && (size_t)(dot - p) + 1 < length) {
				size_t n = length - (size_t)(dot - p) - 1, k;
				char* extension = (char*)rvn_alloc(n + 1);
				uint32_t j;
				int seen = R_FALSE;
				for (k = 0; k < n; k++) extension[k] = (char)tolower((unsigned char)dot[1 + k]);
				extension[n] = '\0';
				for (j = 0; j < used && !seen; j++) seen = strcmp(list[j], extension) == 0;
				if (seen) {
					rvn_free(extension);
				} else {
					RVN_ARRAY_RESERVE(list, cap, used + 1);
					list[used++] = extension;
				}
			}
			p += length;
			if (*p == ';') p++;
		}
	}
	*count = (int)used;
	return list;
}

void rvn_service_extensions_free(char** extensions, int count) {
	int i;
	for (i = 0; i < count; i++) rvn_free(extensions[i]);
	rvn_free(extensions);
}

/* ===========================================================================
 *  Permissions
 * ========================================================================= */

static RPermissionStatus rvn_permission_status(RPermission permission) {
	const rvn_platform_t* platform = g_rvn.platform;
	int status;
	if (platform->permission_status) {
		status = platform->permission_status((int)permission);
		if (status >= (int)R_PERMISSION_UNSUPPORTED && status <= (int)R_PERMISSION_BLOCKED) return (RPermissionStatus)status;
		return R_PERMISSION_UNSUPPORTED;
	}
	/* (Windows and the Linux desktops show notifications without asking) */
	if (permission == R_PERMISSION_NOTIFICATIONS && platform->notify) return R_PERMISSION_GRANTED;
	return R_PERMISSION_UNSUPPORTED;
}

static int rvn_permission_check(RPermission permission, const char* function) {
	if (rvn_permission_index((int)permission) >= 0) return R_TRUE;
	rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "%s: %d is not an RPermission", function, (int)permission);
	return R_FALSE;
}

/* UI thread. Asks the platform, or answers with the status known (at the next frame). */
static void rvn_permission_ask(RPermission permission, RPermissionCallback callback, void* user_data) {
	RPermissionStatus status = rvn_permission_status(permission);
	if ((status == R_PERMISSION_NOT_ASKED || status == R_PERMISSION_DENIED) && g_rvn.platform->permission_request) {
		int index = rvn_permission_index((int)permission);
		if (callback) {
			RVN_ARRAY_RESERVE(g_services.permissions[index].waits, g_services.permissions[index].cap, g_services.permissions[index].count + 1);
			g_services.permissions[index].waits[g_services.permissions[index].count].callback = callback;
			g_services.permissions[index].waits[g_services.permissions[index].count].user_data = user_data;
			g_services.permissions[index].count++;
		}
		if (!g_services.permissions[index].asking) {
			g_services.permissions[index].asking = R_TRUE;
			g_rvn.platform->permission_request((int)permission);
		}
		return;
	}
	if (callback) {
		rvn_service_answer_t* answer = (rvn_service_answer_t*)rvn_calloc(sizeof(*answer));
		answer->permission = (int)permission;
		answer->status = (int)status;
		answer->callback = callback;
		answer->user_data = user_data;
		rvn_service_post(answer);
	}
}

RPermissionStatus Rpermission_get_status(RPermission permission) {
	RVN_API_GUARD(R_PERMISSION_UNSUPPORTED);
	if (!rvn_permission_check(permission, __func__)) return R_PERMISSION_UNSUPPORTED;
	return rvn_permission_status(permission);
}

void Rpermission_request(RPermission permission, RPermissionCallback callback, void* user_data) {
	RVN_API_GUARD_VOID();
	if (!rvn_permission_check(permission, __func__)) return;
	rvn_permission_ask(permission, callback, user_data);
}

int Rpermission_open_settings(void) {
	RVN_API_GUARD(R_FALSE);
	return g_rvn.platform->permission_open_settings ? g_rvn.platform->permission_open_settings() : R_FALSE;
}

const char* Rpermission_to_string(RPermission permission) {
	switch (permission) {
	case R_PERMISSION_CAMERA: return "camera";
	case R_PERMISSION_NOTIFICATIONS: return "notifications";
	case R_PERMISSION_PHOTOS: return "photos";
	case R_PERMISSION_MICROPHONE: return "microphone";
	case R_PERMISSION_LOCATION: return "location";
	default: return "unknown";
	}
}

const char* Rpermission_status_to_string(RPermissionStatus status) {
	switch (status) {
	case R_PERMISSION_UNSUPPORTED: return "unsupported";
	case R_PERMISSION_NOT_ASKED: return "not asked";
	case R_PERMISSION_GRANTED: return "granted";
	case R_PERMISSION_LIMITED: return "limited";
	case R_PERMISSION_DENIED: return "denied";
	case R_PERMISSION_BLOCKED: return "blocked";
	default: return "unknown";
	}
}

/* ===========================================================================
 *  Requests: pickers, camera, share
 * ========================================================================= */

static char* rvn_service_keep(rvn_service_t* service, int slot, const char* text) {
	service->strings[slot] = text ? rvn_strdup(text) : NULL;
	return service->strings[slot];
}

/* The window a request is shown over: the one given, else the first one alive. */
static rvn_window_t* rvn_service_window(Rid id) {
	uint32_t i;
	rvn_window_t* window = id != R_INVALID_ID ? rvn_window_get(id) : NULL;
	if (window && !window->dead && window->platform) return window;
	for (i = 0; i < g_rvn.windows.used; i++) {
		window = (rvn_window_t*)rvn_pool_at(&g_rvn.windows, i);
		if (window && !window->dead && window->platform) return window;
	}
	return NULL;
}

/* A new request, with its own copy of the description; registered as open. */
static rvn_service_t* rvn_service_new(rvn_service_kind_t kind, const RFileDialogDesc* desc, RDeviceCallback callback, void* user_data) {
	rvn_service_t* service = (rvn_service_t*)rvn_calloc(sizeof(rvn_service_t));
	rvn_window_t* window;
	if (++g_services_next_id == 0) g_services_next_id = 1;
	service->id = g_services_next_id;
	service->kind = kind;
	service->callback = callback;
	service->user_data = user_data;
	service->request.id = service->id;
	service->window = desc ? desc->parent : R_INVALID_ID;
	window = rvn_service_window(service->window);
	service->request.window = window ? window->platform : NULL;
	if (desc) {
		int i;
		service->request.title = rvn_service_keep(service, 0, desc->title);
		service->request.file_name = rvn_service_keep(service, 1, desc->file_name);
		service->request.default_extension = rvn_service_keep(service, 2, desc->default_extension);
		/* (desktop dialogs only: kept for them) */
		rvn_service_keep(service, 7, desc->directory);
		service->request.multiple = (desc->flags & R_FILE_DIALOG_MULTIPLE) ? R_TRUE : R_FALSE;
		if (desc->filters && desc->filter_count > 0) {
			service->filters = (RFileFilter*)rvn_calloc(sizeof(RFileFilter) * (size_t)desc->filter_count);
			for (i = 0; i < desc->filter_count; i++) {
				service->filters[i].name = rvn_strdup(desc->filters[i].name ? desc->filters[i].name : "");
				service->filters[i].patterns = rvn_strdup(desc->filters[i].patterns ? desc->filters[i].patterns : "*");
			}
			service->request.filters = service->filters;
			service->request.filter_count = desc->filter_count;
		}
	}
	service->next = g_services.pending;
	g_services.pending = service;
	return service;
}

/* The flags of the desktop dialog a request stands for. */
static void rvn_service_dialog_desc(const rvn_service_t* service, RFileDialogDesc* desc) {
	memset(desc, 0, sizeof(*desc));
	desc->parent = service->window;
	desc->title = service->request.title;
	desc->directory = service->strings[7];
	desc->file_name = service->request.file_name;
	desc->default_extension = service->request.default_extension;
	desc->filters = service->request.filters;
	desc->filter_count = service->request.filter_count;
	if (service->request.multiple) desc->flags |= R_FILE_DIALOG_MULTIPLE;
}

/*
 * Desktops (a task): the file dialog that waits, then the answer. The
 * request id is the argument: the request may have been forgotten meanwhile
 * (Rshutdown).
 */
static void rvn_service_desktop(void* arg) {
	uint32_t id = (uint32_t)(uintptr_t)arg;
	rvn_service_t* service;
	RFileDialogDesc desc;
	const char* chosen = NULL;
	RPickedFile* files = NULL;
	int count = 0, i;
	RDeviceResult result = R_DEVICE_CANCELLED;
	const char* error = NULL;

	if (!g_services_alive) return;
	for (service = g_services.pending; service && service->id != id; service = service->next) {}
	if (!service) return;
	rvn_service_dialog_desc(service, &desc);
	switch (service->kind) {
	case RVN_SERVICE_OPEN:
	case RVN_SERVICE_PICK_PHOTOS:
		chosen = Rfile_dialog_open(&desc);
		break;
	case RVN_SERVICE_SAVE:
		chosen = Rfile_dialog_save(&desc);
		break;
	case RVN_SERVICE_FOLDER:
		chosen = Rfile_dialog_select_folder(&desc);
		break;
	default:
		break;
	}
	if (chosen) {
		count = service->kind == RVN_SERVICE_SAVE || service->kind == RVN_SERVICE_FOLDER ? 1 : Rfile_dialog_get_result_count();
		if (service->kind == RVN_SERVICE_PICK_PHOTOS && service->request.max_count > 0 && count > service->request.max_count)
			count = service->request.max_count;
		files = (RPickedFile*)rvn_calloc(sizeof(RPickedFile) * (size_t)(count > 0 ? count : 1));
		for (i = 0; i < count; i++) files[i].path = Rfile_dialog_get_result(i);
		result = R_DEVICE_DONE;
		if (service->kind == RVN_SERVICE_SAVE) {
			int written;
			if (service->request.path) {
				size_t size = 0;
				char* content = rvn_read_file(service->request.path, &size);
				written = content && rvn_write_file(chosen, content, size);
				rvn_free(content);
			} else {
				written = rvn_write_file(chosen, service->data, service->request.size);
			}
			if (!written) {
				result = R_DEVICE_FAILED;
				error = "the file could not be written";
				count = 0;
			}
		}
	}
	/* (the request is still open: a frame of the dialog cannot have answered it) */
	rvn_service_complete(id, (int)result, files, count, error);
	rvn_free(files);
}

/* Phones have no dialog that waits: without the platform's entry, nothing to show. */
static int rvn_service_desktop_dialogs(void) {
	RWindowSystem system = g_rvn.platform->system;
	return system != R_WINDOW_SYSTEM_UIKIT && system != R_WINDOW_SYSTEM_ANDROID && system != R_WINDOW_SYSTEM_EMSCRIPTEN;
}

/* The platform's entry for a request, or NULL. */
static void (*rvn_service_entry(rvn_service_kind_t kind))(const rvn_service_request_t*) {
	const rvn_platform_t* platform = g_rvn.platform;
	switch (kind) {
	case RVN_SERVICE_OPEN: return platform->file_open;
	case RVN_SERVICE_SAVE: return platform->file_save;
	case RVN_SERVICE_FOLDER: return platform->folder_select;
	case RVN_SERVICE_PHOTO: return platform->camera_take_photo;
	case RVN_SERVICE_PICK_PHOTOS: return platform->photo_pick;
	case RVN_SERVICE_SHARE: return platform->share;
	}
	return NULL;
}

/* Hands the request to the platform (or to the desktop dialogs). It is the screen of the system now. */
static void rvn_service_start(rvn_service_t* service) {
	void (*entry)(const rvn_service_request_t*) = rvn_service_entry(service->kind);
	rvn_window_t* window = rvn_service_window(service->window);
	g_services.sheet = service->id;
	/* (the window may have gone while a permission was asked) */
	service->request.window = window ? window->platform : NULL;
	if (entry) {
		entry(&service->request);
	} else {
		Rtask_post(rvn_service_desktop, (void*)(uintptr_t)service->id);
	}
}

/* The answer about the camera, asked before taking a photo. */
static void rvn_service_camera_allowed(RPermission permission, RPermissionStatus status, void* user_data) {
	uint32_t id = (uint32_t)(uintptr_t)user_data;
	rvn_service_t* service;
	RVN_UNUSED(permission);
	for (service = g_services.pending; service && service->id != id; service = service->next) {}
	if (!service) return;
	if (status == R_PERMISSION_GRANTED || status == R_PERMISSION_LIMITED || status == R_PERMISSION_UNSUPPORTED) rvn_service_start(service);
	else rvn_service_answer_now(id, R_DEVICE_DENIED);
}

/*
 * Common to every request: answered at once (at the next frame) when the
 * platform cannot, or while another screen of the system is open.
 */
static rvn_service_t* rvn_service_begin(rvn_service_kind_t kind, const RFileDialogDesc* desc, RDeviceCallback callback, void* user_data) {
	rvn_service_t* service = rvn_service_new(kind, desc, callback, user_data);
	int desktop = kind != RVN_SERVICE_PHOTO && kind != RVN_SERVICE_SHARE && rvn_service_desktop_dialogs();
	if (!rvn_service_entry(kind) && !desktop) {
		rvn_service_answer_now(service->id, R_DEVICE_UNSUPPORTED);
		return NULL;
	}
	if (kind == RVN_SERVICE_PHOTO && g_rvn.platform->camera_available && !g_rvn.platform->camera_available()) {
		rvn_service_answer_now(service->id, R_DEVICE_UNSUPPORTED);
		return NULL;
	}
	if (g_services.sheet) {
		rvn_service_answer_now(service->id, R_DEVICE_BUSY);
		return NULL;
	}
	return service;
}

void Rfile_open_async(const RFileDialogDesc* desc, RDeviceCallback callback, void* user_data) {
	rvn_service_t* service;
	RVN_API_GUARD_VOID();
	service = rvn_service_begin(RVN_SERVICE_OPEN, desc, callback, user_data);
	if (service) rvn_service_start(service);
}

void Rfile_save_async(const RFileDialogDesc* desc, const void* data, size_t size, RDeviceCallback callback, void* user_data) {
	rvn_service_t* service;
	RVN_API_GUARD_VOID();
	if (!data && size > 0) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rfile_save_async: no data");
		return;
	}
	service = rvn_service_begin(RVN_SERVICE_SAVE, desc, callback, user_data);
	if (!service) return;
	service->data = rvn_alloc(size ? size : 1);
	if (size) memcpy(service->data, data, size);
	service->request.data = service->data;
	service->request.size = size;
	rvn_service_start(service);
}

void Rfile_export_async(const RFileDialogDesc* desc, const char* path, RDeviceCallback callback, void* user_data) {
	rvn_service_t* service;
	RVN_API_GUARD_VOID();
	if (!path || !*path) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rfile_export_async: no file");
		return;
	}
	service = rvn_service_begin(RVN_SERVICE_SAVE, desc, callback, user_data);
	if (!service) return;
	service->request.path = rvn_service_keep(service, 3, path);
	if (!service->request.file_name) service->request.file_name = rvn_service_keep(service, 1, rvn_path_name(path));
	rvn_service_start(service);
}

void Rfile_select_folder_async(const RFileDialogDesc* desc, RDeviceCallback callback, void* user_data) {
	rvn_service_t* service;
	RVN_API_GUARD_VOID();
	service = rvn_service_begin(RVN_SERVICE_FOLDER, desc, callback, user_data);
	if (service) rvn_service_start(service);
}

int Rcamera_is_available(void) {
	RVN_API_GUARD(R_FALSE);
	if (!g_rvn.platform->camera_take_photo) return R_FALSE;
	return g_rvn.platform->camera_available ? g_rvn.platform->camera_available() : R_TRUE;
}

void Rcamera_take_photo(RDeviceCallback callback, void* user_data) {
	rvn_service_t* service;
	RPermissionStatus status;
	RVN_API_GUARD_VOID();
	service = rvn_service_begin(RVN_SERVICE_PHOTO, NULL, callback, user_data);
	if (!service) return;
	/* the camera first: asked when needed, the request waits (it is the screen of the system already) */
	g_services.sheet = service->id;
	status = rvn_permission_status(R_PERMISSION_CAMERA);
	if (status == R_PERMISSION_BLOCKED) {
		rvn_service_answer_now(service->id, R_DEVICE_DENIED);
	} else if ((status == R_PERMISSION_NOT_ASKED || status == R_PERMISSION_DENIED) && g_rvn.platform->permission_request) {
		rvn_permission_ask(R_PERMISSION_CAMERA, rvn_service_camera_allowed, (void*)(uintptr_t)service->id);
	} else if (status == R_PERMISSION_DENIED) {
		rvn_service_answer_now(service->id, R_DEVICE_DENIED);
	} else {
		rvn_service_start(service);
	}
}

static const RFileFilter g_service_image_filter = { "Images", "*.png;*.jpg;*.jpeg;*.gif;*.bmp;*.tga;*.psd;*.svg;*.webp;*.heic" };

void Rphoto_pick(int max_count, RDeviceCallback callback, void* user_data) {
	rvn_service_t* service;
	RFileDialogDesc desc;
	RVN_API_GUARD_VOID();
	memset(&desc, 0, sizeof(desc));
	desc.filters = &g_service_image_filter;
	desc.filter_count = 1;
	if (max_count < 0) max_count = 0;
	if (max_count != 1) desc.flags |= R_FILE_DIALOG_MULTIPLE;
	service = rvn_service_begin(RVN_SERVICE_PICK_PHOTOS, &desc, callback, user_data);
	if (!service) return;
	service->request.max_count = max_count;
	rvn_service_start(service);
}

void Rshare_show(const RShareDesc* desc, RDeviceCallback callback, void* user_data) {
	rvn_service_t* service;
	RFileDialogDesc parent;
	rvn_window_t* window;
	int anchored;
	RVN_API_GUARD_VOID();
	if (!desc || ((!desc->text || !*desc->text) && (!desc->path || !*desc->path))) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rshare_show: no text and no file");
		return;
	}
	memset(&parent, 0, sizeof(parent));
	anchored = desc->anchor != 0 && desc->anchor != R_INVALID_ID && Rwidget_is_valid(desc->anchor);
	parent.parent = anchored ? Rwidget_get_window(desc->anchor) : R_INVALID_ID;
	service = rvn_service_begin(RVN_SERVICE_SHARE, &parent, callback, user_data);
	if (!service) return;
	service->request.text = rvn_service_keep(service, 3, desc->text);
	service->request.path = rvn_service_keep(service, 4, desc->path);
	service->request.mime_type = rvn_service_keep(service, 5, desc->mime_type);
	service->request.share_title = rvn_service_keep(service, 6, desc->title);
	window = rvn_service_window(service->window);
	if (window && anchored) {
		float x = 0, y = 0, w = 0, h = 0;
		Rwidget_get_rect(desc->anchor, &x, &y, &w, &h);
		service->request.anchor[0] = x * window->scale;
		service->request.anchor[1] = y * window->scale;
		service->request.anchor[2] = w * window->scale;
		service->request.anchor[3] = h * window->scale;
	}
	rvn_service_start(service);
}

/* ===========================================================================
 *  Haptics, URLs, downloads, clipboard
 * ========================================================================= */

int Rhaptic_is_supported(void) {
	RVN_API_GUARD(R_FALSE);
	if (!g_rvn.platform->haptic) return R_FALSE;
	return g_rvn.platform->haptic_supported ? g_rvn.platform->haptic_supported() : R_TRUE;
}

int Rhaptic_play(RHaptic haptic) {
	RVN_API_GUARD(R_FALSE);
	if (haptic < R_HAPTIC_LIGHT || haptic > R_HAPTIC_ERROR) {
		rvn_error(R_ERROR_INVALID_ENUM, R_SEVERITY_ERROR, "Rhaptic_play: %d is not an RHaptic", (int)haptic);
		return R_FALSE;
	}
	return g_rvn.platform->haptic ? g_rvn.platform->haptic((int)haptic) : R_FALSE;
}

int Ropen_url(const char* url) {
	if (!url || !url[0]) return R_FALSE;
	/* (desktops: works before Rinit and from any thread, as it always did) */
	if (g_rvn.initialized && g_rvn.platform && g_rvn.platform->open_url) {
		if (!rvn_api_check(__func__)) return R_FALSE;
		return g_rvn.platform->open_url(url);
	}
	return rvn_os_open_url(url);
}

int rvn_service_download(const char* url, uint8_t** data, size_t* size) {
	const rvn_platform_t* platform = g_rvn.platform;
	*data = NULL;
	*size = 0;
	if (platform && platform->download) {
		char* error = NULL;
		int ok = platform->download(url, data, size, &error);
		if (!ok) {
			if (*data) rvn_free(*data);
			*data = NULL;
			*size = 0;
			rvn_error(R_ERROR_PLATFORM, R_SEVERITY_WARNING, "cannot download %s: %s", url, error ? error : "the system did not say why");
		}
		rvn_free(error);
		return ok;
	}
	if (!rvn_fs_download_url(url, data, size)) {
		rvn_error(R_ERROR_PLATFORM, R_SEVERITY_WARNING, "cannot download %s", url);
		return R_FALSE;
	}
	return R_TRUE;
}

int Rclipboard_has_text(void) {
	RVN_API_GUARD(R_FALSE);
	if (g_rvn.platform->clipboard_has_text) return g_rvn.platform->clipboard_has_text();
	{
		char* text = g_rvn.platform->clipboard_get();
		int has = text && *text;
		rvn_free(text);
		return has;
	}
}

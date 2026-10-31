/**
 * Raven tests - test_wayland.c
 * The Wayland platform against a small compositor run in this process: a
 * window is made, configured, resized, scaled, clicked, typed into, given
 * the clipboard, dropped on, hidden and closed, and what Raven asked of the
 * compositor is checked.
 *
 * The compositor is written here on libwayland-server, opened at run time.
 * No GPU is used (null backend): what is tested is the dialogue with the
 * compositor. Skipped (77) where libwayland-server, libwayland-client or
 * libxkbcommon is missing.
 */

#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include "test.h"

#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

/* ---------------------------------------------------------------------------
 *  The protocol tables of Raven (wayland_protocols.h), shared with the server side
 * ------------------------------------------------------------------------- */

struct wl_proxy;
struct wl_message { const char* name; const char* signature; const struct wl_interface** types; };
struct wl_interface { const char* name; int version; int method_count; const struct wl_message* methods; int event_count; const struct wl_message* events; };
struct wl_array { size_t size; size_t alloc; void* data; };

/* (the client side of the tables is not used here) */
static struct wl_proxy* test_no_marshal(struct wl_proxy* proxy, uint32_t opcode, const struct wl_interface* interface, uint32_t version, uint32_t flags, ...) {
	(void)opcode; (void)interface; (void)version; (void)flags;
	return proxy;
}
static int test_no_listener(struct wl_proxy* proxy, void (**listener)(void), void* data) { (void)proxy; (void)listener; (void)data; return 0; }
static void test_no_proxy(struct wl_proxy* proxy) { (void)proxy; }
static uint32_t test_no_version(struct wl_proxy* proxy) { (void)proxy; return 1; }
static void test_no_set(struct wl_proxy* proxy, void* data) { (void)proxy; (void)data; }
static void* test_no_get(struct wl_proxy* proxy) { (void)proxy; return NULL; }
#define RVN_WL_MARSHAL test_no_marshal
#define RVN_WL_MARSHAL_FLAG_DESTROY 1
#define RVN_WL_ADD_LISTENER test_no_listener
#define RVN_WL_PROXY_DESTROY test_no_proxy
#define RVN_WL_GET_VERSION test_no_version
#define RVN_WL_SET_USER_DATA test_no_set
#define RVN_WL_GET_USER_DATA test_no_get

#include "wayland_protocols.h"

/* ---------------------------------------------------------------------------
 *  libwayland-server and libxkbcommon
 * ------------------------------------------------------------------------- */

struct wl_display;
struct wl_event_loop;
struct wl_client;
struct wl_resource;
struct wl_global;
typedef void (*wl_bind_fn)(struct wl_client* client, void* data, uint32_t version, uint32_t id);

static struct {
	void* library;
	struct wl_display* (*display_create)(void);
	void (*display_destroy)(struct wl_display* display);
	void (*display_destroy_clients)(struct wl_display* display);
	const char* (*display_add_socket_auto)(struct wl_display* display);
	struct wl_event_loop* (*display_get_event_loop)(struct wl_display* display);
	void (*display_flush_clients)(struct wl_display* display);
	uint32_t (*display_next_serial)(struct wl_display* display);
	int (*event_loop_dispatch)(struct wl_event_loop* loop, int timeout);
	struct wl_global* (*global_create)(struct wl_display* display, const struct wl_interface* interface, int version, void* data, wl_bind_fn bind);
	struct wl_resource* (*resource_create)(struct wl_client* client, const struct wl_interface* interface, int version, uint32_t id);
	void (*resource_set_implementation)(struct wl_resource* resource, const void* implementation, void* data, void (*destroy)(struct wl_resource*));
	void (*resource_post_event)(struct wl_resource* resource, uint32_t opcode, ...);
	void* (*resource_get_user_data)(struct wl_resource* resource);
	void (*resource_destroy)(struct wl_resource* resource);
	struct wl_client* (*resource_get_client)(struct wl_resource* resource);
	int (*resource_get_version)(struct wl_resource* resource);
} S;

struct xkb_rule_names { const char *rules, *model, *layout, *variant, *options; };
static struct {
	void* library;
	void* (*context_new)(int flags);
	void* (*keymap_new_from_names)(void* context, const struct xkb_rule_names* names, int flags);
	char* (*keymap_get_as_string)(void* keymap, int format);
	uint32_t (*keymap_mod_get_index)(void* keymap, const char* name);
	void (*keymap_unref)(void* keymap);
	void (*context_unref)(void* context);
} X;

static int test_symbol(void* library, const char* name, void* out) {
	void* symbol = dlsym(library, name);
	memcpy(out, &symbol, sizeof(void*));
	return symbol != NULL;
}

static int test_load(void) {
	int ok = 1;
	void* client = dlopen("libwayland-client.so.0", RTLD_NOW);
	S.library = dlopen("libwayland-server.so.0", RTLD_NOW);
	X.library = dlopen("libxkbcommon.so.0", RTLD_NOW);
	if (!client || !S.library || !X.library) return 0;
	dlclose(client);
	ok &= test_symbol(S.library, "wl_display_create", &S.display_create);
	ok &= test_symbol(S.library, "wl_display_destroy", &S.display_destroy);
	ok &= test_symbol(S.library, "wl_display_destroy_clients", &S.display_destroy_clients);
	ok &= test_symbol(S.library, "wl_display_add_socket_auto", &S.display_add_socket_auto);
	ok &= test_symbol(S.library, "wl_display_get_event_loop", &S.display_get_event_loop);
	ok &= test_symbol(S.library, "wl_display_flush_clients", &S.display_flush_clients);
	ok &= test_symbol(S.library, "wl_display_next_serial", &S.display_next_serial);
	ok &= test_symbol(S.library, "wl_event_loop_dispatch", &S.event_loop_dispatch);
	ok &= test_symbol(S.library, "wl_global_create", &S.global_create);
	ok &= test_symbol(S.library, "wl_resource_create", &S.resource_create);
	ok &= test_symbol(S.library, "wl_resource_set_implementation", &S.resource_set_implementation);
	ok &= test_symbol(S.library, "wl_resource_post_event", &S.resource_post_event);
	ok &= test_symbol(S.library, "wl_resource_get_user_data", &S.resource_get_user_data);
	ok &= test_symbol(S.library, "wl_resource_destroy", &S.resource_destroy);
	ok &= test_symbol(S.library, "wl_resource_get_client", &S.resource_get_client);
	ok &= test_symbol(S.library, "wl_resource_get_version", &S.resource_get_version);
	ok &= test_symbol(X.library, "xkb_context_new", &X.context_new);
	ok &= test_symbol(X.library, "xkb_keymap_new_from_names", &X.keymap_new_from_names);
	ok &= test_symbol(X.library, "xkb_keymap_get_as_string", &X.keymap_get_as_string);
	ok &= test_symbol(X.library, "xkb_keymap_mod_get_index", &X.keymap_mod_get_index);
	ok &= test_symbol(X.library, "xkb_keymap_unref", &X.keymap_unref);
	ok &= test_symbol(X.library, "xkb_context_unref", &X.context_unref);
	return ok;
}

/* ---------------------------------------------------------------------------
 *  The compositor
 *
 *  Each interface gets a table of request handlers built from its
 *  description: "destroy" and "release" destroy the resource, the others do
 *  nothing unless the test gives them something to do. (A handler that
 *  ignores its arguments is called with more than it declares; the C
 *  calling conventions of Linux allow it, and it is test code.)
 * ------------------------------------------------------------------------- */

typedef void (*test_handler)(void);

#define TEST_MAX_HANDLERS 32
typedef struct test_impl {
	const struct wl_interface* interface;
	test_handler handlers[TEST_MAX_HANDLERS];
} test_impl;

static void test_request_ignored(void) {}
static void test_request_destroy(struct wl_client* client, struct wl_resource* resource) {
	(void)client;
	S.resource_destroy(resource);
}

static void test_impl_init(test_impl* impl, const struct wl_interface* interface) {
	int i;
	impl->interface = interface;
	for (i = 0; i < interface->method_count && i < TEST_MAX_HANDLERS; i++) {
		const char* name = interface->methods[i].name;
		if (strcmp(name, "destroy") == 0 || strcmp(name, "release") == 0) impl->handlers[i] = (test_handler)test_request_destroy;
		else impl->handlers[i] = test_request_ignored;
	}
}

static int test_opcode(const struct wl_interface* interface, const char* name, int request) {
	int i, count = request ? interface->method_count : interface->event_count;
	const struct wl_message* messages = request ? interface->methods : interface->events;
	for (i = 0; i < count; i++) if (strcmp(messages[i].name, name) == 0) return i;
	printf("  (no %s %s.%s)\n", request ? "request" : "event", interface->name, name);
	abort();
}

static void test_on(test_impl* impl, const char* request, test_handler handler) {
	impl->handlers[test_opcode(impl->interface, request, 1)] = handler;
}

#define EVENT(resource, interface, name, ...) S.resource_post_event((resource), (uint32_t)test_opcode(&rvn_##interface##_interface, name, 0), __VA_ARGS__)
#define EVENT0(resource, interface, name) S.resource_post_event((resource), (uint32_t)test_opcode(&rvn_##interface##_interface, name, 0))

static struct wl_resource* test_create(struct wl_client* client, test_impl* impl, uint32_t version, uint32_t id, void* data) {
	struct wl_resource* resource = S.resource_create(client, impl->interface, (int)version, id);
	S.resource_set_implementation(resource, impl->handlers, data, NULL);
	return resource;
}

static test_impl I_compositor, I_surface, I_region, I_subcompositor, I_subsurface, I_shm, I_seat, I_pointer, I_keyboard,
	I_output, I_wm_base, I_xdg_surface, I_toplevel, I_decoration_manager, I_decoration, I_viewporter, I_viewport,
	I_fractional_manager, I_fractional, I_shape_manager, I_shape_device, I_ddm, I_data_device, I_data_source, I_data_offer;

/* What the compositor saw, and what the test asks it to do: shared between the two threads. */
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static struct {
	struct wl_display* display;
	char socket[64];
	int ready, stop;

	struct wl_resource* surface;          /* of the toplevel */
	struct wl_resource* xdg_surface;
	struct wl_resource* toplevel;
	struct wl_resource* fractional;
	struct wl_resource* pointer;
	struct wl_resource* keyboard;
	struct wl_resource* data_device;
	struct wl_resource* shape_device;
	struct wl_resource* selection_source;
	struct wl_client* client;

	char title[128];
	char app_id[128];
	int toplevels_made;
	int decoration_mode;
	uint32_t configure_serial, acked_serial;
	int null_attached;
	int viewport_width, viewport_height;
	uint32_t last_shape;
	uint32_t shape_serial, enter_serial;
	uint32_t button_serial, move_serial;
	int moves;
	char copied[256];
	int copied_done;
	char selection_mimes[256];

	const char* offer_text;    /* what our offers hold when they are read */

	/* a command from the test thread, run by the compositor thread */
	void (*command)(void*);
	void* argument;
	int command_done;
} C;

static char* g_keymap;
static uint32_t g_shift_mask;

/* ---- wl_surface ---- */

static void surface_attach(struct wl_client* client, struct wl_resource* resource, struct wl_resource* buffer, int32_t x, int32_t y) {
	(void)client; (void)resource; (void)x; (void)y;
	if (!buffer && resource == C.surface) C.null_attached++;
}

static void surface_commit(struct wl_client* client, struct wl_resource* resource) {
	(void)client;
	/* the initial commit of a toplevel: configure it */
	if (resource == C.surface && C.toplevel && C.configure_serial == 0) {
		struct wl_array states = { 0, 0, NULL };
		EVENT(C.toplevel, xdg_toplevel, "configure", 0, 0, &states);
		C.configure_serial = S.display_next_serial(C.display);
		EVENT(C.xdg_surface, xdg_surface, "configure", C.configure_serial);
	}
}

static void compositor_create_surface(struct wl_client* client, struct wl_resource* resource, uint32_t id) {
	test_create(client, &I_surface, (uint32_t)S.resource_get_version(resource), id, NULL);
}

static void compositor_create_region(struct wl_client* client, struct wl_resource* resource, uint32_t id) {
	test_create(client, &I_region, 1, id, NULL);
	(void)resource;
}

static void bind_compositor(struct wl_client* client, void* data, uint32_t version, uint32_t id) {
	(void)data;
	C.client = client;
	test_create(client, &I_compositor, version, id, NULL);
}

static void subcompositor_get_subsurface(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* surface, struct wl_resource* parent) {
	(void)resource; (void)surface; (void)parent;
	test_create(client, &I_subsurface, 1, id, NULL);
}
static void bind_subcompositor(struct wl_client* client, void* data, uint32_t version, uint32_t id) {
	(void)data;
	test_create(client, &I_subcompositor, version, id, NULL);
}
static void bind_shm(struct wl_client* client, void* data, uint32_t version, uint32_t id) {
	struct wl_resource* shm = test_create(client, &I_shm, version, id, NULL);
	(void)data;
	EVENT(shm, wl_shm, "format", 0u);
	EVENT(shm, wl_shm, "format", 1u);
}

/* ---- seat ---- */

static void seat_get_pointer(struct wl_client* client, struct wl_resource* resource, uint32_t id) {
	C.pointer = test_create(client, &I_pointer, (uint32_t)S.resource_get_version(resource), id, NULL);
}

static void seat_get_keyboard(struct wl_client* client, struct wl_resource* resource, uint32_t id) {
	size_t size = strlen(g_keymap) + 1;
	int fd = (int)memfd_create("keymap", MFD_CLOEXEC);
	C.keyboard = test_create(client, &I_keyboard, (uint32_t)S.resource_get_version(resource), id, NULL);
	if (fd >= 0 && write(fd, g_keymap, size) == (ssize_t)size) {
		EVENT(C.keyboard, wl_keyboard, "keymap", 1u, fd, (uint32_t)size);
	}
	if (fd >= 0) close(fd);
	EVENT(C.keyboard, wl_keyboard, "repeat_info", 40, 60);
}

static void bind_seat(struct wl_client* client, void* data, uint32_t version, uint32_t id) {
	struct wl_resource* seat = test_create(client, &I_seat, version, id, NULL);
	(void)data;
	EVENT(seat, wl_seat, "capabilities", 3u);   /* pointer | keyboard */
	if (version >= 2) EVENT(seat, wl_seat, "name", "seat0");
}

static void bind_output(struct wl_client* client, void* data, uint32_t version, uint32_t id) {
	struct wl_resource* output = test_create(client, &I_output, version, id, NULL);
	(void)data;
	EVENT(output, wl_output, "geometry", 0, 0, 600, 340, 0, "Raven", "Test", 0);
	EVENT(output, wl_output, "mode", 3u, 1920, 1080, 60000);
	if (version >= 2) EVENT(output, wl_output, "scale", 1);
	if (version >= 2) EVENT0(output, wl_output, "done");
}

/* ---- xdg-shell ---- */

static void toplevel_set_title(struct wl_client* client, struct wl_resource* resource, const char* title) {
	(void)client; (void)resource;
	snprintf(C.title, sizeof(C.title), "%s", title);
}
static void toplevel_set_app_id(struct wl_client* client, struct wl_resource* resource, const char* app_id) {
	(void)client; (void)resource;
	snprintf(C.app_id, sizeof(C.app_id), "%s", app_id);
}
static void toplevel_move(struct wl_client* client, struct wl_resource* resource, struct wl_resource* seat, uint32_t serial) {
	(void)client; (void)resource; (void)seat;
	C.move_serial = serial;
	C.moves++;
}
static void toplevel_destroy(struct wl_client* client, struct wl_resource* resource) {
	(void)client;
	if (resource == C.toplevel) C.toplevel = NULL;
	S.resource_destroy(resource);
}
static void srv_xdg_surface_get_toplevel(struct wl_client* client, struct wl_resource* resource, uint32_t id) {
	C.toplevel = test_create(client, &I_toplevel, (uint32_t)S.resource_get_version(resource), id, NULL);
	C.xdg_surface = resource;
	C.configure_serial = 0;
	C.toplevels_made++;
}
static void xdg_surface_ack(struct wl_client* client, struct wl_resource* resource, uint32_t serial) {
	(void)client; (void)resource;
	C.acked_serial = serial;
}
static void srv_xdg_surface_destroy(struct wl_client* client, struct wl_resource* resource) {
	(void)client;
	if (resource == C.xdg_surface) C.xdg_surface = NULL;
	S.resource_destroy(resource);
}
static void wm_base_get_xdg_surface(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* surface) {
	test_create(client, &I_xdg_surface, (uint32_t)S.resource_get_version(resource), id, NULL);
	C.surface = surface;
}
static void bind_wm_base(struct wl_client* client, void* data, uint32_t version, uint32_t id) {
	(void)data;
	test_create(client, &I_wm_base, version, id, NULL);
}

/* ---- decorations, viewports, fractional scale, cursor shapes ---- */

static struct wl_resource* g_decoration;
static void decoration_set_mode(struct wl_client* client, struct wl_resource* resource, uint32_t mode) {
	(void)client;
	C.decoration_mode = (int)mode;
	EVENT(resource, zxdg_toplevel_decoration_v1, "configure", mode);
}
static void decoration_manager_get(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* toplevel) {
	(void)resource; (void)toplevel;
	g_decoration = test_create(client, &I_decoration, 1, id, NULL);
}
static void bind_decoration_manager(struct wl_client* client, void* data, uint32_t version, uint32_t id) {
	(void)data;
	test_create(client, &I_decoration_manager, version, id, NULL);
}

static void viewport_set_destination(struct wl_client* client, struct wl_resource* resource, int32_t width, int32_t height) {
	(void)client; (void)resource;
	C.viewport_width = width;
	C.viewport_height = height;
}
static void viewporter_get(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* surface) {
	(void)resource; (void)surface;
	test_create(client, &I_viewport, 1, id, NULL);
}
static void bind_viewporter(struct wl_client* client, void* data, uint32_t version, uint32_t id) {
	(void)data;
	test_create(client, &I_viewporter, version, id, NULL);
}

static void fractional_get(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* surface) {
	struct wl_resource* fractional = test_create(client, &I_fractional, 1, id, NULL);
	(void)resource; (void)surface;
	if (!C.fractional) C.fractional = fractional;
}
static void bind_fractional(struct wl_client* client, void* data, uint32_t version, uint32_t id) {
	(void)data;
	test_create(client, &I_fractional_manager, version, id, NULL);
}

static void shape_set_shape(struct wl_client* client, struct wl_resource* resource, uint32_t serial, uint32_t shape) {
	(void)client; (void)resource;
	C.shape_serial = serial;
	C.last_shape = shape;
}
static void shape_get_pointer(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* pointer) {
	(void)resource; (void)pointer;
	C.shape_device = test_create(client, &I_shape_device, 1, id, NULL);
}
static void bind_shape_manager(struct wl_client* client, void* data, uint32_t version, uint32_t id) {
	(void)data;
	test_create(client, &I_shape_manager, version, id, NULL);
}

/* ---- data devices ---- */

static void source_offer(struct wl_client* client, struct wl_resource* resource, const char* mime) {
	(void)client; (void)resource;
	if (strlen(C.selection_mimes) + strlen(mime) + 2 < sizeof(C.selection_mimes)) {
		strcat(C.selection_mimes, mime);
		strcat(C.selection_mimes, " ");
	}
}
static void ddm_create_source(struct wl_client* client, struct wl_resource* resource, uint32_t id) {
	test_create(client, &I_data_source, (uint32_t)S.resource_get_version(resource), id, NULL);
}
static void device_set_selection(struct wl_client* client, struct wl_resource* resource, struct wl_resource* source, uint32_t serial) {
	(void)client; (void)resource; (void)serial;
	C.selection_source = source;
}
static void ddm_get_device(struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* seat) {
	(void)seat;
	C.data_device = test_create(client, &I_data_device, (uint32_t)S.resource_get_version(resource), id, NULL);
}
static void bind_ddm(struct wl_client* client, void* data, uint32_t version, uint32_t id) {
	(void)data;
	test_create(client, &I_ddm, version, id, NULL);
}
/* Raven reads one of our offers: another application answers. */
static void offer_receive(struct wl_client* client, struct wl_resource* resource, const char* mime, int32_t fd) {
	const char* text = C.offer_text ? C.offer_text : "";
	(void)client; (void)resource; (void)mime;
	if (write(fd, text, strlen(text)) < 0) {}
	close(fd);
}

static void* compositor_thread(void* unused) {
	struct wl_event_loop* loop;
	const char* socket;
	(void)unused;
	C.display = S.display_create();
	socket = S.display_add_socket_auto(C.display);
	snprintf(C.socket, sizeof(C.socket), "%s", socket ? socket : "");
	loop = S.display_get_event_loop(C.display);

	S.global_create(C.display, &rvn_wl_compositor_interface, 6, NULL, bind_compositor);
	S.global_create(C.display, &rvn_wl_subcompositor_interface, 1, NULL, bind_subcompositor);
	S.global_create(C.display, &rvn_wl_shm_interface, 1, NULL, bind_shm);
	S.global_create(C.display, &rvn_wl_seat_interface, 8, NULL, bind_seat);
	S.global_create(C.display, &rvn_wl_output_interface, 4, NULL, bind_output);
	S.global_create(C.display, &rvn_xdg_wm_base_interface, 5, NULL, bind_wm_base);
	S.global_create(C.display, &rvn_zxdg_decoration_manager_v1_interface, 1, NULL, bind_decoration_manager);
	S.global_create(C.display, &rvn_wp_viewporter_interface, 1, NULL, bind_viewporter);
	S.global_create(C.display, &rvn_wp_fractional_scale_manager_v1_interface, 1, NULL, bind_fractional);
	S.global_create(C.display, &rvn_wp_cursor_shape_manager_v1_interface, 1, NULL, bind_shape_manager);
	S.global_create(C.display, &rvn_wl_data_device_manager_interface, 3, NULL, bind_ddm);

	pthread_mutex_lock(&g_lock);
	C.ready = 1;
	pthread_mutex_unlock(&g_lock);
	for (;;) {
		int stop;
		S.event_loop_dispatch(loop, 5);
		pthread_mutex_lock(&g_lock);
		if (C.command) {
			C.command(C.argument);
			C.command = NULL;
			C.command_done = 1;
		}
		stop = C.stop;
		pthread_mutex_unlock(&g_lock);
		S.display_flush_clients(C.display);
		if (stop) break;
	}
	S.display_destroy_clients(C.display);
	S.display_destroy(C.display);
	return NULL;
}

/* Runs a function on the compositor thread and waits for it, while Raven keeps answering. */
static void compositor_do(void (*command)(void*), void* argument) {
	int done = 0, i;
	pthread_mutex_lock(&g_lock);
	C.command = command;
	C.argument = argument;
	C.command_done = 0;
	pthread_mutex_unlock(&g_lock);
	for (i = 0; i < 400 && !done; i++) {
		Rwait_frame(0.005);
		pthread_mutex_lock(&g_lock);
		done = C.command_done;
		pthread_mutex_unlock(&g_lock);
	}
}

/* Lets the two sides talk for a while. */
static void settle(int milliseconds) {
	struct timespec start, now;
	clock_gettime(CLOCK_MONOTONIC, &start);
	do {
		Rwait_frame(0.005);
		clock_gettime(CLOCK_MONOTONIC, &now);
	} while ((now.tv_sec - start.tv_sec) * 1000 + (now.tv_nsec - start.tv_nsec) / 1000000 < milliseconds);
}

/* ---------------------------------------------------------------------------
 *  Input sent by the compositor
 * ------------------------------------------------------------------------- */

typedef struct point { double x, y; } point;

static void do_pointer_enter(void* argument) {
	point* p = (point*)argument;
	C.enter_serial = S.display_next_serial(C.display);
	EVENT(C.pointer, wl_pointer, "enter", C.enter_serial, C.surface, (int32_t)(p->x * 256.0), (int32_t)(p->y * 256.0));
	EVENT0(C.pointer, wl_pointer, "frame");
}

static void do_pointer_click(void* unused) {
	(void)unused;
	C.button_serial = S.display_next_serial(C.display);
	EVENT(C.pointer, wl_pointer, "button", C.button_serial, 1u, 0x110u, 1u);
	EVENT0(C.pointer, wl_pointer, "frame");
	EVENT(C.pointer, wl_pointer, "button", S.display_next_serial(C.display), 2u, 0x110u, 0u);
	EVENT0(C.pointer, wl_pointer, "frame");
}

static void do_pointer_press(void* unused) {
	(void)unused;
	C.button_serial = S.display_next_serial(C.display);
	EVENT(C.pointer, wl_pointer, "button", C.button_serial, 1u, 0x110u, 1u);
	EVENT0(C.pointer, wl_pointer, "frame");
}

static void do_wheel(void* unused) {
	(void)unused;
	EVENT(C.pointer, wl_pointer, "axis_source", 0u);
	EVENT(C.pointer, wl_pointer, "axis_value120", 0u, 120);
	EVENT(C.pointer, wl_pointer, "axis", 3u, 0u, 10 * 256);
	EVENT0(C.pointer, wl_pointer, "frame");
}

static void do_keyboard_enter(void* unused) {
	struct wl_array keys = { 0, 0, NULL };
	(void)unused;
	EVENT(C.keyboard, wl_keyboard, "enter", S.display_next_serial(C.display), C.surface, &keys);
	EVENT(C.keyboard, wl_keyboard, "modifiers", S.display_next_serial(C.display), 0u, 0u, 0u, 0u);
}

typedef struct key_action { uint32_t key; int pressed; uint32_t modifiers; } key_action;

static void do_key(void* argument) {
	key_action* k = (key_action*)argument;
	EVENT(C.keyboard, wl_keyboard, "modifiers", S.display_next_serial(C.display), k->modifiers, 0u, 0u, 0u);
	EVENT(C.keyboard, wl_keyboard, "key", S.display_next_serial(C.display), 0u, k->key, k->pressed ? 1u : 0u);
}

static void type_key(uint32_t key, uint32_t modifiers) {
	key_action press = { key, 1, modifiers }, release = { key, 0, modifiers };
	compositor_do(do_key, &press);
	compositor_do(do_key, &release);
}

typedef struct configure_args { int width, height, maximized; } configure_args;

static void do_configure(void* argument) {
	configure_args* a = (configure_args*)argument;
	uint32_t maximized = 1;   /* XDG_TOPLEVEL_STATE_MAXIMIZED */
	struct wl_array states = { 0, 0, NULL };
	if (a->maximized) {
		states.size = states.alloc = sizeof(uint32_t);
		states.data = &maximized;
	}
	EVENT(C.toplevel, xdg_toplevel, "configure", a->width, a->height, &states);
	C.configure_serial = S.display_next_serial(C.display);
	EVENT(C.xdg_surface, xdg_surface, "configure", C.configure_serial);
}

static void do_scale(void* argument) {
	EVENT(C.fractional, wp_fractional_scale_v1, "preferred_scale", *(uint32_t*)argument);
}

static void do_close(void* unused) {
	(void)unused;
	EVENT0(C.toplevel, xdg_toplevel, "close");
}

/* The selection we were given: read it, as another application would. */
static void do_read_selection(void* unused) {
	int fds[2];
	ssize_t count;
	(void)unused;
	C.copied[0] = '\0';
	C.copied_done = 0;
	if (!C.selection_source || pipe(fds) != 0) {
		C.copied_done = 1;
		return;
	}
	EVENT(C.selection_source, wl_data_source, "send", "text/plain;charset=utf-8", fds[1]);
	close(fds[1]);
	S.display_flush_clients(C.display);
	/* (Raven writes it when it dispatches the event, on the test thread: wait for it) */
	C.command_done = 1;
	pthread_mutex_unlock(&g_lock);
	{
		size_t used = 0;
		while (used + 1 < sizeof(C.copied)) {
			count = read(fds[0], C.copied + used, sizeof(C.copied) - 1 - used);
			if (count <= 0) break;
			used += (size_t)count;
		}
		C.copied[used] = '\0';
	}
	pthread_mutex_lock(&g_lock);
	C.copied_done = 1;
	close(fds[0]);
}

/* Another application takes the clipboard. */
static void do_offer_selection(void* unused) {
	struct wl_resource* offer = S.resource_create(C.client, &rvn_wl_data_offer_interface, S.resource_get_version(C.data_device), 0);
	(void)unused;
	S.resource_set_implementation(offer, I_data_offer.handlers, NULL, NULL);
	EVENT(C.data_device, wl_data_device, "data_offer", offer);
	EVENT(offer, wl_data_offer, "offer", "text/plain;charset=utf-8");
	EVENT(offer, wl_data_offer, "offer", "text/plain");
	if (C.selection_source) EVENT0(C.selection_source, wl_data_source, "cancelled");
	C.selection_source = NULL;
	EVENT(C.data_device, wl_data_device, "selection", offer);
}

/* Files from another application, dragged over the window and dropped. */
static void do_drop(void* argument) {
	point* p = (point*)argument;
	struct wl_resource* offer = S.resource_create(C.client, &rvn_wl_data_offer_interface, S.resource_get_version(C.data_device), 0);
	S.resource_set_implementation(offer, I_data_offer.handlers, NULL, NULL);
	EVENT(C.data_device, wl_data_device, "data_offer", offer);
	EVENT(offer, wl_data_offer, "offer", "text/uri-list");
	EVENT(offer, wl_data_offer, "source_actions", 1u);
	EVENT(C.data_device, wl_data_device, "enter", S.display_next_serial(C.display), C.surface,
		(int32_t)(p->x * 256.0), (int32_t)(p->y * 256.0), offer);
	EVENT(C.data_device, wl_data_device, "motion", 0u, (int32_t)((p->x + 1.0) * 256.0), (int32_t)(p->y * 256.0));
}

static void do_drop_release(void* unused) {
	(void)unused;
	EVENT0(C.data_device, wl_data_device, "drop");
}

/* ---------------------------------------------------------------------------
 *  The application side
 * ------------------------------------------------------------------------- */

static int g_clicks, g_closes, g_drops, g_wheels;
static char g_dropped[256];

static void on_go(Rid sender, const char* action, void* user) {
	(void)sender; (void)action; (void)user;
	g_clicks++;
}

static int on_close(Rid window, void* user) {
	(void)window; (void)user;
	g_closes++;
	return R_FALSE;   /* keep the window */
}

static int on_drop(Rid target, const RDropEvent* event, void* user) {
	(void)target; (void)user;
	if (event->phase == R_DROP_DROPPED) {
		g_drops++;
		snprintf(g_dropped, sizeof(g_dropped), "%s", event->path_count > 0 ? event->paths[0] : "");
	}
	return R_TRUE;
}

static float g_wheel_y;

/* sees every event of the window before its widgets */
static int on_event(Rid window, const REvent* event, void* user) {
	(void)window; (void)user;
	if (event->type == R_EVENT_MOUSE_WHEEL) {
		g_wheels++;
		g_wheel_y = event->wheel_y;
	}
	return R_FALSE;
}

static void center_of(Rid widget, point* p) {
	float x, y, w, h;
	Rwidget_get_rect(widget, &x, &y, &w, &h);
	p->x = x + w * 0.5f;
	p->y = y + h * 0.5f;
}

static const char* XML_WINDOW =
	"<Window Title=\"Raven on Wayland\" Width=\"400\" Height=\"300\">"
	"  <VBox Spacing=\"8\" Slot.Padding=\"12\">"
	"    <TextInput Name=\"input\"/>"
	"    <Button Name=\"go\" Text=\"Go\" OnClicked=\"go\"/>"
	"    <Label Name=\"label\" Text=\"wheel here\" Slot.Fill=\"1\"/>"
	"  </VBox>"
	"</Window>";

static void test_wayland(void) {
	RInitDesc desc;
	RNativeWindow native;
	Rid window, input, go, label;
	point p;
	int width = 0, height = 0;
	configure_args resize = { 500, 350, 0 }, maximize = { 500, 350, 1 }, restore = { 500, 350, 0 };
	uint32_t scale150 = 180;
	int i;

	memset(&desc, 0, sizeof(desc));
	desc.backend = R_BACKEND_NULL;
	desc.headless = R_FALSE;
	Rerror_set_callback(test_error_callback);
	CHECK(Rinit_ex(&desc));
	if (!Ris_initialized()) return;
	Rset_animations_enabled(R_FALSE);

	window = Rwindow_create_from_xml(XML_WINDOW, strlen(XML_WINDOW));
	input = Rwidget_find(window, "input");
	go = Rwidget_find(window, "go");
	label = Rwidget_find(window, "label");
	Raction_bind("go", on_go, NULL);
	Rwindow_set_close_callback(window, on_close, NULL);
	Rwindow_set_drop_callback(window, on_drop, NULL);
	Rwindow_set_event_hook(window, on_event, NULL);
	settle(50);

	/* the window, as the compositor knows it */
	CHECK(Rwindow_get_native_handle(window, &native));
	CHECK(native.system == R_WINDOW_SYSTEM_WAYLAND);
	pthread_mutex_lock(&g_lock);
	CHECK_STR(C.title, "Raven on Wayland");
	CHECK_STR(C.app_id, "org.raven.test");
	CHECK(C.toplevels_made == 1);
	CHECK(C.configure_serial != 0 && C.acked_serial == C.configure_serial);
	CHECK(C.decoration_mode == 2);   /* server side */
	CHECK(C.viewport_width == 400 && C.viewport_height == 300);
	pthread_mutex_unlock(&g_lock);
	Rwindow_get_size(window, &width, &height);
	CHECK(width == 400 && height == 300);
	CHECK_NEAR(Rwindow_get_scale(window), 1.0);

	/* a click on the button */
	center_of(go, &p);
	compositor_do(do_pointer_enter, &p);
	compositor_do(do_pointer_click, NULL);
	settle(20);
	CHECK(g_clicks == 1);

	/* the cursor follows what is under the pointer */
	center_of(input, &p);
	compositor_do(do_pointer_enter, &p);
	settle(20);
	pthread_mutex_lock(&g_lock);
	CHECK(C.last_shape == WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_TEXT);
	CHECK(C.shape_serial == C.enter_serial);
	pthread_mutex_unlock(&g_lock);

	/* typing: click into the field, then keys (evdev codes), with Shift */
	compositor_do(do_pointer_click, NULL);
	compositor_do(do_keyboard_enter, NULL);
	type_key(35, 0);              /* h */
	type_key(23, 0);              /* i */
	type_key(30, g_shift_mask);   /* A */
	settle(20);
	CHECK_STR(Rtext_input_get_text(input), "hiA");

	/* a dead key of the international layout, then e */
	type_key(40, 0);   /* ' : dead acute in us(intl) */
	type_key(18, 0);   /* e */
	settle(20);
	CHECK_STR(Rtext_input_get_text(input), "hiA\xC3\xA9");

	/* a held key repeats (40 per second after 60 ms) */
	{
		key_action press = { 48, 1, 0 }, release = { 48, 0, 0 };   /* b */
		const char* text;
		int count = 0;
		compositor_do(do_key, &press);
		settle(250);
		compositor_do(do_key, &release);
		settle(20);
		for (text = Rtext_input_get_text(input); *text; text++) count += *text == 'b';
		CHECK(count >= 4);
	}

	/* the wheel */
	center_of(label, &p);
	compositor_do(do_pointer_enter, &p);
	compositor_do(do_wheel, NULL);
	settle(20);
	CHECK(g_wheels == 1);
	CHECK_NEAR(g_wheel_y, -1.0);   /* one notch toward the user: down */

	/* the compositor resizes the window, maximizes it, restores it */
	compositor_do(do_configure, &resize);
	settle(20);
	Rwindow_get_size(window, &width, &height);
	CHECK(width == 500 && height == 350);
	CHECK(C.acked_serial == C.configure_serial);
	compositor_do(do_configure, &maximize);
	settle(20);
	CHECK(Rwindow_is_maximized(window));
	compositor_do(do_configure, &restore);
	settle(20);
	CHECK(!Rwindow_is_maximized(window));

	/* a fractional scale: 1.5, the same logical size, a buffer of 750 x 525 */
	compositor_do(do_scale, &scale150);
	settle(20);
	CHECK_NEAR(Rwindow_get_scale(window), 1.5);
	Rwindow_get_size(window, &width, &height);
	CHECK(width == 500 && height == 350);
	Rwindow_get_framebuffer_size(window, &width, &height);
	CHECK(width == 750 && height == 525);
	pthread_mutex_lock(&g_lock);
	CHECK(C.viewport_width == 500 && C.viewport_height == 350);
	pthread_mutex_unlock(&g_lock);
	/* and a click lands where it should at that scale */
	center_of(go, &p);
	compositor_do(do_pointer_enter, &p);
	compositor_do(do_pointer_click, NULL);
	settle(20);
	CHECK(g_clicks == 2);

	/* the clipboard, both ways */
	Rclipboard_set_text("copi\xC3\xA9");
	settle(20);
	pthread_mutex_lock(&g_lock);
	CHECK(C.selection_source != NULL);
	CHECK(strstr(C.selection_mimes, "text/plain;charset=utf-8") != NULL);
	pthread_mutex_unlock(&g_lock);
	compositor_do(do_read_selection, NULL);
	{
		int done = 0;
		for (i = 0; i < 200 && !done; i++) {
			settle(5);
			pthread_mutex_lock(&g_lock);
			done = C.copied_done;
			pthread_mutex_unlock(&g_lock);
		}
	}
	CHECK_STR(C.copied, "copi\xC3\xA9");
	C.offer_text = "from another application";
	compositor_do(do_offer_selection, NULL);
	settle(20);
	CHECK_STR(Rclipboard_get_text(), "from another application");

	/* files dropped from another application */
	C.offer_text = "# a comment\r\nfile:///tmp/a%20b.txt\r\n";
	center_of(label, &p);
	compositor_do(do_drop, &p);
	settle(20);
	compositor_do(do_drop_release, NULL);
	settle(50);
	CHECK(g_drops == 1);
	CHECK_STR(g_dropped, "/tmp/a b.txt");

	/* moving a window: the compositor gets the serial of the press */
	compositor_do(do_pointer_press, NULL);
	settle(10);
	Rwindow_begin_move(window);
	settle(20);
	pthread_mutex_lock(&g_lock);
	CHECK(C.moves == 1 && C.move_serial == C.button_serial);
	pthread_mutex_unlock(&g_lock);

	/* hidden: unmapped and without its role; shown: a new window, configured again */
	Rwindow_hide(window);
	settle(20);
	pthread_mutex_lock(&g_lock);
	CHECK(C.null_attached >= 1 && C.toplevel == NULL);
	pthread_mutex_unlock(&g_lock);
	Rwindow_show(window);
	settle(20);
	pthread_mutex_lock(&g_lock);
	CHECK(C.toplevels_made == 2 && C.toplevel != NULL);
	CHECK(C.acked_serial == C.configure_serial);
	pthread_mutex_unlock(&g_lock);

	/* the close button of the compositor */
	compositor_do(do_close, NULL);
	settle(20);
	CHECK(g_closes == 1);
	CHECK(Rwindow_is_valid(window));

	Rshutdown();
}

int main(void) {
	pthread_t thread;
	char runtime[] = "/tmp/raven-wayland-XXXXXX";
	struct xkb_rule_names names = { NULL, NULL, "us", "intl", NULL };
	void* context;
	void* keymap;
	int ready = 0, i, result;

	if (!test_load()) {
		printf("test_wayland: skipped (libwayland-server, libwayland-client or libxkbcommon missing)\n");
		return 77;
	}
	test_impl_init(&I_compositor, &rvn_wl_compositor_interface);
	test_on(&I_compositor, "create_surface", (test_handler)compositor_create_surface);
	test_on(&I_compositor, "create_region", (test_handler)compositor_create_region);
	test_impl_init(&I_surface, &rvn_wl_surface_interface);
	test_on(&I_surface, "attach", (test_handler)surface_attach);
	test_on(&I_surface, "commit", (test_handler)surface_commit);
	test_impl_init(&I_region, &rvn_wl_region_interface);
	test_impl_init(&I_subcompositor, &rvn_wl_subcompositor_interface);
	test_on(&I_subcompositor, "get_subsurface", (test_handler)subcompositor_get_subsurface);
	test_impl_init(&I_subsurface, &rvn_wl_subsurface_interface);
	test_impl_init(&I_shm, &rvn_wl_shm_interface);
	test_impl_init(&I_seat, &rvn_wl_seat_interface);
	test_on(&I_seat, "get_pointer", (test_handler)seat_get_pointer);
	test_on(&I_seat, "get_keyboard", (test_handler)seat_get_keyboard);
	test_impl_init(&I_pointer, &rvn_wl_pointer_interface);
	test_impl_init(&I_keyboard, &rvn_wl_keyboard_interface);
	test_impl_init(&I_output, &rvn_wl_output_interface);
	test_impl_init(&I_wm_base, &rvn_xdg_wm_base_interface);
	test_on(&I_wm_base, "get_xdg_surface", (test_handler)wm_base_get_xdg_surface);
	test_impl_init(&I_xdg_surface, &rvn_xdg_surface_interface);
	test_on(&I_xdg_surface, "get_toplevel", (test_handler)srv_xdg_surface_get_toplevel);
	test_on(&I_xdg_surface, "ack_configure", (test_handler)xdg_surface_ack);
	test_on(&I_xdg_surface, "destroy", (test_handler)srv_xdg_surface_destroy);
	test_impl_init(&I_toplevel, &rvn_xdg_toplevel_interface);
	test_on(&I_toplevel, "set_title", (test_handler)toplevel_set_title);
	test_on(&I_toplevel, "set_app_id", (test_handler)toplevel_set_app_id);
	test_on(&I_toplevel, "move", (test_handler)toplevel_move);
	test_on(&I_toplevel, "destroy", (test_handler)toplevel_destroy);
	test_impl_init(&I_decoration_manager, &rvn_zxdg_decoration_manager_v1_interface);
	test_on(&I_decoration_manager, "get_toplevel_decoration", (test_handler)decoration_manager_get);
	test_impl_init(&I_decoration, &rvn_zxdg_toplevel_decoration_v1_interface);
	test_on(&I_decoration, "set_mode", (test_handler)decoration_set_mode);
	test_impl_init(&I_viewporter, &rvn_wp_viewporter_interface);
	test_on(&I_viewporter, "get_viewport", (test_handler)viewporter_get);
	test_impl_init(&I_viewport, &rvn_wp_viewport_interface);
	test_on(&I_viewport, "set_destination", (test_handler)viewport_set_destination);
	test_impl_init(&I_fractional_manager, &rvn_wp_fractional_scale_manager_v1_interface);
	test_on(&I_fractional_manager, "get_fractional_scale", (test_handler)fractional_get);
	test_impl_init(&I_fractional, &rvn_wp_fractional_scale_v1_interface);
	test_impl_init(&I_shape_manager, &rvn_wp_cursor_shape_manager_v1_interface);
	test_on(&I_shape_manager, "get_pointer", (test_handler)shape_get_pointer);
	test_impl_init(&I_shape_device, &rvn_wp_cursor_shape_device_v1_interface);
	test_on(&I_shape_device, "set_shape", (test_handler)shape_set_shape);
	test_impl_init(&I_ddm, &rvn_wl_data_device_manager_interface);
	test_on(&I_ddm, "create_data_source", (test_handler)ddm_create_source);
	test_on(&I_ddm, "get_data_device", (test_handler)ddm_get_device);
	test_impl_init(&I_data_device, &rvn_wl_data_device_interface);
	test_on(&I_data_device, "set_selection", (test_handler)device_set_selection);
	test_impl_init(&I_data_source, &rvn_wl_data_source_interface);
	test_on(&I_data_source, "offer", (test_handler)source_offer);
	test_impl_init(&I_data_offer, &rvn_wl_data_offer_interface);
	test_on(&I_data_offer, "receive", (test_handler)offer_receive);

	/* a keymap with dead keys, as a compositor would send it */
	context = X.context_new(0);
	keymap = context ? X.keymap_new_from_names(context, &names, 0) : NULL;
	if (!keymap) {
		printf("test_wayland: skipped (no keyboard layouts: xkeyboard-config is missing)\n");
		return 77;
	}
	g_keymap = X.keymap_get_as_string(keymap, 1);
	g_shift_mask = 1u << X.keymap_mod_get_index(keymap, "Shift");
	X.keymap_unref(keymap);
	X.context_unref(context);

	/* our own session: a runtime folder, no X11, Wayland asked for */
	if (!getenv("XDG_RUNTIME_DIR") || access(getenv("XDG_RUNTIME_DIR"), W_OK) != 0) {
		if (!mkdtemp(runtime)) return 1;
		setenv("XDG_RUNTIME_DIR", runtime, 1);
	}
	setenv("LANG", "C.UTF-8", 1);
	unsetenv("LC_ALL");
	unsetenv("LC_CTYPE");
	unsetenv("DISPLAY");
	setenv("RAVEN_APP_ID", "org.raven.test", 1);
	setenv("RAVEN_WINDOW_SYSTEM", "wayland", 1);
	pthread_create(&thread, NULL, compositor_thread, NULL);
	for (i = 0; i < 1000 && !ready; i++) {
		struct timespec pause = { 0, 2000000 };
		nanosleep(&pause, NULL);
		pthread_mutex_lock(&g_lock);
		ready = C.ready;
		pthread_mutex_unlock(&g_lock);
	}
	if (!ready || !C.socket[0]) {
		printf("test_wayland: the test compositor did not start\n");
		return 1;
	}
	setenv("WAYLAND_DISPLAY", C.socket, 1);

	RUN(test_wayland);

	pthread_mutex_lock(&g_lock);
	C.stop = 1;
	pthread_mutex_unlock(&g_lock);
	pthread_join(thread, NULL);
	free(g_keymap);
	result = test_report("test_wayland");
	return result;
}

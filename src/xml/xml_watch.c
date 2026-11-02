/**
 * Raven - xml/xml_watch.c
 * Live reload: the interfaces built from files follow their files.
 *
 * Watched files use native OS notifications (ReadDirectoryChangesW on Windows,
 * kqueue on macOS and inotify on Linux). File stamps are checked only after a
 * notification, with a short retry timer if an editor is temporarily writing
 * or replacing the file. When one changed, it is read again and compared with the document the interface was
 * built from:
 *
 *   - an element that is still there keeps its widget: the handles the
 *     application holds stay valid, and so do the callbacks, what the user
 *     typed, the scroll positions. Only the attributes that changed are set.
 *   - elements are matched by Name when they have one, else by tag and order.
 *   - a new element gets a new widget; a removed one loses its widget.
 *   - when an attribute was removed, or the tag changed, the widget is built
 *     again from the new element (and what is inside it with it).
 *
 * A file that cannot be read or parsed leaves the interface as it is.
 */

#include "xml_watch.h"
#include "../widgets/common.h"
#include "../core/fs.h"
#include "../core/fs_watch.h"

/** Retry interval only while a changed file is temporarily unreadable. */
#define RVN_WATCH_RETRY_INTERVAL 0.2
/** A file that could not be used is tried again this many times before the error is reported. */
#define RVN_WATCH_RETRIES 2

typedef struct rvn_watch_t {
	char* path;
	/** Canonical absolute path used by the OS directory watcher. */
	char* native_path;
	uint64_t modified;
	uint64_t size;
	int exists;
	int retries;
	rvn_watch_callback_t callback;
	void* user;
} rvn_watch_t;

/** What was built from one file. */
typedef struct rvn_live_t {
	char* path;
	int is_window;
	Rid target;
	rvn_xml_document_t document;
	char** files;
	uint32_t file_count;
} rvn_live_t;

static struct {
	rvn_watch_t* watches;
	uint32_t watch_count;
	uint32_t watch_cap;
	rvn_live_t** lives;
	uint32_t live_count;
	uint32_t live_cap;
	Rid timer;
	int configured;
	int disabled;
	RXmlReloadCallback callback;
	void* callback_user;
	/* files read by the document being loaded */
	int recording;
	char** files;
	uint32_t file_count;
	uint32_t file_cap;
	/** Set while a file is tried again: parse errors are kept quiet until the last try. */
	int quiet;
	int native_started;
	/* Used only on platforms where no native notification mechanism exists or setup fails. */
	int polling_fallback;
} g_live;

/* ---------------------------------------------------------------------------
 *  Watched files
 * ------------------------------------------------------------------------- */

static void rvn_watch_retry_timer(Rid timer, void* user);
static void rvn_watch_on_native_event(void* user);

static void rvn_live_configure(void) {
	const char* env;
	if (g_live.configured) return;
	g_live.configured = R_TRUE;
	g_live.timer = R_INVALID_ID;
	env = getenv("RAVEN_LIVE_RELOAD");
	if (env && (env[0] == '0' || env[0] == 'n' || env[0] == 'N' || env[0] == 'f' || env[0] == 'F')) g_live.disabled = R_TRUE;
}

static void rvn_watch_retry_schedule(void) {
	if (g_live.disabled || (g_live.timer != R_INVALID_ID && Rtimer_is_valid(g_live.timer))) return;
	g_live.timer = Rtimer_create(RVN_WATCH_RETRY_INTERVAL, R_FALSE, rvn_watch_retry_timer, NULL);
}

static void rvn_watch_sync_timer(void) {
	uint32_t i;
	int pending = g_live.polling_fallback;
	for (i = 0; i < g_live.watch_count; i++) {
		if (g_live.watches[i].retries > 0) { pending = R_TRUE; break; }
	}
	if (!pending && g_live.timer != R_INVALID_ID && Rtimer_is_valid(g_live.timer)) {
		Rtimer_destroy(g_live.timer);
		g_live.timer = R_INVALID_ID;
	} else if (pending) rvn_watch_retry_schedule();
}

/* Native callbacks run on a filesystem thread. All reload work must be marshalled to the UI thread. */
static void rvn_watch_native_callback(const char* path, void* user) {
	char* copy;
	RVN_UNUSED(user);
	if (!path || !g_live.native_started || g_live.disabled) return;
	copy = rvn_strdup(path);
	Rtask_post(rvn_watch_on_native_event, copy);
}

void rvn_watch_add(const char* path, rvn_watch_callback_t callback, void* user) {
	rvn_watch_t* watch;
	uint32_t i;
	rvn_live_configure();
	if (!path || !path[0]) return;
	/* a file held in memory never changes on disk */
	if (rvn_memory_file_exists(path)) return;
	for (i = 0; i < g_live.watch_count; i++) {
		watch = &g_live.watches[i];
		if (watch->callback == callback && watch->user == user && strcmp(watch->path, path) == 0) return;
	}
	RVN_ARRAY_RESERVE(g_live.watches, g_live.watch_cap, g_live.watch_count + 1);
	watch = &g_live.watches[g_live.watch_count++];
	memset(watch, 0, sizeof(*watch));
	watch->path = rvn_strdup(path);
	watch->native_path = rvn_path_absolute(path);
	watch->callback = callback;
	watch->user = user;
	watch->exists = rvn_file_stamp(path, &watch->modified, &watch->size);
	if (!g_live.native_started) {
		g_live.native_started = rvn_fs_watch_start(rvn_watch_native_callback, NULL);
		if (!g_live.native_started) g_live.polling_fallback = R_TRUE;
	}
	if (g_live.native_started && watch->native_path && !rvn_fs_watch_add(watch->native_path)) {
		g_live.polling_fallback = R_TRUE;
	}
	if (g_live.polling_fallback) rvn_watch_sync_timer();
}

int rvn_watch_quiet(void) {
	return g_live.quiet;
}

static int rvn_watch_check_index(uint32_t i) {
	rvn_watch_t* watch;
	uint64_t modified = 0, size = 0;
	int exists, differs, ok;
	rvn_watch_callback_t callback;
	void* user;
	char* path;
	int retries;

	if (i >= g_live.watch_count) return 0;
	watch = &g_live.watches[i];
	exists = rvn_file_stamp(watch->path, &modified, &size);
	differs = exists != watch->exists || (exists && (modified != watch->modified || size != watch->size));
	if (!differs && watch->retries == 0) return 0;
	/* Editors often replace a file atomically: a temporary absence is not a completed change. */
	if (!exists) {
		watch->exists = R_FALSE;
		return 0;
	}
	if (differs) watch->retries = 0;
	watch->exists = exists;
	watch->modified = modified;
	watch->size = size;
	callback = watch->callback;
	user = watch->user;
	path = rvn_strdup(watch->path);
	retries = watch->retries;
	g_live.quiet = retries < RVN_WATCH_RETRIES;
	ok = callback(path, user);
	rvn_free(path);
	g_live.quiet = R_FALSE;
	/* A reload callback may add another dependency, reallocating the watch array. */
	if (i < g_live.watch_count) {
		watch = &g_live.watches[i];
		if (ok) watch->retries = 0;
		else if (watch->retries < RVN_WATCH_RETRIES) watch->retries++;
		else watch->retries = 0;
	}
	return 1;
}

/* Looks at every file on explicit Rxml_check_files calls; normal updates are event-driven. */
static int rvn_watch_poll(void) {
	uint32_t i;
	int changed = 0;
	for (i = 0; i < g_live.watch_count; i++) changed += rvn_watch_check_index(i);
	rvn_watch_sync_timer();
	return changed;
}

static void rvn_watch_on_native_event(void* user) {
	const char* path = (const char*)user;
	uint32_t i;
	if (path && !g_live.disabled) {
		/* Several logical watches can share the same path but have different callbacks. */
		for (i = 0; i < g_live.watch_count; i++) {
			if (g_live.watches[i].native_path && strcmp(g_live.watches[i].native_path, path) == 0) {
				rvn_watch_check_index(i);
			}
		}
		rvn_watch_sync_timer();
	}
	rvn_free(user);
}

static void rvn_watch_retry_timer(Rid timer, void* user) {
	uint32_t i;
	RVN_UNUSED(user);
	if (g_live.timer == timer) g_live.timer = R_INVALID_ID;
	if (g_live.disabled) return;
	if (g_live.polling_fallback) {
		rvn_watch_poll();
		return;
	}
	for (i = 0; i < g_live.watch_count; i++) {
		if (g_live.watches[i].retries > 0) rvn_watch_check_index(i);
	}
	rvn_watch_sync_timer();
}

/* ---------------------------------------------------------------------------
 *  Files read by a document
 * ------------------------------------------------------------------------- */

void rvn_live_begin(void) {
	if (g_live.recording++ == 0) g_live.file_count = 0;
}

void rvn_live_end(void) {
	uint32_t i;
	if (--g_live.recording > 0) return;
	for (i = 0; i < g_live.file_count; i++) rvn_free(g_live.files[i]);
	g_live.file_count = 0;
}

void rvn_live_dependency(const char* path) {
	uint32_t i;
	if (g_live.recording <= 0 || !path) return;
	for (i = 0; i < g_live.file_count; i++) {
		if (strcmp(g_live.files[i], path) == 0) return;
	}
	RVN_ARRAY_RESERVE(g_live.files, g_live.file_cap, g_live.file_count + 1);
	g_live.files[g_live.file_count++] = rvn_strdup(path);
}

/* ---------------------------------------------------------------------------
 *  Comparing two documents
 * ------------------------------------------------------------------------- */

static int rvn_live_same_attributes(const rvn_xml_node_t* a, const rvn_xml_node_t* b) {
	const rvn_xml_attribute_t* attribute;
	int count_a = 0, count_b = 0;
	for (attribute = a->attributes; attribute; attribute = attribute->next) {
		const char* other = rvn_xml_attribute(b, attribute->name);
		if (!other || strcmp(other, attribute->value) != 0) return R_FALSE;
		count_a++;
	}
	for (attribute = b->attributes; attribute; attribute = attribute->next) count_b++;
	return count_a == count_b;
}

static int rvn_live_same_tree(const rvn_xml_node_t* a, const rvn_xml_node_t* b) {
	const rvn_xml_node_t* child_a;
	const rvn_xml_node_t* child_b;
	if (strcmp(a->name, b->name) != 0 || strcmp(a->text, b->text) != 0 || !rvn_live_same_attributes(a, b)) return R_FALSE;
	for (child_a = a->first_child, child_b = b->first_child; child_a && child_b; child_a = child_a->next, child_b = child_b->next) {
		if (!rvn_live_same_tree(child_a, child_b)) return R_FALSE;
	}
	return !child_a && !child_b;
}

static int rvn_live_same_children(const rvn_xml_node_t* a, const rvn_xml_node_t* b) {
	const rvn_xml_node_t* child_a;
	const rvn_xml_node_t* child_b;
	for (child_a = a->first_child, child_b = b->first_child; child_a && child_b; child_a = child_a->next, child_b = child_b->next) {
		if (!rvn_live_same_tree(child_a, child_b)) return R_FALSE;
	}
	return !child_a && !child_b;
}

/* The text of an element that stands for its Text attribute (<Label>Hello</Label>), or NULL. */
static const char* rvn_live_text(const rvn_xml_node_t* node) {
	if (node->text[0] && !rvn_xml_attribute(node, "Text") && !node->first_child) return node->text;
	return NULL;
}

/*
 * Can every element of a document be built? Checked before anything is
 * touched, so that a mistake in the file leaves the interface whole.
 */
static int rvn_live_valid(const rvn_xml_node_t* node) {
	const rvn_widget_class_t* cls = rvn_widget_class_by_name(node->name);
	const rvn_xml_node_t* child;
	if (!cls) {
		/* a tag made by code: what it accepts inside is only known once it is built */
		if (rvn_xml_tag_find(node->name)) {
			for (child = node->first_child; child; child = child->next) {
				if (!rvn_live_valid(child)) return R_FALSE;
			}
			return R_TRUE;
		}
		rvn_error(R_ERROR_XML, R_SEVERITY_ERROR, "XML line %d, column %d: unknown widget <%.60s>", node->line, node->column, node->name);
		return R_FALSE;
	}
	/* (what is inside such a widget is its own business: items, separators) */
	if (cls->xml_child) return R_TRUE;
	for (child = node->first_child; child; child = child->next) {
		if (cls->flags & RVN_CF_NO_CHILDREN) {
			rvn_error(R_ERROR_XML, R_SEVERITY_ERROR, "XML line %d, column %d: <%.60s> cannot contain <%.60s>", child->line, child->column, node->name, child->name);
			return R_FALSE;
		}
		if (!rvn_live_valid(child)) return R_FALSE;
	}
	return R_TRUE;
}

static int rvn_live_patch(rvn_xml_node_t* old_node, rvn_xml_node_t* new_node, rvn_widget_t* widget);

/* Makes the children of a widget those of `new_node`, keeping the widgets of the elements that are still there. */
static int rvn_live_patch_children(rvn_xml_node_t* old_node, rvn_xml_node_t* new_node, rvn_widget_t* widget) {
	rvn_xml_node_t** olds = NULL;
	rvn_xml_node_t** pairs = NULL;
	rvn_xml_node_t* child;
	uint32_t old_count = 0, new_count = 0, i, k, cursor = 0;
	int ok = R_TRUE;

	for (child = old_node->first_child; child; child = child->next) old_count++;
	for (child = new_node->first_child; child; child = child->next) new_count++;
	if (old_count) olds = (rvn_xml_node_t**)rvn_alloc(sizeof(rvn_xml_node_t*) * old_count);
	if (new_count) pairs = (rvn_xml_node_t**)rvn_calloc(sizeof(rvn_xml_node_t*) * new_count);
	for (child = old_node->first_child, i = 0; child; child = child->next, i++) {
		rvn_widget_t* built = rvn_widget_get(child->widget);
		/* (the application may have moved or deleted it since) */
		olds[i] = built && built->parent == widget ? child : NULL;
	}

	/* pairs: the same Name; else the very same element; else the next one with the same tag */
	for (child = new_node->first_child, k = 0; child; child = child->next, k++) {
		const char* name = rvn_xml_attribute(child, "Name");
		int found = -1;
		for (i = 0; i < old_count && found < 0; i++) {
			const char* old_name;
			if (!olds[i] || strcmp(olds[i]->name, child->name) != 0) continue;
			old_name = rvn_xml_attribute(olds[i], "Name");
			if (name) {
				if (old_name && strcmp(name, old_name) == 0) found = (int)i;
			} else if (!old_name && rvn_live_same_tree(olds[i], child)) {
				found = (int)i;
			}
		}
		for (i = 0; i < old_count && found < 0 && !name; i++) {
			if (olds[i] && strcmp(olds[i]->name, child->name) == 0 && !rvn_xml_attribute(olds[i], "Name")) found = (int)i;
		}
		if (found >= 0) {
			pairs[k] = olds[found];
			olds[found] = NULL;
		}
	}
	/* the widgets of the elements that are gone */
	for (i = 0; i < old_count; i++) {
		if (olds[i]) rvn_widget_destroy(rvn_widget_get(olds[i]->widget));
	}

	for (child = new_node->first_child, k = 0; child && ok; child = child->next, k++) {
		rvn_widget_t* built = pairs[k] ? rvn_widget_get(pairs[k]->widget) : NULL;
		if (built && !rvn_live_patch(pairs[k], child, built)) {
			rvn_widget_destroy(built);
			built = NULL;
		}
		if (!built) {
			built = rvn_xml_build_node(child);
			if (!built) {
				ok = R_FALSE;
				break;
			}
			rvn_widget_add_child(widget, built, (int)cursor);
			cursor++;
		} else {
			/* the order of the file, among the children the application may have added itself */
			int index = rvn_widget_child_index(widget, built);
			if (index < (int)cursor) {
				rvn_widget_add_child(widget, built, (int)cursor - 1);
			} else {
				cursor = (uint32_t)index + 1;
			}
		}
	}
	rvn_free(olds);
	rvn_free(pairs);
	return ok;
}

/* Turns the widget built from `old_node` into the one `new_node` describes. R_FALSE: it must be built again. */
static int rvn_live_patch(rvn_xml_node_t* old_node, rvn_xml_node_t* new_node, rvn_widget_t* widget) {
	const rvn_widget_class_t* cls = widget->cls;
	const rvn_xml_attribute_t* attribute;
	const char* old_text = rvn_live_text(old_node);
	const char* new_text = rvn_live_text(new_node);

	if (rvn_stricmp(old_node->name, new_node->name) != 0) return R_FALSE;
	/* an attribute that is gone: only a new widget has its default value for sure */
	for (attribute = old_node->attributes; attribute; attribute = attribute->next) {
		if (!rvn_xml_attribute(new_node, attribute->name)) return R_FALSE;
	}
	if (old_text && !new_text && !rvn_xml_attribute(new_node, "Text")) return R_FALSE;

	if (cls->xml_child || (cls->flags & RVN_CF_NO_CHILDREN)) {
		if (!rvn_live_same_children(old_node, new_node)) return R_FALSE;
	} else if (!rvn_live_patch_children(old_node, new_node, widget)) {
		return R_FALSE;
	}

	if (new_text && (!old_text || strcmp(old_text, new_text) != 0)) rvn_widget_set_attribute(widget, "Text", new_text);
	for (attribute = new_node->attributes; attribute; attribute = attribute->next) {
		const char* before = rvn_xml_attribute(old_node, attribute->name);
		if (before && strcmp(before, attribute->value) == 0) continue;
		if (!rvn_widget_set_attribute(widget, attribute->name, attribute->value)) {
			rvn_error(R_ERROR_XML, R_SEVERITY_WARNING, "XML line %d, column %d: <%.60s>: unknown attribute or invalid value %.60s=\"%.60s\"",
				new_node->line, new_node->column, new_node->name, attribute->name, attribute->value);
		}
	}
	new_node->widget = widget->id;
	widget->xml_node = new_node->source;
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Reloading
 * ------------------------------------------------------------------------- */

static void rvn_live_free(rvn_live_t* live) {
	uint32_t i;
	for (i = 0; i < live->file_count; i++) rvn_free(live->files[i]);
	rvn_free(live->files);
	rvn_free(live->path);
	rvn_xml_free(&live->document);
	rvn_free(live);
}

static int rvn_live_file_changed(const char* path, void* user);

/* Takes the files noted while the document was loaded, and watches them. */
static void rvn_live_take_files(rvn_live_t* live) {
	uint32_t i;
	for (i = 0; i < live->file_count; i++) rvn_free(live->files[i]);
	rvn_free(live->files);
	live->files = NULL;
	live->file_count = g_live.file_count;
	if (g_live.file_count) {
		live->files = (char**)rvn_alloc(sizeof(char*) * g_live.file_count);
		for (i = 0; i < g_live.file_count; i++) {
			live->files[i] = rvn_strdup(g_live.files[i]);
			rvn_watch_add(live->files[i], rvn_live_file_changed, NULL);
		}
	}
}

void rvn_live_add(const char* path, int is_window, Rid target, rvn_xml_document_t* document) {
	rvn_live_t* live = (rvn_live_t*)rvn_calloc(sizeof(rvn_live_t));
	live->path = rvn_strdup(path);
	live->is_window = is_window;
	live->target = target;
	live->document = *document;
	memset(document, 0, sizeof(*document));
	rvn_live_take_files(live);
	RVN_ARRAY_RESERVE(g_live.lives, g_live.live_cap, g_live.live_count + 1);
	g_live.lives[g_live.live_count++] = live;
	rvn_watch_add(path, rvn_live_file_changed, NULL);
}

static void rvn_live_reload_window(rvn_live_t* live, rvn_window_t* window, rvn_xml_document_t* document) {
	rvn_xml_node_t* old_root = live->document.root;
	rvn_xml_node_t* new_root = document->root;
	const rvn_xml_node_t* old_content = NULL;
	const rvn_xml_node_t* old_island = NULL;
	const rvn_xml_node_t* new_content = NULL;
	const rvn_xml_node_t* new_island = NULL;
	rvn_xml_window_t before, after;
	Rid id = window->id;

	rvn_xml_window_read(old_root, &before, R_FALSE);
	rvn_xml_window_read(new_root, &after, R_TRUE);
	if (strcmp(before.title, after.title) != 0) Rwindow_set_title(id, after.title);
	if (before.width != after.width || before.height != after.height) Rwindow_set_size(id, after.width, after.height);
	if (before.min_width != after.min_width || before.min_height != after.min_height) Rwindow_set_min_size(id, after.min_width, after.min_height);
	if (after.has_clear && (!before.has_clear || before.clear != after.clear)) {
		float r, g, b, a;
		rvn_color_to_floats(after.clear, &r, &g, &b, &a);
		Rwindow_set_clear_color(id, r, g, b, a);
	}
	if (before.background != after.background) Rwindow_set_background(id, after.background);
	if (after.backdrop && before.backdrop != after.backdrop && (window->window_flags & R_WINDOW_TRANSPARENT)) {
		Rwindow_set_backdrop(id, (RBackdrop)after.backdrop);
	}
	if (strcmp(before.theme ? before.theme : "", after.theme ? after.theme : "") != 0) {
		/* the theme of this window only */
		rvn_window_set_theme(window, rvn_xml_window_theme(after.theme));
	}
	if (after.has_touch) {
		if (!before.has_touch || memcmp(&before.touch, &after.touch, sizeof(after.touch)) != 0) Rwindow_set_touch_settings(id, &after.touch);
	} else if (before.has_touch) {
		Rwindow_set_touch_settings(id, NULL);
	}
	if ((before.flags ^ after.flags) & ~(Ruint)R_WINDOW_HIDDEN) {
		rvn_error(R_ERROR_XML, R_SEVERITY_WARNING, "%s: the frame of a window (Borderless, CustomHeader, Resizable, Transparent...) is chosen when it is created; start the application again to see this change", live->path);
	}

	rvn_xml_window_parts(old_root, &old_content, &old_island);
	rvn_xml_window_parts(new_root, &new_content, &new_island);

	if (new_content && old_content && window->root && window->root->id == old_content->widget &&
		rvn_live_patch((rvn_xml_node_t*)(uintptr_t)old_content, (rvn_xml_node_t*)(uintptr_t)new_content, window->root)) {
		/* (kept) */
	} else if (new_content) {
		rvn_widget_t* content = rvn_xml_build_node(new_content);
		if (content) Rwindow_set_content(id, content->id);
	} else if (old_content && window->root && window->root->id == old_content->widget) {
		Rwindow_set_content(id, R_INVALID_ID);
	}

	if (new_island && old_island && window->island && window->island->id == old_island->widget &&
		rvn_live_patch((rvn_xml_node_t*)(uintptr_t)old_island, (rvn_xml_node_t*)(uintptr_t)new_island, window->island)) {
		/* (kept) */
	} else if (new_island) {
		rvn_widget_t* island = rvn_xml_build_node(new_island);
		if (island) Rwindow_set_island(id, island->id);
	} else if (old_island && window->island && window->island->id == old_island->widget) {
		Rwindow_set_island(id, R_INVALID_ID);
	}

	rvn_xml_window_shortcuts(window, old_root, R_FALSE);
	rvn_xml_window_shortcuts(window, new_root, R_TRUE);
	window->xml_node = new_root->source;
}

static int rvn_live_reload_widget(rvn_live_t* live, rvn_widget_t* widget, rvn_xml_document_t* document) {
	rvn_widget_t* built;
	rvn_widget_t* parent = widget->parent;
	rvn_window_t* window = widget->window;

	if (rvn_live_patch(live->document.root, document->root, widget)) return R_TRUE;
	/* the root itself must be built again: the new widget takes the place of the old one */
	if (parent) {
		int index = rvn_widget_child_index(parent, widget);
		built = rvn_xml_build_node(document->root);
		if (!built) return R_FALSE;
		rvn_widget_destroy(widget);
		rvn_widget_add_child(parent, built, index);
	} else if (window && window->root == widget) {
		built = rvn_xml_build_node(document->root);
		if (!built) return R_FALSE;
		Rwindow_set_content(window->id, built->id);
	} else if (window && window->island == widget) {
		built = rvn_xml_build_node(document->root);
		if (!built) return R_FALSE;
		Rwindow_set_island(window->id, built->id);
	} else {
		rvn_error(R_ERROR_XML, R_SEVERITY_WARNING, "%s: the root element changed in a way that needs a new widget, and the one it built is not shown anywhere: it is left as it was", live->path);
		return R_FALSE;
	}
	live->target = built->id;
	return R_TRUE;
}

/* R_TRUE when the file was used (or will never be); R_FALSE to try again. */
static int rvn_live_reload(rvn_live_t* live) {
	rvn_xml_document_t document;
	rvn_window_t* window = live->is_window ? rvn_window_get(live->target) : NULL;
	rvn_widget_t* widget = live->is_window ? NULL : rvn_widget_get(live->target);
	char* previous_dir;
	size_t size = 0;
	char* text;
	int ok = R_FALSE;

	text = rvn_read_file(live->path, &size);
	if (!text) return R_FALSE;
	/* a file caught half written: quietly tried again before saying anything */
	if (g_live.quiet) {
		int parsed = rvn_xml_parse(&document, text, size);
		rvn_xml_free(&document);
		if (!parsed) {
			rvn_free(text);
			return R_FALSE;
		}
	}

	rvn_xml_push_file_dir(live->path, &previous_dir);
	rvn_live_begin();
	if (rvn_xml_load(&document, text, size, live->path)) {
		const char* root_name = document.root->name;
		int root_is_window = rvn_stricmp(root_name, "Window") == 0;
		const rvn_xml_node_t* content = NULL;
		const rvn_xml_node_t* island = NULL;
		int valid;
		if (live->is_window != root_is_window) {
			rvn_error(R_ERROR_XML, R_SEVERITY_ERROR, "%s: the root element must %s <Window>", live->path, live->is_window ? "be" : "not be");
			valid = R_FALSE;
		} else if (root_is_window) {
			valid = rvn_xml_window_parts(document.root, &content, &island);
			if (valid && content) valid = rvn_live_valid(content);
			if (valid && island) valid = rvn_live_valid(island);
		} else {
			valid = rvn_live_valid(document.root);
		}
		if (valid) {
			if (window) {
				rvn_live_reload_window(live, window, &document);
				ok = R_TRUE;
			} else {
				ok = rvn_live_reload_widget(live, widget, &document);
			}
		}
		if (ok) {
			rvn_xml_free(&live->document);
			live->document = document;
			rvn_live_take_files(live);
		} else {
			rvn_xml_free(&document);
		}
	}
	rvn_live_end();
	rvn_xml_pop_file_dir(previous_dir);
	rvn_free(text);

	if (g_live.callback) {
		char* path = rvn_strdup(live->path);
		rvn_dispatch_begin();
		g_live.callback(path, ok, g_live.callback_user);
		rvn_dispatch_end();
		rvn_free(path);
	}
	/* (an error was reported: the file is left alone until it changes again) */
	return R_TRUE;
}

/* Forgets what was built from files and no longer exists. */
static void rvn_live_sweep(void) {
	uint32_t i = 0;
	while (i < g_live.live_count) {
		rvn_live_t* live = g_live.lives[i];
		int alive = live->is_window ? rvn_window_get(live->target) != NULL : rvn_widget_get(live->target) != NULL;
		if (alive) {
			i++;
			continue;
		}
		rvn_live_free(live);
		g_live.lives[i] = g_live.lives[--g_live.live_count];
	}
}

static int rvn_live_uses(const rvn_live_t* live, const char* path) {
	uint32_t i;
	if (strcmp(live->path, path) == 0) return R_TRUE;
	for (i = 0; i < live->file_count; i++) {
		if (strcmp(live->files[i], path) == 0) return R_TRUE;
	}
	return R_FALSE;
}

/* Reloads what was built from `path`, or everything when it is NULL. */
static int rvn_live_reload_all(const char* path) {
	uint32_t i, count;
	int ok = R_TRUE;
	rvn_live_sweep();
	/* (a reload may build things from files: only what was there is looked at) */
	count = g_live.live_count;
	for (i = 0; i < count && i < g_live.live_count; i++) {
		rvn_live_t* live = g_live.lives[i];
		if (path && !rvn_live_uses(live, path)) continue;
		if (!rvn_live_reload(live)) ok = R_FALSE;
	}
	return ok;
}

static int rvn_live_file_changed(const char* path, void* user) {
	RVN_UNUSED(user);
	return rvn_live_reload_all(path);
}

static int rvn_live_widgets_changed(const char* path, void* user) {
	size_t size = 0;
	char* text = rvn_read_file(path, &size);
	rvn_xml_document_t document;
	char* previous_dir;
	RVN_UNUSED(user);
	if (!text) return R_FALSE;
	if (g_live.quiet) {
		int parsed = rvn_xml_parse(&document, text, size);
		rvn_xml_free(&document);
		if (!parsed) {
			rvn_free(text);
			return R_FALSE;
		}
	}
	rvn_xml_push_file_dir(path, &previous_dir);
	Rxml_register_widgets(text, size);
	rvn_xml_pop_file_dir(previous_dir);
	rvn_free(text);
	/* any interface may use the widgets the file declares */
	rvn_live_reload_all(NULL);
	return R_TRUE;
}

void rvn_live_watch_widgets(const char* path) {
	rvn_watch_add(path, rvn_live_widgets_changed, NULL);
}

void rvn_live_shutdown(void) {
	uint32_t i;
	for (i = 0; i < g_live.live_count; i++) rvn_live_free(g_live.lives[i]);
	rvn_fs_watch_stop();
	for (i = 0; i < g_live.watch_count; i++) {
		rvn_free(g_live.watches[i].path);
		rvn_free(g_live.watches[i].native_path);
	}
	for (i = 0; i < g_live.file_count; i++) rvn_free(g_live.files[i]);
	rvn_free(g_live.lives);
	rvn_free(g_live.watches);
	rvn_free(g_live.files);
	memset(&g_live, 0, sizeof(g_live));
}

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

void Rxml_set_live_reload(int enabled) {
	RVN_API_GUARD_VOID();
	rvn_live_configure();
	g_live.disabled = !enabled;
	if (enabled) rvn_watch_poll();
	rvn_watch_sync_timer();
}

int Rxml_get_live_reload(void) {
	if (!g_rvn.initialized) return R_FALSE;
	rvn_live_configure();
	return !g_live.disabled;
}

void Rxml_set_reload_callback(RXmlReloadCallback callback, void* user_data) {
	RVN_API_GUARD_VOID();
	g_live.callback = callback;
	g_live.callback_user = user_data;
}

int Rxml_check_files(void) {
	RVN_API_GUARD(0);
	rvn_live_configure();
	return rvn_watch_poll();
}

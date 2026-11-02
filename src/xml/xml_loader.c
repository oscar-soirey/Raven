/**
 * Raven - xml/xml_loader.c
 * Builds windows and widget trees from XML (see raven_xml.h and docs/XML.md).
 *
 *   element          -> widget of the class named by the tag
 *   attribute        -> rvn_widget_set_attribute
 *   child element    -> child widget, unless the class of the parent handles
 *                       the tag itself (<Item>, menu <Separator>)
 *   text content     -> the "Text" attribute
 */

#include "xml.h"
#include "xml_watch.h"
#include "../widgets/common.h"
#include "../render/renderer.h"
#include "../core/fs.h"

/* ---------------------------------------------------------------------------
 *  Resources named in attributes (Source="...", Font="...")
 * ------------------------------------------------------------------------- */

typedef struct rvn_xml_resource_t {
	char* path;
	Rid id;
} rvn_xml_resource_t;

static struct {
	rvn_xml_resource_t* textures;
	uint32_t texture_count;
	uint32_t texture_cap;
	rvn_xml_resource_t* fonts;
	uint32_t font_count;
	uint32_t font_cap;
	/** Folder of the file being loaded; overrides the base path meanwhile. */
	char* file_dir;
	/** What Rxml_resolve_path last returned. */
	char* resolved;
} g_xml;

static char* rvn_xml_resolve(const char* path) {
	return rvn_path_join(g_xml.file_dir ? g_xml.file_dir : g_rvn.xml_base_path, path);
}

char* rvn_xml_resolve_path(const char* path) {
	return rvn_xml_resolve(path);
}

Rid rvn_xml_texture(const char* path) {
	char* full = rvn_is_http_url(path) ? rvn_strdup(path) : rvn_xml_resolve(path);
	int remote = rvn_is_http_url(path);
	Rid texture;
	uint32_t i;

	for (i = 0; i < g_xml.texture_count; i++) {
		if (strcmp(g_xml.textures[i].path, full) != 0) continue;
		/* still valid unless the application deleted it */
		if (rvn_texture_get(g_xml.textures[i].id)) {
			rvn_free(full);
			return g_xml.textures[i].id;
		}
		break;
	}
	texture = remote ? rvn_texture_create_from_url_async(full) : Rtexture_create_from_file(full);
	if (texture == R_INVALID_ID) {
		rvn_free(full);
		return R_INVALID_ID;
	}
	if (i < g_xml.texture_count) {
		g_xml.textures[i].id = texture;
		rvn_free(full);
	} else {
		RVN_ARRAY_RESERVE(g_xml.textures, g_xml.texture_cap, g_xml.texture_count + 1);
		g_xml.textures[g_xml.texture_count].path = full;
		g_xml.textures[g_xml.texture_count].id = texture;
		g_xml.texture_count++;
	}
	return texture;
}

Rid rvn_xml_font(const char* path) {
	char* full;
	Rid font;
	uint32_t i;

	/* the fonts of the library have names */
	if (rvn_stricmp(path, "monospace") == 0) return g_rvn.mono_font;
	if (rvn_stricmp(path, "default") == 0) return g_rvn.default_font;
	full = rvn_xml_resolve(path);

	for (i = 0; i < g_xml.font_count; i++) {
		if (strcmp(g_xml.fonts[i].path, full) != 0) continue;
		if (Rfont_is_valid(g_xml.fonts[i].id)) {
			rvn_free(full);
			return g_xml.fonts[i].id;
		}
		break;
	}
	font = Rfont_create_from_file(full);
	if (font == R_INVALID_ID) {
		rvn_free(full);
		return R_INVALID_ID;
	}
	if (i < g_xml.font_count) {
		g_xml.fonts[i].id = font;
		rvn_free(full);
	} else {
		RVN_ARRAY_RESERVE(g_xml.fonts, g_xml.font_cap, g_xml.font_count + 1);
		g_xml.fonts[g_xml.font_count].path = full;
		g_xml.fonts[g_xml.font_count].id = font;
		g_xml.font_count++;
	}
	return font;
}

static void rvn_xml_tags_shutdown(void);

/** Forgets the resource cache. The textures and fonts themselves belong to the library pools. */
void rvn_xml_shutdown(void) {
	uint32_t i;
	for (i = 0; i < g_xml.texture_count; i++) rvn_free(g_xml.textures[i].path);
	for (i = 0; i < g_xml.font_count; i++) rvn_free(g_xml.fonts[i].path);
	rvn_free(g_xml.textures);
	rvn_free(g_xml.fonts);
	rvn_free(g_xml.file_dir);
	rvn_free(g_xml.resolved);
	memset(&g_xml, 0, sizeof(g_xml));
	rvn_xml_tags_shutdown();
	rvn_live_shutdown();
	rvn_xml_components_shutdown();
	rvn_xml_dom_shutdown();
}

/* ---------------------------------------------------------------------------
 *  Building
 * ------------------------------------------------------------------------- */

static void rvn_xml_error(const rvn_xml_node_t* node, RSeverity severity, const char* format, ...) {
	char message[256];
	va_list args;
	va_start(args, format);
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);
	rvn_error(R_ERROR_XML, severity, "XML line %d, column %d: %s", node->line, node->column, message);
}

/* ---------------------------------------------------------------------------
 *  Tags made by code
 * ------------------------------------------------------------------------- */

#define RVN_XML_TAGS_MAX 64

typedef struct rvn_xml_tag_t {
	char* name;
	RXmlTagCreateCallback create;
	RXmlTagAttributeCallback attribute;
	void* user;
} rvn_xml_tag_t;

/* (a slot keeps its place when its tag is removed: the widgets built for it name it by index) */
static rvn_xml_tag_t g_xml_tags[RVN_XML_TAGS_MAX];

int rvn_xml_tag_find(const char* name) {
	int i;
	if (!name) return 0;
	for (i = 0; i < RVN_XML_TAGS_MAX; i++) {
		if (g_xml_tags[i].name && rvn_stricmp(g_xml_tags[i].name, name) == 0) return i + 1;
	}
	return 0;
}

int rvn_xml_tag_attribute(rvn_widget_t* widget, const char* name, const char* value) {
	const rvn_xml_tag_t* tag;
	if (widget->xml_tag == 0 || widget->xml_tag > RVN_XML_TAGS_MAX) return R_FALSE;
	tag = &g_xml_tags[widget->xml_tag - 1];
	if (!tag->name || !tag->attribute) return R_FALSE;
	return tag->attribute(widget->id, name, value, tag->user) != 0;
}

static void rvn_xml_tags_shutdown(void) {
	int i;
	for (i = 0; i < RVN_XML_TAGS_MAX; i++) rvn_free(g_xml_tags[i].name);
	memset(g_xml_tags, 0, sizeof(g_xml_tags));
}

int Rxml_register_tag(const char* tag, RXmlTagCreateCallback create, RXmlTagAttributeCallback set_attribute, void* user_data) {
	int index, i;
	RVN_API_GUARD(R_FALSE);
	if (!tag || !tag[0] || !create) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rxml_register_tag: a tag needs a name and a function that makes its widget");
		return R_FALSE;
	}
	if (rvn_widget_class_by_name(tag) || rvn_stricmp(tag, "Window") == 0) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rxml_register_tag: <%.60s> is a tag of Raven", tag);
		return R_FALSE;
	}
	index = rvn_xml_tag_find(tag) - 1;
	for (i = 0; i < RVN_XML_TAGS_MAX && index < 0; i++) {
		if (!g_xml_tags[i].name) index = i;
	}
	if (index < 0) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rxml_register_tag: too many tags (%d)", RVN_XML_TAGS_MAX);
		return R_FALSE;
	}
	if (!g_xml_tags[index].name) g_xml_tags[index].name = rvn_strdup(tag);
	g_xml_tags[index].create = create;
	g_xml_tags[index].attribute = set_attribute;
	g_xml_tags[index].user = user_data;
	return R_TRUE;
}

void Rxml_unregister_tag(const char* tag) {
	int index;
	RVN_API_GUARD_VOID();
	index = rvn_xml_tag_find(tag) - 1;
	if (index < 0) return;
	rvn_free(g_xml_tags[index].name);
	memset(&g_xml_tags[index], 0, sizeof(g_xml_tags[index]));
}

const char* Rxml_resolve_path(const char* path) {
	RVN_API_GUARD("");
	rvn_free(g_xml.resolved);
	g_xml.resolved = path ? rvn_xml_resolve(path) : NULL;
	return g_xml.resolved ? g_xml.resolved : "";
}

int Rxml_is_tag_registered(const char* tag) {
	RVN_API_GUARD(R_FALSE);
	return rvn_xml_tag_find(tag) != 0;
}

rvn_widget_t* rvn_xml_build_node(const rvn_xml_node_t* node) {
	const rvn_widget_class_t* cls = rvn_widget_class_by_name(node->name);
	const rvn_xml_attribute_t* attribute;
	const rvn_xml_node_t* child;
	rvn_widget_t* widget;

	if (!cls) {
		/* a tag made by code: it builds its widget itself */
		int tag = rvn_xml_tag_find(node->name);
		Rid made;
		if (!tag) {
			rvn_xml_error(node, R_SEVERITY_ERROR, "unknown widget <%.60s>", node->name);
			return NULL;
		}
		made = g_xml_tags[tag - 1].create(g_xml_tags[tag - 1].name, g_xml_tags[tag - 1].user);
		widget = rvn_widget_get(made);
		if (!widget || widget->parent || (widget->flags & RVN_WF_POPUP)) {
			rvn_xml_error(node, R_SEVERITY_ERROR, "<%.60s>: the function registered for this tag did not return a new widget", node->name);
			return NULL;
		}
		widget->xml_tag = (uint8_t)tag;
		cls = widget->cls;
	} else {
		widget = rvn_widget_create(cls->type);
	}
	if (!widget) return NULL;
	widget->xml_node = node->source;
	/* (documents kept for live reload find their widgets again through this) */
	((rvn_xml_node_t*)(uintptr_t)node)->widget = widget->id;

	/* children first: attributes such as Selected="1" or ActiveTab="2" refer to them */
	for (child = node->first_child; child; child = child->next) {
		rvn_widget_t* child_widget;
		/* the class may give the tag its own meaning (<Item>, <Separator> in a menu) */
		if (cls->xml_child && cls->xml_child(widget, child)) continue;
		if (cls->flags & RVN_CF_NO_CHILDREN) {
			rvn_xml_error(child, R_SEVERITY_ERROR, "<%.60s> cannot contain <%.60s>", node->name, child->name);
			rvn_widget_destroy(widget);
			return NULL;
		}
		child_widget = rvn_xml_build_node(child);
		if (!child_widget) {
			rvn_widget_destroy(widget);
			return NULL;
		}
		rvn_widget_add_child(widget, child_widget, -1);
	}

	/* <Label>Hello</Label> is the same as <Label Text="Hello"/> */
	if (node->text[0] && !rvn_xml_attribute(node, "Text") && !node->first_child) {
		rvn_widget_set_attribute(widget, "Text", node->text);
	}
	for (attribute = node->attributes; attribute; attribute = attribute->next) {
		if (!rvn_widget_set_attribute(widget, attribute->name, attribute->value)) {
			/* a typo should not prevent the rest of the interface from loading */
			rvn_xml_error(node, R_SEVERITY_WARNING, "<%.60s>: unknown attribute or invalid value %.60s=\"%.60s\"",
				node->name, attribute->name, attribute->value);
		}
	}
	return widget;
}

int rvn_xml_load(rvn_xml_document_t* document, const char* xml, size_t size, const char* function) {
	if (!xml) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: NULL text", function);
		return R_FALSE;
	}
	if (!rvn_xml_parse(document, xml, size)) {
		rvn_error(R_ERROR_XML, R_SEVERITY_ERROR, "%s: line %d, column %d: %s", function,
			document->error_line, document->error_column, document->error);
		rvn_xml_free(document);
		return R_FALSE;
	}
	/* declared widgets, repetitions, conditions: the document becomes a plain one */
	if (!rvn_xml_expand(document, NULL)) {
		rvn_xml_free(document);
		return R_FALSE;
	}
	if (!document->root) {
		rvn_error(R_ERROR_XML, R_SEVERITY_ERROR, "%s: the document declares widgets but contains nothing to create", function);
		rvn_xml_free(document);
		return R_FALSE;
	}
	return R_TRUE;
}

int rvn_xml_window_parts(const rvn_xml_node_t* root, const rvn_xml_node_t** content, const rvn_xml_node_t** island) {
	const rvn_xml_node_t* child;
	*content = NULL;
	*island = NULL;
	for (child = root->first_child; child; child = child->next) {
		const rvn_widget_class_t* cls = rvn_widget_class_by_name(child->name);
		if (rvn_stricmp(child->name, "Shortcut") == 0) continue;
		/* an island next to the content floats above it at the top of the window */
		if (cls && cls->type == R_WIDGET_ISLAND && !*island) {
			*island = child;
			continue;
		}
		if (*content) {
			rvn_xml_error(child, R_SEVERITY_ERROR, "a <Window> has a single content element; wrap them in a panel");
			return R_FALSE;
		}
		*content = child;
	}
	return R_TRUE;
}

Rid rvn_xml_make_widget(rvn_xml_document_t* document, int window_content) {
	const rvn_xml_node_t* node = document->root;
	rvn_widget_t* widget;
	if (rvn_stricmp(node->name, "Window") == 0) {
		const rvn_xml_node_t* island;
		if (!window_content) {
			rvn_xml_error(node, R_SEVERITY_ERROR, "the root element is a <Window>: use Rwindow_create_from_xml");
			return R_INVALID_ID;
		}
		/* what the window would show: its first element that is neither a shortcut nor an island */
		rvn_xml_window_parts(node, &node, &island);
		if (!node) return R_INVALID_ID;
	}
	widget = rvn_xml_build_node(node);
	return widget ? widget->id : R_INVALID_ID;
}

Rid rvn_xml_build_widget(rvn_xml_document_t* document, int window_content, const char* function) {
	Rid widget = rvn_xml_make_widget(document, window_content);
	RVN_UNUSED(function);
	rvn_xml_free(document);
	return widget;
}

static Rid rvn_xml_create_widget(const char* xml, size_t size, const char* function) {
	rvn_xml_document_t document;
	if (!rvn_xml_load(&document, xml, size, function)) return R_INVALID_ID;
	return rvn_xml_build_widget(&document, R_FALSE, function);
}

static Rid rvn_xml_create_window(const char* xml, size_t size, const char* function) {
	rvn_xml_document_t document;
	if (!rvn_xml_load(&document, xml, size, function)) return R_INVALID_ID;
	return rvn_xml_build_window(&document, function);
}

static int rvn_xml_parse_backdrop(const char* value, int* backdrop) {
	static const struct { const char* name; RBackdrop backdrop; } names[] = {
		{ "None", R_BACKDROP_NONE }, { "Opaque", R_BACKDROP_NONE }, { "Transparent", R_BACKDROP_TRANSPARENT },
		{ "Blur", R_BACKDROP_BLUR }, { "Acrylic", R_BACKDROP_ACRYLIC }, { "Mica", R_BACKDROP_MICA }
	};
	size_t i;
	for (i = 0; i < RVN_COUNT_OF(names); i++) {
		if (rvn_stricmp(value, names[i].name) == 0) {
			*backdrop = (int)names[i].backdrop;
			return R_TRUE;
		}
	}
	return R_FALSE;
}

void rvn_xml_window_read(const rvn_xml_node_t* root, rvn_xml_window_t* window, int report) {
	const rvn_xml_attribute_t* attribute;
	int flag;

	memset(window, 0, sizeof(*window));
	window->title = "";
	window->width = 800;
	window->height = 600;
	window->flags = R_WINDOW_DEFAULT;
	window->background = R_INVALID_ID;
	Rtouch_get_settings(&window->touch);

	for (attribute = root->attributes; attribute; attribute = attribute->next) {
		const char* name = attribute->name;
		const char* value = attribute->value;
		int ok = R_TRUE;
		int touch = rvn_touch_window_attribute(&window->touch, name, value);
		if (touch >= 0) {
			ok = touch;
			window->has_touch = R_TRUE;
		} else if (RVN_ATTR("Title")) window->title = value;
		else if (RVN_ATTR("Width")) ok = rvn_parse_int(value, &window->width) && window->width > 0;
		else if (RVN_ATTR("Height")) ok = rvn_parse_int(value, &window->height) && window->height > 0;
		else if (RVN_ATTR("MinWidth")) ok = rvn_parse_int(value, &window->min_width);
		else if (RVN_ATTR("MinHeight")) ok = rvn_parse_int(value, &window->min_height);
		else if (RVN_ATTR("Resizable")) {
			ok = rvn_parse_bool(value, &flag);
			if (ok && !flag) window->flags |= R_WINDOW_NO_RESIZE;
		} else if (RVN_ATTR("Borderless")) {
			ok = rvn_parse_bool(value, &flag);
			if (ok && flag) window->flags |= R_WINDOW_BORDERLESS;
		} else if (RVN_ATTR("CustomHeader")) {
			ok = rvn_parse_bool(value, &flag);
			if (ok && flag) window->flags |= R_WINDOW_CUSTOM_HEADER;
		} else if (RVN_ATTR("Maximized")) {
			ok = rvn_parse_bool(value, &flag);
			if (ok && flag) window->flags |= R_WINDOW_MAXIMIZED;
		} else if (RVN_ATTR("Hidden")) {
			ok = rvn_parse_bool(value, &flag);
			if (ok && flag) window->flags |= R_WINDOW_HIDDEN;
		} else if (RVN_ATTR("Transparent")) {
			ok = rvn_parse_bool(value, &flag);
			if (ok && flag) {
				window->flags |= R_WINDOW_TRANSPARENT;
				if (!window->backdrop) window->backdrop = (int)R_BACKDROP_TRANSPARENT;
			}
		} else if (RVN_ATTR("Backdrop")) {
			ok = rvn_xml_parse_backdrop(value, &window->backdrop);
			if (ok && window->backdrop != (int)R_BACKDROP_NONE) window->flags |= R_WINDOW_TRANSPARENT;
		} else if (RVN_ATTR("ClearColor")) {
			ok = rvn_parse_color(value, &window->clear);
			window->has_clear = ok;
		} else if (RVN_ATTR("Background")) {
			window->background = rvn_xml_texture(value);
			ok = window->background != R_INVALID_ID;
		} else if (RVN_ATTR("Theme")) {
			window->theme = value;
		} else {
			ok = R_FALSE;
		}
		if (!ok && report) {
			rvn_xml_error(root, R_SEVERITY_WARNING, "<Window>: unknown attribute or invalid value %.60s=\"%.60s\"", name, value);
		}
	}
}

void rvn_xml_window_shortcuts(rvn_window_t* window, const rvn_xml_node_t* root, int add) {
	const rvn_xml_node_t* child;
	/* <Shortcut Keys="Ctrl+S" Action="file.save"/> */
	for (child = root->first_child; child; child = child->next) {
		const char* keys;
		const char* action;
		RKey key;
		uint32_t modifiers;
		if (rvn_stricmp(child->name, "Shortcut") != 0) continue;
		keys = rvn_xml_attribute(child, "Keys");
		action = rvn_xml_attribute(child, "Action");
		if (!keys || !action || !action[0] || !rvn_parse_shortcut(keys, &key, &modifiers)) {
			if (add) rvn_xml_error(child, R_SEVERITY_WARNING, "<Shortcut> needs Keys=\"Ctrl+S\" and Action=\"name\"");
			continue;
		}
		if (add) rvn_window_add_shortcut(window, key, modifiers, action);
		else rvn_window_remove_shortcut(window, key, modifiers);
	}
}

const rvn_theme_t* rvn_xml_window_theme(const char* value) {
	if (!value || !value[0] || rvn_stricmp(value, "Default") == 0 || rvn_stricmp(value, "Inherit") == 0) return NULL;
	return rvn_xml_theme(value);
}

Rid rvn_xml_make_window(rvn_xml_document_t* document) {
	const rvn_xml_node_t* root = document->root;
	const rvn_xml_node_t* content_node;
	const rvn_xml_node_t* island_node;
	rvn_widget_t* content = NULL;
	rvn_widget_t* island = NULL;
	rvn_window_t* window_object;
	rvn_xml_window_t description;
	Rid window;

	if (rvn_stricmp(root->name, "Window") != 0) {
		rvn_xml_error(root, R_SEVERITY_ERROR, "the root element must be <Window>, found <%.60s>", root->name);
		return R_INVALID_ID;
	}
	rvn_xml_window_read(root, &description, R_TRUE);

	/* the content is built first: a mistake in it must not leave an empty window behind */
	if (!rvn_xml_window_parts(root, &content_node, &island_node)) return R_INVALID_ID;
	if (island_node) {
		island = rvn_xml_build_node(island_node);
		if (!island) return R_INVALID_ID;
	}
	if (content_node) {
		content = rvn_xml_build_node(content_node);
		if (!content) {
			if (island) rvn_widget_destroy(island);
			return R_INVALID_ID;
		}
	}

	window = Rwindow_create(description.title, description.width, description.height, description.flags);
	if (window == R_INVALID_ID) {
		if (content) rvn_widget_destroy(content);
		if (island) rvn_widget_destroy(island);
		return R_INVALID_ID;
	}
	window_object = rvn_window_get(window);
	if (window_object) window_object->xml_node = root->source;
	if (window_object && description.theme && description.theme[0]) {
		/* the theme of this window (the name of one, or its file): the other windows keep the one of the application */
		rvn_window_set_theme(window_object, rvn_xml_window_theme(description.theme));
	}
	if (description.min_width > 0 || description.min_height > 0) Rwindow_set_min_size(window, description.min_width, description.min_height);
	if (description.has_clear) {
		float r, g, b, a;
		rvn_color_to_floats(description.clear, &r, &g, &b, &a);
		Rwindow_set_clear_color(window, r, g, b, a);
	}
	if (description.background != R_INVALID_ID) Rwindow_set_background(window, description.background);
	if (description.backdrop) Rwindow_set_backdrop(window, (RBackdrop)description.backdrop);
	if (description.has_touch) Rwindow_set_touch_settings(window, &description.touch);
	if (content) Rwindow_set_content(window, content->id);
	if (island) Rwindow_set_island(window, island->id);
	if (window_object) rvn_xml_window_shortcuts(window_object, root, R_TRUE);
	return window;
}

Rid rvn_xml_build_window(rvn_xml_document_t* document, const char* function) {
	Rid window = rvn_xml_make_window(document);
	RVN_UNUSED(function);
	rvn_xml_free(document);
	return window;
}

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

Rid Rwidget_create_from_xml(const char* xml, size_t size) {
	RVN_API_GUARD(R_INVALID_ID);
	return rvn_xml_create_widget(xml, size, __func__);
}

Rid Rwindow_create_from_xml(const char* xml, size_t size) {
	RVN_API_GUARD(R_INVALID_ID);
	return rvn_xml_create_window(xml, size, __func__);
}

void rvn_xml_push_file_dir(const char* path, char** previous) {
	*previous = g_xml.file_dir;
	g_xml.file_dir = path ? rvn_path_dir(path) : NULL;
}

void rvn_xml_pop_file_dir(char* previous) {
	rvn_free(g_xml.file_dir);
	g_xml.file_dir = previous;
}

static Rid rvn_xml_create_from_file(const char* path, int is_window, const char* function) {
	rvn_xml_document_t document;
	size_t size = 0;
	char* text = rvn_read_file(path, &size);
	char* previous_dir;
	Rid result = R_INVALID_ID;
	if (!text) {
		rvn_error(R_ERROR_FILE_NOT_FOUND, R_SEVERITY_ERROR, "%s: cannot read '%s'", function, path ? path : "");
		return R_INVALID_ID;
	}
	/* relative paths inside the file are relative to the file */
	rvn_xml_push_file_dir(path, &previous_dir);
	/* (the files it includes are noted: a change in one of them is a change in this one) */
	rvn_live_begin();
	if (rvn_xml_load(&document, text, size, function)) {
		result = is_window ? rvn_xml_make_window(&document) : rvn_xml_make_widget(&document, R_FALSE);
		/* the document stays with what it built: when the file changes, the two are compared */
		if (result != R_INVALID_ID) rvn_live_add(path, is_window, result, &document);
		else rvn_xml_free(&document);
	}
	rvn_live_end();
	rvn_xml_pop_file_dir(previous_dir);
	rvn_free(text);
	return result;
}

Rid Rwidget_create_from_xml_file(const char* path) {
	RVN_API_GUARD(R_INVALID_ID);
	return rvn_xml_create_from_file(path, R_FALSE, __func__);
}

Rid Rwindow_create_from_xml_file(const char* path) {
	RVN_API_GUARD(R_INVALID_ID);
	return rvn_xml_create_from_file(path, R_TRUE, __func__);
}

int Rxml_register_widgets_from_file(const char* path) {
	size_t size = 0;
	char* text;
	char* previous_dir = g_xml.file_dir;
	int registered;
	RVN_API_GUARD(0);
	text = rvn_read_file(path, &size);
	if (!text) {
		rvn_error(R_ERROR_FILE_NOT_FOUND, R_SEVERITY_ERROR, "Rxml_register_widgets_from_file: cannot read '%s'", path ? path : "");
		return 0;
	}
	g_xml.file_dir = rvn_path_dir(path);
	registered = Rxml_register_widgets(text, size);
	rvn_free(g_xml.file_dir);
	g_xml.file_dir = previous_dir;
	rvn_free(text);
	/* a change in the file declares its widgets again and rebuilds what uses them */
	rvn_live_watch_widgets(path);
	return registered;
}

void Rxml_set_base_path(const char* path) {
	RVN_API_GUARD_VOID();
	rvn_str_set(&g_rvn.xml_base_path, (path && path[0]) ? path : NULL);
}

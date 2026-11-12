/**
 * Raven - xml/xml_dom.c
 * Documents that can be edited (Rxml_document_..., Rxml_node_...).
 *
 * The loader reads a text once and forgets it; a tool needs the tree to stay:
 * here each node is an object of its own, with a handle, that can be changed
 * and written back. To build widgets the tree is turned into the document of
 * the loader, every element carrying the handle of its node (`source`): the
 * widgets keep it, which is how a click in a preview finds its node.
 */

#include "xml.h"
#include "../widgets/common.h"
#include "../core/fs.h"

typedef struct rvn_dom_attribute_t {
	char* name;
	char* value;
} rvn_dom_attribute_t;

typedef struct rvn_dom_document_t rvn_dom_document_t;

typedef struct rvn_dom_node_t {
	Rid id;
	rvn_dom_document_t* document;
	struct rvn_dom_node_t* parent;
	struct rvn_dom_node_t** children;
	uint32_t child_count;
	uint32_t child_cap;
	/** NULL for a comment. */
	char* tag;
	/** Never NULL. */
	char* text;
	rvn_dom_attribute_t* attributes;
	uint32_t attribute_count;
	uint32_t attribute_cap;
	int line;
	int column;
	/** What the node became the last time the document was built. */
	Rid widget;
} rvn_dom_node_t;

struct rvn_dom_document_t {
	Rid id;
	rvn_dom_node_t* root;
	/** Comments written before the root. */
	rvn_dom_node_t** prolog;
	uint32_t prolog_count;
	uint32_t prolog_cap;
	int declaration;
	/** Folder of the file it was read from: the paths written in it start there. */
	char* file;
	/** Result of the last ..._to_string. */
	char* string;
};

static rvn_dom_document_t* rvn_dom_document(Rid id, const char* function) {
	rvn_dom_document_t* document;
	if (!rvn_api_check(function)) return NULL;
	document = (rvn_dom_document_t*)rvn_pool_get(&g_rvn.xml_documents, id);
	if (!document) rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "%s: invalid document handle 0x%08X", function, (unsigned)id);
	return document;
}

static rvn_dom_node_t* rvn_dom_node(Rid id, const char* function) {
	rvn_dom_node_t* node;
	if (!rvn_api_check(function)) return NULL;
	node = (rvn_dom_node_t*)rvn_pool_get(&g_rvn.xml_nodes, id);
	if (!node) rvn_error(R_ERROR_INVALID_ID, R_SEVERITY_ERROR, "%s: invalid node handle 0x%08X", function, (unsigned)id);
	return node;
}

/* ---------------------------------------------------------------------------
 *  Nodes
 * ------------------------------------------------------------------------- */

static rvn_dom_node_t* rvn_dom_new(rvn_dom_document_t* document, const char* tag, const char* text) {
	rvn_dom_node_t* node = (rvn_dom_node_t*)rvn_calloc(sizeof(rvn_dom_node_t));
	node->id = rvn_pool_add(&g_rvn.xml_nodes, node);
	if (node->id == R_INVALID_ID) {
		rvn_error(R_ERROR_OUT_OF_MEMORY, R_SEVERITY_ERROR, "XML document: too many nodes");
		rvn_free(node);
		return NULL;
	}
	node->document = document;
	node->tag = tag ? rvn_strdup(tag) : NULL;
	node->text = rvn_strdup(text ? text : "");
	return node;
}

static void rvn_dom_free(rvn_dom_node_t* node) {
	uint32_t i;
	for (i = 0; i < node->child_count; i++) rvn_dom_free(node->children[i]);
	for (i = 0; i < node->attribute_count; i++) {
		rvn_free(node->attributes[i].name);
		rvn_free(node->attributes[i].value);
	}
	rvn_pool_remove(&g_rvn.xml_nodes, node->id);
	rvn_free(node->children);
	rvn_free(node->attributes);
	rvn_free(node->tag);
	rvn_free(node->text);
	rvn_free(node);
}

static void rvn_dom_insert(rvn_dom_node_t* parent, rvn_dom_node_t* child, int index) {
	uint32_t at = (index < 0 || index > (int)parent->child_count) ? parent->child_count : (uint32_t)index;
	RVN_ARRAY_RESERVE(parent->children, parent->child_cap, parent->child_count + 1);
	memmove(parent->children + at + 1, parent->children + at, sizeof(rvn_dom_node_t*) * (parent->child_count - at));
	parent->children[at] = child;
	parent->child_count++;
	child->parent = parent;
}

static int rvn_dom_index(const rvn_dom_node_t* node) {
	uint32_t i;
	if (!node->parent) return 0;
	for (i = 0; i < node->parent->child_count; i++) {
		if (node->parent->children[i] == node) return (int)i;
	}
	return 0;
}

/*
 * A document is never nested deeper than the reader accepts
 * (RVN_XML_MAX_DEPTH): what walks it (freeing, copying, writing, building)
 * goes down one call per level, and what is written can be read back.
 */
static int rvn_dom_depth(const rvn_dom_node_t* node) {
	int depth = 0;
	for (; node->parent; node = node->parent) depth++;
	return depth;
}

/* Number of levels under a node: 0 when it has no children. */
static int rvn_dom_height(const rvn_dom_node_t* node) {
	int height = 0;
	uint32_t i;
	for (i = 0; i < node->child_count; i++) {
		int below = rvn_dom_height(node->children[i]) + 1;
		if (below > height) height = below;
	}
	return height;
}

/* Can a node with `height` levels under it become a child of `parent`? */
static int rvn_dom_fits(const rvn_dom_node_t* parent, int height, const char* function) {
	if (rvn_dom_depth(parent) + 1 + height <= RVN_XML_MAX_DEPTH) return R_TRUE;
	rvn_error(R_ERROR_XML, R_SEVERITY_ERROR, "%s: elements are nested too deeply (more than %d levels)", function, RVN_XML_MAX_DEPTH);
	return R_FALSE;
}

static void rvn_dom_detach(rvn_dom_node_t* node) {
	rvn_dom_node_t* parent = node->parent;
	uint32_t at;
	if (!parent) return;
	at = (uint32_t)rvn_dom_index(node);
	memmove(parent->children + at, parent->children + at + 1, sizeof(rvn_dom_node_t*) * (parent->child_count - at - 1));
	parent->child_count--;
	node->parent = NULL;
}

static rvn_dom_attribute_t* rvn_dom_find_attribute(const rvn_dom_node_t* node, const char* name) {
	uint32_t i;
	for (i = 0; i < node->attribute_count; i++) {
		if (rvn_stricmp(node->attributes[i].name, name) == 0) return &node->attributes[i];
	}
	return NULL;
}

static void rvn_dom_set_attribute(rvn_dom_node_t* node, const char* name, const char* value) {
	rvn_dom_attribute_t* attribute = rvn_dom_find_attribute(node, name);
	if (!value) {
		if (!attribute) return;
		rvn_free(attribute->name);
		rvn_free(attribute->value);
		memmove(attribute, attribute + 1, sizeof(*attribute) * (size_t)(node->attributes + node->attribute_count - attribute - 1));
		node->attribute_count--;
		return;
	}
	if (attribute) {
		rvn_str_set(&attribute->value, value);
		return;
	}
	RVN_ARRAY_RESERVE(node->attributes, node->attribute_cap, node->attribute_count + 1);
	attribute = &node->attributes[node->attribute_count++];
	attribute->name = rvn_strdup(name);
	attribute->value = rvn_strdup(value);
}

/* Can this be the name of a tag or of an attribute? */
static int rvn_dom_is_name(const char* name) {
	const char* cursor;
	if (!name || !name[0]) return R_FALSE;
	for (cursor = name; *cursor; cursor++) {
		char c = *cursor;
		int letter = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == ':' || (unsigned char)c >= 0x80;
		if (!letter && !(cursor > name && ((c >= '0' && c <= '9') || c == '.' || c == '-'))) return R_FALSE;
	}
	return R_TRUE;
}

/* A node, and what is under it, from what the parser read. */
static rvn_dom_node_t* rvn_dom_from_parsed(rvn_dom_document_t* document, const rvn_xml_node_t* parsed) {
	int comment = strcmp(parsed->name, RVN_XML_COMMENT) == 0;
	rvn_dom_node_t* node = rvn_dom_new(document, comment ? NULL : parsed->name, parsed->text);
	const rvn_xml_attribute_t* attribute;
	const rvn_xml_node_t* child;
	if (!node) return NULL;
	node->line = parsed->line;
	node->column = parsed->column;
	for (attribute = parsed->attributes; attribute; attribute = attribute->next) {
		RVN_ARRAY_RESERVE(node->attributes, node->attribute_cap, node->attribute_count + 1);
		node->attributes[node->attribute_count].name = rvn_strdup(attribute->name);
		node->attributes[node->attribute_count].value = rvn_strdup(attribute->value);
		node->attribute_count++;
	}
	for (child = parsed->first_child; child; child = child->next) {
		rvn_dom_node_t* child_node = rvn_dom_from_parsed(document, child);
		if (child_node) rvn_dom_insert(node, child_node, -1);
	}
	return node;
}

static rvn_dom_node_t* rvn_dom_copy(const rvn_dom_node_t* source) {
	rvn_dom_node_t* node = rvn_dom_new(source->document, source->tag, source->text);
	uint32_t i;
	if (!node) return NULL;
	for (i = 0; i < source->attribute_count; i++) rvn_dom_set_attribute(node, source->attributes[i].name, source->attributes[i].value);
	for (i = 0; i < source->child_count; i++) {
		rvn_dom_node_t* child = rvn_dom_copy(source->children[i]);
		if (child) rvn_dom_insert(node, child, -1);
	}
	return node;
}

/* ---------------------------------------------------------------------------
 *  Writing
 * ------------------------------------------------------------------------- */

typedef struct rvn_dom_writer_t {
	char* text;
	uint32_t count;
	uint32_t cap;
	int line;
	int column;
} rvn_dom_writer_t;

static void rvn_dom_put(rvn_dom_writer_t* writer, const char* text, size_t length) {
	size_t i;
	RVN_ARRAY_RESERVE(writer->text, writer->cap, writer->count + (uint32_t)length + 1);
	memcpy(writer->text + writer->count, text, length);
	writer->count += (uint32_t)length;
	writer->text[writer->count] = '\0';
	for (i = 0; i < length; i++) {
		if (text[i] == '\n') {
			writer->line++;
			writer->column = 1;
		} else {
			writer->column++;
		}
	}
}

static void rvn_dom_puts(rvn_dom_writer_t* writer, const char* text) {
	rvn_dom_put(writer, text, strlen(text));
}

/* Text with the characters that have a meaning in XML written as entities. */
static void rvn_dom_put_escaped(rvn_dom_writer_t* writer, const char* text, int attribute) {
	const char* start = text;
	const char* cursor;
	for (cursor = text; *cursor; cursor++) {
		const char* entity = NULL;
		if (*cursor == '&') entity = "&amp;";
		else if (*cursor == '<') entity = "&lt;";
		else if (*cursor == '>') entity = "&gt;";
		else if (attribute && *cursor == '"') entity = "&quot;";
		else if (attribute && *cursor == '\n') entity = "&#10;";
		else if (attribute && *cursor == '\t') entity = "&#9;";
		if (!entity) continue;
		rvn_dom_put(writer, start, (size_t)(cursor - start));
		rvn_dom_puts(writer, entity);
		start = cursor + 1;
	}
	rvn_dom_put(writer, start, (size_t)(cursor - start));
}

static void rvn_dom_indent(rvn_dom_writer_t* writer, int depth) {
	while (depth-- > 0) rvn_dom_put(writer, "\t", 1);
}

static void rvn_dom_write(rvn_dom_writer_t* writer, rvn_dom_node_t* node, int depth, int track) {
	uint32_t i;
	rvn_dom_indent(writer, depth);
	if (track) {
		node->line = writer->line;
		node->column = writer->column;
	}
	if (!node->tag) {
		rvn_dom_puts(writer, "<!--");
		rvn_dom_puts(writer, node->text);
		rvn_dom_puts(writer, "-->\n");
		return;
	}
	rvn_dom_puts(writer, "<");
	rvn_dom_puts(writer, node->tag);
	for (i = 0; i < node->attribute_count; i++) {
		rvn_dom_puts(writer, " ");
		rvn_dom_puts(writer, node->attributes[i].name);
		rvn_dom_puts(writer, "=\"");
		rvn_dom_put_escaped(writer, node->attributes[i].value, R_TRUE);
		rvn_dom_puts(writer, "\"");
	}
	if (node->child_count == 0 && !node->text[0]) {
		rvn_dom_puts(writer, "/>\n");
		return;
	}
	rvn_dom_puts(writer, ">");
	if (node->child_count == 0) {
		rvn_dom_put_escaped(writer, node->text, R_FALSE);
	} else {
		rvn_dom_puts(writer, "\n");
		if (node->text[0]) {
			rvn_dom_indent(writer, depth + 1);
			rvn_dom_put_escaped(writer, node->text, R_FALSE);
			rvn_dom_puts(writer, "\n");
		}
		for (i = 0; i < node->child_count; i++) rvn_dom_write(writer, node->children[i], depth + 1, track);
		rvn_dom_indent(writer, depth);
	}
	rvn_dom_puts(writer, "</");
	rvn_dom_puts(writer, node->tag);
	rvn_dom_puts(writer, ">\n");
}

/* ---------------------------------------------------------------------------
 *  Documents
 * ------------------------------------------------------------------------- */

static rvn_dom_document_t* rvn_dom_document_new(void) {
	rvn_dom_document_t* document = (rvn_dom_document_t*)rvn_calloc(sizeof(rvn_dom_document_t));
	document->id = rvn_pool_add(&g_rvn.xml_documents, document);
	if (document->id == R_INVALID_ID) {
		rvn_error(R_ERROR_OUT_OF_MEMORY, R_SEVERITY_ERROR, "XML document: too many documents");
		rvn_free(document);
		return NULL;
	}
	return document;
}

static void rvn_dom_document_free(rvn_dom_document_t* document) {
	uint32_t i;
	if (document->root) rvn_dom_free(document->root);
	for (i = 0; i < document->prolog_count; i++) rvn_dom_free(document->prolog[i]);
	rvn_pool_remove(&g_rvn.xml_documents, document->id);
	rvn_free(document->prolog);
	rvn_free(document->file);
	rvn_free(document->string);
	rvn_free(document);
}

void rvn_xml_dom_shutdown(void) {
	uint32_t i;
	for (i = 0; i < g_rvn.xml_documents.used; i++) {
		rvn_dom_document_t* document = (rvn_dom_document_t*)rvn_pool_at(&g_rvn.xml_documents, i);
		if (document) rvn_dom_document_free(document);
	}
}

Rid Rxml_document_create(const char* root_tag) {
	rvn_dom_document_t* document;
	RVN_API_GUARD(R_INVALID_ID);
	if (!rvn_dom_is_name(root_tag)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rxml_document_create: \"%.60s\" cannot be the name of a tag", root_tag ? root_tag : "");
		return R_INVALID_ID;
	}
	document = rvn_dom_document_new();
	if (!document) return R_INVALID_ID;
	document->root = rvn_dom_new(document, root_tag, "");
	if (!document->root) {
		rvn_dom_document_free(document);
		return R_INVALID_ID;
	}
	return document->id;
}

static Rid rvn_dom_parse(const char* xml, size_t size, RXmlError* error, const char* function) {
	rvn_xml_document_t parsed;
	rvn_dom_document_t* document;
	const rvn_xml_node_t* comment;

	if (error) memset(error, 0, sizeof(*error));
	if (!xml) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "%s: NULL text", function);
		return R_INVALID_ID;
	}
	if (!rvn_xml_parse_keeping_comments(&parsed, xml, size)) {
		if (error) {
			error->line = parsed.error_line;
			error->column = parsed.error_column;
			snprintf(error->message, sizeof(error->message), "%s", parsed.error);
		} else {
			rvn_error(R_ERROR_XML, R_SEVERITY_ERROR, "%s: line %d, column %d: %s", function, parsed.error_line, parsed.error_column, parsed.error);
		}
		rvn_xml_free(&parsed);
		return R_INVALID_ID;
	}
	document = rvn_dom_document_new();
	if (!document) {
		rvn_xml_free(&parsed);
		return R_INVALID_ID;
	}
	document->declaration = parsed.has_declaration;
	for (comment = parsed.prolog; comment; comment = comment->next) {
		rvn_dom_node_t* node = rvn_dom_from_parsed(document, comment);
		if (!node) continue;
		RVN_ARRAY_RESERVE(document->prolog, document->prolog_cap, document->prolog_count + 1);
		document->prolog[document->prolog_count++] = node;
	}
	document->root = rvn_dom_from_parsed(document, parsed.root);
	rvn_xml_free(&parsed);
	if (!document->root) {
		rvn_dom_document_free(document);
		return R_INVALID_ID;
	}
	return document->id;
}

Rid Rxml_document_parse(const char* xml, size_t size, RXmlError* error) {
	RVN_API_GUARD(R_INVALID_ID);
	return rvn_dom_parse(xml, size, error, __func__);
}

Rid Rxml_document_load(const char* path) {
	size_t size = 0;
	char* text;
	Rid id;
	RVN_API_GUARD(R_INVALID_ID);
	text = rvn_read_file(path, &size);
	if (!text) {
		rvn_error(R_ERROR_FILE_NOT_FOUND, R_SEVERITY_ERROR, "Rxml_document_load: cannot read '%s'", path ? path : "");
		return R_INVALID_ID;
	}
	id = rvn_dom_parse(text, size, NULL, __func__);
	rvn_free(text);
	if (id != R_INVALID_ID) ((rvn_dom_document_t*)rvn_pool_get(&g_rvn.xml_documents, id))->file = rvn_strdup(path);
	return id;
}

void Rxml_document_destroy(Rid id) {
	rvn_dom_document_t* document = rvn_dom_document(id, __func__);
	if (document) rvn_dom_document_free(document);
}

int Rxml_document_is_valid(Rid id) {
	return g_rvn.initialized && rvn_pool_get(&g_rvn.xml_documents, id) != NULL;
}

Rid Rxml_document_get_root(Rid id) {
	rvn_dom_document_t* document = rvn_dom_document(id, __func__);
	return document ? document->root->id : R_INVALID_ID;
}

const char* Rxml_document_to_string(Rid id) {
	rvn_dom_document_t* document = rvn_dom_document(id, __func__);
	rvn_dom_writer_t writer;
	uint32_t i;
	if (!document) return "";
	memset(&writer, 0, sizeof(writer));
	writer.line = 1;
	writer.column = 1;
	if (document->declaration) rvn_dom_puts(&writer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	for (i = 0; i < document->prolog_count; i++) rvn_dom_write(&writer, document->prolog[i], 0, R_TRUE);
	rvn_dom_write(&writer, document->root, 0, R_TRUE);
	rvn_free(document->string);
	document->string = writer.text;
	return document->string;
}

int Rxml_document_save(Rid id, const char* path) {
	rvn_dom_document_t* document = rvn_dom_document(id, __func__);
	const char* text;
	if (!document) return R_FALSE;
	if (!path || !path[0]) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rxml_document_save: no path");
		return R_FALSE;
	}
	text = Rxml_document_to_string(id);
	if (!rvn_write_file(path, text, strlen(text))) {
		rvn_error(R_ERROR_FILE_NOT_FOUND, R_SEVERITY_ERROR, "Rxml_document_save: cannot write '%s'", path);
		return R_FALSE;
	}
	rvn_str_set(&document->file, path);
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Building
 * ------------------------------------------------------------------------- */

/* The element of the loader for a node, with what is under it; comments are left out. */
static rvn_xml_node_t* rvn_dom_to_parsed(rvn_arena_t* arena, rvn_dom_node_t* node, rvn_xml_node_t* parent) {
	rvn_xml_node_t* parsed = (rvn_xml_node_t*)rvn_arena_alloc(arena, sizeof(rvn_xml_node_t));
	rvn_xml_attribute_t* last = NULL;
	uint32_t i;
	memset(parsed, 0, sizeof(*parsed));
	parsed->name = rvn_arena_strndup(arena, node->tag, strlen(node->tag));
	parsed->text = rvn_arena_strndup(arena, node->text, strlen(node->text));
	parsed->parent = parent;
	parsed->line = node->line;
	parsed->column = node->column;
	parsed->source = node->id;
	node->widget = R_INVALID_ID;
	for (i = 0; i < node->attribute_count; i++) {
		rvn_xml_attribute_t* attribute = (rvn_xml_attribute_t*)rvn_arena_alloc(arena, sizeof(rvn_xml_attribute_t));
		attribute->name = rvn_arena_strndup(arena, node->attributes[i].name, strlen(node->attributes[i].name));
		attribute->value = rvn_arena_strndup(arena, node->attributes[i].value, strlen(node->attributes[i].value));
		attribute->next = NULL;
		if (last) last->next = attribute;
		else parsed->attributes = attribute;
		last = attribute;
	}
	for (i = 0; i < node->child_count; i++) {
		rvn_xml_node_t* child;
		if (!node->children[i]->tag) continue;
		child = rvn_dom_to_parsed(arena, node->children[i], parsed);
		if (parsed->last_child) parsed->last_child->next = child;
		else parsed->first_child = child;
		parsed->last_child = child;
	}
	return parsed;
}

/* Each node learns the first widget that was built from it: for a declared tag, the widget it stands for. */
static void rvn_dom_bind(rvn_dom_document_t* document, rvn_widget_t* widget) {
	rvn_dom_node_t* node = widget->xml_node ? (rvn_dom_node_t*)rvn_pool_get(&g_rvn.xml_nodes, widget->xml_node) : NULL;
	uint32_t i;
	if (node && node->document == document && node->widget == R_INVALID_ID) node->widget = widget->id;
	for (i = 0; i < widget->child_count; i++) rvn_dom_bind(document, widget->children[i]);
}

static Rid rvn_dom_build(rvn_dom_document_t* document, int window, const char* function) {
	rvn_xml_document_t parsed;
	char* previous_dir;
	Rid result = R_INVALID_ID;

	memset(&parsed, 0, sizeof(parsed));
	rvn_arena_init(&parsed.arena, 16384);
	parsed.root = rvn_dom_to_parsed(&parsed.arena, document->root, NULL);
	rvn_xml_push_file_dir(document->file, &previous_dir);
	if (!rvn_xml_expand(&parsed, NULL)) {
		rvn_xml_free(&parsed);
	} else if (!parsed.root) {
		rvn_error(R_ERROR_XML, R_SEVERITY_ERROR, "%s: the document declares widgets but contains nothing to create", function);
		rvn_xml_free(&parsed);
	} else if (window) {
		rvn_window_t* built;
		result = rvn_xml_build_window(&parsed, function);
		built = rvn_window_get(result);
		if (built) {
			document->root->widget = result;
			if (built->root) rvn_dom_bind(document, built->root);
			if (built->island) rvn_dom_bind(document, built->island);
		}
	} else {
		rvn_widget_t* built;
		result = rvn_xml_build_widget(&parsed, R_TRUE, function);
		built = rvn_widget_get(result);
		if (built) rvn_dom_bind(document, built);
	}
	rvn_xml_pop_file_dir(previous_dir);
	return result;
}

Rid Rxml_document_create_widget(Rid id) {
	rvn_dom_document_t* document = rvn_dom_document(id, __func__);
	return document ? rvn_dom_build(document, R_FALSE, __func__) : R_INVALID_ID;
}

Rid Rxml_document_create_window(Rid id) {
	rvn_dom_document_t* document = rvn_dom_document(id, __func__);
	return document ? rvn_dom_build(document, R_TRUE, __func__) : R_INVALID_ID;
}

Rid Rxml_node_get_widget(Rid id) {
	rvn_dom_node_t* node = rvn_dom_node(id, __func__);
	if (!node || node->widget == R_INVALID_ID) return R_INVALID_ID;
	if (!rvn_widget_get(node->widget) && !rvn_window_get(node->widget)) return R_INVALID_ID;
	return node->widget;
}

Rid Rwidget_get_xml_node(Rid id) {
	rvn_widget_t* widget;
	RVN_API_GUARD(R_INVALID_ID);
	/* (R_INVALID_ID is what a hit test gives when nothing is there: not an error) */
	if (id == R_INVALID_ID) return R_INVALID_ID;
	widget = rvn_widget_check(id, 0, __func__);
	if (!widget || !widget->xml_node || !rvn_pool_get(&g_rvn.xml_nodes, widget->xml_node)) return R_INVALID_ID;
	return widget->xml_node;
}

/* ---------------------------------------------------------------------------
 *  Nodes: public API
 * ------------------------------------------------------------------------- */

int Rxml_node_is_valid(Rid id) {
	return g_rvn.initialized && rvn_pool_get(&g_rvn.xml_nodes, id) != NULL;
}

Rid Rxml_node_get_document(Rid id) {
	rvn_dom_node_t* node = rvn_dom_node(id, __func__);
	return node ? node->document->id : R_INVALID_ID;
}

const char* Rxml_node_get_tag(Rid id) {
	rvn_dom_node_t* node = rvn_dom_node(id, __func__);
	return node && node->tag ? node->tag : "";
}

void Rxml_node_set_tag(Rid id, const char* tag) {
	rvn_dom_node_t* node = rvn_dom_node(id, __func__);
	if (!node) return;
	if (!node->tag || !rvn_dom_is_name(tag)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, node->tag ? "Rxml_node_set_tag: \"%.60s\" cannot be the name of a tag"
			: "Rxml_node_set_tag: a comment has no tag%.0s", tag ? tag : "");
		return;
	}
	rvn_str_set(&node->tag, tag);
}

int Rxml_node_is_comment(Rid id) {
	rvn_dom_node_t* node = rvn_dom_node(id, __func__);
	return node && !node->tag;
}

const char* Rxml_node_get_text(Rid id) {
	rvn_dom_node_t* node = rvn_dom_node(id, __func__);
	return node ? node->text : "";
}

void Rxml_node_set_text(Rid id, const char* text) {
	rvn_dom_node_t* node = rvn_dom_node(id, __func__);
	if (!node) return;
	if (!text) text = "";
	if (!node->tag && strstr(text, "--")) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rxml_node_set_text: a comment cannot contain \"--\"");
		return;
	}
	rvn_str_set(&node->text, text);
	if (!node->text) node->text = rvn_strdup("");
}

Rid Rxml_node_get_parent(Rid id) {
	rvn_dom_node_t* node = rvn_dom_node(id, __func__);
	return node && node->parent ? node->parent->id : R_INVALID_ID;
}

int Rxml_node_get_child_count(Rid id) {
	rvn_dom_node_t* node = rvn_dom_node(id, __func__);
	return node ? (int)node->child_count : 0;
}

Rid Rxml_node_get_child(Rid id, int index) {
	rvn_dom_node_t* node = rvn_dom_node(id, __func__);
	if (!node || index < 0 || index >= (int)node->child_count) return R_INVALID_ID;
	return node->children[index]->id;
}

int Rxml_node_get_index(Rid id) {
	rvn_dom_node_t* node = rvn_dom_node(id, __func__);
	return node ? rvn_dom_index(node) : 0;
}

/* The parent of a node about to be added: an element of a document. */
static rvn_dom_node_t* rvn_dom_parent(Rid id, const char* function) {
	rvn_dom_node_t* parent = rvn_dom_node(id, function);
	if (parent && !parent->tag) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "%s: a comment cannot contain anything", function);
		return NULL;
	}
	/* (the comments before the root have no parent and take no children either) */
	return parent;
}

Rid Rxml_node_add_child(Rid parent_id, const char* tag, int index) {
	rvn_dom_node_t* parent = rvn_dom_parent(parent_id, __func__);
	rvn_dom_node_t* node;
	if (!parent) return R_INVALID_ID;
	if (!rvn_dom_is_name(tag)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rxml_node_add_child: \"%.60s\" cannot be the name of a tag", tag ? tag : "");
		return R_INVALID_ID;
	}
	if (!rvn_dom_fits(parent, 0, __func__)) return R_INVALID_ID;
	node = rvn_dom_new(parent->document, tag, "");
	if (!node) return R_INVALID_ID;
	rvn_dom_insert(parent, node, index);
	return node->id;
}

Rid Rxml_node_add_comment(Rid parent_id, const char* text, int index) {
	rvn_dom_node_t* parent = rvn_dom_parent(parent_id, __func__);
	rvn_dom_node_t* node;
	if (!parent) return R_INVALID_ID;
	if (text && strstr(text, "--")) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rxml_node_add_comment: a comment cannot contain \"--\"");
		return R_INVALID_ID;
	}
	if (!rvn_dom_fits(parent, 0, __func__)) return R_INVALID_ID;
	node = rvn_dom_new(parent->document, NULL, text);
	if (!node) return R_INVALID_ID;
	rvn_dom_insert(parent, node, index);
	return node->id;
}

Rid Rxml_node_add_xml(Rid parent_id, const char* xml, size_t size, int index) {
	rvn_dom_node_t* parent = rvn_dom_parent(parent_id, __func__);
	rvn_xml_document_t parsed;
	rvn_dom_node_t* node;
	if (!parent) return R_INVALID_ID;
	if (!xml) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, "Rxml_node_add_xml: NULL text");
		return R_INVALID_ID;
	}
	if (!rvn_xml_parse_keeping_comments(&parsed, xml, size)) {
		rvn_error(R_ERROR_XML, R_SEVERITY_ERROR, "Rxml_node_add_xml: line %d, column %d: %s", parsed.error_line, parsed.error_column, parsed.error);
		rvn_xml_free(&parsed);
		return R_INVALID_ID;
	}
	node = rvn_dom_from_parsed(parent->document, parsed.root);
	rvn_xml_free(&parsed);
	if (!node) return R_INVALID_ID;
	if (!rvn_dom_fits(parent, rvn_dom_height(node), __func__)) {
		rvn_dom_free(node);
		return R_INVALID_ID;
	}
	/* (its lines were those of the piece of text, not of the document) */
	node->line = 0;
	node->column = 0;
	rvn_dom_insert(parent, node, index);
	return node->id;
}

void Rxml_node_remove(Rid id) {
	rvn_dom_node_t* node = rvn_dom_node(id, __func__);
	if (!node) return;
	if (!node->parent) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rxml_node_remove: the root of a document cannot be removed");
		return;
	}
	rvn_dom_detach(node);
	rvn_dom_free(node);
}

int Rxml_node_move(Rid id, Rid new_parent_id, int index) {
	rvn_dom_node_t* node = rvn_dom_node(id, __func__);
	rvn_dom_node_t* parent = node ? rvn_dom_parent(new_parent_id, __func__) : NULL;
	rvn_dom_node_t* walk;
	if (!node || !parent || !node->parent || parent->document != node->document) return R_FALSE;
	for (walk = parent; walk; walk = walk->parent) {
		if (walk == node) return R_FALSE;
	}
	if (!rvn_dom_fits(parent, rvn_dom_height(node), __func__)) return R_FALSE;
	rvn_dom_detach(node);
	rvn_dom_insert(parent, node, index);
	return R_TRUE;
}

Rid Rxml_node_duplicate(Rid id) {
	rvn_dom_node_t* node = rvn_dom_node(id, __func__);
	rvn_dom_node_t* copy;
	if (!node) return R_INVALID_ID;
	if (!node->parent) {
		rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR, "Rxml_node_duplicate: a document has a single root");
		return R_INVALID_ID;
	}
	copy = rvn_dom_copy(node);
	if (!copy) return R_INVALID_ID;
	rvn_dom_insert(node->parent, copy, rvn_dom_index(node) + 1);
	return copy->id;
}

const char* Rxml_node_to_string(Rid id) {
	rvn_dom_node_t* node = rvn_dom_node(id, __func__);
	rvn_dom_writer_t writer;
	if (!node) return "";
	memset(&writer, 0, sizeof(writer));
	rvn_dom_write(&writer, node, 0, R_FALSE);
	rvn_free(node->document->string);
	node->document->string = writer.text;
	return writer.text;
}

int Rxml_node_get_attribute_count(Rid id) {
	rvn_dom_node_t* node = rvn_dom_node(id, __func__);
	return node ? (int)node->attribute_count : 0;
}

const char* Rxml_node_get_attribute_name(Rid id, int index) {
	rvn_dom_node_t* node = rvn_dom_node(id, __func__);
	if (!node || index < 0 || index >= (int)node->attribute_count) return NULL;
	return node->attributes[index].name;
}

const char* Rxml_node_get_attribute_value(Rid id, int index) {
	rvn_dom_node_t* node = rvn_dom_node(id, __func__);
	if (!node || index < 0 || index >= (int)node->attribute_count) return NULL;
	return node->attributes[index].value;
}

const char* Rxml_node_get_attribute(Rid id, const char* name) {
	rvn_dom_node_t* node = rvn_dom_node(id, __func__);
	rvn_dom_attribute_t* attribute = node && name ? rvn_dom_find_attribute(node, name) : NULL;
	return attribute ? attribute->value : NULL;
}

void Rxml_node_set_attribute(Rid id, const char* name, const char* value) {
	rvn_dom_node_t* node = rvn_dom_node(id, __func__);
	if (!node) return;
	if (!node->tag || !rvn_dom_is_name(name)) {
		rvn_error(R_ERROR_INVALID_VALUE, R_SEVERITY_ERROR, node->tag ? "Rxml_node_set_attribute: \"%.60s\" cannot be the name of an attribute"
			: "Rxml_node_set_attribute: a comment has no attributes%.0s", name ? name : "");
		return;
	}
	rvn_dom_set_attribute(node, name, value);
}

int Rxml_node_get_line(Rid id) {
	rvn_dom_node_t* node = rvn_dom_node(id, __func__);
	return node ? node->line : 0;
}

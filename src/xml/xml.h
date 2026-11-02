/**
 * Raven - xml/xml.h
 * Minimal XML reader: elements, attributes, text, comments, CDATA and the
 * predefined / numeric entities. No DTD, no namespaces.
 *
 * The document owns every string through one arena: releasing it frees all.
 */

#ifndef RVN_XML_INTERNAL_H
#define RVN_XML_INTERNAL_H

#include "../core/base.h"

struct rvn_widget_t;
struct rvn_window_t;

typedef struct rvn_xml_attribute_t {
	const char* name;
	const char* value;
	struct rvn_xml_attribute_t* next;
} rvn_xml_attribute_t;

typedef struct rvn_xml_node_t {
	const char* name;
	/** Text content of the element with surrounding white space removed; "" when it has none. */
	const char* text;
	rvn_xml_attribute_t* attributes;
	struct rvn_xml_node_t* parent;
	struct rvn_xml_node_t* first_child;
	struct rvn_xml_node_t* last_child;
	struct rvn_xml_node_t* next;
	/** Position of the opening tag in the source, starting at 1. */
	int line;
	int column;
	/**
	 * Node of a public document (Rxml_document_...) this element was made
	 * from, or 0. The widgets built from the element remember it.
	 */
	Rid source;
	/** Widget built from the element (documents kept for live reload), or 0. */
	Rid widget;
} rvn_xml_node_t;

/** Name of the nodes that hold a comment, when the parser keeps them: the text is the comment. */
#define RVN_XML_COMMENT "#comment"

typedef struct rvn_xml_document_t {
	rvn_arena_t arena;
	rvn_xml_node_t* root;
	/** Comments are kept, as nodes named RVN_XML_COMMENT (those before the root in `prolog`, in order). */
	int keep_comments;
	rvn_xml_node_t* prolog;
	rvn_xml_node_t* prolog_last;
	/** The text starts with an <?xml ...?> declaration. */
	int has_declaration;
	/** Filled when parsing fails. */
	char error[160];
	int error_line;
	int error_column;
} rvn_xml_document_t;

/** Parses `size` bytes of UTF-8. Returns R_FALSE and fills `error` on failure. */
int  rvn_xml_parse(rvn_xml_document_t* document, const char* text, size_t size);
/** The same, keeping the comments (for tools that write the document back). */
int  rvn_xml_parse_keeping_comments(rvn_xml_document_t* document, const char* text, size_t size);
void rvn_xml_free(rvn_xml_document_t* document);
/** Value of an attribute (name compared without case), or NULL. */
const char* rvn_xml_attribute(const rvn_xml_node_t* node, const char* name);

/**
 * Rewrites a parsed document into a plain one (xml_components.c): the
 * <Widget> declarations are registered and removed, their uses are replaced
 * by their bodies, <Repeat>, <ForEach>, <If> and <Include> are carried out.
 * The root is NULL afterwards when the document only held declarations.
 * `registered` (may be NULL) receives the number of widgets it declared.
 * Returns R_FALSE, after reporting the error, when the document cannot be used.
 */
int  rvn_xml_expand(rvn_xml_document_t* document, int* registered);
void rvn_xml_components_shutdown(void);
/** Releases the public documents the application left behind (xml_dom.c). */
void rvn_xml_dom_shutdown(void);
/** The i-th parameter of a declared widget and its default value. R_FALSE past the last one, or when the name is unknown. */
int  rvn_xml_component_parameter(const char* component, int index, const char** name, const char** default_value);

/**
 * Builds what an expanded document describes (xml_loader.c) and releases the
 * document. `function` names the caller in the errors.
 * rvn_xml_build_widget: with `window_content`, a <Window> root gives its
 * content (R_INVALID_ID, without an error, when it has none).
 */
Rid rvn_xml_build_window(rvn_xml_document_t* document, const char* function);
Rid rvn_xml_build_widget(rvn_xml_document_t* document, int window_content, const char* function);
/** Parses and expands a text: what the build functions take. R_FALSE after reporting the error. */
int rvn_xml_load(rvn_xml_document_t* document, const char* xml, size_t size, const char* function);
/** Tags made by code (Rxml_register_tag): the index + 1 of a tag, or 0. */
int rvn_xml_tag_find(const char* name);
/** Gives an attribute to the tag a widget was built for. R_FALSE when it has none, or does not know the attribute. */
int rvn_xml_tag_attribute(struct rvn_widget_t* widget, const char* name, const char* value);
/** Builds the widget of an element and of everything inside it. NULL after reporting the error. */
struct rvn_widget_t* rvn_xml_build_node(const rvn_xml_node_t* node);
/** What the attributes of a <Window> say. */
typedef struct rvn_xml_window_t {
	const char* title;
	int width, height;
	int min_width, min_height;
	Ruint flags;
	int has_clear;
	rvn_color_t clear;
	Rid background;
	/** RBackdrop, or 0 when not given. */
	int backdrop;
	const char* theme;
	/** The Touch.* and Pen.* attributes, applied to the settings of the application. */
	int has_touch;
	RTouchSettings touch;
} rvn_xml_window_t;
void rvn_xml_window_read(const rvn_xml_node_t* root, rvn_xml_window_t* description, int report);
/** The element of a <Window> that is its content, and the one that is its island (NULL when missing). R_FALSE when it has several contents. */
int  rvn_xml_window_parts(const rvn_xml_node_t* root, const rvn_xml_node_t** content, const rvn_xml_node_t** island);
/** Adds (or removes) the <Shortcut> elements of a <Window>. */
void rvn_xml_window_shortcuts(struct rvn_window_t* window, const rvn_xml_node_t* root, int add);
/** Same as rvn_xml_build_window / rvn_xml_build_widget, leaving the document to the caller. */
Rid rvn_xml_make_window(rvn_xml_document_t* document);
Rid rvn_xml_make_widget(rvn_xml_document_t* document, int window_content);

/** Runs `load` with the paths of the documents relative to the folder of `path` (NULL: the base path). */
void rvn_xml_push_file_dir(const char* path, char** previous);
void rvn_xml_pop_file_dir(char* previous);

/** Path of a file named in a document, relative to the document or to the base path. Free with rvn_free. */
char* rvn_xml_resolve_path(const char* path);

#endif /* RVN_XML_INTERNAL_H */

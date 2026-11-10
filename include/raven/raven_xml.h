/**
 * Raven
 * raven_xml.h : building windows and widget trees from XML.
 *
 *     <Window Title="Editor" Width="1280" Height="720">
 *       <VBox>
 *         <Label Text="Hello" Slot.Padding="8" Slot.HAlign="Center"/>
 *         <HBox Slot.Fill="1" Spacing="4">
 *           <Button Name="save" Text="Save" OnClicked="file.save"/>
 *           <Viewport Name="scene" Slot.Fill="1"/>
 *         </HBox>
 *       </VBox>
 *     </Window>
 *
 *   - An element is a widget; its tag is the widget type (VBox, Button, ...).
 *   - Child elements are child widgets, in order.
 *   - Attributes set properties. Attributes starting with "Slot." set the
 *     slot of the widget inside its parent (same meaning as Rslot_set_...).
 *   - Tags and attribute names are not case sensitive.
 *   - Name="..." gives the widget a name for Rwidget_find.
 *   - OnClicked / OnChanged / OnCommitted / OnActivated="name" set the action
 *     of the widget (Raction_bind). Some work with no C at all: "app.quit",
 *     "window.close", "show:panel", "toggle:panel", "page:pages=+1",
 *     "theme.toggle", "open:https://..." (Raction_set_builtins lists them).
 *
 *
 * NEW TAGS
 *
 * A document can declare its own tags: a <Widget> names a piece of
 * interface, with parameters, and is then used like any built-in widget.
 *
 *     <Widget Name="Field" Label="Name" Hint="" Wide="false">
 *       <HBox Spacing="8">
 *         <Label Text="{Label}:" Width="{Wide ? 140 : 90}"/>
 *         <TextInput Placeholder="{Hint}" Slot.Fill="1"/>
 *         <Children/>
 *       </HBox>
 *     </Widget>
 *
 *     <Field Label="Email" Hint="you@example.com" Name="email"/>
 *     <Field Label="Password" Wide="true"><Button Text="Show"/></Field>
 *
 *   - The attributes of <Widget> other than Name are its parameters, with
 *     their default values.
 *   - Inside it, {...} is computed: a parameter, or an expression of them
 *     (+ - * / %, comparisons, && || !, a ? b : c, 'text', min, max, round,
 *     floor, ceil, abs, len, upper, lower, empty, default). "{{" is a brace.
 *   - <Children/> is replaced by the children given where the tag is used;
 *     <Children Name="x"/> by those marked Into="x".
 *   - Where the tag is used, the attributes that are not parameters go to the
 *     widget it becomes (Name, Slot.Fill, OnClicked, Material...).
 *   - If="..." keeps or drops an element; <Repeat Count="3" Var="i"> and
 *     <ForEach Items="a|b|c" Var="item"> repeat their content;
 *     <Include Source="file.xml"/> brings in another file.
 *   - A declaration is known from the moment it is read until the library
 *     shuts down, in every document loaded afterwards.
 *
 *
 * DOCUMENTS
 *
 * A tool that edits an interface (the designer does) needs more than loading
 * a text: Rxml_document_parse gives the document as a tree of nodes that can
 * be read, changed, written back and built. A widget built from a document
 * knows the node it comes from, and the node the widget it became:
 *
 *     Rid document = Rxml_document_load("main.xml");
 *     Rid button = Rxml_node_add_child(Rxml_document_get_root(document), "Button", -1);
 *     Rxml_node_set_attribute(button, "Text", "Save");
 *     Rid preview = Rxml_document_create_widget(document);
 *     ...
 *     Rid node = Rwidget_get_xml_node(Rwidget_hit_test(preview, x, y));
 *     Rxml_document_save(document, "main.xml");
 *
 * Rxml_get_tag_count and Rxml_get_attribute_info list what can be written:
 * the tags, and for each tag its attributes with the kind of value they take.
 *
 * The complete list of tags and attributes is in docs/XML.md.
 */

#ifndef RAVEN_XML_H
#define RAVEN_XML_H

#include "raven_core.h"

/** Where and why a text could not be read (Rxml_document_parse). */
typedef struct RXmlError {
	/** Both start at 1. */
	int line;
	int column;
	char message[160];
} RXmlError;

/** The kind of value an attribute takes. */
typedef enum RXmlValueType{
	R_XML_TEXT = 0x03C0,
	/** "true" or "false". */
	R_XML_BOOL,
	R_XML_NUMBER,
	/** One of the words of `choices`. */
	R_XML_CHOICE,
	/** "#RGB", "#RRGGBB", "#RRGGBBAA", or a name: white, black, red, transparent... */
	R_XML_COLOR,
	/** The name of an action (Raction_bind). */
	R_XML_ACTION,
	/** The path of a file. */
	R_XML_FILE,
	/** Several numbers separated by commas: "8,4", "0,0,1,1". */
	R_XML_NUMBERS,
	/** Texts separated by '|': "One|Two|Three". */
	R_XML_LIST
}RXmlValueType;

/** One attribute of a tag (Rxml_get_attribute_info). The strings belong to the library. */
typedef struct RXmlAttributeInfo {
	const char* name;
	RXmlValueType type;
	/** R_XML_CHOICE: the words, separated by '|'. NULL otherwise. */
	const char* choices;
	/** What it belongs to: the tag itself ("Button"), "Widget" for what every widget has, "Slot" for the place in the parent. */
	const char* group;
	/** One line saying what it does. */
	const char* description;
} RXmlAttributeInfo;

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * @brief Creates a window from a document whose root element is <Window>.
	 * @param xml  UTF-8 text. It does not need to be null-terminated.
	 * @param size Size of the text in bytes.
	 * @return The window, or R_INVALID_ID. Parse errors are reported with
	 *         their line and column through the error callback (R_ERROR_XML).
	 */
	RAVEN_API Rid Rwindow_create_from_xml(const char* xml, size_t size);
	RAVEN_API Rid Rwindow_create_from_xml_file(const char* path);

	/**
	 * @brief Creates a widget tree from a document whose root element is a
	 * widget. The caller owns the returned widget until it is added to a parent.
	 */
	RAVEN_API Rid Rwidget_create_from_xml(const char* xml, size_t size);
	RAVEN_API Rid Rwidget_create_from_xml_file(const char* path);

	/* -----------------------------------------------------------------------
	 *  Where files are looked for
	 *
	 *  A relative path written in a document (Source="...", Font="...",
	 *  <Include>, <Strings>, Theme="file.xml") is looked for, in this order:
	 *    1. the folder of the document being loaded (the *FromXmlFile
	 *       functions), then its subfolders, the closest first (8 levels at
	 *       most; hidden folders and node_modules are skipped);
	 *    2. the base path (Rxml_set_base_path);
	 *    3. the current folder;
	 *    4. the folders of the PATH environment variable, in their order.
	 *  The first place that holds the file wins; a file of
	 *  Rfile_register_memory counts. An absolute path or a URL is used as it is.
	 *
	 *  Auto-loading, on by default, also takes from these places what a
	 *  document may need before it is read, without any register call:
	 *  - the translation tables: the *.csv files whose first column is
	 *    "key" (as with Rlocale_load_csv);
	 *  - the widget declarations: the *.xml files whose root is <Widgets>
	 *    or <Widget> (as with Rxml_register_widgets_from_file).
	 *  Those of PATH, of the current folder and of the base path are loaded
	 *  before the first document; those of the folder of a file and of its
	 *  subfolders before that file (again when it is reloaded, for the files
	 *  added since). Each file is loaded once, in the order of the places
	 *  from the last to the first, so that the closest one wins when two of
	 *  them give the same key or the same widget. A tag that still names
	 *  nothing, <Card/>, is then looked for as the file Card.xml.
	 *  The register functions keep working as before, and load what they are
	 *  given wherever it is.
	 * --------------------------------------------------------------------- */

	/**
	 * @brief A folder looked into after the one of the document (see above),
	 * and the first one for the documents given from memory. NULL or "": none.
	 */
	RAVEN_API void Rxml_set_base_path(const char* path);

	/**
	 * @brief Turns auto-loading on or off (see above). It is on by default;
	 * the environment variable RAVEN_AUTO_LOAD=0 turns it off without
	 * rebuilding. Turned off, the register functions are the only way in;
	 * the places where paths are looked for stay the same.
	 */
	RAVEN_API void Rxml_set_auto_load(int enabled);
	RAVEN_API int Rxml_get_auto_load(void);

	/* -----------------------------------------------------------------------
	 *  Live reload
	 *
	 *  What is created from a file (Rwindow_create_from_xml_file,
	 *  Rwidget_create_from_xml_file, the files they <Include>, the files of
	 *  Rxml_register_widgets_from_file, the theme files) follows its file:
	 *  save it in any editor and the window changes while the application
	 *  runs. Nothing is rebuilt that did not change: an element that is
	 *  still in the file keeps its widget, so the handles of the
	 *  application, its callbacks, what was typed and where things were
	 *  scrolled all stay; only the attributes that changed are set again.
	 *  Elements are matched by their Name when they have one, else by tag
	 *  and order. A file with a mistake leaves the interface as it was and
	 *  reports the error.
	 *
	 *  Texts given from memory are not concerned.
	 * --------------------------------------------------------------------- */

	/**
	 * @brief Turns live reload on or off. It is on by default; the
	 * environment variable RAVEN_LIVE_RELOAD=0 turns it off without
	 * rebuilding. When off, files are never looked at again.
	 */
	RAVEN_API void Rxml_set_live_reload(int enabled);
	RAVEN_API int Rxml_get_live_reload(void);

	/**
	 * @brief Called each time a file was loaded again.
	 * @param path The file given when the interface was created.
	 * @param ok   R_FALSE when the file could not be used (the interface is unchanged).
	 *
	 * Widgets that had to be built again have new handles: this is the
	 * place to look them up again (Rwidget_find).
	 */
	typedef void (*RXmlReloadCallback)(const char* path, int ok, void* user_data);
	RAVEN_API void Rxml_set_reload_callback(RXmlReloadCallback callback, void* user_data);

	/**
	 * @brief Looks at the files now, without waiting for the next look
	 * (there are five per second), even when live reload is off.
	 * @return The number of files found changed.
	 */
	RAVEN_API int Rxml_check_files(void);

	/**
	 * @brief Reads the <Widget Name="..."> declarations of a document (its
	 * root, or the children of a <Widgets> root) so that the tags they
	 * declare can be used in the documents loaded afterwards.
	 * @return The number of tags declared.
	 */
	RAVEN_API int Rxml_register_widgets(const char* xml, size_t size);
	RAVEN_API int Rxml_register_widgets_from_file(const char* path);
	RAVEN_API void Rxml_unregister_widget(const char* name);
	RAVEN_API int Rxml_is_widget_registered(const char* name);
	/** @brief The declared tags, for tools that list them. */
	RAVEN_API int Rxml_get_registered_widget_count(void);
	RAVEN_API const char* Rxml_get_registered_widget_name(int index);

	/**
	 * Tags made by code. A library or an application that builds a widget of
	 * its own (a R_WIDGET_CUSTOM with its callbacks, or an assembly of
	 * widgets) can give it a tag: documents then write <MyWidget .../> as
	 * they write <Button/>.
	 *
	 * `create` makes the widget and returns it. `set_attribute` (may be
	 * NULL) receives the attributes of the element before Raven looks at
	 * them: it returns R_TRUE for those it understands, R_FALSE to leave
	 * them to the widget (Name, Width, Slot..., and what its type knows).
	 * Rwidget_set_attribute goes through it too.
	 */
	typedef Rid (*RXmlTagCreateCallback)(const char* tag, void* user_data);
	typedef int (*RXmlTagAttributeCallback)(Rid widget, const char* name, const char* value, void* user_data);
	/**
	 * @brief Registers a tag (or replaces what it was registered with).
	 * @return R_FALSE when the name is the one of a widget of Raven, or when too many tags are registered (64).
	 */
	RAVEN_API int Rxml_register_tag(const char* tag, RXmlTagCreateCallback create, RXmlTagAttributeCallback set_attribute, void* user_data);
	RAVEN_API void Rxml_unregister_tag(const char* tag);
	RAVEN_API int Rxml_is_tag_registered(const char* tag);
	/**
	 * @brief The path of a file named in a document: as the attributes
	 * Source of Raven read it, looked for where files are looked for (see
	 * Rxml_set_base_path): the folder of the document being loaded (while it
	 * loads) and its subfolders, the base path, the current folder, PATH.
	 * When no place holds it, the path it would have next to the document
	 * (else in the base path). For the attributes of the tags made by code
	 * that name files.
	 * @return The path; valid until the next call.
	 */
	RAVEN_API const char* Rxml_resolve_path(const char* path);

	/**
	 * @brief Sets a property from text, with the names and value syntax of the
	 * XML attributes. Handy for scripting and data-driven tools.
	 * @return R_TRUE when the attribute exists for this widget and the value is valid.
	 */
	RAVEN_API int Rwidget_set_attribute(Rid widget, const char* name, const char* value);

	/* ---- documents ---- */

	/** @brief A new document with one element. */
	RAVEN_API Rid Rxml_document_create(const char* root_tag);
	/**
	 * @brief Reads a text into a document. Comments are kept.
	 * @param error When not NULL, a mistake in the text is described there
	 * and nothing is reported; when NULL it goes to the error callback.
	 * @return The document, or R_INVALID_ID.
	 */
	RAVEN_API Rid Rxml_document_parse(const char* xml, size_t size, RXmlError* error);
	/** @brief Reads a file. The paths written in it are relative to its folder. */
	RAVEN_API Rid Rxml_document_load(const char* path);
	/** @brief Releases a document and its nodes. What was built from it lives on. */
	RAVEN_API void Rxml_document_destroy(Rid document);
	RAVEN_API int Rxml_document_is_valid(Rid document);
	RAVEN_API Rid Rxml_document_get_root(Rid document);
	/**
	 * @brief Writes the document as text, one element per line, indented
	 * with tabs. Rxml_node_get_line then says where each node was written.
	 * @return The text, valid until the next call for this document or its destruction.
	 */
	RAVEN_API const char* Rxml_document_to_string(Rid document);
	RAVEN_API int Rxml_document_save(Rid document, const char* path);

	/**
	 * @brief Builds the widgets the document describes, as
	 * Rwidget_create_from_xml does. When the root is a <Window>, builds what
	 * the window would show (R_INVALID_ID when it is empty).
	 */
	RAVEN_API Rid Rxml_document_create_widget(Rid document);
	/** @brief Builds the window the document describes, as Rwindow_create_from_xml does. */
	RAVEN_API Rid Rxml_document_create_window(Rid document);

	/* ---- nodes ---- */

	RAVEN_API int Rxml_node_is_valid(Rid node);
	RAVEN_API Rid Rxml_node_get_document(Rid node);
	/** @return The tag of an element, "" for a comment. */
	RAVEN_API const char* Rxml_node_get_tag(Rid node);
	RAVEN_API void Rxml_node_set_tag(Rid node, const char* tag);
	RAVEN_API int Rxml_node_is_comment(Rid node);
	/** @brief The text inside an element (<Label>this</Label>), or the text of a comment. */
	RAVEN_API const char* Rxml_node_get_text(Rid node);
	RAVEN_API void Rxml_node_set_text(Rid node, const char* text);

	RAVEN_API Rid Rxml_node_get_parent(Rid node);
	RAVEN_API int Rxml_node_get_child_count(Rid node);
	RAVEN_API Rid Rxml_node_get_child(Rid node, int index);
	/** @return The place of a node among the children of its parent; 0 for the root. */
	RAVEN_API int Rxml_node_get_index(Rid node);

	/**
	 * @param index Where among the children; -1 for the end.
	 * @return The new node; R_INVALID_ID (R_ERROR_XML) when it would be nested
	 *         deeper than a document can be: 256 levels under the root, as
	 *         when a text is read. The same holds for the functions below.
	 */
	RAVEN_API Rid Rxml_node_add_child(Rid parent, const char* tag, int index);
	RAVEN_API Rid Rxml_node_add_comment(Rid parent, const char* text, int index);
	/**
	 * @brief Reads a piece of XML (one element and what it contains) and
	 * adds it: what a palette, a paste or a template inserts.
	 */
	RAVEN_API Rid Rxml_node_add_xml(Rid parent, const char* xml, size_t size, int index);
	/** @brief Removes a node and what it contains. The root stays. */
	RAVEN_API void Rxml_node_remove(Rid node);
	/**
	 * @brief Moves a node, with what it contains, under another parent of
	 * the same document or elsewhere under its own.
	 * @param index Where among the children of the new parent, counted once the node has left its place; -1 for the end.
	 * @return R_FALSE when it cannot be done (the root, a parent that is inside the node, or a place too deep).
	 */
	RAVEN_API int Rxml_node_move(Rid node, Rid new_parent, int index);
	/** @brief Copies a node and what it contains, right after it. Returns the copy. */
	RAVEN_API Rid Rxml_node_duplicate(Rid node);
	/** @brief The node as text. Valid until the next call for this document. */
	RAVEN_API const char* Rxml_node_to_string(Rid node);

	RAVEN_API int Rxml_node_get_attribute_count(Rid node);
	RAVEN_API const char* Rxml_node_get_attribute_name(Rid node, int index);
	RAVEN_API const char* Rxml_node_get_attribute_value(Rid node, int index);
	/** @return The value of an attribute (names are compared without case), or NULL when the node does not have it. */
	RAVEN_API const char* Rxml_node_get_attribute(Rid node, const char* name);
	/** @brief Sets an attribute, added after the others when it is new. A NULL value removes it. */
	RAVEN_API void Rxml_node_set_attribute(Rid node, const char* name, const char* value);

	/**
	 * @return The line of the node, starting at 1, in the text the document
	 * was read from or last written to; 0 for a node added since.
	 */
	RAVEN_API int Rxml_node_get_line(Rid node);
	/**
	 * @return The widget (the window, for a <Window>) the node became the
	 * last time the document was built, or R_INVALID_ID: never built,
	 * destroyed since, or not a widget (an <Item>, a comment). A tag declared
	 * with <Widget> gives the widget it stands for.
	 */
	RAVEN_API Rid Rxml_node_get_widget(Rid node);
	/**
	 * @return The node a widget was built from, or R_INVALID_ID: not built
	 * from a document, or the document is gone. The widgets inside a tag
	 * declared with <Widget> all come from the node where the tag is used.
	 */
	RAVEN_API Rid Rwidget_get_xml_node(Rid widget);

	/* ---- what can be written ---- */

	/** @brief The tags of the library, <Window> included. Those declared with <Widget> are listed by Rxml_get_registered_widget_name. */
	RAVEN_API int Rxml_get_tag_count(void);
	RAVEN_API const char* Rxml_get_tag_name(int index);
	/** @return One line about a tag, "" when there is nothing to say. */
	RAVEN_API const char* Rxml_get_tag_description(const char* tag);
	/** @return 0: no child widget; 1: a single one; 2: any number. Declared and unknown tags give 2. */
	RAVEN_API int Rxml_get_tag_children(const char* tag);
	/**
	 * @brief The attributes of a tag: its own first, then those every widget
	 * has, then those of its place in its parent. For a tag declared with
	 * <Widget>, its own are its parameters.
	 */
	RAVEN_API int Rxml_get_attribute_count(const char* tag);
	RAVEN_API int Rxml_get_attribute_info(const char* tag, int index, RXmlAttributeInfo* info);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_XML_H */

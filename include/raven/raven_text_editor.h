/**
 * Raven
 * raven_text_editor.h : multi-line text editor, and the languages it knows.
 *
 * R_WIDGET_TEXT_EDITOR edits a text of any number of lines: selection with
 * the mouse and the keyboard, clipboard, undo and redo, line numbers, word
 * wrap. It scrolls by itself. In XML:
 *
 *     <TextEditor Name="notes" Wrap="true" Placeholder="Write here"/>
 *     <TextEditor Language="Rust" LineNumbers="true" Slot.Fill="1"/>
 *
 * Given a language, the editor colors the text, proposes words while typing
 * (the words of the language and the names already written in the text),
 * shows which bracket closes which, indents after an opening bracket, closes
 * brackets and quotes, and comments lines out. C, C++, C#, Java, JavaScript,
 * TypeScript, Python, Rust, GLSL, JSON, CSS, XML and HTML are built in; any
 * of them can be changed, and others declared, with Rlanguage_register.
 *
 * This is what the editor knows from the text alone. What needs to
 * understand a whole program (the type of an expression, the errors of a
 * compiler) comes from outside, through two doors: a completion callback
 * that adds its own proposals, and markers that underline a part of the text
 * with a message.
 *
 * Positions are a line and a column, both starting at 0; a column is a byte
 * offset in the UTF-8 text of its line. Out-of-range positions are clamped.
 *
 * Keys: arrows, Home / End, Page Up / Down, with Ctrl to move by word or to
 * the ends of the text and with Shift to select; Ctrl+A, C, X, V; Ctrl+Z,
 * Ctrl+Y or Ctrl+Shift+Z; Tab and Shift+Tab indent the selected lines;
 * Ctrl+Space proposes words; Ctrl+/ comments the selected lines out.
 * A double click selects a word, a triple click a line.
 */

#ifndef RAVEN_TEXT_EDITOR_H
#define RAVEN_TEXT_EDITOR_H

#include "raven_widget.h"

/** Shorthands of Rtext_editor_set_language for two languages. */
typedef enum RSyntax{
	R_SYNTAX_NONE = 0x0340,
	R_SYNTAX_XML,
	R_SYNTAX_C
}RSyntax;

typedef enum RLanguageFlags{
	/** Tags and attributes instead of code: XML, HTML. The words and the comments of the description are not used. */
	R_LANGUAGE_MARKUP          = (1 << 0),
	/** Selectors, properties and values: CSS. */
	R_LANGUAGE_STYLE_SHEET     = (1 << 1),
	/** A block comment may hold another one (Rust). */
	R_LANGUAGE_NESTED_COMMENTS = (1 << 2),
	/** Between single quotes there is one character; any other single quote is not a text (C, Java, Rust). */
	R_LANGUAGE_CHAR_LITERALS   = (1 << 3),
	/** A line that ends with ':' indents the next one (Python). */
	R_LANGUAGE_INDENT_ON_COLON = (1 << 4),
	/** The words of the language are recognized whatever their case. */
	R_LANGUAGE_IGNORE_CASE     = (1 << 5),
	/** Markup that holds JavaScript in script elements and CSS in style elements (HTML). */
	R_LANGUAGE_EMBEDS          = (1 << 6),
	/**
	 * Formatted text written in Markdown: titles, strong and emphasized
	 * text, code, links, lists. The editor shows the text as it is written,
	 * marks included, with strong text in bold, emphasized text in italic
	 * and titles in bold: what will be formatted is seen while typing.
	 */
	R_LANGUAGE_FORMATTED_TEXT  = (1 << 7)
}RLanguageFlags;

/**
 * What the editor knows of a language. Every field but the name may be NULL.
 * Lists of words are separated by spaces.
 */
typedef struct RLanguageDesc {
	/** "Rust". Compared without case. */
	const char* name;
	/** Extensions of its files, separated by ';': "rs". */
	const char* extensions;
	/** Words of the language itself: if, while, return... */
	const char* keywords;
	/** Names of types and of what is always there: int, String, print... */
	const char* types;
	/** Values with a name: true, false, null... */
	const char* constants;
	/** What starts a comment that ends with its line: two slashes, a hash. */
	const char* line_comment;
	/** What opens and closes a comment that may span lines. */
	const char* block_comment_start;
	const char* block_comment_end;
	/** Characters that open and close a text on one line: a double quote, a single quote. */
	const char* string_quotes;
	/**
	 * Marks of the texts that span lines, separated by spaces: three double
	 * quotes, a backquote. When what closes such a text is not what opens it,
	 * write both around three dots: R"(...)" . Nothing is escaped in those.
	 */
	const char* multiline_strings;
	/** What starts a line for the preprocessor: "#". */
	const char* directive;
	/** What starts an annotation: "@" (@Override), or "#[" up to its closing bracket. */
	const char* annotation;
	/** Bitmask of RLanguageFlags. */
	Ruint flags;
	/**
	 * Markup with R_LANGUAGE_EMBEDS: the language of what is between
	 * <script> and </script> (the name of a language: "C", "Rust"...).
	 * NULL: JavaScript, as in HTML.
	 */
	const char* script_language;
} RLanguageDesc;

/** What a part of the text is, for the language of the editor (Rtext_editor_get_token). */
typedef enum RToken{
	R_TOKEN_TEXT = 0x03A0,
	R_TOKEN_COMMENT,
	R_TOKEN_STRING,
	R_TOKEN_NUMBER,
	R_TOKEN_KEYWORD,
	/** A type, or a name that is always there. */
	R_TOKEN_TYPE,
	/** A name followed by an opening parenthesis. */
	R_TOKEN_FUNCTION,
	/** The name of an element, in markup and in the selectors of a style sheet. */
	R_TOKEN_TAG,
	/** The name of an attribute (markup), of a class or of a property (style sheets), of a key (JSON). */
	R_TOKEN_ATTRIBUTE,
	R_TOKEN_PUNCTUATION,
	/** A line for the preprocessor, an annotation, a declaration of a document. */
	R_TOKEN_DIRECTIVE,
	/** Formatted text (the Markdown language): a title, strong text, emphasized text, a link. */
	R_TOKEN_HEADING,
	R_TOKEN_STRONG,
	R_TOKEN_EMPHASIS,
	R_TOKEN_LINK
}RToken;

/** What a marker says about the text it underlines. */
typedef enum RMarkerKind{
	R_MARKER_ERROR = 0x0350,
	R_MARKER_WARNING,
	R_MARKER_INFO
}RMarkerKind;


#ifdef __cplusplus
extern "C" {
#endif

	/** @param text Initial content, or NULL. */
	RAVEN_API Rid Rtext_editor_create(const char* text);

	/* ---- content ---- */

	/** @brief Replaces the whole text. The history of undo is cleared; the changed callback is not called. */
	RAVEN_API void Rtext_editor_set_text(Rid editor, const char* text);
	/** @return The whole text, lines separated by '\n'. Valid until the next call or the next edit. */
	RAVEN_API const char* Rtext_editor_get_text(Rid editor);
	/** @return The number of bytes of Rtext_editor_get_text. */
	RAVEN_API int Rtext_editor_get_length(Rid editor);

	RAVEN_API int Rtext_editor_get_line_count(Rid editor);
	/** @return One line, without its line break. Valid until the next edit. NULL when `line` is out of range. */
	RAVEN_API const char* Rtext_editor_get_line(Rid editor, int line);

	/**
	 * @brief Types a text at the cursor, replacing the selection, as the user
	 * would: it can be undone and the changed callback is called.
	 */
	RAVEN_API void Rtext_editor_insert_text(Rid editor, const char* text);

	/* ---- cursor and selection ---- */

	RAVEN_API void Rtext_editor_set_cursor(Rid editor, int line, int column);
	RAVEN_API void Rtext_editor_get_cursor(Rid editor, int* line, int* column);

	/** @brief Selects from one position to another; the cursor is on the second. */
	RAVEN_API void Rtext_editor_select(Rid editor, int from_line, int from_column, int to_line, int to_column);
	RAVEN_API void Rtext_editor_select_all(Rid editor);
	/** @brief Start and end of the selection, in the order of the text. Returns R_FALSE when nothing is selected. */
	RAVEN_API int Rtext_editor_get_selection(Rid editor, int* from_line, int* from_column, int* to_line, int* to_column);
	/** @return The selected text, "" when there is none. Valid until the next call or the next edit. */
	RAVEN_API const char* Rtext_editor_get_selected_text(Rid editor);

	/** @brief Scrolls so that a line is visible. */
	RAVEN_API void Rtext_editor_scroll_to_line(Rid editor, int line);

	/* ---- history ---- */

	RAVEN_API void Rtext_editor_undo(Rid editor);
	RAVEN_API void Rtext_editor_redo(Rid editor);
	RAVEN_API int Rtext_editor_can_undo(Rid editor);
	RAVEN_API int Rtext_editor_can_redo(Rid editor);

	/* ---- options ---- */

	/** @brief A read-only editor can still be scrolled, selected and copied. */
	RAVEN_API void Rtext_editor_set_read_only(Rid editor, int read_only);
	/** @brief Long lines continue on the next row instead of scrolling sideways. Off by default. */
	RAVEN_API void Rtext_editor_set_word_wrap(Rid editor, int enabled);
	/** @brief Shows the line numbers in a margin. Off by default. */
	RAVEN_API void Rtext_editor_set_line_numbers(Rid editor, int enabled);
	/** @brief Width of a tab, in spaces (4 by default). */
	RAVEN_API void Rtext_editor_set_tab_size(Rid editor, int spaces);
	/** @brief R_TRUE makes the Tab key insert spaces instead of a tab character. Off by default. */
	RAVEN_API void Rtext_editor_set_tab_inserts_spaces(Rid editor, int enabled);
	/** @brief The same as Rtext_editor_set_language with "XML", "C" or NULL. */
	RAVEN_API void Rtext_editor_set_syntax(Rid editor, RSyntax syntax);
	/**
	 * @param font R_INVALID_ID uses the font of the theme (or the fixed-width
	 * font, with a language).
	 * @param size 0 uses the font size of the theme.
	 */
	RAVEN_API void Rtext_editor_set_font(Rid editor, Rid font, float size);
	/** @brief Text shown, dimmed, while the editor is empty. */
	RAVEN_API void Rtext_editor_set_placeholder(Rid editor, const char* text);

	/* ---- zoom ---- */

	/**
	 * @brief Zoom of the text: a factor of its font size, 1 = 100 % (0.5 .. 4).
	 * The user zooms with Ctrl + wheel (around the pointer), Ctrl + '+',
	 * Ctrl + '-' and Ctrl + 0 (back to 100 %), keypad included. Set by the
	 * program, the top of the view stays and the zoom callback is not called.
	 * XML: Zoom="1.25".
	 */
	RAVEN_API void Rtext_editor_set_zoom(Rid editor, float zoom);
	RAVEN_API float Rtext_editor_get_zoom(Rid editor);
	/** @brief R_FALSE: Ctrl + wheel and the keys do not zoom (the wheel scrolls). On by default. XML: Zoomable="false". */
	RAVEN_API void Rtext_editor_set_zoomable(Rid editor, int zoomable);
	/** @brief Called when the user zoomed (to zoom other editors the same, or to keep it for the next time). */
	typedef void (*RTextEditorZoomCallback)(Rid editor, float zoom, void* user_data);
	RAVEN_API void Rtext_editor_set_zoom_callback(Rid editor, RTextEditorZoomCallback callback, void* user_data);

	/* ---- languages ---- */

	/**
	 * @brief Declares a language, or replaces the one that has this name: to
	 * add words to a built-in language, read it with Rlanguage_get, change
	 * what you want and register it again. The strings are copied.
	 */
	RAVEN_API int Rlanguage_register(const RLanguageDesc* desc);
	/** @brief Reads the description of a language. The strings belong to the library. Returns R_FALSE when it is unknown. */
	RAVEN_API int Rlanguage_get(const char* name, RLanguageDesc* desc);
	RAVEN_API int Rlanguage_get_count(void);
	RAVEN_API const char* Rlanguage_get_name(int index);
	/** @return The name of the language of a file, from its extension, or NULL. */
	RAVEN_API const char* Rlanguage_for_file(const char* path);

	/**
	 * @brief Gives the editor a language, by name ("C++", "Python"...), by
	 * another usual name ("cpp", "js") or by the extension of its files
	 * ("rs"). NULL or "" removes it. An editor with a language uses the
	 * fixed-width font unless it was given a font.
	 */
	RAVEN_API void Rtext_editor_set_language(Rid editor, const char* name);
	/** @return The name of the language of the editor, or "". */
	RAVEN_API const char* Rtext_editor_get_language(Rid editor);

	/**
	 * @brief R_TRUE makes the editor an editor of Markdown (the "Markdown"
	 * language; Markdown="true" in XML): the text stays as it is written,
	 * and what it formats shows while typing, strong text in bold,
	 * emphasized text in italic, titles in bold, code, links and marks in
	 * their colors. A label with Rlabel_set_markdown shows the same text
	 * formatted. R_FALSE removes the language.
	 */
	RAVEN_API void Rtext_editor_set_markdown(Rid editor, int enabled);
	RAVEN_API int Rtext_editor_get_markdown(Rid editor);

	/**
	 * @brief What the language of the editor makes of the character at a
	 * position: the colors come from it. R_TOKEN_TEXT without a language.
	 */
	RAVEN_API RToken Rtext_editor_get_token(Rid editor, int line, int column);

	/* ---- help while typing ---- */

	/** @brief R_FALSE stops the editor from proposing words by itself; Ctrl+Space still does. On by default. */
	RAVEN_API void Rtext_editor_set_completion(Rid editor, int enabled);
	/** @brief Proposes words for what is before the cursor, as Ctrl+Space does. */
	RAVEN_API void Rtext_editor_show_completions(Rid editor);
	RAVEN_API int Rtext_editor_is_completing(Rid editor);
	/**
	 * @brief Called each time the editor gathers its proposals, with the
	 * start of the word being typed: add your own with
	 * Rtext_editor_add_completion (the fields of a structure, the answers of
	 * a language server...). They are listed before the others.
	 */
	typedef void (*RCompletionCallback)(Rid editor, const char* prefix, void* user_data);
	RAVEN_API void Rtext_editor_set_completion_callback(Rid editor, RCompletionCallback callback, void* user_data);
	/**
	 * @brief Adds a proposal. Only from a completion callback. Proposals that
	 * do not start with the prefix are left out; the others keep the order
	 * they were added in.
	 * @param detail A few words shown beside it ("function", "int"), or NULL.
	 */
	RAVEN_API void Rtext_editor_add_completion(Rid editor, const char* text, const char* detail);
	/**
	 * @brief R_FALSE stops the editor from closing brackets and quotes and
	 * from indenting after them. On by default; it only acts with a language.
	 */
	RAVEN_API void Rtext_editor_set_auto_pairs(Rid editor, int enabled);

	/* ---- markers ---- */

	/**
	 * @brief Underlines a part of a line and attaches a message to it, shown
	 * when the pointer rests there: an error of a compiler, a warning.
	 * Markers follow their line when text is added or removed above it, and
	 * go away when their own line is edited.
	 * @param to_column -1 for the end of the line.
	 */
	RAVEN_API void Rtext_editor_add_marker(Rid editor, int line, int from_column, int to_column, RMarkerKind kind, const char* message);
	RAVEN_API void Rtext_editor_clear_markers(Rid editor);
	RAVEN_API int Rtext_editor_get_marker_count(Rid editor);

	/* ---- colors ---- */

	/**
	 * @brief Color of a kind of token in the theme in use, instead of the
	 * one Raven picks for dark and for light themes. Theme files set them
	 * with <Syntax Keyword="#FF7AB2" String="..."/>.
	 */
	RAVEN_API void Rtheme_set_token_color(RToken token, float r, float g, float b, float a);
	RAVEN_API void Rtheme_get_token_color(RToken token, float* r, float* g, float* b, float* a);
	/** @brief Back to the colors Raven picks. */
	RAVEN_API void Rtheme_clear_token_colors(void);

	/* ---- callback ---- */

	/**
	 * @brief Called after each change made by the user (typing, pasting,
	 * undoing...) or by Rtext_editor_insert_text. The action of the widget
	 * (OnChanged in XML) fires at the same time.
	 */
	typedef void (*RTextEditorChangedCallback)(Rid editor, void* user_data);
	RAVEN_API void Rtext_editor_set_changed_callback(Rid editor, RTextEditorChangedCallback callback, void* user_data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_TEXT_EDITOR_H */

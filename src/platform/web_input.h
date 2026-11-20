/**
 * Raven - platform/web_input.h
 * The bookkeeping of the text field of the browser platform
 * (platform/emscripten), apart from the browser so that it can be tested
 * anywhere:
 *
 *   - UTF-16 offsets (what the DOM counts in) and UTF-8 bytes (what Raven
 *     counts in);
 *   - the part of the text of the focused TextInput the hidden textarea
 *     holds around the selection, for the predictive text, autocorrection
 *     and composition of on-screen keyboards;
 *   - the attributes of the textarea or password input for each kind of
 *     field (inputmode, enterkeyhint, autocomplete, spellcheck...).
 */

#ifndef RVN_WEB_INPUT_H
#define RVN_WEB_INPUT_H

#include "../core/base.h"

/** UTF-16 code units in the first `bytes` bytes of UTF-8 `text` (a character out of the BMP counts two). */
int rvn_utf16_count(const char* text, int bytes);

/**
 * The byte offset of UTF-16 unit `units` of UTF-8 `text` (`length` bytes),
 * clamped to [0, length]. A unit in the middle of a surrogate pair goes back
 * to the start of its character.
 */
int rvn_utf16_to_utf8(const char* text, int length, int units);

/** What the textarea holds of the text of a field. */
typedef struct rvn_web_surrounding_t {
	/** The part of the text: bytes [offset, offset + length). */
	int offset, length;
	/** The selection, in UTF-16 units from the start of the part. */
	int start16, end16;
} rvn_web_surrounding_t;

/**
 * The part of `text` around the selection [start, end) (bytes, either order)
 * that the textarea holds: the whole selection, and up to `max_units` UTF-16
 * units before and after it, cut on character boundaries.
 */
void rvn_web_surrounding(const char* text, int start, int end, int max_units, rvn_web_surrounding_t* out);

/** How the page takes the text of the focused field. */
typedef enum rvn_web_field_mode_t {
	/** The focus takes no text: the canvas has the keyboard. */
	RVN_WEB_FIELD_NONE,
	/** A TextInput: the textarea holds the text around the caret, edits are replaced ranges. */
	RVN_WEB_FIELD_MIRROR,
	/** A password: an <input type="password">, always empty; only what is typed goes through. */
	RVN_WEB_FIELD_SECURE,
	/** Text without the text services of the core (TextEditor, SpinBox): keys and typed text. */
	RVN_WEB_FIELD_KEYS
} rvn_web_field_mode_t;

/** The kind of widget that has the focus, for rvn_web_field_hints. */
typedef enum rvn_web_field_kind_t {
	RVN_WEB_KIND_TEXT_INPUT,
	RVN_WEB_KIND_TEXT_EDITOR,
	RVN_WEB_KIND_SPIN_BOX,
	RVN_WEB_KIND_OTHER
} rvn_web_field_kind_t;

/** The attributes of the element that takes the text (static strings). */
typedef struct rvn_web_field_hints_t {
	rvn_web_field_mode_t mode;
	const char* inputmode;
	const char* enterkeyhint;
	const char* autocomplete;
	const char* autocapitalize;
	/** spellcheck, and autocorrect (Safari): suggestions and corrections. */
	int spellcheck;
	int read_only;
} rvn_web_field_hints_t;

/**
 * The mode and attributes for a field of `kind`. `info` is what
 * Rwindow_get_text_input_info answered (NULL when it answered R_FALSE: a
 * field the core does not describe). `wants_text` R_FALSE: RVN_WEB_FIELD_NONE.
 */
void rvn_web_field_hints(int wants_text, rvn_web_field_kind_t kind, const RTextInputInfo* info, rvn_web_field_hints_t* out);

#endif /* RVN_WEB_INPUT_H */

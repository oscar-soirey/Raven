/**
 * Raven
 * raven_controls.h : interactive widgets and their settings.
 *
 * Every control is created with Rwidget_create(type). Its colors and sizes
 * come from the theme and can be overridden per widget (Rwidget_set_color,
 * Rwidget_set_metric). Its main event can be received either through the
 * typed callback declared here, or by name through an action
 * (Rwidget_set_action + Raction_bind).
 */

#ifndef RAVEN_CONTROLS_H
#define RAVEN_CONTROLS_H

#include "raven_widget.h"
#include "raven_locale.h"


typedef enum RTextAlign{
	R_TEXT_ALIGN_LEFT = 0x0280,
	R_TEXT_ALIGN_CENTER,
	R_TEXT_ALIGN_RIGHT
}RTextAlign;

typedef enum RToastKind{
	/** A neutral message (the accent of the theme). */
	R_TOAST_INFO = 0x04E0,
	R_TOAST_SUCCESS,
	R_TOAST_WARNING,
	R_TOAST_ERROR
}RToastKind;

/** What a Toast does with a message that comes while another one is shown. */
typedef enum RToastQueue{
	/** The new message takes the place of the one shown (the default). */
	R_TOAST_REPLACE = 0x0500,
	/** The messages are shown one after the other, each for its own time. */
	R_TOAST_SEQUENCE,
	/** Up to Rtoast_set_max_visible messages are shown at once, one above the other; the others wait. */
	R_TOAST_STACK
}RToastQueue;

typedef enum RButtonStyle{
	/** Filled with R_COLOR_CONTROL. */
	R_BUTTON_DEFAULT = 0x0290,
	/** Filled with R_COLOR_ACCENT. */
	R_BUTTON_PRIMARY,
	/** No background until hovered (toolbars). */
	R_BUTTON_FLAT
}RButtonStyle;

typedef enum RCheckboxStyle{
	R_CHECKBOX_CHECK = 0x02A0,
	/** Round mark. Checking one unchecks the other radio boxes of the same parent. */
	R_CHECKBOX_RADIO,
	/** On / off switch. */
	R_CHECKBOX_SWITCH
}RCheckboxStyle;

typedef enum RStretch{
	/** Stretches the image to the widget, ignoring its aspect ratio. */
	R_STRETCH_FILL = 0x02B0,
	/** Largest size that fits entirely, keeping the aspect ratio. */
	R_STRETCH_FIT,
	/** Smallest size that covers the widget, keeping the aspect ratio (cropped). */
	R_STRETCH_COVER,
	/** Original size, centered. */
	R_STRETCH_NONE
}RStretch;

/** The axes of a Plot. */
typedef enum RPlotAxis{
	/** The horizontal axis under the data. */
	R_PLOT_X = 0x0520,
	/** The vertical axis on the left. */
	R_PLOT_Y,
	/** A second horizontal axis, over the data: shown when a series or a drag line uses it. */
	R_PLOT_X2,
	/** A second vertical axis, on the right: shown when a series or a drag line uses it. */
	R_PLOT_Y2
}RPlotAxis;

/** How a PdfView sizes its pages. */
typedef enum RPdfFit{
	/** The widest page fills the width of the view (default). */
	R_PDF_FIT_WIDTH = 0x05C0,
	/** The largest page fits whole in the view. */
	R_PDF_FIT_PAGE,
	/** The zoom of Rpdf_view_set_zoom; zooming with the wheel or the keys switches to it. */
	R_PDF_FIT_NONE
}RPdfFit;

/** What a PdfView holds. */
typedef enum RPdfStatus{
	/** No document. */
	R_PDF_EMPTY = 0x05D0,
	/** A document is shown. */
	R_PDF_LOADED,
	/** The document needs a password, or the one given is wrong: load it again with one. */
	R_PDF_NEEDS_PASSWORD,
	/** The file could not be read, or is not a PDF document. */
	R_PDF_ERROR,
	/** The PDFium library was not found (see Rpdf_is_available). */
	R_PDF_UNAVAILABLE
}RPdfStatus;

/** The fonts of a PDF document (Rpdf_document_set_font): those of Raven, then the ones added (2, 3...). */
typedef enum RPdfFont{
	/** Roboto, the font of the interface of Raven. */
	R_PDF_FONT_SANS = 0,
	/** Roboto Mono. */
	R_PDF_FONT_MONO = 1
}RPdfFont;

/** How a shape of a PDF document is painted: a bitmask. */
typedef enum RPdfPaint{
	/** Filled with the fill color. */
	R_PDF_FILL   = (1 << 0),
	/** Outlined with the stroke color and the line width. */
	R_PDF_STROKE = (1 << 1)
}RPdfPaint;

/** A bitmask of styles of the text of a PDF document. */
typedef enum RPdfTextStyle{
	R_PDF_TEXT_BOLD      = (1 << 0),
	R_PDF_TEXT_ITALIC    = (1 << 1),
	R_PDF_TEXT_UNDERLINE = (1 << 2)
}RPdfTextStyle;

typedef enum RPdfPrintFlags{
	/** No dialog: printed at once on the printer named, else the default one. */
	R_PDF_PRINT_NO_DIALOG = (1 << 0)
}RPdfPrintFlags;

/** How a PDF document is printed. Every field may be left to zero. */
typedef struct RPdfPrintDesc {
	/** Window the print dialog belongs to (0 or R_INVALID_ID: none). */
	Rid window;
	/** Name of the print job, shown in the queue of the printer. NULL: the title of the document or the name of the file. */
	const char* title;
	/** Name of the printer, as the system knows it. NULL: the one chosen in the dialog, else the default printer. */
	const char* printer;
	/** Copies to print (0: one); the dialog may change it. */
	int copies;
	/** Bitmask of RPdfPrintFlags. */
	Ruint flags;
	/**
	 * Path of a file the job is written to instead of a printer. macOS: a
	 * PDF made by the print system; Windows: what the driver of the printer
	 * makes; elsewhere: the document itself.
	 */
	const char* output;
	/** Password of a document protected by one. */
	const char* password;
} RPdfPrintDesc;

/** Where a tile of a MapView stands (Rmap_view_get_tile_state). */
typedef enum RMapTileState{
	/** Not in the view's cache: never asked for, no longer wanted, or evicted. */
	R_MAP_TILE_NONE = 0x0600,
	/** Asked for (the tile callback was called, or the file is being read): waiting for its pixels. */
	R_MAP_TILE_PENDING,
	/** Its pixels are in the cache. */
	R_MAP_TILE_LOADED,
	/** The application said it has none (Rmap_view_set_tile_failed), or its file could not be read. */
	R_MAP_TILE_FAILED
}RMapTileState;

/** How an axis of a Plot spreads its values. */
typedef enum RPlotScale{
	R_PLOT_SCALE_LINEAR = 0x0530,
	/** Powers of ten at even distances; only values above zero are shown. */
	R_PLOT_SCALE_LOG,
	/**
	 * Symmetric logarithm: linear between -t and t (Rplot_set_axis_linear_threshold,
	 * 1 by default), logarithmic beyond on both sides: shows zero and the
	 * negative values a logarithm cannot.
	 */
	R_PLOT_SCALE_SYMLOG
}RPlotScale;

/** How the line of a series of a Plot is drawn. */
typedef enum RPlotLine{
	R_PLOT_LINE_SOLID = 0x0540,
	R_PLOT_LINE_DASHED,
	R_PLOT_LINE_DOTTED,
	R_PLOT_LINE_DASH_DOT,
	/** No line: only the markers (a scatter plot). */
	R_PLOT_LINE_NONE
}RPlotLine;

/** The mark drawn on each point of a series of a Plot. */
typedef enum RPlotMarker{
	R_PLOT_MARKER_NONE = 0x0550,
	R_PLOT_MARKER_CIRCLE,
	R_PLOT_MARKER_SQUARE,
	R_PLOT_MARKER_DIAMOND,
	R_PLOT_MARKER_TRIANGLE_UP,
	R_PLOT_MARKER_TRIANGLE_DOWN,
	R_PLOT_MARKER_CROSS,
	R_PLOT_MARKER_PLUS,
	R_PLOT_MARKER_ASTERISK
}RPlotMarker;

/** Where the legend of a Plot is. */
typedef enum RPlotLegend{
	R_PLOT_LEGEND_HIDDEN = 0x0560,
	R_PLOT_LEGEND_TOP_LEFT,
	R_PLOT_LEGEND_TOP,
	R_PLOT_LEGEND_TOP_RIGHT,
	R_PLOT_LEGEND_RIGHT,
	R_PLOT_LEGEND_BOTTOM_RIGHT,
	R_PLOT_LEGEND_BOTTOM,
	R_PLOT_LEGEND_BOTTOM_LEFT,
	R_PLOT_LEGEND_LEFT,
	/** To the right of the data, outside it. */
	R_PLOT_LEGEND_OUTSIDE
}RPlotLegend;

/** What a Chart draws. */
typedef enum RChartKind{
	/** Slices of a whole, as parts of a disc (the default). */
	R_CHART_PIE = 0x0590,
	/** Slices of a whole, as parts of a ring; its middle shows the total, or the slice under the pointer. */
	R_CHART_DONUT,
	/** One value within a range, on an arc; bands color parts of the range. */
	R_CHART_GAUGE,
	/** A grid of values, each cell colored from the low color to the high one. */
	R_CHART_HEATMAP,
	/** A small line of values without axes, to sit in a tile or a cell of a table. */
	R_CHART_SPARKLINE
}RChartKind;

/** What happens in a SystemWebView (Rsystem_web_view_set_event_callback). */
typedef enum RSystemWebViewEvent{
	/** The view shows another address (Rsystem_web_view_get_url). */
	R_SYSTEM_WEB_VIEW_NAVIGATED = 0x0B90,
	/** The title of the page changed (Rsystem_web_view_get_title). */
	R_SYSTEM_WEB_VIEW_TITLE,
	/** A page starts loading. */
	R_SYSTEM_WEB_VIEW_LOADING,
	/** The page and what it refers to are loaded. */
	R_SYSTEM_WEB_VIEW_LOADED,
	/** The page could not be loaded, or the engine showing it stopped (Rsystem_web_view_get_error). */
	R_SYSTEM_WEB_VIEW_FAILED
}RSystemWebViewEvent;


#ifdef __cplusplus
extern "C" {
#endif

	/* ============================================================================
	 *  LABEL
	 * ============================================================================ */

	/** @brief Shortcut for Rwidget_create(R_WIDGET_LABEL) + Rlabel_set_text. */
	RAVEN_API Rid Rlabel_create(const char* text);
	/** @brief Sets the text. '\n' starts a new line. */
	RAVEN_API void Rlabel_set_text(Rid label, const char* text);
	RAVEN_API const char* Rlabel_get_text(Rid label);
	/** @brief R_INVALID_ID uses the theme font. */
	RAVEN_API void Rlabel_set_font(Rid label, Rid font);
	/** @brief Font size in logical units. 0 uses R_METRIC_FONT_SIZE. */
	RAVEN_API void Rlabel_set_text_size(Rid label, float size);
	RAVEN_API void Rlabel_set_text_align(Rid label, RTextAlign align);
	/** @brief R_TRUE wraps the text on the width given by the parent. */
	RAVEN_API void Rlabel_set_wrap(Rid label, int wrap);
	/** @brief Text color (R_COLOR_TEXT of this widget). */
	RAVEN_API void Rlabel_set_color(Rid label, float r, float g, float b, float a);

	/**
	 * @brief R_TRUE reads the text as Markdown and shows it formatted
	 * (Markdown="true" in XML):
	 *
	 *   # titles, **strong**, *emphasis*, `code`, ~~struck~~, [links](url),
	 *   lists and lists in lists, - [x] task lists, > quotes, --- rules,
	 *   | tables |, images alone on their line, and ``` blocks of code with
	 *   the colors of their language (```c, ```python...: the languages of
	 *   the text editor, raven_text_editor.h).
	 *
	 * The text wraps on the width of the label unless Rlabel_set_wrap said
	 * otherwise; put the label in a scroll box for a long document. Strong
	 * and emphasized text use the bold and italic of the font
	 * (Rfont_set_variant), which Raven derives from the font itself when it
	 * was given none. The colors come from the theme: R_COLOR_LINK,
	 * R_COLOR_CODE, R_COLOR_SEPARATOR, R_COLOR_ACCENT.
	 */
	RAVEN_API void Rlabel_set_markdown(Rid label, int enabled);
	RAVEN_API int Rlabel_get_markdown(Rid label);
	/**
	 * @brief Called when the user clicks a link of a Markdown label.
	 *
	 * Without a callback, a link written [Save](action:file.save) fires the
	 * action "file.save" (Raction_bind), and http, https and mailto links
	 * open in the browser or the mail program of the user (Ropen_url).
	 */
	typedef void (*RLabelLinkCallback)(Rid label, const char* url, void* user_data);
	RAVEN_API void Rlabel_set_link_callback(Rid label, RLabelLinkCallback callback, void* user_data);
	/** @brief The text of a Markdown label without its marks, as a reader sees it. Valid until the next call. */
	RAVEN_API const char* Rlabel_get_plain_text(Rid label);

	/**
	 * @brief R_TRUE lets the user select the text of the label with the
	 * mouse, as on a web page (Selectable="true" in XML): drag to select, a
	 * double click selects a word, a triple click a paragraph, Shift + click
	 * extends the selection; then Ctrl+C (Cmd+C) copies it and Ctrl+A selects
	 * everything. A click elsewhere takes the selection away. Plain and
	 * Markdown labels alike: a copy of a Markdown label is its text without
	 * the marks, and its links are still clicked, not selected. The label
	 * takes the mouse (what holds it no longer receives the clicks on it) and
	 * the keyboard after a click; Tab passes it by. Default R_FALSE.
	 */
	RAVEN_API void Rlabel_set_selectable(Rid label, int selectable);
	RAVEN_API int Rlabel_get_selectable(Rid label);
	/** @brief The text selected, as Ctrl+C copies it ("" when nothing is). Valid until the next call. */
	RAVEN_API const char* Rlabel_get_selected_text(Rid label);
	/** @brief Selects the whole text of a selectable label. */
	RAVEN_API void Rlabel_select_all(Rid label);
	RAVEN_API void Rlabel_clear_selection(Rid label);


	/* ============================================================================
	 *  BUTTON
	 * ============================================================================ */

	/** @brief Shortcut for Rwidget_create(R_WIDGET_BUTTON) + Rbutton_set_text. */
	RAVEN_API Rid Rbutton_create(const char* text);
	/**
	 * @brief Sets the content of the button to a label. A button can instead
	 * hold any single child widget (Rwidget_add_child), e.g. an HBox with an icon.
	 */
	RAVEN_API void Rbutton_set_text(Rid button, const char* text);
	RAVEN_API void Rbutton_set_style(Rid button, RButtonStyle style);
	typedef void (*RButtonClickedCallback)(Rid button, void* user_data);
	RAVEN_API void Rbutton_set_clicked_callback(Rid button, RButtonClickedCallback callback, void* user_data);


	/* ============================================================================
	 *  CHECKBOX
	 * ============================================================================ */

	RAVEN_API Rid Rcheckbox_create(const char* text);
	RAVEN_API void Rcheckbox_set_text(Rid checkbox, const char* text);
	RAVEN_API const char* Rcheckbox_get_text(Rid checkbox);
	RAVEN_API void Rcheckbox_set_checked(Rid checkbox, int checked);
	RAVEN_API int Rcheckbox_is_checked(Rid checkbox);
	RAVEN_API void Rcheckbox_set_style(Rid checkbox, RCheckboxStyle style);
	typedef void (*RCheckboxChangedCallback)(Rid checkbox, int checked, void* user_data);
	RAVEN_API void Rcheckbox_set_changed_callback(Rid checkbox, RCheckboxChangedCallback callback, void* user_data);


	/* ============================================================================
	 *  SLIDER
	 * ============================================================================ */

	RAVEN_API void Rslider_set_range(Rid slider, float minimum, float maximum);
	RAVEN_API void Rslider_set_value(Rid slider, float value);
	RAVEN_API float Rslider_get_value(Rid slider);
	/** @brief The range given with Rslider_set_range and the step (NULL for what is not wanted). */
	RAVEN_API void Rslider_get_range(Rid slider, float* minimum, float* maximum, float* step);
	/** @brief Snaps the value to multiples of `step`. 0 = continuous (default). */
	RAVEN_API void Rslider_set_step(Rid slider, float step);
	RAVEN_API void Rslider_set_orientation(Rid slider, ROrientation orientation);
	typedef void (*RSliderChangedCallback)(Rid slider, float value, void* user_data);
	RAVEN_API void Rslider_set_changed_callback(Rid slider, RSliderChangedCallback callback, void* user_data);


	/* ============================================================================
	 *  PROGRESS BAR
	 * ============================================================================ */

	/** @brief Progress between 0 and 1. */
	RAVEN_API void Rprogress_bar_set_value(Rid progress_bar, float value);
	RAVEN_API float Rprogress_bar_get_value(Rid progress_bar);


	/* ============================================================================
	 *  IMAGE
	 * ============================================================================ */

	RAVEN_API void Rimage_set_texture(Rid image, Rid texture);
	/**
	 * @brief Color the image is multiplied by (white by default). An SVG in
	 * currentColor (Ricon_get, Rtexture_create_from_svg) is drawn in the color of
	 * the text around it until a tint is given; then in the tint. XML: Tint.
	 */
	RAVEN_API void Rimage_set_tint_color(Rid image, float r, float g, float b, float a);
	/** @brief R_TRUE: the image is tinted with the text color around it (white icons that follow the theme). XML: TintText. */
	RAVEN_API void Rimage_set_tint_text(Rid image, int enabled);
	RAVEN_API void Rimage_set_stretch(Rid image, RStretch stretch);
	/** @brief Part of the texture to show, in UV coordinates. Default: 0, 0, 1, 1. */
	RAVEN_API void Rimage_set_region(Rid image, float min_u, float min_v, float max_u, float max_v);
	RAVEN_API void Rimage_set_corner_radius(Rid image, float radius);

	/*
	 * Animated images. An Image whose texture is an animated GIF or APNG
	 * (Rtexture_create_from_file, Source="..." in XML...) plays it: each Image
	 * has its own playback, driven by the clock of the animations
	 * (Rset_animation_speed, Rset_animation_time_step, Rtest_set_deterministic).
	 * Frames are only produced while the Image is painted: hidden, scrolled
	 * away or in a window that is not shown, it asks for no frame and goes on
	 * from where it was when it is shown again. When the animations are off
	 * (Rset_animations_enabled, reduced motion) it stays on its frame.
	 * Frames of 10 ms or less last 100 ms, as in browsers. Other widgets that
	 * draw such a texture show its first frame.
	 */

	/** @brief R_TRUE when the texture of the image is an animated GIF or APNG (two frames or more). */
	RAVEN_API int Rimage_is_animated(Rid image);
	/** @return The frames of its animation; 1 for a still image, 0 without a texture. */
	RAVEN_API int Rimage_get_frame_count(Rid image);
	/** @return The seconds one play of its animation lasts (0 for a still image). */
	RAVEN_API double Rimage_get_duration(Rid image);
	/** @return The seconds a frame of its animation is shown (0 for a frame that does not exist). */
	RAVEN_API double Rimage_get_frame_delay(Rid image, int frame);
	/**
	 * @brief Plays the animation from the frame it is on (from the first
	 * again when it had finished). An Image plays as soon as it is given an
	 * animated texture, unless XML says Animate="false".
	 */
	RAVEN_API void Rimage_play(Rid image);
	/** @brief Stops the animation on the frame it is on. */
	RAVEN_API void Rimage_pause(Rid image);
	/** @brief Stops the animation and shows its first frame. */
	RAVEN_API void Rimage_stop(Rid image);
	RAVEN_API int Rimage_is_playing(Rid image);
	/** @brief Shows a frame (from 0), playing on from it if it plays. */
	RAVEN_API void Rimage_set_frame(Rid image, int frame);
	RAVEN_API int Rimage_get_frame(Rid image);
	/**
	 * @brief How many times the animation plays before it stops on its last
	 * frame: 0 forever, -1 what the file says (the default). XML: Loop
	 * (a number; also Forever, Once).
	 */
	RAVEN_API void Rimage_set_loop_count(Rid image, int count);
	/** @return The plays of the animation (0: forever), the file's unless it was set. */
	RAVEN_API int Rimage_get_loop_count(Rid image);
	/** @brief Called when the animation stops after its last play (at the next frame), before the action of the image. */
	typedef void (*RImageFinishedCallback)(Rid image, void* user_data);
	RAVEN_API void Rimage_set_finished_callback(Rid image, RImageFinishedCallback callback, void* user_data);


	/* ============================================================================
	 *  TEXT INPUT (single line)
	 * ============================================================================ */

	RAVEN_API void Rtext_input_set_text(Rid text_input, const char* text);
	RAVEN_API const char* Rtext_input_get_text(Rid text_input);
	/** @brief Dimmed text shown while the input is empty. */
	RAVEN_API void Rtext_input_set_placeholder(Rid text_input, const char* text);
	RAVEN_API void Rtext_input_set_password(Rid text_input, int password);
	RAVEN_API int Rtext_input_is_password(Rid text_input);
	RAVEN_API int Rtext_input_is_read_only(Rid text_input);
	RAVEN_API void Rtext_input_set_read_only(Rid text_input, int read_only);
	/** @brief Maximum number of characters. 0 = no limit (default). */
	RAVEN_API void Rtext_input_set_max_length(Rid text_input, int max_length);
	RAVEN_API void Rtext_input_select_all(Rid text_input);
	/** @brief What it holds: picks the on-screen keyboard on mobile. Default: R_TEXT_INPUT_TYPE_TEXT. */
	RAVEN_API void Rtext_input_set_input_type(Rid text_input, RTextInputType type);
	RAVEN_API RTextInputType Rtext_input_get_input_type(Rid text_input);
	/** @brief The return key of the on-screen keyboard (Done, Next, Search...). Default: R_RETURN_KEY_DEFAULT. */
	RAVEN_API void Rtext_input_set_return_key(Rid text_input, RReturnKey key);
	RAVEN_API RReturnKey Rtext_input_get_return_key(Rid text_input);
	typedef void (*RTextChangedCallback)(Rid text_input, const char* text, void* user_data);
	/** @brief Called on every edit. */
	RAVEN_API void Rtext_input_set_changed_callback(Rid text_input, RTextChangedCallback callback, void* user_data);
	/** @brief Called when Enter is pressed. This also fires the action of the widget. */
	RAVEN_API void Rtext_input_set_committed_callback(Rid text_input, RTextChangedCallback callback, void* user_data);
	/**
	 * @brief Underlines the misspelled words with a wavy line (R_COLOR_SPELLING);
	 * a right click on one offers corrections (see SPELL CHECKING). Off by
	 * default. A password is never checked.
	 */
	RAVEN_API void Rtext_input_set_spell_check(Rid text_input, int enabled);
	RAVEN_API int Rtext_input_get_spell_check(Rid text_input);
	/** @brief Language of the spell checking ("en_US", "fr"...). NULL: the default (Rspell_set_language). */
	RAVEN_API void Rtext_input_set_spell_language(Rid text_input, const char* language);


	/* ============================================================================
	 *  SPELL CHECKING
	 *
	 * The checker of the system finds the misspelled words: NSSpellChecker on
	 * macOS, the spell checker of Windows (8 and later), Enchant 2 or else
	 * Hunspell on Linux and the BSDs (loaded at run time, with the
	 * dictionaries installed). Raven has none on iOS, Android and the web:
	 * Rspell_is_supported is then R_FALSE, nothing is ever underlined and the
	 * other functions find nothing.
	 *
	 * A TextInput or a TextEditor with spell checking on (SpellCheck="true")
	 * underlines the misspelled words, but not the one being typed at the
	 * caret. A right click on one opens a menu: up to five corrections,
	 * "Ignore Spelling" and "Add to Dictionary" (localization keys
	 * Raven.SpellIgnore, Raven.SpellLearn, Raven.SpellNoGuesses).
	 *
	 * Languages are codes such as "en", "en_US" or "fr-FR"; a code the checker
	 * does not have exactly falls back to one of the same language ("fr" for
	 * "fr_CA"). The language used when none is given is the one of
	 * Rspell_set_language, else that of the locale of the application
	 * (Rlocale_get_language), else that of the system.
	 * Byte offsets are offsets in the UTF-8 text.
	 * ============================================================================ */

	/** @brief R_TRUE when the system has a spell checker that Raven can use. */
	RAVEN_API int Rspell_is_supported(void);
	/** @brief The languages the checker has dictionaries for. */
	RAVEN_API int Rspell_get_language_count(void);
	RAVEN_API const char* Rspell_get_language_at(int index);
	/**
	 * @brief The language used when a widget or a call gives none. NULL
	 * follows the locale again.
	 * @return R_FALSE when the checker has no dictionary for it (the default
	 * of the system is then used).
	 */
	RAVEN_API int Rspell_set_language(const char* language);
	/** @brief The code of the language used by default, as the checker names it; "" when there is no checker. */
	RAVEN_API const char* Rspell_get_language(void);
	/**
	 * @brief Checks a text. `language` NULL: the default.
	 * @return The number of misspelled words, read with Rspell_get_misspelling.
	 */
	RAVEN_API int Rspell_check(const char* text, const char* language);
	/** @brief A misspelled word found by the last Rspell_check: bytes [start, end) of its text. */
	RAVEN_API int Rspell_get_misspelling(int index, int* start, int* end);
	/** @return The number of corrections for a word (best first), read with Rspell_get_suggestion. */
	RAVEN_API int Rspell_suggest(const char* word, const char* language);
	/** @brief A correction found by the last Rspell_suggest; valid until the next call. */
	RAVEN_API const char* Rspell_get_suggestion(int index);
	/** @brief Adds a word to the dictionary of the user, which the system keeps (with Hunspell alone: for the session). */
	RAVEN_API void Rspell_learn(const char* word, const char* language);
	/** @brief The word is no longer a misspelling, in every language, until the application quits. */
	RAVEN_API void Rspell_ignore(const char* word);


	/* ============================================================================
	 *  SPIN BOX (numeric input: drag to scrub, click to type, arrows / wheel / step buttons to step)
	 * ============================================================================ */

	RAVEN_API void Rspin_box_set_range(Rid spin_box, double minimum, double maximum);
	RAVEN_API void Rspin_box_set_value(Rid spin_box, double value);
	RAVEN_API double Rspin_box_get_value(Rid spin_box);
	/** @brief The range and the step (NULL for what is not wanted). */
	RAVEN_API void Rspin_box_get_range(Rid spin_box, double* minimum, double* maximum, double* step);
	RAVEN_API void Rspin_box_set_step(Rid spin_box, double step);
	/** @brief Digits shown after the decimal point. Default: 2. */
	RAVEN_API void Rspin_box_set_decimals(Rid spin_box, int decimals);
	/**
	 * @brief Step buttons in the looks of Raven too: a down button at the left
	 * end and an up button at the right end, the value between them (XML
	 * `Buttons="True"`). A click steps once; holding a button repeats its step.
	 * The platform looks (Windows, GNOME, macOS) always draw their own buttons.
	 * Default: R_FALSE (drag to scrub, arrows / wheel to step).
	 */
	RAVEN_API void Rspin_box_set_buttons(Rid spin_box, int buttons);
	RAVEN_API int Rspin_box_get_buttons(Rid spin_box);
	typedef void (*RSpinBoxChangedCallback)(Rid spin_box, double value, void* user_data);
	RAVEN_API void Rspin_box_set_changed_callback(Rid spin_box, RSpinBoxChangedCallback callback, void* user_data);


	/* ============================================================================
	 *  COMBO BOX
	 * ============================================================================ */

	/** @return The index of the new item. */
	RAVEN_API int Rcombo_box_add_item(Rid combo_box, const char* text);
	RAVEN_API void Rcombo_box_clear_items(Rid combo_box);
	RAVEN_API int Rcombo_box_get_item_count(Rid combo_box);
	RAVEN_API const char* Rcombo_box_get_item_text(Rid combo_box, int index);
	/** @brief Selects an item. -1 selects nothing. */
	RAVEN_API void Rcombo_box_set_selected(Rid combo_box, int index);
	RAVEN_API int Rcombo_box_get_selected(Rid combo_box);
	typedef void (*RComboBoxChangedCallback)(Rid combo_box, int index, void* user_data);
	RAVEN_API void Rcombo_box_set_changed_callback(Rid combo_box, RComboBoxChangedCallback callback, void* user_data);
	/**
	 * @brief R_TRUE: the combo box is a text field with its list: the user
	 * may type a text that is not one of the items (Editable="true"). The text
	 * is Rcombo_box_get_text; choosing an item puts its text there. A typed
	 * text equal to an item selects it, any other selects nothing (-1).
	 */
	RAVEN_API void Rcombo_box_set_editable(Rid combo_box, int editable);
	RAVEN_API int Rcombo_box_is_editable(Rid combo_box);
	/**
	 * @brief R_TRUE: typing in the combo box filters its list (Searchable="true"):
	 * the items that contain what was typed, without regard to case or
	 * accents ("zoe" finds "Zoë", "sao" finds "São Paulo"), or a row saying that nothing
	 * matches. Up and Down move in the list, Enter chooses, Escape closes it.
	 * A searchable combo box that is not editable comes back to the item
	 * chosen when nothing is.
	 */
	RAVEN_API void Rcombo_box_set_searchable(Rid combo_box, int searchable);
	RAVEN_API int Rcombo_box_is_searchable(Rid combo_box);
	/** @brief The text shown: what was typed in an editable combo box, else the text of the selected item ("" for none). */
	RAVEN_API const char* Rcombo_box_get_text(Rid combo_box);
	/** @brief Editable combo box: sets the text of the field (the item equal to it is selected); otherwise selects the item with this text. */
	RAVEN_API void Rcombo_box_set_text(Rid combo_box, const char* text);
	/** @brief Called each time the user changes the text of an editable or searchable combo box. */
	typedef void (*RComboBoxTextCallback)(Rid combo_box, const char* text, void* user_data);
	RAVEN_API void Rcombo_box_set_text_changed_callback(Rid combo_box, RComboBoxTextCallback callback, void* user_data);
	/** @brief Opens (R_TRUE) or closes the list. */
	RAVEN_API void Rcombo_box_set_open(Rid combo_box, int open);
	RAVEN_API int Rcombo_box_is_open(Rid combo_box);


	/* ============================================================================
	 *  SEPARATOR
	 * ============================================================================ */

	/** @brief R_HORIZONTAL draws a horizontal line (default). */
	RAVEN_API void Rseparator_set_orientation(Rid separator, ROrientation orientation);


	/* ============================================================================
	 *  LIST VIEW (virtualized: only the visible rows are processed)
	 * ============================================================================ */

	/** @return The index of the new item. */
	RAVEN_API int Rlist_view_add_item(Rid list_view, const char* text);
	RAVEN_API void Rlist_view_set_item_text(Rid list_view, int index, const char* text);
	RAVEN_API const char* Rlist_view_get_item_text(Rid list_view, int index);
	RAVEN_API void Rlist_view_remove_item(Rid list_view, int index);
	RAVEN_API void Rlist_view_clear_items(Rid list_view);
	RAVEN_API int Rlist_view_get_item_count(Rid list_view);

	/**
	 * Virtual mode, for large or computed data: the list stores nothing and
	 * asks for the text of the rows it draws. The returned string must stay
	 * valid until the next call.
	 */
	typedef const char* (*RListItemTextCallback)(Rid list_view, int index, void* user_data);
	RAVEN_API void Rlist_view_set_virtual_items(Rid list_view, int count, RListItemTextCallback callback, void* user_data);

	/**
	 * Colors the text of rows (a level, a verdict): the list asks for the color
	 * of each row it draws, stored or virtual. Set r, g, b, a (0..1) and return
	 * R_TRUE; R_FALSE (or alpha 0) keeps the color of the theme. The color is
	 * kept on a selected or hovered row. NULL removes the callback.
	 */
	typedef int (*RListItemColorCallback)(Rid list_view, int index, float* r, float* g, float* b, float* a, void* user_data);
	RAVEN_API void Rlist_view_set_item_color_callback(Rid list_view, RListItemColorCallback callback, void* user_data);

	/** @brief Selects a row. -1 selects nothing. */
	RAVEN_API void Rlist_view_set_selected(Rid list_view, int index);
	RAVEN_API int Rlist_view_get_selected(Rid list_view);
	RAVEN_API void Rlist_view_scroll_to_item(Rid list_view, int index);
	typedef void (*RListEventCallback)(Rid list_view, int index, void* user_data);
	RAVEN_API void Rlist_view_set_selection_changed_callback(Rid list_view, RListEventCallback callback, void* user_data);
	/** @brief Called on double click or Enter. This also fires the action of the widget. */
	RAVEN_API void Rlist_view_set_activated_callback(Rid list_view, RListEventCallback callback, void* user_data);


	/* ============================================================================
	 *  TREE VIEW (virtualized)
	 * ============================================================================ */

	/**
	 * @brief Adds an item.
	 * @param parent_item Item to add under, or R_INVALID_ID for a top-level item.
	 * @return The handle of the new item.
	 */
	RAVEN_API Rid Rtree_view_add_item(Rid tree_view, Rid parent_item, const char* text);
	/** @brief Deletes an item and its descendants. */
	RAVEN_API void Rtree_item_destroy(Rid item);
	RAVEN_API void Rtree_view_clear_items(Rid tree_view);
	RAVEN_API int Rtree_item_is_valid(Rid item);

	RAVEN_API void Rtree_item_set_text(Rid item, const char* text);
	RAVEN_API const char* Rtree_item_get_text(Rid item);
	RAVEN_API void Rtree_item_set_expanded(Rid item, int expanded);
	RAVEN_API int Rtree_item_is_expanded(Rid item);
	RAVEN_API void Rtree_item_set_user_handle(Rid item, void* handle);
	RAVEN_API void* Rtree_item_get_user_handle(Rid item);
	/** @brief Top-level items of a tree. */
	RAVEN_API int Rtree_view_get_item_count(Rid tree_view);
	RAVEN_API Rid Rtree_view_get_item(Rid tree_view, int index);
	/** @return The parent item, or R_INVALID_ID for a top-level item. */
	RAVEN_API Rid Rtree_item_get_parent(Rid item);
	RAVEN_API int Rtree_item_get_child_count(Rid item);
	RAVEN_API Rid Rtree_item_get_child(Rid item, int index);

	/** @brief Selects an item. R_INVALID_ID selects nothing. */
	RAVEN_API void Rtree_view_set_selected_item(Rid tree_view, Rid item);
	RAVEN_API Rid Rtree_view_get_selected_item(Rid tree_view);
	typedef void (*RTreeEventCallback)(Rid tree_view, Rid item, void* user_data);
	RAVEN_API void Rtree_view_set_selection_changed_callback(Rid tree_view, RTreeEventCallback callback, void* user_data);
	/** @brief Called on double click or Enter. This also fires the action of the widget. */
	RAVEN_API void Rtree_view_set_activated_callback(Rid tree_view, RTreeEventCallback callback, void* user_data);

	/**
	 * @brief Icon drawn before the text of the item (a region u0,v0,u1,v1 of the texture). R_INVALID_ID removes it.
	 * An SVG in currentColor (Ricon_get) takes the color of the text. XML: Icon on an Item.
	 */
	RAVEN_API void Rtree_item_set_icon(Rid item, Rid texture, float u0, float v0, float u1, float v1);
	/** @brief Color of the text of the item; alpha 0 goes back to the theme. */
	RAVEN_API void Rtree_item_set_text_color(Rid item, float r, float g, float b, float a);

	/**
	 * @brief Several items can be selected: Ctrl + click adds or removes one,
	 * Shift + click selects a range. The selection callback receives the item
	 * clicked last (the primary one, Rtree_view_get_selected_item).
	 */
	RAVEN_API void Rtree_view_set_multi_select(Rid tree_view, int enabled);
	RAVEN_API void Rtree_item_set_selected(Rid item, int selected);
	RAVEN_API int Rtree_item_is_selected(Rid item);
	RAVEN_API void Rtree_view_clear_selection(Rid tree_view);
	/** @brief Selected items, top to bottom. */
	RAVEN_API int Rtree_view_get_selection_count(Rid tree_view);
	RAVEN_API Rid Rtree_view_get_selection(Rid tree_view, int index);
	RAVEN_API void Rtree_view_scroll_to_item(Rid tree_view, Rid item);

	/**
	 * @brief Right click: `item` is the item under the pointer (selected
	 * first), or R_INVALID_ID in the empty space; x, y in the window, for Rmenu_open.
	 */
	typedef void (*RTreeContextCallback)(Rid tree_view, Rid item, float x, float y, void* user_data);
	RAVEN_API void Rtree_view_set_context_callback(Rid tree_view, RTreeContextCallback callback, void* user_data);

	/**
	 * @brief Items can be dragged: the selection is given to Rdrag_begin with
	 * this type and, as data, the handles of the items in decimal separated
	 * by spaces. NULL: no drag (default).
	 */
	RAVEN_API void Rtree_view_set_drag_type(Rid tree_view, const char* type);
	/** @brief Item at a point relative to the tree (drop callbacks give local_x, local_y), or R_INVALID_ID. */
	RAVEN_API Rid Rtree_view_get_item_at(Rid tree_view, float local_x, float local_y);
	/** @brief Highlights where a drop would go: an item, or the whole tree (`whole_tree`). R_INVALID_ID, 0 removes it. */
	RAVEN_API void Rtree_view_set_drop_highlight(Rid tree_view, Rid item, int whole_tree);


	/* ============================================================================
	 *  MENUS
	 * ============================================================================ */

	/**
	 * A MENU_BAR holds MENUs; a MENU holds MENU_ITEMs and sub-MENUs. A MENU
	 * that has no parent can be opened anywhere as a context menu.
	 */

	/**
	 * @brief The menu bar of the system. On macOS, the first MenuBar of a
	 * window is shown in the bar at the top of the screen while the window is
	 * the active one, after the menu of the application (About, Hide, Quit);
	 * it then takes no room in the window. Its shortcuts are those of the
	 * system: R_MOD_SUPER is Command, R_MOD_CTRL is Control. Elsewhere it is
	 * drawn in the window. On by default; R_FALSE keeps it in the window
	 * everywhere. In XML: <MenuBar Native="false">.
	 */
	RAVEN_API void Rmenu_bar_set_native(Rid menu_bar, int native);
	/** @return R_TRUE while the menu bar is shown by the system instead of the window. */
	RAVEN_API int Rmenu_bar_is_native(Rid menu_bar);

	/** @brief Creates a menu inside a menu bar or as a sub-menu of another menu. */
	RAVEN_API Rid Rmenu_add_menu(Rid menu_bar_or_menu, const char* title);
	RAVEN_API void Rmenu_set_title(Rid menu, const char* title);

	RAVEN_API Rid Rmenu_add_item(Rid menu, const char* text);
	RAVEN_API Rid Rmenu_add_separator(Rid menu);
	/** @brief A noninteractive caption row, skipped by keyboard navigation. */
	RAVEN_API Rid Rmenu_add_header(Rid menu, const char* text);
	/** @brief A vertical divider between titles of an in-window MenuBar. Ignored by native macOS bars. */
	RAVEN_API Rid Rmenu_bar_add_divider(Rid menu_bar);
	/** @brief Title color in an in-window bar; alpha 0 restores the theme. Ignored by native macOS bars. */
	RAVEN_API void Rmenu_set_title_color(Rid menu, float r, float g, float b, float a);

	RAVEN_API void Rmenu_item_set_text(Rid item, const char* text);
	RAVEN_API const char* Rmenu_item_get_text(Rid item);
	/**
	 * @brief Shows a shortcut next to the item and triggers the item when the
	 * keys are pressed in the window of its menu bar.
	 */
	RAVEN_API void Rmenu_item_set_shortcut(Rid item, RKey key, Ruint modifiers);
	/**
	 * @brief Display-only shortcut hint; copied, never binds a key. Overrides
	 * the formatted shortcut without changing its binding. NULL restores the
	 * formatted shortcut; an empty string hides it. XML: ShortcutText.
	 */
	RAVEN_API void Rmenu_item_set_shortcut_text(Rid item, const char* text);
	RAVEN_API void Rmenu_item_set_checkable(Rid item, int checkable);
	RAVEN_API void Rmenu_item_set_checked(Rid item, int checked);
	/**
	 * @brief Icon drawn before the text of a menu item (after the check mark column).
	 * @param texture R_INVALID_ID removes it. u0, v0, u1, v1 : region of the texture.
	 * @param tint    R_TRUE: the texture is a mask drawn in the text color (monochrome icons).
	 *                An SVG in currentColor (Ricon_get) always is. XML: Icon.
	 */
	RAVEN_API void Rmenu_item_set_icon(Rid item, Rid texture, float u0, float v0, float u1, float v1, int tint);
	RAVEN_API int Rmenu_item_is_checked(Rid item);
	typedef void (*RMenuItemClickedCallback)(Rid item, void* user_data);
	RAVEN_API void Rmenu_item_set_clicked_callback(Rid item, RMenuItemClickedCallback callback, void* user_data);

	/** @brief Opens a menu as a popup at a position of the window (logical units). */
	RAVEN_API void Rmenu_open(Rid menu, Rid window, float x, float y);
	/** @brief Closes every open menu and popup of the window. */
	RAVEN_API void Rwindow_close_popups(Rid window);


	/* ============================================================================
	 *  TILE VIEW (grid of tiles: an icon over a name; virtualized)
	 * ============================================================================ */

	/** What a tile shows, filled by the application when the tile is drawn. */
	typedef struct RTileItem {
		/** Name under the icon (valid until the callback returns). */
		const char* text;
		/** Second line, dimmer, or NULL. */
		const char* detail;
		/** Small text at the top right of the tile, or NULL. */
		const char* badge;
		/** Icon texture (R_INVALID_ID: none), the region of it and its tint (white by default; an SVG in currentColor is also tinted with the color of the name). */
		Rid icon;
		float uv[4];
		float tint[4];
		float icon_radius;
		/** Color of the name; alpha 0 = the theme. */
		float text_color[4];
		/** Layout="Rows": a third line under the detail, dimmer, or NULL. */
		const char* note;
		/** Color of the detail line; alpha 0 = the theme (dimmer than the name). */
		float detail_color[4];
	} RTileItem;

	/** A section of a tile view: its first tile, and what its header shows. */
	typedef struct RTileSection {
		/** Index of the first tile of the section (read when the sections are given). */
		int first;
		/** Title of the header (valid until the callback returns). */
		const char* text;
		/** Dimmer text after the title (a count...), or NULL. */
		const char* detail;
	} RTileSection;

	/**
	 * Describes section `section`: when the sections are given (Rtile_view_set_sections,
	 * `first` is read) and when its header is drawn (`text`, `detail`). UI thread.
	 */
	typedef void (*RTileSectionCallback)(Rid tile_view, int section, RTileSection* out, void* user_data);

	/** Fills `item` for tile `index`. Called on the UI thread, only for the tiles in view. */
	typedef void (*RTileItemCallback)(Rid tile_view, int index, RTileItem* item, void* user_data);
	/** Selection change (`index`: the current tile, or -1) and activation (double click, Enter). */
	typedef void (*RTileEventCallback)(Rid tile_view, int index, void* user_data);
	/** Right click: the tile under the pointer (selected first), or -1 in the empty space; x, y in the window. */
	typedef void (*RTileContextCallback)(Rid tile_view, int index, float x, float y, void* user_data);

	/** @brief Gives the tiles: `count` of them, described by `callback`. The selection is cleared. */
	RAVEN_API void Rtile_view_set_items(Rid tile_view, int count, RTileItemCallback callback, void* user_data);
	/** @brief Draws the tiles again (their description changed, not their number). */
	RAVEN_API void Rtile_view_refresh(Rid tile_view);
	RAVEN_API int Rtile_view_get_count(Rid tile_view);
	/** @brief Size of a tile (its name included) and of its icon. Default 112 x 112, icon 64. */
	RAVEN_API void Rtile_view_set_tile_size(Rid tile_view, float width, float height, float icon_size);
	RAVEN_API void Rtile_view_set_empty_text(Rid tile_view, const char* text);
	/**
	 * @brief Layout="Rows": width of the box the icon is drawn in (its height is the
	 * icon size), for icons that are not square; 0 = square (default). XML: IconWidth.
	 */
	RAVEN_API void Rtile_view_set_icon_width(Rid tile_view, float width);

	/*
	 * Sections: the tiles, in their order, are cut into runs under a header row (a
	 * chevron, a title, a dimmer detail). A click on a header folds its section (its
	 * tiles are neither drawn nor hit; the keys, Shift ranges and the rubber band pass
	 * over them) or unfolds it. Tiles before the first section have no header.
	 * The height of the headers: XML HeaderHeight (default 24).
	 */
	/** @brief `count` sections, described by `callback`, all unfolded. Rtile_view_set_items removes them: give them after the tiles. */
	RAVEN_API void Rtile_view_set_sections(Rid tile_view, int count, RTileSectionCallback callback, void* user_data);
	RAVEN_API int Rtile_view_get_section_count(Rid tile_view);
	/** @brief The section of a tile, or -1 (before the first section). */
	RAVEN_API int Rtile_view_get_section_of(Rid tile_view, int index);
	/** @brief Folds or unfolds a section (the toggled callback is not called). */
	RAVEN_API void Rtile_view_set_section_folded(Rid tile_view, int section, int folded);
	RAVEN_API int Rtile_view_is_section_folded(Rid tile_view, int section);
	/** @brief The section whose header is at a point relative to the widget, or -1. */
	RAVEN_API int Rtile_view_get_section_at(Rid tile_view, float local_x, float local_y);
	/** @brief Called when the user folds or unfolds a section (`index`: the section). */
	RAVEN_API void Rtile_view_set_section_toggled_callback(Rid tile_view, RTileEventCallback callback, void* user_data);

	/* Several tiles can be selected: Ctrl + click, Shift + click, a rubber band from the empty space, Ctrl + A. */
	RAVEN_API void Rtile_view_set_selected(Rid tile_view, int index, int selected);
	RAVEN_API int Rtile_view_is_selected(Rid tile_view, int index);
	RAVEN_API void Rtile_view_clear_selection(Rid tile_view);
	RAVEN_API void Rtile_view_select_only(Rid tile_view, int index);
	/** @brief The tile clicked last, or -1. */
	RAVEN_API int Rtile_view_get_current(Rid tile_view);
	RAVEN_API int Rtile_view_get_selection_count(Rid tile_view);
	/** @brief Index of the nth selected tile, in order. */
	RAVEN_API int Rtile_view_get_selection(Rid tile_view, int nth);
	RAVEN_API void Rtile_view_scroll_to(Rid tile_view, int index);
	/** @brief Tile at a point relative to the widget (drop callbacks give local_x, local_y), or -1. */
	RAVEN_API int Rtile_view_get_item_at(Rid tile_view, float local_x, float local_y);
	/** @brief Highlights the tile a drop would go to; -1 removes it. */
	RAVEN_API void Rtile_view_set_drop_highlight(Rid tile_view, int index);
	/**
	 * @brief Tiles can be dragged: the selected tiles are given to Rdrag_begin
	 * with this type and, as data, their indices separated by spaces.
	 */
	RAVEN_API void Rtile_view_set_drag_type(Rid tile_view, const char* type);
	/**
	 * @brief The data of a drag, given by the application (paths of files...)
	 * instead of the indices: the dragged tiles are the selected ones
	 * (Rtile_view_get_selection). The text returned is copied; NULL keeps
	 * the indices.
	 */
	typedef const char* (*RTileDragDataCallback)(Rid tile_view, void* user_data);
	RAVEN_API void Rtile_view_set_drag_data_callback(Rid tile_view, RTileDragDataCallback callback, void* user_data);
	/**
	 * @brief Tiles can be dragged out of the window, to other applications:
	 * when the user starts dragging the selected tiles (`index` is the one
	 * under the pointer), the callback gives their data by calling
	 * Rdrag_begin_external (the pictures, the files they stand for...), and
	 * Rdrag_set_image. With a drag type too (Rtile_view_set_drag_type), the
	 * drag first goes to the drop targets of the window and only leaves it
	 * with the pointer. NULL: the tiles stay in the application (default).
	 */
	RAVEN_API void Rtile_view_set_drag_out_callback(Rid tile_view, RTileEventCallback callback, void* user_data);

	RAVEN_API void Rtile_view_set_selection_changed_callback(Rid tile_view, RTileEventCallback callback, void* user_data);
	RAVEN_API void Rtile_view_set_activated_callback(Rid tile_view, RTileEventCallback callback, void* user_data);
	RAVEN_API void Rtile_view_set_context_callback(Rid tile_view, RTileContextCallback callback, void* user_data);

	/* ============================================================================
	 *  TABLE (rows of cells under columns; virtualized)
	 *
	 *      <Table Name="people" Striped="true">
	 *          <Column Header="Name" Width="160"/>
	 *          <Column Header="Age" Width="60" Align="Right"/>
	 *          <Column Header="City" Fill="1"/>
	 *          <Row>Ada|36|London</Row>
	 *          <Row><Cell>Grace</Cell><Cell>85</Cell><Cell>New York</Cell></Row>
	 *      </Table>
	 *
	 * The user drags the edge of a header to resize its column (a double
	 * click fits it to its texts) and clicks a header to sort. A stored table
	 * sorts its rows itself (numbers by value, texts without regard to case):
	 * the indices are always those of the order shown. A virtual table only
	 * shows the sorted column and calls the sort callback, where the
	 * application orders its data.
	 *
	 * Cells of the columns made editable (Rtable_set_column_editable, or
	 * Editable="true" on a Column) are edited in place: a double click, F2,
	 * Enter or a typed character on the current cell opens a text field over
	 * it. Enter keeps the text and goes down a row, Tab and Shift+Tab keep it
	 * and go to the next or previous editable cell, Escape forgets it, and a
	 * click elsewhere keeps it. Left and Right move the current cell. The
	 * first columns can stay in place while the others scroll sideways
	 * (Rtable_set_frozen_columns, FrozenColumns="1").
	 *
	 * One row is selected at a time by default. With SelectionMode="Multiple"
	 * (Rtable_set_selection_mode) a click selects one row, Ctrl/Cmd+click
	 * toggles a row, Shift+click and Shift with the arrows, Page Up / Page
	 * Down, Home and End extend from the anchor, Ctrl+arrows move the current
	 * row without selecting, Space toggles the current row and Ctrl/Cmd+A
	 * selects every row; a tap of a finger toggles a row. The selection is
	 * kept as ranges, so selecting a million virtual rows costs nothing.
	 *
	 * A column can draw its cells as a check box or a progress bar instead
	 * of a text (Rtable_set_column_kind, Kind="Check" or Kind="Progress"),
	 * from the text of the cell, stored or virtual: "1"/"0" for a check box,
	 * 0..1 (or "45%") for a progress bar. The table draws them itself: no
	 * widget is made per cell.
	 * ============================================================================ */

	/** How the rows of a Table are selected (Rtable_set_selection_mode). */
	typedef enum RTableSelectionMode{
		/** One row at a time (the default). */
		R_TABLE_SELECTION_SINGLE = 0x0BC0,
		/** Any number of rows: Ctrl/Cmd+click, Shift+click, Shift+arrows, Space, Ctrl/Cmd+A. */
		R_TABLE_SELECTION_MULTIPLE,
		/** The user selects nothing (the rows are only shown; cells may still be edited and checked). */
		R_TABLE_SELECTION_NONE
	}RTableSelectionMode;

	/** What the cells of a column show (Rtable_set_column_kind). */
	typedef enum RTableColumnKind{
		/** Their text (the default). */
		R_TABLE_COLUMN_TEXT = 0x0BD0,
		/**
		 * A check box, checked when the text is "1", "true" or "yes". The user
		 * toggles it with a click or Space unless the column is made not
		 * editable; the cell edited callback is then called with "1" or "0".
		 */
		R_TABLE_COLUMN_CHECK,
		/** A progress bar, from a number 0..1 or a percentage ("45%"); an empty cell shows nothing. */
		R_TABLE_COLUMN_PROGRESS
	}RTableColumnKind;

	/**
	 * @brief Adds a column.
	 * @param width Width in logical units; 0 or less gives the default (120).
	 * @return The index of the column. The stored rows get an empty cell in it.
	 */
	RAVEN_API int Rtable_add_column(Rid table, const char* header, float width);
	RAVEN_API int Rtable_get_column_count(Rid table);
	/** @brief Removes the columns and the stored rows. */
	RAVEN_API void Rtable_clear_columns(Rid table);
	RAVEN_API void Rtable_set_column_header(Rid table, int column, const char* header);
	RAVEN_API const char* Rtable_get_column_header(Rid table, int column);
	RAVEN_API void Rtable_set_column_width(Rid table, int column, float width);
	/** @brief The width the column has on screen (a filling column takes more than its width). */
	RAVEN_API float Rtable_get_column_width(Rid table, int column);
	/** @brief > 0: the column takes this share of the room the others leave; 0: its width only. */
	RAVEN_API void Rtable_set_column_fill(Rid table, int column, float fill);
	/** @brief R_FALSE: a click on the header does not sort (default R_TRUE). */
	RAVEN_API void Rtable_set_column_sortable(Rid table, int column, int sortable);
	/** @brief Where the texts sit in the cells: 0 left, 0.5 center, 1 right. */
	RAVEN_API void Rtable_set_column_align(Rid table, int column, float align);

	/**
	 * @brief Adds a row.
	 * @param cells The texts of the cells, separated by '|' ("Ada|36|London"),
	 *              or NULL for an empty row; Rtable_set_cell gives a text holding a '|'.
	 * @return The index of the row.
	 */
	RAVEN_API int Rtable_add_row(Rid table, const char* cells);
	RAVEN_API void Rtable_set_cell(Rid table, int row, int column, const char* text);
	RAVEN_API const char* Rtable_get_cell(Rid table, int row, int column);
	RAVEN_API void Rtable_remove_row(Rid table, int row);
	RAVEN_API void Rtable_clear_rows(Rid table);
	RAVEN_API int Rtable_get_row_count(Rid table);

	/**
	 * Virtual mode, for large or computed data: the table stores nothing and
	 * asks for the texts of the cells it draws. The returned string must stay
	 * valid until the next call.
	 */
	typedef const char* (*RTableCellTextCallback)(Rid table, int row, int column, void* user_data);
	RAVEN_API void Rtable_set_virtual_rows(Rid table, int count, RTableCellTextCallback callback, void* user_data);
	/**
	 * Colors the text of cells (a verdict, a state): the table asks for the
	 * color of each cell it draws, stored or virtual (`row` is the index of the
	 * order shown). Set r, g, b, a (0..1) and return R_TRUE; R_FALSE (or alpha 0)
	 * keeps the color set with Rtable_set_cell_text_color, else the theme's.
	 * The color is kept on a selected or hovered row. NULL removes the callback.
	 */
	typedef int (*RTableCellColorCallback)(Rid table, int row, int column, float* r, float* g, float* b, float* a, void* user_data);
	RAVEN_API void Rtable_set_cell_color_callback(Rid table, RTableCellColorCallback callback, void* user_data);
	/**
	 * @brief Stored rows: a fixed text color; column -1 colors the row; cells override it.
	 * Alpha 0 restores the inherited/theme color. A color from the cell color callback wins over it.
	 */
	RAVEN_API void Rtable_set_cell_text_color(Rid table, int row, int column, float r, float g, float b, float a);
	/** @brief Opt into Ctrl/Cmd toggles and Shift ranges; single selection is the default. Same as Rtable_set_selection_mode(MULTIPLE or SINGLE). */
	RAVEN_API void Rtable_set_multi_select(Rid table, int enabled);
	/**
	 * @brief Single (default), multiple or no selection (SelectionMode="Multiple" in XML).
	 * Going to single keeps the selected row, going to none clears the selection.
	 */
	RAVEN_API void Rtable_set_selection_mode(Rid table, RTableSelectionMode mode);
	RAVEN_API RTableSelectionMode Rtable_get_selection_mode(Rid table);
	RAVEN_API void Rtable_set_row_selected(Rid table, int row, int selected);
	RAVEN_API int Rtable_is_row_selected(Rid table, int row);
	/**
	 * @brief Selects (`selected` R_TRUE) or deselects the rows first..last, both
	 * included, keeping the others: multiple selection only (in single
	 * selection, selecting a range selects `last`).
	 */
	RAVEN_API void Rtable_select_range(Rid table, int first, int last, int selected);
	/** @brief Multiple selection: selects every row (Ctrl/Cmd+A). Nothing in the other modes. */
	RAVEN_API void Rtable_select_all(Rid table);
	RAVEN_API void Rtable_clear_selection(Rid table);
	RAVEN_API int Rtable_get_selection_count(Rid table);
	/** @brief Selected row at index, top to bottom; -1 outside the selection. */
	RAVEN_API int Rtable_get_selection(Rid table, int index);
	/**
	 * @brief The selection as runs of consecutive rows, top to bottom: the
	 * fast way to go through a large selection (select all of a million rows
	 * is one range).
	 */
	RAVEN_API int Rtable_get_selection_range_count(Rid table);
	/** @brief The rows first..last (both included) of a range of the selection. @return R_FALSE outside the ranges. */
	RAVEN_API int Rtable_get_selection_range(Rid table, int index, int* first, int* last);

	/** @brief Replaces the selection with one row. -1 selects nothing, including in multi-select mode. */
	RAVEN_API void Rtable_set_selected(Rid table, int row);
	/**
	 * @brief The selected row (-1: none). In multiple selection, the one
	 * selected last, or else the first one; the selection changed callback
	 * receives it.
	 */
	RAVEN_API int Rtable_get_selected(Rid table);
	/**
	 * @brief The row the keyboard is on: the selected row in single
	 * selection; in multiple selection the row last clicked or reached with
	 * the arrows, which Space toggles (it may be unselected). -1: none.
	 */
	RAVEN_API int Rtable_get_current_row(Rid table);
	RAVEN_API void Rtable_scroll_to_row(Rid table, int row);
	/** @brief Sorts by a column (stored rows are moved); -1 shows no sorted column. */
	RAVEN_API void Rtable_sort(Rid table, int column, int descending);
	RAVEN_API int Rtable_get_sort_column(Rid table);
	RAVEN_API int Rtable_is_sort_descending(Rid table);
	RAVEN_API void Rtable_set_header_visible(Rid table, int visible);
	RAVEN_API void Rtable_set_grid_lines(Rid table, int visible);
	/** @brief Every other row a little darker. */
	RAVEN_API void Rtable_set_striped(Rid table, int striped);

	typedef void (*RTableEventCallback)(Rid table, int row, void* user_data);
	RAVEN_API void Rtable_set_selection_changed_callback(Rid table, RTableEventCallback callback, void* user_data);
	/** @brief Called on double click or Enter. This also fires the action of the widget. */
	RAVEN_API void Rtable_set_activated_callback(Rid table, RTableEventCallback callback, void* user_data);
	/** @brief Called when the user clicks a header; `column` -1: not sorted. */
	typedef void (*RTableSortCallback)(Rid table, int column, int descending, void* user_data);
	RAVEN_API void Rtable_set_sort_callback(Rid table, RTableSortCallback callback, void* user_data);

	/** @brief R_TRUE: the cells of the column are edited in place by the user (default R_FALSE). */
	RAVEN_API void Rtable_set_column_editable(Rid table, int column, int editable);
	RAVEN_API int Rtable_is_column_editable(Rid table, int column);
	/**
	 * @brief Called when the user keeps the text typed in a cell (Enter, Tab,
	 * a click elsewhere). Return R_TRUE to accept it: a stored table then
	 * keeps the text in the cell, unless the callback set the cell itself
	 * (Rtable_set_cell, to keep "12.50" for "12.5"). A virtual table stores
	 * nothing: the application keeps the text in its data. Return R_FALSE to
	 * refuse it: after Enter or Tab the field stays open on the text, so the
	 * user can correct it; after a click elsewhere the edit is dropped.
	 * Without a callback, a stored table keeps every text.
	 */
	typedef int (*RTableCellEditedCallback)(Rid table, int row, int column, const char* text, void* user_data);
	RAVEN_API void Rtable_set_cell_edited_callback(Rid table, RTableCellEditedCallback callback, void* user_data);
	/**
	 * @brief Opens the editor on a cell, its text selected, as F2 does (the
	 * row is selected and scrolled into view). Any column can be edited this
	 * way, editable or not. @return R_TRUE when the editor is open.
	 */
	RAVEN_API int Rtable_edit_cell(Rid table, int row, int column);
	RAVEN_API int Rtable_is_editing(Rid table);
	/**
	 * @brief Closes the editor: `commit` R_TRUE keeps the text (through the
	 * cell edited callback), R_FALSE forgets it. @return R_FALSE when the text
	 * was refused (the editor then stays open), R_TRUE otherwise.
	 */
	RAVEN_API int Rtable_end_edit(Rid table, int commit);
	/**
	 * @brief The first `count` columns stay in place when the table scrolls
	 * sideways; a thin shadow marks where the others pass under them. 0 (the
	 * default): every column scrolls.
	 */
	RAVEN_API void Rtable_set_frozen_columns(Rid table, int count);
	RAVEN_API int Rtable_get_frozen_columns(Rid table);
	/**
	 * @brief Text (default), check box or progress bar: how the cells of the
	 * column are drawn from their texts. A check column is made editable (the
	 * user toggles it; Rtable_set_column_editable(R_FALSE) after this shows it
	 * only) and centered. For anything else, a cell color callback and the
	 * texts are what the table offers: it draws no other widget in its cells.
	 */
	RAVEN_API void Rtable_set_column_kind(Rid table, int column, RTableColumnKind kind);
	RAVEN_API RTableColumnKind Rtable_get_column_kind(Rid table, int column);
	/** @brief A cell of a check column: R_TRUE when its text is "1", "true" or "yes". Any cell can be asked. */
	RAVEN_API int Rtable_is_cell_checked(Rid table, int row, int column);
	/** @brief Stored rows: writes "1" or "0" in the cell (no callback is called). */
	RAVEN_API void Rtable_set_cell_checked(Rid table, int row, int column, int checked);


	/* ============================================================================
	 *  DATE PICKER (a date and the calendar that drops down from it)
	 *
	 *      <DatePicker Date="2026-11-03" FirstDayOfWeek="1"/>
	 *
	 * Months are 1 to 12, days 1 to 31. The names of the months and of the days
	 * are English unless the locale has the keys Raven.Month1 to Raven.Month12
	 * and Raven.Day0 (Sunday) to Raven.Day6.
	 * ============================================================================ */

	/** @brief Sets the date (today when the picker is created). An invalid date is refused. */
	RAVEN_API void Rdate_picker_set_date(Rid date_picker, int year, int month, int day);
	RAVEN_API void Rdate_picker_get_date(Rid date_picker, int* year, int* month, int* day);
	/** @brief The earliest and the latest dates that can be chosen; 0 for year leaves that side open. */
	RAVEN_API void Rdate_picker_set_range(Rid date_picker, int min_year, int min_month, int min_day, int max_year, int max_month, int max_day);
	/** @brief 0 = Sunday (default), 1 = Monday... */
	RAVEN_API void Rdate_picker_set_first_day_of_week(Rid date_picker, int day);
	/**
	 * @brief How the field writes the date: YYYY, MM, M, DD, D, MMMM (the name
	 * of the month), MMM (its first three letters), dddd (the name of the day),
	 * ddd; anything else is kept. Default "YYYY-MM-DD"; "D MMMM YYYY" gives "3 November 2026".
	 */
	RAVEN_API void Rdate_picker_set_format(Rid date_picker, const char* format);
	/** @brief The date as the field shows it (valid until the next call). */
	RAVEN_API const char* Rdate_picker_get_text(Rid date_picker);
	/** @brief Opens or closes the calendar. */
	RAVEN_API void Rdate_picker_set_open(Rid date_picker, int open);
	RAVEN_API int Rdate_picker_is_open(Rid date_picker);
	/** @brief Called when the user chooses a date. This also fires the action of the widget. */
	typedef void (*RDateChangedCallback)(Rid date_picker, int year, int month, int day, void* user_data);
	RAVEN_API void Rdate_picker_set_changed_callback(Rid date_picker, RDateChangedCallback callback, void* user_data);
	/** @brief The time zone of the date, for the instants below; NULL or "" (default): the zone of the system. TimeZone="..." in XML. */
	RAVEN_API void Rdate_picker_set_time_zone(Rid date_picker, const char* zone);
	RAVEN_API const char* Rdate_picker_get_time_zone(Rid date_picker);
	/** @brief Shows the date an instant (seconds since 1970, UTC) has in the zone of the picker. */
	RAVEN_API void Rdate_picker_set_instant(Rid date_picker, double utc);
	/** @brief The first instant of the date in the zone: its midnight (or, where the clocks skip midnight, the time they go to). */
	RAVEN_API double Rdate_picker_get_instant(Rid date_picker);


	/* ============================================================================
	 *  TIME PICKER (hours : minutes [: seconds], on 12 or 24 hours, in a time zone)
	 *
	 *      <TimePicker Time="15:30" Clock="Auto" TimeZone="America/New_York" ShowTimeZone="true"/>
	 *
	 * A click on a part selects it; the arrows, the wheel or the digits change
	 * it. On 12 hours (the clock of the locale, see Rlocale_uses_12_hour_clock,
	 * or Clock="12") the hours show 1 to 12 and a part shows AM or PM: A and P
	 * (or the first letter of the AM and PM of the language) choose it, the
	 * arrows on it switch it, and the hours go on from 11 AM to 12 PM. The
	 * values given and returned stay on 24 hours (0 to 23).
	 *
	 * With a time zone, the time is a time of a day in that zone (today
	 * unless Rtime_picker_set_date says another): Rtime_picker_get_instant
	 * gives the instant it is, and ShowTimeZone writes the zone after it
	 * ("EST", "CET", "UTC+3").
	 * ============================================================================ */

	RAVEN_API void Rtime_picker_set_time(Rid time_picker, int hour, int minute, int second);
	RAVEN_API void Rtime_picker_get_time(Rid time_picker, int* hour, int* minute, int* second);
	RAVEN_API void Rtime_picker_set_seconds_visible(Rid time_picker, int visible);
	/** @brief Step of the minutes for the arrows and the wheel (default 1). */
	RAVEN_API void Rtime_picker_set_minute_step(Rid time_picker, int step);
	/** @brief Called when the user changes the time. This also fires the action of the widget. */
	typedef void (*RTimeChangedCallback)(Rid time_picker, int hour, int minute, int second, void* user_data);
	RAVEN_API void Rtime_picker_set_changed_callback(Rid time_picker, RTimeChangedCallback callback, void* user_data);
	/** @brief 12 or 24 hours for this picker; R_CLOCK_AUTO (default): the clock of the locale. Clock="Auto|12|24" in XML. */
	RAVEN_API void Rtime_picker_set_clock(Rid time_picker, RClock clock);
	RAVEN_API RClock Rtime_picker_get_clock(Rid time_picker);
	/** @brief The time as the field shows it: "3:05 PM", "15:05", with the zone when it is shown (valid until the next call). */
	RAVEN_API const char* Rtime_picker_get_text(Rid time_picker);
	/**
	 * @brief The time zone of the time ("America/New_York", see
	 * Rtimezone_is_valid); NULL or "" (default): the zone of the system. An
	 * unknown zone is refused. TimeZone="..." in XML.
	 */
	RAVEN_API void Rtime_picker_set_time_zone(Rid time_picker, const char* zone);
	/** @brief The zone set, "" for none. */
	RAVEN_API const char* Rtime_picker_get_time_zone(Rid time_picker);
	/** @brief Writes the zone after the time ("EST", "CET", "UTC+3"). ShowTimeZone="true" in XML. */
	RAVEN_API void Rtime_picker_set_time_zone_visible(Rid time_picker, int visible);
	/** @brief The day of the time, for its zone (month 1 to 12); year 0: today (default). Date="YYYY-MM-DD" in XML. */
	RAVEN_API void Rtime_picker_set_date(Rid time_picker, int year, int month, int day);
	RAVEN_API void Rtime_picker_get_date(Rid time_picker, int* year, int* month, int* day);
	/** @brief Shows an instant (seconds since 1970, UTC) in the zone of the picker: its day and its time. */
	RAVEN_API void Rtime_picker_set_instant(Rid time_picker, double utc);
	/**
	 * @brief The instant the day and the time are in the zone. A time the
	 * clocks skipped is moved forward, a time they repeated is the first
	 * (see Rtimezone_from_local).
	 */
	RAVEN_API double Rtime_picker_get_instant(Rid time_picker);


	/* ============================================================================
	 *  TOAST (a short message that rises over the content, then goes)
	 *
	 * Put it over the content, where it should appear:
	 *
	 *      <Overlay>
	 *          ...the content...
	 *          <Toast Name="toast" Slot.HAlign="Center" Slot.VAlign="Bottom" Slot.Padding="16"/>
	 *      </Overlay>
	 *
	 * Hidden, it takes no click. By default Rtoast_show replaces the message
	 * shown; Rtoast_set_queue makes the messages wait their turn
	 * (R_TOAST_SEQUENCE) or stack up (R_TOAST_STACK: the oldest at the top,
	 * the newest at the bottom; put the toast against the bottom of the
	 * Overlay so that the stack grows upwards). Each message keeps the kind
	 * and the button it was shown with, and its own time.
	 * ============================================================================ */

	/** @brief Shows a message for `seconds` (0 or less: until Rtoast_hide), with the kind and button set now. */
	RAVEN_API void Rtoast_show(Rid toast, const char* text, double seconds);
	/** @brief Same, with a kind of its own (the kind set by Rtoast_set_kind is left as it is). */
	RAVEN_API void Rtoast_show_kind(Rid toast, const char* text, RToastKind kind, double seconds);
	/** @brief Hides the newest message shown; the next one waiting comes. */
	RAVEN_API void Rtoast_hide(Rid toast);
	/** @brief Hides every message, and forgets those waiting. */
	RAVEN_API void Rtoast_clear(Rid toast);
	RAVEN_API int Rtoast_is_shown(Rid toast);
	/** @return The messages shown and waiting. */
	RAVEN_API int Rtoast_get_count(Rid toast);
	/** @return The newest message shown (the last one, once hidden; the clicked one in the action callback). */
	RAVEN_API const char* Rtoast_get_text(Rid toast);
	RAVEN_API void Rtoast_set_queue(Rid toast, RToastQueue queue);
	RAVEN_API RToastQueue Rtoast_get_queue(Rid toast);
	/** @brief How many messages a R_TOAST_STACK shows at once (default 3). */
	RAVEN_API void Rtoast_set_max_visible(Rid toast, int count);
	/** @brief The kind of the next messages (and of the message shown, with R_TOAST_REPLACE). */
	RAVEN_API void Rtoast_set_kind(Rid toast, RToastKind kind);
	/** @brief A button at the end of the next messages ("Undo"); NULL or "" removes it. */
	RAVEN_API void Rtoast_set_action_text(Rid toast, const char* text);
	/** @brief Called by the button of a message, which then hides. This also fires the action of the widget. */
	typedef void (*RToastCallback)(Rid toast, void* user_data);
	RAVEN_API void Rtoast_set_action_callback(Rid toast, RToastCallback callback, void* user_data);


	/* ============================================================================
	 *  BREADCRUMB (Home > Documents > Report)
	 *
	 * The items before the last one are links; the last one is where the user is.
	 * When they do not fit, the first ones give their place to "...".
	 * ============================================================================ */

	/** @brief Replaces the items: their texts separated by '|'. */
	RAVEN_API void Rbreadcrumb_set_items(Rid breadcrumb, const char* items);
	/** @return The index of the new item, at the end. */
	RAVEN_API int Rbreadcrumb_add_item(Rid breadcrumb, const char* text);
	/** @brief Keeps the first `count` items: Rbreadcrumb_truncate(b, index + 1) goes back to the item clicked. */
	RAVEN_API void Rbreadcrumb_truncate(Rid breadcrumb, int count);
	RAVEN_API int Rbreadcrumb_get_item_count(Rid breadcrumb);
	RAVEN_API const char* Rbreadcrumb_get_item_text(Rid breadcrumb, int index);
	/** @brief Called when the user clicks an item. This also fires the action of the widget. */
	typedef void (*RBreadcrumbCallback)(Rid breadcrumb, int index, void* user_data);
	RAVEN_API void Rbreadcrumb_set_clicked_callback(Rid breadcrumb, RBreadcrumbCallback callback, void* user_data);


	/* ============================================================================
	 *  RATING (stars)
	 * ============================================================================ */

	/** @brief The score, from 0 to the number of stars. */
	RAVEN_API void Rrating_set_value(Rid rating, float value);
	RAVEN_API float Rrating_get_value(Rid rating);
	/** @brief Number of stars (default 5, at most 20). */
	RAVEN_API void Rrating_set_max(Rid rating, int max);
	RAVEN_API int Rrating_get_max(Rid rating);
	/** @brief Half stars can be chosen (a whole star by default). */
	RAVEN_API void Rrating_set_allow_half(Rid rating, int allow);
	/** @brief Shown only, the user cannot change it. */
	RAVEN_API void Rrating_set_read_only(Rid rating, int read_only);
	/** @brief Size of a star (default 18). */
	RAVEN_API void Rrating_set_star_size(Rid rating, float size);
	/** @brief Called when the user changes the score. This also fires the action of the widget. */
	typedef void (*RRatingChangedCallback)(Rid rating, float value, void* user_data);
	RAVEN_API void Rrating_set_changed_callback(Rid rating, RRatingChangedCallback callback, void* user_data);


	/* ============================================================================
	 *  PAGINATION (< 1 ... 4 5 6 ... 20 >)
	 *
	 * Pages are counted from 0; they are shown from 1.
	 * ============================================================================ */

	RAVEN_API void Rpagination_set_page_count(Rid pagination, int count);
	RAVEN_API int Rpagination_get_page_count(Rid pagination);
	RAVEN_API void Rpagination_set_page(Rid pagination, int page);
	RAVEN_API int Rpagination_get_page(Rid pagination);
	/** @brief Page numbers shown around the current one (default 1). */
	RAVEN_API void Rpagination_set_siblings(Rid pagination, int siblings);
	/** @brief Called when the user goes to another page. This also fires the action of the widget. */
	typedef void (*RPageChangedCallback)(Rid pagination, int page, void* user_data);
	RAVEN_API void Rpagination_set_changed_callback(Rid pagination, RPageChangedCallback callback, void* user_data);


	/* ============================================================================
	 *  PLOT (curves of X / Y data)
	 *
	 * Series of points drawn as lines and markers over two horizontal axes
	 * (R_PLOT_X under the data, R_PLOT_X2 over it) and two vertical ones
	 * (R_PLOT_Y on the left, R_PLOT_Y2 on the right). Each axis is linear,
	 * logarithmic or symmetric logarithmic, and follows the data until the
	 * user or the application moves it.
	 *
	 * The user moves the view with the left (or middle) button, zooms into a
	 * rectangle drawn with the right button, zooms around the pointer with
	 * the wheel, and fits the data again with a double click. Over an axis,
	 * the button and the wheel move only that axis. A click in the legend
	 * hides or shows a series; drag lines are dragged. Fingers move and pinch
	 * the view; the arrows, + / - and Home do the same from the keyboard.
	 *
	 * Series and drag lines are counted from 0, in the order they were added.
	 * ============================================================================ */

	RAVEN_API void Rplot_set_title(Rid plot, const char* title);
	RAVEN_API const char* Rplot_get_title(Rid plot);

	/** @brief Adds a series (its name in the legend; NULL or "": not in the legend). Returns its index. */
	RAVEN_API int Rplot_add_series(Rid plot, const char* name);
	RAVEN_API void Rplot_remove_series(Rid plot, int series);
	/** @brief Removes every series and every drag line. */
	RAVEN_API void Rplot_clear(Rid plot);
	RAVEN_API int Rplot_get_series_count(Rid plot);
	RAVEN_API void Rplot_set_series_name(Rid plot, int series, const char* name);
	RAVEN_API const char* Rplot_get_series_name(Rid plot, int series);
	/**
	 * @brief Replaces the points of a series with `count` points, copied.
	 * @param x The X of the points, or NULL: 0, 1, 2...
	 * @param y The Y of the points. NaN leaves a gap in the line.
	 */
	RAVEN_API void Rplot_set_series_data(Rid plot, int series, const double* x, const double* y, int count);
	/** @brief Adds points after those of a series (x NULL: numbered after the last one). */
	RAVEN_API void Rplot_append_series_data(Rid plot, int series, const double* x, const double* y, int count);
	/** @brief Adds one point after those of a series. */
	RAVEN_API void Rplot_add_point(Rid plot, int series, double x, double y);
	RAVEN_API void Rplot_clear_series_data(Rid plot, int series);
	RAVEN_API int Rplot_get_point_count(Rid plot, int series);
	/** @brief One point of a series. Returns R_FALSE when there is no such point. */
	RAVEN_API int Rplot_get_point(Rid plot, int series, int index, double* x, double* y);
	/** @brief Keeps only the last `max` points of a series, for data that streams in (0: all of them, the default). */
	RAVEN_API void Rplot_set_series_max_points(Rid plot, int series, int max);
	/** @brief The color of a series (by default the next one of a palette of ten). */
	RAVEN_API void Rplot_set_series_color(Rid plot, int series, float r, float g, float b, float a);
	/** @brief How the line is drawn and how wide (default solid, 1.75; width 0 keeps the width). */
	RAVEN_API void Rplot_set_series_line(Rid plot, int series, RPlotLine line, float width);
	/** @brief The mark on each point and its radius (default none, 4; size 0 keeps the size). */
	RAVEN_API void Rplot_set_series_marker(Rid plot, int series, RPlotMarker marker, float size);
	/** @brief Filled markers (the default) or only their outline. */
	RAVEN_API void Rplot_set_series_marker_filled(Rid plot, int series, int filled);
	/** @brief Draws the line in steps: along at the height of a point, then up or down to the next. */
	RAVEN_API void Rplot_set_series_step(Rid plot, int series, int step);
	/** @brief Shades the area between the line and zero, at this opacity (0: none, the default). */
	RAVEN_API void Rplot_set_series_fill(Rid plot, int series, float opacity);
	/** @brief The axes a series is drawn against: R_PLOT_X or R_PLOT_X2, R_PLOT_Y or R_PLOT_Y2. */
	RAVEN_API void Rplot_set_series_axes(Rid plot, int series, RPlotAxis x_axis, RPlotAxis y_axis);
	/** @brief Shows or hides a series (the user does it with a click on its name in the legend). */
	RAVEN_API void Rplot_set_series_visible(Rid plot, int series, int visible);
	RAVEN_API int Rplot_is_series_visible(Rid plot, int series);

	/** @brief The name written along an axis. */
	RAVEN_API void Rplot_set_axis_label(Rid plot, RPlotAxis axis, const char* label);
	RAVEN_API const char* Rplot_get_axis_label(Rid plot, RPlotAxis axis);
	RAVEN_API void Rplot_set_axis_scale(Rid plot, RPlotAxis axis, RPlotScale scale);
	RAVEN_API RPlotScale Rplot_get_axis_scale(Rid plot, RPlotAxis axis);
	/** @brief R_PLOT_SCALE_SYMLOG: the axis is linear from -threshold to threshold (default 1). */
	RAVEN_API void Rplot_set_axis_linear_threshold(Rid plot, RPlotAxis axis, double threshold);
	/** @brief Shows this range of an axis; the axis then stops following the data. */
	RAVEN_API void Rplot_set_axis_range(Rid plot, RPlotAxis axis, double min, double max);
	/** @brief The range an axis shows now (after the user moved or zoomed it). */
	RAVEN_API void Rplot_get_axis_range(Rid plot, RPlotAxis axis, double* min, double* max);
	/** @brief The axis is fitted to the data whenever it changes (the default, until the view is moved; a double click turns it on again). */
	RAVEN_API void Rplot_set_axis_auto_fit(Rid plot, RPlotAxis axis, int auto_fit);
	RAVEN_API int Rplot_is_axis_auto_fit(Rid plot, RPlotAxis axis);
	/** @brief How the labels of an axis are written: a printf format for one double ("%.1f s", "%g"), NULL: chosen from the ticks. */
	RAVEN_API void Rplot_set_axis_format(Rid plot, RPlotAxis axis, const char* format);
	/** @brief 1: shown, 0: hidden, -1: R_PLOT_X and R_PLOT_Y always, the second axes when something uses them (the default). */
	RAVEN_API void Rplot_set_axis_visible(Rid plot, RPlotAxis axis, int visible);
	/** @brief Lines across the data at the ticks of an axis (on for R_PLOT_X and R_PLOT_Y). */
	RAVEN_API void Rplot_set_axis_grid(Rid plot, RPlotAxis axis, int grid);
	/** @brief The values grow the other way (to the left, or down). */
	RAVEN_API void Rplot_set_axis_inverted(Rid plot, RPlotAxis axis, int inverted);
	/** @brief The user cannot move or zoom this axis (the application still can). */
	RAVEN_API void Rplot_set_axis_locked(Rid plot, RPlotAxis axis, int locked);
	/** @brief Fits every axis that is not locked to the data, and has them follow it again (what a double click does). */
	RAVEN_API void Rplot_fit(Rid plot);

	/** @brief Where the legend is (default R_PLOT_LEGEND_TOP_RIGHT). */
	RAVEN_API void Rplot_set_legend(Rid plot, RPlotLegend legend);
	/** @brief Lines across the data through the pointer. */
	RAVEN_API void Rplot_set_crosshair(Rid plot, int crosshair);
	/** @brief The coordinates of the pointer in a corner of the data (on by default). */
	RAVEN_API void Rplot_set_mouse_text(Rid plot, int shown);
	/** @brief The values of the point under the pointer, written next to it (on by default). */
	RAVEN_API void Rplot_set_tooltips(Rid plot, int shown);
	/** @brief Where the pointer is, in the units of R_PLOT_X and R_PLOT_Y. Returns R_TRUE when it is over the data. */
	RAVEN_API int Rplot_get_mouse_position(Rid plot, double* x, double* y);
	/** @brief Where a point of the data (R_PLOT_X, R_PLOT_Y) is in the widget, in logical units from its corner. */
	RAVEN_API void Rplot_data_to_point(Rid plot, double x, double y, float* point_x, float* point_y);
	/** @brief The data (R_PLOT_X, R_PLOT_Y) at a point of the widget, in logical units from its corner. */
	RAVEN_API void Rplot_point_to_data(Rid plot, float point_x, float point_y, double* x, double* y);

	/**
	 * @brief Adds a line across the data that the user drags: vertical at an X
	 * (axis R_PLOT_X or R_PLOT_X2), horizontal at a Y (R_PLOT_Y or R_PLOT_Y2).
	 * Its value is written on it, after `label` (may be NULL). Returns its index.
	 */
	RAVEN_API int Rplot_add_drag_line(Rid plot, RPlotAxis axis, double value, const char* label);
	RAVEN_API void Rplot_remove_drag_line(Rid plot, int line);
	RAVEN_API int Rplot_get_drag_line_count(Rid plot);
	RAVEN_API void Rplot_set_drag_line_value(Rid plot, int line, double value);
	RAVEN_API double Rplot_get_drag_line_value(Rid plot, int line);
	/** @brief The color of a drag line (by default the accent of the theme). */
	RAVEN_API void Rplot_set_drag_line_color(Rid plot, int line, float r, float g, float b, float a);

	/** @brief Called when the user moves, zooms or fits the view (Rplot_get_axis_range tells where it is). */
	typedef void (*RPlotViewCallback)(Rid plot, void* user_data);
	RAVEN_API void Rplot_set_view_changed_callback(Rid plot, RPlotViewCallback callback, void* user_data);
	/** @brief Called while the user drags a drag line. This also fires the action of the widget. */
	typedef void (*RPlotDragLineCallback)(Rid plot, int line, double value, void* user_data);
	RAVEN_API void Rplot_set_drag_line_callback(Rid plot, RPlotDragLineCallback callback, void* user_data);
	/** @brief Called when the user hides or shows a series from the legend. */
	typedef void (*RPlotSeriesCallback)(Rid plot, int series, int visible, void* user_data);
	RAVEN_API void Rplot_set_series_toggled_callback(Rid plot, RPlotSeriesCallback callback, void* user_data);


	/* ============================================================================
	 *  CHART (the charts of dashboards: pie, donut, gauge, heatmap, sparkline)
	 *
	 * One widget, five kinds (RChartKind), each fed by its own calls:
	 *
	 *   R_CHART_PIE, R_CHART_DONUT   slices: Rchart_add_slice and the others
	 *   R_CHART_GAUGE                Rchart_set_range, Rchart_set_value, bands
	 *   R_CHART_HEATMAP              Rchart_set_grid, labels of the rows and columns
	 *   R_CHART_SPARKLINE            Rchart_set_values or Rchart_push_value
	 *
	 * The colors follow the theme: the slices take the accent then a palette
	 * of ten colors made for the dark and the light themes, the gauge and the
	 * sparkline the accent (Rchart_set_color), the heatmap goes from a tint of
	 * the accent to the accent. The values glide to their new place when they
	 * change. The pointer over a slice lifts it and tells its label, value and
	 * share; over a cell of a heatmap or a point of a sparkline, its value. A
	 * click on a slice selects it (and again, unselects it), a click on a cell
	 * calls the cell callback; both also fire the action of the widget.
	 * The background is left to what holds the chart (a tile, a panel).
	 *
	 * Slices, bands, rows, columns and values are counted from 0.
	 *
	 * XML:
	 *   <Chart Kind="Donut" Title="Browsers" Format="%.0f">
	 *       <Slice Label="Chrome" Value="64"/> <Slice Label="Safari" Value="19" Color="#ff9f0a"/>
	 *   </Chart>
	 *   <Chart Kind="Gauge" Min="0" Max="100" Value="72" Format="%.0f%%">
	 *       <Band From="0" To="60"/> <Band From="60" To="85"/> <Band From="85" To="100"/>
	 *   </Chart>
	 *   <Chart Kind="Heatmap" ColumnLabels="0|6|12|18"> <Row Label="Mon" Values="1 4 2 0"/> ... </Chart>
	 *   <Chart Kind="Sparkline" Values="3 5 4 6 8 7 9"/>
	 * ============================================================================ */

	/** @brief A chart of a kind (Rwidget_create(R_WIDGET_CHART) makes a pie). */
	RAVEN_API Rid Rchart_create(RChartKind kind);
	RAVEN_API void Rchart_set_kind(Rid chart, RChartKind kind);
	RAVEN_API RChartKind Rchart_get_kind(Rid chart);
	/** @brief The title over a pie, a donut or a heatmap, under the value of a gauge (a sparkline only tells it to screen readers). */
	RAVEN_API void Rchart_set_title(Rid chart, const char* title);
	RAVEN_API const char* Rchart_get_title(Rid chart);
	/**
	 * @brief How the values are written (the gauge, the middle of a donut, what
	 * the pointer shows): a printf format for one double with its unit, "%.0f%%",
	 * "$%.2f". NULL: whole numbers as they are, others with up to two decimals.
	 */
	RAVEN_API void Rchart_set_format(Rid chart, const char* format);
	/** @brief The color of the arc of a gauge outside any band, and of the line of a sparkline (default: the accent). */
	RAVEN_API void Rchart_set_color(Rid chart, float r, float g, float b, float a);
	/** @brief The legend: the slices beside a pie or a donut, the scale of colors under a heatmap (shown by default). */
	RAVEN_API void Rchart_set_legend(Rid chart, int shown);
	/** @brief Removes the slices, the bands, the grid and the values. */
	RAVEN_API void Rchart_clear(Rid chart);
	/**
	 * @brief What is under the pointer: a slice, a point of a sparkline, or a
	 * cell of a heatmap as row * columns + column; -1 for nothing.
	 */
	RAVEN_API int Rchart_get_hovered(Rid chart);

	/** @brief Adds a slice (a value of 0 or more). Returns its index, -1 when the value is not valid. */
	RAVEN_API int Rchart_add_slice(Rid chart, const char* label, double value);
	RAVEN_API void Rchart_remove_slice(Rid chart, int slice);
	RAVEN_API int Rchart_get_slice_count(Rid chart);
	RAVEN_API void Rchart_set_slice_value(Rid chart, int slice, double value);
	RAVEN_API double Rchart_get_slice_value(Rid chart, int slice);
	RAVEN_API void Rchart_set_slice_label(Rid chart, int slice, const char* label);
	RAVEN_API const char* Rchart_get_slice_label(Rid chart, int slice);
	/** @brief The color of a slice; an alpha of 0 gives it back its color of the palette. */
	RAVEN_API void Rchart_set_slice_color(Rid chart, int slice, float r, float g, float b, float a);
	/** @brief The sum of the values of the slices. */
	RAVEN_API double Rchart_get_total(Rid chart);
	/** @brief The slice lifted as selected, or -1 for none (the user selects one with a click). */
	RAVEN_API void Rchart_set_selected(Rid chart, int slice);
	RAVEN_API int Rchart_get_selected(Rid chart);
	/** @brief The hole of a donut, as a part of its radius, from 0.2 to 0.9 (default 0.62). */
	RAVEN_API void Rchart_set_hole(Rid chart, float ratio);
	/**
	 * @brief What the middle of a donut shows when the pointer is not over a
	 * slice: `text` in large (NULL or "": the total) over `caption` in small
	 * (NULL or "": none).
	 */
	RAVEN_API void Rchart_set_center_text(Rid chart, const char* text, const char* caption);
	/** @brief Called when the user clicks a slice; `selected` tells whether it is now the selected one. */
	typedef void (*RChartSliceCallback)(Rid chart, int slice, int selected, void* user_data);
	RAVEN_API void Rchart_set_slice_clicked_callback(Rid chart, RChartSliceCallback callback, void* user_data);

	/**
	 * @brief The range of a gauge (default 0 to 100), and a fixed range for the
	 * colors of a heatmap and the height of a sparkline, which otherwise follow
	 * their values. min == max goes back to these defaults.
	 */
	RAVEN_API void Rchart_set_range(Rid chart, double min, double max);
	/** @brief The range in use now (a gauge's, or the one of the values of a heatmap or a sparkline). */
	RAVEN_API void Rchart_get_range(Rid chart, double* min, double* max);
	/** @brief The value of a gauge, kept within its range when drawn. */
	RAVEN_API void Rchart_set_value(Rid chart, double value);
	RAVEN_API double Rchart_get_value(Rid chart);
	/**
	 * @brief Colors a part of the range of a gauge (an alpha of 0: the success,
	 * warning and danger colors of the theme, one after the other). The arc
	 * takes the color of the band its value is in. Returns the index of the band.
	 */
	RAVEN_API int Rchart_add_band(Rid chart, double from, double to, float r, float g, float b, float a);
	RAVEN_API void Rchart_clear_bands(Rid chart);
	RAVEN_API int Rchart_get_band_count(Rid chart);

	/**
	 * @brief The cells of a heatmap: `rows` x `columns` values, row after row,
	 * copied (NULL: all 0). NaN leaves a cell empty.
	 */
	RAVEN_API void Rchart_set_grid(Rid chart, int rows, int columns, const float* values);
	RAVEN_API int Rchart_get_row_count(Rid chart);
	RAVEN_API int Rchart_get_column_count(Rid chart);
	RAVEN_API void Rchart_set_cell(Rid chart, int row, int column, float value);
	RAVEN_API float Rchart_get_cell(Rid chart, int row, int column);
	/** @brief The names of the rows, separated by '|': "Mon|Tue|Wed" (shown at their left). */
	RAVEN_API void Rchart_set_row_labels(Rid chart, const char* labels);
	/** @brief The names of the columns, separated by '|' (shown over them, as many as fit). */
	RAVEN_API void Rchart_set_column_labels(Rid chart, const char* labels);
	/** @brief The name of a row or of a column, "" when it has none. */
	RAVEN_API const char* Rchart_get_row_label(Rid chart, int row);
	RAVEN_API const char* Rchart_get_column_label(Rid chart, int column);
	/** @brief The colors of the lowest and of the highest values of a heatmap (default: a tint of the accent, the accent). */
	RAVEN_API void Rchart_set_color_scale(Rid chart, float low_r, float low_g, float low_b, float low_a,
		float high_r, float high_g, float high_b, float high_a);
	/** @brief Called when the user clicks a cell of a heatmap. */
	typedef void (*RChartCellCallback)(Rid chart, int row, int column, float value, void* user_data);
	RAVEN_API void Rchart_set_cell_clicked_callback(Rid chart, RChartCellCallback callback, void* user_data);

	/** @brief The values of a sparkline, copied. NaN leaves a gap. */
	RAVEN_API void Rchart_set_values(Rid chart, const float* values, int count);
	/** @brief Adds a value after the others, for live data; only the last `max_count` are kept (0: all of them). */
	RAVEN_API void Rchart_push_value(Rid chart, float value, int max_count);
	RAVEN_API int Rchart_get_value_count(Rid chart);
	RAVEN_API float Rchart_get_value_at(Rid chart, int index);
	/** @brief Shades the area under the line of a sparkline (on by default). */
	RAVEN_API void Rchart_set_fill(Rid chart, int fill);
	/** @brief A dot on the last value of a sparkline (on by default). */
	RAVEN_API void Rchart_set_marker(Rid chart, int shown);


	/* ============================================================================
	 *  PDF VIEW (the pages of a PDF document, scrolled and zoomed)
	 *
	 * The pages are drawn by PDFium, the PDF library of Chromium (BSD-3 /
	 * Apache-2.0), opened at run time when the first document is loaded: it
	 * is not needed to build Raven. It is looked for at the path in the
	 * environment variable RAVEN_PDFIUM, else next to the executable, else
	 * where the system finds libraries: pdfium.dll (Windows),
	 * libpdfium.dylib (macOS), libpdfium.so (Linux). Builds of it for every
	 * system: https://github.com/bblanchon/pdfium-binaries. Without it the
	 * view says that it cannot show the pages. Not in the browser backend.
	 *
	 * The pages show one under the other; the wheel and the keys scroll,
	 * Ctrl + wheel, Ctrl + = and Ctrl + - zoom (Ctrl + 0: fit the width),
	 * the pages can be dragged with the mouse and pinched with fingers.
	 * Pages are counted from 0. Sizes are in points (1/72 inch); at a zoom of
	 * 1, a page shows at its printed size (96 logical units per inch).
	 *
	 * XML: <PdfView Source="report.pdf" Fit="Width|Page|None" Zoom="1.5"
	 * Page="1" Password="..."/> (Page counted from 1 there, as it is shown).
	 * ============================================================================ */

	/** @brief R_TRUE when PDFium can be loaded (it is, the first time this is asked). */
	RAVEN_API int Rpdf_is_available(void);
	/**
	 * @brief Shows a PDF file. `password` may be NULL. Returns R_FALSE when it
	 * cannot: Rpdf_view_get_status tells why (a password is needed, not a
	 * PDF, no PDFium). The view then shows that instead of pages.
	 */
	RAVEN_API int Rpdf_view_load_file(Rid view, const char* path, const char* password);
	/** @brief Same as Rpdf_view_load_file, from bytes in memory (copied). */
	RAVEN_API int Rpdf_view_load_memory(Rid view, const char* data, size_t data_size, const char* password);
	/** @brief Closes the document: the view is empty. */
	RAVEN_API void Rpdf_view_close(Rid view);
	RAVEN_API RPdfStatus Rpdf_view_get_status(Rid view);
	RAVEN_API int Rpdf_view_get_page_count(Rid view);
	/** @brief Size of a page in points (1/72 inch), as it is shown (its rotation applied). */
	RAVEN_API void Rpdf_view_get_page_size(Rid view, int page, float* width, float* height);
	/** @brief Scrolls to the top of a page; it becomes the current page. */
	RAVEN_API void Rpdf_view_set_page(Rid view, int page);
	/** @brief The current page: the one a third down the view. */
	RAVEN_API int Rpdf_view_get_page(Rid view);
	/** @brief A zoom of its own (1: the printed size): the fit becomes R_PDF_FIT_NONE. */
	RAVEN_API void Rpdf_view_set_zoom(Rid view, float zoom);
	/** @brief The zoom the pages show at, the one a fit gives included. */
	RAVEN_API float Rpdf_view_get_zoom(Rid view);
	RAVEN_API void Rpdf_view_set_fit(Rid view, RPdfFit fit);
	RAVEN_API RPdfFit Rpdf_view_get_fit(Rid view);
	/** @brief The title of the document (its information), "" when it has none. */
	RAVEN_API const char* Rpdf_view_get_title(Rid view);
	/**
	 * @brief The text of a page (UTF-8), to search it or copy it. Valid until
	 * the next call for this view.
	 */
	RAVEN_API const char* Rpdf_view_get_page_text(Rid view, int page);
	/** @brief Called when the current page changes as the user scrolls. This also fires the action of the widget. */
	typedef void (*RPdfPageChangedCallback)(Rid view, int page, void* user_data);
	RAVEN_API void Rpdf_view_set_page_changed_callback(Rid view, RPdfPageChangedCallback callback, void* user_data);


	/* ============================================================================
	 *  CAROUSEL (pages shown one at a time and swiped: a page view)
	 *
	 * Every child is a page (Rwidget_add_child), the size of the carousel.
	 * One page shows at a time; the pages are swiped side by side (or one
	 * above the other, Rcarousel_set_orientation). They follow a drag of the
	 * mouse, a finger and two fingers on a trackpad as if they were held,
	 * stretch before the first page and after the last one, and settle on
	 * the nearest page when let go, or on the next one when flicked. The
	 * arrow keys (Left / Right, Up / Down when vertical), Page Up / Page
	 * Down, Home and End, a click on a dot, the round arrows shown under the
	 * pointer and the mouse wheel go to a page with a glide. Only the page
	 * shown (and the one coming in while the pages move) is painted and can
	 * be reached by the mouse and the keyboard.
	 *
	 * Pages are counted from 0. In a right-to-left layout the pages of a
	 * horizontal carousel run from right to left.
	 *
	 * XML: <Carousel Page="0" Loop="true" Autoplay="5" Indicators="true"
	 * Arrows="true" Orientation="Horizontal|Vertical" OnChanged="...">
	 * followed by the pages, one element each.
	 * ============================================================================ */

	RAVEN_API Rid Rcarousel_create(void);
	/**
	 * @brief Shows a page, gliding to it when `animated` (and the animations
	 * are on), at once otherwise. The page changed callback is not called.
	 */
	RAVEN_API void Rcarousel_set_page(Rid carousel, int page, int animated);
	/** @brief The page shown, -1 when the carousel has none. */
	RAVEN_API int Rcarousel_get_page(Rid carousel);
	/** @brief Its children. */
	RAVEN_API int Rcarousel_get_page_count(Rid carousel);
	/** @brief Glides to the next / previous page; nothing at the last / first one unless it loops. */
	RAVEN_API void Rcarousel_next(Rid carousel);
	RAVEN_API void Rcarousel_prev(Rid carousel);
	/** @brief R_TRUE: after the last page comes the first one, and before the first the last (default R_FALSE). */
	RAVEN_API void Rcarousel_set_loop(Rid carousel, int loop);
	RAVEN_API int Rcarousel_get_loop(Rid carousel);
	/**
	 * @brief Goes to the next page by itself every `seconds` (0: never, the
	 * default); after the last page it comes back to the first. It waits
	 * while the pointer is on the carousel, while the pages are dragged and
	 * while the keyboard is in it; a page chosen by the user starts the
	 * interval again.
	 */
	RAVEN_API void Rcarousel_set_autoplay(Rid carousel, double seconds);
	RAVEN_API double Rcarousel_get_autoplay(Rid carousel);
	/** @brief The dots along the bottom edge (the right edge when vertical), one per page (default R_TRUE). */
	RAVEN_API void Rcarousel_set_indicators_visible(Rid carousel, int visible);
	RAVEN_API int Rcarousel_get_indicators_visible(Rid carousel);
	/** @brief The previous / next buttons shown while the pointer is on the carousel (default R_TRUE). */
	RAVEN_API void Rcarousel_set_arrows_visible(Rid carousel, int visible);
	RAVEN_API int Rcarousel_get_arrows_visible(Rid carousel);
	/** @brief R_HORIZONTAL: the pages side by side (default); R_VERTICAL: one above the other. */
	RAVEN_API void Rcarousel_set_orientation(Rid carousel, ROrientation orientation);
	RAVEN_API ROrientation Rcarousel_get_orientation(Rid carousel);
	/**
	 * @brief Called when the user goes to another page (a swipe, the keys, a
	 * dot, an arrow) and when the autoplay does. This also fires the action
	 * of the widget.
	 */
	typedef void (*RCarouselPageChangedCallback)(Rid carousel, int page, void* user_data);
	RAVEN_API void Rcarousel_set_page_changed_callback(Rid carousel, RCarouselPageChangedCallback callback, void* user_data);
	/** @brief Prints the document shown (see Rpdf_print); the job is named after its title. */
	RAVEN_API int Rpdf_view_print(Rid view, const RPdfPrintDesc* desc);


	/* ============================================================================
	 *  MAP VIEW (a map of slippy-map tiles, panned and zoomed)
	 *
	 * The square tiles of the Web Mercator projection that OpenStreetMap and
	 * most tile servers use: at zoom z the world is 2^z x 2^z tiles of 256
	 * logical units (Rmap_view_set_tile_size), x going east from longitude
	 * -180, y going south from latitude 85.0511 (the limit of the projection).
	 * Raven fetches nothing from the network: the application gives the
	 * tiles, from a server, a disk cache or drawn by itself.
	 *
	 * When a tile is needed, the tile callback asks for it once; the answer
	 * comes then or later, from any thread: the pixels
	 * (Rmap_view_set_tile), the bytes of a PNG / JPEG file
	 * (Rmap_view_set_tile_data, decoded on the calling thread), or that there
	 * is none (Rmap_view_set_tile_failed). Only the tiles seen, and one more
	 * around them, are asked for; a tile that is no longer wanted is
	 * forgotten (the cancel callback tells it) and an answer for it that
	 * comes later is dropped. Rmap_view_set_tile_path reads the tiles from
	 * files instead ("tiles/{z}/{x}/{y}.png"), on a worker thread.
	 *
	 * The tiles are kept in a cache of bounded size (Rmap_view_set_cache_size),
	 * the least recently seen dropped first. While a tile is missing, a
	 * coarser one that is there is shown stretched (and finer ones shrunk),
	 * else a faint grid. Between two whole zooms the tiles are scaled.
	 *
	 * The user drags the map (it glides on when flung), zooms around the
	 * pointer with the wheel, a pinch or Ctrl + the wheel of a trackpad (two
	 * fingers move it), zooms in with a double click (out with Shift), and
	 * uses + / - and the arrows when it has the focus. Longitudes wrap
	 * around; latitudes stop at the limits of the projection.
	 *
	 * Points of the widget are in logical units from its top left corner.
	 * Latitudes and longitudes are in degrees.
	 *
	 * XML: <MapView Latitude="48.85" Longitude="2.35" Zoom="5" MinZoom="2"
	 * MaxZoom="18" TilePath="tiles/{z}/{x}/{y}.png" TileSize="256"
	 * Attribution="(c) OpenStreetMap contributors" OnChanged="map.moved">
	 * with <Marker Latitude Longitude Label Color/> elements.
	 * ============================================================================ */

	RAVEN_API Rid Rmap_view_create(void);
	/** @brief Centers the map on a place. The latitude is held within +/-85.0511; the longitude wraps. */
	RAVEN_API void Rmap_view_set_center(Rid map, double latitude, double longitude);
	/** @brief The place at the middle of the map (longitude from -180 to 180). */
	RAVEN_API void Rmap_view_get_center(Rid map, double* latitude, double* longitude);
	/** @brief The zoom: 0 shows the whole world in one tile, each step doubles the scale. Fractions are allowed. */
	RAVEN_API void Rmap_view_set_zoom(Rid map, double zoom);
	RAVEN_API double Rmap_view_get_zoom(Rid map);
	/** @brief The zooms the user can reach (default 0 to 19, at most 0 to 24). Tiles are asked for at whole zooms of this range. */
	RAVEN_API void Rmap_view_set_zoom_range(Rid map, double min_zoom, double max_zoom);
	RAVEN_API void Rmap_view_get_zoom_range(Rid map, double* min_zoom, double* max_zoom);
	/** @brief Glides to a place and a zoom (at once when animations are off). */
	RAVEN_API void Rmap_view_animate_to(Rid map, double latitude, double longitude, double zoom);
	/** @brief The place at a point of the widget. */
	RAVEN_API void Rmap_view_screen_to_latlon(Rid map, float x, float y, double* latitude, double* longitude);
	/** @brief Where a place is in the widget (of the copies of the world side by side, the one nearest the middle). */
	RAVEN_API void Rmap_view_latlon_to_screen(Rid map, double latitude, double longitude, float* x, float* y);

	/** @brief Asks the application for a tile (z, x, y), or tells it that a tile asked for is no longer wanted. */
	typedef void (*RMapTileCallback)(Rid map, int z, int x, int y, void* user_data);
	/**
	 * @brief Called once for each tile the map needs. Answer with
	 * Rmap_view_set_tile, Rmap_view_set_tile_data or Rmap_view_set_tile_failed,
	 * at once or later. Setting it forgets the tiles of the previous source.
	 */
	RAVEN_API void Rmap_view_set_tile_callback(Rid map, RMapTileCallback callback, void* user_data);
	/** @brief Called when a tile asked for and not answered yet is no longer wanted (the user went elsewhere). Optional. */
	RAVEN_API void Rmap_view_set_tile_cancel_callback(Rid map, RMapTileCallback callback, void* user_data);
	/**
	 * @brief The pixels of a tile: `width` x `height` RGBA8, tightly packed
	 * (they are copied; any size, shown at the tile size). Thread-safe: from
	 * another thread it is applied on the UI thread during the next frame.
	 */
	RAVEN_API void Rmap_view_set_tile(Rid map, int z, int x, int y, const uint8_t* rgba, int width, int height);
	/** @brief A tile as the bytes of an image file (PNG, JPEG...), decoded on the calling thread. Thread-safe. */
	RAVEN_API void Rmap_view_set_tile_data(Rid map, int z, int x, int y, const void* data, size_t data_size);
	/** @brief There is no such tile: the coarser one stays shown in its place. Thread-safe. */
	RAVEN_API void Rmap_view_set_tile_failed(Rid map, int z, int x, int y);
	/**
	 * @brief Reads the tiles from files instead of asking for them: a path
	 * where {z}, {x} and {y} are replaced ({-y}: the row counted from the
	 * south, as in TMS and MBTiles). The files are read and decoded on a
	 * worker thread; a missing file is a tile that failed. NULL or "": the
	 * tile callback again. Local files only.
	 */
	RAVEN_API void Rmap_view_set_tile_path(Rid map, const char* path_template);
	RAVEN_API const char* Rmap_view_get_tile_path(Rid map);
	/** @brief Logical size of a tile at a whole zoom (default 256, from 16 to 1024): larger tiles show the map larger. */
	RAVEN_API void Rmap_view_set_tile_size(Rid map, int size);
	RAVEN_API int Rmap_view_get_tile_size(Rid map);
	/** @brief Memory the decoded tiles may take, in bytes (default 64 MiB). The tiles on screen are always kept. */
	RAVEN_API void Rmap_view_set_cache_size(Rid map, size_t bytes);
	/** @brief Memory the decoded tiles take now, in bytes. */
	RAVEN_API size_t Rmap_view_get_cache_usage(Rid map);
	/** @brief Forgets every tile (they are asked for again): the tiles of the source changed. */
	RAVEN_API void Rmap_view_clear_tiles(Rid map);
	RAVEN_API RMapTileState Rmap_view_get_tile_state(Rid map, int z, int x, int y);

	/** @brief Adds a pin at a place, with a label written next to it (may be NULL). Returns its id (from 1), or -1. */
	RAVEN_API int Rmap_view_add_marker(Rid map, double latitude, double longitude, const char* label);
	RAVEN_API void Rmap_view_remove_marker(Rid map, int marker);
	RAVEN_API void Rmap_view_clear_markers(Rid map);
	RAVEN_API int Rmap_view_get_marker_count(Rid map);
	RAVEN_API void Rmap_view_set_marker_position(Rid map, int marker, double latitude, double longitude);
	RAVEN_API void Rmap_view_set_marker_label(Rid map, int marker, const char* label);
	/** @brief The color of a pin (by default the accent of the theme). */
	RAVEN_API void Rmap_view_set_marker_color(Rid map, int marker, float r, float g, float b, float a);
	/** @brief Called when the user clicks a marker. */
	typedef void (*RMapMarkerCallback)(Rid map, int marker, void* user_data);
	RAVEN_API void Rmap_view_set_marker_clicked_callback(Rid map, RMapMarkerCallback callback, void* user_data);

	/**
	 * @brief Called when the user moves or zooms the map (during a drag, at the
	 * end of an animated zoom), with its new center and zoom. This also fires
	 * the action of the widget.
	 */
	typedef void (*RMapViewCallback)(Rid map, double latitude, double longitude, double zoom, void* user_data);
	RAVEN_API void Rmap_view_set_view_changed_callback(Rid map, RMapViewCallback callback, void* user_data);
	/**
	 * @brief A line written in the bottom right corner: the credit most tile
	 * licences ask for, such as "(c) OpenStreetMap contributors". NULL or "": none.
	 */
	RAVEN_API void Rmap_view_set_attribution(Rid map, const char* text);
	RAVEN_API const char* Rmap_view_get_attribution(Rid map);


	/* ============================================================================
	 *  SYSTEM WEB VIEW (the web engine of the system, embedded in the window)
	 *
	 * Web pages shown by the engine that comes with the system, in a view of
	 * its own kept over the widget: WebKit (WKWebView) on macOS and iOS,
	 * Microsoft Edge WebView2 on Windows (when its runtime is installed: it
	 * is found at run time, nothing is linked). Pages are drawn, scrolled
	 * and typed into by the system, as in its browser; the engine is opened
	 * when the first view is made, so a program that has none does not load
	 * it.
	 *
	 * Like a native child window (Rviewport_set_native), the page is not
	 * drawn by Raven: it follows the widget (its place, its size, whether it
	 * is shown, the part of it a ScrollBox lets be seen), but Raven cannot
	 * draw over it: popups, menus and tooltips that cross it go under it.
	 *
	 * Elsewhere (Linux, the browser, Android, Windows without the WebView2
	 * Runtime, and without a window system) Rsystem_web_view_is_supported is
	 * R_FALSE. The widget exists all
	 * the same, so programs and XML stay the same everywhere: it keeps the
	 * address it is given and shows a message instead of the page; scripts
	 * get an error. raven_web.h shows pages with an installed browser of the
	 * Chromium family on those systems.
	 *
	 * The page talks to the application with
	 * `window.raven.postMessage(value)` (Rsystem_web_view_set_message_callback):
	 * a string as it is, anything else written as JSON. Every page the view
	 * shows can call it: check what arrives as you would any input.
	 *
	 * Everything here is called from the UI thread; the callbacks come on it.
	 *
	 * XML: <SystemWebView Url="https://example.org" OnChanged="..."/>, or
	 * <SystemWebView Html="&lt;h1&gt;Hello&lt;/h1&gt;" BaseUrl="file:///..."/>
	 * ============================================================================ */

	/** @return R_TRUE when web views show pages here (the system has an engine Raven can embed, and Raven runs with a window system). */
	RAVEN_API int Rsystem_web_view_is_supported(void);
	RAVEN_API Rid Rsystem_web_view_create(void);
	/** @brief Goes to an address ("https://...", "file:///...", "about:blank"). A file: address may read the files of its folder. */
	RAVEN_API void Rsystem_web_view_load_url(Rid view, const char* url);
	/** @brief Shows a file of the computer (an .html file, and what it refers to in its folder). */
	RAVEN_API void Rsystem_web_view_load_file(Rid view, const char* path);
	/**
	 * @brief Shows HTML given as text.
	 * @param base_url Where the relative addresses of the text are looked
	 * for ("file:///Users/me/app/pages/", "https://example.org/"), or NULL.
	 */
	RAVEN_API void Rsystem_web_view_load_html(Rid view, const char* html, const char* base_url);
	RAVEN_API void Rsystem_web_view_reload(Rid view);
	RAVEN_API void Rsystem_web_view_stop(Rid view);
	RAVEN_API void Rsystem_web_view_go_back(Rid view);
	RAVEN_API void Rsystem_web_view_go_forward(Rid view);
	RAVEN_API int Rsystem_web_view_can_go_back(Rid view);
	RAVEN_API int Rsystem_web_view_can_go_forward(Rid view);
	/** @return The address of the page shown (the one asked for, where unsupported). Valid until it changes. */
	RAVEN_API const char* Rsystem_web_view_get_url(Rid view);
	/** @return The title of the page, "" when it has none. Valid until it changes. */
	RAVEN_API const char* Rsystem_web_view_get_title(Rid view);
	RAVEN_API int Rsystem_web_view_is_loading(Rid view);
	/** @return Why the last page could not be shown, or "" when all is well. */
	RAVEN_API const char* Rsystem_web_view_get_error(Rid view);

	/**
	 * @brief Receives the result of a script: `result` is its value written
	 * as JSON ("42", "\"text\"", "{\"a\":1}", "null"), or NULL when the
	 * script failed, `error` then telling why.
	 */
	typedef void (*RSystemWebViewResultCallback)(Rid view, const char* result, const char* error, void* user_data);
	/**
	 * @brief Runs JavaScript in the page; the value of its last statement is
	 * the result. If it is a promise, the result is what the promise
	 * resolves to. The callback (may be NULL) always comes later, never
	 * during this call.
	 */
	RAVEN_API void Rsystem_web_view_run_javascript(Rid view, const char* script, RSystemWebViewResultCallback callback, void* user_data);
	/** @brief Receives what the page sends with `window.raven.postMessage(value)`. */
	typedef void (*RSystemWebViewMessageCallback)(Rid view, const char* message, void* user_data);
	RAVEN_API void Rsystem_web_view_set_message_callback(Rid view, RSystemWebViewMessageCallback callback, void* user_data);
	typedef void (*RSystemWebViewEventCallback)(Rid view, RSystemWebViewEvent event, void* user_data);
	/**
	 * @brief Called when something happens in the page. The action of the
	 * widget (Rwidget_set_action, OnChanged in XML) is fired as well each
	 * time the address changes.
	 */
	RAVEN_API void Rsystem_web_view_set_event_callback(Rid view, RSystemWebViewEventCallback callback, void* user_data);
	/**
	 * @brief The view of the system, to go further than these functions:
	 * a WKWebView* on macOS and iOS, an ICoreWebView2* on Windows (NULL
	 * until WebView2 has made the view, a moment after it is created). NULL
	 * where unsupported.
	 */
	RAVEN_API void* Rsystem_web_view_get_native_handle(Rid view);



	/* ============================================================================
	 *  PDF DOCUMENTS (written by the application: reports, invoices, tickets...)
	 *
	 * A document is a list of pages, each drawn with text, shapes and
	 * pictures, then saved to a file or to memory: Rpdf_view_load_memory
	 * shows it, Rpdf_document_print prints it. Nothing outside Raven is needed
	 * (PDFium only shows and prints documents).
	 *
	 * Sizes and positions are in points (1/72 inch) from the top left corner
	 * of the page, y going down. A4 is 595.28 x 841.89 points, US Letter 612 x
	 * 792. Drawing goes to the last page added (an A4 page is added when
	 * there is none). Colors are r, g, b, a from 0 to 1.
	 *
	 * The text is set in a font embedded in the file: Roboto (R_PDF_FONT_SANS)
	 * and Roboto Mono (R_PDF_FONT_MONO) as Raven has them (the Latin letters
	 * with their accents, punctuation, the euro sign), or a TrueType font
	 * added with Rpdf_document_add_font for other scripts; a character the
	 * font lacks shows as an empty box. The text of the file can be searched
	 * and copied.
	 *
	 *   Rid doc = Rpdf_document_create();
	 *   Rpdf_document_add_page(doc, 595.28f, 841.89f);
	 *   Rpdf_document_set_font(doc, R_PDF_FONT_SANS, 24, R_PDF_TEXT_BOLD);
	 *   Rpdf_document_draw_text(doc, 56, 56, "Invoice 2026-114");
	 *   Rpdf_document_save(doc, "invoice.pdf");
	 *   Rpdf_document_delete(doc);
	 * ============================================================================ */

	RAVEN_API Rid Rpdf_document_create(void);
	RAVEN_API void Rpdf_document_delete(Rid document);
	/** @brief The information of the document, shown by readers (the title in their window). */
	RAVEN_API void Rpdf_document_set_title(Rid document, const char* title);
	RAVEN_API void Rpdf_document_set_author(Rid document, const char* author);
	RAVEN_API void Rpdf_document_set_subject(Rid document, const char* subject);
	/** @brief Adds a page of a size in points; drawing goes to it. Returns its index, -1 for an invalid size. */
	RAVEN_API int Rpdf_document_add_page(Rid document, float width, float height);
	RAVEN_API int Rpdf_document_get_page_count(Rid document);

	RAVEN_API void Rpdf_document_set_fill_color(Rid document, float r, float g, float b, float a);
	RAVEN_API void Rpdf_document_set_stroke_color(Rid document, float r, float g, float b, float a);
	/** @brief Width of the lines and outlines, in points (default 1). */
	RAVEN_API void Rpdf_document_set_line_width(Rid document, float width);
	/** @brief A rectangle, its corners rounded by `radius`; `paint` is a bitmask of RPdfPaint. */
	RAVEN_API void Rpdf_document_draw_rect(Rid document, float x, float y, float width, float height, float radius, Ruint paint);
	/** @brief The ellipse inside a rectangle. */
	RAVEN_API void Rpdf_document_draw_ellipse(Rid document, float x, float y, float width, float height, Ruint paint);
	/** @brief A line in the stroke color. */
	RAVEN_API void Rpdf_document_draw_line(Rid document, float x1, float y1, float x2, float y2);

	/**
	 * @brief Adds a TrueType font (.ttf) to the document (the data is
	 * copied, and embedded in the file). Returns its number for
	 * Rpdf_document_set_font, -1 when it is not a TrueType font.
	 */
	RAVEN_API int Rpdf_document_add_font(Rid document, const char* data, size_t data_size);
	RAVEN_API int Rpdf_document_add_font_from_file(Rid document, const char* path);
	/** @brief The font of the text drawn next: an RPdfFont or a number of Rpdf_document_add_font, a size in points, a bitmask of RPdfTextStyle. */
	RAVEN_API void Rpdf_document_set_font(Rid document, int font, float size, Ruint style);
	/** @brief Distance between two lines of text in the current font. */
	RAVEN_API float Rpdf_document_get_line_height(Rid document);
	/** @brief Width of a text in the current font, in points (of its widest line). */
	RAVEN_API float Rpdf_document_measure_text(Rid document, const char* text);
	/**
	 * @brief Text in the fill color, the top of its first line at y ('\n'
	 * starts a line below). Returns its width.
	 */
	RAVEN_API float Rpdf_document_draw_text(Rid document, float x, float y, const char* text);
	/**
	 * @brief Text wrapped at the spaces to fit `width`, each line placed by
	 * `align` (R_HALIGN_LEFT, R_HALIGN_CENTER or R_HALIGN_RIGHT). Returns the
	 * height it took.
	 */
	RAVEN_API float Rpdf_document_draw_paragraph(Rid document, float x, float y, float width, const char* text, RHAlign align);
	/**
	 * @brief A picture of RGBA8 pixels stretched over a rectangle. A width or
	 * height of 0 keeps the proportions of the picture; both 0: 96 pixels per inch.
	 */
	RAVEN_API void Rpdf_document_draw_image(Rid document, float x, float y, float width, float height, const uint8_t* rgba, int pixels_width, int pixels_height);
	/**
	 * @brief What a window shows (Rwindow_capture), as a picture: a report of
	 * the screen, a chart. A width and height of 0: the size it has on the
	 * screen. Returns R_FALSE when the window cannot be captured.
	 */
	RAVEN_API int Rpdf_document_draw_window(Rid document, Rid window, float x, float y, float width, float height);

	/** @brief Writes the file. Returns R_FALSE when it cannot be written. */
	RAVEN_API int Rpdf_document_save(Rid document, const char* path);
	/** @brief The file in memory (followed by a '\0' not counted), to release with Rmemory_free; `size` receives its length. */
	RAVEN_API char* Rpdf_document_save_memory(Rid document, size_t* size);

	/**
	 * @brief Prints a PDF document with the print system: on macOS the print
	 * panel, on Windows the print dialog (the pages are drawn by PDFium, see
	 * PDF VIEW), on Linux and the other systems CUPS (lp or lpr) after a
	 * dialog of Raven that chooses the printer. It waits until the job is
	 * sent. `desc` may be NULL. Returns R_FALSE when cancelled or when it
	 * cannot print. Not in the browser.
	 */
	RAVEN_API int Rpdf_print(const RPdfPrintDesc* desc, const char* data, size_t data_size);
	RAVEN_API int Rpdf_print_file(const RPdfPrintDesc* desc, const char* path);
	/** @brief Prints a document written by the application. */
	RAVEN_API int Rpdf_document_print(Rid document, const RPdfPrintDesc* desc);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RAVEN_CONTROLS_H */

/**
 * Raven
 * raven_accessibility.h : interfaces for everyone.
 *
 * Screen readers. Every window of Raven is exposed to the screen reader of
 * the system: UI Automation on Windows (Narrator, NVDA, JAWS), NSAccessibility
 * on macOS (VoiceOver), AT-SPI on Linux (Orca). Nothing is needed for that:
 * the widgets say what they are (a button, a check box, a slider, a list and
 * its items, a tab...), their name (the text they show, or the label just
 * before them), their value and their state, and they can be used from the
 * screen reader (press, toggle, set a value, expand). The focus is followed,
 * and the application can have a message read (Raccessibility_announce).
 * What a widget cannot know (an icon button, a drawing) is given with
 * Rwidget_set_accessible_name / _description / _role, or in XML with the
 * attributes AccessibleName, AccessibleDescription, AccessibleRole and
 * AccessibleHidden.
 *
 * Sight. The size of every text (Raccessibility_set_text_scale), themes of
 * high contrast, a focus that is always shown and thicker, animations that
 * can be stopped, and color filters for the color blind: a correction that
 * moves the colors they confuse to colors they tell apart, and simulations
 * for those who design for them. Raccessibility_follow_system takes the
 * settings of the system.
 *
 * Writings. Right-to-left languages (Arabic, Hebrew, Persian, Urdu) are
 * drawn in their order, with their letters joined, and the layout mirrors
 * itself for them (raven_layout.h, Rset_layout_direction). The characters
 * that the fonts of the application do not have (other alphabets, Chinese,
 * Japanese, Korean, symbols, emoji in color) are taken from the fonts of the
 * system. See raven_locale.h for translations.
 */

#ifndef RAVEN_ACCESSIBILITY_H
#define RAVEN_ACCESSIBILITY_H

#include "raven_core.h"

#ifdef __cplusplus
extern "C" {
#endif

	/* ============================================================================
	 *  Sight
	 * ============================================================================ */

	/** @brief Size of every text, 1 = as designed (0.5 to 4). Controls grow with their text. */
	RAVEN_API void Raccessibility_set_text_scale(float scale);
	RAVEN_API float Raccessibility_get_text_scale(void);

	/**
	 * @brief Themes of high contrast: black and white, yellow focus, thick
	 * borders (the dark one, or the light one when the theme in use is light).
	 * R_FALSE goes back to the theme used before.
	 */
	RAVEN_API void Raccessibility_set_high_contrast(int enabled);
	RAVEN_API int Raccessibility_get_high_contrast(void);

	/** @brief R_TRUE: the focus ring is always shown (not only after the keyboard was used), and thicker. */
	RAVEN_API void Raccessibility_set_focus_always_visible(int enabled);
	RAVEN_API int Raccessibility_get_focus_always_visible(void);

	/**
	 * @brief Reduced motion, for the whole application: R_TRUE and nothing
	 * moves by itself. Changes are immediate (no spring, no slide, no bounce,
	 * no transition), and the effects (raven_shader.h) stand still; they still
	 * follow the pointer. Same as Rset_animations_enabled(R_FALSE).
	 */
	RAVEN_API void Raccessibility_set_reduce_motion(int enabled);
	RAVEN_API int Raccessibility_get_reduce_motion(void);

	typedef enum RColorFilter {
		R_COLOR_FILTER_NONE = 0x0740,
		/** Corrections: the colors that cannot be told apart are moved to colors that can. */
		R_COLOR_FILTER_PROTANOPIA,      /* red is weak */
		R_COLOR_FILTER_DEUTERANOPIA,    /* green is weak (the most frequent) */
		R_COLOR_FILTER_TRITANOPIA,      /* blue is weak */
		R_COLOR_FILTER_GRAYSCALE,
		R_COLOR_FILTER_INVERT,
		/** Simulations, to see an interface as the color blind see it. */
		R_COLOR_FILTER_SIMULATE_PROTANOPIA,
		R_COLOR_FILTER_SIMULATE_DEUTERANOPIA,
		R_COLOR_FILTER_SIMULATE_TRITANOPIA,
		R_COLOR_FILTER_SIMULATE_ACHROMATOPSIA
	} RColorFilter;

	/** @brief A filter on the colors of the interface (pictures and viewports keep theirs). */
	RAVEN_API void Raccessibility_set_color_filter(RColorFilter filter);
	RAVEN_API RColorFilter Raccessibility_get_color_filter(void);

	/** What the system asks for. */
	typedef struct RAccessibilitySettings {
		/** Size of the texts asked by the user (1 = normal). */
		float text_scale;
		int high_contrast;
		int reduce_motion;
		/** R_TRUE when a screen reader is running. */
		int screen_reader;
	} RAccessibilitySettings;

	RAVEN_API void Raccessibility_get_system_settings(RAccessibilitySettings* settings);
	/** @brief Applies the settings of the system (text size, contrast, motion). */
	RAVEN_API void Raccessibility_follow_system(void);

	/* ============================================================================
	 *  Screen readers
	 * ============================================================================ */

	typedef enum RAccessibleRole {
		/** The role of the type of the widget (the default). */
		R_ROLE_AUTO = 0,
		/** Not exposed: its children are seen as the children of its parent. */
		R_ROLE_NONE = 0x0760,
		R_ROLE_WINDOW,
		R_ROLE_GROUP,
		R_ROLE_TEXT,
		R_ROLE_HEADING,
		R_ROLE_LINK,
		R_ROLE_BUTTON,
		R_ROLE_TOGGLE_BUTTON,
		R_ROLE_CHECKBOX,
		R_ROLE_RADIO_BUTTON,
		R_ROLE_SLIDER,
		R_ROLE_PROGRESS_BAR,
		R_ROLE_SPIN_BUTTON,
		R_ROLE_TEXT_FIELD,
		R_ROLE_DOCUMENT,
		R_ROLE_COMBO_BOX,
		R_ROLE_LIST,
		R_ROLE_LIST_ITEM,
		R_ROLE_TREE,
		R_ROLE_TREE_ITEM,
		R_ROLE_TABLE,
		R_ROLE_MENU_BAR,
		R_ROLE_MENU,
		R_ROLE_MENU_ITEM,
		R_ROLE_TAB_LIST,
		R_ROLE_TAB,
		R_ROLE_TAB_PANEL,
		R_ROLE_TOOLBAR,
		R_ROLE_STATUS_BAR,
		R_ROLE_DIALOG,
		R_ROLE_ALERT,
		R_ROLE_IMAGE,
		R_ROLE_CANVAS,
		R_ROLE_SEPARATOR,
		R_ROLE_SCROLL_BAR,
		R_ROLE_PANE
	} RAccessibleRole;

	/** @brief What the screen reader says of the widget; NULL: what it shows (or the label before it). */
	RAVEN_API void Rwidget_set_accessible_name(Rid widget, const char* name);
	/** @brief The name the screen reader uses (given, or found). Valid until the next call. */
	RAVEN_API const char* Rwidget_get_accessible_name(Rid widget);
	/** @brief A longer explanation, read after the name. */
	RAVEN_API void Rwidget_set_accessible_description(Rid widget, const char* description);
	RAVEN_API void Rwidget_set_accessible_role(Rid widget, RAccessibleRole role);
	/** @brief The role the screen reader sees (given, or the one of the type). */
	RAVEN_API RAccessibleRole Rwidget_get_accessible_role(Rid widget);
	/** @brief R_TRUE: hidden from screen readers, with what it contains (decoration). */
	RAVEN_API void Rwidget_set_accessible_hidden(Rid widget, int hidden);
	/** @brief The widget is named by another one (a label): its text is the name. */
	RAVEN_API void Rwidget_set_accessible_labelled_by(Rid widget, Rid label);

	typedef void (*RAccessiblePressCallback)(Rid widget, void* user_data);
	/**
	 * @brief Explicit screen-reader press action, including for custom widgets.
	 * NULL removes the override. Does not change pointer or keyboard handling.
	 * Disabled widgets (including disabled ancestors) cannot be pressed.
	 */
	RAVEN_API void Rwidget_set_accessible_action(Rid widget, RAccessiblePressCallback press, void* user_data);
	/**
	 * @brief Override the checked state: 1 checked, 0 unchecked, -1 restores the
	 * widget's default (not checkable for a custom widget). The application owns
	 * this state; a press callback updates it when appropriate.
	 */
	RAVEN_API void Rwidget_set_accessible_checked(Rid widget, int checked);

	/** @brief Has a message read (a save that ended, an error). `assertive`: interrupts what is being read. */
	RAVEN_API void Raccessibility_announce(const char* text, int assertive);
	/** @brief R_TRUE when a screen reader is reading the windows of the application. */
	RAVEN_API int Raccessibility_is_screen_reader_active(void);

#ifdef __cplusplus
}
#endif

#endif /* RAVEN_ACCESSIBILITY_H */

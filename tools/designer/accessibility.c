/*
 * Raven Designer - accessibility.c
 * View > Accessibility: every accessibility setting of Raven, live, in a
 * window of its own. It is also a showcase: what any application built with
 * Raven offers its users without writing a line for it.
 *
 *   text size              Raccessibility_set_text_scale
 *   interface size         Rset_ui_scale
 *   high contrast          Raccessibility_set_high_contrast
 *   color filters          Raccessibility_set_color_filter (corrections and simulations)
 *   focus ring             Raccessibility_set_focus_always_visible
 *   reduce motion          Raccessibility_set_reduce_motion
 *   animation speed        Rset_animation_speed
 *   screen readers         Raccessibility_is_screen_reader_active, Raccessibility_announce
 *   the system             Raccessibility_get_system_settings, Raccessibility_follow_system
 *
 * The settings are kept in designer.ini (a11y.*) and applied again at the next
 * start. View > Reduce motion switches the one most asked for from the menu.
 */

#include "designer.h"
#include "../crash_report.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const struct { const char* name; RColorFilter filter; } g_filters[] = {
	{ "None", R_COLOR_FILTER_NONE },
	{ "Protanopia (red is weak)", R_COLOR_FILTER_PROTANOPIA },
	{ "Deuteranopia (green is weak)", R_COLOR_FILTER_DEUTERANOPIA },
	{ "Tritanopia (blue is weak)", R_COLOR_FILTER_TRITANOPIA },
	{ "Grayscale", R_COLOR_FILTER_GRAYSCALE },
	{ "Inverted colors", R_COLOR_FILTER_INVERT },
	{ "Simulate protanopia", R_COLOR_FILTER_SIMULATE_PROTANOPIA },
	{ "Simulate deuteranopia", R_COLOR_FILTER_SIMULATE_DEUTERANOPIA },
	{ "Simulate tritanopia", R_COLOR_FILTER_SIMULATE_TRITANOPIA },
	{ "Simulate achromatopsia", R_COLOR_FILTER_SIMULATE_ACHROMATOPSIA }
};
#define FILTER_COUNT ((int)(sizeof(g_filters) / sizeof(g_filters[0])))

static const char A11Y_XML[] =
"<Window Title=\"Accessibility\" Width=\"520\" Height=\"640\" MinWidth=\"420\" MinHeight=\"360\">"
"<ScrollBox>"
"  <VBox Spacing=\"14\" Slot.Padding=\"22\">"
"    <Label Name=\"a11y.heading0\" Text=\"Accessibility\" TextSize=\"22\" AccessibleRole=\"Heading\"/>"
"    <Label Text=\"What every application made with Raven offers its users, with nothing to write: the settings apply at once, to every window. The designer keeps them for the next time.\" Wrap=\"true\" Opacity=\"0.65\"/>"

"    <Label Name=\"a11y.heading1\" Text=\"Sight\" TextSize=\"16\" AccessibleRole=\"Heading\"/>"
"    <Grid Columns=\"0,1\" Spacing=\"12\">"
"      <Label Width=\"150\" Name=\"text.label\" Text=\"Text size\" Slot.VAlign=\"Center\"/>"
"      <HBox Spacing=\"10\" Slot.Column=\"1\"><Slider Name=\"text\" Min=\"0.75\" Max=\"2\" Step=\"0.05\" Slot.Fill=\"1\" AccessibleName=\"Text size\"/><Label Name=\"text.value\" Width=\"52\" Slot.VAlign=\"Center\"/></HBox>"
"      <Label Width=\"150\" Text=\"Interface size\" Slot.Row=\"1\" Slot.VAlign=\"Center\" Tooltip=\"Everything grows: texts, controls, spaces\"/>"
"      <HBox Spacing=\"10\" Slot.Row=\"1\" Slot.Column=\"1\"><Slider Name=\"ui\" Min=\"0.75\" Max=\"2\" Step=\"0.05\" Slot.Fill=\"1\" AccessibleName=\"Interface size\"/><Label Name=\"ui.value\" Width=\"52\" Slot.VAlign=\"Center\"/></HBox>"
"      <Label Width=\"150\" Text=\"High contrast\" Slot.Row=\"2\" Slot.VAlign=\"Center\" Tooltip=\"Black and white, a yellow focus, thick borders\"/>"
"      <Checkbox Name=\"contrast\" Style=\"Switch\" Slot.Row=\"2\" Slot.Column=\"1\" AccessibleName=\"High contrast\"/>"
"      <Label Width=\"150\" Text=\"Colors\" Slot.Row=\"3\" Slot.VAlign=\"Center\" Tooltip=\"Corrections move the colors that cannot be told apart; simulations show the interface as the color blind see it\"/>"
"      <ComboBox Name=\"filter\" Slot.Row=\"3\" Slot.Column=\"1\" AccessibleName=\"Color filter\"/>"
"      <Label Width=\"150\" Text=\"Focus ring\" Slot.Row=\"4\" Slot.VAlign=\"Center\" Tooltip=\"Always shown and thicker, not only after the keyboard was used\"/>"
"      <Checkbox Name=\"focus\" Text=\"Always visible\" Style=\"Switch\" Slot.Row=\"4\" Slot.Column=\"1\"/>"
"    </Grid>"

"    <Label Name=\"a11y.heading2\" Text=\"Motion\" TextSize=\"16\" AccessibleRole=\"Heading\"/>"
"    <Grid Columns=\"0,1\" Spacing=\"12\">"
"      <Label Width=\"150\" Text=\"Reduce motion\" Slot.VAlign=\"Center\" Tooltip=\"Nothing moves by itself: no spring, no slide, no bounce, and the effects stand still\"/>"
"      <Checkbox Name=\"motion\" Style=\"Switch\" Slot.Column=\"1\" AccessibleName=\"Reduce motion\"/>"
"      <Label Width=\"150\" Text=\"Animation speed\" Slot.Row=\"1\" Slot.VAlign=\"Center\"/>"
"      <HBox Spacing=\"10\" Slot.Row=\"1\" Slot.Column=\"1\"><Slider Name=\"speed\" Min=\"0.25\" Max=\"3\" Step=\"0.05\" Slot.Fill=\"1\" AccessibleName=\"Animation speed\"/><Label Name=\"speed.value\" Width=\"52\" Slot.VAlign=\"Center\"/></HBox>"
"    </Grid>"
"    <Border Name=\"motion.demo\" Effect=\"aurora\" CornerRadius=\"14\" Height=\"64\" Tooltip=\"An effect: it stands still with reduced motion, and still follows the pointer\">"
"      <Label Text=\"Move the pointer here\" Slot.Padding=\"14\" Slot.VAlign=\"Center\" AccessibleHidden=\"true\"/>"
"    </Border>"

"    <Label Name=\"a11y.heading3\" Text=\"Screen readers\" TextSize=\"16\" AccessibleRole=\"Heading\"/>"
"    <Label Name=\"reader\" Wrap=\"true\" Opacity=\"0.8\"/>"
"    <HBox Spacing=\"8\"><Button Name=\"announce\" Text=\"Read a message\" Tooltip=\"Raccessibility_announce: what a screen reader says when something happens\"/></HBox>"

"    <Label Name=\"a11y.heading4\" Text=\"The system\" TextSize=\"16\" AccessibleRole=\"Heading\"/>"
"    <Label Name=\"system\" Wrap=\"true\" Opacity=\"0.8\"/>"
"    <HBox Spacing=\"8\">"
"      <Button Name=\"follow\" Text=\"Follow the system\" Tooltip=\"Text size, contrast and motion as the system asks\"/>"
"      <Button Name=\"reset\" Text=\"Reset\"/>"
"      <Spacer Slot.Fill=\"1\"/>"
"      <Button Name=\"close\" Text=\"Close\" Style=\"Primary\"/>"
"    </HBox>"
"  </VBox>"
"</ScrollBox>"
"</Window>";

static struct {
	Rid window;
	int syncing;
} g_a;

/* ---------------------------------------------------------------------------
 *  Keeping the settings
 * ------------------------------------------------------------------------- */

static void save_float(const char* key, float value) {
	char text[32];
	snprintf(text, sizeof(text), "%.2f", value);
	prefs_set(key, text);
}

static void save_all(void) {
	char text[16];
	save_float("a11y.text_scale", Raccessibility_get_text_scale());
	save_float("a11y.ui_scale", Rget_ui_scale());
	prefs_set("a11y.high_contrast", Raccessibility_get_high_contrast() ? "1" : "0");
	snprintf(text, sizeof(text), "%d", (int)Raccessibility_get_color_filter());
	prefs_set("a11y.color_filter", text);
	prefs_set("a11y.focus_visible", Raccessibility_get_focus_always_visible() ? "1" : "0");
	prefs_set("a11y.reduce_motion", Raccessibility_get_reduce_motion() ? "1" : "0");
	save_float("a11y.animation_speed", Rget_animation_speed());
}

void accessibility_apply_saved(void) {
	const char* value;
	if ((value = prefs_get("a11y.text_scale"))[0] && atof(value) > 0.0) Raccessibility_set_text_scale((float)atof(value));
	if ((value = prefs_get("a11y.ui_scale"))[0] && atof(value) > 0.0) Rset_ui_scale((float)atof(value));
	if ((value = prefs_get("a11y.high_contrast"))[0]) Raccessibility_set_high_contrast(atoi(value) != 0);
	if ((value = prefs_get("a11y.color_filter"))[0]) {
		int filter = atoi(value), i;
		for (i = 0; i < FILTER_COUNT; i++) if ((int)g_filters[i].filter == filter) Raccessibility_set_color_filter(g_filters[i].filter);
	}
	if ((value = prefs_get("a11y.focus_visible"))[0]) Raccessibility_set_focus_always_visible(atoi(value) != 0);
	if ((value = prefs_get("a11y.reduce_motion"))[0]) Raccessibility_set_reduce_motion(atoi(value) != 0);
	if ((value = prefs_get("a11y.animation_speed"))[0] && atof(value) > 0.0) Rset_animation_speed((float)atof(value));
	accessibility_sync_menu();
}

/* ---------------------------------------------------------------------------
 *  The window
 * ------------------------------------------------------------------------- */

static Rid find(const char* name) {
	return Rwidget_find(g_a.window, name);
}

static void show_percent(const char* name, float value) {
	char text[16];
	snprintf(text, sizeof(text), "%d %%", (int)(value * 100.0f + 0.5f));
	Rlabel_set_text(find(name), text);
}

/* The window shows the settings as they are (they may change from the menu, or by following the system). */
static void refresh(void) {
	RAccessibilitySettings system;
	char text[300];
	int i;
	if (g_a.window == R_INVALID_ID) return;
	g_a.syncing++;
	Rslider_set_value(find("text"), Raccessibility_get_text_scale());
	show_percent("text.value", Raccessibility_get_text_scale());
	Rslider_set_value(find("ui"), Rget_ui_scale());
	show_percent("ui.value", Rget_ui_scale());
	Rcheckbox_set_checked(find("contrast"), Raccessibility_get_high_contrast());
	for (i = 0; i < FILTER_COUNT; i++) if (g_filters[i].filter == Raccessibility_get_color_filter()) Rcombo_box_set_selected(find("filter"), i);
	Rcheckbox_set_checked(find("focus"), Raccessibility_get_focus_always_visible());
	Rcheckbox_set_checked(find("motion"), Raccessibility_get_reduce_motion());
	Rslider_set_value(find("speed"), Rget_animation_speed());
	show_percent("speed.value", Rget_animation_speed());
	/* (no speed to choose when nothing moves) */
	Rwidget_set_enabled(find("speed"), !Raccessibility_get_reduce_motion());
	g_a.syncing--;

	Rlabel_set_text(find("reader"), Raccessibility_is_screen_reader_active()
		? "A screen reader is running: it reads the controls of every window (their names, roles, values and states)."
		: "No screen reader is running. Raven speaks to them (UI Automation on Windows, AT-SPI on Linux): start Narrator or NVDA, and every control is read.");
	Raccessibility_get_system_settings(&system);
	snprintf(text, sizeof(text), "The system asks for: text at %d %%, %s, %s%s.", (int)(system.text_scale * 100.0f + 0.5f),
		system.high_contrast ? "high contrast" : "normal contrast", system.reduce_motion ? "reduced motion" : "animations",
		system.screen_reader ? ", a screen reader" : "");
	Rlabel_set_text(find("system"), text);
}

static void changed(void) {
	save_all();
	accessibility_sync_menu();
	refresh();
}

static void on_slider(Rid slider, float value, void* user_data) {
	const char* which = (const char*)user_data;
	if (g_a.syncing) return;
	(void)slider;
	if (!strcmp(which, "text")) Raccessibility_set_text_scale(value);
	else if (!strcmp(which, "ui")) Rset_ui_scale(value);
	else if (!strcmp(which, "speed")) Rset_animation_speed(value);
	changed();
}

static void on_switch(Rid checkbox, int checked, void* user_data) {
	const char* which = (const char*)user_data;
	if (g_a.syncing) return;
	(void)checkbox;
	if (!strcmp(which, "contrast")) Raccessibility_set_high_contrast(checked);
	else if (!strcmp(which, "focus")) Raccessibility_set_focus_always_visible(checked);
	else if (!strcmp(which, "motion")) Raccessibility_set_reduce_motion(checked);
	changed();
}

static void on_filter(Rid combo_box, int index, void* user_data) {
	(void)combo_box;
	(void)user_data;
	if (g_a.syncing || index < 0 || index >= FILTER_COUNT) return;
	Raccessibility_set_color_filter(g_filters[index].filter);
	changed();
}

static void on_button(Rid button, void* user_data) {
	const char* which = (const char*)user_data;
	(void)button;
	if (!strcmp(which, "announce")) {
		Raccessibility_announce("This is Raven: the screen reader reads what the application announces.", R_FALSE);
		ds_status("Announced to the screen reader%s.", Raccessibility_is_screen_reader_active() ? "" : " (none is running)");
		return;
	}
	if (!strcmp(which, "close")) {
		Rwindow_destroy(g_a.window);
		g_a.window = R_INVALID_ID;
		return;
	}
	if (!strcmp(which, "follow")) {
		Raccessibility_follow_system();
	} else if (!strcmp(which, "reset")) {
		Raccessibility_set_text_scale(1.0f);
		Rset_ui_scale(1.0f);
		Raccessibility_set_high_contrast(R_FALSE);
		Raccessibility_set_color_filter(R_COLOR_FILTER_NONE);
		Raccessibility_set_focus_always_visible(R_FALSE);
		Raccessibility_set_reduce_motion(R_FALSE);
		Rset_animation_speed(1.0f);
	}
	changed();
}

static int on_close(Rid window, void* user_data) {
	(void)window;
	(void)user_data;
	g_a.window = R_INVALID_ID;
	return R_TRUE;
}

void accessibility_window(void) {
	static const char* const sliders[] = { "text", "ui", "speed" };
	static const char* const switches[] = { "contrast", "focus", "motion" };
	static const char* const buttons[] = { "announce", "follow", "reset", "close" };
	size_t i;
	if (g_a.window != R_INVALID_ID && Rwindow_is_valid(g_a.window)) {
		Rwindow_show(g_a.window);
		refresh();
		return;
	}
	g_a.window = Rwindow_create_from_xml(A11Y_XML, sizeof(A11Y_XML) - 1);
	if (g_a.window == R_INVALID_ID) return;
	for (i = 0; i < (size_t)FILTER_COUNT; i++) Rcombo_box_add_item(find("filter"), g_filters[i].name);
	for (i = 0; i < sizeof(sliders) / sizeof(sliders[0]); i++) Rslider_set_changed_callback(find(sliders[i]), on_slider, (void*)sliders[i]);
	for (i = 0; i < sizeof(switches) / sizeof(switches[0]); i++) Rcheckbox_set_changed_callback(find(switches[i]), on_switch, (void*)switches[i]);
	for (i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++) Rbutton_set_clicked_callback(find(buttons[i]), on_button, (void*)buttons[i]);
	Rcombo_box_set_changed_callback(find("filter"), on_filter, NULL);
	Rwindow_set_close_callback(g_a.window, on_close, NULL);
	for (i = 0; i < 5; i++) {
		char name[32];
		snprintf(name, sizeof(name), "a11y.heading%d", (int)i);
		fonts_title_on(find(name));
	}
	refresh();
}

/* ---------------------------------------------------------------------------
 *  The menu
 * ------------------------------------------------------------------------- */

void accessibility_sync_menu(void) {
	Rid item = Rwidget_find(g_ds_window, "menu.reduce_motion");
	if (item != R_INVALID_ID) Rmenu_item_set_checked(item, Raccessibility_get_reduce_motion());
	refresh();
}

static void action_accessibility(Rid sender, const char* action, void* user_data) {
	crash_report_note("action %s", action);
	(void)sender;
	(void)user_data;
	if (!strcmp(action, "view.accessibility")) {
		accessibility_window();
	} else if (!strcmp(action, "view.reduce_motion")) {
		Raccessibility_set_reduce_motion(!Raccessibility_get_reduce_motion());
		save_all();
		accessibility_sync_menu();
		ds_status(Raccessibility_get_reduce_motion() ? "Reduced motion: nothing moves by itself." : "Animations are back.");
	}
}

void accessibility_init(void) {
	g_a.window = R_INVALID_ID;
	Raction_bind("view.accessibility", action_accessibility, NULL);
	Raction_bind("view.reduce_motion", action_accessibility, NULL);
}

void accessibility_shutdown(void) {
	if (g_a.window != R_INVALID_ID && Rwindow_is_valid(g_a.window)) Rwindow_destroy(g_a.window);
	g_a.window = R_INVALID_ID;
}

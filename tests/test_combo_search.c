/**
 * Raven tests - ComboBox: editable and searchable.
 *
 * The comparison of a search (case, accents, combining marks, Greek and
 * Cyrillic). A searchable combo box: a click opens it ready to type, typing
 * filters the list, Up / Down / Enter choose among the matches, "No
 * matches" when nothing does, Escape and a click elsewhere bring back the
 * item chosen. An editable one: any text, the item equal to it selected,
 * the text callback, the arrow that opens all the items. Both together,
 * the XML attributes, and what a screen reader sees: the value, the states,
 * the rows of the open list and the highlighted one as the focus.
 */

#include "test.h"
#include "../src/core/accessibility.h"
#include "../src/widget/widget.h"
#include "../src/widgets/common.h"

#define W 400.0f
#define H 300.0f

static Rid g_window;
static int g_changes, g_last_index, g_texts;
static char g_last_text[64];

static void on_changed(Rid combo, int index, void* user) {
	(void)combo; (void)user;
	g_changes++;
	g_last_index = index;
}

static void on_text(Rid combo, const char* text, void* user) {
	(void)combo; (void)user;
	g_texts++;
	snprintf(g_last_text, sizeof(g_last_text), "%s", text);
}

static const char* const ITEMS[] = { "Zoë", "Mazé", "Paris", "São Paulo", "Zürich", "Ångström", "Москва", "Αθήνα" };
#define ITEM_COUNT 8

/* A combo box at the top of a column, a button under it (to click elsewhere). */
static Rid make_combo(Rid* other) {
	Rid box = Rwidget_create(R_WIDGET_VBOX);
	Rid combo = Rwidget_create(R_WIDGET_COMBO_BOX);
	Rid button = Rbutton_create("Elsewhere");
	int i;
	Rwindow_set_content(g_window, box);
	Rwidget_add_child(box, combo);
	/* (the button at the bottom, away from the list that drops down) */
	Rslot_set_fill(Rwidget_add_child(box, Rwidget_create(R_WIDGET_VBOX)), 1.0f);
	Rwidget_add_child(box, button);
	for (i = 0; i < ITEM_COUNT; i++) Rcombo_box_add_item(combo, ITEMS[i]);
	Rcombo_box_set_changed_callback(combo, on_changed, NULL);
	Rcombo_box_set_text_changed_callback(combo, on_text, NULL);
	g_changes = g_texts = 0;
	g_last_index = -2;
	g_last_text[0] = '\0';
	Rdraw_frame();
	if (other) *other = button;
	return combo;
}

static rvn_widget_t* open_list(Rid combo) {
	return rvn_combo_box_open_list(rvn_widget_get(combo));
}

/* The texts of the rows of the open list, joined by '|'. */
static const char* rows(Rid combo) {
	static char out[256];
	rvn_widget_t* list = open_list(combo);
	int i, n;
	out[0] = '\0';
	if (!list) return "(closed)";
	n = rvn_combo_list_row_count(list);
	for (i = 0; i < n; i++) {
		const char* text = NULL;
		rvn_rect_t rect;
		rvn_combo_list_row_info(list, i, &text, &rect);
		if (i) strncat(out, "|", sizeof(out) - strlen(out) - 1);
		strncat(out, text, sizeof(out) - strlen(out) - 1);
	}
	return out;
}

static void click_text(Rid combo) {
	float x, y, w, h;
	Rwidget_get_rect(combo, &x, &y, &w, &h);
	test_click_at(g_window, x + 20.0f, y + h * 0.5f);
}

static void click_arrow(Rid combo) {
	float x, y, w, h;
	Rwidget_get_rect(combo, &x, &y, &w, &h);
	test_click_at(g_window, x + w - 8.0f, y + h * 0.5f);
}

static void test_fold(void) {
	CHECK(rvn_utf8_contains_folded("Zoë", "zoe"));
	CHECK(rvn_utf8_contains_folded("Zoë", "OË"));
	CHECK(rvn_utf8_contains_folded("São Paulo", "sao p"));
	CHECK(rvn_utf8_contains_folded("Ångström", "angstrom"));
	CHECK(rvn_utf8_contains_folded("Łódź", "lodz"));
	CHECK(rvn_utf8_contains_folded("Œuvre", "œu"));
	CHECK(rvn_utf8_contains_folded("Straße", "straße"));
	CHECK(!rvn_utf8_contains_folded("Straße", "strasse"));
	/* a decomposed é (e and a combining acute) is an e */
	CHECK(rvn_utf8_contains_folded("Caf" "e\xCC\x81" " noir", "cafe n"));
	CHECK(rvn_utf8_contains_folded("café", "cafe\xCC\x81"));
	CHECK(rvn_utf8_contains_folded("Αθήνα", "αθην"));
	CHECK(rvn_utf8_contains_folded("ΑΘΗΝΑ", "αθήνα"));
	CHECK(rvn_utf8_contains_folded("Москва", "МОСК"));
	CHECK(rvn_utf8_contains_folded("Ёлка", "елка"));
	CHECK(rvn_utf8_contains_folded("anything", ""));
	CHECK(rvn_utf8_contains_folded("aab", "ab"));
	CHECK(!rvn_utf8_contains_folded("Paris", "parisx"));
	CHECK(!rvn_utf8_contains_folded("", "a"));
	CHECK(!rvn_utf8_contains_folded(NULL, "a"));
	CHECK(rvn_utf8_fold('Q') == 'q' && rvn_utf8_fold(0xC9) == 'e' && rvn_utf8_fold(0x17D) == 'z');
	CHECK(rvn_utf8_fold(0x301) == 0 && rvn_utf8_fold(0x4E2D) == 0x4E2D);
}

static void test_api(void) {
	Rid combo = make_combo(NULL);
	CHECK(!Rcombo_box_is_editable(combo) && !Rcombo_box_is_searchable(combo));
	CHECK(Rwidget_get_child_count(combo) == 0);
	CHECK_STR(Rcombo_box_get_text(combo), "");
	Rcombo_box_set_text(combo, "Paris");
	CHECK(Rcombo_box_get_selected(combo) == 2);
	CHECK_STR(Rcombo_box_get_text(combo), "Paris");
	Rcombo_box_set_text(combo, "Nowhere");
	CHECK(Rcombo_box_get_selected(combo) == -1);
	Rcombo_box_set_open(combo, R_TRUE);
	CHECK(Rcombo_box_is_open(combo));
	CHECK(strcmp(rows(combo), "Zoë|Mazé|Paris|São Paulo|Zürich|Ångström|Москва|Αθήνα") == 0);
	Rcombo_box_set_open(combo, R_FALSE);
	CHECK(!Rcombo_box_is_open(combo));
	CHECK(g_changes == 0);
}

static void test_searchable(void) {
	Rid other;
	Rid combo = make_combo(&other);
	Rid field;
	Rcombo_box_set_searchable(combo, R_TRUE);
	Rcombo_box_set_selected(combo, 2);
	Rdraw_frame();
	CHECK(Rcombo_box_is_searchable(combo) && Rwidget_get_child_count(combo) == 1);
	field = Rwidget_get_child(combo, 0);
	CHECK_STR(Rtext_input_get_text(field), "Paris");
	/* a click opens the list (all the items), the text selected, ready to be typed over */
	click_text(combo);
	CHECK(Rcombo_box_is_open(combo));
	CHECK(Rwindow_get_focused_widget(g_window) == field);
	CHECK(rvn_combo_list_hot(open_list(combo)) == 2);
	Rwindow_inject_text(g_window, "Z");
	CHECK(g_texts == 1);
	CHECK_STR(g_last_text, "Z");
	CHECK(strcmp(rows(combo), "Zoë|Mazé|Zürich") == 0);
	CHECK(rvn_combo_list_hot(open_list(combo)) == 0);
	Rwindow_inject_text(g_window, "U");
	CHECK(strcmp(rows(combo), "Zürich") == 0);
	/* nothing chosen yet, the selection is still Paris */
	CHECK(Rcombo_box_get_selected(combo) == 2 && g_changes == 0);
	test_key(g_window, R_KEY_BACKSPACE, 0);
	CHECK(strcmp(rows(combo), "Zoë|Mazé|Zürich") == 0);
	test_key(g_window, R_KEY_DOWN, 0);
	test_key(g_window, R_KEY_DOWN, 0);
	CHECK(rvn_combo_list_hot(open_list(combo)) == 2);
	test_key(g_window, R_KEY_UP, 0);
	CHECK(rvn_combo_list_hot(open_list(combo)) == 1);
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(!Rcombo_box_is_open(combo));
	CHECK(Rcombo_box_get_selected(combo) == 1 && g_changes == 1 && g_last_index == 1);
	CHECK_STR(Rtext_input_get_text(field), "Mazé");
	CHECK_STR(Rcombo_box_get_text(combo), "Mazé");
	/* nothing matches: one row that says so; Enter chooses nothing */
	Rwindow_inject_text(g_window, "xyz");
	CHECK(Rcombo_box_is_open(combo));
	CHECK(strcmp(rows(combo), "No matches") == 0);
	CHECK(rvn_combo_list_hot(open_list(combo)) == -1);
	Rdraw_frame();
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(!Rcombo_box_is_open(combo));
	CHECK(Rcombo_box_get_selected(combo) == 1 && g_changes == 1);
	CHECK_STR(Rtext_input_get_text(field), "Mazé");
	/* Escape closes the list and brings the chosen item back */
	Rwindow_inject_text(g_window, "\xD0\xBC\xD0\x9E\xD1\x81"); /* мОс */
	CHECK(strcmp(rows(combo), "Москва") == 0);
	test_key(g_window, R_KEY_ESCAPE, 0);
	CHECK(!Rcombo_box_is_open(combo));
	CHECK_STR(Rtext_input_get_text(field), "Mazé");
	CHECK(Rwindow_get_focused_widget(g_window) == field);
	/* typed accents and case: "ATH" finds Αθήνα? no: Latin is not Greek; "αθη" does */
	Rwindow_inject_text(g_window, "\xCE\x91\xCE\x98\xCE\x97"); /* ΑΘΗ */
	CHECK(strcmp(rows(combo), "Αθήνα") == 0);
	/* a click on a row chooses it */
	{
		rvn_widget_t* list = open_list(combo);
		rvn_vec2_t origin = rvn_widget_origin(list);
		test_click_at(g_window, origin.x + 20.0f, origin.y + list->rect.h * 0.5f);
	}
	CHECK(!Rcombo_box_is_open(combo));
	CHECK(Rcombo_box_get_selected(combo) == 7 && g_changes == 2);
	/* Down opens all the items, the selected one highlighted */
	test_key(g_window, R_KEY_DOWN, 0);
	CHECK(Rcombo_box_is_open(combo));
	CHECK(rvn_combo_list_hot(open_list(combo)) == 7);
	test_key(g_window, R_KEY_ESCAPE, 0);
	/* a click elsewhere while typing: the chosen item again */
	Rwindow_inject_text(g_window, "par");
	CHECK(Rcombo_box_is_open(combo));
	test_click(g_window, other); /* closes the list */
	test_click(g_window, other); /* takes the focus */
	CHECK(!Rcombo_box_is_open(combo));
	CHECK(Rwindow_get_focused_widget(g_window) == other);
	CHECK_STR(Rtext_input_get_text(field), "Αθήνα");
	CHECK(Rcombo_box_get_selected(combo) == 7 && g_changes == 2);
	/* the combo box itself is no tab stop any more: its field is */
	Rwidget_focus(other);
	test_key(g_window, R_KEY_TAB, R_MOD_SHIFT);
	CHECK(Rwindow_get_focused_widget(g_window) == field);
	/* back to a plain combo box: the field goes, the combo box takes the keyboard */
	Rcombo_box_set_searchable(combo, R_FALSE);
	CHECK(Rwindow_get_focused_widget(g_window) == combo);
	Rdraw_frame();
	test_key(g_window, R_KEY_UP, 0);
	CHECK(Rcombo_box_get_selected(combo) == 6);
}

static void test_editable(void) {
	Rid combo = make_combo(NULL);
	Rid field;
	Rcombo_box_set_editable(combo, R_TRUE);
	Rdraw_frame();
	field = Rwidget_get_child(combo, 0);
	click_text(combo);
	CHECK(!Rcombo_box_is_open(combo)); /* a click in the text places the caret */
	CHECK(Rwindow_get_focused_widget(g_window) == field);
	Rwindow_inject_text(g_window, "Lyon");
	CHECK(!Rcombo_box_is_open(combo)); /* not searchable: no filter */
	CHECK_STR(Rcombo_box_get_text(combo), "Lyon");
	CHECK(Rcombo_box_get_selected(combo) == -1 && g_changes == 0);
	CHECK(g_texts == 4);
	CHECK_STR(g_last_text, "Lyon");
	/* a text equal to an item selects it, a different one selects nothing again */
	Rcombo_box_set_text(combo, "");
	Rwindow_inject_text(g_window, "Paris");
	CHECK(Rcombo_box_get_selected(combo) == 2 && g_changes == 1 && g_last_index == 2);
	Rwindow_inject_text(g_window, "!");
	CHECK(Rcombo_box_get_selected(combo) == -1 && g_changes == 2 && g_last_index == -1);
	CHECK_STR(Rcombo_box_get_text(combo), "Paris!");
	/* the arrow opens all the items; choosing one puts its text in the field */
	click_arrow(combo);
	CHECK(Rcombo_box_is_open(combo));
	CHECK(strcmp(rows(combo), "Zoë|Mazé|Paris|São Paulo|Zürich|Ångström|Москва|Αθήνα") == 0);
	test_key(g_window, R_KEY_DOWN, 0);
	test_key(g_window, R_KEY_ENTER, 0);
	CHECK(Rcombo_box_get_selected(combo) == 0);
	CHECK_STR(Rcombo_box_get_text(combo), "Zoë");
	/* Escape keeps a typed text */
	Rwindow_inject_text(g_window, " et al");
	click_arrow(combo);
	test_key(g_window, R_KEY_ESCAPE, 0);
	CHECK_STR(Rcombo_box_get_text(combo), "Zoë et al");
	/* the program sets the text: no text callback */
	g_texts = 0;
	Rcombo_box_set_text(combo, "Paris");
	CHECK(Rcombo_box_get_selected(combo) == 2 && g_texts == 0);
	CHECK_STR(Rtext_input_get_text(field), "Paris");
	/* editable and searchable: typing filters, a free text stays */
	Rcombo_box_set_searchable(combo, R_TRUE);
	Rcombo_box_set_text(combo, "");
	Rwidget_focus(field);
	Rwindow_inject_text(g_window, "o");
	CHECK(strcmp(rows(combo), "Zoë|São Paulo|Ångström") == 0);
	Rwindow_inject_text(g_window, "e");
	CHECK(strcmp(rows(combo), "Zoë") == 0);
	test_key(g_window, R_KEY_ESCAPE, 0);
	CHECK_STR(Rcombo_box_get_text(combo), "oe");
	Rcombo_box_clear_items(combo);
	CHECK_STR(Rcombo_box_get_text(combo), "oe");
	Rwindow_inject_text(g_window, "x"); /* no items: no list */
	CHECK(!Rcombo_box_is_open(combo));
}

static void test_accessibility(void) {
	Rid combo = make_combo(NULL);
	rvn_a11y_node_t node;
	rvn_a11y_id_t focus;
	rvn_widget_t* list;
	Rcombo_box_set_searchable(combo, R_TRUE);
	Rcombo_box_set_editable(combo, R_TRUE);
	Rcombo_box_set_text(combo, "Lyon");
	Rdraw_frame();
	CHECK(rvn_a11y_describe(combo, &node));
	CHECK(node.role == R_ROLE_COMBO_BOX);
	CHECK((node.states & RVN_A11Y_EDITABLE) && !(node.states & RVN_A11Y_EXPANDED));
	CHECK_STR(node.value, "Lyon");
	/* the field is part of the combo box, not a node of its own */
	CHECK(rvn_a11y_child_count(combo) == 0);
	CHECK(rvn_a11y_set_value(combo, "Rome"));
	CHECK_STR(Rcombo_box_get_text(combo), "Rome");
	Rwidget_focus(Rwidget_get_child(combo, 0));
	CHECK(rvn_a11y_focus(g_window) == combo);
	Rcombo_box_set_text(combo, "");
	Rwindow_inject_text(g_window, "a");
	list = open_list(combo);
	CHECK(list != NULL);
	CHECK(rvn_a11y_describe(combo, &node) && (node.states & RVN_A11Y_EXPANDED));
	/* the list: a list of the matching rows, the highlighted one has the focus */
	CHECK(rvn_a11y_describe(list->id, &node) && node.role == R_ROLE_LIST);
	CHECK(rvn_a11y_child_count(list->id) == rvn_combo_list_row_count(list));
	CHECK(rvn_a11y_child_count(list->id) == 4); /* Mazé, Paris, São Paulo, Ångström (+ Москва, Αθήνα: no Latin a) */
	test_key(g_window, R_KEY_DOWN, 0);
	focus = rvn_a11y_focus(g_window);
	CHECK(focus == rvn_a11y_child(list->id, 1));
	CHECK(rvn_a11y_describe(focus, &node));
	CHECK(node.role == R_ROLE_LIST_ITEM && (node.states & RVN_A11Y_FOCUSED) && (node.states & RVN_A11Y_SELECTABLE));
	CHECK_STR(node.name, "Paris");
	CHECK(node.width > 0 && node.height > 0);
	/* chosen from the screen reader */
	CHECK(rvn_a11y_act(rvn_a11y_child(list->id, 2), RVN_A11Y_SELECT));
	CHECK(!Rcombo_box_is_open(combo));
	CHECK_STR(Rcombo_box_get_text(combo), "São Paulo");
	/* "No matches" is read, and cannot be chosen */
	Rwindow_inject_text(g_window, "qq");
	list = open_list(combo);
	CHECK(rvn_a11y_child_count(list->id) == 1);
	CHECK(rvn_a11y_describe(rvn_a11y_child(list->id, 0), &node));
	CHECK_STR(node.name, "No matches");
	CHECK((node.states & RVN_A11Y_DISABLED) && !(node.states & RVN_A11Y_SELECTABLE));
	CHECK(!rvn_a11y_act(rvn_a11y_child(list->id, 0), RVN_A11Y_SELECT));
	CHECK(rvn_a11y_focus(g_window) == combo);
	/* a combo box that is not editable takes only the text of an item */
	Rcombo_box_set_editable(combo, R_FALSE);
	CHECK(!rvn_a11y_set_value(combo, "Lyon"));
	CHECK(rvn_a11y_set_value(combo, "Zürich"));
	CHECK(Rcombo_box_get_selected(combo) == 4);
}

static void test_xml(void) {
	static const char* xml = "<ComboBox Items=\"Red|Green|Blue\" Editable=\"true\" Searchable=\"true\" Text=\"Teal\"/>";
	Rid combo = Rwidget_create_from_xml(xml, strlen(xml));
	CHECK(combo != R_INVALID_ID);
	CHECK(Rcombo_box_is_editable(combo) && Rcombo_box_is_searchable(combo));
	CHECK_STR(Rcombo_box_get_text(combo), "Teal");
	CHECK(Rcombo_box_get_selected(combo) == -1);
	Rwindow_set_content(g_window, combo);
	Rdraw_frame();
	Rwidget_focus(Rwidget_get_child(combo, 0));
	Rcombo_box_set_text(combo, "");
	Rwindow_inject_text(g_window, "e");
	CHECK(strcmp(rows(combo), "Red|Green|Blue") == 0);
	Rwindow_inject_text(g_window, "e");
	CHECK(strcmp(rows(combo), "Green") == 0);
	/* destroyed while its list is open */
	Rwindow_set_content(g_window, R_INVALID_ID);
	Rdraw_frame();
}

int main(void) {
	if (!test_init(1)) return 1;
	Rset_animations_enabled(R_FALSE);
	Rlook_set(R_LOOK_RAVEN_DARK);
	g_window = Rwindow_create("Combo box", (int)W, (int)H, 0);
	RUN(test_fold);
	RUN(test_api);
	RUN(test_searchable);
	RUN(test_editable);
	RUN(test_accessibility);
	RUN(test_xml);
	Rwindow_destroy(g_window);
	Rshutdown();
	return test_report("test_combo_search");
}

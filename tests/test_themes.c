/**
 * Raven tests - themes (styles, classes, themes of parts of a window, theme
 * files) and live reload of the interfaces and themes read from files.
 */

#include "test.h"

#include <stdlib.h>

static int g_reloads;
static int g_reload_ok;
static char g_reload_path[512];

static void on_reload(const char* path, int ok, void* user_data) {
	(void)user_data;
	g_reloads++;
	g_reload_ok = ok;
	snprintf(g_reload_path, sizeof(g_reload_path), "%s", path);
}

static int g_clicks;

static void on_click(Rid button, void* user_data) {
	(void)button;
	(void)user_data;
	g_clicks++;
}

/* The color a widget uses for a role, as 0xRRGGBBAA. */
static unsigned color_of(Rid widget, RColorRole role) {
	float r, g, b, a;
	Rwidget_get_color(widget, role, &r, &g, &b, &a);
	return ((unsigned)(r * 255.0f + 0.5f) << 24) | ((unsigned)(g * 255.0f + 0.5f) << 16) | ((unsigned)(b * 255.0f + 0.5f) << 8) | (unsigned)(a * 255.0f + 0.5f);
}

static unsigned theme_color(RColorRole role) {
	float r, g, b, a;
	Rtheme_get_color(role, &r, &g, &b, &a);
	return ((unsigned)(r * 255.0f + 0.5f) << 24) | ((unsigned)(g * 255.0f + 0.5f) << 16) | ((unsigned)(b * 255.0f + 0.5f) << 8) | (unsigned)(a * 255.0f + 0.5f);
}

static const char* temp_dir(void) {
	const char* temp = getenv("TMPDIR") ? getenv("TMPDIR") : (getenv("TEMP") ? getenv("TEMP") : "/tmp");
	return temp;
}

static void write_text(const char* path, const char* text) {
	CHECK(Rfile_write(path, text, strlen(text)));
}

/* ---------------------------------------------------------------------------
 *  Styles and classes
 * ------------------------------------------------------------------------- */

static void test_styles(void) {
	const char* xml =
		"<Window Width=\"400\" Height=\"300\"><VBox>"
		"<Button Name=\"plain\" Text=\"Plain\"/>"
		"<Button Name=\"danger\" Class=\"danger\" Text=\"Delete\"/>"
		"<Button Name=\"both\" Class=\"large danger\" Text=\"Delete all\"/>"
		"<Label Name=\"label\" Class=\"danger\">Careful</Label>"
		"<Button Name=\"own\" Class=\"danger\" Color.Accent=\"#00FF00\" Text=\"Mine\"/>"
		"</VBox></Window>";
	Rid window = Rwindow_create_from_xml(xml, strlen(xml));
	Rid plain = Rwidget_find(window, "plain"), danger = Rwidget_find(window, "danger");
	Rid both = Rwidget_find(window, "both"), label = Rwidget_find(window, "label"), own = Rwidget_find(window, "own");
	unsigned accent = theme_color(R_COLOR_ACCENT);
	float height;

	CHECK_STR(Rtheme_get_name(), "Dark");
	CHECK_STR(Rwidget_get_class(danger), "danger");
	CHECK_STR(Rwidget_get_class(plain), "");
	CHECK(Rwidget_has_class(both, "large") && Rwidget_has_class(both, "danger") && !Rwidget_has_class(both, "dan"));
	CHECK(color_of(danger, R_COLOR_ACCENT) == accent);

	/* a class */
	Rtheme_set_style_color(NULL, "danger", R_COLOR_ACCENT, 1.0f, 0.0f, 0.0f, 1.0f);
	CHECK(color_of(plain, R_COLOR_ACCENT) == accent);
	CHECK(color_of(danger, R_COLOR_ACCENT) == 0xFF0000FFu);
	CHECK(color_of(both, R_COLOR_ACCENT) == 0xFF0000FFu);
	CHECK(color_of(label, R_COLOR_ACCENT) == 0xFF0000FFu);
	/* the widget itself wins over the styles */
	CHECK(color_of(own, R_COLOR_ACCENT) == 0x00FF00FFu);
	/* the theme itself is untouched */
	CHECK(theme_color(R_COLOR_ACCENT) == accent);

	/* a type of widget */
	height = Rwidget_get_metric(plain, R_METRIC_CONTROL_HEIGHT);
	Rtheme_set_style_metric("Button", NULL, R_METRIC_CONTROL_HEIGHT, 40.0f);
	Rdraw_frame();
	CHECK_NEAR(Rwidget_get_metric(plain, R_METRIC_CONTROL_HEIGHT), 40.0f);
	CHECK_NEAR(Rwidget_get_metric(label, R_METRIC_CONTROL_HEIGHT), height);
	{
		float x, y, w, h;
		Rwidget_get_rect(plain, &x, &y, &w, &h);
		CHECK_NEAR(h, 40.0f);
	}

	/* a type and a class together win over each alone, whatever the order they were given in */
	Rtheme_set_style_color("Button", "danger", R_COLOR_ACCENT, 0.0f, 0.0f, 1.0f, 1.0f);
	Rtheme_set_style_color("Button", NULL, R_COLOR_ACCENT, 1.0f, 1.0f, 0.0f, 1.0f);
	CHECK(color_of(danger, R_COLOR_ACCENT) == 0x0000FFFFu);
	CHECK(color_of(plain, R_COLOR_ACCENT) == 0xFFFF00FFu);
	CHECK(color_of(label, R_COLOR_ACCENT) == 0xFF0000FFu);
	Rtheme_set_style_material(NULL, "large", R_MATERIAL_GLASS);
	CHECK(Rwidget_get_material(both) == R_MATERIAL_GLASS);
	CHECK(Rwidget_get_material(plain) == R_MATERIAL_SOLID);

	/* classes change while the application runs */
	Rwidget_add_class(plain, "danger");
	CHECK(color_of(plain, R_COLOR_ACCENT) == 0x0000FFFFu);
	Rwidget_add_class(plain, "danger");
	CHECK_STR(Rwidget_get_class(plain), "danger");
	Rwidget_add_class(plain, "wide");
	CHECK_STR(Rwidget_get_class(plain), "danger wide");
	Rwidget_remove_class(plain, "danger");
	CHECK_STR(Rwidget_get_class(plain), "wide");
	CHECK(color_of(plain, R_COLOR_ACCENT) == 0xFFFF00FFu);
	Rwidget_set_class(plain, NULL);
	CHECK_STR(Rwidget_get_class(plain), "");
	Rwidget_set_attribute(plain, "Class", "danger");
	CHECK(color_of(plain, R_COLOR_ACCENT) == 0x0000FFFFu);

	Rtheme_clear_styles();
	CHECK(color_of(danger, R_COLOR_ACCENT) == accent);
	CHECK_NEAR(Rwidget_get_metric(plain, R_METRIC_CONTROL_HEIGHT), height);
	CHECK(Rwidget_get_material(both) == R_MATERIAL_SOLID);

	/* another theme forgets the styles */
	Rtheme_set_style_color(NULL, "danger", R_COLOR_ACCENT, 1.0f, 0.0f, 0.0f, 1.0f);
	Rtheme_apply(R_THEME_LIGHT);
	CHECK_STR(Rtheme_get_name(), "Light");
	CHECK(color_of(danger, R_COLOR_ACCENT) == theme_color(R_COLOR_ACCENT));
	Rtheme_apply(R_THEME_DARK);

	EXPECT_ERRORS_BEGIN();
	Rtheme_set_style_color("NoSuchWidget", NULL, R_COLOR_ACCENT, 1.0f, 0.0f, 0.0f, 1.0f);
	Rwidget_add_class(plain, "two words");
	EXPECT_ERRORS_END(2);
	Rwindow_destroy(window);
}

/* ---------------------------------------------------------------------------
 *  The accent, and the theme of a part of a window
 * ------------------------------------------------------------------------- */

static void test_accent_and_scopes(void) {
	const char* xml =
		"<Window Width=\"400\" Height=\"300\"><HBox>"
		"<VBox Name=\"side\" Theme=\"Light\"><Button Name=\"in\" Text=\"In\"/><ComboBox Name=\"combo\"><Item>One</Item><Item>Two</Item></ComboBox>"
		"<VBox Name=\"back\" Theme=\"Dark\"><Label Name=\"deep\">Deep</Label></VBox></VBox>"
		"<Button Name=\"out\" Text=\"Out\"/>"
		"</HBox></Window>";
	Rid window, in, out, deep, moved;
	unsigned dark_text, light_text;

	Rtheme_apply(R_THEME_LIGHT);
	light_text = theme_color(R_COLOR_TEXT);
	Rtheme_apply(R_THEME_DARK);
	dark_text = theme_color(R_COLOR_TEXT);
	CHECK(dark_text != light_text);
	CHECK(Rtheme_is_dark());

	/* the accent brings its shades along */
	Rtheme_set_accent(1.0f, 0.5f, 0.0f);
	CHECK(theme_color(R_COLOR_ACCENT) == 0xFF8000FFu);
	CHECK(theme_color(R_COLOR_ACCENT_HOVERED) != 0xFF8000FFu && theme_color(R_COLOR_ACCENT_PRESSED) != 0xFF8000FFu);
	CHECK(theme_color(R_COLOR_FOCUS) == 0xFF8000FFu);
	/* (white would not be readable on a pale accent) */
	Rtheme_set_accent(1.0f, 1.0f, 0.4f);
	CHECK(theme_color(R_COLOR_ACCENT_TEXT) != 0xFFFFFFFFu);
	Rtheme_apply(R_THEME_DARK);

	window = Rwindow_create_from_xml(xml, strlen(xml));
	in = Rwidget_find(window, "in");
	out = Rwidget_find(window, "out");
	deep = Rwidget_find(window, "deep");
	CHECK_STR(Rwidget_get_theme(out), "Dark");
	CHECK_STR(Rwidget_get_theme(in), "Light");
	CHECK_STR(Rwidget_get_theme(Rwidget_find(window, "side")), "Light");
	CHECK_STR(Rwidget_get_theme(deep), "Dark");
	CHECK(color_of(out, R_COLOR_TEXT) == dark_text);
	CHECK(color_of(in, R_COLOR_TEXT) == light_text);
	CHECK(color_of(deep, R_COLOR_TEXT) == dark_text);

	/* changing the theme of the application leaves the parts that have their own alone */
	Rtheme_apply(R_THEME_GLASS_LIGHT);
	CHECK(color_of(in, R_COLOR_TEXT) == light_text);
	CHECK(color_of(out, R_COLOR_TEXT) == theme_color(R_COLOR_TEXT));
	CHECK(Rwidget_get_material(in) == R_MATERIAL_SOLID);
	CHECK(Rwidget_get_material(out) == R_MATERIAL_FROSTED_GLASS);
	Rtheme_apply(R_THEME_DARK);

	/* a widget takes the theme of where it is put */
	moved = Rbutton_create("Moved");
	CHECK(color_of(moved, R_COLOR_TEXT) == dark_text);
	Rwidget_add_child(Rwidget_find(window, "side"), moved);
	CHECK(color_of(moved, R_COLOR_TEXT) == light_text);
	Rwidget_add_child(Rwidget_find(window, "back"), moved);
	CHECK(color_of(moved, R_COLOR_TEXT) == dark_text);

	/* ... and the list of a combo box that of its combo box */
	Rdraw_frame();
	test_click(window, Rwidget_find(window, "combo"));
	Rdraw_frame();
	Rwindow_close_popups(window);

	CHECK(Rwidget_set_theme(Rwidget_find(window, "side"), "GlassDark"));
	CHECK_STR(Rwidget_get_theme(in), "GlassDark");
	CHECK_STR(Rwidget_get_theme(deep), "Dark");
	CHECK(Rwidget_set_theme(Rwidget_find(window, "side"), NULL));
	CHECK_STR(Rwidget_get_theme(in), "Dark");
	CHECK(color_of(in, R_COLOR_TEXT) == dark_text);
	EXPECT_ERRORS_BEGIN();
	CHECK(!Rwidget_set_theme(in, "No such theme"));
	CHECK(!Rtheme_use("No such theme"));
	EXPECT_ERRORS_END(2);
	Rdraw_frame();
	Rwindow_destroy(window);
}

/* ---------------------------------------------------------------------------
 *  Theme files
 * ------------------------------------------------------------------------- */

static const char g_theme_xml[] =
	"<Theme Name=\"Ember\" Base=\"Dark\" Accent=\"#FF6A00\" CornerRadius=\"9\">\n"
	"  <Colors Panel=\"#201510\" Text=\"#FFE8D0\" AccentPressed=\"#102030\"/>\n"
	"  <Metrics ControlHeight=\"31\" FontSize=\"15\"/>\n"
	"  <Glass Frost=\"0.5\" Rim=\"1.5\"/>\n"
	"  <Syntax Keyword=\"#FF0000\" String=\"#00FF00\"/>\n"
	"  <Style Class=\"danger\" Accent=\"#FF0033\" Material=\"TintedGlass\"/>\n"
	"  <Style Widget=\"Button\" ControlHeight=\"36\"/>\n"
	"</Theme>\n";

static void test_theme_files(void) {
	const char* xml = "<Window Width=\"400\" Height=\"300\"><VBox><Button Name=\"a\" Text=\"A\"/><Button Name=\"b\" Class=\"danger\" Text=\"B\"/><Label Name=\"l\">L</Label></VBox></Window>";
	Rid window = Rwindow_create_from_xml(xml, strlen(xml));
	Rid a = Rwidget_find(window, "a"), b = Rwidget_find(window, "b"), l = Rwidget_find(window, "l");
	char path[512], saved[512];
	const char* name;
	int count = Rtheme_get_count();
	float r, g, bl, al;

	/* Dark, Light, two more and the two high contrast themes */
	CHECK(count == 6);
	CHECK_STR(Rtheme_get_name_at(0), "Dark");
	CHECK(Rtheme_get_name_at(6) == NULL);

	name = Rtheme_load_from_memory(g_theme_xml, 0);
	CHECK_STR(name, "Ember");
	CHECK(Rtheme_get_count() == 7);
	/* loading a theme does not put it on */
	CHECK_STR(Rtheme_get_name(), "Dark");
	CHECK(theme_color(R_COLOR_PANEL) != 0x201510FFu);

	CHECK(Rtheme_use("ember"));
	CHECK_STR(Rtheme_get_name(), "Ember");
	CHECK(theme_color(R_COLOR_PANEL) == 0x201510FFu);
	CHECK(theme_color(R_COLOR_TEXT) == 0xFFE8D0FFu);
	CHECK(theme_color(R_COLOR_ACCENT) == 0xFF6A00FFu);
	/* a shade given by the file replaces the one worked out from the accent; the others follow the accent */
	CHECK(theme_color(R_COLOR_ACCENT_PRESSED) == 0x102030FFu);
	CHECK(theme_color(R_COLOR_FOCUS) == 0xFF6A00FFu);
	CHECK_NEAR(Rtheme_get_metric(R_METRIC_CORNER_RADIUS), 9.0f);
	CHECK_NEAR(Rtheme_get_metric(R_METRIC_CONTROL_HEIGHT), 31.0f);
	CHECK_NEAR(Rtheme_get_metric(R_METRIC_GLASS_FROST) * 10.0f, 5.0f);
	CHECK_NEAR(Rtheme_get_metric(R_METRIC_GLASS_RIM) * 10.0f, 15.0f);
	CHECK(Rtheme_is_dark());
	Rtheme_get_token_color(R_TOKEN_KEYWORD, &r, &g, &bl, &al);
	CHECK(r > 0.99f && g < 0.01f && bl < 0.01f);
	/* its styles */
	CHECK(color_of(b, R_COLOR_ACCENT) == 0xFF0033FFu);
	CHECK(color_of(a, R_COLOR_ACCENT) == 0xFF6A00FFu);
	CHECK(Rwidget_get_material(b) == R_MATERIAL_TINTED_GLASS);
	CHECK_NEAR(Rwidget_get_metric(a, R_METRIC_CONTROL_HEIGHT), 36.0f);
	CHECK_NEAR(Rwidget_get_metric(l, R_METRIC_CONTROL_HEIGHT), 31.0f);
	Rdraw_frame();

	/* written to a file and read back under another name: the same theme */
	snprintf(saved, sizeof(saved), "%s/raven_test_saved_theme.xml", temp_dir());
	Rtheme_set_color(R_COLOR_WINDOW, 0.25f, 0.5f, 0.75f, 1.0f);
	CHECK(Rtheme_save(saved, "Saved"));
	Rtheme_apply(R_THEME_LIGHT);
	CHECK(color_of(b, R_COLOR_ACCENT) != 0xFF0033FFu);
	name = Rtheme_load(saved);
	CHECK_STR(name, "Saved");
	CHECK(Rtheme_use("Saved"));
	CHECK(theme_color(R_COLOR_WINDOW) == 0x4080BFFFu);
	CHECK(theme_color(R_COLOR_PANEL) == 0x201510FFu);
	CHECK(theme_color(R_COLOR_ACCENT_PRESSED) == 0x102030FFu);
	CHECK_NEAR(Rtheme_get_metric(R_METRIC_CONTROL_HEIGHT), 31.0f);
	CHECK(color_of(b, R_COLOR_ACCENT) == 0xFF0033FFu);
	CHECK(Rwidget_get_material(b) == R_MATERIAL_TINTED_GLASS);
	CHECK_NEAR(Rwidget_get_metric(a, R_METRIC_CONTROL_HEIGHT), 36.0f);
	Rtheme_get_token_color(R_TOKEN_STRING, &r, &g, &bl, &al);
	CHECK(r < 0.01f && g > 0.99f);
	CHECK(Rtheme_is_dark());
	remove(saved);

	/* a theme file is followed like an interface file: saving it changes what uses it */
	snprintf(path, sizeof(path), "%s/raven_test_theme.xml", temp_dir());
	write_text(path, "<Theme Name=\"Live\" Base=\"Light\" Accent=\"#00AA00\"/>");
	CHECK_STR(Rtheme_load(path), "Live");
	CHECK(Rtheme_use("Live"));
	CHECK(!Rtheme_is_dark());
	CHECK(theme_color(R_COLOR_ACCENT) == 0x00AA00FFu);
	CHECK(Rwidget_set_theme(l, "Ember"));
	Rxml_set_reload_callback(on_reload, NULL);
	write_text(path, "<Theme Name=\"Live\" Base=\"Light\" Accent=\"#AA00AA\"><Style Class=\"danger\" Accent=\"#123456\"/></Theme>");
	CHECK(Rxml_check_files() >= 1);
	CHECK(theme_color(R_COLOR_ACCENT) == 0xAA00AAFFu);
	CHECK(color_of(a, R_COLOR_ACCENT) == 0xAA00AAFFu);
	CHECK(color_of(b, R_COLOR_ACCENT) == 0x123456FFu);
	/* (the part of the window that has another theme keeps it) */
	CHECK(color_of(l, R_COLOR_ACCENT) == 0xFF6A00FFu);
	/* a file with a mistake changes nothing */
	EXPECT_ERRORS_BEGIN();
	write_text(path, "<Theme Name=\"Live\" Base=\"Light\" Accent=\"#00FFFF\"");
	Rxml_check_files();
	Rxml_check_files();
	Rxml_check_files();
	Rxml_check_files();
	EXPECT_ERRORS_END(1);
	CHECK(theme_color(R_COLOR_ACCENT) == 0xAA00AAFFu);
	Rxml_set_reload_callback(NULL, NULL);
	remove(path);

	/* mistakes are reported and the rest of the file is used */
	EXPECT_ERRORS_BEGIN();
	name = Rtheme_load_from_memory("<Theme Name=\"Odd\" Base=\"Nope\" Speed=\"3\"><Colors Panel=\"red-ish\" Text=\"#102030\"/><Style Widget=\"Nope\" Accent=\"#fff\"/><Other/></Theme>", 0);
	EXPECT_ERRORS_END(5);
	CHECK_STR(name, "Odd");
	CHECK(Rtheme_use("Odd"));
	CHECK(theme_color(R_COLOR_TEXT) == 0x102030FFu);
	EXPECT_ERRORS_BEGIN();
	CHECK(Rtheme_load_from_memory("<Theme Name=\"Dark\"/>", 0) == NULL);
	CHECK(Rtheme_load_from_memory("<NotATheme/>", 0) == NULL);
	CHECK(Rtheme_load_from_memory("<Theme", 0) == NULL);
	CHECK(Rtheme_load("/no/such/folder/theme.xml") == NULL);
	EXPECT_ERRORS_END(4);

	Rtheme_apply(R_THEME_DARK);
	Rwindow_destroy(window);
}

/* ---------------------------------------------------------------------------
 *  Live reload of interface files
 * ------------------------------------------------------------------------- */

static void test_live_reload(void) {
	char path[512], part[512], library[512];
	Rid window, title, input, list, button, extra, added;

	snprintf(path, sizeof(path), "%s/raven_test_live.xml", temp_dir());
	snprintf(part, sizeof(part), "%s/raven_test_live_part.xml", temp_dir());
	snprintf(library, sizeof(library), "%s/raven_test_live_library.xml", temp_dir());

	CHECK(Rxml_get_live_reload());
	write_text(part, "<Label Name=\"part\">Part 1</Label>");
	write_text(library, "<Widgets><Widget Name=\"LiveCard\" Title=\"t\"><Border><Label Text=\"card {Title}\"/></Border></Widget></Widgets>");
	CHECK(Rxml_register_widgets_from_file(library) == 1);
	write_text(path,
		"<Window Title=\"One\" Width=\"400\" Height=\"300\"><VBox Name=\"box\" Spacing=\"4\">"
		"<Label Name=\"title\">Hello</Label>"
		"<TextInput Name=\"input\" Placeholder=\"Type\"/>"
		"<ListView Name=\"list\"><Item>a</Item><Item>b</Item></ListView>"
		"<Button Name=\"go\" Text=\"Go\"/>"
		"<Label>unnamed 1</Label><Label>unnamed 2</Label>"
		"<Include Source=\"raven_test_live_part.xml\"/>"
		"<LiveCard Name=\"card\" Title=\"A\"/>"
		"</VBox><Shortcut Keys=\"Ctrl+K\" Action=\"k\"/></Window>");
	window = Rwindow_create_from_xml_file(path);
	CHECK(window != R_INVALID_ID);
	title = Rwidget_find(window, "title");
	input = Rwidget_find(window, "input");
	list = Rwidget_find(window, "list");
	button = Rwidget_find(window, "go");
	Rbutton_set_clicked_callback(button, on_click, NULL);
	Rtext_input_set_text(input, "typed by the user");
	Rlist_view_set_selected(list, 1);
	/* something the application added itself */
	added = Rlabel_create("added by the application");
	Rwidget_add_child(Rwidget_find(window, "box"), added);
	Rdraw_frame();
	Rxml_set_reload_callback(on_reload, NULL);
	g_reloads = 0;

	/* nothing changed: nothing happens */
	CHECK(Rxml_check_files() == 0);
	CHECK(g_reloads == 0);

	/* attributes change: the widgets stay, with what the application and the user gave them */
	write_text(path,
		"<Window Title=\"Two\" Width=\"400\" Height=\"300\"><VBox Name=\"box\" Spacing=\"10\">"
		"<Label Name=\"title\">Hello, world</Label>"
		"<TextInput Name=\"input\" Placeholder=\"Type here\"/>"
		"<ListView Name=\"list\"><Item>a</Item><Item>b</Item></ListView>"
		"<Button Name=\"go\" Text=\"Go now\" Class=\"primary\"/>"
		"<Label>unnamed 1</Label><Label>unnamed 2</Label>"
		"<Include Source=\"raven_test_live_part.xml\"/>"
		"<LiveCard Name=\"card\" Title=\"A\"/>"
		"</VBox><Shortcut Keys=\"Ctrl+J\" Action=\"j\"/></Window>");
	CHECK(Rxml_check_files() == 1);
	CHECK(g_reloads == 1 && g_reload_ok);
	CHECK_STR(g_reload_path, path);
	CHECK_STR(Rwindow_get_title(window), "Two");
	CHECK(Rwidget_find(window, "title") == title);
	CHECK(Rwidget_find(window, "input") == input);
	CHECK(Rwidget_find(window, "list") == list);
	CHECK(Rwidget_find(window, "go") == button);
	CHECK_STR(Rlabel_get_text(title), "Hello, world");
	CHECK_STR(Rlabel_get_text(Rwidget_get_child(button, 0)), "Go now");
	CHECK_STR(Rwidget_get_class(button), "primary");
	CHECK_STR(Rtext_input_get_text(input), "typed by the user");
	CHECK(Rlist_view_get_selected(list) == 1);
	CHECK(Rwidget_is_valid(added));
	CHECK_NEAR(Rpanel_get_spacing(Rwidget_find(window, "box")), 10.0f);
	g_clicks = 0;
	Rdraw_frame();
	test_click(window, button);
	CHECK(g_clicks == 1);

	/* elements come, go and move: the others keep their widgets */
	write_text(path,
		"<Window Title=\"Two\" Width=\"400\" Height=\"300\"><VBox Name=\"box\" Spacing=\"10\">"
		"<Button Name=\"go\" Text=\"Go now\" Class=\"primary\"/>"
		"<Label Name=\"new\">New</Label>"
		"<Label Name=\"title\">Hello, world</Label>"
		"<ListView Name=\"list\"><Item>a</Item><Item>b</Item><Item>c</Item></ListView>"
		"<Label>unnamed 0</Label><Label>unnamed 1</Label><Label>unnamed 2</Label>"
		"<Include Source=\"raven_test_live_part.xml\"/>"
		"<LiveCard Name=\"card\" Title=\"A\"/>"
		"</VBox></Window>");
	CHECK(Rxml_check_files() == 1);
	CHECK(g_reloads == 2 && g_reload_ok);
	CHECK(Rwidget_find(window, "go") == button);
	CHECK(Rwidget_find(window, "title") == title);
	CHECK(!Rwidget_is_valid(input));
	CHECK(Rwidget_find(window, "new") != R_INVALID_ID);
	/* (the list has other items: it had to be built again) */
	CHECK(!Rwidget_is_valid(list));
	list = Rwidget_find(window, "list");
	CHECK(Rlist_view_get_item_count(list) == 3);
	{
		Rid box = Rwidget_find(window, "box");
		CHECK(Rwidget_get_child(box, 0) == button);
		CHECK(Rwidget_get_child(box, 1) == Rwidget_find(window, "new"));
		CHECK(Rwidget_get_child(box, 2) == title);
		CHECK(Rwidget_get_child(box, 3) == list);
		CHECK_STR(Rlabel_get_text(Rwidget_get_child(box, 4)), "unnamed 0");
		CHECK_STR(Rlabel_get_text(Rwidget_get_child(box, 5)), "unnamed 1");
		CHECK_STR(Rlabel_get_text(Rwidget_get_child(box, 6)), "unnamed 2");
		CHECK(Rwidget_is_valid(added));
		CHECK(Rwidget_get_child_count(box) == 10);
	}
	g_clicks = 0;
	Rdraw_frame();
	test_click(window, button);
	CHECK(g_clicks == 1);

	/* a file it includes changes */
	extra = Rwidget_find(window, "part");
	write_text(part, "<Label Name=\"part\">Part two</Label>");
	CHECK(Rxml_check_files() == 1);
	CHECK(g_reloads == 3);
	CHECK(Rwidget_find(window, "part") == extra);
	CHECK_STR(Rlabel_get_text(extra), "Part two");

	/* the file of a widget it uses changes */
	write_text(library, "<Widgets><Widget Name=\"LiveCard\" Title=\"t\"><Border><Label Name=\"inner\" Text=\"new card {Title}\"/></Border></Widget></Widgets>");
	CHECK(Rxml_check_files() == 1);
	CHECK_STR(Rlabel_get_text(Rwidget_find(window, "inner")), "new card A");
	CHECK(Rwidget_find(window, "go") == button);

	/* a file with a mistake leaves everything as it is */
	EXPECT_ERRORS_BEGIN();
	write_text(path, "<Window Title=\"Broken\"><VBox><Label>half");
	Rxml_check_files();
	Rxml_check_files();
	Rxml_check_files();
	Rxml_check_files();
	write_text(path, "<Window Title=\"Wrong tag\"><VBox><NoSuchWidget/></VBox></Window>");
	Rxml_check_files();
	EXPECT_ERRORS_END(2);
	CHECK(!g_reload_ok);
	CHECK_STR(Rwindow_get_title(window), "Two");
	CHECK(Rwidget_find(window, "go") == button);
	CHECK(Rwidget_is_valid(title));

	/* and the next good one is taken */
	write_text(path, "<Window Title=\"Three\" Width=\"400\" Height=\"300\"><HBox Name=\"box\"><Button Name=\"go\" Text=\"Again\"/></HBox></Window>");
	CHECK(Rxml_check_files() == 1);
	CHECK(g_reload_ok);
	CHECK_STR(Rwindow_get_title(window), "Three");
	CHECK(!Rwidget_is_valid(button));
	CHECK(!Rwidget_is_valid(added));
	CHECK(Rwidget_get_type(Rwindow_get_content(window)) == R_WIDGET_HBOX);
	Rdraw_frame();

	/* turned off: the files are no longer looked at by themselves, but can still be checked by hand */
	Rxml_set_live_reload(R_FALSE);
	CHECK(!Rxml_get_live_reload());
	Rxml_set_live_reload(R_TRUE);

	/* the window is gone: its file is forgotten */
	Rwindow_destroy(window);
	write_text(path, "<Window Title=\"Four\" Width=\"400\" Height=\"300\"><HBox/></Window>");
	g_reloads = 0;
	Rxml_check_files();
	CHECK(g_reloads == 0);

	/* a widget made from a file follows it too, where it was put */
	{
		Rid host = Rwindow_create("Host", 300, 200, R_WINDOW_DEFAULT);
		Rid box = Rwidget_create(R_WIDGET_VBOX);
		Rid piece, label;
		Rwindow_set_content(host, box);
		write_text(part, "<HBox Name=\"piece\"><Label Name=\"p\">one</Label></HBox>");
		piece = Rwidget_create_from_xml_file(part);
		Rwidget_add_child(box, Rlabel_create("before"));
		Rwidget_add_child(box, piece);
		Rwidget_add_child(box, Rlabel_create("after"));
		label = Rwidget_find(piece, "p");
		write_text(part, "<HBox Name=\"piece\" Spacing=\"7\"><Label Name=\"p\">one more</Label><Label>two</Label></HBox>");
		Rxml_check_files();
		CHECK(Rwidget_is_valid(piece));
		CHECK_STR(Rlabel_get_text(label), "one more");
		CHECK(Rwidget_get_child_count(piece) == 2);
		/* its root becomes another kind of widget: a new one takes its place */
		write_text(part, "<VBox Name=\"piece\"><Label Name=\"p\">changed</Label></VBox>");
		Rxml_check_files();
		CHECK(!Rwidget_is_valid(piece));
		CHECK(Rwidget_get_child_count(box) == 3);
		piece = Rwidget_get_child(box, 1);
		CHECK(Rwidget_get_type(piece) == R_WIDGET_VBOX);
		CHECK_STR(Rlabel_get_text(Rwidget_find(piece, "p")), "changed");
		Rwindow_destroy(host);
	}

	Rxml_set_reload_callback(NULL, NULL);
	Rxml_unregister_widget("LiveCard");
	remove(path);
	remove(part);
	remove(library);
}

/* ---------------------------------------------------------------------------
 *  Looks
 * ------------------------------------------------------------------------- */

static void test_looks(void) {
	Rid window, button;
	const char* name;
	char saved[512];
	char* text;
	size_t size = 0;
	float r, g, b, a;

	/* (the theme files of the tests before: gone) */
	Rtheme_apply(R_THEME_DARK);
	CHECK(Rlook_get() == R_LOOK_RAVEN_DARK);
	/* a look of a platform: its colors, its sizes, opaque */
	Rlook_set(R_LOOK_WINDOWS_LIGHT);
	CHECK(Rlook_get() == R_LOOK_WINDOWS_LIGHT);
	CHECK_STR(Rtheme_get_name(), "WindowsLight");
	CHECK(!Rtheme_is_dark());
	CHECK(theme_color(R_COLOR_WINDOW) == 0xF3F3F3FFu);
	CHECK_NEAR(Rtheme_get_metric(R_METRIC_CONTROL_HEIGHT), 32.0f);
	CHECK(Rtheme_get_material() == R_MATERIAL_SOLID);
	/* no glass on Windows: said, and nothing changes */
	EXPECT_ERRORS_BEGIN();
	Rtheme_set_material(R_MATERIAL_FROSTED_GLASS);
	EXPECT_ERRORS_END(1);
	CHECK(Rtheme_get_material() == R_MATERIAL_SOLID);
	Rlook_set(R_LOOK_WINDOWS_DARK);
	CHECK(Rtheme_is_dark());
	CHECK(theme_color(R_COLOR_WINDOW) == 0x202020FFu);
	/* the accent of Windows: the shades are the accent seen through */
	Rtheme_set_accent(1.0f, 0.0f, 0.0f);
	Rtheme_get_color(R_COLOR_ACCENT_HOVERED, &r, &g, &b, &a);
	CHECK(r > 0.99f && a < 0.95f && a > 0.85f);

	/* the old names and the new ones */
	Rtheme_apply(R_THEME_GLASS_LIGHT);
	CHECK(Rlook_get() == R_LOOK_RAVEN_GLASS_LIGHT);
	CHECK_STR(Rtheme_get_name(), "GlassLight");
	CHECK(Rtheme_use("WindowsDark"));
	CHECK(Rlook_get() == R_LOOK_WINDOWS_DARK);
	/* the look of the platform: Windows on Windows, Raven elsewhere (for now) */
	Rlook_set(R_LOOK_NATIVE_LIGHT);
	CHECK(Rlook_get() == R_LOOK_NATIVE_LIGHT);
	CHECK(!Rtheme_is_dark());
#if defined(_WIN32)
	CHECK_STR(Rtheme_get_name(), "WindowsLight");
#elif defined(__APPLE__)
	CHECK_STR(Rtheme_get_name(), "MacOSLight");
#else
	CHECK_STR(Rtheme_get_name(), "GnomeLight");
#endif
	/* macOS and GNOME: their colors and sizes, opaque */
	Rlook_set(R_LOOK_MACOS_LIGHT);
	CHECK_STR(Rtheme_get_name(), "MacOSLight");
	CHECK(theme_color(R_COLOR_WINDOW) == 0xECECECFFu);
	CHECK_NEAR(Rtheme_get_metric(R_METRIC_CONTROL_HEIGHT), 22.0f);
	CHECK(Rtheme_get_material() == R_MATERIAL_SOLID);
	Rlook_set(R_LOOK_MACOS_DARK);
	CHECK(Rtheme_is_dark());
	CHECK(theme_color(R_COLOR_WINDOW) == 0x323232FFu);
	CHECK(Rtheme_use("GnomeDark"));
	CHECK(Rlook_get() == R_LOOK_GNOME_DARK);
	CHECK(Rtheme_is_dark());
	CHECK(theme_color(R_COLOR_WINDOW) == 0x222226FFu);
	CHECK(Rtheme_use("Adwaita"));
	CHECK(Rlook_get() == R_LOOK_GNOME_LIGHT);
	CHECK(theme_color(R_COLOR_WINDOW) == 0xFAFAFAFFu);
	CHECK(Rtheme_use("macOS"));
	CHECK(Rlook_get() == R_LOOK_MACOS_LIGHT);
	/* the accent of macOS: no change under the pointer */
	Rtheme_set_accent(1.0f, 0.0f, 0.0f);
	CHECK(theme_color(R_COLOR_ACCENT_HOVERED) == 0xFF0000FFu);
	{
		/* every look draws a window of every widget */
		static const RLook looks[] = { R_LOOK_WINDOWS_DARK, R_LOOK_WINDOWS_LIGHT, R_LOOK_MACOS_DARK, R_LOOK_MACOS_LIGHT,
			R_LOOK_GNOME_DARK, R_LOOK_GNOME_LIGHT };
		const char* all =
			"<Window Width=\"500\" Height=\"400\"><VBox><HeaderBar/><MenuBar><Menu Text=\"File\"><MenuItem Text=\"Open\"/></Menu></MenuBar>"
			"<Button Text=\"A\" Style=\"Primary\"/><Button Text=\"B\" Style=\"Flat\"/><Checkbox Text=\"C\" Checked=\"true\"/>"
			"<Checkbox Style=\"Radio\" Checked=\"true\"/><Checkbox Style=\"Switch\"/><Slider Value=\"0.5\"/><ProgressBar Value=\"0.5\"/>"
			"<TextInput Placeholder=\"P\"/><ComboBox Items=\"X|Y\" Selected=\"0\"/><SpinBox Value=\"3\"/>"
			"<TabView Height=\"60\"><Label Slot.Title=\"T1\"/><Label Slot.Title=\"T2\"/></TabView>"
			"<ListView Items=\"1|2\" Height=\"60\"/><TreeView Height=\"60\"><Item Text=\"I\"><Item Text=\"J\"/></Item></TreeView>"
			"<Expander Title=\"E\" Expanded=\"true\"><Label Text=\"in\"/></Expander></VBox></Window>";
		Rid all_window = Rwindow_create_from_xml(all, strlen(all));
		size_t k;
		CHECK(Rwindow_is_valid(all_window));
		for (k = 0; k < sizeof(looks) / sizeof(looks[0]); k++) {
			Rlook_set(looks[k]);
			Rdraw_frame();
		}
		Rwindow_destroy(all_window);
	}
	Rlook_set(R_LOOK_NATIVE_LIGHT);
	EXPECT_ERRORS_BEGIN();
	Rlook_set((RLook)0x1234);
	EXPECT_ERRORS_END(1);

	/* a theme file that names no look is laid over the look in use, and follows it */
	Rlook_set(R_LOOK_RAVEN_DARK);
	name = Rtheme_load_from_memory("<Theme Name=\"Follow\" Accent=\"#FF0000\"><Colors Panel=\"#123456\"/></Theme>", 0);
	CHECK_STR(name, "Follow");
	CHECK(Rtheme_use("Follow"));
	CHECK(theme_color(R_COLOR_PANEL) == 0x123456FFu);
	CHECK(theme_color(R_COLOR_ACCENT) == 0xFF0000FFu);
	CHECK(theme_color(R_COLOR_WINDOW) == 0x1E1E20FFu);
	Rlook_set(R_LOOK_WINDOWS_LIGHT);
	CHECK_STR(Rtheme_get_name(), "Follow");
	CHECK(theme_color(R_COLOR_PANEL) == 0x123456FFu);
	CHECK(theme_color(R_COLOR_ACCENT) == 0xFF0000FFu);
	CHECK(theme_color(R_COLOR_WINDOW) == 0xF3F3F3FFu);
	CHECK_NEAR(Rtheme_get_metric(R_METRIC_CONTROL_HEIGHT), 32.0f);
	/* a theme that names its look imposes it */
	name = Rtheme_load_from_memory("<Theme Name=\"Fixed\" Look=\"RavenLight\"/>", 0);
	CHECK(Rtheme_use("Fixed"));
	CHECK(theme_color(R_COLOR_WINDOW) == 0xF3F3F5FFu);
	/* a theme cannot take the name of a look */
	EXPECT_ERRORS_BEGIN();
	CHECK(Rtheme_load_from_memory("<Theme Name=\"WindowsDark\"/>", 0) == NULL);
	EXPECT_ERRORS_END(1);
	/* a theme file asking a look of a platform for glass: said, and opaque */
	EXPECT_ERRORS_BEGIN();
	CHECK(Rtheme_load_from_memory("<Theme Name=\"GlassWin\" Look=\"WindowsDark\" Material=\"FrostedGlass\"/>", 0) != NULL);
	EXPECT_ERRORS_END(1);
	CHECK(Rtheme_use("GlassWin"));
	CHECK(Rtheme_get_material() == R_MATERIAL_SOLID);

	/* parts and states: read, kept, written back */
	EXPECT_ERRORS_BEGIN();
	name = Rtheme_load_from_memory(
		"<Theme Name=\"Parts\" Look=\"WindowsLight\" Motion=\"Springy\">"
		"<Style Widget=\"Slider\" Part=\"Thumb\" State=\"Hovered Pressed\" Background=\"#FF0000\" Size=\"24\"/>"
		"<Style Widget=\"Button\" Class=\"danger\" Background=\"#C42B1C\" Foreground=\"#FFFFFF\"/>"
		"<Style Part=\"Item\" State=\"Selected\" BorderColor=\"#00FF00\" CornerRadius=\"2\"/>"
		"<Style Part=\"Nope\" Background=\"#000\"/>"
		"<Style State=\"Sleepy\" Background=\"#000\"/>"
		"<Style Part=\"Thumb\" Accent=\"#000\"/>"
		"</Theme>", 0);
	EXPECT_ERRORS_END(3);
	CHECK_STR(name, "Parts");
	CHECK(Rtheme_use("Parts"));
	window = Rwindow_create_from_xml("<Window Width=\"300\" Height=\"200\"><VBox><Button Name=\"b\" Class=\"danger\" Text=\"Delete\"/>"
		"<Slider/><ListView Items=\"A|B\"/></VBox></Window>", strlen("<Window Width=\"300\" Height=\"200\"><VBox><Button Name=\"b\" Class=\"danger\" Text=\"Delete\"/>"
		"<Slider/><ListView Items=\"A|B\"/></VBox></Window>"));
	CHECK(Rwindow_is_valid(window));
	button = Rwidget_find(window, "b");
	CHECK(button != R_INVALID_ID);
	Rdraw_frame();
	snprintf(saved, sizeof(saved), "%s/raven_test_parts_theme.xml", temp_dir());
	CHECK(Rtheme_save(saved, "PartsAgain"));
	text = Rfile_read(saved, &size);
	CHECK(text != NULL);
	if (text) {
		CHECK(strstr(text, "Look=\"WindowsLight\"") != NULL);
		CHECK(strstr(text, "Motion=\"Springy\"") != NULL);
		CHECK(strstr(text, "Part=\"Thumb\" State=\"Hovered Pressed\"") != NULL);
		CHECK(strstr(text, "Background=\"#FF0000\"") != NULL);
		CHECK(strstr(text, "Size=\"24\"") != NULL);
		CHECK(strstr(text, "Class=\"danger\" Background=\"#C42B1C\" Foreground=\"#FFFFFF\"") != NULL);
		CHECK(strstr(text, "Part=\"Item\" State=\"Selected\"") != NULL);
		Rmemory_free(text);
	}
	CHECK_STR(Rtheme_load(saved), "PartsAgain");
	CHECK(Rtheme_use("PartsAgain"));
	Rdraw_frame();
	remove(saved);
	Rwindow_destroy(window);

	/* back to Raven: the font of Raven comes back */
	Rlook_set(R_LOOK_RAVEN_DARK);
	CHECK(Rtheme_get_font() == Rfont_get_default());
	CHECK_STR(Rtheme_get_name(), "Dark");
}

int main(void) {
	if (!test_init(0)) {
		printf("test_themes: cannot initialize Raven\n");
		return 1;
	}
	RUN(test_styles);
	RUN(test_accent_and_scopes);
	RUN(test_theme_files);
	RUN(test_live_reload);
	RUN(test_looks);
	Rshutdown();
	return test_report("test_themes");
}

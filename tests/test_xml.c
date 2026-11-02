/**
 * Raven tests - XML: parsing, building windows and widgets, error reporting.
 */

#include "test.h"

#include <stdlib.h>

static int g_actions;
static Rid g_sender;

static void on_action(Rid sender, const char* action, void* user_data) {
	(void)action;
	(void)user_data;
	g_actions++;
	g_sender = sender;
}

static Rid widget_from(const char* xml) {
	return Rwidget_create_from_xml(xml, strlen(xml));
}

static void test_window(void) {
	static const char xml[] =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<!-- a small editor -->\n"
		"<Window Title=\"Editor &amp; tools\" Width=\"640\" Height=\"480\" ClearColor=\"#102030\">\n"
		"  <Shortcut Keys=\"Ctrl+S\" Action=\"file.save\"/>\n"
		"  <VBox Spacing=\"4\">\n"
		"    <MenuBar>\n"
		"      <Menu Text=\"File\">\n"
		"        <MenuItem Name=\"open\" Text=\"Open...\" Shortcut=\"Ctrl+O\" OnClicked=\"file.open\"/>\n"
		"        <Separator/>\n"
		"        <Menu Text=\"Recent\"><MenuItem Text=\"a.lynx\"/></Menu>\n"
		"      </Menu>\n"
		"    </MenuBar>\n"
		"    <HBox Slot.Fill=\"1\" Spacing=\"8\">\n"
		"      <Border Name=\"side\" Width=\"200\" Padding=\"8,4\" Background=\"#202020\" CornerRadius=\"6\">\n"
		"        <VBox>\n"
		"          <Label Name=\"title\">Scene &lt;1&gt;</Label>\n"
		"          <Button Name=\"save\" Text=\"Save\" Style=\"Primary\" OnClicked=\"file.save\" Slot.HAlign=\"Left\"/>\n"
		"          <Checkbox Name=\"grid\" Text=\"Grid\" Checked=\"true\"/>\n"
		"          <Slider Name=\"zoom\" Min=\"1\" Max=\"5\" Value=\"2.5\"/>\n"
		"          <ComboBox Name=\"quality\" Selected=\"1\"><Item Text=\"Low\"/><Item>High</Item></ComboBox>\n"
		"          <TreeView Name=\"tree\" Slot.Fill=\"1\">\n"
		"            <Item Text=\"World\" Expanded=\"true\"><Item Text=\"Player\"/></Item>\n"
		"          </TreeView>\n"
		"        </VBox>\n"
		"      </Border>\n"
		"      <Viewport Name=\"scene\" Slot.Fill=\"1\"/>\n"
		"    </HBox>\n"
		"  </VBox>\n"
		"</Window>\n";
	Rid window = Rwindow_create_from_xml(xml, sizeof(xml) - 1);
	Rid side, title, save, scene, tree, quality;
	int w = 0, h = 0, pw = 0, ph = 0;
	float x, y, fw, fh;

	CHECK(Rwindow_is_valid(window));
	if (!Rwindow_is_valid(window)) return;
	Rwindow_get_size(window, &w, &h);
	CHECK(w == 640 && h == 480);

	side = Rwidget_find(window, "side");
	title = Rwidget_find(window, "title");
	save = Rwidget_find(window, "save");
	scene = Rwidget_find(window, "scene");
	tree = Rwidget_find(window, "tree");
	quality = Rwidget_find(window, "quality");
	CHECK(Rwidget_get_type(side) == R_WIDGET_BORDER);
	CHECK(Rwidget_get_type(scene) == R_WIDGET_VIEWPORT);

	/* entities and text content */
	CHECK_STR(Rlabel_get_text(title), "Scene <1>");
	CHECK(Rcheckbox_is_checked(Rwidget_find(window, "grid")));
	CHECK_NEAR(Rslider_get_value(Rwidget_find(window, "zoom")) * 10, 25);
	CHECK(Rcombo_box_get_item_count(quality) == 2 && Rcombo_box_get_selected(quality) == 1);
	CHECK_STR(Rcombo_box_get_item_text(quality, 1), "High");
	CHECK(Rtree_view_get_item_count(tree) == 1);
	CHECK_STR(Rtree_item_get_text(Rtree_view_get_item(tree, 0)), "World");
	CHECK(Rtree_item_is_expanded(Rtree_view_get_item(tree, 0)));
	CHECK_STR(Rtree_item_get_text(Rtree_item_get_child(Rtree_view_get_item(tree, 0), 0)), "Player");

	/* layout described by the attributes */
	Rwidget_get_rect(side, &x, &y, &fw, &fh);
	CHECK_NEAR(fw, 200);
	CHECK_NEAR(x, 0);
	Rwidget_get_rect(scene, &x, &y, &fw, &fh);
	CHECK_NEAR(x, 208);
	CHECK_NEAR(fw, 640 - 208);
	Rviewport_get_pixel_size(scene, &pw, &ph);
	CHECK(pw == 640 - 208 && ph > 400);
	Rwidget_get_rect(title, &x, &y, &fw, &fh);
	CHECK_NEAR(x, 8);

	/* actions: button click, window shortcut, menu shortcut */
	Raction_bind("file.save", on_action, NULL);
	Raction_bind("file.open", on_action, NULL);
	g_actions = 0;
	test_click(window, save);
	CHECK(g_actions == 1 && g_sender == save);
	test_key(window, R_KEY_S, R_MOD_CTRL);
	CHECK(g_actions == 2 && g_sender == window);
	test_key(window, R_KEY_O, R_MOD_CTRL);
	CHECK(g_actions == 3 && g_sender == Rwidget_find(window, "open"));

	Rwindow_destroy(window);
	CHECK(!Rwindow_is_valid(window));
	CHECK(!Rwidget_is_valid(save));
}

static void test_widget_fragment(void) {
	Rid grid = widget_from(
		"<Grid Columns='auto,1' Rows='auto,auto' Spacing='2'>"
		"  <Label Text='Name' Slot.Column='0' Slot.Row='0'/>"
		"  <TextInput Name='name' Placeholder='type' Slot.Column='1' Slot.Row='0'/>"
		"  <ProgressBar Value='0.25' Slot.Column='0' Slot.Row='1' Slot.ColumnSpan='2'/>"
		"</Grid>");
	Rid tabs, canvas;
	CHECK(Rwidget_get_type(grid) == R_WIDGET_GRID);
	CHECK(Rwidget_get_child_count(grid) == 3);
	CHECK(Rwidget_get_type(Rwidget_find(grid, "name")) == R_WIDGET_TEXT_INPUT);
	CHECK_NEAR(Rprogress_bar_get_value(Rwidget_get_child(grid, 2)) * 100, 25);
	Rwidget_destroy(grid);

	/* tags and attribute names are not case sensitive; Slate names are accepted */
	tabs = widget_from(
		"<tabview activetab='1'>"
		"  <VerticalBox slot.title='General'><textblock text='a'/></VerticalBox>"
		"  <ScrollBox Slot.Title='Advanced' orientation='horizontal'><Spacer Width='10' Height='10'/></ScrollBox>"
		"</tabview>");
	CHECK(Rwidget_get_type(tabs) == R_WIDGET_TAB_VIEW);
	CHECK(Rtab_view_get_active(tabs) == 1);
	CHECK(Rwidget_get_type(Rwidget_get_child(tabs, 0)) == R_WIDGET_VBOX);
	Rwidget_destroy(tabs);

	canvas = widget_from(
		"<Canvas><Image Slot.Anchors='1,0' Slot.Alignment='1,0' Slot.Position='-4,4' Slot.Size='32,32'/>"
		"<![CDATA[ ignored <text> ]]></Canvas>");
	CHECK(Rwidget_get_child_count(canvas) == 1);
	Rwidget_destroy(canvas);
}

static void test_errors(void) {
	EXPECT_ERRORS_BEGIN();
	/* malformed documents */
	CHECK(widget_from("<VBox><Label></VBox>") == R_INVALID_ID);
	CHECK(widget_from("<VBox") == R_INVALID_ID);
	CHECK(widget_from("") == R_INVALID_ID);
	CHECK(widget_from("<VBox attr=unquoted/>") == R_INVALID_ID);
	CHECK(widget_from("<VBox/><VBox/>") == R_INVALID_ID);
	CHECK(widget_from("<VBox>&bogus;</VBox>") == R_INVALID_ID);
	/* unknown widget, child in a widget that takes none, wrong root */
	CHECK(widget_from("<VBox><Blink/></VBox>") == R_INVALID_ID);
	CHECK(widget_from("<Label><Button/></Label>") == R_INVALID_ID);
	CHECK(widget_from("<Window/>") == R_INVALID_ID);
	CHECK(Rwindow_create_from_xml("<VBox/>", 7) == R_INVALID_ID);
	CHECK(Rwindow_create_from_xml("<Window><VBox/><VBox/></Window>", 31) == R_INVALID_ID);
	CHECK(Rwidget_create_from_xml_file("/no/such/file.xml") == R_INVALID_ID);
	EXPECT_ERRORS_END(12);

	/* an unknown attribute is reported but does not stop the loading */
	{
		const char* detail = NULL;
		Rid label;
		EXPECT_ERRORS_BEGIN();
		label = widget_from("<VBox>\n  <Label Text=\"ok\" Colour=\"red\"/>\n</VBox>");
		EXPECT_ERRORS_END(1);
		CHECK(Rwidget_is_valid(label));
		CHECK(Rerror_get_last(&detail, NULL) == R_ERROR_XML);
		/* the message carries the position of the element */
		CHECK(detail && strstr(detail, "line 2") != NULL);
		CHECK(detail && strstr(detail, "Colour") != NULL);
		Rwidget_destroy(label);
	}
	/* nothing leaked from the failed documents: no widget is left */
	CHECK(Rerror_get_last(NULL, NULL) == R_NO_ERROR);
}

/* ---------------------------------------------------------------------------
 *  Widgets declared in XML
 * ------------------------------------------------------------------------- */

static int register_text(const char* xml) {
	return Rxml_register_widgets(xml, strlen(xml));
}

static const char* label_of(Rid scope, const char* name) {
	Rid label = Rwidget_find(scope, name);
	return label != R_INVALID_ID ? Rlabel_get_text(label) : "(not found)";
}

static void test_components(void) {
	static const char library[] =
		"<Widgets>"
		"  <Widget Name=\"Field\" Label=\"Name\" Hint=\"\" Wide=\"false\">"
		"    <HBox Spacing=\"8\">"
		"      <Label Name=\"label\" Text=\"{Label}:\" Width=\"{Wide ? 140 : 90}\"/>"
		"      <TextInput Name=\"input\" Placeholder=\"{Hint}\" Slot.Fill=\"1\"/>"
		"      <Children/>"
		"    </HBox>"
		"  </Widget>"
		"  <Widget Name=\"Card\" Title=\"Untitled\" Count=\"0\">"
		"    <Border Padding=\"10\">"
		"      <VBox Name=\"box\">"
		"        <Label Name=\"title\" Text=\"{upper(Title)} ({Count * 2 + 1})\"/>"
		"        <Label Name=\"badge\" If=\"{Count &gt; 2}\">many</Label>"
		"        <Children><Label Name=\"empty\">Nothing here</Label></Children>"
		"        <HBox Name=\"footer\"><Children Name=\"footer\"/></HBox>"
		"      </VBox>"
		"    </Border>"
		"  </Widget>"
		"</Widgets>";
	Rid window, row, field, card, box, footer;

	CHECK(!Rxml_is_widget_registered("Field"));
	CHECK(Rxml_register_widgets(library, sizeof(library) - 1) == 2);
	CHECK(Rxml_is_widget_registered("Field"));
	CHECK(Rxml_is_widget_registered("card"));
	CHECK(Rxml_get_registered_widget_count() == 2);
	CHECK_STR(Rxml_get_registered_widget_name(0), "Field");
	CHECK(Rxml_get_registered_widget_name(2) == NULL);

	/* the new tags are used like the built-in ones */
	{
		const char* xml =
			"<Window Width=\"400\" Height=\"300\">"
			"<VBox Name=\"rows\">"
			"<Field Name=\"email\" Label=\"Email\" Hint=\"you@example.com\" Slot.Padding=\"4\"/>"
			"<Field Name=\"password\" Label=\"Password\" Wide=\"true\"><Button Name=\"show\" Text=\"Show\"/></Field>"
			"<Field Name=\"plain\"/>"
			"</VBox></Window>";
		window = Rwindow_create_from_xml(xml, strlen(xml));
	}
	CHECK(Rwindow_is_valid(window));
	row = Rwidget_find(window, "email");
	/* the tag became the root of its body, and took the attributes that are not parameters */
	CHECK(Rwidget_get_type(row) == R_WIDGET_HBOX);
	CHECK(Rwidget_get_child_count(row) == 2);
	CHECK_STR(label_of(row, "label"), "Email:");
	CHECK_STR(Rtext_input_get_text(Rwidget_find(row, "input")), "");
	CHECK_RECT(row, 4, 4, 392, Rtheme_get_metric(R_METRIC_CONTROL_HEIGHT));
	CHECK_RECT(Rwidget_find(row, "label"), 4, 4, 90, Rtheme_get_metric(R_METRIC_CONTROL_HEIGHT));
	/* an expression of a parameter, and children given at the use */
	field = Rwidget_find(window, "password");
	CHECK_STR(label_of(field, "label"), "Password:");
	{
		float x, y, w, h;
		Rwidget_get_rect(Rwidget_find(field, "label"), &x, &y, &w, &h);
		CHECK_NEAR(w, 140);
	}
	CHECK(Rwidget_get_child_count(field) == 3);
	CHECK(Rwidget_get_parent(Rwidget_find(window, "show")) == field);
	/* default values */
	CHECK_STR(label_of(Rwidget_find(window, "plain"), "label"), "Name:");
	Rwindow_destroy(window);

	/* expressions, conditions, default content and named places for children */
	card = widget_from("<Card Title=\"Hello\" Count=\"3\"><Button Name=\"ok\" Text=\"OK\" Into=\"footer\"/><Label Name=\"body\">Body</Label></Card>");
	CHECK(Rwidget_get_type(card) == R_WIDGET_BORDER);
	box = Rwidget_find(card, "box");
	footer = Rwidget_find(card, "footer");
	CHECK_STR(label_of(card, "title"), "HELLO (7)");
	CHECK(Rwidget_find(card, "badge") != R_INVALID_ID);
	CHECK(Rwidget_find(card, "empty") == R_INVALID_ID);
	CHECK(Rwidget_get_parent(Rwidget_find(card, "body")) == box);
	CHECK(Rwidget_get_parent(Rwidget_find(card, "ok")) == footer);
	CHECK(Rwidget_get_child_count(footer) == 1);
	Rwidget_destroy(card);

	card = widget_from("<Card/>");
	CHECK_STR(label_of(card, "title"), "UNTITLED (1)");
	CHECK(Rwidget_find(card, "badge") == R_INVALID_ID);
	CHECK(Rwidget_find(card, "empty") != R_INVALID_ID);
	CHECK(Rwidget_get_child_count(Rwidget_find(card, "footer")) == 0);
	Rwidget_destroy(card);

	/* a declared widget uses another one, and passes its own children on */
	{
		const char* xml =
			"<VBox>"
			"<Widget Name=\"Titled\" Title=\"?\" Level=\"1\">"
			"  <Card Title=\"{Title} {Level}\" Count=\"{Level + 1}\"><Children/></Card>"
			"</Widget>"
			"<Titled Name=\"t\" Title=\"Deep\" Level=\"2\"><Label Name=\"inner\">in</Label></Titled>"
			"</VBox>";
		Rid root = widget_from(xml);
		CHECK(Rwidget_get_child_count(root) == 1);
		card = Rwidget_find(root, "t");
		CHECK(Rwidget_get_type(card) == R_WIDGET_BORDER);
		CHECK_STR(label_of(card, "title"), "DEEP 2 (7)");
		CHECK(Rwidget_find(card, "badge") != R_INVALID_ID);
		CHECK(Rwidget_get_parent(Rwidget_find(card, "inner")) == Rwidget_find(card, "box"));
		Rwidget_destroy(root);
		CHECK(Rxml_is_widget_registered("Titled"));
	}

	/* repetitions */
	{
		const char* xml =
			"<VBox>"
			"<Widget Name=\"Stars\" Count=\"3\" Names=\"a|b\">"
			"  <HBox>"
			"    <Repeat Count=\"{Count}\" Var=\"i\"><Label Name=\"star{i}\" Text=\"{i + 1}/{Count}\"/></Repeat>"
			"    <ForEach Items=\"{Names}\" Var=\"n\"><Button Name=\"b_{n}\" Text=\"{Index}:{n}\" If=\"{n != 'skip'}\"/></ForEach>"
			"    <If Test=\"{Count == 0}\"><Label Name=\"none\">none</Label></If>"
			"    <Label Name=\"brace\" Text=\"{{literal}} {'}'}\"/>"
			"  </HBox>"
			"</Widget>"
			"<Stars Name=\"s3\"/>"
			"<Stars Name=\"s0\" Count=\"0\" Names=\"x|skip|y\"/>"
			"</VBox>";
		Rid root = widget_from(xml), s3, s0;
		s3 = Rwidget_find(root, "s3");
		s0 = Rwidget_find(root, "s0");
		CHECK(Rwidget_get_child_count(s3) == 3 + 2 + 1);
		CHECK_STR(label_of(s3, "star0"), "1/3");
		CHECK_STR(label_of(s3, "star2"), "3/3");
		CHECK(Rwidget_find(s3, "b_a") != R_INVALID_ID && Rwidget_find(s3, "b_b") != R_INVALID_ID);
		CHECK(Rwidget_find(s3, "none") == R_INVALID_ID);
		CHECK_STR(label_of(s3, "brace"), "{literal} }");
		CHECK(Rwidget_get_child_count(s0) == 2 + 1 + 1);
		CHECK(Rwidget_find(s0, "b_x") != R_INVALID_ID && Rwidget_find(s0, "b_y") != R_INVALID_ID);
		CHECK(Rwidget_find(s0, "b_skip") == R_INVALID_ID);
		CHECK(Rwidget_find(s0, "none") != R_INVALID_ID);
		Rwidget_destroy(root);
	}

	/* outside of a declaration, braces are only braces */
	{
		Rid label = widget_from("<Label>int main() { return {0}; }</Label>");
		CHECK_STR(Rlabel_get_text(label), "int main() { return {0}; }");
		Rwidget_destroy(label);
	}

	/* the text between the tags of a use is its Text */
	{
		const char* xml = "<VBox><Widget Name=\"Shout\" Text=\"\"><Label Text=\"{Text}!\"/></Widget>"
			"<Widget Name=\"Plain\"><Label/></Widget><Shout Name=\"a\">hey</Shout><Plain Name=\"b\">ho</Plain></VBox>";
		Rid root = widget_from(xml);
		CHECK_STR(label_of(root, "a"), "hey!");
		CHECK_STR(label_of(root, "b"), "ho");
		Rwidget_destroy(root);
	}

	/* mistakes are reported and leave nothing behind */
	EXPECT_ERRORS_BEGIN();
	CHECK(widget_from("<VBox><Widget><Label/></Widget></VBox>") != R_INVALID_ID);
	CHECK(register_text("<Widget Name=\"Button\"><Label/></Widget>") == 0);
	CHECK(register_text("<Widget Name=\"Empty\"/>") == 0);
	CHECK(register_text("<Widget Name=\"1x\"><Label/></Widget>") == 0);
	CHECK(register_text("<Widget Name=\"Loop\"><Loop/></Widget>") == 1);
	CHECK(widget_from("<Loop/>") == R_INVALID_ID);
	CHECK(register_text("<Widget Name=\"Two\"><Label/><Label/></Widget>") == 1);
	CHECK(widget_from("<Two/>") == R_INVALID_ID);
	CHECK(widget_from("<Widget Name=\"OnlyDeclared\"><Label/></Widget>") == R_INVALID_ID);
	CHECK(register_text("<Widget Name=\"Broken\"") == 0);
	EXPECT_ERRORS_END(8);
	/* If="..." on the use of a declared tag keeps or drops it, and is not passed on to the widget (no warning) */
	{
		const char* xml = "<VBox><Widget Name=\"IfChip\" T=\"\"><Label Text=\"{T}\"/></Widget>"
			"<Widget Name=\"IfHost\" Show=\"\"><HBox><IfChip If=\"{!empty(Show)}\" T=\"{Show}\" Name=\"chip\"/></HBox></Widget>"
			"<IfHost Name=\"with\" Show=\"yes\"/><IfHost Name=\"without\"/></VBox>";
		Rid root = widget_from(xml);
		CHECK(root != R_INVALID_ID);
		CHECK(Rwidget_get_child_count(Rwidget_find(root, "with")) == 1);
		CHECK_STR(label_of(Rwidget_find(root, "with"), "chip"), "yes");
		CHECK(Rwidget_get_child_count(Rwidget_find(root, "without")) == 0);
		Rwidget_destroy(root);
	}
	/* an expression that cannot be computed gives an empty text and a warning */
	EXPECT_ERRORS_BEGIN();
	{
		const char* xml = "<VBox><Widget Name=\"Bad\" A=\"1\"><Label Text=\"[{Unknown}][{A +}][{A / 0}][{nope(A)}]\"/></Widget><Bad Name=\"bad\"/></VBox>";
		Rid root = widget_from(xml);
		CHECK_STR(label_of(root, "bad"), "[][][0][]");
		Rwidget_destroy(root);
	}
	EXPECT_ERRORS_END(3);

	/* declarations kept in a file of their own, and brought in where they are needed */
	{
		const char* library_xml = "<Widgets><Widget Name=\"FromFile\" Text=\"file\"><Label Text=\"from {Text}\"/></Widget></Widgets>";
		const char* part_xml = "<Label Name=\"part\">a part</Label>";
		const char* temp = getenv("TMPDIR") ? getenv("TMPDIR") : (getenv("TEMP") ? getenv("TEMP") : "/tmp");
		char library_path[512], part_path[512], xml[1400];
		Rid root;
		snprintf(library_path, sizeof(library_path), "%s/raven_test_library.xml", temp);
		snprintf(part_path, sizeof(part_path), "%s/raven_test_part.xml", temp);
		CHECK(Rfile_write(library_path, library_xml, strlen(library_xml)));
		CHECK(Rfile_write(part_path, part_xml, strlen(part_xml)));
		snprintf(xml, sizeof(xml), "<VBox><Include Source=\"%s\"/><Include Source=\"%s\"/><FromFile Name=\"used\"/></VBox>", library_path, part_path);
		root = widget_from(xml);
		CHECK(Rwidget_get_child_count(root) == 2);
		CHECK_STR(label_of(root, "part"), "a part");
		CHECK_STR(label_of(root, "used"), "from file");
		Rwidget_destroy(root);
		Rxml_unregister_widget("FromFile");
		CHECK(Rxml_register_widgets_from_file(library_path) == 1);
		CHECK(Rxml_is_widget_registered("FromFile"));
		remove(library_path);
		remove(part_path);
		EXPECT_ERRORS_BEGIN();
		CHECK(Rxml_register_widgets_from_file(library_path) == 0);
		root = widget_from(xml);
		CHECK(root != R_INVALID_ID && Rwidget_get_child_count(root) == 1);
		Rwidget_destroy(root);
		EXPECT_ERRORS_END(3);
	}

	/* declaring a name again replaces it; a name can be forgotten */
	CHECK(register_text("<Widget Name=\"Two\"><Label Text=\"second\"/></Widget>") == 1);
	card = widget_from("<Two/>");
	CHECK_STR(Rlabel_get_text(card), "second");
	Rwidget_destroy(card);
	Rxml_unregister_widget("Two");
	Rxml_unregister_widget("never declared");
	CHECK(!Rxml_is_widget_registered("Two"));
	EXPECT_ERRORS_BEGIN();
	CHECK(widget_from("<Two/>") == R_INVALID_ID);
	EXPECT_ERRORS_END(1);
}


/* ---------------------------------------------------------------------------
 *  Documents that can be edited
 * ------------------------------------------------------------------------- */

static Rid parse_text(const char* xml) {
	return Rxml_document_parse(xml, strlen(xml), NULL);
}

static void test_documents(void) {
	const char* source =
		"<?xml version=\"1.0\"?>\n"
		"<!-- header -->\n"
		"<Window Title=\"A &amp; B\" Width=\"320\" Height=\"200\">\n"
		"  <VBox Name=\"box\" Spacing=\"4\">\n"
		"    <!-- first -->\n"
		"    <Label Name=\"l\">Hello &lt;you&gt;</Label>\n"
		"    <Button Name=\"b\" Text=\"Go\" OnClicked=\"go\"/>\n"
		"  </VBox>\n"
		"</Window>\n";
	RXmlError error;
	Rid document, root, box, label, button, comment, added, copy, other;
	const char* text;

	/* reading */
	document = parse_text(source);
	CHECK(Rxml_document_is_valid(document));
	root = Rxml_document_get_root(document);
	CHECK(Rxml_node_is_valid(root));
	CHECK(Rxml_node_get_document(root) == document);
	CHECK_STR(Rxml_node_get_tag(root), "Window");
	CHECK(Rxml_node_get_parent(root) == R_INVALID_ID);
	CHECK(Rxml_node_get_index(root) == 0);
	CHECK(Rxml_node_get_line(root) == 3);
	CHECK(Rxml_node_get_attribute_count(root) == 3);
	CHECK_STR(Rxml_node_get_attribute_name(root, 0), "Title");
	CHECK_STR(Rxml_node_get_attribute_value(root, 0), "A & B");
	CHECK(Rxml_node_get_attribute_name(root, 3) == NULL);
	CHECK(Rxml_node_get_attribute_value(root, -1) == NULL);
	CHECK_STR(Rxml_node_get_attribute(root, "width"), "320");
	CHECK(Rxml_node_get_attribute(root, "Nope") == NULL);
	CHECK(Rxml_node_get_child_count(root) == 1);
	box = Rxml_node_get_child(root, 0);
	CHECK(Rxml_node_get_child(root, 1) == R_INVALID_ID);
	CHECK(Rxml_node_get_parent(box) == root);
	CHECK(Rxml_node_get_child_count(box) == 3);
	comment = Rxml_node_get_child(box, 0);
	label = Rxml_node_get_child(box, 1);
	button = Rxml_node_get_child(box, 2);
	CHECK(Rxml_node_is_comment(comment) && !Rxml_node_is_comment(label));
	CHECK_STR(Rxml_node_get_tag(comment), "");
	CHECK_STR(Rxml_node_get_text(comment), " first ");
	CHECK_STR(Rxml_node_get_text(label), "Hello <you>");
	CHECK_STR(Rxml_node_get_text(button), "");
	CHECK(Rxml_node_get_index(button) == 2);
	CHECK(Rxml_node_get_line(label) == 6);

	/* writing: one element per line, tabs, what must be escaped is, comments are kept */
	text = Rxml_document_to_string(document);
	CHECK_STR(text,
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<!-- header -->\n"
		"<Window Title=\"A &amp; B\" Width=\"320\" Height=\"200\">\n"
		"\t<VBox Name=\"box\" Spacing=\"4\">\n"
		"\t\t<!-- first -->\n"
		"\t\t<Label Name=\"l\">Hello &lt;you&gt;</Label>\n"
		"\t\t<Button Name=\"b\" Text=\"Go\" OnClicked=\"go\"/>\n"
		"\t</VBox>\n"
		"</Window>\n");
	/* (written, the nodes know their new lines) */
	CHECK(Rxml_node_get_line(root) == 3 && Rxml_node_get_line(button) == 7);
	/* what is written reads back the same */
	other = parse_text(text);
	CHECK(other != R_INVALID_ID);
	{
		char* first = strdup(text);
		CHECK_STR(Rxml_document_to_string(other), first);
		free(first);
	}
	Rxml_document_destroy(other);
	CHECK(!Rxml_document_is_valid(other));

	/* changing */
	Rxml_node_set_attribute(button, "Text", "Say \"hi\"\n<now>");
	Rxml_node_set_attribute(button, "onclicked", NULL);
	Rxml_node_set_attribute(button, "Style", "Primary");
	Rxml_node_set_attribute(button, "Nope", NULL);
	CHECK(Rxml_node_get_attribute_count(button) == 3);
	CHECK_STR(Rxml_node_to_string(button), "<Button Name=\"b\" Text=\"Say &quot;hi&quot;&#10;&lt;now&gt;\" Style=\"Primary\"/>\n");
	Rxml_node_set_text(label, "");
	Rxml_node_set_tag(label, "Checkbox");
	CHECK_STR(Rxml_node_to_string(label), "<Checkbox Name=\"l\"/>\n");
	Rxml_node_set_text(label, "On & on");
	added = Rxml_node_add_child(box, "Slider", 0);
	CHECK(Rxml_node_get_child(box, 0) == added && Rxml_node_get_index(comment) == 1);
	CHECK(Rxml_node_get_line(added) == 0);
	Rxml_node_set_attribute(added, "Value", "0.5");
	other = Rxml_node_add_comment(box, " last ", -1);
	CHECK(Rxml_node_get_index(other) == 4);
	other = Rxml_node_add_xml(box, "<HBox><Label Text='in'/><!-- c --></HBox>", 41, 1);
	CHECK(Rxml_node_get_index(other) == 1);
	CHECK(Rxml_node_get_child_count(other) == 2);
	CHECK(Rxml_node_get_document(Rxml_node_get_child(other, 0)) == document);
	copy = Rxml_node_duplicate(other);
	CHECK(Rxml_node_get_index(copy) == 2 && copy != other);
	CHECK_STR(Rxml_node_get_attribute(Rxml_node_get_child(copy, 0), "Text"), "in");
	/* moving: inside the copy, then back out, and never into itself */
	CHECK(Rxml_node_move(button, copy, 0));
	CHECK(Rxml_node_get_parent(button) == copy && Rxml_node_get_index(button) == 0);
	CHECK(Rxml_node_get_child_count(box) == 6);
	CHECK(!Rxml_node_move(copy, button, 0));
	CHECK(!Rxml_node_move(copy, copy, 0));
	CHECK(!Rxml_node_move(root, box, 0));
	CHECK(Rxml_node_move(button, box, -1));
	CHECK(Rxml_node_get_index(button) == 6);
	CHECK(Rxml_node_move(button, box, 0));
	CHECK(Rxml_node_get_index(button) == 0 && Rxml_node_get_index(added) == 1);
	/* removing takes what is inside along */
	{
		Rid inner = Rxml_node_get_child(copy, 0);
		Rxml_node_remove(copy);
		CHECK(!Rxml_node_is_valid(copy) && !Rxml_node_is_valid(inner));
	}
	CHECK_STR(Rxml_node_to_string(box),
		"<VBox Name=\"box\" Spacing=\"4\">\n"
		"\t<Button Name=\"b\" Text=\"Say &quot;hi&quot;&#10;&lt;now&gt;\" Style=\"Primary\"/>\n"
		"\t<Slider Value=\"0.5\"/>\n"
		"\t<HBox>\n"
		"\t\t<Label Text=\"in\"/>\n"
		"\t\t<!-- c -->\n"
		"\t</HBox>\n"
		"\t<!-- first -->\n"
		"\t<Checkbox Name=\"l\">On &amp; on</Checkbox>\n"
		"\t<!-- last -->\n"
		"</VBox>\n");

	/* what cannot be done */
	EXPECT_ERRORS_BEGIN();
	Rxml_node_remove(root);
	CHECK(Rxml_node_is_valid(root));
	CHECK(Rxml_node_duplicate(root) == R_INVALID_ID);
	CHECK(Rxml_node_add_child(box, "1bad", -1) == R_INVALID_ID);
	CHECK(Rxml_node_add_child(box, "", -1) == R_INVALID_ID);
	CHECK(Rxml_node_add_child(comment, "Label", -1) == R_INVALID_ID);
	CHECK(Rxml_node_add_comment(box, "a -- b", -1) == R_INVALID_ID);
	CHECK(Rxml_node_add_xml(box, "<Open>", 6, -1) == R_INVALID_ID);
	Rxml_node_set_tag(box, "a b");
	Rxml_node_set_tag(comment, "Label");
	Rxml_node_set_attribute(box, "a b", "1");
	Rxml_node_set_attribute(comment, "A", "1");
	CHECK_STR(Rxml_node_get_tag(box), "VBox");
	CHECK(Rxml_node_get_attribute_count(box) == 2);
	CHECK(Rxml_node_get_child_count(box) == 6);
	CHECK(Rxml_document_create("no good") == R_INVALID_ID);
	CHECK(Rxml_document_get_root(0x12345) == R_INVALID_ID);
	CHECK_STR(Rxml_node_get_tag(0x12345), "");
	CHECK(parse_text("<A><B></A>") == R_INVALID_ID);
	CHECK(Rxml_document_load("/nonexistent/file.xml") == R_INVALID_ID);
	EXPECT_ERRORS_END(16);
	/* a mistake in the text, told to the caller instead of the error callback */
	CHECK(Rxml_document_parse("<A>\n  <B></A>", 13, &error) == R_INVALID_ID);
	CHECK(error.line == 2 && error.column > 1 && strstr(error.message, "does not match") != NULL);
	CHECK(Rxml_document_parse("<A/>", 4, &error) != R_INVALID_ID);
	CHECK(error.line == 0 && error.message[0] == '\0');

	/* building: the widgets and the nodes know each other */
	{
		Rid widget = Rxml_document_create_widget(document);
		Rid slider_widget, window;
		CHECK(Rwidget_is_valid(widget));
		/* (the root is a <Window>: this is what it would show) */
		CHECK(Rwidget_get_type(widget) == R_WIDGET_VBOX);
		CHECK(Rwidget_get_xml_node(widget) == box);
		CHECK(Rxml_node_get_widget(box) == widget);
		CHECK(Rxml_node_get_widget(root) == R_INVALID_ID);
		slider_widget = Rxml_node_get_widget(added);
		CHECK(Rwidget_get_type(slider_widget) == R_WIDGET_SLIDER);
		CHECK(Rslider_get_value(slider_widget) == 0.5f);
		CHECK(Rwidget_get_xml_node(Rwidget_find(widget, "b")) == button);
		CHECK(Rxml_node_get_widget(comment) == R_INVALID_ID);
		CHECK(Rwidget_get_xml_node(R_INVALID_ID) == R_INVALID_ID);
		/* a widget that was not built from a document, and one whose widget is gone */
		{
			Rid plain = Rbutton_create("x");
			CHECK(Rwidget_get_xml_node(plain) == R_INVALID_ID);
			Rwidget_destroy(plain);
		}
		Rwidget_destroy(widget);
		Rdraw_frame();
		CHECK(Rxml_node_get_widget(box) == R_INVALID_ID);

		window = Rxml_document_create_window(document);
		CHECK(Rwindow_is_valid(window));
		CHECK_STR(Rwindow_get_title(window), "A & B");
		CHECK(Rxml_node_get_widget(root) == window);
		CHECK(Rwidget_get_xml_node(Rwidget_find(window, "box")) == box);
		CHECK(Rxml_node_get_widget(button) == Rwidget_find(window, "b"));
		/* the document can go: what was built stays, and no longer leads to a node */
		Rxml_document_destroy(document);
		CHECK(!Rxml_node_is_valid(box));
		CHECK(Rwidget_get_xml_node(Rwidget_find(window, "box")) == R_INVALID_ID);
		Rdraw_frame();
		Rwindow_destroy(window);
	}

	/* declared widgets: everything a use becomes leads to the node of the use; its children to their own */
	document = parse_text(
		"<VBox>"
		"<Widget Name=\"DomCard\" Title=\"t\"><Border><VBox><Label Name=\"title\" Text=\"{Title}\"/><Children/></VBox></Border></Widget>"
		"<DomCard Name=\"one\" Title=\"One\"><Button Name=\"inner\" Text=\"x\"/></DomCard>"
		"<Repeat Count=\"2\" Var=\"i\"><Label Text=\"r{i}\"/></Repeat>"
		"</VBox>");
	CHECK(document != R_INVALID_ID);
	{
		Rid widget = Rxml_document_create_widget(document);
		Rid use, repeat, card, title, inner;
		root = Rxml_document_get_root(document);
		use = Rxml_node_get_child(root, 1);
		repeat = Rxml_node_get_child(root, 2);
		CHECK(Rwidget_is_valid(widget));
		CHECK(Rxml_is_widget_registered("DomCard"));
		card = Rwidget_find(widget, "one");
		CHECK(Rwidget_get_type(card) == R_WIDGET_BORDER);
		CHECK(Rxml_node_get_widget(use) == card);
		CHECK(Rwidget_get_xml_node(card) == use);
		title = Rwidget_find(card, "title");
		CHECK_STR(Rlabel_get_text(title), "One");
		CHECK(Rwidget_get_xml_node(title) == use);
		inner = Rwidget_find(card, "inner");
		CHECK(Rwidget_get_xml_node(inner) == Rxml_node_get_child(use, 0));
		CHECK(Rxml_node_get_widget(Rxml_node_get_child(use, 0)) == inner);
		/* a repeated element: its copies all come from the one node, which gives the first */
		CHECK(Rwidget_get_child_count(widget) == 3);
		CHECK(Rwidget_get_xml_node(Rwidget_get_child(widget, 1)) == Rxml_node_get_child(repeat, 0));
		CHECK(Rwidget_get_xml_node(Rwidget_get_child(widget, 2)) == Rxml_node_get_child(repeat, 0));
		CHECK(Rxml_node_get_widget(Rxml_node_get_child(repeat, 0)) == Rwidget_get_child(widget, 1));
		/* the declaration itself, and <Repeat>, are not widgets */
		CHECK(Rxml_node_get_widget(Rxml_node_get_child(root, 0)) == R_INVALID_ID);
		CHECK(Rxml_node_get_widget(repeat) == R_INVALID_ID);
		Rwidget_destroy(widget);
	}
	Rxml_unregister_widget("DomCard");
	Rxml_document_destroy(document);

	/* a mistake in what is built is reported with the line the node has */
	document = parse_text("<VBox>\n<Label/>\n<Nonsense/>\n</VBox>");
	EXPECT_ERRORS_BEGIN();
	CHECK(Rxml_document_create_widget(document) == R_INVALID_ID);
	EXPECT_ERRORS_END(1);
	Rxml_document_destroy(document);

	/* from nothing, to a file and back */
	document = Rxml_document_create("HBox");
	root = Rxml_document_get_root(document);
	Rxml_node_set_attribute(root, "Spacing", "6");
	Rxml_node_set_text(Rxml_node_add_child(root, "Label", -1), "saved");
	CHECK_STR(Rxml_document_to_string(document), "<HBox Spacing=\"6\">\n\t<Label>saved</Label>\n</HBox>\n");
	{
		const char* path = "raven_test_document.xml";
		Rid loaded, widget;
		CHECK(Rxml_document_save(document, path));
		loaded = Rxml_document_load(path);
		CHECK(loaded != R_INVALID_ID);
		CHECK_STR(Rxml_document_to_string(loaded), "<HBox Spacing=\"6\">\n\t<Label>saved</Label>\n</HBox>\n");
		widget = Rxml_document_create_widget(loaded);
		CHECK(Rwidget_get_type(widget) == R_WIDGET_HBOX && Rwidget_get_child_count(widget) == 1);
		CHECK_STR(Rlabel_get_text(Rwidget_get_child(widget, 0)), "saved");
		Rwidget_destroy(widget);
		Rxml_document_destroy(loaded);
		remove(path);
		EXPECT_ERRORS_BEGIN();
		CHECK(!Rxml_document_save(document, ""));
		CHECK(!Rxml_document_save(document, "/nonexistent-folder/x.xml"));
		EXPECT_ERRORS_END(2);
	}
	/* (left alive on purpose: the library releases the documents it is left with) */
}

/* The sample value given to an attribute of each kind. */
static const char* schema_sample(const RXmlAttributeInfo* info, char* buffer, size_t size) {
	switch (info->type) {
		case R_XML_BOOL: return "true";
		case R_XML_NUMBER: return "1";
		case R_XML_COLOR: return "#336699";
		case R_XML_ACTION: return "test.action";
		case R_XML_NUMBERS: return "1";
		case R_XML_LIST: return "a|b";
		case R_XML_CHOICE: {
			size_t length = strcspn(info->choices, "|");
			if (length >= size) length = size - 1;
			memcpy(buffer, info->choices, length);
			buffer[length] = '\0';
			return buffer;
		}
		default: return "x";
	}
}

static void test_schema(void) {
	RXmlAttributeInfo info;
	int tag, index, total = 0;
	char buffer[64];

	CHECK(Rxml_get_tag_count() > 30);
	CHECK_STR(Rxml_get_tag_name(0), "Window");
	CHECK(Rxml_get_tag_name(-1) == NULL && Rxml_get_tag_name(Rxml_get_tag_count()) == NULL);
	CHECK(Rxml_get_tag_children("Window") == 1);
	CHECK(Rxml_get_tag_children("VBox") == 2);
	CHECK(Rxml_get_tag_children("Border") == 1);
	CHECK(Rxml_get_tag_children("Label") == 0);
	CHECK(Rxml_get_tag_children("textblock") == 0);
	CHECK(Rxml_get_tag_children("NotATag") == 2);
	CHECK(strlen(Rxml_get_tag_description("button")) > 0);
	CHECK_STR(Rxml_get_tag_description("NotATag"), "");

	/* every tag is one the loader knows, and every attribute listed for it is accepted with a value of its kind */
	for (tag = 1; tag < Rxml_get_tag_count(); tag++) {
		const char* name = Rxml_get_tag_name(tag);
		char xml[96];
		Rid widget, parent;
		int count = Rxml_get_attribute_count(name), choices;
		snprintf(xml, sizeof(xml), "<VBox><%s/></VBox>", name);
		parent = Rwidget_create_from_xml(xml, strlen(xml));
		CHECK(parent != R_INVALID_ID);
		if (parent == R_INVALID_ID) continue;
		/* (a menu and its items are not children like the others) */
		widget = Rwidget_get_child_count(parent) ? Rwidget_get_child(parent, 0) : R_INVALID_ID;
		if (widget == R_INVALID_ID) {
			Rwidget_destroy(parent);
			parent = R_INVALID_ID;
			snprintf(xml, sizeof(xml), "<%s/>", name);
			widget = Rwidget_create_from_xml(xml, strlen(xml));
		}
		CHECK(widget != R_INVALID_ID);
		CHECK(count > 60);
		CHECK(!Rxml_get_attribute_info(name, count, &info));
		for (index = 0; index < count; index++) {
			const char* value;
			CHECK(Rxml_get_attribute_info(name, index, &info));
			CHECK(info.name && info.group && info.description);
			CHECK((info.type == R_XML_CHOICE) == (info.choices != NULL));
			total++;
			/* (a file would have to exist, and a saved arrangement to be one; an effect is a name or a file) */
			if (info.type == R_XML_FILE || strcmp(info.name, "Layout") == 0 || strcmp(info.name, "Swatches") == 0) continue;
			if (strcmp(info.name, "Effect") == 0) {
				CHECK(Rwidget_set_attribute(widget, info.name, "aurora"));
				CHECK(Rwidget_set_attribute(widget, info.name, "none"));
				continue;
			}
			value = schema_sample(&info, buffer, sizeof(buffer));
			/* (some take a share below 1, some two numbers, some four) */
			if (!Rwidget_set_attribute(widget, info.name, value) &&
				!(info.type == R_XML_NUMBER && Rwidget_set_attribute(widget, info.name, "0.5")) &&
				!(info.type == R_XML_NUMBERS && (Rwidget_set_attribute(widget, info.name, "1,1") ||
					Rwidget_set_attribute(widget, info.name, "0,0,1,1")))) {
				g_test_failures++;
				printf("  FAIL <%s %s=\"%s\"> is listed but not accepted\n", name, info.name, value);
			}
			g_test_checks++;
			/* each of the choices, not only the first */
			for (choices = 0; info.type == R_XML_CHOICE && info.choices[choices]; ) {
				size_t length = strcspn(info.choices + choices, "|");
				snprintf(buffer, sizeof(buffer), "%.*s", (int)length, info.choices + choices);
				if (!Rwidget_set_attribute(widget, info.name, buffer)) {
					g_test_failures++;
					printf("  FAIL <%s %s=\"%s\"> is listed but not accepted\n", name, info.name, buffer);
				}
				g_test_checks++;
				choices += (int)length;
				if (info.choices[choices] == '|') choices++;
			}
		}
		Rwidget_destroy(parent != R_INVALID_ID ? parent : widget);
		Rdraw_frame();
	}
	CHECK(total > 2000);

	/* the window: its own attributes only, each accepted */
	{
		char xml[2048];
		int length = snprintf(xml, sizeof(xml), "<Window");
		Rid window;
		int count = Rxml_get_attribute_count("Window");
		CHECK(count >= 20 && count < 30);
		for (index = 0; index < count; index++) {
			int touch;
			CHECK(Rxml_get_attribute_info("window", index, &info));
			/* its own attributes, then those of fingers and pens */
			touch = strcmp(info.group, "Touch") == 0;
			CHECK(touch || strcmp(info.group, "Window") == 0);
			CHECK(!touch || strncmp(info.name, "Touch.", 6) == 0 || strncmp(info.name, "Pen.", 4) == 0);
			if (info.type == R_XML_FILE) continue;
			length += snprintf(xml + length, sizeof(xml) - (size_t)length, " %s=\"%s\"", info.name,
				info.type == R_XML_NUMBER ? (touch ? "0.5" : "300") : schema_sample(&info, buffer, sizeof(buffer)));
		}
		snprintf(xml + length, sizeof(xml) - (size_t)length, "/>");
		/* (an attribute it does not know would be reported, and fail the test) */
		window = Rwindow_create_from_xml(xml, strlen(xml));
		CHECK(Rwindow_is_valid(window));
		Rwindow_destroy(window);
	}

	/* groups, in order: the tag, Widget, Slot, Colors, Metrics */
	CHECK(Rxml_get_attribute_info("Button", 0, &info));
	CHECK_STR(info.name, "Text");
	CHECK_STR(info.group, "Button");
	CHECK(info.type == R_XML_TEXT);
	CHECK(Rxml_get_attribute_info("Button", 1, &info) && info.type == R_XML_CHOICE);
	CHECK_STR(info.choices, "Default|Primary|Flat");
	CHECK(Rxml_get_attribute_info("Button", 3, &info));
	CHECK_STR(info.group, "Widget");
	CHECK_STR(info.name, "Name");
	CHECK(Rxml_get_attribute_info("Button", Rxml_get_attribute_count("Button") - 1, &info));
	CHECK_STR(info.group, "Metrics");
	CHECK(!Rxml_get_attribute_info("Button", -1, &info));
	CHECK(!Rxml_get_attribute_info(NULL, 0, &info));
	CHECK(Rxml_get_attribute_count(NULL) == 0);

	/* a tag declared with <Widget>: its parameters, with their default value as description, then what every widget has */
	CHECK(register_text("<Widget Name=\"SchemaCard\" Title=\"Untitled\" Wide=\"false\"><Border/></Widget>") == 1);
	CHECK(Rxml_get_tag_children("SchemaCard") == 2);
	CHECK(Rxml_get_attribute_count("SchemaCard") == Rxml_get_attribute_count("Spacer") + 2);
	CHECK(Rxml_get_attribute_info("SchemaCard", 0, &info));
	CHECK_STR(info.name, "Title");
	CHECK_STR(info.group, "SchemaCard");
	CHECK_STR(info.description, "Untitled");
	CHECK(Rxml_get_attribute_info("SchemaCard", 1, &info));
	CHECK_STR(info.name, "Wide");
	CHECK(Rxml_get_attribute_info("SchemaCard", 2, &info));
	CHECK_STR(info.name, "Name");
	CHECK_STR(info.group, "Widget");
	Rxml_unregister_widget("SchemaCard");
}

/*
 * A <Widget> declared again while it is being used: inside its own body, or
 * declarations made inside a use (the list of declarations grows while it is
 * read). What is being expanded stays alive until the expansion is over.
 */
static void test_declared_again_inside(void) {
	const char* again =
		"<VBox>"
		"<Widget Name=\"Card\"><Border><VBox><Widget Name=\"Card\"><Label Text=\"again\"/></Widget><Label Name=\"inside\" Text=\"first\"/></VBox></Border></Widget>"
		"<Card/><Card/><Card/>"
		"</VBox>";
	char grows[4096];
	int length = 0, i;
	Rid widget;
	widget = Rwidget_create_from_xml(again, strlen(again));
	CHECK(widget != R_INVALID_ID);
	if (widget != R_INVALID_ID) {
		CHECK(Rwidget_get_child_count(widget) == 3);
		Rwidget_destroy(widget);
	}
	/* a body that declares twenty more: the array of declarations is reallocated during the use */
	length += snprintf(grows + length, sizeof(grows) - (size_t)length, "<VBox><Widget Name=\"Grower\"><VBox>");
	for (i = 0; i < 20; i++) length += snprintf(grows + length, sizeof(grows) - (size_t)length, "<Widget Name=\"Made%d\"><Label/></Widget>", i);
	length += snprintf(grows + length, sizeof(grows) - (size_t)length, "<Label Text=\"grown\"/></VBox></Widget><Grower/><Grower/><Made3/></VBox>");
	widget = Rwidget_create_from_xml(grows, (size_t)length);
	CHECK(widget != R_INVALID_ID);
	if (widget != R_INVALID_ID) Rwidget_destroy(widget);
	for (i = 0; i < 20; i++) {
		char name[32];
		snprintf(name, sizeof(name), "Made%d", i);
		Rxml_unregister_widget(name);
	}
	Rxml_unregister_widget("Grower");
	Rxml_unregister_widget("Card");
}

/* The actions Raven knows by itself: an interface that works with no C. */
static int g_unbound_count;
static void on_unbound(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)action;
	(void)user_data;
	g_unbound_count++;
}
static void on_bound_hide(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)action;
	(*(int*)user_data)++;
}

static void test_builtin_actions(void) {
	static const char xml[] =
		"<Window Width=\"300\" Height=\"200\">"
		"  <VBox>"
		"    <Button Name=\"go\" Text=\"Go\" OnClicked=\"page:pages=+1\"/>"
		"    <Label Name=\"details\" Text=\"Details\"/>"
		"    <TextInput Name=\"field\"/>"
		"    <Switcher Name=\"pages\"><Label Text=\"One\"/><Label Text=\"Two\"/><Label Text=\"Three\"/></Switcher>"
		"    <ComboBox Name=\"choice\" Items=\"a|b|c\" Selected=\"0\"/>"
		"  </VBox>"
		"</Window>";
	Rid window = Rwindow_create_from_xml(xml, sizeof(xml) - 1);
	Rid go, details, pages, choice, field;
	int bound = 0;
	CHECK(Rwindow_is_valid(window));
	if (!Rwindow_is_valid(window)) return;
	go = Rwidget_find(window, "go");
	details = Rwidget_find(window, "details");
	pages = Rwidget_find(window, "pages");
	choice = Rwidget_find(window, "choice");
	field = Rwidget_find(window, "field");
	g_unbound_count = 0;
	Raction_set_default_handler(on_unbound, NULL);

	/* the names */
	CHECK(Raction_is_builtin("app.quit"));
	CHECK(Raction_is_builtin("show:details"));
	CHECK(Raction_is_builtin("page:pages=2"));
	CHECK(!Raction_is_builtin("show:"));
	CHECK(!Raction_is_builtin("file.save"));

	/* widgets of the window of the sender, by their names */
	Raction_fire("hide:details", go);
	CHECK(Rwidget_get_visibility(details) == R_COLLAPSED);
	Raction_fire("toggle:details", go);
	CHECK(Rwidget_get_visibility(details) == R_VISIBLE);
	Raction_fire("disable:field", go);
	CHECK(!Rwidget_is_enabled(field));
	Raction_fire("enable:field", go);
	CHECK(Rwidget_is_enabled(field));
	Raction_fire("page:pages=2", go);
	CHECK(Rswitcher_get_index(pages) == 2);
	Raction_fire("page:pages=+1", go);
	CHECK(Rswitcher_get_index(pages) == 0);
	Raction_fire("page:pages=-1", go);
	CHECK(Rswitcher_get_index(pages) == 2);
	Raction_fire("page:choice=1", window);
	CHECK(Rcombo_box_get_selected(choice) == 1);
	/* a name that is no widget does nothing, and is not passed on */
	Raction_fire("show:nothing", go);
	CHECK(g_unbound_count == 0);
	/* the look */
	Raction_fire("theme.light", go);
	CHECK(Rlook_get() == R_LOOK_RAVEN_LIGHT);
	Raction_fire("theme.toggle", go);
	CHECK(Rlook_get() == R_LOOK_RAVEN_DARK);
	/* the clipboard */
	Raction_fire("copy:Hello", go);
	CHECK_STR(Rclipboard_get_text(), "Hello");

	/* a function bound to the name wins */
	Raction_bind("hide:details", on_bound_hide, &bound);
	Raction_fire("hide:details", go);
	CHECK(bound == 1 && Rwidget_get_visibility(details) == R_VISIBLE);
	Raction_unbind("hide:details");
	/* other names go to the default handler */
	Raction_fire("test.not_bound", go);
	CHECK(g_unbound_count == 1);
	/* turned off: they all go to the default handler */
	Raction_set_builtins(R_FALSE);
	Raction_fire("hide:details", go);
	CHECK(Rwidget_get_visibility(details) == R_VISIBLE && g_unbound_count == 2);
	Raction_set_builtins(R_TRUE);
	/* window.close closes the window of the widget */
	Raction_fire("window.close", go);
	Rdraw_frame();
	CHECK(!Rwindow_is_valid(window));
	Raction_set_default_handler(NULL, NULL);
}

/* <Window Theme> and Rwindow_set_theme: the theme of that window only, never the one of the application. */
static void test_window_theme(void) {
	static const char xml[] =
		"<Window Title=\"Dark one\" Width=\"300\" Height=\"200\" Theme=\"RavenDark\">\n"
		"  <VBox><Label Name=\"label\" Text=\"a\"/><Button Name=\"own\" Text=\"b\" Theme=\"WindowsLight\"/></VBox>\n"
		"</Window>\n";
	static const char plain[] = "<Window Title=\"Plain\" Width=\"300\" Height=\"200\"><Label Name=\"label\" Text=\"c\"/></Window>";
	char app[64], initial[64];
	Rid dark, other, label, own, other_label, moved;
	float r1, g1, b1, a1, r2, g2, b2, a2;
	snprintf(initial, sizeof(initial), "%s", Rtheme_get_name());
	CHECK(Rtheme_use("RavenLight"));
	snprintf(app, sizeof(app), "%s", Rtheme_get_name());
	dark = Rwindow_create_from_xml(xml, strlen(xml));
	other = Rwindow_create_from_xml(plain, strlen(plain));
	CHECK(dark != R_INVALID_ID && other != R_INVALID_ID);
	label = Rwidget_find(dark, "label");
	own = Rwidget_find(dark, "own");
	other_label = Rwidget_find(other, "label");
	Rdraw_frame();
	/* the application keeps its theme */
	CHECK_STR(Rtheme_get_name(), app);
	CHECK(Rwindow_has_own_theme(dark));
	CHECK(!Rwindow_has_own_theme(other));
	CHECK_STR(Rwindow_get_theme(dark), "RavenDark");
	CHECK_STR(Rwindow_get_theme(other), app);
	CHECK_STR(Rwidget_get_theme(label), "RavenDark");
	CHECK_STR(Rwidget_get_theme(own), "WindowsLight");
	CHECK_STR(Rwidget_get_theme(other_label), app);
	/* drawn with it: the text of the dark window is light, the one of the other dark */
	Rwidget_get_color(label, R_COLOR_TEXT, &r1, &g1, &b1, &a1);
	Rwidget_get_color(other_label, R_COLOR_TEXT, &r2, &g2, &b2, &a2);
	CHECK(r1 + g1 + b1 > r2 + g2 + b2 + 0.5f);
	/* a widget put in the window wears its theme, and the one of the next window when it moves */
	moved = Rwidget_create(R_WIDGET_LABEL);
	Rwindow_set_content(dark, moved);
	CHECK_STR(Rwidget_get_theme(moved), "RavenDark");
	/* the application changes its theme: the window keeps its own */
	CHECK(Rtheme_use("WindowsLight"));
	CHECK_STR(Rwindow_get_theme(dark), "RavenDark");
	CHECK_STR(Rwidget_get_theme(moved), "RavenDark");
	/* NULL: the theme of the application again */
	CHECK(Rwindow_set_theme(dark, NULL));
	CHECK(!Rwindow_has_own_theme(dark));
	CHECK_STR(Rwidget_get_theme(moved), Rtheme_get_name());
	CHECK(Rwindow_set_theme(dark, "GnomeDark"));
	CHECK_STR(Rwidget_get_theme(moved), "GnomeDark");
	EXPECT_ERRORS_BEGIN();
	CHECK(!Rwindow_set_theme(dark, "NoSuchTheme"));
	EXPECT_ERRORS_END(1);
	CHECK_STR(Rwindow_get_theme(dark), "GnomeDark");
	Rdraw_frame();
	Rwindow_destroy(dark);
	Rwindow_destroy(other);
	if (initial[0]) Rtheme_use(initial);
}

int main(void) {
	if (!test_init(0)) {
		printf("cannot initialize Raven\n");
		return 1;
	}
	RUN(test_window);
	RUN(test_window_theme);
	RUN(test_widget_fragment);
	RUN(test_errors);
	RUN(test_components);
	RUN(test_declared_again_inside);
	RUN(test_documents);
	RUN(test_schema);
	RUN(test_builtin_actions);
	Rshutdown();
	return test_report("test_xml");
}

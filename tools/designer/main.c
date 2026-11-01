/**
 * Raven Designer - a tool to make the interfaces of Raven (.xml) by looking
 * at them, in a folder. Nothing is built nor compiled here: the program that
 * loads an interface is written elsewhere, in any language.
 *
 *   raven_designer [folder | interface.xml]
 *
 * The other files: designer.h says what each one does (files, island,
 * strings, welcome, os). This one is the window and the visual editor.
 *
 * The window shows the same document five ways, all kept in step:
 *
 *   Widgets      what can be added, by family (each with its colored picture,
 *                icons.c): the tags the document declares with <Widget> first
 *   Tree         the elements; arrows and buttons move, wrap, copy, delete
 *   Preview      the interface, built for real: it fits the panel (or takes
 *                the size of its XML, or of a device), in the theme and the
 *                language chosen in the island; a click selects, a click
 *                around it or Escape selects nothing
 *   Properties   the attributes of the selected element, by kind of value
 *   XML          the text itself, colored, completed, the mistakes underlined
 *
 * Around it: Files (the folder, followed live), Translations, Problems, and
 * the island at the top (Ctrl+K: search everything, edit the selection, the
 * look of the designer, the theme and the language of the preview).
 *
 * Everything here uses the public API of Raven only: the document is a
 * Rxml_document, the lists of tags and attributes come from Rxml_get_tag_...
 * and Rxml_get_attribute_..., and the preview finds the element under the
 * pointer with Rwidget_pick and Rwidget_get_xml_node. The interface of the
 * designer is itself described in XML (DESIGNER_XML below).
 *
 * For automated runs (the tests of the build use them):
 *
 *   --null             no window system and no GPU
 *   --headless         offscreen rendering
 *   --frames N         stops after N frames
 *   --script "..."     commands run before the frames: see designer_script
 *   --capture x.ppm    saves the last frame
 */

#include "designer.h"
#include "../crash_report.h"
#include "../theme_editor/editor.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 *  The interface of the designer
 * ------------------------------------------------------------------------- */

/* (in pieces: a C compiler is only required to accept texts of 4095 characters) */
static const char* const DESIGNER_XML[] = {
"<Window Title=\"Raven Designer\" Width=\"1480\" Height=\"900\" MinWidth=\"960\" MinHeight=\"600\" CustomHeader=\"true\">"
"  <Widget Name=\"ToolButton\" Text=\"\" Tip=\"\" Do=\"\">"
"    <Button Text=\"{Text}\" Style=\"Flat\" Tooltip=\"{Tip}\" OnClicked=\"{Do}\" Metric.PaddingX=\"8\"/>"
"  </Widget>"
"  <VBox>"
"    <HeaderBar Name=\"header\" Title=\"\">"
"      <MenuBar>"
"        <Menu Text=\"File\">"
"          <MenuItem Text=\"Open a folder...\" Shortcut=\"Ctrl+Shift+O\" OnClicked=\"folder.open\"/>"
"          <MenuItem Text=\"Open an interface...\" Shortcut=\"Ctrl+O\" OnClicked=\"file.open\"/>"
"          <Menu Text=\"New interface\">"
"            <MenuItem Text=\"Window\" Shortcut=\"Ctrl+N\" OnClicked=\"files.new\"/>"
"            <MenuItem Text=\"Panel\" OnClicked=\"files.new.panel\"/>"
"            <MenuItem Text=\"Form with declared widgets\" OnClicked=\"files.new.form\"/>"
"            <MenuItem Text=\"Widget library\" OnClicked=\"files.new.widgets\"/>"
"            <MenuItem Text=\"Theme\" OnClicked=\"files.new.theme\"/>"
"          </Menu>"
"          <Separator/>"
"          <MenuItem Text=\"Save\" Shortcut=\"Ctrl+S\" OnClicked=\"file.save\"/>"
"          <MenuItem Text=\"Save as...\" Shortcut=\"Ctrl+Shift+S\" OnClicked=\"file.save_as\"/>"
"          <MenuItem Text=\"Save all\" Shortcut=\"Ctrl+Alt+S\" OnClicked=\"file.save_all\"/>"
"          <Separator/>"
"          <MenuItem Text=\"Start page\" OnClicked=\"file.welcome\"/>"
"          <MenuItem Text=\"Quit\" Shortcut=\"Ctrl+Q\" OnClicked=\"file.quit\"/>"
"        </Menu>",
"        <Menu Text=\"Edit\">"
"          <MenuItem Text=\"Undo\" Shortcut=\"Ctrl+Z\" OnClicked=\"edit.undo\"/>"
"          <MenuItem Text=\"Redo\" Shortcut=\"Ctrl+Y\" OnClicked=\"edit.redo\"/>"
"          <Separator/>"
"          <MenuItem Text=\"Search everything...\" Shortcut=\"Ctrl+K\" OnClicked=\"island.search\"/>"
"          <Separator/>"
"          <MenuItem Text=\"Select nothing\" OnClicked=\"edit.deselect\"/>"
"          <MenuItem Text=\"Duplicate\" Shortcut=\"Ctrl+D\" OnClicked=\"edit.duplicate\"/>"
"          <MenuItem Text=\"Delete\" Shortcut=\"Delete\" OnClicked=\"edit.delete\"/>"
"          <MenuItem Text=\"Move up\" Shortcut=\"Alt+Up\" OnClicked=\"edit.up\"/>"
"          <MenuItem Text=\"Move down\" Shortcut=\"Alt+Down\" OnClicked=\"edit.down\"/>"
"          <MenuItem Text=\"Move out\" Shortcut=\"Alt+Left\" OnClicked=\"edit.out\"/>"
"          <MenuItem Text=\"Move into the previous\" Shortcut=\"Alt+Right\" OnClicked=\"edit.in\"/>"
"          <Separator/>"
"          <Menu Text=\"Wrap in\">"
"            <MenuItem Text=\"VBox\" OnClicked=\"edit.wrap.VBox\"/>"
"            <MenuItem Text=\"HBox\" OnClicked=\"edit.wrap.HBox\"/>"
"            <MenuItem Text=\"Border\" OnClicked=\"edit.wrap.Border\"/>"
"            <MenuItem Text=\"ScrollBox\" OnClicked=\"edit.wrap.ScrollBox\"/>"
"            <MenuItem Text=\"Overlay\" OnClicked=\"edit.wrap.Overlay\"/>"
"          </Menu>"
"          <MenuItem Text=\"Unwrap\" OnClicked=\"edit.unwrap\"/>"
"          <Separator/>"
"          <MenuItem Text=\"Cut\" Shortcut=\"Ctrl+X\" OnClicked=\"edit.cut\"/>"
"          <MenuItem Text=\"Copy (as XML)\" Shortcut=\"Ctrl+C\" OnClicked=\"edit.copy\"/>"
"          <MenuItem Text=\"Paste inside\" Shortcut=\"Ctrl+V\" OnClicked=\"edit.paste\"/>"
"        </Menu>",
"        <Menu Text=\"View\">"
"          <MenuItem Name=\"menu.interact\" Text=\"Use the preview\" Shortcut=\"F6\" Checkable=\"true\" OnClicked=\"view.interact\"/>"
"          <MenuItem Text=\"Open the window\" Shortcut=\"F7\" OnClicked=\"view.run\"/>"
"          <Separator/>"
"          <MenuItem Text=\"Window\" Shortcut=\"Ctrl+1\" OnClicked=\"tab.window\"/>"
"          <MenuItem Text=\"Widget\" Shortcut=\"Ctrl+2\" OnClicked=\"tab.widget\"/>"
"          <MenuItem Text=\"Theme\" Shortcut=\"Ctrl+3\" OnClicked=\"tab.theme\"/>"
"          <MenuItem Text=\"Translations\" Shortcut=\"Ctrl+4\" OnClicked=\"tab.strings\"/>"
"          <Separator/>"
"          <MenuItem Name=\"menu.reduce_motion\" Text=\"Reduce motion\" Checkable=\"true\" OnClicked=\"view.reduce_motion\"/>"
"          <MenuItem Text=\"Accessibility...\" OnClicked=\"view.accessibility\"/>"
"          <Separator/>"
"          <MenuItem Text=\"Reset the panels\" OnClicked=\"view.reset\"/>"
"        </Menu>"
"      </MenuBar>"
"      <HBox Name=\"tabs\" Spacing=\"2\" Slot.VAlign=\"Center\" Slot.Padding=\"14,0,0,0\">"
"        <Button Name=\"tab.window\" Text=\"Window\" Style=\"Primary\" OnClicked=\"tab.window\" Tooltip=\"The windows and panels of the folder (Ctrl+1)\" Metric.ControlHeight=\"26\"/>"
"        <Button Name=\"tab.widget\" Text=\"Widget\" Style=\"Flat\" OnClicked=\"tab.widget\" Tooltip=\"Tags of your own, declared with &lt;Widget&gt;, used by the windows (Ctrl+2)\" Metric.ControlHeight=\"26\"/>"
"        <Button Name=\"tab.theme\" Text=\"Theme\" Style=\"Flat\" OnClicked=\"tab.theme\" Tooltip=\"The themes of the folder: colors, sizes, glass, styles (Ctrl+3)\" Metric.ControlHeight=\"26\"/>"
"        <Button Name=\"tab.strings\" Text=\"Translations\" Style=\"Flat\" OnClicked=\"tab.strings\" Tooltip=\"The tables of the texts, one column per language (Ctrl+4)\" Metric.ControlHeight=\"26\"/>"
"      </HBox>"
"      <HBox Spacing=\"6\" Slot.HAlign=\"Right\" Slot.VAlign=\"Center\" Slot.Padding=\"8,0\">"
"        <Button Text=\"Undo\" Style=\"Flat\" OnClicked=\"edit.undo\" Tooltip=\"Ctrl+Z\" Metric.ControlHeight=\"26\"/>"
"        <Button Text=\"Redo\" Style=\"Flat\" OnClicked=\"edit.redo\" Tooltip=\"Ctrl+Y\" Metric.ControlHeight=\"26\"/>"
"        <Button Text=\"Save\" Style=\"Primary\" OnClicked=\"file.save\" Tooltip=\"Ctrl+S\" Metric.ControlHeight=\"26\"/>"
"      </HBox>"
"    </HeaderBar>",
"    <Switcher Name=\"pages\" Slot.Fill=\"1\">"
"    <VBox Name=\"page.welcome\"/>"
"    <VBox Name=\"page.workspace\">"
"    <DockArea Name=\"dock\" Slot.Fill=\"1\">"
"      <VBox Name=\"panel.preview\" Slot.Title=\"Preview\" Slot.Closable=\"false\">"
"        <HBox Spacing=\"6\" Slot.Padding=\"8,6\">"
"          <ComboBox Name=\"preview.device\" Width=\"200\" Tooltip=\"The size the window is shown in: the room the panel gives it, the size its XML says, or the one of a device\"/>"
"          <Label Name=\"stage.size\" Opacity=\"0.6\" Slot.VAlign=\"Center\" Slot.Padding=\"6,0,0,0\"/>"
"          <Spacer Slot.Fill=\"1\"/>"
"          <Button Text=\"Open the window\" Style=\"Primary\" OnClicked=\"view.run\" Tooltip=\"The window itself, as the program will open it: it follows every change (F7)\" Slot.VAlign=\"Center\"/>"
"        </HBox>"
"        <Separator/>"
"        <Overlay Slot.Fill=\"1\">"
"          <ScrollBox Name=\"stage.scroll\" ScrollBoth=\"true\">"
"            <Overlay Name=\"stage\" Slot.Padding=\"28\" Slot.HAlign=\"Center\"/>"
"          </ScrollBox>"
"          <Overlay Name=\"stage.fit\" Visibility=\"Collapsed\"/>"
"          <VBox Name=\"stage.empty\" Spacing=\"8\" Slot.HAlign=\"Center\" Slot.VAlign=\"Center\" Visibility=\"Collapsed\">"
"            <Label Name=\"stage.empty.title\" Text=\"No interface open\" TextSize=\"20\" Slot.HAlign=\"Center\"/>"
"            <Label Name=\"stage.empty.text\" Text=\"Double click one in Files, make one with New, or press Ctrl+K.\" Opacity=\"0.6\" Slot.HAlign=\"Center\"/>"
"          </VBox>"
"        </Overlay>"
"      </VBox>"
"      <VBox Name=\"panel.files\" Slot.Title=\"Files\" Slot.Dock=\"Left\" Slot.DockSize=\"0.2\" Slot.Closable=\"false\">"
"        <WrapBox Spacing=\"0\" Slot.Padding=\"4,2\">"
"          <ToolButton Text=\"New\" Tip=\"A new file in the folder (Ctrl+N)\" Do=\"files.new\"/>"
"          <ToolButton Text=\"Folder...\" Tip=\"Open another folder (Ctrl+Shift+O)\" Do=\"folder.open\"/>"
"          <ToolButton Text=\"Refresh\" Tip=\"The folder is followed by itself; this looks now\" Do=\"files.refresh\"/>"
"        </WrapBox>"
"        <Separator/>"
"        <TreeView Name=\"files\" Slot.Fill=\"1\" Tooltip=\"Double click: an interface opens, a table goes to Translations, a theme dresses the preview\"/>"
"      </VBox>"
"      <VBox Name=\"panel.widgets\" Slot.Title=\"Widgets\" Slot.Dock=\"Tab\" Slot.DockTo=\"panel.files\" Slot.Closable=\"false\">"
"        <TextInput Name=\"palette.filter\" Placeholder=\"Search a widget\" Slot.Padding=\"6\" OnChanged=\"palette.filter\"/>"
"        <TreeView Name=\"palette\" Slot.Fill=\"1\" Tooltip=\"Double click, or drag it into the preview: the widget goes inside the selected element\"/>"
"        <Button Text=\"Add to the selection\" OnClicked=\"palette.add\" Slot.Padding=\"6\"/>"
"      </VBox>",
"      <VBox Name=\"panel.tree\" Slot.Title=\"Tree\" Slot.Dock=\"Bottom\" Slot.DockTo=\"panel.files\" Slot.DockSize=\"0.45\" Slot.Closable=\"false\">"
"        <WrapBox Spacing=\"0\" Slot.Padding=\"4,2\">"
"          <ToolButton Text=\"Up\" Tip=\"Before the previous element (Alt+Up)\" Do=\"edit.up\"/>"
"          <ToolButton Text=\"Down\" Tip=\"After the next element (Alt+Down)\" Do=\"edit.down\"/>"
"          <ToolButton Text=\"Out\" Tip=\"Out of its parent (Alt+Left)\" Do=\"edit.out\"/>"
"          <ToolButton Text=\"In\" Tip=\"Into the element before it (Alt+Right)\" Do=\"edit.in\"/>"
"          <ToolButton Text=\"Copy\" Tip=\"Duplicate (Ctrl+D)\" Do=\"edit.duplicate\"/>"
"          <ToolButton Text=\"Delete\" Tip=\"Delete (Delete)\" Do=\"edit.delete\"/>"
"        </WrapBox>"
"        <Separator/>"
"        <TreeView Name=\"tree\" Slot.Fill=\"1\"/>"
"      </VBox>",
"      <VBox Name=\"panel.properties\" Slot.Title=\"Properties\" Slot.Dock=\"Right\" Slot.DockSize=\"0.27\" Slot.Closable=\"false\">"
"        <HBox Slot.Padding=\"10,8\" Spacing=\"10\">"
"          <Image Name=\"inspector.icon\" Width=\"30\" Height=\"30\" Slot.VAlign=\"Top\" AccessibleHidden=\"true\"/>"
"          <VBox Spacing=\"2\" Slot.Fill=\"1\">"
"            <Label Name=\"inspector.title\" TextSize=\"17\"/>"
"            <Label Name=\"inspector.about\" Opacity=\"0.6\" Wrap=\"true\"/>"
"          </VBox>"
"        </HBox>"
"        <Separator/>"
"        <ScrollBox Slot.Fill=\"1\">"
"          <VBox Name=\"inspector\" Spacing=\"4\" Slot.Padding=\"8,6\"/>"
"        </ScrollBox>"
"      </VBox>"
"      <TextEditor Name=\"source\" Language=\"XML\" LineNumbers=\"true\" Slot.Title=\"XML\" Slot.Dock=\"Bottom\" Slot.DockTo=\"panel.preview\""
"        Slot.DockSize=\"0.34\" Slot.Closable=\"false\"/>"
"      <ListView Name=\"problems\" Slot.Title=\"Problems\" Slot.Dock=\"Tab\" Slot.DockTo=\"source\" Slot.Closable=\"false\" OnActivated=\"problems.go\"/>",
"    </DockArea>"
"    </VBox>"
"    <HBox Name=\"page.theme\">"
"      <VBox Width=\"230\" Spacing=\"6\" Slot.Padding=\"10\">"
"        <Label Text=\"THEMES OF THE FOLDER\" TextSize=\"11\" Opacity=\"0.55\"/>"
"        <ListView Name=\"theme.list\" Slot.Fill=\"1\" Tooltip=\"A click opens the theme in the editor\"/>"
"        <Button Text=\"New theme\" OnClicked=\"files.new.theme\"/>"
"        <Label Wrap=\"true\" Opacity=\"0.55\" TextSize=\"11\" Text=\"A theme dresses the windows of the folder: in the Window tab, choose it above the preview.\"/>"
"      </VBox>"
"      <Separator Orientation=\"Vertical\"/>"
"      <VBox Name=\"theme.host\" Slot.Fill=\"1\"/>"
"    </HBox>"
"    <VBox Name=\"page.strings\">"
"      <VBox Name=\"panel.strings\" Slot.Fill=\"1\">"
"        <HBox Spacing=\"6\" Slot.Padding=\"6,4\">"
"          <ComboBox Name=\"strings.file\" Width=\"170\" Tooltip=\"The CSV tables of the folder\"/>"
"          <TextInput Name=\"strings.filter\" Placeholder=\"Search a key or a text\" OnChanged=\"strings.filter\" Slot.Fill=\"1\"/>"
"          <ToolButton Text=\"Add key\" Tip=\"A new row\" Do=\"strings.add_key\"/>"
"          <TextInput Name=\"strings.language\" Placeholder=\"de, ar...\" Width=\"80\"/>"
"          <ToolButton Text=\"Add language\" Tip=\"A new column, for the code typed on its left\" Do=\"strings.add_language\"/>"
"          <ToolButton Text=\"New table\" Tip=\"Creates strings.csv in the folder\" Do=\"strings.add_table\"/>"
"          <Button Text=\"Save\" Style=\"Primary\" OnClicked=\"strings.save\"/>"
"        </HBox>"
"        <Separator/>"
"        <ScrollBox Slot.Fill=\"1\">"
"          <VBox Name=\"strings.grid\" Spacing=\"4\" Slot.Padding=\"8,6\"/>"
"        </ScrollBox>"
"      </VBox>"
"    </VBox>"
"    </Switcher>"
"    <Separator/>"
"    <HBox Spacing=\"12\" Slot.Padding=\"10,4\">"
"      <Label Name=\"status\" Slot.Fill=\"1\" Opacity=\"0.8\"/>"
"      <Label Name=\"path\" Opacity=\"0.6\"/>"
"    </HBox>"
"  </VBox>"
"</Window>",
};

/* ---------------------------------------------------------------------------
 *  Documents to start from
 * ------------------------------------------------------------------------- */

/* A new window is empty: only the panel the widgets dropped on it go into. */
static const char TEMPLATE_WINDOW[] =
"<Window Title=\"My window\" Width=\"520\" Height=\"360\">\n"
"\t<VBox Spacing=\"8\" Slot.Padding=\"16\"/>\n"
"</Window>\n";

static const char TEMPLATE_PANEL[] =
"<Border Padding=\"14\" Width=\"320\">\n"
"\t<VBox Spacing=\"8\">\n"
"\t\t<Label Text=\"A panel\" TextSize=\"18\"/>\n"
"\t\t<Separator/>\n"
"\t\t<Checkbox Text=\"One\" Checked=\"true\"/>\n"
"\t\t<Checkbox Text=\"Two\"/>\n"
"\t\t<Button Text=\"Apply\" Slot.HAlign=\"Right\"/>\n"
"\t</VBox>\n"
"</Border>\n";

static const char TEMPLATE_FORM[] =
"<Window Title=\"Sign up\" Width=\"460\" Height=\"380\">\n"
"\t<!-- A tag of our own: a label and an input on one line. -->\n"
"\t<Widget Name=\"Field\" Label=\"Name\" Hint=\"\">\n"
"\t\t<HBox Spacing=\"10\">\n"
"\t\t\t<Label Text=\"{Label}\" Width=\"90\" Slot.VAlign=\"Center\"/>\n"
"\t\t\t<TextInput Placeholder=\"{Hint}\" Slot.Fill=\"1\"/>\n"
"\t\t\t<Children/>\n"
"\t\t</HBox>\n"
"\t</Widget>\n"
"\t<VBox Spacing=\"10\" Slot.Padding=\"18\">\n"
"\t\t<Label Text=\"Create an account\" TextSize=\"20\"/>\n"
"\t\t<Field Label=\"Name\" Hint=\"Ada Lovelace\"/>\n"
"\t\t<Field Label=\"Email\" Hint=\"ada@example.com\"/>\n"
"\t\t<Field Label=\"Password\">\n"
"\t\t\t<Button Text=\"Show\" Style=\"Flat\"/>\n"
"\t\t</Field>\n"
"\t\t<Repeat Count=\"2\" Var=\"i\">\n"
"\t\t\t<Checkbox Text=\"Option {i + 1}\"/>\n"
"\t\t</Repeat>\n"
"\t\t<Spacer Slot.Fill=\"1\"/>\n"
"\t\t<Button Text=\"Sign up\" Style=\"Primary\" Slot.HAlign=\"Right\"/>\n"
"\t</VBox>\n"
"</Window>\n";

static const char TEMPLATE_WIDGETS[] =
"<Widgets>\n"
"\t<!-- Tags of your own: every window of the folder can use them, as <Card Title=\"...\">...</Card>. -->\n"
"\t<Widget Name=\"Card\" Title=\"Title\" Text=\"\">\n"
"\t\t<Border Padding=\"14\" CornerRadius=\"10\" Background=\"#8080801A\">\n"
"\t\t\t<VBox Spacing=\"6\">\n"
"\t\t\t\t<Label Text=\"{Title}\" TextSize=\"17\"/>\n"
"\t\t\t\t<Label Text=\"{Text}\" Wrap=\"true\" Opacity=\"0.7\"/>\n"
"\t\t\t\t<Children/>\n"
"\t\t\t</VBox>\n"
"\t\t</Border>\n"
"\t</Widget>\n"
"</Widgets>\n";

/** What the palette adds for a tag, when an empty element would not show. */
static const struct { const char* tag; const char* xml; } TEMPLATES[] = {
	{ "VBox", "<VBox Spacing=\"8\"/>" },
	{ "HBox", "<HBox Spacing=\"8\"/>" },
	{ "Grid", "<Grid Columns=\"1,1\" Spacing=\"8\"><Label Text=\"A\"/><Label Text=\"B\" Slot.Column=\"1\"/></Grid>" },
	{ "UniformGrid", "<UniformGrid Columns=\"2\" Spacing=\"8\"><Button Text=\"1\"/><Button Text=\"2\"/><Button Text=\"3\"/><Button Text=\"4\"/></UniformGrid>" },
	{ "Border", "<Border Padding=\"12\"><Label Text=\"Border\"/></Border>" },
	{ "SizeBox", "<SizeBox Width=\"120\" Height=\"80\"><Border/></SizeBox>" },
	{ "Spacer", "<Spacer Width=\"16\" Height=\"16\"/>" },
	{ "ScrollBox", "<ScrollBox Height=\"120\"><VBox Spacing=\"6\"><Label Text=\"One\"/><Label Text=\"Two\"/><Label Text=\"Three\"/></VBox></ScrollBox>" },
	{ "Splitter", "<Splitter Height=\"120\"><Label Text=\"Left\"/><Label Text=\"Right\"/></Splitter>" },
	{ "WrapBox", "<WrapBox Spacing=\"6\"><Button Text=\"One\"/><Button Text=\"Two\"/><Button Text=\"Three\"/></WrapBox>" },
	{ "Canvas", "<Canvas Height=\"120\"><Button Text=\"Here\" Slot.Position=\"20,20\" Slot.Size=\"80,28\"/></Canvas>" },
	{ "Switcher", "<Switcher><Label Text=\"First\"/><Label Text=\"Second\"/></Switcher>" },
	{ "TabView", "<TabView Height=\"140\"><Label Text=\"First tab\" Slot.Title=\"One\"/><Label Text=\"Second tab\" Slot.Title=\"Two\"/></TabView>" },
	{ "Expander", "<Expander Title=\"More\" Expanded=\"true\"><Label Text=\"Details\"/></Expander>" },
	{ "Label", "<Label Text=\"Label\"/>" },
	{ "Button", "<Button Text=\"Button\"/>" },
	{ "Checkbox", "<Checkbox Text=\"Checkbox\"/>" },
	{ "Slider", "<Slider Value=\"0.5\"/>" },
	{ "ProgressBar", "<ProgressBar Value=\"0.4\"/>" },
	{ "Image", "<Image Width=\"96\" Height=\"96\"/>" },
	{ "TextInput", "<TextInput Placeholder=\"Type here\"/>" },
	{ "SpinBox", "<SpinBox Min=\"0\" Max=\"100\" Value=\"10\"/>" },
	{ "ComboBox", "<ComboBox Items=\"One|Two|Three\" Selected=\"0\"/>" },
	{ "ListView", "<ListView Items=\"One|Two|Three\" Height=\"100\"/>" },
	{ "TreeView", "<TreeView Height=\"110\"><Item Text=\"Root\"><Item Text=\"Child\"/><Item Text=\"Other child\"/></Item></TreeView>" },
	{ "MenuBar", "<MenuBar><Menu Text=\"File\"><MenuItem Text=\"Open\"/><Separator/><MenuItem Text=\"Quit\"/></Menu></MenuBar>" },
	{ "Menu", "<Menu Text=\"Menu\"><MenuItem Text=\"Item\"/></Menu>" },
	{ "MenuItem", "<MenuItem Text=\"Item\"/>" },
	{ "Viewport", "<Viewport Width=\"160\" Height=\"100\"/>" },
	{ "Custom", "<Custom Width=\"80\" Height=\"40\"/>" },
	{ "TextEditor", "<TextEditor Height=\"110\" Placeholder=\"Notes\"/>" },
	{ "HeaderBar", "<HeaderBar Title=\"Title\"/>" },
	{ "FileBrowser", "<FileBrowser Height=\"300\"/>" },
	{ "DockArea", "<DockArea Height=\"220\"><Label Text=\"Center\" Slot.Title=\"Main\"/><Label Text=\"Side\" Slot.Title=\"Side\" Slot.Dock=\"Left\" Slot.DockSize=\"0.3\"/></DockArea>" },
	{ "Island", "<Island ExpandOnHover=\"true\"><Label Text=\"Island\"/><VBox Spacing=\"6\" Slot.Padding=\"8\"><Label Text=\"Unfolded\"/><Button Text=\"Action\"/></VBox></Island>" },
	{ "BackgroundBlur", "<BackgroundBlur Style=\"Frosted\" Padding=\"12\"><Label Text=\"Blur\"/></BackgroundBlur>" },
	{ "ColorPicker", "<ColorPicker Color=\"#3B82F6\"/>" },
	{ "Timeline", "<Timeline Duration=\"10\" Height=\"150\"><Track Name=\"Track\"><Clip Start=\"1\" Length=\"3\" Label=\"Clip\"/><Key Time=\"6\"/></Track><Marker Time=\"5\" Label=\"Mark\"/></Timeline>" },
	{ "NodeGraph", "<NodeGraph Height=\"220\"><Node Name=\"a\" Title=\"Source\" X=\"30\" Y=\"40\"><Output Name=\"out\" Label=\"Value\"/></Node>"
		"<Node Name=\"b\" Title=\"Result\" X=\"260\" Y=\"70\"><Input Name=\"in\" Label=\"Value\"/></Node><Link From=\"a.out\" To=\"b.in\"/></NodeGraph>" },
	{ "Item", "<Item Text=\"Item\"/>" },
	{ "Shortcut", "<Shortcut Keys=\"Ctrl+S\" Action=\"file.save\"/>" }
};

/** Tags that are not widgets, and what can be written on them. */
static const struct { const char* tag; const char* attributes; const char* about; } SPECIAL_TAGS[] = {
	{ "Item", "Text", "A row of the list, tree or combo box it is in." },
	{ "Shortcut", "Keys|Action", "Keys that fire an action anywhere in the window." },
	{ "Track", "Name|Color|Height|Locked|Muted", "A track of a Timeline: it holds Clip and Key elements." },
	{ "Clip", "Start|Length|End|Label|Color|Selected", "Something that lasts, on a track of a Timeline." },
	{ "Key", "Time|Label|Color", "A moment, on a track of a Timeline." },
	{ "Marker", "Time|Label|Color", "A flag on the ruler of a Timeline." },
	{ "Node", "Name|Title|X|Y|Color|Selected", "A node of a NodeGraph: Input and Output elements, and a widget for its inside." },
	{ "Input", "Name|Label|Type", "A pin on the left of a Node." },
	{ "Output", "Name|Label|Type", "A pin on the right of a Node." },
	{ "Link", "From|To", "A wire of a NodeGraph, from \"node.output\" to \"node.input\"." },
	{ "Type", "Name|Color", "The color of the pins of a type, in a NodeGraph." },
	{ "Widget", "Name", "Declares a tag: its other attributes are parameters, with their default values." },
	{ "Widgets", "", "A document of <Widget> declarations." },
	{ "Children", "Name", "Where the elements given to a declared tag go." },
	{ "Repeat", "Count|Var", "Repeats what it contains." },
	{ "ForEach", "Items|Var|Index|Separator", "Repeats what it contains for each text of a list." },
	{ "If", "Test", "Keeps what it contains when the test is true." },
	{ "Include", "Source", "Brings in another file." }
};

/* ---------------------------------------------------------------------------
 *  State
 * ------------------------------------------------------------------------- */

#define MAX_HISTORY 200
#define MAX_EXTRA 256
#define MAX_PROBLEMS 64

enum { TAB_WINDOW, TAB_WIDGET, TAB_THEME, TAB_STRINGS };

/** A row of the inspector: which attribute its editor writes. */
typedef struct row_t {
	char name[72];
	RXmlValueType type;
	/** Choices of its combo box: index 0 is "not set". */
	const char* choices;
	Rid swatch;
} row_t;

typedef struct problem_t {
	char text[260];
	int line;
	int error;
} problem_t;

static struct {
	Rid window, dock, source, tree, palette, palette_filter, inspector, problems_list, status, path_label;
	Rid stage, stage_scroll, stage_fit, frame, outline, preview, device_combo, theme_combo;
	/** The size the frame had when it fits the panel (the label above the preview follows it). */
	float fit_width, fit_height;
	Rid fit_timer;
	Rid context_menu;
	Rid run_window;

	/** The document, and the element everything is about (the one clicked last). */
	Rid document;
	Rid selected;
	Rid hovered;
	/** The other elements selected with it (Ctrl / Shift + click), in the order they were added. */
	Rid extra[MAX_EXTRA];
	int extra_count;
	/** A press on an element of a selection of several: it alone is selected at the release, unless it was a drag. */
	Rid press_reduce;

	char* path;
	int modified;
	/** The text of the editor could not be read: the document is the last one that could. */
	int broken;
	/** An interface is shown (R_FALSE: the preview says how to open one). */
	int has_document;

	/** Texts of the document, oldest first; `history_at` is the current one. */
	char* history[MAX_HISTORY];
	int history_count;
	int history_at;
	double history_typed_at;

	/** Set while the designer itself writes the text of the editor, or fills a list. */
	int syncing;
	Rid parse_timer;
	int interact;
	int quitting;
	/* random input (the fuzz command): no file dialog, no message box, no quitting */
	int fuzzing;

	row_t* rows;
	int row_count;
	int row_cap;

	problem_t problems[MAX_PROBLEMS];
	int problem_count;
	int collecting;
	/** Set while the designer tries something that may fail without it being a mistake of the user (a paste of several elements). */
	int quiet;

	/** The tags of the palette as shown (the groups left out), and their items in its tree. */
	char** palette_tags;
	Rid* palette_items;
	int palette_count;
	/** The items of the groups of the palette, and the groups folded by the user. */
	Rid palette_groups[8];
	int palette_folded[8];

	/** The tab in front (TAB_...), and what the Window and Widget tabs hold while the other one is in front. */
	int tab;
	struct {
		char* path;
		char* text;
		int modified, has_document;
		char* history[MAX_HISTORY];
		int history_count, history_at;
	} slots[2];
	Rid tab_buttons[4], appearance, theme_list, theme_root;

	/** Drag and drop in the preview: a widget of the palette, or an element moved. */
	int drag_armed, dragging, drag_from_palette;
	char drag_tag[96];
	Rid drag_node;
	float drag_start_x, drag_start_y, drag_x, drag_y;
	Rid drop_parent;
	int drop_index;
	/** A drop on the Tree: the place it chose, which drop_place gives instead of the one under the pointer. */
	int place_fixed;
	Rid fixed_parent;
	int fixed_index;

	/** The live preview in a window of its own: its frame and its bar. */
	/** The window it is: was it made from a <Window> of the document (its size and its header are those of the XML)? */
	int run_is_window;

	/** The theme the preview wears ("": the look of the designer), and the device it is shown on (0: the size of the XML). */
	char preview_theme[128];
	int device, device_width, device_height;

	/** Where the outline of the selection is drawn: it glides to its place. */
	RSpring box[4];
	RSpring box_shown;
} g;

static void rebuild_all(int keep_inspector);
static void update_path_label(void);
static void on_fit_timer(Rid timer, void* user_data);
static int known_tag(const char* tag);
static void fill_theme_list(void);
static void fill_appearance(void);
static void run_window_follow(void);
static void open_run_window(void);
static void select_node(Rid node, int reveal);
static int load_text(const char* text, int record);
static int is_selected(Rid node);
static int selection_count(void);
static void sync_tree_selection(void);
static void prune_selection(void);
static void selection_shown(int reveal);
static void select_more(Rid node, int range);
static void select_nodes(const Rid* nodes, int count);

/* what the other parts of the designer see (designer.h) */
Rid g_ds_window = R_INVALID_ID;
Rid g_ds_dock = R_INVALID_ID;
char* g_folder = NULL;
int g_ds_fuzzing = 0;
/* ---------------------------------------------------------------------------
 *  Small things
 * ------------------------------------------------------------------------- */

static char* copy_text(const char* text) {
	size_t length = strlen(text) + 1;
	char* copy = (char*)malloc(length);
	memcpy(copy, text, length);
	return copy;
}

static void set_status(const char* format, ...) {
	char text[320];
	va_list args;
	va_start(args, format);
	vsnprintf(text, sizeof(text), format, args);
	va_end(args);
	Rlabel_set_text(g.status, text);
}

void ds_status(const char* format, ...) {
	char text[700];
	va_list args;
	va_start(args, format);
	vsnprintf(text, sizeof(text), format, args);
	va_end(args);
	Rlabel_set_text(g.status, text);
}

static int same_text(const char* a, const char* b) {
	for (; *a && *b; a++, b++) {
		char x = *a, y = *b;
		if (x >= 'A' && x <= 'Z') x = (char)(x + 32);
		if (y >= 'A' && y <= 'Z') y = (char)(y + 32);
		if (x != y) return 0;
	}
	return *a == *b;
}

static int contains_text(const char* text, const char* part) {
	size_t length = strlen(part), i, j;
	if (length == 0) return 1;
	for (i = 0; text[i]; i++) {
		for (j = 0; j < length; j++) {
			char x = text[i + j], y = part[j];
			if (x >= 'A' && x <= 'Z') x = (char)(x + 32);
			if (y >= 'A' && y <= 'Z') y = (char)(y + 32);
			if (x != y) break;
		}
		if (j == length) return 1;
	}
	return 0;
}

static int special_tag(const char* tag) {
	int i;
	for (i = 0; i < (int)(sizeof(SPECIAL_TAGS) / sizeof(SPECIAL_TAGS[0])); i++) {
		if (same_text(SPECIAL_TAGS[i].tag, tag)) return i;
	}
	return -1;
}

static void update_title(void) {
	char title[600];
	const char* file = g.path ? ds_basename(g.path) : "Untitled";
	if (welcome_is_shown() || (!g.has_document && !g_folder)) snprintf(title, sizeof(title), "Raven Designer");
	else if (!g.has_document) snprintf(title, sizeof(title), "%s - Raven Designer", ds_basename(g_folder));
	else if (g_folder) snprintf(title, sizeof(title), "%s%s - %s - Raven Designer", file, g.modified ? " *" : "", ds_basename(g_folder));
	else snprintf(title, sizeof(title), "%s%s - Raven Designer", file, g.modified ? " *" : "");
	Rwindow_set_title(g.window, title);
	/* (the header shows no title: the island in its middle says what is open) */
	island_refresh();
}

/* ---------------------------------------------------------------------------
 *  Problems: what the library reports while the preview is built
 * ------------------------------------------------------------------------- */

static void add_problem(const char* text, int line, int error) {
	problem_t* problem;
	if (g.problem_count >= MAX_PROBLEMS) return;
	problem = &g.problems[g.problem_count++];
	snprintf(problem->text, sizeof(problem->text), "%s", text);
	problem->line = line;
	problem->error = error;
}

static void on_error(RError code, RSeverity severity, const char* detail) {
	if (g.quiet && code == R_ERROR_XML) return;
	/* (the editor of themes reads what loading its theme said) */
	te_error(code, severity, detail);
	if (g.collecting && code == R_ERROR_XML) {
		/* "XML line 12, column 3: ..." */
		const char* at = strstr(detail, "line ");
		add_problem(detail, at ? atoi(at + 5) : 0, severity != R_SEVERITY_WARNING);
		return;
	}
	crash_report_note("Raven said: %s: %.100s", Rerror_to_string(code), detail);
	fprintf(stderr, "[Raven] %s (%s): %s\n", Rerror_to_string(code), Rseverity_to_string(severity), detail);
}

static void show_problems(void) {
	int i;
	Rlist_view_clear_items(g.problems_list);
	Rtext_editor_clear_markers(g.source);
	for (i = 0; i < g.problem_count; i++) {
		const problem_t* problem = &g.problems[i];
		Rlist_view_add_item(g.problems_list, problem->text);
		if (problem->line > 0) {
			Rtext_editor_add_marker(g.source, problem->line - 1, 0, -1, problem->error ? R_MARKER_ERROR : R_MARKER_WARNING, problem->text);
		}
	}
	if (g.problem_count == 0) {
		Rlist_view_add_item(g.problems_list, "No problems.");
		Rdock_area_set_panel_title(g.dock, g.problems_list, "Problems");
	} else {
		char title[32];
		snprintf(title, sizeof(title), "Problems (%d)", g.problem_count);
		Rdock_area_set_panel_title(g.dock, g.problems_list, title);
	}
}

/* ---------------------------------------------------------------------------
 *  Nodes
 * ------------------------------------------------------------------------- */

/* The place of a node as the indices from the root: what survives the text being read again. */
static int node_path(Rid node, int* path, int max) {
	int depth = 0, i;
	Rid walk;
	for (walk = node; Rxml_node_get_parent(walk) != R_INVALID_ID; walk = Rxml_node_get_parent(walk)) depth++;
	if (depth > max) return -1;
	for (walk = node, i = depth - 1; i >= 0; i--, walk = Rxml_node_get_parent(walk)) path[i] = Rxml_node_get_index(walk);
	return depth;
}

static Rid node_at_path(const int* path, int depth) {
	Rid node = Rxml_document_get_root(g.document);
	int i;
	for (i = 0; i < depth; i++) {
		Rid child = Rxml_node_get_child(node, path[i]);
		if (child == R_INVALID_ID) break;
		node = child;
	}
	return node;
}

/* "Button  ok  \"Greet\"": what the tree and the status bar call an element. */
static void node_caption(Rid node, char* out, size_t size) {
	const char* name;
	const char* text;
	if (Rxml_node_is_comment(node)) {
		snprintf(out, size, "<!--%.40s-->", Rxml_node_get_text(node));
		return;
	}
	name = Rxml_node_get_attribute(node, "Name");
	text = Rxml_node_get_attribute(node, "Text");
	if (!text) text = Rxml_node_get_attribute(node, "Title");
	if (!text && Rxml_node_get_text(node)[0]) text = Rxml_node_get_text(node);
	if (name && text) snprintf(out, size, "%s  %s  \"%.28s\"", Rxml_node_get_tag(node), name, text);
	else if (name) snprintf(out, size, "%s  %s", Rxml_node_get_tag(node), name);
	else if (text) snprintf(out, size, "%s  \"%.28s\"", Rxml_node_get_tag(node), text);
	else snprintf(out, size, "%s", Rxml_node_get_tag(node));
}

/* The widget that shows a node in the preview: its own, or the one of the nearest element around it that has one. */
static Rid node_widget(Rid node) {
	if (node == R_INVALID_ID || !Rxml_node_is_valid(node)) return R_INVALID_ID;
	for (; node != R_INVALID_ID; node = Rxml_node_get_parent(node)) {
		Rid widget = Rxml_node_get_widget(node);
		if (widget != R_INVALID_ID && Rwidget_is_valid(widget)) return widget;
	}
	/* the root of a window, or something in it that is not a widget (a shortcut): the frame that stands for the window */
	return g.frame;
}

/* ---------------------------------------------------------------------------
 *  History
 * ------------------------------------------------------------------------- */

static void history_push(const char* text, int typed) {
	int i;
	if (g.history_count > 0 && strcmp(g.history[g.history_at], text) == 0) return;
	/* what was undone is forgotten once something else is done */
	for (i = g.history_at + 1; i < g.history_count; i++) free(g.history[i]);
	if (g.history_count > 0) g.history_count = g.history_at + 1;
	/* characters typed in a row are one step */
	if (typed && g.history_typed_at > 0.0 && Rget_time() - g.history_typed_at < 1.5 && g.history_count > 1) {
		free(g.history[g.history_at]);
		g.history[g.history_at] = copy_text(text);
		g.history_typed_at = Rget_time();
		return;
	}
	if (g.history_count == MAX_HISTORY) {
		free(g.history[0]);
		memmove(g.history, g.history + 1, sizeof(char*) * (MAX_HISTORY - 1));
		g.history_count--;
	}
	g.history[g.history_count++] = copy_text(text);
	g.history_at = g.history_count - 1;
	g.history_typed_at = typed ? Rget_time() : 0.0;
}

static void history_clear(void) {
	int i;
	for (i = 0; i < g.history_count; i++) free(g.history[i]);
	g.history_count = 0;
	g.history_at = 0;
	g.history_typed_at = 0.0;
}

/* ---------------------------------------------------------------------------
 *  Preview
 * ------------------------------------------------------------------------- */

static Rid node_under(float x, float y);
static int accepts_children(Rid node);

/* Laid out from left to right: where a pointer falls among the children goes by x. */
static int horizontal(Rid node) {
	const char* tag = Rxml_node_get_tag(node);
	const char* orientation = Rxml_node_get_attribute(node, "Orientation");
	if (orientation) return same_text(orientation, "Horizontal") && !same_text(tag, "ScrollBox");
	return same_text(tag, "HBox") || same_text(tag, "HorizontalBox") || same_text(tag, "WrapBox") || same_text(tag, "MenuBar") ||
		same_text(tag, "HeaderBar") || same_text(tag, "Splitter");
}

static int inside_of(Rid node, Rid ancestor) {
	for (; node != R_INVALID_ID; node = Rxml_node_get_parent(node)) if (node == ancestor) return R_TRUE;
	return R_FALSE;
}

/* The index among the children of `parent` where the pointer falls, by their places on screen. */
static int index_at(Rid parent, float x, float y) {
	int i, count = Rxml_node_get_child_count(parent), across = horizontal(parent);
	for (i = 0; i < count; i++) {
		Rid child = Rxml_node_get_child(parent, i);
		Rid widget = child == g.drag_node ? R_INVALID_ID : Rxml_node_get_widget(child);
		float wx, wy, ww, wh;
		if (widget == R_INVALID_ID || !Rwidget_is_valid(widget)) continue;
		Rwidget_get_rect(widget, &wx, &wy, &ww, &wh);
		if (across ? x < wx + ww * 0.5f : y < wy + wh * 0.5f) return i;
	}
	return count;
}

/*
 * Where something dropped at (x, y) goes: inside the panel under the pointer,
 * at the place among its children the pointer shows; next to a widget that
 * holds nothing, before or after it; near the edge of a panel, next to it.
 * R_FALSE when it cannot go there (an element into itself).
 */
static int drop_place(float x, float y, Rid* parent, int* index) {
	Rid node, widget;
	float wx, wy, ww, wh;
	if (g.place_fixed) {
		*parent = g.fixed_parent;
		*index = g.fixed_index;
		return g.fixed_parent != R_INVALID_ID;
	}
	node = node_under(x, y);
	if (node == R_INVALID_ID) node = Rxml_document_get_root(g.document);
	/* (what is dragged is not a place: what holds it is) */
	if (g.drag_node != R_INVALID_ID) {
		while (node != R_INVALID_ID && inside_of(node, g.drag_node)) node = Rxml_node_get_parent(node);
		if (node == R_INVALID_ID) return R_FALSE;
	}
	widget = Rxml_node_get_widget(node);
	if (accepts_children(node) && !(Rxml_get_tag_children(Rxml_node_get_tag(node)) == 1 && Rxml_node_get_child_count(node) > 0 &&
		!(Rxml_node_get_child_count(node) == 1 && Rxml_node_get_child(node, 0) == g.drag_node))) {
		int edge = R_FALSE;
		Rid up = Rxml_node_get_parent(node);
		/* near the edge of a panel (not the root): next to it, in its parent */
		if (up != R_INVALID_ID && widget != R_INVALID_ID && Rwidget_is_valid(widget)) {
			Rwidget_get_rect(widget, &wx, &wy, &ww, &wh);
			edge = horizontal(up) ? (x < wx + 6.0f || x > wx + ww - 6.0f) : (y < wy + 6.0f || y > wy + wh - 6.0f);
			if (edge) {
				*parent = up;
				*index = Rxml_node_get_index(node) + ((horizontal(up) ? x > wx + ww * 0.5f : y > wy + wh * 0.5f) ? 1 : 0);
				return R_TRUE;
			}
		}
		*parent = node;
		*index = index_at(node, x, y);
		return R_TRUE;
	}
	*parent = Rxml_node_get_parent(node);
	if (*parent == R_INVALID_ID) return R_FALSE;
	*index = Rxml_node_get_index(node);
	if (widget != R_INVALID_ID && Rwidget_is_valid(widget)) {
		Rwidget_get_rect(widget, &wx, &wy, &ww, &wh);
		if (horizontal(*parent) ? x > wx + ww * 0.5f : y > wy + wh * 0.5f) (*index)++;
	}
	return R_TRUE;
}

/* The line that shows where a drop goes: between two children of the panel, across it. */
static void drop_line(Rid parent, int index, float ox, float oy, RPainter* painter, float r, float gr, float b) {
	Rid widget = node_widget(parent), child = R_INVALID_ID;
	float px, py, pw, ph, cx, cy, cw, ch;
	int count = Rxml_node_get_child_count(parent), across = horizontal(parent), after = R_FALSE;
	if (widget == R_INVALID_ID) return;
	Rwidget_get_rect(widget, &px, &py, &pw, &ph);
	/* the panel that receives it */
	Rdraw_rect_outline(painter, px - ox, py - oy, pw, ph, 4.0f, 1.5f, r, gr, b, 0.8f);
	/* the child it goes before (or the last one it goes after) */
	if (index < count) child = Rxml_node_get_child(parent, index);
	if (child == g.drag_node && index + 1 <= count) child = index + 1 < count ? Rxml_node_get_child(parent, index + 1) : R_INVALID_ID;
	if (child == R_INVALID_ID && count > 0) {
		child = Rxml_node_get_child(parent, count - 1);
		after = R_TRUE;
	}
	if (child != R_INVALID_ID && Rxml_node_get_widget(child) != R_INVALID_ID && Rwidget_is_valid(Rxml_node_get_widget(child))) {
		Rwidget_get_rect(Rxml_node_get_widget(child), &cx, &cy, &cw, &ch);
		if (across) Rdraw_rect(painter, (after ? cx + cw + 1.0f : cx - 3.0f) - ox, cy - oy, 3.0f, ch, 1.5f, r, gr, b, 1.0f);
		else Rdraw_rect(painter, cx - ox, (after ? cy + ch + 1.0f : cy - 3.0f) - oy, cw, 3.0f, 1.5f, r, gr, b, 1.0f);
	} else {
		/* an empty panel: a line across its middle */
		if (across) Rdraw_rect(painter, px + pw * 0.5f - 1.5f - ox, py + 4.0f - oy, 3.0f, ph - 8.0f, 1.5f, r, gr, b, 1.0f);
		else Rdraw_rect(painter, px + 4.0f - ox, py + ph * 0.5f - 1.5f - oy, pw - 8.0f, 3.0f, 1.5f, r, gr, b, 1.0f);
	}
}

static void outline_paint(Rid widget, RPainter* painter, float width, float height, void* user_data) {
	float ox, oy, ow, oh, x, y, w, h, shown, r, gr, b, a;
	Rid target = node_widget(g.selected);
	Rid hover = g.interact || g.dragging ? R_INVALID_ID : node_widget(g.hovered);
	(void)user_data;
	(void)width;
	(void)height;
	Rwidget_get_rect(widget, &ox, &oy, &ow, &oh);
	Rtheme_get_color(R_COLOR_ACCENT, &r, &gr, &b, &a);
	/* the frame fits the panel: its size is said above it when it changed (not now: a label is not laid out while painting) */
	if (Rwidget_get_parent(g.stage) == g.stage_fit) {
		float fx, fy, fw, fh;
		Rwidget_get_rect(g.frame, &fx, &fy, &fw, &fh);
		if ((fabsf(fw - g.fit_width) > 0.5f || fabsf(fh - g.fit_height) > 0.5f) && fw > 0.0f) {
			g.fit_width = fw;
			g.fit_height = fh;
			if (g.fit_timer == R_INVALID_ID || !Rtimer_is_valid(g.fit_timer)) g.fit_timer = Rtimer_create(0.05, R_FALSE, on_fit_timer, NULL);
		}
	}

	if (hover != R_INVALID_ID && hover != target && !is_selected(g.hovered)) {
		Rwidget_get_rect(hover, &x, &y, &w, &h);
		Rdraw_rect_outline(painter, x - ox, y - oy, w, h, 3.0f, 1.0f, r, gr, b, 0.45f);
	}
	/* the others of a selection of several: a frame each, and their tags, smaller */
	{
		int i;
		for (i = 0; i < g.extra_count; i++) {
			Rid other = Rxml_node_is_valid(g.extra[i]) ? node_widget(g.extra[i]) : R_INVALID_ID;
			float tw = 0.0f, th = 0.0f;
			if (other == R_INVALID_ID) continue;
			Rwidget_get_rect(other, &x, &y, &w, &h);
			x -= ox;
			y -= oy;
			Rdraw_rect_outline(painter, x - 1.0f, y - 1.0f, w + 2.0f, h + 2.0f, 5.0f, 1.5f, r, gr, b, g.dragging && !g.drag_from_palette ? 0.3f : 0.85f);
			Rfont_measure_text(R_INVALID_ID, 10.0f, Rxml_node_get_tag(g.extra[i]), &tw, &th);
			Rdraw_rect(painter, x - 1.0f, y - 1.0f, tw + 10.0f, 15.0f, 4.0f, r, gr, b, 0.75f);
			Rdraw_text(painter, R_INVALID_ID, 10.0f, x + 4.0f, y + 0.5f, Rxml_node_get_tag(g.extra[i]), 1.0f, 1.0f, 1.0f, 1.0f);
		}
	}
	/* the frame of the selection flies from one element to the next; inside, nothing: the widget shows as it is */
	shown = Rdraw_spring(painter, &g.box_shown, target != R_INVALID_ID ? 1.0f : 0.0f, R_MOTION_SMOOTH);
	if (target != R_INVALID_ID) {
		char caption[96];
		float text_width = 0.0f, text_height = 0.0f, label_y;
		Rwidget_get_rect(target, &x, &y, &w, &h);
		x = Rdraw_spring(painter, &g.box[0], x - ox, R_MOTION_BOUNCY);
		y = Rdraw_spring(painter, &g.box[1], y - oy, R_MOTION_BOUNCY);
		w = Rdraw_spring(painter, &g.box[2], w, R_MOTION_BOUNCY);
		h = Rdraw_spring(painter, &g.box[3], h, R_MOTION_BOUNCY);
		Rdraw_rect_outline(painter, x - 1.0f, y - 1.0f, w + 2.0f, h + 2.0f, 5.0f, 2.0f, r, gr, b, shown * (g.dragging && !g.drag_from_palette ? 0.35f : 1.0f));
		/* its tag, on a tab above it (inside when there is no room); with the others, how many they are */
		if (g.extra_count > 0) snprintf(caption, sizeof(caption), "%s   %d selected", Rxml_node_get_tag(g.selected), g.extra_count + 1);
		else snprintf(caption, sizeof(caption), "%s", Rxml_node_get_tag(g.selected));
		Rfont_measure_text(R_INVALID_ID, 11.0f, caption, &text_width, &text_height);
		label_y = y - 19.0f < -24.0f ? y + 2.0f : y - 19.0f;
		Rdraw_rect(painter, x - 1.0f, label_y, text_width + 12.0f, 17.0f, 4.0f, r, gr, b, shown);
		Rdraw_text(painter, R_INVALID_ID, 11.0f, x + 5.0f, label_y + 2.0f, caption, 1.0f, 1.0f, 1.0f, shown);
	}
	/* a drag: where it would go, and what is carried, under the pointer */
	if (g.dragging) {
		char caption[120];
		float text_width = 0.0f, text_height = 0.0f;
		int inside = g.drag_x >= ox && g.drag_x < ox + ow && g.drag_y >= oy && g.drag_y < oy + oh;
		if (inside && g.drop_parent != R_INVALID_ID) drop_line(g.drop_parent, g.drop_index, ox, oy, painter, r, gr, b);
		if (!g.drag_from_palette && is_selected(g.drag_node) && g.extra_count > 0) snprintf(caption, sizeof(caption), "%d elements", g.extra_count + 1);
		else snprintf(caption, sizeof(caption), "%s%s", g.drag_from_palette ? "+ " : "", g.drag_from_palette ? g.drag_tag : Rxml_node_get_tag(g.drag_node));
		Rfont_measure_text(R_INVALID_ID, 12.0f, caption, &text_width, &text_height);
		Rdraw_rect(painter, g.drag_x - ox + 14.0f, g.drag_y - oy + 10.0f, text_width + 16.0f, 22.0f, 11.0f, r, gr, b, inside ? 0.95f : 0.5f);
		Rdraw_text(painter, R_INVALID_ID, 12.0f, g.drag_x - ox + 22.0f, g.drag_y - oy + 14.0f, caption, 1.0f, 1.0f, 1.0f, 1.0f);
	}
}

static Rid node_under(float x, float y) {
	Rid widget = g.preview != R_INVALID_ID && Rwidget_is_valid(g.preview) ? Rwidget_pick(g.preview, x, y) : R_INVALID_ID;
	while (widget != R_INVALID_ID) {
		Rid node = Rwidget_get_xml_node(widget);
		if (node != R_INVALID_ID) return node;
		if (widget == g.preview) break;
		widget = Rwidget_get_parent(widget);
	}
	return R_INVALID_ID;
}

static int outline_event(Rid widget, const REvent* event, void* user_data) {
	Rid node;
	(void)user_data;
	switch (event->type) {
		case R_EVENT_MOUSE_MOVE:
			node = node_under(event->x, event->y);
			if (node != g.hovered) {
				g.hovered = node;
				Rwidget_invalidate(widget);
			}
			return R_TRUE;
		case R_EVENT_MOUSE_LEAVE:
			g.hovered = R_INVALID_ID;
			Rwidget_invalidate(widget);
			return R_FALSE;
		case R_EVENT_MOUSE_DOWN:
			Rwidget_focus(widget);
			node = node_under(event->x, event->y);
			/* Ctrl (Cmd) + click: one more, or one less; Shift + click: the ones between */
			if (event->button == R_MOUSE_LEFT && (event->modifiers & (R_MOD_CTRL | R_MOD_SUPER | R_MOD_SHIFT))) {
				if (node != R_INVALID_ID) select_more(node, (event->modifiers & R_MOD_SHIFT) && !(event->modifiers & (R_MOD_CTRL | R_MOD_SUPER)));
				return R_TRUE;
			}
			/* a press on one of several selected keeps them (a drag takes them all); the release selects it alone */
			if (node != R_INVALID_ID && is_selected(node) && selection_count() > 1) {
				if (event->button == R_MOUSE_LEFT) {
					g.press_reduce = node;
				} else {
					g.press_reduce = R_INVALID_ID;
					Rmenu_open(g.context_menu, g.window, event->x, event->y);
				}
				return R_TRUE;
			}
			/* a click on nothing selects nothing (the window is selected in the Tree, or by a click on its content) */
			select_node(node, R_TRUE);
			if (event->button == R_MOUSE_RIGHT && node != R_INVALID_ID) Rmenu_open(g.context_menu, g.window, event->x, event->y);
			return R_TRUE;
		case R_EVENT_KEY_DOWN:
			/* Escape: nothing selected */
			if (event->key == R_KEY_ESCAPE && g.selected != R_INVALID_ID && !g.dragging) {
				select_node(R_INVALID_ID, R_FALSE);
				set_status("Nothing selected.");
				return R_TRUE;
			}
			return R_FALSE;
		case R_EVENT_MOUSE_UP:
			if (g.press_reduce != R_INVALID_ID) {
				node = g.press_reduce;
				g.press_reduce = R_INVALID_ID;
				if (Rxml_node_is_valid(node)) select_node(node, R_TRUE);
			}
			return R_TRUE;
		default:
			return R_FALSE;
	}
}

/* The Widget tab: a use of each tag the document declares, under its name. */
static Rid build_showcase(void) {
	Rid root = Rxml_document_get_root(g.document);
	ds_buffer_t xml = { 0 };
	const char* text = Rxml_document_to_string(g.document);
	Rid widget;
	int i, count, shown = 0;
	/* the declarations are made known first (they replace those of the same name) */
	Rxml_register_widgets(text, strlen(text));
	ds_buffer_printf(&xml, "<VBox Spacing=\"20\" Slot.Padding=\"22\">");
	count = same_text(Rxml_node_get_tag(root), "Widget") ? 1 : Rxml_node_get_child_count(root);
	for (i = 0; i < count; i++) {
		Rid node = same_text(Rxml_node_get_tag(root), "Widget") ? root : Rxml_node_get_child(root, i);
		const char* name;
		if (Rxml_node_is_comment(node) || !same_text(Rxml_node_get_tag(node), "Widget")) continue;
		name = Rxml_node_get_attribute(node, "Name");
		if (!name || !Rxml_is_widget_registered(name)) continue;
		ds_buffer_printf(&xml, "<VBox Spacing=\"6\"><Label Text=\"&lt;%s/&gt;\" TextSize=\"11\" Opacity=\"0.55\"/><%s/></VBox>", name, name);
		shown++;
	}
	if (!shown) ds_buffer_printf(&xml, "<Label Wrap=\"true\" Opacity=\"0.6\" Text=\"No tag declared yet: add a &lt;Widget Name=&quot;Card&quot;&gt; with what it stands for inside.\"/>");
	ds_buffer_printf(&xml, "</VBox>");
	widget = Rwidget_create_from_xml(xml.data, xml.size);
	ds_buffer_free(&xml);
	return widget;
}

/* ---- the Widget tab: the declarations themselves, built from the nodes of the document ---- */

/* What a node was before build_library changed it for a moment. */
typedef struct snapshot_t {
	Rid node;
	char* tag;
	char* text;
	int count;
	char** names;
	char** values;
} snapshot_t;

typedef struct snapshots_t {
	snapshot_t* items;
	int count, cap;
} snapshots_t;

static void snapshot_take(snapshots_t* list, Rid node) {
	snapshot_t* shot;
	int i;
	for (i = 0; i < list->count; i++) if (list->items[i].node == node) return;
	if (list->count == list->cap) {
		list->cap = list->cap ? list->cap * 2 : 32;
		list->items = (snapshot_t*)realloc(list->items, sizeof(snapshot_t) * (size_t)list->cap);
	}
	shot = &list->items[list->count++];
	shot->node = node;
	shot->tag = copy_text(Rxml_node_get_tag(node));
	shot->text = copy_text(Rxml_node_get_text(node));
	shot->count = Rxml_node_get_attribute_count(node);
	shot->names = (char**)calloc((size_t)shot->count + 1, sizeof(char*));
	shot->values = (char**)calloc((size_t)shot->count + 1, sizeof(char*));
	for (i = 0; i < shot->count; i++) {
		shot->names[i] = copy_text(Rxml_node_get_attribute_name(node, i));
		shot->values[i] = copy_text(Rxml_node_get_attribute_value(node, i));
	}
}

static void clear_attributes(Rid node) {
	while (Rxml_node_get_attribute_count(node) > 0) {
		char* name = copy_text(Rxml_node_get_attribute_name(node, 0));
		Rxml_node_set_attribute(node, name, NULL);
		free(name);
	}
}

/* Everything back as it was, in the same order: the text of the document does not change. */
static void snapshots_restore(snapshots_t* list) {
	int i, k;
	for (i = list->count - 1; i >= 0; i--) {
		snapshot_t* shot = &list->items[i];
		Rxml_node_set_tag(shot->node, shot->tag);
		Rxml_node_set_text(shot->node, shot->text);
		clear_attributes(shot->node);
		for (k = 0; k < shot->count; k++) Rxml_node_set_attribute(shot->node, shot->names[k], shot->values[k]);
		for (k = 0; k < shot->count; k++) {
			free(shot->names[k]);
			free(shot->values[k]);
		}
		free(shot->names);
		free(shot->values);
		free(shot->tag);
		free(shot->text);
	}
	free(list->items);
	memset(list, 0, sizeof(*list));
}

/* "{Title}" becomes the default value of the parameter Title (its name when it has none: something to see and to click). */
static char* fill_parameters(const char* text, Rid declaration) {
	ds_buffer_t out = { 0 };
	const char* walk = text;
	int changed = R_FALSE;
	while (*walk) {
		const char* close = walk[0] == '{' ? strchr(walk, '}') : NULL;
		if (close && close - walk > 1 && close - walk < 64) {
			char name[64];
			const char* value;
			snprintf(name, sizeof(name), "%.*s", (int)(close - walk - 1), walk + 1);
			value = strcmp(name, "Name") ? Rxml_node_get_attribute(declaration, name) : NULL;
			if (value) {
				ds_buffer_add(&out, value[0] ? value : name, strlen(value[0] ? value : name));
				walk = close + 1;
				changed = R_TRUE;
				continue;
			}
		}
		ds_buffer_add(&out, walk, 1);
		walk++;
	}
	if (!changed) {
		ds_buffer_free(&out);
		return NULL;
	}
	return out.data ? out.data : copy_text("");
}

/* Inside a declaration: <Children/> becomes an empty box, the parameters their values. */
static void prepare_body(snapshots_t* list, Rid node, Rid declaration) {
	int i, count;
	char* filled;
	if (Rxml_node_is_comment(node)) return;
	if (same_text(Rxml_node_get_tag(node), "Children")) {
		snapshot_take(list, node);
		Rxml_node_set_tag(node, "VBox");
		clear_attributes(node);
		return;
	}
	if ((filled = fill_parameters(Rxml_node_get_text(node), declaration)) != NULL) {
		snapshot_take(list, node);
		Rxml_node_set_text(node, filled);
		free(filled);
	}
	count = Rxml_node_get_attribute_count(node);
	for (i = 0; i < count; i++) {
		if ((filled = fill_parameters(Rxml_node_get_attribute_value(node, i), declaration)) != NULL) {
			char* name = copy_text(Rxml_node_get_attribute_name(node, i));
			snapshot_take(list, node);
			Rxml_node_set_attribute(node, name, filled);
			free(name);
			free(filled);
		}
	}
	count = Rxml_node_get_child_count(node);
	for (i = 0; i < count; i++) prepare_body(list, Rxml_node_get_child(node, i), declaration);
}

/* A <Widget> shown as what it stands for: a box around its body, its parameters at their default values. */
static void prepare_declaration(snapshots_t* list, Rid declaration) {
	int i, count = Rxml_node_get_child_count(declaration);
	/* (the body first: it reads the parameters on the declaration) */
	for (i = 0; i < count; i++) prepare_body(list, Rxml_node_get_child(declaration, i), declaration);
	snapshot_take(list, declaration);
	Rxml_node_set_tag(declaration, "VBox");
	clear_attributes(declaration);
}

/*
 * The Widget tab: the bodies of the declarations, one under the other,
 * built from the nodes of the document itself, so that a click selects
 * what it is on and a drag moves it, as in the Window tab. For a moment the
 * document is made a plain one (<Widgets> and <Widget> become boxes, the
 * parameters their values), built, then put back exactly as it was.
 */
static Rid build_library(void) {
	Rid root = Rxml_document_get_root(g.document), widget;
	const char* tag = Rxml_node_get_tag(root);
	const char* text = Rxml_document_to_string(g.document);
	snapshots_t list = { 0 };
	int i, count;
	/* the declarations are made known first: a declared tag may use another one */
	Rxml_register_widgets(text, strlen(text));
	if (same_text(tag, "Widget")) {
		prepare_declaration(&list, root);
	} else if (same_text(tag, "Widgets")) {
		count = Rxml_node_get_child_count(root);
		for (i = 0; i < count; i++) {
			Rid child = Rxml_node_get_child(root, i);
			if (!Rxml_node_is_comment(child) && same_text(Rxml_node_get_tag(child), "Widget")) prepare_declaration(&list, child);
		}
		snapshot_take(&list, root);
		Rxml_node_set_tag(root, "VBox");
		clear_attributes(root);
		Rxml_node_set_attribute(root, "Spacing", "24");
		Rxml_node_set_attribute(root, "Slot.Padding", "22");
	} else {
		return Rxml_document_create_widget(g.document);
	}
	widget = Rxml_document_create_widget(g.document);
	snapshots_restore(&list);
	return widget;
}

/* In a library, a widget goes inside a <Widget>, not next to the declarations. */
static int drop_allowed(Rid parent) {
	return !(g.tab == TAB_WIDGET && parent != R_INVALID_ID && same_text(Rxml_node_get_tag(parent), "Widgets"));
}

static void rebuild_run_window(void);

/* The size of the frame that fits the panel, said above the preview (once the layout gave it). */
static void on_fit_timer(Rid timer, void* user_data) {
	char text[64];
	(void)timer;
	(void)user_data;
	g.fit_timer = R_INVALID_ID;
	if (g.fit_width <= 0.0f) return;
	snprintf(text, sizeof(text), "%.0f x %.0f, the room of the panel", (double)g.fit_width, (double)g.fit_height);
	Rlabel_set_text(Rwidget_find(g.window, "stage.size"), text);
	update_path_label();
}

/*
 * Where the stage is: in a box that scrolls, around a frame of a given size
 * (the XML, a device, a piece of interface), or filling the panel, the frame
 * as large as the room it has.
 */
static void stage_host(int fit) {
	Rid host = fit ? g.stage_fit : g.stage_scroll;
	if (Rwidget_get_parent(g.stage) != host) {
		Rwidget_add_child(host, g.stage);
		if (fit) {
			Rslot_set_padding(g.stage, 24.0f, 24.0f, 24.0f, 24.0f);
			Rslot_set_halign(g.stage, R_HALIGN_FILL);
			Rslot_set_valign(g.stage, R_VALIGN_FILL);
		} else {
			Rslot_set_padding(g.stage, 28.0f, 28.0f, 28.0f, 28.0f);
			Rslot_set_halign(g.stage, R_HALIGN_CENTER);
			Rslot_set_valign(g.stage, R_VALIGN_TOP);
		}
	}
	Rwidget_set_visibility(g.stage_fit, fit ? R_VISIBLE : R_COLLAPSED);
	Rwidget_set_visibility(g.stage_scroll, fit ? R_COLLAPSED : R_VISIBLE);
}

static void rebuild_preview(void) {
	Rid root = Rxml_document_get_root(g.document);
	const char* tag = Rxml_node_get_tag(root);
	char size[64];

	if (g.preview != R_INVALID_ID && Rwidget_is_valid(g.preview)) Rwidget_destroy(g.preview);
	g.preview = R_INVALID_ID;
	g.hovered = R_INVALID_ID;

	g.problem_count = 0;
	g.collecting = R_TRUE;
	/* the window of its own first: the nodes lead to the widgets built last, those of the preview here */
	rebuild_run_window();
	g.preview = g.tab == TAB_WIDGET ? build_library() : Rxml_document_create_widget(g.document);
	g.collecting = R_FALSE;

	/* the frame fits the panel: a window as it is when the user resizes it */
	stage_host(g.device_width < 0 && same_text(tag, "Window") && g.tab == TAB_WINDOW);
	if (g.device_width < 0 && same_text(tag, "Window") && g.tab == TAB_WINDOW) {
		Rwidget_set_size(g.frame, -1.0f, -1.0f);
		g.fit_width = g.fit_height = 0.0f;
		snprintf(size, sizeof(size), "%s", "fits the panel");
	} else if (g.device_width > 0 && (same_text(tag, "Window") || g.device)) {
		/* the size of a device: the interface as it will be there */
		Rwidget_set_size(g.frame, (float)g.device_width, (float)g.device_height);
		snprintf(size, sizeof(size), "%d x %d", g.device_width, g.device_height);
	} else if (same_text(tag, "Window") && g.tab == TAB_WINDOW) {
		const char* width = Rxml_node_get_attribute(root, "Width");
		const char* height = Rxml_node_get_attribute(root, "Height");
		int w = width ? atoi(width) : 800, h = height ? atoi(height) : 600;
		if (w <= 0) w = 800;
		if (h <= 0) h = 600;
		Rwidget_set_size(g.frame, (float)w, (float)h);
		snprintf(size, sizeof(size), "%d x %d", w, h);
	} else {
		/* a piece of interface: the size it asks for */
		Rwidget_set_size(g.frame, -1.0f, -1.0f);
		snprintf(size, sizeof(size), "%s", g.tab == TAB_WIDGET ? "the tags it declares" : tag);
	}
	Rlabel_set_text(Rwidget_find(g.window, "stage.size"), size);
	if (g.preview != R_INVALID_ID) Rwidget_add_child(g.frame, g.preview);
	show_problems();
	Rwidget_invalidate(g.outline);
}

/* ---------------------------------------------------------------------------
 *  Tree
 * ------------------------------------------------------------------------- */

static void tree_add(Rid parent_item, Rid node) {
	char caption[160];
	Rid item;
	int i, count = Rxml_node_get_child_count(node);
	node_caption(node, caption, sizeof(caption));
	item = Rtree_view_add_item(g.tree, parent_item, caption);
	icons_set_tree_item(item, Rxml_node_is_comment(node) ? "#comment" : Rxml_node_get_tag(node));
	Rtree_item_set_user_handle(item, (void*)(size_t)node);
	Rtree_item_set_expanded(item, R_TRUE);
	for (i = 0; i < count; i++) tree_add(item, Rxml_node_get_child(node, i));
}

static void rebuild_tree(void) {
	g.syncing++;
	Rtree_view_clear_items(g.tree);
	tree_add(R_INVALID_ID, Rxml_document_get_root(g.document));
	g.syncing--;
	prune_selection();
	sync_tree_selection();
}

static Rid tree_item_of(Rid item, Rid node) {
	int i, count;
	if (item != R_INVALID_ID && (Rid)(size_t)Rtree_item_get_user_handle(item) == node) return item;
	count = item == R_INVALID_ID ? Rtree_view_get_item_count(g.tree) : Rtree_item_get_child_count(item);
	for (i = 0; i < count; i++) {
		Rid found = tree_item_of(item == R_INVALID_ID ? Rtree_view_get_item(g.tree, i) : Rtree_item_get_child(item, i), node);
		if (found != R_INVALID_ID) return found;
	}
	return R_INVALID_ID;
}

/* The Tree selects several items with Ctrl and Shift too: the selection is what it shows, the item clicked last first. */
static void on_tree_selected(Rid tree, Rid item, void* user_data) {
	int i, count;
	(void)user_data;
	if (g.syncing) return;
	g.selected = item != R_INVALID_ID ? (Rid)(size_t)Rtree_item_get_user_handle(item) : R_INVALID_ID;
	g.extra_count = 0;
	g.press_reduce = R_INVALID_ID;
	count = g.selected != R_INVALID_ID ? Rtree_view_get_selection_count(tree) : 0;
	for (i = 0; i < count && g.extra_count < MAX_EXTRA; i++) {
		Rid node = (Rid)(size_t)Rtree_item_get_user_handle(Rtree_view_get_selection(tree, i));
		if (node != g.selected) g.extra[g.extra_count++] = node;
	}
	prune_selection();
	selection_shown(R_TRUE);
}

/* ---------------------------------------------------------------------------
 *  The document changed
 * ------------------------------------------------------------------------- */

/* After the nodes were changed: the text follows, then everything that shows the document. */
static void document_changed(int keep_inspector) {
	const char* text = Rxml_document_to_string(g.document);
	int line = g.selected != R_INVALID_ID && Rxml_node_is_valid(g.selected) ? Rxml_node_get_line(g.selected) : 0;
	g.syncing++;
	Rtext_editor_set_text(g.source, text);
	/* the text stays on what is being changed */
	if (line > 0) {
		Rtext_editor_set_cursor(g.source, line - 1, 0);
		Rtext_editor_scroll_to_line(g.source, line - 1);
	}
	g.syncing--;
	g.broken = R_FALSE;
	g.modified = R_TRUE;
	history_push(text, R_FALSE);
	rebuild_all(keep_inspector);
	update_title();
}

/* ---------------------------------------------------------------------------
 *  Inspector
 * ------------------------------------------------------------------------- */

static row_t* add_row_record(const char* name, RXmlValueType type, const char* choices) {
	row_t* row;
	if (g.row_count == g.row_cap) {
		/* (the editors keep pointers to their rows: the array never moves while they live) */
		return NULL;
	}
	row = &g.rows[g.row_count++];
	snprintf(row->name, sizeof(row->name), "%s", name);
	row->type = type;
	row->choices = choices;
	row->swatch = R_INVALID_ID;
	return row;
}

/* An attribute written on the element shown and, when several are selected, on the others of the same tag. */
static void set_on_selection(const char* name, const char* value) {
	int i;
	if (strcmp(name, "#text") == 0) Rxml_node_set_text(g.selected, value ? value : "");
	else Rxml_node_set_attribute(g.selected, name, value && value[0] ? value : NULL);
	for (i = 0; i < g.extra_count; i++) {
		Rid other = g.extra[i];
		if (!Rxml_node_is_valid(other) || Rxml_node_is_comment(other) != Rxml_node_is_comment(g.selected)) continue;
		/* (what a tag knows, another may not: the attributes of its tag go to the same tags; those of every widget to all) */
		if (!same_text(Rxml_node_get_tag(other), Rxml_node_get_tag(g.selected)) && strcmp(name, "#text") != 0) {
			RXmlAttributeInfo info;
			int k, count = Rxml_get_attribute_count(Rxml_node_get_tag(other)), known = R_FALSE;
			for (k = 0; k < count && !known; k++) known = Rxml_get_attribute_info(Rxml_node_get_tag(other), k, &info) && same_text(info.name, name);
			if (!known) continue;
		}
		if (strcmp(name, "#text") == 0) Rxml_node_set_text(other, value ? value : "");
		else Rxml_node_set_attribute(other, name, value && value[0] ? value : NULL);
	}
}

static void apply_value(row_t* row, const char* value) {
	prune_selection();
	if (g.selected == R_INVALID_ID || !Rxml_node_is_valid(g.selected)) return;
	set_on_selection(row->name, value);
	if (row->swatch != R_INVALID_ID) Rwidget_set_attribute(row->swatch, "Background", value && value[0] ? value : "transparent");
	document_changed(R_TRUE);
}

static void on_row_text(Rid input, const char* text, void* user_data) {
	if (g.syncing) return;
	apply_value((row_t*)user_data, text);
	/* "@": the keys of the tables, under the input */
	strings_key_picker(input, text, on_row_text, user_data);
}

static void on_row_choice(Rid combo, int index, void* user_data) {
	if (!g.syncing) apply_value((row_t*)user_data, index <= 0 ? NULL : Rcombo_box_get_item_text(combo, index));
}

/* One line of the inspector: the name of an attribute and what edits its value. */
static void add_row(Rid box, const char* label, const char* name, RXmlValueType type, const char* choices, const char* value, const char* tip) {
	row_t* row = add_row_record(name, type, choices);
	Rid line, caption, editor;
	if (!row) return;
	line = Rwidget_create(R_WIDGET_HBOX);
	Rpanel_set_spacing(line, 6.0f);
	caption = Rlabel_create(label);
	Rwidget_set_size(caption, 104.0f, -1.0f);
	Rslot_set_valign(caption, R_VALIGN_CENTER);
	if (tip && tip[0]) Rwidget_set_tooltip(caption, tip);
	/* what is set stands out from what is left to its default */
	if (!value) Rwidget_set_opacity(caption, 0.6f);
	Rwidget_add_child(line, caption);

	if (type == R_XML_BOOL || type == R_XML_CHOICE) {
		const char* list = type == R_XML_BOOL ? "true|false" : choices;
		int selected = 0, index = 1, known = !value;
		editor = Rwidget_create(R_WIDGET_COMBO_BOX);
		Rcombo_box_add_item(editor, "(not set)");
		while (*list) {
			char item[96];
			size_t length = strcspn(list, "|");
			snprintf(item, sizeof(item), "%.*s", (int)length, list);
			Rcombo_box_add_item(editor, item);
			if (value && same_text(item, value)) {
				selected = index;
				known = R_TRUE;
			}
			index++;
			list += length;
			if (*list == '|') list++;
		}
		/* a value that is none of the choices (an expression of a declared widget) is kept as it is */
		if (!known) {
			Rcombo_box_add_item(editor, value);
			selected = index;
		}
		Rcombo_box_set_selected(editor, selected);
		Rcombo_box_set_changed_callback(editor, on_row_choice, row);
	} else {
		editor = Rwidget_create(R_WIDGET_TEXT_INPUT);
		Rtext_input_set_text(editor, value ? value : "");
		Rtext_input_set_placeholder(editor, type == R_XML_NUMBER ? "number" : type == R_XML_COLOR ? "#RRGGBB" : type == R_XML_NUMBERS ? "1,2"
			: type == R_XML_ACTION ? "action.name" : type == R_XML_FILE ? "path" : type == R_XML_LIST ? "One|Two" : "");
		Rtext_input_set_changed_callback(editor, on_row_text, row);
	}
	if (tip && tip[0]) Rwidget_set_tooltip(editor, tip);
	Rslot_set_fill(editor, 1.0f);
	Rwidget_add_child(line, editor);
	if (type == R_XML_COLOR) {
		row->swatch = Rwidget_create(R_WIDGET_BORDER);
		Rwidget_set_size(row->swatch, 22.0f, 22.0f);
		Rslot_set_valign(row->swatch, R_VALIGN_CENTER);
		Rwidget_set_attribute(row->swatch, "CornerRadius", "6");
		Rwidget_set_attribute(row->swatch, "BorderWidth", "1");
		Rwidget_set_attribute(row->swatch, "Background", value && value[0] ? value : "transparent");
		Rwidget_add_child(line, row->swatch);
	}
	Rwidget_add_child(box, line);
}

static Rid add_group(const char* title, int expanded) {
	Rid expander = Rwidget_create(R_WIDGET_EXPANDER);
	Rid box = Rwidget_create(R_WIDGET_VBOX);
	Rexpander_set_title(expander, title);
	Rexpander_set_expanded(expander, expanded);
	Rpanel_set_spacing(box, 4.0f);
	Rslot_set_padding(box, 4.0f, 4.0f, 0.0f, 6.0f);
	Rwidget_add_child(expander, box);
	Rwidget_add_child(g.inspector, expander);
	return box;
}

static int listed(const char* list, const char* name) {
	while (*list) {
		size_t length = strcspn(list, "|");
		if (strlen(name) == length) {
			char item[96];
			snprintf(item, sizeof(item), "%.*s", (int)length, list);
			if (same_text(item, name)) return 1;
		}
		list += length;
		if (*list == '|') list++;
	}
	return 0;
}

static void rebuild_inspector_of_one(void);

/* The Properties panel: the element shown; with others selected, a change goes to them too. */
static void rebuild_inspector(void) {
	int count = selection_count();
	rebuild_inspector_of_one();
	if (count > 1) {
		char title[120], about[300];
		snprintf(title, sizeof(title), "%s  and %d more", Rxml_node_is_comment(g.selected) ? "Comment" : Rxml_node_get_tag(g.selected), count - 1);
		snprintf(about, sizeof(about), "%d elements selected. The values are those of the one clicked last; "
			"what is changed here is written on all of them (an attribute of a tag, on those of that tag). Escape selects nothing.", count);
		Rlabel_set_text(Rwidget_find(g.window, "inspector.title"), title);
		Rlabel_set_text(Rwidget_find(g.window, "inspector.about"), about);
	}
}

static void rebuild_inspector_of_one(void) {
	Rid node = g.selected;
	Rid title = Rwidget_find(g.window, "inspector.title");
	Rid about = Rwidget_find(g.window, "inspector.about");
	Rid icon = Rwidget_find(g.window, "inspector.icon");
	Rid box = R_INVALID_ID;
	const char* tag;
	const char* group = "";
	char known[4096];
	int special, count, i, known_length = 0;

	g.syncing++;
	Rwidget_clear_children(g.inspector);
	g.row_count = 0;
	known[0] = '\0';
	if (node == R_INVALID_ID || !Rxml_node_is_valid(node)) {
		Rwidget_set_visibility(icon, R_COLLAPSED);
		Rlabel_set_text(title, "Nothing selected");
		Rlabel_set_text(about, g.has_document ? "Click an element in the preview or in the Tree to see and change its attributes; Ctrl + click selects several, Shift + click the ones between. Escape selects nothing." : "");
		g.syncing--;
		return;
	}
	Rwidget_set_visibility(icon, R_VISIBLE);
	icons_set_image(icon, Rxml_node_is_comment(node) ? "#comment" : Rxml_node_get_tag(node));
	if (Rxml_node_is_comment(node)) {
		Rlabel_set_text(title, "Comment");
		Rlabel_set_text(about, "Kept in the XML, ignored when the interface is built.");
		add_row(add_group("Comment", R_TRUE), "Text", "#text", R_XML_TEXT, NULL, Rxml_node_get_text(node), NULL);
		g.syncing--;
		return;
	}
	tag = Rxml_node_get_tag(node);
	special = special_tag(tag);
	Rlabel_set_text(title, tag);
	Rlabel_set_text(about, special >= 0 ? SPECIAL_TAGS[special].about
		: Rxml_is_widget_registered(tag) ? "A tag declared with <Widget>: its own attributes are its parameters." : Rxml_get_tag_description(tag));

	if (special >= 0) {
		const char* list = SPECIAL_TAGS[special].attributes;
		box = add_group(tag, R_TRUE);
		while (*list) {
			char name[72];
			size_t length = strcspn(list, "|");
			snprintf(name, sizeof(name), "%.*s", (int)length, list);
			add_row(box, name, name, R_XML_TEXT, NULL, Rxml_node_get_attribute(node, name), NULL);
			known_length += snprintf(known + known_length, sizeof(known) - (size_t)known_length, "%s|", name);
			list += length;
			if (*list == '|') list++;
		}
	} else {
		count = Rxml_get_attribute_count(tag);
		for (i = 0; i < count; i++) {
			RXmlAttributeInfo info;
			const char* label;
			if (!Rxml_get_attribute_info(tag, i, &info)) continue;
			if (strcmp(info.group, group) != 0) {
				int own = same_text(info.group, tag) || (strcmp(info.group, "Widget") != 0 && strcmp(info.group, "Slot") != 0
					&& strcmp(info.group, "Colors") != 0 && strcmp(info.group, "Metrics") != 0);
				group = info.group;
				box = add_group(strcmp(group, "Slot") == 0 ? "Place in the parent" : strcmp(group, "Widget") == 0 ? "Widget" : group,
					own || strcmp(group, "Slot") == 0 || strcmp(group, "Widget") == 0);
			}
			/* "Slot.Fill" and "Color.Text" read better without what their group already says */
			label = strchr(info.name, '.') ? strchr(info.name, '.') + 1 : info.name;
			add_row(box, label, info.name, info.type, info.choices, Rxml_node_get_attribute(node, info.name), info.description);
			if (known_length < (int)sizeof(known) - 80) known_length += snprintf(known + known_length, sizeof(known) - (size_t)known_length, "%s|", info.name);
		}
	}
	/* the text between the tags, where it means something */
	if (Rxml_node_get_text(node)[0] || Rxml_node_get_child_count(node) == 0) {
		if (Rxml_node_get_text(node)[0]) add_row(add_group("Text inside", R_TRUE), "Text", "#text", R_XML_TEXT, NULL, Rxml_node_get_text(node), NULL);
	}
	/* what is written on the element and is in no list: other names of an attribute, parameters, mistakes */
	box = R_INVALID_ID;
	count = Rxml_node_get_attribute_count(node);
	for (i = 0; i < count; i++) {
		const char* name = Rxml_node_get_attribute_name(node, i);
		if (listed(known, name)) continue;
		if (box == R_INVALID_ID) box = add_group(special >= 0 && same_text(tag, "Widget") ? "Parameters" : "Other attributes", R_TRUE);
		add_row(box, name, name, R_XML_TEXT, NULL, Rxml_node_get_attribute_value(node, i), NULL);
	}
	g.syncing--;
}

/* ---------------------------------------------------------------------------
 *  Selection
 * ------------------------------------------------------------------------- */

static void update_path_label(void) {
	char text[400], caption[96];
	int length = 0, depth = 0, i;
	Rid chain[32], walk;
	float x, y, w, h;
	if (g.selected == R_INVALID_ID || !Rxml_node_is_valid(g.selected)) {
		Rlabel_set_text(g.path_label, "");
		return;
	}
	for (walk = g.selected; walk != R_INVALID_ID && depth < 32; walk = Rxml_node_get_parent(walk)) chain[depth++] = walk;
	text[0] = '\0';
	for (i = depth - 1; i >= 0 && length < (int)sizeof(text) - 80; i--) {
		length += snprintf(text + length, sizeof(text) - (size_t)length, "%s%s", Rxml_node_is_comment(chain[i]) ? "comment" : Rxml_node_get_tag(chain[i]),
			i > 0 ? "  >  " : "");
	}
	walk = Rxml_node_get_widget(g.selected);
	if (walk != R_INVALID_ID && Rwidget_is_valid(walk)) {
		Rwidget_get_rect(walk, &x, &y, &w, &h);
		snprintf(caption, sizeof(caption), "     %.0f x %.0f", (double)w, (double)h);
		if (length < (int)sizeof(text) - 40) snprintf(text + length, sizeof(text) - (size_t)length, "%s", caption);
	}
	Rlabel_set_text(g.path_label, text);
}

/*
 * The selection is one element (g.selected, the one clicked last: the
 * Properties panel shows it) and the others selected with it (g.extra), with
 * Ctrl + click (one more, or one less) or Shift + click (the elements between,
 * when they are in the same panel), in the preview as in the Tree.
 */

/* What is no longer in the document leaves the selection; the one clicked last is always there when anything is. */
static void prune_selection(void) {
	int i, k = 0;
	if (g.selected != R_INVALID_ID && !Rxml_node_is_valid(g.selected)) g.selected = R_INVALID_ID;
	for (i = 0; i < g.extra_count; i++) {
		Rid node = g.extra[i];
		int seen = node == g.selected, j;
		for (j = 0; j < k && !seen; j++) seen = g.extra[j] == node;
		if (!seen && node != R_INVALID_ID && Rxml_node_is_valid(node)) g.extra[k++] = node;
	}
	g.extra_count = k;
	if (g.selected == R_INVALID_ID && g.extra_count > 0) g.selected = g.extra[--g.extra_count];
}

static int is_selected(Rid node) {
	int i;
	if (node == R_INVALID_ID) return R_FALSE;
	if (node == g.selected) return R_TRUE;
	for (i = 0; i < g.extra_count; i++) if (g.extra[i] == node) return R_TRUE;
	return R_FALSE;
}

static int selection_count(void) {
	prune_selection();
	return (g.selected != R_INVALID_ID ? 1 : 0) + g.extra_count;
}

/* Before b in the document (a above b, or an element written before it). */
static int node_before(Rid a, Rid b) {
	int pa[32], pb[32], da = node_path(a, pa, 32), db = node_path(b, pb, 32), i;
	for (i = 0; i < da && i < db; i++) if (pa[i] != pb[i]) return pa[i] < pb[i];
	return da < db;
}

/*
 * The elements selected, in the order of the document, without those that
 * are inside another one selected (they go with it), and without the root of
 * the document when `movable`.
 */
static int selection_roots(Rid* out, int max, int movable) {
	Rid all[MAX_EXTRA + 1];
	int count = 0, n = 0, i, j;
	prune_selection();
	if (g.selected != R_INVALID_ID) all[count++] = g.selected;
	for (i = 0; i < g.extra_count; i++) all[count++] = g.extra[i];
	for (i = 0; i < count; i++) {
		Rid up;
		int inside = R_FALSE;
		if (movable && Rxml_node_get_parent(all[i]) == R_INVALID_ID) continue;
		for (up = Rxml_node_get_parent(all[i]); up != R_INVALID_ID && !inside; up = Rxml_node_get_parent(up)) inside = is_selected(up);
		if (!inside && n < max) out[n++] = all[i];
	}
	/* (a few elements: an insertion sort) */
	for (i = 1; i < n; i++) {
		Rid node = out[i];
		for (j = i; j > 0 && node_before(node, out[j - 1]); j--) out[j] = out[j - 1];
		out[j] = node;
	}
	return n;
}

/* The Tree shows the selection: the one clicked last, then the others. */
static void sync_tree_selection(void) {
	int i;
	g.syncing++;
	Rtree_view_set_selected_item(g.tree, g.selected != R_INVALID_ID ? tree_item_of(R_INVALID_ID, g.selected) : R_INVALID_ID);
	for (i = 0; i < g.extra_count; i++) {
		Rid item = tree_item_of(R_INVALID_ID, g.extra[i]);
		if (item != R_INVALID_ID) Rtree_item_set_selected(item, R_TRUE);
	}
	g.syncing--;
}

/* The selection changed: the panels that show it follow. */
static void selection_shown(int reveal) {
	Rid node = g.selected;
	int line;
	rebuild_inspector();
	update_path_label();
	Rwidget_invalidate(g.outline);
	/* the text shows where the element is written */
	line = node != R_INVALID_ID ? Rxml_node_get_line(node) : 0;
	if (reveal && line > 0 && !g.broken && !Rwidget_is_focused(g.source)) {
		Rtext_editor_set_cursor(g.source, line - 1, 0);
		Rtext_editor_scroll_to_line(g.source, line - 1);
	}
}

/* One element selected (R_INVALID_ID: nothing). */
static void select_node(Rid node, int reveal) {
	if (node != R_INVALID_ID && !Rxml_node_is_valid(node)) node = R_INVALID_ID;
	g.selected = node;
	g.extra_count = 0;
	g.press_reduce = R_INVALID_ID;
	sync_tree_selection();
	selection_shown(reveal);
}

/* Several elements selected; the last one is the one the Properties panel shows. */
static void select_nodes(const Rid* nodes, int count) {
	int i;
	g.selected = R_INVALID_ID;
	g.extra_count = 0;
	for (i = 0; i < count; i++) {
		if (nodes[i] == R_INVALID_ID || !Rxml_node_is_valid(nodes[i])) continue;
		if (g.selected != R_INVALID_ID && g.extra_count < MAX_EXTRA) g.extra[g.extra_count++] = g.selected;
		g.selected = nodes[i];
	}
	prune_selection();
	sync_tree_selection();
	selection_shown(R_TRUE);
}

/*
 * A click with Ctrl (Cmd): the element joins the selection, or leaves it.
 * With Shift: the elements from the one clicked last to this one, when they
 * are in the same panel; else this one joins.
 */
static void select_more(Rid node, int range) {
	int i;
	if (node == R_INVALID_ID || !Rxml_node_is_valid(node)) return;
	prune_selection();
	if (g.selected == R_INVALID_ID) {
		select_node(node, R_TRUE);
		return;
	}
	if (range && Rxml_node_get_parent(node) != R_INVALID_ID && Rxml_node_get_parent(node) == Rxml_node_get_parent(g.selected)) {
		Rid parent = Rxml_node_get_parent(node);
		int from = Rxml_node_get_index(g.selected), to = Rxml_node_get_index(node), step = from < to ? 1 : -1;
		for (i = from; i != to + step; i += step) {
			Rid child = Rxml_node_get_child(parent, i);
			if (child == R_INVALID_ID || Rxml_node_is_comment(child) || is_selected(child)) continue;
			if (g.extra_count < MAX_EXTRA) g.extra[g.extra_count++] = child;
		}
		/* the one clicked is the one shown */
		for (i = 0; i < g.extra_count; i++) {
			if (g.extra[i] == node) {
				g.extra[i] = g.selected;
				g.selected = node;
				break;
			}
		}
	} else if (is_selected(node) && !range) {
		/* Ctrl + click on a selected element: it leaves the selection */
		if (node == g.selected) {
			g.selected = g.extra_count > 0 ? g.extra[--g.extra_count] : R_INVALID_ID;
		} else {
			for (i = 0; i < g.extra_count; i++) {
				if (g.extra[i] == node) {
					g.extra[i] = g.extra[--g.extra_count];
					break;
				}
			}
		}
	} else if (!is_selected(node)) {
		if (g.extra_count < MAX_EXTRA) g.extra[g.extra_count++] = g.selected;
		g.selected = node;
	} else if (node != g.selected) {
		/* Shift + click on one of them: it becomes the one shown */
		for (i = 0; i < g.extra_count; i++) if (g.extra[i] == node) g.extra[i] = g.selected;
		g.selected = node;
	}
	prune_selection();
	sync_tree_selection();
	selection_shown(R_TRUE);
	if (selection_count() > 1) set_status("%d elements selected: Ctrl + click adds or removes one, Shift + click the ones between.", selection_count());
}

/* Everything that shows the document, from the document. */
static void rebuild_all(int keep_inspector) {
	rebuild_preview();
	rebuild_tree();
	if (!keep_inspector) rebuild_inspector();
	update_path_label();
}

/* ---------------------------------------------------------------------------
 *  Text -> document
 * ------------------------------------------------------------------------- */

/*
 * Reads a text as the document. On a mistake the document stays what it was
 * and the mistake is shown; R_FALSE is returned.
 */
static int load_text(const char* text, int record) {
	RXmlError error;
	Rid document;
	int path[32], depth = -1, i, kept = 0;
	/* (the others selected, by their places, the same way) */
	static int extra_paths[MAX_EXTRA][32], extra_depths[MAX_EXTRA];

	document = Rxml_document_parse(text, strlen(text), &error);
	if (document == R_INVALID_ID) {
		char message[240];
		g.broken = R_TRUE;
		g.problem_count = 0;
		snprintf(message, sizeof(message), "XML line %d, column %d: %s", error.line, error.column, error.message);
		add_problem(message, error.line, R_TRUE);
		show_problems();
		set_status("%s", message);
		return R_FALSE;
	}
	if (g.document != R_INVALID_ID) {
		if (g.selected != R_INVALID_ID && Rxml_node_is_valid(g.selected)) depth = node_path(g.selected, path, 32);
		for (i = 0; i < g.extra_count; i++) {
			if (!Rxml_node_is_valid(g.extra[i])) continue;
			extra_depths[kept] = node_path(g.extra[i], extra_paths[kept], 32);
			if (extra_depths[kept] >= 0) kept++;
		}
		Rxml_document_destroy(g.document);
	}
	g.document = document;
	g.broken = R_FALSE;
	/* the selection stays where it was; nothing selected stays so (an interface opens with nothing selected) */
	g.selected = depth >= 0 ? node_at_path(path, depth) : R_INVALID_ID;
	g.extra_count = 0;
	for (i = 0; i < kept && g.selected != R_INVALID_ID; i++) {
		Rid node = node_at_path(extra_paths[i], extra_depths[i]);
		if (node != R_INVALID_ID) g.extra[g.extra_count++] = node;
	}
	g.press_reduce = R_INVALID_ID;
	g.hovered = R_INVALID_ID;
	if (record) history_push(text, record == 2);
	rebuild_all(R_FALSE);
	if (g.problem_count == 0) set_status("Ready.");
	else set_status("%d problem%s in the document: see the Problems tab.", g.problem_count, g.problem_count > 1 ? "s" : "");
	return R_TRUE;
}

/* Replaces everything: a new file, a file opened, a step of history. */
static int set_document_text(const char* text, int record) {
	g.syncing++;
	Rtext_editor_set_text(g.source, text);
	g.syncing--;
	return load_text(text, record);
}

static void on_parse_timer(Rid timer, void* user_data) {
	crash_report_note("the XML typed is read");
	(void)timer;
	(void)user_data;
	g.parse_timer = R_INVALID_ID;
	if (load_text(Rtext_editor_get_text(g.source), 2)) {
		g.modified = R_TRUE;
		update_title();
	}
}

static void on_source_changed(Rid editor, void* user_data) {
	(void)editor;
	(void)user_data;
	if (g.syncing) return;
	/* read once the typing pauses: half-written tags are not worth reporting */
	if (g.parse_timer != R_INVALID_ID && Rtimer_is_valid(g.parse_timer)) Rtimer_destroy(g.parse_timer);
	g.parse_timer = Rtimer_create(0.30, R_FALSE, on_parse_timer, NULL);
	g.modified = R_TRUE;
	update_title();
}

/* The text of the editor was not read yet (the user is typing): read it now, before acting on the document. */
static int flush_source(void) {
	if (g.parse_timer != R_INVALID_ID && Rtimer_is_valid(g.parse_timer)) {
		Rtimer_destroy(g.parse_timer);
		g.parse_timer = R_INVALID_ID;
		load_text(Rtext_editor_get_text(g.source), 2);
	}
	if (g.broken) {
		set_status("The XML has a mistake: fix it first (see the Problems tab).");
		return R_FALSE;
	}
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  Palette
 * ------------------------------------------------------------------------- */

/* A tag in the palette: an item, with its picture, in the group of its family. */
static void palette_add_tag(int group, const char* tag) {
	Rid item;
	if (g.palette_groups[group] == R_INVALID_ID) {
		char title[64];
		snprintf(title, sizeof(title), "%s", icons_group_name(group));
		g.palette_groups[group] = Rtree_view_add_item(g.palette, R_INVALID_ID, title);
		Rtree_item_set_expanded(g.palette_groups[group], !g.palette_folded[group]);
		Rtree_item_set_user_handle(g.palette_groups[group], NULL);
	}
	item = Rtree_view_add_item(g.palette, g.palette_groups[group], tag);
	icons_set_tree_item(item, tag);
	g.palette_tags[g.palette_count] = copy_text(tag);
	g.palette_items[g.palette_count] = item;
	g.palette_count++;
	/* (the user handle is the place in palette_tags, plus one: a group has none) */
	Rtree_item_set_user_handle(item, (void*)(size_t)g.palette_count);
}

/* A tag of Raven is listed by a group of icons.c. */
static int palette_listed(const char* tag) {
	int group, i;
	for (group = 1; group < icons_group_count() - 1; group++) {
		for (i = 0; i < icons_group_tag_count(group); i++) if (same_text(icons_group_tag_at(group, i), tag)) return R_TRUE;
	}
	return R_FALSE;
}

/*
 * The palette, by groups: the tags the folder declares first (what this
 * interface is made of), then those of Raven by family, then the rest. What
 * is typed in the search keeps the tags whose name holds it.
 */
static void palette_fill(void) {
	const char* filter = Rtext_input_get_text(g.palette_filter);
	int i, group, groups = icons_group_count();
	int total = Rxml_get_tag_count() + Rxml_get_registered_widget_count() + 64;

	/* the groups the user folded stay folded */
	for (group = 0; group < groups && group < 8; group++) {
		if (g.palette_groups[group] != R_INVALID_ID && Rtree_item_is_valid(g.palette_groups[group]))
			g.palette_folded[group] = !Rtree_item_is_expanded(g.palette_groups[group]);
		g.palette_groups[group] = R_INVALID_ID;
	}
	for (i = 0; i < g.palette_count; i++) free(g.palette_tags[i]);
	free(g.palette_tags);
	free(g.palette_items);
	g.palette_tags = (char**)calloc((size_t)total, sizeof(char*));
	g.palette_items = (Rid*)calloc((size_t)total, sizeof(Rid));
	g.palette_count = 0;
	g.syncing++;
	Rtree_view_clear_items(g.palette);
	/* the tags of the folder */
	for (i = 0; i < Rxml_get_registered_widget_count() && g.palette_count < total; i++) {
		const char* name = Rxml_get_registered_widget_name(i);
		if (!name || !contains_text(name, filter) || same_text(name, "ToolButton")) continue;
		palette_add_tag(0, name);
	}
	/* the tags of Raven, by family */
	for (group = 1; group < groups - 1; group++) {
		for (i = 0; i < icons_group_tag_count(group) && g.palette_count < total; i++) {
			const char* tag = icons_group_tag_at(group, i);
			/* (Track, Node, Link... are only written inside their widget: known_tag leaves them out) */
			if (!contains_text(tag, filter) || !known_tag(tag)) continue;
			palette_add_tag(group, tag);
		}
	}
	/* those of Raven no group lists (a tag newer than the designer) */
	for (i = 1; i < Rxml_get_tag_count() && g.palette_count < total; i++) {
		const char* name = Rxml_get_tag_name(i);
		if (palette_listed(name) || !contains_text(name, filter)) continue;
		palette_add_tag(groups - 1, name);
	}
	g.syncing--;
}

/* The place in palette_tags of the tag chosen in the palette, or -1 (nothing, or a group). */
static int palette_selected(void) {
	Rid item = Rtree_view_get_selected_item(g.palette);
	int index;
	if (item == R_INVALID_ID) return -1;
	index = (int)(size_t)Rtree_item_get_user_handle(item) - 1;
	return index >= 0 && index < g.palette_count ? index : -1;
}

/* Adds a piece of XML inside the selection, or after it when it cannot contain anything. */
static void insert_xml(const char* xml) {
	Rid target = g.selected, added;
	int index = -1;
	if (!g.has_document) {
		set_status("Open an interface first: double click one in Files, or File > New interface.");
		return;
	}
	if (!flush_source()) return;
	if (target == R_INVALID_ID || !Rxml_node_is_valid(target)) target = Rxml_document_get_root(g.document);
	/* in a library: inside a declaration (the one selected, else the first) */
	if (same_text(Rxml_node_get_tag(target), "Widgets")) {
		int i, count = Rxml_node_get_child_count(target);
		Rid first = R_INVALID_ID;
		for (i = 0; i < count && first == R_INVALID_ID; i++) {
			Rid child = Rxml_node_get_child(target, i);
			if (!Rxml_node_is_comment(child) && same_text(Rxml_node_get_tag(child), "Widget")) first = child;
		}
		if (first == R_INVALID_ID) {
			set_status("Add a <Widget Name=\"...\"> to the library first (in the XML).");
			return;
		}
		target = first;
	}
	if (Rxml_node_is_comment(target) || (special_tag(Rxml_node_get_tag(target)) < 0 && Rxml_get_tag_children(Rxml_node_get_tag(target)) == 0)) {
		Rid parent = Rxml_node_get_parent(target);
		if (parent == R_INVALID_ID) {
			set_status("<%s> cannot contain anything.", Rxml_node_get_tag(target));
			return;
		}
		index = Rxml_node_get_index(target) + 1;
		target = parent;
	}
	g.quiet++;
	added = Rxml_node_add_xml(target, xml, strlen(xml), index);
	g.quiet--;
	if (added == R_INVALID_ID) {
		/* several elements one after the other (several copied together): each of them, in their order */
		ds_buffer_t text = { 0 };
		Rid holder, made[MAX_EXTRA + 1];
		int count = 0, at = index;
		ds_buffer_printf(&text, "<VBox>%s</VBox>", xml);
		holder = Rxml_node_add_xml(target, text.data, text.size, index);
		ds_buffer_free(&text);
		if (holder == R_INVALID_ID || Rxml_node_get_child_count(holder) == 0) {
			if (holder != R_INVALID_ID) Rxml_node_remove(holder);
			set_status("This is not XML that can be added.");
			return;
		}
		if (at < 0) at = Rxml_node_get_index(holder);
		while (Rxml_node_get_child_count(holder) > 0) {
			Rid child = Rxml_node_get_child(holder, 0);
			Rxml_node_move(child, target, at < 0 ? -1 : at);
			if (at >= 0) at++;
			if (count < MAX_EXTRA) made[count++] = child;
		}
		Rxml_node_remove(holder);
		g.selected = made[count - 1];
		g.extra_count = 0;
		document_changed(R_FALSE);
		select_nodes(made, count);
		set_status("Added %d elements to <%s>.", count, Rxml_node_get_tag(target));
		return;
	}
	g.selected = added;
	document_changed(R_FALSE);
	select_node(added, R_TRUE);
	set_status("Added <%s> to <%s>.", Rxml_node_get_tag(added), Rxml_node_get_tag(target));
}

static void action_palette_add(Rid sender, const char* action, void* user_data) {
	int index = palette_selected(), i;
	const char* tag;
	char xml[160];
	crash_report_note("action %s", action);
	(void)sender;
	(void)action;
	(void)user_data;
	if (index < 0 || index >= g.palette_count) {
		set_status("Choose a widget in the palette first.");
		return;
	}
	tag = g.palette_tags[index];
	for (i = 0; i < (int)(sizeof(TEMPLATES) / sizeof(TEMPLATES[0])); i++) {
		if (same_text(TEMPLATES[i].tag, tag)) {
			insert_xml(TEMPLATES[i].xml);
			return;
		}
	}
	snprintf(xml, sizeof(xml), "<%s/>", tag);
	insert_xml(xml);
}

/* A double click (or Enter) on a tag adds it; on a group, the tree folds or unfolds it. */
static void on_palette_activated(Rid tree, Rid item, void* user_data) {
	(void)tree;
	(void)user_data;
	if (g.syncing || item == R_INVALID_ID || (size_t)Rtree_item_get_user_handle(item) == 0) return;
	Raction_fire("palette.add", R_INVALID_ID);
}

static void action_palette_filter(Rid sender, const char* action, void* user_data) {
	crash_report_note("action %s", action);
	(void)sender;
	(void)action;
	(void)user_data;
	if (!g.syncing) palette_fill();
}

/* ---------------------------------------------------------------------------
 *  Drag and drop in the preview
 * ------------------------------------------------------------------------- */

static int accepts_children(Rid node) {
	const char* tag;
	if (node == R_INVALID_ID || Rxml_node_is_comment(node)) return R_FALSE;
	tag = Rxml_node_get_tag(node);
	if (special_tag(tag) >= 0) {
		return same_text(tag, "Widget") || same_text(tag, "Widgets") || same_text(tag, "Repeat") || same_text(tag, "ForEach") ||
			same_text(tag, "If") || same_text(tag, "Node") || same_text(tag, "Track") || same_text(tag, "Item");
	}
	return Rxml_get_tag_children(tag) != 0;
}

static const char* template_of(const char* tag);

/* A widget of the palette dropped at (x, y) of the window. */
/* A tag the palette offers: one of Raven, one the folder declares, or Item and Shortcut. */
static int known_tag(const char* tag) {
	int i;
	if (same_text(tag, "Item") || same_text(tag, "Shortcut") || Rxml_is_widget_registered(tag)) return R_TRUE;
	for (i = 1; i < Rxml_get_tag_count(); i++) if (same_text(Rxml_get_tag_name(i), tag)) return R_TRUE;
	return R_FALSE;
}

static int drop_widget(const char* tag, float x, float y) {
	const char* xml = template_of(tag);
	char text[160];
	Rid parent, added;
	int index;
	crash_report_note("drop <%s> at %.0f,%.0f", tag, x, y);
	if (!g.has_document || (g.tab != TAB_WINDOW && g.tab != TAB_WIDGET) || !known_tag(tag) || !flush_source()) return R_FALSE;
	if (!drop_place(x, y, &parent, &index)) return R_FALSE;
	if (!drop_allowed(parent)) {
		set_status("In a widget library, a widget goes inside a <Widget>.");
		return R_FALSE;
	}
	if (!xml) {
		snprintf(text, sizeof(text), "<%s/>", tag);
		xml = text;
	}
	added = Rxml_node_add_xml(parent, xml, strlen(xml), index);
	if (added == R_INVALID_ID) {
		set_status("<%s> cannot go there.", tag);
		return R_FALSE;
	}
	g.selected = added;
	document_changed(R_FALSE);
	select_node(added, R_TRUE);
	set_status("Dropped <%s> into <%s>.", tag, Rxml_node_get_tag(parent));
	return R_TRUE;
}

/* A drag of an element that is one of several selected carries them all: none may receive them. */
static int inside_dragged(Rid parent) {
	Rid roots[MAX_EXTRA + 1];
	int i, count;
	if (g.drag_node == R_INVALID_ID || !is_selected(g.drag_node)) return inside_of(parent, g.drag_node);
	count = selection_roots(roots, MAX_EXTRA + 1, R_TRUE);
	for (i = 0; i < count; i++) if (inside_of(parent, roots[i])) return R_TRUE;
	return inside_of(parent, g.drag_node);
}

/* Several elements selected, moved together to (x, y), in the order of the document. */
static int drop_nodes(Rid node, float x, float y) {
	Rid roots[MAX_EXTRA + 1], parent;
	int count, index, i;
	g.drag_node = node;
	if (!drop_place(x, y, &parent, &index) || inside_dragged(parent)) {
		g.drag_node = R_INVALID_ID;
		set_status("Elements cannot go inside one of themselves.");
		return R_FALSE;
	}
	g.drag_node = R_INVALID_ID;
	if (!drop_allowed(parent)) {
		set_status("In a widget library, a widget goes inside a <Widget>.");
		return R_FALSE;
	}
	count = selection_roots(roots, MAX_EXTRA + 1, R_TRUE);
	if (count == 0) return R_FALSE;
	for (i = 0; i < count; i++) {
		/* (in the same parent, the indices after it move down by one once it is taken out) */
		if (Rxml_node_get_parent(roots[i]) == parent && Rxml_node_get_index(roots[i]) < index) index--;
		Rxml_node_move(roots[i], parent, index);
		index++;
	}
	document_changed(R_FALSE);
	/* (the one dragged stays the one shown: last) */
	for (i = 0; i < count - 1; i++) {
		if (roots[i] == node) {
			roots[i] = roots[count - 1];
			roots[count - 1] = node;
		}
	}
	select_nodes(roots, count);
	set_status("Moved %d elements into <%s>.", count, Rxml_node_get_tag(parent));
	return R_TRUE;
}

/* An element of the preview moved to (x, y). */
static int drop_node(Rid node, float x, float y) {
	Rid parent;
	int index;
	crash_report_note("move an element to %.0f,%.0f", x, y);
	if (!g.has_document || (g.tab != TAB_WINDOW && g.tab != TAB_WIDGET) || node == R_INVALID_ID || !Rxml_node_is_valid(node) || !flush_source()) return R_FALSE;
	if (Rxml_node_get_parent(node) == R_INVALID_ID) {
		set_status("The root of the document stays where it is.");
		return R_FALSE;
	}
	if (is_selected(node) && selection_count() > 1) return drop_nodes(node, x, y);
	g.drag_node = node;
	if (!drop_place(x, y, &parent, &index) || inside_of(parent, node)) {
		g.drag_node = R_INVALID_ID;
		set_status("An element cannot go inside itself.");
		return R_FALSE;
	}
	g.drag_node = R_INVALID_ID;
	if (!drop_allowed(parent)) {
		set_status("In a widget library, a widget goes inside a <Widget>.");
		return R_FALSE;
	}
	/* (in the same parent, the indices after it move down by one once it is taken out) */
	if (Rxml_node_get_parent(node) == parent && Rxml_node_get_index(node) < index) index--;
	if (Rxml_node_get_parent(node) == parent && Rxml_node_get_index(node) == index) return R_FALSE;
	Rxml_node_move(node, parent, index);
	g.selected = node;
	document_changed(R_FALSE);
	select_node(node, R_TRUE);
	set_status("Moved <%s> into <%s>.", Rxml_node_get_tag(node), Rxml_node_get_tag(parent));
	return R_TRUE;
}

static int over_stage(float x, float y) {
	float ox, oy, ow, oh;
	Rwidget_get_rect(g.outline, &ox, &oy, &ow, &oh);
	return x >= ox && x < ox + ow && y >= oy && y < oy + oh;
}

static int is_inside_widget(Rid widget, Rid ancestor) {
	for (; widget != R_INVALID_ID; widget = Rwidget_get_parent(widget)) if (widget == ancestor) return R_TRUE;
	return R_FALSE;
}

/*
 * Around the frame: the room of the preview panel the window does not take
 * (the panels there let the pointer through: the press reaches the dock).
 * Not its scrollbars, not the frame itself.
 */
static int on_stage_margin(Rid target, float x, float y) {
	Rid host = Rwidget_get_visibility(g.stage_fit) == R_VISIBLE ? g.stage_fit : g.stage_scroll;
	float hx, hy, hw, hh, fx, fy, fw, fh, bar = 16.0f;
	if (target != g.dock && target != g.stage && target != g.stage_fit && target != g.stage_scroll) return R_FALSE;
	Rwidget_get_rect(host, &hx, &hy, &hw, &hh);
	Rwidget_get_rect(g.frame, &fx, &fy, &fw, &fh);
	if (x < hx || y < hy || x >= hx + hw - bar || y >= hy + hh - bar) return R_FALSE;
	return !(x >= fx && y >= fy && x < fx + fw && y < fy + fh);
}

static void drag_reset(void) {
	g.drag_armed = g.dragging = R_FALSE;
	g.drag_node = R_INVALID_ID;
	g.drop_parent = R_INVALID_ID;
	g.place_fixed = R_FALSE;
	Rtree_view_set_drop_highlight(g.tree, R_INVALID_ID, R_FALSE);
	Rwidget_invalidate(g.outline);
}

static int over_tree(float x, float y) {
	float tx, ty, tw, th;
	/* (a panel in a tab behind another keeps its old place: it is not there) */
	if (Rwidget_get_visibility(g.tree) != R_VISIBLE || !Rdock_area_is_panel_shown(g.dock, Rwidget_find(g.window, "panel.tree"))) return R_FALSE;
	Rwidget_get_rect(g.tree, &tx, &ty, &tw, &th);
	return tw > 0.0f && x >= tx && x < tx + tw && y >= ty && y < ty + th;
}

/* A panel that can take one more child (a Border that holds one already cannot). */
static int can_take_child(Rid node) {
	return accepts_children(node) && !(Rxml_get_tag_children(Rxml_node_get_tag(node)) == 1 && Rxml_node_get_child_count(node) > 0);
}

/*
 * Where something dropped on the Tree at (x, y) goes: near the top of a row,
 * before its element; near the bottom, after it; in the middle of a panel,
 * inside it, at the end. The item that shows the receiving panel is lit.
 */
static int tree_drop_place(float x, float y, Rid* parent, int* index) {
	float tx, ty, tw, th, lx, ly;
	Rid item, node, holder = R_INVALID_ID;
	int above, below;
	Rwidget_get_rect(g.tree, &tx, &ty, &tw, &th);
	lx = x - tx;
	ly = y - ty;
	item = Rtree_view_get_item_at(g.tree, lx, ly);
	*parent = R_INVALID_ID;
	if (item == R_INVALID_ID) {
		/* below the last row: at the end of the first panel of the document */
		node = Rxml_document_get_root(g.document);
		if (Rxml_node_get_child_count(node) == 1 && can_take_child(Rxml_node_get_child(node, 0))) node = Rxml_node_get_child(node, 0);
		if (!can_take_child(node)) return R_FALSE;
		*parent = node;
		*index = Rxml_node_get_child_count(node);
	} else {
		node = (Rid)(size_t)Rtree_item_get_user_handle(item);
		if (node == R_INVALID_ID || !Rxml_node_is_valid(node)) return R_FALSE;
		above = Rtree_view_get_item_at(g.tree, lx, ly - 6.0f) != item;
		below = Rtree_view_get_item_at(g.tree, lx, ly + 6.0f) != item;
		holder = Rxml_node_get_parent(node);
		if (holder == R_INVALID_ID || (can_take_child(node) && !above && !below)) {
			if (!can_take_child(node)) return R_FALSE;
			*parent = node;
			*index = Rxml_node_get_child_count(node);
		} else {
			*parent = holder;
			*index = Rxml_node_get_index(node) + (above ? 0 : 1);
		}
	}
	/* (what is dragged cannot receive itself) */
	if (!g.drag_from_palette && inside_dragged(*parent)) return R_FALSE;
	if (!drop_allowed(*parent)) return R_FALSE;
	return R_TRUE;
}

/*
 * Every event of the window passes here first: a press on the palette or on
 * an element of the preview arms a drag; past a few pixels it is one, and
 * the release drops it where the line shows.
 */
static int window_hook(Rid window, const REvent* event, void* user_data) {
	(void)window;
	(void)user_data;
	/* what the hand did, for the report of a crash */
	if (event->type == R_EVENT_MOUSE_DOWN) {
		const char* name = event->target != R_INVALID_ID ? Rwidget_get_name(event->target) : NULL;
		crash_report_note("tab %d: click %d at %.0f,%.0f on %s (type 0x%X)", g.tab, (int)event->button, event->x, event->y,
			name && name[0] ? name : "a widget with no name", event->target != R_INVALID_ID ? (unsigned)Rwidget_get_type(event->target) : 0u);
	} else if (event->type == R_EVENT_KEY_DOWN) {
		crash_report_note("tab %d: key 0x%X, modifiers 0x%X", g.tab, (unsigned)event->key, (unsigned)event->modifiers);
	}
	switch (event->type) {
		case R_EVENT_MOUSE_DOWN:
			/* a click around the window (on the stage, not on the frame) selects nothing */
			if (g.has_document && !g.interact && (g.tab == TAB_WINDOW || g.tab == TAB_WIDGET) && on_stage_margin(event->target, event->x, event->y)) {
				if (g.selected != R_INVALID_ID && !(event->modifiers & (R_MOD_CTRL | R_MOD_SUPER | R_MOD_SHIFT))) select_node(R_INVALID_ID, R_FALSE);
				return R_FALSE;
			}
			if (event->button != R_MOUSE_LEFT || (g.tab != TAB_WINDOW && g.tab != TAB_WIDGET) || !g.has_document) return R_FALSE;
			if (is_inside_widget(event->target, g.palette)) {
				g.drag_armed = R_TRUE;
				g.drag_from_palette = R_TRUE;
			} else if (is_inside_widget(event->target, g.tree) && !(event->modifiers & (R_MOD_CTRL | R_MOD_SUPER | R_MOD_SHIFT))) {
				/* an element of the Tree: dragged to another place of the Tree, or into the preview */
				float tx, ty, tw, th;
				Rid item, node;
				Rwidget_get_rect(g.tree, &tx, &ty, &tw, &th);
				item = Rtree_view_get_item_at(g.tree, event->x - tx, event->y - ty);
				node = item != R_INVALID_ID ? (Rid)(size_t)Rtree_item_get_user_handle(item) : R_INVALID_ID;
				if (node == R_INVALID_ID || !Rxml_node_is_valid(node) || Rxml_node_get_parent(node) == R_INVALID_ID) return R_FALSE;
				g.drag_armed = R_TRUE;
				g.drag_from_palette = R_FALSE;
				g.drag_node = node;
			} else if (!g.interact && over_stage(event->x, event->y)) {
				Rid node = node_under(event->x, event->y);
				/* (with Ctrl or Shift, a click chooses: it does not drag) */
				if (event->modifiers & (R_MOD_CTRL | R_MOD_SUPER | R_MOD_SHIFT)) return R_FALSE;
				if (node == R_INVALID_ID || Rxml_node_get_parent(node) == R_INVALID_ID) return R_FALSE;
				g.drag_armed = R_TRUE;
				g.drag_from_palette = R_FALSE;
				g.drag_node = node;
			}
			g.drag_start_x = event->x;
			g.drag_start_y = event->y;
			return R_FALSE;
		case R_EVENT_MOUSE_MOVE:
			if (!g.drag_armed) return R_FALSE;
			g.drag_x = event->x;
			g.drag_y = event->y;
			if (!g.dragging) {
				float dx = event->x - g.drag_start_x, dy = event->y - g.drag_start_y;
				if (dx * dx + dy * dy < 36.0f) return R_FALSE;
				if (g.drag_from_palette) {
					int row = palette_selected();
					if (row < 0 || row >= g.palette_count) {
						drag_reset();
						return R_FALSE;
					}
					snprintf(g.drag_tag, sizeof(g.drag_tag), "%s", g.palette_tags[row]);
				}
				g.dragging = R_TRUE;
			}
			g.drop_parent = R_INVALID_ID;
			Rtree_view_set_drop_highlight(g.tree, R_INVALID_ID, R_FALSE);
			if (over_tree(event->x, event->y)) {
				/* over the Tree: the panel that would receive it is lit, and said in the status bar */
				if (tree_drop_place(event->x, event->y, &g.drop_parent, &g.drop_index)) {
					Rid child = Rxml_node_get_child(g.drop_parent, g.drop_index);
					Rtree_view_set_drop_highlight(g.tree, tree_item_of(R_INVALID_ID, g.drop_parent), R_FALSE);
					if (child != R_INVALID_ID && child != g.drag_node) set_status("Into <%s>, before <%s>.", Rxml_node_get_tag(g.drop_parent), Rxml_node_is_comment(child) ? "!--" : Rxml_node_get_tag(child));
					else set_status("Into <%s>, at the end.", Rxml_node_get_tag(g.drop_parent));
				} else {
					g.drop_parent = R_INVALID_ID;
				}
				Rwidget_set_cursor(g.tree, g.drop_parent != R_INVALID_ID ? R_CURSOR_MOVE : R_CURSOR_NOT_ALLOWED);
				Rwidget_invalidate(g.outline);
				return R_TRUE;
			}
			Rwidget_set_cursor(g.tree, R_CURSOR_ARROW);
			if (over_stage(event->x, event->y) && !drop_place(event->x, event->y, &g.drop_parent, &g.drop_index)) g.drop_parent = R_INVALID_ID;
			if (g.drop_parent != R_INVALID_ID && !g.drag_from_palette && inside_dragged(g.drop_parent)) g.drop_parent = R_INVALID_ID;
			if (g.drop_parent != R_INVALID_ID && !drop_allowed(g.drop_parent)) g.drop_parent = R_INVALID_ID;
			Rwidget_set_cursor(g.outline, g.drop_parent != R_INVALID_ID ? R_CURSOR_MOVE : R_CURSOR_NOT_ALLOWED);
			Rwidget_invalidate(g.outline);
			return R_TRUE;
		case R_EVENT_MOUSE_UP:
			if (!g.drag_armed) return R_FALSE;
			if (g.dragging) {
				Rid node = g.drag_node;
				int from_palette = g.drag_from_palette;
				char tag[96];
				float x = event->x, y = event->y;
				snprintf(tag, sizeof(tag), "%s", g.drag_tag);
				drag_reset();
				/* (a drag: the selection stays as it is) */
				g.press_reduce = R_INVALID_ID;
				Rwidget_set_cursor(g.outline, R_CURSOR_ARROW);
				Rwidget_set_cursor(g.tree, R_CURSOR_ARROW);
				if (over_tree(x, y)) {
					/* dropped on the Tree: the place it showed */
					Rid parent;
					int index;
					g.drag_from_palette = from_palette;
					g.drag_node = node;
					if (tree_drop_place(x, y, &parent, &index)) {
						g.place_fixed = R_TRUE;
						g.fixed_parent = parent;
						g.fixed_index = index;
						g.drag_node = R_INVALID_ID;
						if (from_palette) drop_widget(tag, x, y);
						else drop_node(node, x, y);
					}
					g.place_fixed = R_FALSE;
					g.drag_node = R_INVALID_ID;
				} else if (over_stage(x, y)) {
					if (from_palette) drop_widget(tag, x, y);
					else drop_node(node, x, y);
				}
				return R_TRUE;
			}
			drag_reset();
			return R_FALSE;
		case R_EVENT_KEY_DOWN:
			if (g.dragging && event->key == R_KEY_ESCAPE) {
				drag_reset();
				set_status("Drag cancelled.");
				return R_TRUE;
			}
			return R_FALSE;
		default:
			return R_FALSE;
	}
}

/* ---------------------------------------------------------------------------
 *  Editing the tree
 * ------------------------------------------------------------------------- */

/* The element the commands of the Edit menu act on, or R_INVALID_ID with a word in the status bar. */
static Rid edit_target(int needs_parent) {
	Rid focus = Rwindow_get_focused_widget(g.window);
	/* (a key that also edits text, pressed in an input, was for the input) */
	if (focus != R_INVALID_ID && (Rwidget_get_type(focus) == R_WIDGET_TEXT_INPUT || Rwidget_get_type(focus) == R_WIDGET_TEXT_EDITOR)
		&& focus != g.palette_filter) {
		return R_INVALID_ID;
	}
	if (!g.has_document) {
		set_status("No interface is open.");
		return R_INVALID_ID;
	}
	if (!flush_source()) return R_INVALID_ID;
	if (g.selected == R_INVALID_ID || !Rxml_node_is_valid(g.selected)) {
		set_status("Nothing is selected.");
		return R_INVALID_ID;
	}
	if (needs_parent && Rxml_node_get_parent(g.selected) == R_INVALID_ID) {
		set_status("This cannot be done to the root of the document.");
		return R_INVALID_ID;
	}
	return g.selected;
}

/* The commands of the Edit menu on several elements selected: each one, or all of them together (wrap, move in). */
static void edit_many(const char* what) {
	Rid roots[MAX_EXTRA + 1], made[MAX_EXTRA + 1], parent;
	int count = selection_roots(roots, MAX_EXTRA + 1, strcmp(what, "copy") != 0), made_count = 0, i, same = R_TRUE;
	if (count == 0) {
		set_status("This cannot be done to the root of the document.");
		return;
	}
	parent = Rxml_node_get_parent(roots[0]);
	for (i = 1; i < count; i++) same = same && Rxml_node_get_parent(roots[i]) == parent;

	if (strcmp(what, "copy") == 0) {
		ds_buffer_t text = { 0 };
		for (i = 0; i < count; i++) {
			const char* xml = Rxml_node_to_string(roots[i]);
			ds_buffer_add(&text, xml, strlen(xml));
			ds_buffer_add(&text, "\n", 1);
		}
		Rclipboard_set_text(text.data ? text.data : "");
		ds_buffer_free(&text);
		set_status("Copied %d elements as XML.", count);
		return;
	}
	if (strcmp(what, "delete") == 0) {
		for (i = 0; i < count; i++) Rxml_node_remove(roots[i]);
		g.selected = parent;
		g.extra_count = 0;
		document_changed(R_FALSE);
		select_node(parent, R_TRUE);
		set_status("Deleted %d elements.", count);
		return;
	}
	if (strcmp(what, "duplicate") == 0) {
		/* (each copy goes right after its element) */
		for (i = 0; i < count; i++) {
			Rid copy = Rxml_node_duplicate(roots[i]);
			if (copy != R_INVALID_ID) made[made_count++] = copy;
		}
		set_status("Duplicated %d elements.", made_count);
	} else if (strcmp(what, "up") == 0 || strcmp(what, "down") == 0) {
		int up = what[0] == 'u';
		/* one step each, the first ones first; a block that touches the edge, or another selected, stays */
		for (i = up ? 0 : count - 1; up ? i < count : i >= 0; i += up ? 1 : -1) {
			Rid holder = Rxml_node_get_parent(roots[i]), next;
			int index = Rxml_node_get_index(roots[i]);
			next = up ? (index > 0 ? Rxml_node_get_child(holder, index - 1) : R_INVALID_ID) : Rxml_node_get_child(holder, index + 1);
			if (next == R_INVALID_ID || is_selected(next)) continue;
			Rxml_node_move(roots[i], holder, up ? index - 1 : index + 1);
		}
		for (i = 0; i < count; i++) made[made_count++] = roots[i];
	} else if (strcmp(what, "out") == 0) {
		/* (the last first: they keep their order after the panel they leave) */
		for (i = count - 1; i >= 0; i--) {
			Rid holder = Rxml_node_get_parent(roots[i]);
			Rid grand = holder != R_INVALID_ID ? Rxml_node_get_parent(holder) : R_INVALID_ID;
			if (grand == R_INVALID_ID) continue;
			Rxml_node_move(roots[i], grand, Rxml_node_get_index(holder) + 1);
		}
		for (i = 0; i < count; i++) made[made_count++] = roots[i];
	} else if (strcmp(what, "in") == 0) {
		int index = Rxml_node_get_index(roots[0]);
		Rid previous = same && index > 0 ? Rxml_node_get_child(parent, index - 1) : R_INVALID_ID;
		if (!same) {
			set_status("To move them into the element before them, select elements of the same panel.");
			return;
		}
		if (previous == R_INVALID_ID || Rxml_node_is_comment(previous) || !accepts_children(previous)) {
			set_status("There is no panel before them to move into.");
			return;
		}
		for (i = 0; i < count; i++) {
			Rxml_node_move(roots[i], previous, -1);
			made[made_count++] = roots[i];
		}
	} else if (strncmp(what, "wrap.", 5) == 0) {
		if (same) {
			/* elements of one panel: one panel around them all, where the first one was */
			Rid wrapper = Rxml_node_add_child(parent, what + 5, Rxml_node_get_index(roots[0]));
			if (wrapper == R_INVALID_ID) return;
			for (i = 0; i < count; i++) Rxml_node_move(roots[i], wrapper, -1);
			made[made_count++] = wrapper;
			set_status("Wrapped %d elements in one <%s>.", count, what + 5);
		} else {
			for (i = 0; i < count; i++) {
				Rid holder = Rxml_node_get_parent(roots[i]);
				Rid wrapper = Rxml_node_add_child(holder, what + 5, Rxml_node_get_index(roots[i]));
				if (wrapper == R_INVALID_ID) continue;
				Rxml_node_move(roots[i], wrapper, -1);
				made[made_count++] = wrapper;
			}
			set_status("Wrapped each element in a <%s>.", what + 5);
		}
	} else if (strcmp(what, "unwrap") == 0) {
		for (i = 0; i < count; i++) {
			Rid holder = Rxml_node_get_parent(roots[i]);
			int index = Rxml_node_get_index(roots[i]), children = Rxml_node_get_child_count(roots[i]), k;
			if (children == 0) continue;
			for (k = 0; k < children; k++) {
				Rid child = Rxml_node_get_child(roots[i], 0);
				Rxml_node_move(child, holder, index + k);
				if (made_count < MAX_EXTRA) made[made_count++] = child;
			}
			Rxml_node_remove(roots[i]);
		}
		set_status(made_count ? "Unwrapped." : "They contain nothing to keep.");
	} else {
		return;
	}
	if (made_count == 0) return;
	/* (set before the document is shown again: the Tree shows them) */
	g.selected = made[made_count - 1];
	g.extra_count = 0;
	for (i = 0; i < made_count - 1; i++) g.extra[g.extra_count++] = made[i];
	document_changed(R_FALSE);
	select_nodes(made, made_count);
}

static void action_edit(Rid sender, const char* action, void* user_data) {
	const char* what = action + 5;
	Rid node, parent;
	int index;
	crash_report_note("action %s", action);
	(void)sender;
	(void)user_data;

	if (strcmp(what, "deselect") == 0) {
		select_node(R_INVALID_ID, R_FALSE);
		set_status("Nothing selected.");
		return;
	}
	/* (Delete, Backspace, Ctrl+X, C, V: for the elements of the Window and Widget tabs only; the other tabs have their own) */
	if (g.tab != TAB_WINDOW && g.tab != TAB_WIDGET) return;
	/* Cut: copied, then deleted */
	if (strcmp(what, "cut") == 0) {
		int count = selection_count();
		if (edit_target(R_TRUE) == R_INVALID_ID) return;
		action_edit(sender, "edit.copy", user_data);
		action_edit(sender, "edit.delete", user_data);
		if (count > 1) set_status("Cut %d elements: Ctrl+V puts them back inside the element selected.", count);
		else set_status("Cut: Ctrl+V puts it back inside the element selected.");
		return;
	}
	/* several elements selected: the command acts on all of them */
	if (strcmp(what, "paste") != 0 && selection_count() > 1) {
		if (edit_target(R_FALSE) != R_INVALID_ID) edit_many(what);
		return;
	}
	if (strcmp(what, "copy") == 0) {
		if ((node = edit_target(R_FALSE)) == R_INVALID_ID) return;
		Rclipboard_set_text(Rxml_node_to_string(node));
		set_status("Copied <%s> as XML.", Rxml_node_get_tag(node));
		return;
	}
	if (strcmp(what, "paste") == 0) {
		const char* text = Rclipboard_get_text();
		if (text && text[0]) insert_xml(text);
		else set_status("The clipboard is empty.");
		return;
	}
	if ((node = edit_target(R_TRUE)) == R_INVALID_ID) return;
	parent = Rxml_node_get_parent(node);
	index = Rxml_node_get_index(node);

	if (strcmp(what, "delete") == 0) {
		Rid next = Rxml_node_get_child(parent, index + 1);
		if (next == R_INVALID_ID) next = index > 0 ? Rxml_node_get_child(parent, index - 1) : parent;
		Rxml_node_remove(node);
		g.selected = next;
		set_status("Deleted.");
	} else if (strcmp(what, "duplicate") == 0) {
		g.selected = Rxml_node_duplicate(node);
		set_status("Duplicated.");
	} else if (strcmp(what, "up") == 0) {
		if (index == 0) return;
		Rxml_node_move(node, parent, index - 1);
	} else if (strcmp(what, "down") == 0) {
		if (index + 1 >= Rxml_node_get_child_count(parent)) return;
		Rxml_node_move(node, parent, index + 1);
	} else if (strcmp(what, "out") == 0) {
		Rid grand = Rxml_node_get_parent(parent);
		if (grand == R_INVALID_ID) {
			set_status("It is already at the top.");
			return;
		}
		Rxml_node_move(node, grand, Rxml_node_get_index(parent) + 1);
	} else if (strcmp(what, "in") == 0) {
		Rid previous = index > 0 ? Rxml_node_get_child(parent, index - 1) : R_INVALID_ID;
		if (previous == R_INVALID_ID || Rxml_node_is_comment(previous)) {
			set_status("There is no element before it to move into.");
			return;
		}
		Rxml_node_move(node, previous, -1);
	} else if (strncmp(what, "wrap.", 5) == 0) {
		Rid wrapper = Rxml_node_add_child(parent, what + 5, index);
		if (wrapper == R_INVALID_ID) return;
		Rxml_node_move(node, wrapper, -1);
		/* its place in the parent goes to what now holds it */
		{
			static const char* const slot[] = { "Slot.Fill", "Slot.HAlign", "Slot.VAlign", "Slot.Padding", "Slot.Title", "Slot.Column", "Slot.Row", "Slot.Dock" };
			int i;
			for (i = 0; i < (int)(sizeof(slot) / sizeof(slot[0])); i++) {
				const char* value = Rxml_node_get_attribute(node, slot[i]);
				if (!value) continue;
				Rxml_node_set_attribute(wrapper, slot[i], value);
				Rxml_node_set_attribute(node, slot[i], NULL);
			}
		}
		g.selected = wrapper;
		set_status("Wrapped in <%s>.", what + 5);
	} else if (strcmp(what, "unwrap") == 0) {
		int count = Rxml_node_get_child_count(node), i;
		if (count == 0) {
			set_status("<%s> contains nothing to keep.", Rxml_node_get_tag(node));
			return;
		}
		g.selected = Rxml_node_get_child(node, 0);
		for (i = 0; i < count; i++) Rxml_node_move(Rxml_node_get_child(node, 0), parent, index + i);
		Rxml_node_remove(node);
		set_status("Unwrapped.");
	} else {
		return;
	}
	document_changed(R_FALSE);
	select_node(g.selected, R_TRUE);
}

static void action_history(Rid sender, const char* action, void* user_data) {
	int step = strcmp(action, "edit.undo") == 0 ? -1 : 1;
	Rid focus = Rwindow_get_focused_widget(g.window);
	crash_report_note("action %s", action);
	(void)sender;
	(void)user_data;
	/* the Theme tab has its own history */
	if (g.tab == TAB_THEME) {
		Raction_fire(step < 0 ? "te.undo" : "te.redo", R_INVALID_ID);
		return;
	}
	if (g.tab == TAB_STRINGS) return;
	/* in a text, Ctrl+Z is for the text */
	if (focus != R_INVALID_ID && (Rwidget_get_type(focus) == R_WIDGET_TEXT_INPUT || Rwidget_get_type(focus) == R_WIDGET_TEXT_EDITOR)) return;
	if (!g.has_document) return;
	if (!flush_source() && step > 0) return;
	if (g.history_at + step < 0 || g.history_at + step >= g.history_count) {
		set_status(step < 0 ? "Nothing to undo." : "Nothing to redo.");
		return;
	}
	g.history_at += step;
	g.history_typed_at = 0.0;
	set_document_text(g.history[g.history_at], R_FALSE);
	g.modified = R_TRUE;
	update_title();
	set_status(step < 0 ? "Undone." : "Redone.");
}

/* ---------------------------------------------------------------------------
 *  Files
 * ------------------------------------------------------------------------- */

static int save_file(int ask);

/* Save, don't save, or cancel: what the user wants before `why` ("opening another folder"...). */
int ds_confirm_unsaved(const char* why) {
	char text[400];
	RMessageResult answer;
	int interface = g.has_document && g.modified;
	int other = g.slots[0].modified || g.slots[1].modified, k;
	if (g.quitting) return R_TRUE;
	if (!interface && !other && !strings_any_modified() && !te_is_modified()) return R_TRUE;
	snprintf(text, sizeof(text), "Some files have changes that were not saved. Save them before %s?", why);
	answer = Rmessage_box(g.window, "Unsaved changes", text, R_MESSAGE_YES_NO_CANCEL);
	if (answer == R_MESSAGE_RESULT_CANCEL) return R_FALSE;
	if (answer == R_MESSAGE_RESULT_NO) {
		g.modified = R_FALSE;
		g.slots[0].modified = g.slots[1].modified = R_FALSE;
		/* (the tables go back to what their files say) */
		if (strings_any_modified()) strings_reload();
		return R_TRUE;
	}
	/* (an interface never saved asks where; refusing there cancels) */
	if (interface && !save_file(!g.path)) return R_FALSE;
	for (k = 0; k < 2; k++) {
		if (!g.slots[k].modified || !g.slots[k].path || !g.slots[k].text) continue;
		if (!Rfile_write(g.slots[k].path, g.slots[k].text, strlen(g.slots[k].text))) return R_FALSE;
		g.slots[k].modified = R_FALSE;
	}
	if (te_is_modified() && !te_save()) return R_FALSE;
	if (strings_any_modified() && !strings_save()) return R_FALSE;
	return R_TRUE;
}

static int confirm_discard(void) {
	return ds_confirm_unsaved("closing");
}

static void show_document(int shown) {
	g.has_document = shown;
	Rwidget_set_visibility(g.frame, shown ? R_VISIBLE : R_COLLAPSED);
	Rwidget_set_visibility(g.outline, shown ? R_VISIBLE : R_COLLAPSED);
	Rwidget_set_visibility(Rwidget_find(g.window, "stage.empty"), shown ? R_COLLAPSED : R_VISIBLE);
	Rtext_editor_set_read_only(g.source, !shown);
}

static void new_document(const char* text, const char* path) {
	free(g.path);
	g.path = path ? copy_text(path) : NULL;
	show_document(R_TRUE);
	welcome_hide();
	history_clear();
	if (g.document != R_INVALID_ID) Rxml_document_destroy(g.document);
	g.document = R_INVALID_ID;
	g.selected = R_INVALID_ID;
	if (!set_document_text(text, R_TRUE)) {
		/* a file that is not XML: an empty document to stand on, the text left to be fixed */
		g.document = Rxml_document_create("VBox");
		g.selected = R_INVALID_ID;
		rebuild_all(R_FALSE);
		g.broken = R_TRUE;
	}
	g.modified = R_FALSE;
	update_title();
	palette_fill();
}

/* the paths written in a file (images, fonts, includes) start from its folder */
static void base_path_of(const char* path) {
	char* folder = copy_text(path);
	char* slash = strrchr(folder, '/');
	char* backslash = strrchr(folder, '\\');
	if (backslash && (!slash || backslash > slash)) slash = backslash;
	if (slash) {
		*slash = '\0';
		Rxml_set_base_path(folder);
	}
	free(folder);
}

static void open_file(const char* path) {
	size_t size = 0;
	char* text = Rfile_read(path, &size);
	crash_report_note("open %s", path);
	if (!text) {
		set_status("Cannot read %s", path);
		return;
	}
	base_path_of(path);
	Rdock_area_set_panel_title(g.dock, g.source, "XML");
	Rtext_editor_set_language(g.source, "XML");
	new_document(text, path);
	Rmemory_free(text);
	if (!g.broken) set_status("Opened %s", path);
}

static int save_file(int ask) {
	static const RFileFilter filters[] = { { "XML", "*.xml" }, { "All files", "*" } };
	const char* text;
	if (!g.has_document) return R_TRUE;
	if (ask || !g.path) {
		RFileDialogDesc desc;
		const char* chosen;
		memset(&desc, 0, sizeof(desc));
		desc.parent = g.window;
		desc.title = "Save the interface";
		desc.file_name = "interface.xml";
		desc.directory = g_folder;
		desc.filters = filters;
		desc.filter_count = 2;
		desc.default_extension = "xml";
		chosen = Rfile_dialog_save(&desc);
		if (!chosen) return R_FALSE;
		chosen = copy_text(chosen);
		free(g.path);
		g.path = (char*)chosen;
	}
	/* what is saved is the text as it is written, mistakes included: nothing typed is lost */
	text = Rtext_editor_get_text(g.source);
	if (!Rfile_write(g.path, text, strlen(text))) {
		set_status("Cannot write %s", g.path);
		return R_FALSE;
	}
	g.modified = R_FALSE;
	update_title();
	set_status("Saved %s", g.path);
	/* (the folder shows it at once, and does not take the save for a change made elsewhere) */
	folder_refresh();
	return R_TRUE;
}

/* The folder of a file: what opens with it, when it is not in the folder open. */
static void open_with_folder(const char* path) {
	char* folder = ds_normalize(path);
	char* slash = strrchr(folder, '/');
	int inside = g_folder && !strncmp(folder, g_folder, strlen(g_folder)) && (folder[strlen(g_folder)] == '/' || folder[strlen(g_folder)] == '\0');
	if (slash) *slash = '\0';
	if (!inside) {
		if (slash && ds_is_dir(folder)) {
			/* (folder_open opens the first interface of the folder: this one is opened after it) */
			if (!folder_open(folder)) {
				free(folder);
				return;
			}
		}
	} else if (!ds_confirm_unsaved("opening another interface")) {
		free(folder);
		return;
	}
	if (!g.path || !ds_same_path(g.path, path)) open_file(path);
	free(folder);
}

static void action_file(Rid sender, const char* action, void* user_data) {
	static const RFileFilter filters[] = { { "Interfaces", "*.xml" }, { "All files", "*" } };
	crash_report_note("action %s", action);
	(void)sender;
	(void)user_data;
	if (g.fuzzing && (strcmp(action, "folder.open") == 0 || strcmp(action, "file.open") == 0 || strcmp(action, "file.save_as") == 0 ||
		strcmp(action, "file.quit") == 0 || ((strcmp(action, "file.save") == 0 || strcmp(action, "file.save_all") == 0) && !g.path))) return;
	if (strcmp(action, "file.welcome") == 0) {
		welcome_show();
		update_title();
	} else if (strcmp(action, "folder.open") == 0) {
		RFileDialogDesc desc;
		const char* chosen;
		memset(&desc, 0, sizeof(desc));
		desc.parent = g.window;
		desc.title = "Open the folder of your interfaces";
		desc.directory = g_folder;
		chosen = Rfile_dialog_select_folder(&desc);
		if (chosen) {
			char* path = copy_text(chosen);
			folder_open(path);
			free(path);
		}
	} else if (strcmp(action, "file.open") == 0) {
		RFileDialogDesc desc;
		const char* chosen;
		memset(&desc, 0, sizeof(desc));
		desc.parent = g.window;
		desc.title = "Open an interface";
		desc.directory = g_folder;
		desc.filters = filters;
		desc.filter_count = 2;
		chosen = Rfile_dialog_open(&desc);
		if (chosen) {
			char* path = copy_text(chosen);
			open_with_folder(path);
			free(path);
		}
	} else if (strcmp(action, "file.save") == 0) {
		if (g.tab == TAB_THEME) {
			if (te_save()) folder_refresh();
		} else if (g.tab == TAB_STRINGS) {
			if (strings_save()) set_status("Translations saved.");
		} else {
			save_file(R_FALSE);
		}
	} else if (strcmp(action, "file.save_as") == 0) {
		if (g.tab == TAB_THEME) Raction_fire("te.save_as", R_INVALID_ID);
		else save_file(R_TRUE);
	} else if (strcmp(action, "file.save_all") == 0) {
		int k;
		if (g.has_document && g.modified) save_file(R_FALSE);
		/* the interface the other tab holds */
		for (k = 0; k < 2; k++) {
			if (g.slots[k].has_document && g.slots[k].modified && g.slots[k].path && g.slots[k].text &&
				Rfile_write(g.slots[k].path, g.slots[k].text, strlen(g.slots[k].text))) g.slots[k].modified = R_FALSE;
		}
		if (te_is_modified()) te_save();
		strings_save();
		folder_refresh();
		set_status("Everything is saved.");
	} else if (strcmp(action, "file.quit") == 0) {
		if (confirm_discard()) Rquit(0);
	}
}

static int on_close(Rid window, void* user_data) {
	(void)window;
	(void)user_data;
	if (g.fuzzing) return R_FALSE;
	if (!confirm_discard()) return R_FALSE;
	Rquit(0);
	return R_TRUE;
}

static int on_drop(Rid target, const RDropEvent* event, void* user_data) {
	(void)target;
	(void)user_data;
	/* a folder dropped on the window opens; an interface opens with its folder; a text is added as XML */
	if (event->phase != R_DROP_DROPPED) return R_TRUE;
	if (event->path_count > 0) {
		if (ds_is_dir(event->paths[0])) folder_open(event->paths[0]);
		else if (ds_iequal(ds_extension(event->paths[0]), "xml")) open_with_folder(event->paths[0]);
		else set_status("Only folders and interfaces (.xml) open here.");
	} else if (event->text && event->text[0]) {
		insert_xml(event->text);
	}
	return R_TRUE;
}

/* ---------------------------------------------------------------------------
 *  What the rest of the designer asks the visual editor (designer.h)
 * ------------------------------------------------------------------------- */

void designer_open_file(const char* path) {
	open_file(path);
	folder_refresh();
}

void designer_update_title(void) {
	update_title();
}

const char* designer_get_path(void) {
	return g.has_document ? g.path : NULL;
}

int designer_is_modified(void) {
	return g.has_document && g.modified;
}

int designer_save(void) {
	if (!g.has_document) return R_TRUE;
	return save_file(!g.path);
}

int designer_problem_count(void) {
	return g.problem_count;
}

const char* designer_problem_text(int index) {
	return index >= 0 && index < g.problem_count ? g.problems[index].text : "";
}

int designer_has_document(void) {
	return g.has_document;
}

const char* designer_template(const char* kind) {
	if (!strcmp(kind, "panel")) return TEMPLATE_PANEL;
	if (!strcmp(kind, "form")) return TEMPLATE_FORM;
	if (!strcmp(kind, "widgets")) return TEMPLATE_WIDGETS;
	return TEMPLATE_WINDOW;
}

/* The file was changed by another program: read again, unless it has changes of its own here. */
void designer_file_changed(void) {
	size_t size = 0;
	char* text;
	if (!g.has_document || !g.path) return;
	if (g.modified) {
		set_status("%s changed on disk; it has unsaved changes here, which are kept.", ds_basename(g.path));
		return;
	}
	text = Rfile_read(g.path, &size);
	if (!text) return;
	if (strcmp(text, Rtext_editor_get_text(g.source)) != 0) {
		if (set_document_text(text, R_TRUE)) set_status("%s changed on disk: read again.", ds_basename(g.path));
		g.modified = R_FALSE;
		update_title();
	}
	Rmemory_free(text);
}

void designer_close(void) {
	if (g.parse_timer != R_INVALID_ID && Rtimer_is_valid(g.parse_timer)) Rtimer_destroy(g.parse_timer);
	g.parse_timer = R_INVALID_ID;
	free(g.path);
	g.path = NULL;
	history_clear();
	if (g.preview != R_INVALID_ID && Rwidget_is_valid(g.preview)) Rwidget_destroy(g.preview);
	g.preview = R_INVALID_ID;
	if (g.document != R_INVALID_ID) Rxml_document_destroy(g.document);
	/* (an empty document stands behind, so that nothing has to check for none) */
	g.document = Rxml_document_create("VBox");
	g.selected = R_INVALID_ID;
	g.hovered = R_INVALID_ID;
	g.broken = R_FALSE;
	g.modified = R_FALSE;
	g.syncing++;
	Rtext_editor_set_text(g.source, "");
	Rtree_view_clear_items(g.tree);
	g.syncing--;
	g.problem_count = 0;
	show_problems();
	rebuild_inspector();
	Rlabel_set_text(Rwidget_find(g.window, "stage.size"), "");
	Rlabel_set_text(g.path_label, "");
	show_document(R_FALSE);
	update_title();
}

void designer_show_problems(void) {
	Rdock_area_open_panel(g.dock, g.problems_list);
	Rdock_area_show_panel(g.dock, g.problems_list);
	if (g.problem_count > 0 && g.problems[0].line > 0) {
		Rtext_editor_set_cursor(g.source, g.problems[0].line - 1, 0);
		Rtext_editor_scroll_to_line(g.source, g.problems[0].line - 1);
	}
}

void designer_refresh_preview(void) {
	if (g.document != R_INVALID_ID && !g.broken && g.has_document) rebuild_preview();
}

Rid designer_get_frame(void) {
	return g.frame;
}

Rid designer_get_selected(void) {
	return g.has_document && g.selected != R_INVALID_ID && Rxml_node_is_valid(g.selected) ? g.selected : R_INVALID_ID;
}

void designer_selection_path(char* out, size_t size) {
	Rid chain[32], walk;
	int depth = 0, i, length = 0;
	out[0] = '\0';
	if (designer_get_selected() == R_INVALID_ID) return;
	for (walk = g.selected; walk != R_INVALID_ID && depth < 32; walk = Rxml_node_get_parent(walk)) chain[depth++] = walk;
	for (i = depth - 1; i >= 0 && length < (int)size - 40; i--) {
		length += snprintf(out + length, size - (size_t)length, "%s%s", Rxml_node_is_comment(chain[i]) ? "comment" : Rxml_node_get_tag(chain[i]), i > 0 ? "  >  " : "");
	}
	if (selection_count() > 1 && length < (int)size - 24) snprintf(out + length, size - (size_t)length, "  (+%d)", g.extra_count);
}

void designer_set_attribute(const char* name, const char* value) {
	if (designer_get_selected() == R_INVALID_ID || !flush_source()) return;
	prune_selection();
	if (g.selected == R_INVALID_ID) return;
	set_on_selection(name, value);
	document_changed(R_TRUE);
	/* (the Properties panel shows it too) */
	rebuild_inspector();
}

static const char* template_of(const char* tag) {
	int i;
	for (i = 0; i < (int)(sizeof(TEMPLATES) / sizeof(TEMPLATES[0])); i++) {
		if (same_text(TEMPLATES[i].tag, tag)) return TEMPLATES[i].xml;
	}
	return NULL;
}

void designer_add_widget(const char* tag) {
	const char* xml = template_of(tag);
	char text[160];
	if (!xml) {
		snprintf(text, sizeof(text), "<%s/>", tag);
		xml = text;
	}
	insert_xml(xml);
}

int designer_widget_count(void) {
	return g.palette_count;
}

const char* designer_widget_at(int index) {
	return index >= 0 && index < g.palette_count ? g.palette_tags[index] : "";
}

/* ---- the preview: device, theme ---- */

/* (width -1: the frame takes the room the panel gives it; 0: the size the XML says) */
static const struct { const char* name; int width, height; } DEVICES[] = {
	{ "Fit to the panel", -1, -1 },
	{ "Size in the XML", 0, 0 },
	{ "Phone  390 x 844", 390, 844 },
	{ "Phone landscape  844 x 390", 844, 390 },
	{ "Tablet  820 x 1180", 820, 1180 },
	{ "Laptop  1280 x 800", 1280, 800 },
	{ "Desktop  1920 x 1080", 1920, 1080 }
};
#define DEVICE_COUNT ((int)(sizeof(DEVICES) / sizeof(DEVICES[0])))

/* The looks Raven has, by the name a theme file or Theme= gives them. */
static const struct { const char* name; const char* label; } LOOKS[] = {
	{ "RavenDark", "Raven dark" }, { "RavenLight", "Raven light" }, { "RavenGlassDark", "Raven glass dark" },
	{ "RavenGlassLight", "Raven glass light" }, { "RavenHighContrastDark", "High contrast dark" },
	{ "RavenHighContrastLight", "High contrast light" }, { "WindowsDark", "Windows 11 dark" }, { "WindowsLight", "Windows 11 light" },
	{ "MacOSDark", "macOS dark" }, { "MacOSLight", "macOS light" }, { "MacOSGlassDark", "macOS 26 glass dark" },
	{ "MacOSGlassLight", "macOS 26 glass light" }, { "GnomeDark", "GNOME dark" }, { "GnomeLight", "GNOME light" }
};
#define LOOK_COUNT ((int)(sizeof(LOOKS) / sizeof(LOOKS[0])))

/* The theme combo box: the look of the designer, the looks of Raven, then the themes of the folder. */
static void fill_theme_combo(void) {
	int i, selected = 0, count = folder_theme_count();
	char item[200];
	g.syncing++;
	Rcombo_box_clear_items(g.theme_combo);
	Rcombo_box_add_item(g.theme_combo, "Same as the designer");
	for (i = 0; i < count; i++) {
		snprintf(item, sizeof(item), "Theme: %s", folder_theme_name(i));
		Rcombo_box_add_item(g.theme_combo, item);
		if (g.preview_theme[0] && !strcmp(folder_theme_name(i), g.preview_theme)) selected = 1 + i;
	}
	for (i = 0; i < LOOK_COUNT; i++) {
		Rcombo_box_add_item(g.theme_combo, LOOKS[i].label);
		if (g.preview_theme[0] && !strcmp(LOOKS[i].name, g.preview_theme)) selected = 1 + count + i;
	}
	Rcombo_box_set_selected(g.theme_combo, selected);
	g.syncing--;
}

void designer_set_preview_theme(const char* name) {
	snprintf(g.preview_theme, sizeof(g.preview_theme), "%s", name ? name : "");
	if (!Rwidget_set_theme(g.frame, g.preview_theme[0] ? g.preview_theme : NULL)) {
		g.preview_theme[0] = '\0';
		Rwidget_set_theme(g.frame, NULL);
	}
	fill_theme_combo();
	run_window_follow();
	designer_refresh_preview();
}

void designer_themes_changed(void) {
	int i, still = !g.preview_theme[0];
	for (i = 0; i < LOOK_COUNT && !still; i++) still = !strcmp(LOOKS[i].name, g.preview_theme);
	for (i = 0; i < folder_theme_count() && !still; i++) still = !strcmp(folder_theme_name(i), g.preview_theme);
	/* (a theme whose file went away: back to the look of the designer) */
	if (!still) designer_set_preview_theme("");
	else fill_theme_combo();
	fill_theme_list();
	fill_appearance();
	run_window_follow();
}

static void on_theme_changed(Rid combo, int index, void* user_data) {
	int count = folder_theme_count();
	(void)combo;
	(void)user_data;
	if (g.syncing || index < 0) return;
	if (index == 0) designer_set_preview_theme("");
	else if (index <= count) designer_set_preview_theme(folder_theme_name(index - 1));
	else if (index - 1 - count < LOOK_COUNT) designer_set_preview_theme(LOOKS[index - 1 - count].name);
	set_status(g.preview_theme[0] ? "The preview wears %s." : "The preview wears the look of the designer.%s", g.preview_theme);
}

static void on_device_changed(Rid combo, int index, void* user_data) {
	(void)combo;
	(void)user_data;
	if (g.syncing || index < 0 || index >= DEVICE_COUNT) return;
	g.device = index;
	g.device_width = DEVICES[index].width;
	g.device_height = DEVICES[index].height;
	designer_refresh_preview();
	if (DEVICES[index].width < 0) set_status("The window takes the room of the panel: it follows the size of the designer.%s", "");
	else set_status(DEVICES[index].width ? "The preview is the size of a %s." : "The preview is the size the XML gives.%s", DEVICES[index].width ? DEVICES[index].name : "");
	run_window_follow();
}

/* ---------------------------------------------------------------------------
 *  Tabs: Window, Widget, Theme, Translations
 * ------------------------------------------------------------------------- */

static const char* const TAB_ACTIONS[4] = { "tab.window", "tab.widget", "tab.theme", "tab.strings" };

/* What the Window or the Widget tab holds goes aside while the other one is in front. */
static void slot_store(int tab) {
	int i;
	if (tab != TAB_WINDOW && tab != TAB_WIDGET) return;
	free(g.slots[tab].path);
	free(g.slots[tab].text);
	for (i = 0; i < g.slots[tab].history_count; i++) free(g.slots[tab].history[i]);
	g.slots[tab].has_document = g.has_document;
	g.slots[tab].path = g.path;
	g.slots[tab].text = g.has_document ? copy_text(Rtext_editor_get_text(g.source)) : NULL;
	g.slots[tab].modified = g.modified;
	memcpy(g.slots[tab].history, g.history, sizeof(g.history));
	g.slots[tab].history_count = g.history_count;
	g.slots[tab].history_at = g.history_at;
	/* (taken by the slot: the editor forgets them without freeing them) */
	g.path = NULL;
	g.history_count = 0;
	g.history_at = 0;
}

static void slot_restore(int tab) {
	char* path = g.slots[tab].path;
	char* text = g.slots[tab].text;
	int has = g.slots[tab].has_document, modified = g.slots[tab].modified;
	g.slots[tab].path = NULL;
	g.slots[tab].text = NULL;
	designer_close();
	if (!has || !text) {
		free(path);
		free(text);
		g.slots[tab].history_count = 0;
		return;
	}
	new_document(text, path);
	history_clear();
	memcpy(g.history, g.slots[tab].history, sizeof(g.history));
	g.history_count = g.slots[tab].history_count;
	g.history_at = g.slots[tab].history_at;
	g.slots[tab].history_count = 0;
	if (g.history_count == 0) history_push(text, R_FALSE);
	g.modified = modified;
	update_title();
	free(path);
	free(text);
}

/* The page of the switcher a tab shows (the start page is 0). */
int designer_tab_page(void) {
	return g.tab == TAB_THEME ? 2 : g.tab == TAB_STRINGS ? 3 : 1;
}

int designer_get_tab(void) {
	return g.tab;
}

void designer_show_tab(int tab) {
	crash_report_note("show the tab %d (was %d)", tab, g.tab);
	static const char* const empty[2] = {
		"Double click a window in Files, make one with New, or press Ctrl+K.",
		"Double click a widget library in Files, or make one with New: its tags can then be used by every window of the folder."
	};
	int i;
	if (tab < TAB_WINDOW || tab > TAB_STRINGS) return;
	if (tab != g.tab) {
		if (g.tab == TAB_WINDOW || g.tab == TAB_WIDGET) {
			if (g.has_document) flush_source();
			slot_store(g.tab);
			designer_close();
		}
		g.tab = tab;
		folder_set_view(tab);
		if (tab == TAB_WINDOW || tab == TAB_WIDGET) {
			slot_restore(tab);
			/* nothing open in it yet: its first file */
			if (!g.has_document) folder_open_first(tab);
		}
		/* (the tags declared in the folder may have changed while a library was edited) */
		folder_register_widgets();
		palette_fill();
	}
	/* the Theme tab opens on the first theme of the folder */
	if (tab == TAB_THEME && !te_get_path()[0] && !te_is_modified() && folder_theme_count() > 0) {
		te_open(folder_theme_path(0));
		fill_theme_list();
	}
	for (i = 0; i < 4; i++) Rbutton_set_style(g.tab_buttons[i], i == tab ? R_BUTTON_PRIMARY : R_BUTTON_FLAT);
	if (tab == TAB_WINDOW || tab == TAB_WIDGET) Rlabel_set_text(Rwidget_find(g.window, "stage.empty.text"), empty[tab]);
	if (!welcome_is_shown()) Rswitcher_set_index(Rwidget_find(g.window, "pages"), designer_tab_page());
	update_title();
}

void designer_widgets_changed(void) {
	palette_fill();
	designer_refresh_preview();
}

/* Another folder: what the tabs held goes. */
void designer_forget_tabs(void) {
	int k, h;
	for (k = 0; k < 2; k++) {
		free(g.slots[k].path);
		free(g.slots[k].text);
		for (h = 0; h < g.slots[k].history_count; h++) free(g.slots[k].history[h]);
		memset(&g.slots[k], 0, sizeof(g.slots[k]));
	}
}

static void action_tab(Rid sender, const char* action, void* user_data) {
	int i;
	crash_report_note("action %s", action);
	(void)sender;
	(void)user_data;
	for (i = 0; i < 4; i++) {
		if (!strcmp(action, TAB_ACTIONS[i])) {
			if (welcome_is_shown() && g_folder) welcome_hide();
			designer_show_tab(i);
		}
	}
}

/* ---- the Theme tab: the themes of the folder, and the editor of themes ---- */

static void fill_theme_list(void) {
	int i, count = folder_theme_count(), selected = -1;
	const char* open = te_get_path();
	g.syncing++;
	Rlist_view_clear_items(g.theme_list);
	for (i = 0; i < count; i++) {
		Rlist_view_add_item(g.theme_list, folder_theme_name(i));
		if (open && open[0] && ds_same_path(open, folder_theme_path(i))) selected = i;
	}
	if (selected >= 0) Rlist_view_set_selected(g.theme_list, selected);
	g.syncing--;
}

static int confirm_theme(void) {
	RMessageResult answer;
	if (!te_is_modified() || g.quitting || g_ds_fuzzing) return R_TRUE;
	answer = Rmessage_box(g.window, "Unsaved theme", "The theme has changes that were not saved. Save them?", R_MESSAGE_YES_NO_CANCEL);
	if (answer == R_MESSAGE_RESULT_CANCEL) return R_FALSE;
	if (answer == R_MESSAGE_RESULT_YES) return te_save();
	return R_TRUE;
}

void designer_open_theme(const char* path) {
	if (!confirm_theme()) return;
	if (te_open(path)) ds_status("The theme %s is open: the windows of the folder wear it once it is saved.", ds_basename(path));
	fill_theme_list();
}

static void on_theme_list(Rid list, int index, void* user_data) {
	(void)list;
	(void)user_data;
	if (g.syncing || index < 0 || index >= folder_theme_count()) return;
	if (te_get_path()[0] && ds_same_path(te_get_path(), folder_theme_path(index))) return;
	designer_open_theme(folder_theme_path(index));
}

/* ---- the appearance of the designer itself ---- */

static const struct { const char* name; RLook look; } DESIGNER_LOOKS[] = {
	{ "Raven dark", R_LOOK_RAVEN_DARK }, { "Raven light", R_LOOK_RAVEN_LIGHT }, { "Raven glass dark", R_LOOK_RAVEN_GLASS_DARK },
	{ "Raven glass light", R_LOOK_RAVEN_GLASS_LIGHT }, { "High contrast dark", R_LOOK_RAVEN_HIGH_CONTRAST_DARK },
	{ "High contrast light", R_LOOK_RAVEN_HIGH_CONTRAST_LIGHT }, { "Windows 11 dark", R_LOOK_WINDOWS_DARK }, { "Windows 11 light", R_LOOK_WINDOWS_LIGHT },
	{ "macOS dark", R_LOOK_MACOS_DARK }, { "macOS light", R_LOOK_MACOS_LIGHT }, { "macOS 26 glass dark", R_LOOK_MACOS_GLASS_DARK },
	{ "macOS 26 glass light", R_LOOK_MACOS_GLASS_LIGHT }, { "GNOME dark", R_LOOK_GNOME_DARK }, { "GNOME light", R_LOOK_GNOME_LIGHT },
	{ "The system's, dark", R_LOOK_NATIVE_DARK }, { "The system's, light", R_LOOK_NATIVE_LIGHT }
};
#define DESIGNER_LOOK_COUNT ((int)(sizeof(DESIGNER_LOOKS) / sizeof(DESIGNER_LOOKS[0])))

static void fill_appearance(void) {
	const char* chosen = prefs_get("appearance");
	int i, selected = 0, count = folder_theme_count();
	char item[160];
	g.syncing++;
	Rcombo_box_clear_items(g.appearance);
	for (i = 0; i < DESIGNER_LOOK_COUNT; i++) {
		Rcombo_box_add_item(g.appearance, DESIGNER_LOOKS[i].name);
		if (!strcmp(DESIGNER_LOOKS[i].name, chosen)) selected = i;
	}
	for (i = 0; i < count; i++) {
		snprintf(item, sizeof(item), "Theme: %s", folder_theme_name(i));
		Rcombo_box_add_item(g.appearance, item);
		if (!strcmp(item, chosen)) selected = DESIGNER_LOOK_COUNT + i;
	}
	Rcombo_box_set_selected(g.appearance, selected);
	g.syncing--;
}

/* The designer wears a look of Raven, or a theme of the folder ("Theme: Night"). */
void designer_set_appearance(const char* name) {
	int i;
	for (i = 0; i < DESIGNER_LOOK_COUNT; i++) {
		if (!strcmp(DESIGNER_LOOKS[i].name, name)) {
			Rlook_set(DESIGNER_LOOKS[i].look);
			break;
		}
	}
	if (i == DESIGNER_LOOK_COUNT && !strncmp(name, "Theme: ", 7) && !Rtheme_use(name + 7)) return;
	if (!g.quitting) prefs_set("appearance", name);
	fill_appearance();
}

static void on_appearance(Rid combo, int index, void* user_data) {
	(void)user_data;
	if (g.syncing || index < 0) return;
	designer_set_appearance(Rcombo_box_get_item_text(combo, index));
	set_status("The designer wears %s.", Rcombo_box_get_item_text(combo, index));
}

/* ---- the window itself, opened as the program will open it ---- */

/* The theme of the preview and the direction of its language, on what the window shows. */
static void run_dress(Rid widget) {
	if (widget == R_INVALID_ID || !Rwidget_is_valid(widget)) return;
	/* the window of the interface wears the font of Raven, as in the application (its island too) */
	Rwidget_set_font(widget, fonts_preview());
	if (g.run_window != R_INVALID_ID && Rwindow_get_island(g.run_window) != R_INVALID_ID) Rwidget_set_font(Rwindow_get_island(g.run_window), fonts_preview());
	Rwidget_set_theme(widget, g.preview_theme[0] ? g.preview_theme : NULL);
	Rwidget_set_direction(widget, Rlocale_get_language_count() > 0 && Rlocale_is_rtl() ? R_DIRECTION_RTL : R_DIRECTION_LTR);
}

/* Its title: the one of the XML, else the name of the file. */
static void run_title(void) {
	Rid root = g.document != R_INVALID_ID ? Rxml_document_get_root(g.document) : R_INVALID_ID;
	const char* title = root != R_INVALID_ID && g.run_is_window ? Rxml_node_get_attribute(root, "Title") : NULL;
	char text[300];
	if (title && title[0]) snprintf(text, sizeof(text), "%s", title[0] == '@' ? Rtr(title + 1) : title);
	else snprintf(text, sizeof(text), "%s", g.path ? ds_basename(g.path) : "Untitled");
	Rwindow_set_title(g.run_window, text);
}

/* The size of a device, when one is chosen above the preview. */
static void run_window_follow(void) {
	if (g.run_window == R_INVALID_ID || !Rwindow_is_valid(g.run_window)) return;
	if (g.device_width > 0) Rwindow_set_size(g.run_window, g.device_width, g.device_height);
	run_dress(Rwindow_get_content(g.run_window));
}

/*
 * The window follows the designer: each change builds what it shows again
 * (its content, and its island), in place, without moving it nor taking
 * the keyboard. Called before the preview is built: the nodes lead to the
 * widgets built last, those of the preview.
 */
static void rebuild_run_window(void) {
	Rid root, content, island = R_INVALID_ID;
	int i, count;
	if (g.run_window == R_INVALID_ID || !Rwindow_is_valid(g.run_window)) {
		g.run_window = R_INVALID_ID;
		return;
	}
	if (!g.has_document || g.document == R_INVALID_ID) return;
	root = Rxml_document_get_root(g.document);
	content = g.tab == TAB_WIDGET ? build_showcase() : Rxml_document_create_widget(g.document);
	if (content == R_INVALID_ID) content = Rwidget_create(R_WIDGET_VBOX);
	run_dress(content);
	Rwindow_set_content(g.run_window, content);
	/* the island of a <Window>, built from its own text */
	if (g.run_is_window) {
		count = Rxml_node_get_child_count(root);
		for (i = 0; i < count && island == R_INVALID_ID; i++) {
			Rid child = Rxml_node_get_child(root, i);
			if (!Rxml_node_is_comment(child) && same_text(Rxml_node_get_tag(child), "Island")) {
				const char* text = Rxml_node_to_string(child);
				island = Rwidget_create_from_xml(text, strlen(text));
			}
		}
		if (island != R_INVALID_ID || Rwindow_get_island(g.run_window) != R_INVALID_ID) Rwindow_set_island(g.run_window, island);
	}
	run_title();
}

/* F7: the window, as the program will open it (or back in front); it follows every change from then on. */
static void open_run_window(void) {
	Rid root;
	crash_report_note("open the window");
	if (!g.has_document) {
		set_status("Open an interface first.");
		return;
	}
	if (g.run_window != R_INVALID_ID && Rwindow_is_valid(g.run_window)) {
		Rwindow_show(g.run_window);
		set_status("The window is in front: it follows every change.");
		return;
	}
	root = Rxml_document_get_root(g.document);
	g.run_is_window = g.tab == TAB_WINDOW && same_text(Rxml_node_get_tag(root), "Window");
	if (g.run_is_window) {
		/* the window the XML describes: its size, its header, its island, its shortcuts */
		g.collecting = R_TRUE;
		g.run_window = Rxml_document_create_window(g.document);
		g.collecting = R_FALSE;
	} else {
		/* a piece of interface, or the tags of a library: in a plain window */
		Rid content = g.tab == TAB_WIDGET ? build_showcase() : Rxml_document_create_widget(g.document);
		g.run_window = Rwindow_create("Preview", 640, 480, 0);
		if (g.run_window != R_INVALID_ID && content != R_INVALID_ID) Rwindow_set_content(g.run_window, content);
	}
	if (g.run_window == R_INVALID_ID) {
		set_status("The window cannot be built: see the Problems tab.");
		return;
	}
	run_dress(Rwindow_get_content(g.run_window));
	run_title();
	if (g.device_width > 0) Rwindow_set_size(g.run_window, g.device_width, g.device_height);
	Rwindow_show(g.run_window);
	/* (the nodes lead to the preview again) */
	designer_refresh_preview();
	set_status("The window, as the program will show it: it follows every change of the designer.");
}

/* ---------------------------------------------------------------------------
 *  View
 * ------------------------------------------------------------------------- */

static void set_interact(int interact) {
	g.interact = interact;
	/* the frame of the selection stays, but lets the pointer through to the interface */
	Rwidget_set_hit_testable(g.outline, !interact);
	g.syncing++;
	Rmenu_item_set_checked(Rwidget_find(g.window, "menu.interact"), interact);
	g.syncing--;
	g.hovered = R_INVALID_ID;
	Rwidget_invalidate(g.outline);
	set_status(interact ? "The preview reacts as the interface will." : "A click in the preview selects an element.");
}

static void action_view(Rid sender, const char* action, void* user_data) {
	crash_report_note("action %s", action);
	(void)user_data;
	if (strncmp(action, "view.theme.", 11) == 0) {
		/* (the commands of the island, and the preferences of the designer before it had a list) */
		const char* look = action + 11;
		designer_set_appearance(!strcmp(look, "light") ? "Raven light" : !strcmp(look, "glass_dark") ? "Raven glass dark" :
			!strcmp(look, "glass_light") ? "Raven glass light" : "Raven dark");	} else if (strcmp(action, "view.interact") == 0) {
		set_interact(!g.interact);
	} else if (strcmp(action, "view.run") == 0) {
		if (g.tab == TAB_THEME) Raction_fire("te.window", R_INVALID_ID);
		else if (flush_source() || g.broken) open_run_window();
	} else if (strcmp(action, "view.reset") == 0) {
		Rdock_area_load_layout(g.dock, (const char*)Rwidget_get_user_handle(g.dock));
	} else if (strcmp(action, "view.strings") == 0) {
		designer_show_tab(TAB_STRINGS);
	}
}

static void action_problems(Rid sender, const char* action, void* user_data) {
	int index = Rlist_view_get_selected(g.problems_list);
	crash_report_note("action %s", action);
	(void)sender;
	(void)action;
	(void)user_data;
	if (index < 0 || index >= g.problem_count || g.problems[index].line <= 0) return;
	Rdock_area_show_panel(g.dock, g.source);
	Rtext_editor_set_cursor(g.source, g.problems[index].line - 1, 0);
	Rtext_editor_scroll_to_line(g.source, g.problems[index].line - 1);
	Rwidget_focus(g.source);
}

/* Actions of the interface being designed have nothing bound to them here: say what would happen. */
static void on_unbound_action(Rid sender, const char* action, void* user_data) {
	crash_report_note("action %s", action);
	(void)sender;
	(void)user_data;
	if (Raction_is_builtin(action)) set_status("The interface fired \"%s\", an action of Raven: the application does it by itself (not in the designer).", action);
	else set_status("The interface fired the action \"%s\".", action);
}

/* ---------------------------------------------------------------------------
 *  Scripted runs
 * ------------------------------------------------------------------------- */

static void run_frames(int count) {
	int i;
	for (i = 0; i < count && Ris_running(); i++) {
		Rwindow_request_redraw(g.window);
		Rdraw_frame();
	}
}

/*
 * Commands, separated by spaces:
 *   f N            N frames            do NAME        fires an action
 *   mv X Y         moves the pointer   click          left click
 *   key CODE MODS  a key               text WORD      types a word
 *   pick INDEX     selects a row of the palette
 *   path I/J/K     selects an element by its indices from the root ("" = the root)
 *   set NAME VALUE sets an attribute of the selection ('-' removes it)
 *   expect TEXT    stops with an error unless the XML contains TEXT ('~' stands for a space)
 *   open PATH      opens a folder, or an interface (with its folder)
 *   search TEXT    types in the search of the island (Ctrl+K) and runs the first result
 *   expectpanel NAME 0|1    stops unless the panel NAME is open (1) or hidden (0)
 *   wheel DY       turns the wheel
 *   expectfile PATH TEXT   stops unless the file contains TEXT
 *   write PATH TEXT        writes a file, as another program would ('~' for spaces, '|' for new lines)
 *   cell KEY COLUMN TEXT   writes a cell of the Translations panel
 *   language CODE  the language of the preview ('-': the first one)
 *   theme NAME     the theme of the preview ('-': the look of the designer)
 *   device INDEX   the device of the preview (0: fits the panel, 1: the size of the XML)
 *   expectlabel NAME TEXT  stops unless the label NAME of the preview shows TEXT
 *   expectsize W H stops unless the preview is W x H
 *   expecttheme NAME       stops unless the preview wears the theme NAME ('-': none)
 *   expectfiles N  stops unless the folder holds N interfaces
 *   expectthemes N stops unless the folder holds N themes
 *   expectselected TAG     stops unless the selection is a TAG
 *   focus NAME     gives the keyboard to a widget       expectkeys N   stops unless the list of @keys shows N keys
 *   expectnotch 0|1        stops unless the island is in the camera housing of the screen (1: RAVEN_FAKE_NOTCH on the null platform) or in the window
 *   expectcamera W H       stops unless the folded island is against the top, H high, with the room of a camera W wide in its middle
 *   expectkeyboard designer|island  stops unless that window has the keyboard
 *   expectnothing  stops unless nothing is selected
 *   expectrunwindow TITLE W H  stops unless the window opened (F7) is titled TITLE and is W x H
 *   clickaround    clicks the stage left of the frame      clickin    clicks inside the frame, near its corner
 *   expectfit      stops unless the frame takes the room of the panel
 *   expectgroup TAG GROUP  stops unless the palette shows TAG in GROUP ('~' for spaces)
 *   icons PATH     saves the pictures of the tags (.ppm)
 *   refresh        looks at the folder now
 *   tab NAME       shows a tab: window, widget, theme, strings     expecttab NAME
 *   dropon TAG PATH [before|after]   drops a widget of the palette on the element at PATH
 *   moveto PATH PATH       drags the element at the first path onto the one at the second
 *   expectrun NAME stops unless the window opened (F7) shows a widget named NAME
 *   fuzz STEPS SEED        random clicks, drags, keys, text and actions (the same seed replays the same run)
 *   size W H       the size of the window          capture PATH   saves the frame now (.ppm)
 *   defer S words --   the words run S seconds later, from a timer (inside a dialog that is open then)
 *   clickon NAME   clicks the middle of a widget   settext NAME TEXT   sets an input ('-': empty)
 *   expectpage welcome|workspace   expectstatus TEXT   expecttitle TEXT (the island)
 */
/* The next word of a script (strtok would not survive a script run from a timer inside a dialog). */
static char* next_token(char** cursor) {
	char* start = *cursor;
	char* end;
	if (!start) return NULL;
	while (*start == ' ') start++;
	if (!*start) {
		*cursor = NULL;
		return NULL;
	}
	end = strchr(start, ' ');
	if (end) {
		*end = 0;
		*cursor = end + 1;
	} else {
		*cursor = NULL;
	}
	return start;
}

static void capture(const char* path);
static int tree_row_span(Rid tree, Rid item, float* x, float* y0, float* y1);
static int designer_script(char* script);

/* xorshift: the same seed gives the same run, to replay a crash */
static unsigned g_fuzz_state = 1;
static int fuzz_int(int n) {
	g_fuzz_state ^= g_fuzz_state << 13;
	g_fuzz_state ^= g_fuzz_state >> 17;
	g_fuzz_state ^= g_fuzz_state << 5;
	return n > 0 ? (int)(g_fuzz_state % (unsigned)n) : 0;
}

/* What a person could do, at random: under a sanitizer, this finds what crashes. */
static void fuzz(int steps, unsigned seed) {
	static const RKey keys[] = { R_KEY_TAB, R_KEY_ENTER, R_KEY_ESCAPE, R_KEY_BACKSPACE, R_KEY_DELETE, R_KEY_LEFT, R_KEY_RIGHT,
		R_KEY_UP, R_KEY_DOWN, R_KEY_HOME, R_KEY_END, R_KEY_SPACE, R_KEY_A, R_KEY_Z, R_KEY_Y, R_KEY_D, R_KEY_K, R_KEY_C, R_KEY_V, R_KEY_F6, R_KEY_F7 };
	static const char* const texts[] = { "a", "<", "<Button/>", "\"", "/>", "Text=\"x\"", " ", "@hello", "\xC3\xA9", "</VBox>", "&amp;", "12" };
	static const char* const actions[] = { "edit.undo", "edit.redo", "edit.duplicate", "edit.delete", "edit.up", "edit.down", "edit.out", "edit.in",
		"edit.wrap.VBox", "edit.wrap.Border", "edit.unwrap", "edit.copy", "edit.paste", "palette.add", "view.interact", "view.run", "view.strings",
		"view.reset", "files.new.window", "files.new.panel", "files.new.form", "files.refresh", "file.save", "island.search", "file.welcome",
		"view.theme.dark", "view.theme.light", "strings.add_key", "strings.save", "tab.window", "tab.widget", "tab.theme", "tab.strings", "files.new.widgets", "te.reset.colors", "te.undo", "te.generate" };
	int step;
	g_fuzz_state = seed ? seed : 1;
	g.fuzzing = g_ds_fuzzing = R_TRUE;
	te_set_fuzzing(R_TRUE);
	for (step = 0; step < steps && Ris_running(); step++) {
		int what = fuzz_int(100);
		float x = (float)fuzz_int(1480), y = (float)fuzz_int(900);
		Rid window = g.window;
		if (g.run_window != R_INVALID_ID && Rwindow_is_valid(g.run_window) && fuzz_int(8) == 0) window = g.run_window;
		/* the island in the camera housing has a window of its own */
		else if (fuzz_int(6) == 0) {
			Rid island = Rwindow_get_island(g.window);
			Rid own = island != R_INVALID_ID ? Rwidget_get_window(island) : R_INVALID_ID;
			if (own != R_INVALID_ID && own != g.window) {
				window = own;
				x = (float)fuzz_int(640);
				y = (float)fuzz_int(700);
			}
		}
		if (what < 30) {
			/* (now and then with Ctrl or Shift held: a selection of several) */
			int held = fuzz_int(6);
			RKey modifier = held == 0 ? R_KEY_LEFT_CTRL : R_KEY_LEFT_SHIFT;
			if (held <= 1) Rwindow_inject_key(window, modifier, R_TRUE, held == 0 ? R_MOD_CTRL : R_MOD_SHIFT);
			Rwindow_inject_mouse_move(window, x, y);
			Rwindow_inject_mouse_button(window, fuzz_int(6) == 0 ? R_MOUSE_RIGHT : R_MOUSE_LEFT, R_TRUE);
			Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
			Rwindow_inject_mouse_button(window, R_MOUSE_RIGHT, R_FALSE);
			if (held <= 1) Rwindow_inject_key(window, modifier, R_FALSE, 0);
			if (fuzz_int(5) == 0) {
				Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
				Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
			}
		} else if (what < 42) {
			int k;
			Rwindow_inject_mouse_move(window, x, y);
			Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_TRUE);
			for (k = 0; k < 3; k++) {
				Rwindow_inject_mouse_move(window, x + (float)(fuzz_int(240) - 120), y + (float)(fuzz_int(160) - 80));
				Rdraw_frame();
			}
			Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, R_FALSE);
		} else if (what < 47) {
			Rwindow_inject_mouse_move(window, x, y);
			Rwindow_inject_mouse_wheel(window, 0.0f, (float)(fuzz_int(7) - 3));
		} else if (what < 60) {
			RKey key = keys[fuzz_int((int)(sizeof(keys) / sizeof(keys[0])))];
			Ruint mods = fuzz_int(4) == 0 ? R_MOD_CTRL : fuzz_int(6) == 0 ? R_MOD_SHIFT : 0;
			Rwindow_inject_key(window, key, R_TRUE, mods);
			Rwindow_inject_key(window, key, R_FALSE, mods);
		} else if (what < 72) {
			Rwindow_inject_text(window, texts[fuzz_int((int)(sizeof(texts) / sizeof(texts[0])))]);
		} else if (what < 86) {
			Raction_fire(actions[fuzz_int((int)(sizeof(actions) / sizeof(actions[0])))], R_INVALID_ID);
		} else if (what < 90) {
			int count = folder_interface_count();
			if (count) open_file(folder_interface_at(fuzz_int(count)));
		} else if (what < 93) {
			int count = Rcombo_box_get_item_count(g.theme_combo);
			if (count) {
				Rcombo_box_set_selected(g.theme_combo, fuzz_int(count));
				on_theme_changed(g.theme_combo, Rcombo_box_get_selected(g.theme_combo), NULL);
			}
		} else if (what < 95) {
			int index = fuzz_int(DEVICE_COUNT);
			Rcombo_box_set_selected(g.device_combo, index);
			on_device_changed(g.device_combo, index, NULL);
		} else if (what < 97) {
			if (g.has_document && g.document != R_INVALID_ID) {
				int path[3], depth = fuzz_int(4), k;
				for (k = 0; k < depth; k++) path[k] = fuzz_int(4);
				if (fuzz_int(2)) select_more(node_at_path(path, depth), fuzz_int(2));
				else select_node(node_at_path(path, depth), R_TRUE);
			}
		} else if (what < 98) {
			if (g.run_window != R_INVALID_ID && Rwindow_is_valid(g.run_window)) Rwindow_close(g.run_window);
		} else {
			/* the rest of what happens to a real window: a resize, files dropped on it, the folder changed by another program */
			int kind = fuzz_int(5);
			if (kind == 0) {
				Rwindow_set_size(window, 300 + fuzz_int(1400), 200 + fuzz_int(900));
			} else if (kind == 1 && g_folder) {
				char* path = ds_join(g_folder, fuzz_int(2) ? "main.xml" : "dropped.xml");
				const char* paths[1];
				paths[0] = path;
				Rwindow_inject_drag(window, x, y, R_TRUE);
				Rwindow_inject_drop(window, x, y, paths, 1, NULL);
				free(path);
			} else if (g_folder) {
				static const char* const names[] = { "main.xml", "strings.csv", "night.xml", "widgets.xml", "other.xml" };
				static const char* const bodies[] = {
					"<Window Width=\"320\" Height=\"240\"><VBox><Label Text=\"@hello\"/><Card/></VBox></Window>",
					"<Window><VBox><Label Text=\"broken",
					"key,en,fr\nhello,Hi,Salut\nbye,Bye,\n",
					"key,en\n\"open",
					"<Theme Name=\"FuzzNight\" Look=\"RavenLight\" Accent=\"#30D158\"/>",
					"<Theme Name=\"Other\" Look=\"Nope\"",
					"<Widgets><Widget Name=\"Card\"><Border Padding=\"4\"><Label Text=\"Changed\"/></Border></Widget></Widgets>",
					"<Widgets><Widget Name=\"Loop\"><Loop/></Widget></Widgets>",
					"" };
				char* path = ds_join(g_folder, names[fuzz_int(5)]);
				if (kind == 4 && fuzz_int(3) == 0) ds_remove_file(path);
				else {
					const char* body = bodies[fuzz_int((int)(sizeof(bodies) / sizeof(bodies[0])))];
					Rfile_write(path, body, strlen(body));
				}
				free(path);
			}
		}
		/* the pointer passing over things (the island unfolds on hover) */
		if (fuzz_int(4) == 0) Rwindow_inject_mouse_move(window, (float)fuzz_int(1480), (float)fuzz_int(900));
		/* now and then a real pause: the timers go off between two gestures, as they do for a person */
		if (fuzz_int(30) == 0) Rwait_events(0.05 + 0.4 * (double)fuzz_int(100) / 100.0);
		if (fuzz_int(3) == 0) Rdraw_frame();
		if (fuzz_int(40) == 0) {
			int k;
			/* (timers wait: the parse of the XML, the folder, the island) */
			for (k = 0; k < 30; k++) Rdraw_frame();
		}
	}
	g.fuzzing = g_ds_fuzzing = R_FALSE;
	te_set_fuzzing(R_FALSE);
}

static void on_deferred_script(Rid timer, void* user_data) {
	char* script = (char*)user_data;
	(void)timer;
	if (!designer_script(script)) fprintf(stderr, "raven_designer: the deferred script failed\n");
	free(script);
}

/* A widget found by name in the window (the island included). */
static Rid find_anywhere(const char* name) {
	return Rwidget_find(g.window, name);
}

static int widget_center(const char* name, float* x, float* y) {
	Rid widget = find_anywhere(name);
	float wx, wy, ww, wh;
	if (widget == R_INVALID_ID) return R_FALSE;
	Rwidget_get_rect(widget, &wx, &wy, &ww, &wh);
	*x = wx + ww * 0.5f;
	*y = wy + wh * 0.5f;
	return R_TRUE;
}

/* The window that has the keyboard, as the window system would send the keys: the designer, or the island in the camera housing. */
static Rid keyboard_window(void) {
	Rid island = Rwindow_get_island(g.window);
	Rid window = island != R_INVALID_ID ? Rwidget_get_window(island) : R_INVALID_ID;
	return window != R_INVALID_ID && window != g.window && Rwindow_is_active(window) ? window : g.window;
}

static int designer_script(char* script) {
	char* cursor = script;
	char* token = next_token(&cursor);
	while (token) {
		if (strcmp(token, "f") == 0) {
			run_frames(atoi(next_token(&cursor)));
		} else if (strcmp(token, "do") == 0) {
			Raction_fire(next_token(&cursor), R_INVALID_ID);
		} else if (strcmp(token, "mv") == 0) {
			float x = (float)atof(next_token(&cursor)), y = (float)atof(next_token(&cursor));
			Rwindow_inject_mouse_move(g.window, x, y);
		} else if (strcmp(token, "click") == 0) {
			Rwindow_inject_mouse_button(g.window, R_MOUSE_LEFT, R_TRUE);
			Rwindow_inject_mouse_button(g.window, R_MOUSE_LEFT, R_FALSE);
		} else if (strcmp(token, "key") == 0) {
			int key = atoi(next_token(&cursor)), modifiers = atoi(next_token(&cursor));
			Rwindow_inject_key(keyboard_window(), (RKey)key, R_TRUE, (Ruint)modifiers);
			/* (released with the modifiers: the next click is a plain one) */
			Rwindow_inject_key(keyboard_window(), (RKey)key, R_FALSE, 0);
		} else if (strcmp(token, "text") == 0) {
			Rwindow_inject_text(keyboard_window(), next_token(&cursor));
		} else if (strcmp(token, "pick") == 0) {
			int index = atoi(next_token(&cursor));
			if (index >= 0 && index < g.palette_count) Rtree_view_set_selected_item(g.palette, g.palette_items[index]);
		} else if (strcmp(token, "path") == 0) {
			char* text = next_token(&cursor);
			int path[32], depth = 0;
			while (text && *text && *text != '.' && depth < 32) {
				path[depth++] = atoi(text);
				text = strchr(text, '/');
				if (text) text++;
			}
			select_node(node_at_path(path, depth), R_TRUE);
		} else if (strcmp(token, "set") == 0) {
			const char* name = next_token(&cursor);
			const char* value = next_token(&cursor);
			Rxml_node_set_attribute(g.selected, name, strcmp(value, "-") == 0 ? NULL : value);
			document_changed(R_FALSE);
		} else if (strcmp(token, "expect") == 0) {
			char* text = next_token(&cursor);
			char* cursor;
			for (cursor = text; *cursor; cursor++) if (*cursor == '~') *cursor = ' ';
			if (!strstr(Rtext_editor_get_text(g.source), text)) {
				fprintf(stderr, "raven_designer: expected the XML to contain: %s\n---\n%s---\n", text, Rtext_editor_get_text(g.source));
				return R_FALSE;
			}
		} else if (strcmp(token, "treedrag") == 0) {
			/*
			 * treedrag FROM TO ZONE: a drag with the mouse that ends on the row of the Tree of the element at
			 * the path TO, at its top (before), middle (inside) or bottom (after). FROM is "tree:PATH" (a row
			 * of the Tree), "stage:PATH" (the element in the preview) or "palette:INDEX" (a widget of the palette).
			 */
			char* from = next_token(&cursor);
			char* to = next_token(&cursor);
			char* zone = next_token(&cursor);
			float sx = 0.0f, sy = 0.0f, ex, ey, y0, y1;
			Rid source_tree = g.tree, source_item = R_INVALID_ID;
			int k;
			run_frames(1);
			if (!strncmp(from, "palette:", 8)) {
				int index = atoi(from + 8);
				if (index < 0 || index >= g.palette_count) return R_FALSE;
				source_tree = g.palette;
				source_item = g.palette_items[index];
				Rdock_area_show_panel(g.dock, Rwidget_find(g.window, "panel.widgets"));
				run_frames(1);
				Rtree_view_scroll_to_item(g.palette, source_item);
				run_frames(1);
			} else {
				int path[32], depth = 0;
				char* walk = strchr(from, ':') ? strchr(from, ':') + 1 : from;
				while (walk && *walk && *walk != '.' && depth < 32) {
					path[depth++] = atoi(walk);
					walk = strchr(walk, '/');
					if (walk) walk++;
				}
				if (!strncmp(from, "stage:", 6)) {
					Rid widget = node_widget(node_at_path(path, depth));
					float wx, wy, ww, wh;
					if (widget == R_INVALID_ID) return R_FALSE;
					Rwidget_get_rect(widget, &wx, &wy, &ww, &wh);
					sx = wx + ww * 0.5f;
					sy = wy + wh * 0.5f;
				} else {
					source_item = tree_item_of(R_INVALID_ID, node_at_path(path, depth));
				}
			}
			if (source_item != R_INVALID_ID) {
				if (!tree_row_span(source_tree, source_item, &sx, &y0, &y1)) {
					fprintf(stderr, "raven_designer: treedrag: the row to drag is not shown\n");
					return R_FALSE;
				}
				sy = (y0 + y1) * 0.5f;
			}
			{
				int path[32], depth = 0;
				char* walk = to;
				while (walk && *walk && *walk != '.' && depth < 32) {
					path[depth++] = atoi(walk);
					walk = strchr(walk, '/');
					if (walk) walk++;
				}
				if (!tree_row_span(g.tree, tree_item_of(R_INVALID_ID, node_at_path(path, depth)), &ex, &y0, &y1)) {
					fprintf(stderr, "raven_designer: treedrag: the row to drop on is not shown\n");
					return R_FALSE;
				}
				ey = !strcmp(zone, "before") ? y0 + 2.0f : !strcmp(zone, "after") ? y1 - 2.0f : (y0 + y1) * 0.5f;
			}
			Rwindow_inject_mouse_move(g.window, sx, sy);
			Rwindow_inject_mouse_button(g.window, R_MOUSE_LEFT, R_TRUE);
			run_frames(1);
			for (k = 1; k <= 4; k++) {
				Rwindow_inject_mouse_move(g.window, sx + (ex - sx) * (float)k / 4.0f, sy + (ey - sy) * (float)k / 4.0f);
				run_frames(1);
			}
			Rwindow_inject_mouse_button(g.window, R_MOUSE_LEFT, R_FALSE);
			run_frames(1);
		} else if (strcmp(token, "expectnot") == 0) {
			/* expectnot TEXT: the XML does not contain TEXT ('~' for spaces) */
			char* text = next_token(&cursor);
			char* c;
			for (c = text; *c; c++) if (*c == '~') *c = ' ';
			if (strstr(Rtext_editor_get_text(g.source), text)) {
				fprintf(stderr, "raven_designer: expected the XML not to contain: %s\n---\n%s---\n", text, Rtext_editor_get_text(g.source));
				return R_FALSE;
			}
		} else if (strcmp(token, "expectpanel") == 0) {
			char* name = next_token(&cursor);
			int open = atoi(next_token(&cursor));
			if (Rdock_area_is_panel_open(g.dock, find_anywhere(name)) != open) {
				fprintf(stderr, "raven_designer: expected the panel %s to be %s\n", name, open ? "open" : "hidden");
				return R_FALSE;
			}
		} else if (strcmp(token, "wheel") == 0) {
			Rwindow_inject_mouse_wheel(g.window, 0.0f, (float)atof(next_token(&cursor)));
		} else if (strcmp(token, "open") == 0) {
			char* path = next_token(&cursor);
			if (ds_is_dir(path)) folder_open(path);
			else open_with_folder(path);
		} else if (strcmp(token, "search") == 0) {
			char* text = next_token(&cursor);
			char* c;
			for (c = text; *c; c++) if (*c == '~') *c = ' ';
			if (!island_run_search(text)) {
				fprintf(stderr, "raven_designer: the search of the island finds nothing for \"%s\"\n", text);
				return R_FALSE;
			}
		} else if (strcmp(token, "tab") == 0) {
			char* name = next_token(&cursor);
			int k;
			for (k = 0; k < 4; k++) if (!strcmp(name, TAB_ACTIONS[k] + 4)) Raction_fire(TAB_ACTIONS[k], R_INVALID_ID);
		} else if (strcmp(token, "expecttab") == 0) {
			char* name = next_token(&cursor);
			if (strcmp(name, TAB_ACTIONS[g.tab] + 4) != 0) {
				fprintf(stderr, "raven_designer: expected the tab %s, it is %s\n", name, TAB_ACTIONS[g.tab] + 4);
				return R_FALSE;
			}
		} else if (strcmp(token, "dropon") == 0 || strcmp(token, "moveto") == 0) {
			/* dropon TAG PATH [before|after]: a widget of the palette dropped on the element at PATH (its middle, or near an edge) */
			int moving = token[0] == 'm';
			char* what = next_token(&cursor);
			char* where = next_token(&cursor);
			char* side = cursor && (!strncmp(cursor, "before", 6) || !strncmp(cursor, "after", 5)) ? next_token(&cursor) : NULL;
			int path[32], depth = 0, from[32], from_depth = 0;
			Rid target, widget;
			float x, y, w, h;
			char* walk = where;
			while (walk && *walk && *walk != '.' && depth < 32) {
				path[depth++] = atoi(walk);
				walk = strchr(walk, '/');
				if (walk) walk++;
			}
			if (moving) {
				walk = what;
				while (walk && *walk && *walk != '.' && from_depth < 32) {
					from[from_depth++] = atoi(walk);
					walk = strchr(walk, '/');
					if (walk) walk++;
				}
			}
			run_frames(1);
			target = node_at_path(path, depth);
			widget = node_widget(target);
			if (widget == R_INVALID_ID) {
				fprintf(stderr, "raven_designer: nothing is shown for %s\n", where);
				return R_FALSE;
			}
			Rwidget_get_rect(widget, &x, &y, &w, &h);
			x += w * 0.5f;
			y += h * 0.5f;
			if (side && !strcmp(side, "before")) y -= h * 0.5f - 2.0f;
			if (side && !strcmp(side, "after")) y += h * 0.5f - 2.0f;
			if (moving ? !drop_node(node_at_path(from, from_depth), x, y) : !drop_widget(what, x, y)) {
				fprintf(stderr, "raven_designer: the drop on %s did nothing\n", where);
				return R_FALSE;
			}
		} else if (strcmp(token, "expectrun") == 0) {
			char* text = next_token(&cursor);
			char* c;
			for (c = text; *c; c++) if (*c == '~') *c = ' ';
			if (g.run_window == R_INVALID_ID || !Rwindow_is_valid(g.run_window) || Rwidget_find(g.run_window, text) == R_INVALID_ID) {
				fprintf(stderr, "raven_designer: the preview window shows no widget named %s\n", text);
				return R_FALSE;
			}
		} else if (strcmp(token, "fuzz") == 0) {
			int steps = atoi(next_token(&cursor));
			fuzz(steps, (unsigned)strtoul(next_token(&cursor), NULL, 10));
		} else if (strcmp(token, "write") == 0) {
			char* path = next_token(&cursor);
			char* text = next_token(&cursor);
			char* c;
			for (c = text; *c; c++) {
				if (*c == '~') *c = ' ';
				else if (*c == '|') *c = '\n';
			}
			Rfile_write(path, text, strlen(text));
		} else if (strcmp(token, "refresh") == 0) {
			folder_refresh();
		} else if (strcmp(token, "theme") == 0) {
			char* name = next_token(&cursor);
			designer_set_preview_theme(strcmp(name, "-") ? name : "");
		} else if (strcmp(token, "device") == 0) {
			int index = atoi(next_token(&cursor));
			Rcombo_box_set_selected(g.device_combo, index);
			on_device_changed(g.device_combo, index, NULL);
		} else if (strcmp(token, "expectsize") == 0) {
			float w = (float)atof(next_token(&cursor)), h = (float)atof(next_token(&cursor)), fx, fy, fw, fh;
			run_frames(1);
			Rwidget_get_rect(g.frame, &fx, &fy, &fw, &fh);
			if (fabsf(fw - w) > 0.5f || fabsf(fh - h) > 0.5f) {
				fprintf(stderr, "raven_designer: expected the preview to be %.0f x %.0f, it is %.0f x %.0f\n", w, h, fw, fh);
				return R_FALSE;
			}
		} else if (strcmp(token, "expecttheme") == 0) {
			char* name = next_token(&cursor);
			if (strcmp(g.preview_theme, strcmp(name, "-") ? name : "") != 0) {
				fprintf(stderr, "raven_designer: expected the preview to wear \"%s\", it wears \"%s\"\n", name, g.preview_theme);
				return R_FALSE;
			}
		} else if (strcmp(token, "expectfiles") == 0 || strcmp(token, "expectthemes") == 0) {
			int themes = token[6] == 't';
			int want = atoi(next_token(&cursor)), have = themes ? folder_theme_count() : folder_interface_count();
			if (want != have) {
				fprintf(stderr, "raven_designer: expected %d %s in the folder, there are %d\n", want, themes ? "themes" : "interfaces", have);
				return R_FALSE;
			}
		} else if (strcmp(token, "expectrunwindow") == 0) {
			/* expectrunwindow TITLE W H: the window opened is titled TITLE ('~' for spaces) and is W x H */
			char* title = next_token(&cursor);
			int w = atoi(next_token(&cursor)), h = atoi(next_token(&cursor)), rw = 0, rh = 0;
			char* c;
			for (c = title; *c; c++) if (*c == '~') *c = ' ';
			if (g.run_window == R_INVALID_ID || !Rwindow_is_valid(g.run_window)) {
				fprintf(stderr, "raven_designer: no window is open\n");
				return R_FALSE;
			}
			Rwindow_get_size(g.run_window, &rw, &rh);
			if (strcmp(Rwindow_get_title(g.run_window), title) != 0 || rw != w || rh != h) {
				fprintf(stderr, "raven_designer: expected the window \"%s\" %d x %d, it is \"%s\" %d x %d\n", title, w, h, Rwindow_get_title(g.run_window), rw, rh);
				return R_FALSE;
			}
		} else if (strcmp(token, "focus") == 0) {
			Rwidget_focus(find_anywhere(next_token(&cursor)));
		} else if (strcmp(token, "expectkeys") == 0) {
			/* expectkeys N: the list of keys under the input shows N keys (0: closed) */
			int want = atoi(next_token(&cursor));
			if (strings_key_picker_count() != want) {
				fprintf(stderr, "raven_designer: expected %d keys listed, there are %d\n", want, strings_key_picker_count());
				return R_FALSE;
			}
		} else if (strcmp(token, "expectnotch") == 0) {
			/* expectnotch 0|1: the island is in the camera housing of the screen (1), in the window (0) */
			int want = atoi(next_token(&cursor));
			Rid island = Rwindow_get_island(g.window);
			int in = island != R_INVALID_ID && Risland_is_in_notch(island);
			Rid window = island != R_INVALID_ID ? Rwidget_get_window(island) : R_INVALID_ID;
			run_frames(2);
			if (in != want || (want ? window == g.window : window != g.window)) {
				fprintf(stderr, "raven_designer: expected the island %s\n", want ? "in the camera housing" : "in the window");
				return R_FALSE;
			}
		} else if (strcmp(token, "expectcamera") == 0) {
			/* expectcamera W H: folded in the housing, the island is against the top of its window, H high, and the room of the camera (W wide, with its margins) is in its middle */
			float cw = (float)atof(next_token(&cursor)), chh = (float)atof(next_token(&cursor));
			Rid island = Rwindow_get_island(g.window), camera = find_anywhere("island.camera");
			float ix, iy, iw, ih, sx, sy, sw, sh, wx = 0.0f, wy = 0.0f;
			int ww = 0, wh = 0;
			run_frames(3);
			Rwidget_get_rect(island, &ix, &iy, &iw, &ih);
			Rwidget_get_rect(camera, &sx, &sy, &sw, &sh);
			Rwindow_get_size(Rwidget_get_window(island), &ww, &wh);
			(void)wx;
			(void)wy;
			if (fabsf(iy) > 0.5f || fabsf(ih - chh) > 0.5f || sw < cw || fabsf((sx + sw * 0.5f) - (ix + iw * 0.5f)) > 1.0f || fabsf((ix + iw * 0.5f) - ww * 0.5f) > 1.0f) {
				fprintf(stderr, "raven_designer: the island is at %.0f,%.0f %.0f x %.0f in a window %d x %d, the camera at %.0f %.0f wide\n", ix, iy, iw, ih, ww, wh, sx, sw);
				return R_FALSE;
			}
		} else if (strcmp(token, "expectkeyboard") == 0) {
			/* expectkeyboard designer|island: the window that has the keyboard */
			char* name = next_token(&cursor);
			int island = keyboard_window() != g.window;
			if (island != (strcmp(name, "island") == 0)) {
				fprintf(stderr, "raven_designer: expected the keyboard in the %s\n", name);
				return R_FALSE;
			}
		} else if (strcmp(token, "clickpath") == 0) {
			/* clickpath PATH MODIFIERS: a click in the middle of the widget of an element, with Shift (1) or Ctrl (2) held */
			char* text = next_token(&cursor);
			int modifiers = atoi(next_token(&cursor)), path[32], depth = 0;
			Rid widget;
			float wx, wy, ww, wh;
			while (text && *text && *text != '.' && depth < 32) {
				path[depth++] = atoi(text);
				text = strchr(text, '/');
				if (text) text++;
			}
			run_frames(1);
			widget = node_widget(node_at_path(path, depth));
			if (widget == R_INVALID_ID) {
				fprintf(stderr, "raven_designer: clickpath: the element has no widget\n");
				return R_FALSE;
			}
			Rwidget_get_rect(widget, &wx, &wy, &ww, &wh);
			if (modifiers) Rwindow_inject_key(g.window, (modifiers & R_MOD_CTRL) ? R_KEY_LEFT_CTRL : R_KEY_LEFT_SHIFT, R_TRUE, (Ruint)modifiers);
			Rwindow_inject_mouse_move(g.window, wx + ww * 0.5f, wy + wh * 0.5f);
			Rwindow_inject_mouse_button(g.window, R_MOUSE_LEFT, R_TRUE);
			Rwindow_inject_mouse_button(g.window, R_MOUSE_LEFT, R_FALSE);
			if (modifiers) Rwindow_inject_key(g.window, (modifiers & R_MOD_CTRL) ? R_KEY_LEFT_CTRL : R_KEY_LEFT_SHIFT, R_FALSE, 0);
			run_frames(1);
		} else if (strcmp(token, "expectcount") == 0) {
			/* expectcount N: how many elements are selected (in the preview and in the Tree) */
			int expected = atoi(next_token(&cursor)), count = selection_count(), in_tree = Rtree_view_get_selection_count(g.tree);
			if (count != expected || in_tree != expected) {
				fprintf(stderr, "raven_designer: expected %d selected, they are %d (%d in the Tree)\n", expected, count, in_tree);
				return R_FALSE;
			}
		} else if (strcmp(token, "expectnothing") == 0) {
			if (designer_get_selected() != R_INVALID_ID) {
				fprintf(stderr, "raven_designer: expected nothing selected, it is a %s\n", Rxml_node_get_tag(g.selected));
				return R_FALSE;
			}
		} else if (strcmp(token, "clickaround") == 0) {
			/* a click on the stage, a little left of the frame (in the middle of its height) */
			float fx, fy, fw, fh;
			run_frames(1);
			Rwidget_get_rect(g.frame, &fx, &fy, &fw, &fh);
			Rwindow_inject_mouse_move(g.window, fx - 10.0f, fy + fh * 0.5f);
			Rwindow_inject_mouse_button(g.window, R_MOUSE_LEFT, R_TRUE);
			Rwindow_inject_mouse_button(g.window, R_MOUSE_LEFT, R_FALSE);
			run_frames(1);
		} else if (strcmp(token, "clickin") == 0) {
			/* a click inside the frame, near its top left corner */
			float fx, fy, fw, fh;
			run_frames(1);
			Rwidget_get_rect(g.frame, &fx, &fy, &fw, &fh);
			Rwindow_inject_mouse_move(g.window, fx + 30.0f, fy + 30.0f);
			Rwindow_inject_mouse_button(g.window, R_MOUSE_LEFT, R_TRUE);
			Rwindow_inject_mouse_button(g.window, R_MOUSE_LEFT, R_FALSE);
			run_frames(1);
		} else if (strcmp(token, "expectfit") == 0) {
			/* the frame takes the room of the panel, its margin aside */
			float fx, fy, fw, fh, px, py, pw, ph;
			run_frames(2);
			Rwidget_get_rect(g.frame, &fx, &fy, &fw, &fh);
			Rwidget_get_rect(g.stage_fit, &px, &py, &pw, &ph);
			if (Rwidget_get_parent(g.stage) != g.stage_fit || fabsf(fw - (pw - 48.0f)) > 1.0f || fabsf(fh - (ph - 48.0f)) > 1.0f) {
				fprintf(stderr, "raven_designer: expected the preview to fit the panel (%.0f x %.0f), it is %.0f x %.0f\n", pw - 48.0f, ph - 48.0f, fw, fh);
				return R_FALSE;
			}
		} else if (strcmp(token, "expectgroup") == 0) {
			/* expectgroup TAG GROUP: the palette shows TAG in the group GROUP ('~' for spaces) */
			char* tag = next_token(&cursor);
			char* group = next_token(&cursor);
			char* c;
			int k, found = R_FALSE;
			for (c = group; *c; c++) if (*c == '~') *c = ' ';
			for (k = 0; k < g.palette_count && !found; k++) {
				Rid parent = Rtree_item_get_parent(g.palette_items[k]);
				found = same_text(g.palette_tags[k], tag) && parent != R_INVALID_ID && !strcmp(Rtree_item_get_text(parent), group);
			}
			if (!found) {
				fprintf(stderr, "raven_designer: expected %s in the group %s of the palette\n", tag, group);
				return R_FALSE;
			}
		} else if (strcmp(token, "icons") == 0) {
			if (!icons_save(next_token(&cursor))) return R_FALSE;
		} else if (strcmp(token, "expectselected") == 0) {
			char* tag = next_token(&cursor);
			Rid node = designer_get_selected();
			if (node == R_INVALID_ID || !same_text(Rxml_node_get_tag(node), tag)) {
				fprintf(stderr, "raven_designer: expected a %s selected, it is %s\n", tag, node != R_INVALID_ID ? Rxml_node_get_tag(node) : "nothing");
				return R_FALSE;
			}
		} else if (strcmp(token, "expectfile") == 0) {
			char* path = next_token(&cursor);
			char* text = next_token(&cursor);
			char* cursor;
			size_t size = 0;
			char* content = Rfile_read(path, &size);
			for (cursor = text; *cursor; cursor++) if (*cursor == '~') *cursor = ' ';
			if (!content || !strstr(content, text)) {
				fprintf(stderr, "raven_designer: expected %s to contain: %s\n---\n%s---\n", path, text, content ? content : "(cannot read it)");
				if (content) Rmemory_free(content);
				return R_FALSE;
			}
			Rmemory_free(content);
		} else if (strcmp(token, "cell") == 0) {
			char* key = next_token(&cursor);
			int column = atoi(next_token(&cursor));
			char* text = next_token(&cursor);
			char* cursor;
			for (cursor = text; *cursor; cursor++) if (*cursor == '~') *cursor = ' ';
			if (!strings_set_cell(key, column, text)) {
				fprintf(stderr, "raven_designer: no key %s in the table\n", key);
				return R_FALSE;
			}
		} else if (strcmp(token, "language") == 0) {
			char* code = next_token(&cursor);
			strings_set_preview_language(strcmp(code, "-") == 0 ? "" : code);
		} else if (strcmp(token, "expectlabel") == 0) {
			char* name = next_token(&cursor);
			char* text = next_token(&cursor);
			char* cursor;
			Rid label = g.preview != R_INVALID_ID ? Rwidget_find(g.preview, name) : R_INVALID_ID;
			for (cursor = text; *cursor; cursor++) if (*cursor == '~') *cursor = ' ';
			if (label == R_INVALID_ID || strcmp(Rlabel_get_text(label), text) != 0) {
				fprintf(stderr, "raven_designer: expected the label %s to show \"%s\", it shows \"%s\"\n", name, text,
					label != R_INVALID_ID ? Rlabel_get_text(label) : "(no such label)");
				return R_FALSE;
			}
		} else if (strcmp(token, "size") == 0) {
			int w = atoi(next_token(&cursor)), h = atoi(next_token(&cursor));
			Rwindow_set_size(g.window, w, h);
		} else if (strcmp(token, "capture") == 0) {
			run_frames(2);
			capture(next_token(&cursor));
		} else if (strcmp(token, "defer") == 0) {
			/* defer SECONDS words -- : the words run later, from a timer (a dialog may be open then) */
			double seconds = atof(next_token(&cursor));
			ds_buffer_t later = { 0 };
			char* word;
			while ((word = next_token(&cursor)) != NULL && strcmp(word, "--") != 0) {
				if (later.size) ds_buffer_add(&later, " ", 1);
				ds_buffer_add(&later, word, strlen(word));
			}
			Rtimer_create(seconds, R_FALSE, on_deferred_script, later.data ? later.data : ds_strdup(""));
		} else if (strcmp(token, "clickon") == 0) {
			float x, y;
			char* name = next_token(&cursor);
			if (!widget_center(name, &x, &y)) {
				fprintf(stderr, "raven_designer: no widget named %s\n", name);
				return R_FALSE;
			}
			Rwindow_inject_mouse_move(g.window, x, y);
			run_frames(1);
			Rwindow_inject_mouse_button(g.window, R_MOUSE_LEFT, R_TRUE);
			Rwindow_inject_mouse_button(g.window, R_MOUSE_LEFT, R_FALSE);
			run_frames(1);
		} else if (strcmp(token, "settext") == 0) {
			char* name = next_token(&cursor);
			char* text = next_token(&cursor);
			char* c;
			for (c = text; *c; c++) if (*c == '~') *c = ' ';
			Rtext_input_set_text(find_anywhere(name), strcmp(text, "-") ? text : "");
		} else if (strcmp(token, "expectpage") == 0) {
			char* page = next_token(&cursor);
			if ((strcmp(page, "welcome") == 0) != welcome_is_shown()) {
				fprintf(stderr, "raven_designer: expected the page %s\n", page);
				return R_FALSE;
			}
		} else if (strcmp(token, "expectstatus") == 0) {
			char* text = next_token(&cursor);
			char* c;
			for (c = text; *c; c++) if (*c == '~') *c = ' ';
			if (!strstr(Rlabel_get_text(g.status), text)) {
				fprintf(stderr, "raven_designer: expected the status to contain \"%s\", it is \"%s\"\n", text, Rlabel_get_text(g.status));
				return R_FALSE;
			}
		} else if (strcmp(token, "expecttitle") == 0) {
			char* text = next_token(&cursor);
			char* c;
			for (c = text; *c; c++) if (*c == '~') *c = ' ';
			island_refresh();
			if (!strstr(island_get_title(), text)) {
				fprintf(stderr, "raven_designer: expected the island to say \"%s\", it says \"%s\"\n", text, island_get_title());
				return R_FALSE;
			}
		} else {
			fprintf(stderr, "raven_designer: unknown script command '%s'\n", token);
			return R_FALSE;
		}
		token = next_token(&cursor);
	}
	return R_TRUE;
}

static void capture(const char* path) {
	int width = 0, height = 0, i;
	uint8_t* pixels = Rwindow_capture(g.window, &width, &height);
	FILE* file = pixels ? fopen(path, "wb") : NULL;
	if (file) {
		fprintf(file, "P6\n%d %d\n255\n", width, height);
		for (i = 0; i < width * height; i++) fwrite(pixels + i * 4, 1, 3, file);
		fclose(file);
	}
	Rpixels_free(pixels);
}

/* ---------------------------------------------------------------------------
 *  Completion in the XML panel: tags, attributes, @keys of the translations
 * ------------------------------------------------------------------------- */

/* The rows a tree item takes, in the window (for the scripts): x in the middle of the tree, y from y0 to y1. */
static int tree_row_span(Rid tree, Rid item, float* x, float* y0, float* y1) {
	float tx, ty, tw, th, y;
	int found = R_FALSE;
	if (item == R_INVALID_ID) return R_FALSE;
	Rwidget_get_rect(tree, &tx, &ty, &tw, &th);
	*x = tx + tw * 0.4f;
	for (y = 0.5f; y < th; y += 1.0f) {
		if (Rtree_view_get_item_at(tree, tw * 0.4f, y) == item) {
			if (!found) *y0 = ty + y;
			*y1 = ty + y;
			found = R_TRUE;
		} else if (found) {
			break;
		}
	}
	return found;
}

static int is_word(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

static void on_complete(Rid editor, const char* prefix, void* user_data) {
	char before[1024];
	int line, column, i, quotes = 0;
	const char* text;
	const char* word_start;
	const char* open;
	const char* close;
	const char* p;
	size_t lp = strlen(prefix), lb;
	(void)user_data;
	Rtext_editor_get_cursor(editor, &line, &column);
	text = Rtext_editor_get_line(editor, line);
	if (!text) return;
	if ((size_t)column >= sizeof(before)) column = (int)sizeof(before) - 1;
	if ((int)strlen(text) < column) column = (int)strlen(text);
	memcpy(before, text, (size_t)column);
	before[column] = 0;
	lb = strlen(before);
	if (lp > lb) return;
	word_start = before + lb - lp;
	open = strrchr(before, '<');
	close = strrchr(before, '>');
	/* a value that starts with @: a key of the tables */
	if (word_start > before && word_start[-1] == '@') {
		for (i = 0; i < strings_key_count(); i++) Rtext_editor_add_completion(editor, strings_key(i), "translation");
		return;
	}
	if (!open || (close && close > open)) return;
	/* the name of an element */
	if (word_start == open + 1 || (word_start == open + 2 && open[1] == '/')) {
		for (i = 0; i < Rxml_get_tag_count(); i++) Rtext_editor_add_completion(editor, Rxml_get_tag_name(i), "widget");
		for (i = 0; i < Rxml_get_registered_widget_count(); i++) Rtext_editor_add_completion(editor, Rxml_get_registered_widget_name(i), "declared");
		Rtext_editor_add_completion(editor, "Item", "row");
		Rtext_editor_add_completion(editor, "Shortcut", "keys");
		return;
	}
	/* inside a value: nothing to propose */
	for (p = open; p < word_start; p++) if (*p == '"') quotes++;
	if (quotes % 2) return;
	/* an attribute of that element; "Slot.Fi" proposes "Fill" after the dot */
	{
		char tag[96], group[64] = "";
		const char* t = open + 1;
		const char* gs;
		size_t n = 0;
		while (*t && is_word(*t) && n + 1 < sizeof(tag)) tag[n++] = *t++;
		tag[n] = 0;
		for (gs = word_start; gs > open && (is_word(gs[-1]) || gs[-1] == '.'); gs--) {}
		if (gs < word_start && word_start[-1] == '.') snprintf(group, sizeof(group), "%.*s", (int)(word_start - gs), gs);
		for (i = 0; i < Rxml_get_attribute_count(tag); i++) {
			RXmlAttributeInfo info;
			if (!Rxml_get_attribute_info(tag, i, &info)) continue;
			if (group[0]) {
				if (!strncmp(info.name, group, strlen(group))) Rtext_editor_add_completion(editor, info.name + strlen(group), info.group);
			} else {
				Rtext_editor_add_completion(editor, info.name, info.group);
			}
		}
	}
}

/* ---------------------------------------------------------------------------
 *  Start
 * ------------------------------------------------------------------------- */

static int build_interface(void) {
	static const char* const edits[] = {
		"edit.delete", "edit.duplicate", "edit.up", "edit.down", "edit.out", "edit.in", "edit.unwrap", "edit.cut", "edit.copy", "edit.paste", "edit.deselect",
		"edit.wrap.VBox", "edit.wrap.HBox", "edit.wrap.Border", "edit.wrap.ScrollBox", "edit.wrap.Overlay"
	};
	static const char* const files[] = { "folder.open", "file.open", "file.welcome", "file.save", "file.save_as", "file.save_all", "file.quit" };
	static const char* const views[] = {
		"view.theme.dark", "view.theme.light", "view.theme.glass_dark", "view.theme.glass_light", "view.interact",
		"view.run", "view.reset", "view.strings"
	};
	static const struct { const char* text; const char* action; } context[] = {
		{ "Duplicate", "edit.duplicate" }, { "Delete", "edit.delete" }, { NULL, NULL }, { "Move up", "edit.up" }, { "Move down", "edit.down" },
		{ "Move out", "edit.out" }, { NULL, NULL }, { "Wrap in a VBox", "edit.wrap.VBox" }, { "Wrap in an HBox", "edit.wrap.HBox" },
		{ "Wrap in a Border", "edit.wrap.Border" }, { "Unwrap", "edit.unwrap" }, { NULL, NULL }, { "Cut", "edit.cut" }, { "Copy (as XML)", "edit.copy" },
		{ "Paste inside", "edit.paste" }
	};
	size_t i;

	for (i = 0; i < sizeof(edits) / sizeof(edits[0]); i++) Raction_bind(edits[i], action_edit, NULL);
	for (i = 0; i < sizeof(files) / sizeof(files[0]); i++) Raction_bind(files[i], action_file, NULL);
	for (i = 0; i < sizeof(views) / sizeof(views[0]); i++) Raction_bind(views[i], action_view, NULL);
	Raction_bind("edit.undo", action_history, NULL);
	Raction_bind("edit.redo", action_history, NULL);
	Raction_bind("palette.add", action_palette_add, NULL);
	Raction_bind("palette.filter", action_palette_filter, NULL);
	Raction_bind("problems.go", action_problems, NULL);
	for (i = 0; i < 4; i++) Raction_bind(TAB_ACTIONS[i], action_tab, NULL);
	Raction_set_default_handler(on_unbound_action, NULL);

	{
		size_t length = 0, piece;
		char* xml;
		for (piece = 0; piece < sizeof(DESIGNER_XML) / sizeof(DESIGNER_XML[0]); piece++) length += strlen(DESIGNER_XML[piece]);
		xml = (char*)malloc(length + 1);
		xml[0] = '\0';
		for (piece = 0; piece < sizeof(DESIGNER_XML) / sizeof(DESIGNER_XML[0]); piece++) strcat(xml, DESIGNER_XML[piece]);
		g.window = Rwindow_create_from_xml(xml, length);
		free(xml);
	}
	if (g.window == R_INVALID_ID) return R_FALSE;
	g.dock = Rwidget_find(g.window, "dock");
	g.source = Rwidget_find(g.window, "source");
	g.tree = Rwidget_find(g.window, "tree");
	g.palette = Rwidget_find(g.window, "palette");
	g.palette_filter = Rwidget_find(g.window, "palette.filter");
	g.inspector = Rwidget_find(g.window, "inspector");
	g.problems_list = Rwidget_find(g.window, "problems");
	g.status = Rwidget_find(g.window, "status");
	g.path_label = Rwidget_find(g.window, "path");
	g.stage = Rwidget_find(g.window, "stage");
	g.stage_scroll = Rwidget_find(g.window, "stage.scroll");
	g.stage_fit = Rwidget_find(g.window, "stage.fit");
	g_ds_window = g.window;
	g_ds_dock = g.dock;
	/* the pictures, then the island first: the look of the designer, the theme and the language of the preview are chosen in it */
	icons_init();
	island_init();
	for (i = 0; i < 4; i++) {
		static const char* const names[4] = { "tab.window", "tab.widget", "tab.theme", "tab.strings" };
		g.tab_buttons[i] = Rwidget_find(g.window, names[i]);
	}
	g.appearance = Rwidget_find(g.window, "appearance");
	g.theme_list = Rwidget_find(g.window, "theme.list");
	Rcombo_box_set_changed_callback(g.appearance, on_appearance, NULL);
	Rlist_view_set_selection_changed_callback(g.theme_list, on_theme_list, NULL);
	Rwindow_set_event_hook(g.window, window_hook, NULL);
	/* Backspace deletes as Delete does (the menu shows Delete; a text being typed takes the key first) */
	Rwindow_register_shortcut(g.window, R_KEY_BACKSPACE, 0, "edit.delete");
	g.device_combo = Rwidget_find(g.window, "preview.device");
	g.theme_combo = Rwidget_find(g.window, "preview.theme");
	for (i = 0; i < (size_t)DEVICE_COUNT; i++) Rcombo_box_add_item(g.device_combo, DEVICES[i].name);
	Rcombo_box_set_selected(g.device_combo, 0);
	Rcombo_box_set_changed_callback(g.device_combo, on_device_changed, NULL);
	Rcombo_box_set_changed_callback(g.theme_combo, on_theme_changed, NULL);
	Rdock_area_show_panel(g.dock, g.source);
	Rdock_area_show_panel(g.dock, Rwidget_find(g.window, "panel.files"));
	/* the arrangement the panels start with, for "Reset the panels" */
	Rwidget_set_user_handle(g.dock, copy_text(Rdock_area_save_layout(g.dock)));

	/* the stage: a frame that stands for the window, and above it what draws the selection */
	g.frame = Rwidget_create(R_WIDGET_BORDER);
	Rwidget_set_attribute(g.frame, "CornerRadius", "10");
	Rwidget_set_attribute(g.frame, "BorderWidth", "1");
	Rwidget_set_attribute(g.frame, "Shadow", "26,0,10");
	Rwidget_set_clipping(g.frame, R_TRUE);
	/* the preview keeps the font of Raven: the designer's is its own */
	Rwidget_set_font(g.frame, fonts_preview());
	Rwidget_add_child(g.stage, g.frame);
	g.outline = Rwidget_create(R_WIDGET_CUSTOM);
	Rcustom_set_paint_callback(g.outline, outline_paint, NULL);
	Rwidget_set_event_callback(g.outline, outline_event, NULL);
	Rwidget_set_focusable(g.outline, R_TRUE);
	Rwidget_add_child(g.stage, g.outline);

	g.context_menu = Rwidget_create(R_WIDGET_MENU);
	for (i = 0; i < sizeof(context) / sizeof(context[0]); i++) {
		if (!context[i].text) Rmenu_add_separator(g.context_menu);
		else Rwidget_set_action(Rmenu_add_item(g.context_menu, context[i].text), context[i].action);
	}
	Rwidget_set_context_menu(g.tree, g.context_menu);

	Rtree_view_set_multi_select(g.tree, R_TRUE);
	Rtree_view_set_selection_changed_callback(g.tree, on_tree_selected, NULL);
	Rtree_view_set_activated_callback(g.palette, on_palette_activated, NULL);
	for (i = 0; i < 8; i++) g.palette_groups[i] = R_INVALID_ID;
	Rtext_editor_set_changed_callback(g.source, on_source_changed, NULL);
	Rtext_editor_set_tab_size(g.source, 4);
	Rwindow_set_close_callback(g.window, on_close, NULL);
	Rwindow_set_drop_callback(g.window, on_drop, NULL);

	g.row_cap = 512;
	g.rows = (row_t*)calloc((size_t)g.row_cap, sizeof(row_t));
	g.document = R_INVALID_ID;
	g.selected = R_INVALID_ID;
	g.hovered = R_INVALID_ID;
	g.preview = R_INVALID_ID;
	g.run_window = R_INVALID_ID;
	g.parse_timer = R_INVALID_ID;
	g.drag_node = R_INVALID_ID;
	g.drop_parent = R_INVALID_ID;
	g.fit_timer = R_INVALID_ID;
	g.tab = TAB_WINDOW;
	/* the window fits the panel, until another size is chosen */
	g.device = 0;
	g.device_width = DEVICES[0].width;
	g.device_height = DEVICES[0].height;

	/* what is around the visual editor */
	Rtext_editor_set_completion_callback(g.source, on_complete, NULL);
	strings_init();
	welcome_init();
	accessibility_init();
	folder_init();
	/* the Theme tab: the editor of themes, inside */
	g.theme_root = te_embed(g.window);
	if (g.theme_root != R_INVALID_ID) {
		Rslot_set_fill(g.theme_root, 1.0f);
		Rwidget_add_child(Rwidget_find(g.window, "theme.host"), g.theme_root);
	}
	fill_theme_combo();
	fill_appearance();
	/* the fonts: the titles, the code; the preview of the theme editor keeps the font of Raven */
	Rwidget_set_font(Rwidget_find(g.window, "preview.root"), fonts_preview());
	fonts_title_on(Rwidget_find(g.window, "inspector.title"));
	fonts_title_on(Rwidget_find(g.window, "stage.empty.title"));
	fonts_dress(Rwindow_get_content(g.window), g.frame);
	Rdock_area_show_panel(g.dock, g.source);
	return R_TRUE;
}

int main(int argc, char** argv) {
	RInitDesc desc;
	const char* file = NULL;
	const char* capture_path = NULL;
	char* script = NULL;
	int frames = -1, i, ok = R_TRUE;

	memset(&desc, 0, sizeof(desc));
	desc.backend = R_BACKEND_VULKAN;
	desc.app_name = "Raven Designer";
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--null") == 0) {
			desc.backend = R_BACKEND_NULL;
			desc.headless = R_TRUE;
		} else if (strcmp(argv[i], "--headless") == 0) {
			desc.headless = R_TRUE;
		} else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
			frames = atoi(argv[++i]);
		} else if (strcmp(argv[i], "--script") == 0 && i + 1 < argc) {
			script = copy_text(argv[++i]);
		} else if (strcmp(argv[i], "--capture") == 0 && i + 1 < argc) {
			capture_path = argv[++i];
		} else if (argv[i][0] == '-') {
			fprintf(stderr, "usage: raven_designer [folder | interface.xml] [--null | --headless] [--frames N] [--script \"...\"] [--capture file.ppm]\n");
			return 2;
		} else {
			file = argv[i];
		}
	}
	/* a crash leaves a report in the folder of the preferences (raven_designer-crash.txt) */
	if (!(frames >= 0 || script)) crash_report_install(ds_config_dir(), "raven_designer");
	Rerror_set_callback(on_error);
	if (!Rinit_ex(&desc)) {
		fprintf(stderr, "raven_designer: cannot start Raven (is a Vulkan driver installed?)\n");
		return 1;
	}
	/* scripted runs do not depend on how long a frame takes */
	if (frames >= 0 || script) {
		Rset_animation_time_step(1.0 / 60.0);
		g.quitting = R_TRUE;
	}
	/* the designer reads from left to right whatever the language of the preview (the preview has its own direction) */
	Rset_layout_direction(R_DIRECTION_LTR);
	/* the preview shows the interface, it does not run it: OnClicked="app.quit" must not quit the designer */
	Raction_set_builtins(R_FALSE);
	/* the fonts of the designer (embedded) */
	fonts_init();
	if (!build_interface()) {
		Rshutdown();
		return 1;
	}
	/* the appearance of the last time (not for the scripted runs: they start the same way everywhere) */
	if (!g.quitting && prefs_get("appearance")[0]) designer_set_appearance(prefs_get("appearance"));
	/* the accessibility settings of the last time (text size, contrast, motion...) */
	if (!g.quitting) accessibility_apply_saved();
	/* nothing is opened by itself: the start page, unless a folder or an interface was given */
	designer_close();
	if (file && ds_is_dir(file)) folder_open(file);
	else if (file) open_with_folder(file);
	else welcome_show();
	{
		/* the last run stopped on a crash: where its report is (raven_designer-crash.txt, and a .dmp on Windows) */
		const char* report = crash_report_previous();
		if (report) ds_status("The designer stopped on a crash last time. What happened is in %s: send it with the .dmp next to it.", report);
	}
	update_title();

	if (script) ok = designer_script(script);
	if (frames >= 0) {
		run_frames(frames);
		if (capture_path) capture(capture_path);
	} else if (!script) {
		while (Rwait_frame(-1.0)) {}
	}

	island_shutdown();
	te_shutdown();
	folder_shutdown();
	accessibility_shutdown();
	welcome_shutdown();
	strings_shutdown();
	history_clear();
	{
		int k, h;
		for (k = 0; k < 2; k++) {
			free(g.slots[k].path);
			free(g.slots[k].text);
			for (h = 0; h < g.slots[k].history_count; h++) free(g.slots[k].history[h]);
		}
	}
	for (i = 0; i < g.palette_count; i++) free(g.palette_tags[i]);
	free(g.palette_tags);
	free(g.palette_items);
	icons_shutdown();
	free(g.rows);
	free(g.path);
	free(script);
	if (Rwidget_is_valid(g.dock)) free(Rwidget_get_user_handle(g.dock));
	i = Rget_exit_code();
	Rshutdown();
	return ok ? i : 1;
}

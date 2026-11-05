/**
 * Raven - xml/xml_schema.c
 * What can be written in XML, as data: the tags and, for each, its
 * attributes with the kind of value they take (Rxml_get_tag_...,
 * Rxml_get_attribute_...). Tools use it to propose and to check; the loader
 * does not: an attribute exists because a class accepts it. The two are kept
 * together by tests/test_xml.c, which gives every attribute listed here to
 * the loader.
 */

#include "xml.h"
#include "../widgets/common.h"

typedef struct rvn_schema_attribute_t {
	const char* name;
	RXmlValueType type;
	const char* choices;
	const char* description;
} rvn_schema_attribute_t;

#define T(name, description) { name, R_XML_TEXT, NULL, description }
#define B(name, description) { name, R_XML_BOOL, NULL, description }
#define N(name, description) { name, R_XML_NUMBER, NULL, description }
#define C(name, choices, description) { name, R_XML_CHOICE, choices, description }
#define K(name, description) { name, R_XML_COLOR, NULL, description }
#define A(name, description) { name, R_XML_ACTION, NULL, description }
#define F(name, description) { name, R_XML_FILE, NULL, description }
#define V(name, description) { name, R_XML_NUMBERS, NULL, description }
#define L(name, description) { name, R_XML_LIST, NULL, description }

#define ORIENTATION "Horizontal|Vertical"
#define STRETCH "Fill|Fit|Cover|None"

/* ---- what every widget has ---- */

static const rvn_schema_attribute_t g_widget[] = {
	T("Name", "Name of the widget, for Rwidget_find."),
	N("Width", "Fixed width; a negative value lets the content decide."),
	N("Height", "Fixed height; a negative value lets the content decide."),
	N("MinWidth", "The width never goes below this."),
	N("MinHeight", "The height never goes below this."),
	N("MaxWidth", "The width never goes above this."),
	N("MaxHeight", "The height never goes above this."),
	C("Visibility", "Visible|Hidden|Collapsed", "Hidden keeps the place of the widget, Collapsed gives it back."),
	B("Enabled", "A disabled widget is dimmed and does not react."),
	N("Opacity", "From 0 (invisible) to 1."),
	C("Material", "Default|Solid|Glass|FrostedGlass|TintedGlass", "What the surface of the widget is made of; Default follows the theme."),
	T("Tooltip", "Text shown when the pointer rests on the widget."),
	C("Cursor", "Arrow|IBeam|Hand|Crosshair|ResizeH|ResizeV|ResizeNWSE|ResizeNESW|Move|NotAllowed|None", "Pointer shown over the widget."),
	C("Appear", "None|Fade|Pop|Zoom|FlyUp|FlyDown|FlyLeft|FlyRight|Drop|SlideLeft|SlideRight", "How the widget comes in when it is first shown."),
	N("AppearDelay", "Seconds before the widget comes in."),
	A("Action", "Name of the action the widget fires."),
	B("HitTestable", "false lets the pointer through to what is behind."),
	B("Focusable", "Can take the keyboard."),
	B("Clip", "Cuts what is inside at the bounds of the widget."),
	B("WindowDrag", "Dragging the widget moves the window (custom header)."),
	T("Class", "Style classes of the widget, separated by spaces: the styles of the theme that name one of them apply."),
	C("Theme", "Default|Dark|Light|GlassDark|GlassLight|HighContrastDark|HighContrastLight", "Theme of the widget and of what it contains: a name, or a theme file; Default follows what is around."),
	C("Direction", "Inherit|LTR|RTL", "Right to left: the children placed as in a mirror, the texts aligned on the right (raven_layout.h)."),
	T("AccessibleName", "What a screen reader says of the widget (its text by default)."),
	T("AccessibleDescription", "A longer explanation, read after the name."),
	C("AccessibleRole", "Auto|None|Group|Text|Heading|Link|Button|ToggleButton|CheckBox|RadioButton|Slider|ProgressBar|SpinButton|TextField|Document|ComboBox|List|ListItem|Tree|TreeItem|Table|MenuBar|Menu|MenuItem|TabList|Tab|TabPanel|Toolbar|StatusBar|Dialog|Alert|Image|Canvas|Separator|ScrollBar|Pane", "What the widget is for a screen reader."),
	B("AccessibleHidden", "Hidden from screen readers, with what it contains (decoration)."),
};

/* ---- the place of a widget in its parent ---- */

static const rvn_schema_attribute_t g_slot[] = {
	N("Slot.Fill", "Share of the free space of the panel the widget takes; 0 keeps its own size."),
	V("Slot.Padding", "Space around the widget: one number, \"x,y\" or \"left,top,right,bottom\"."),
	C("Slot.HAlign", "Fill|Left|Center|Right", "Where the widget sits across its place."),
	C("Slot.VAlign", "Fill|Top|Center|Bottom", "Where the widget sits down its place."),
	B("Slot.Auto", "Takes its own size (the opposite of Slot.Fill)."),
	N("Slot.MaxSize", "The most a filling widget takes along the panel."),
	N("Slot.Column", "Grid: column, from 0."),
	N("Slot.Row", "Grid: row, from 0."),
	N("Slot.ColumnSpan", "Grid: number of columns taken."),
	N("Slot.RowSpan", "Grid: number of rows taken."),
	V("Slot.Anchors", "Canvas: \"x,y\" or \"minx,miny,maxx,maxy\", from 0 to 1 across the canvas."),
	V("Slot.Position", "Canvas: \"x,y\" from the anchor."),
	V("Slot.Size", "Canvas: \"width,height\"."),
	V("Slot.Alignment", "Canvas: \"x,y\", the point of the widget placed at the position, from 0 to 1."),
	T("Slot.Title", "Title of the tab or of the docked panel that holds the widget."),
	C("Slot.Dock", "Left|Right|Top|Bottom|Tab", "DockArea: the side of the area, or of Slot.DockTo, the panel takes."),
	T("Slot.DockTo", "DockArea: name of the panel the panel is placed against."),
	N("Slot.DockSize", "DockArea: share of the place it is cut from, between 0 and 1."),
	B("Slot.Closable", "DockArea: the tab has a close button."),
};

/* ---- fingers, pens and trackpads on one widget (raven_touch.h) ---- */

static const rvn_schema_attribute_t g_touch[] = {
	C("Touch.Mode", "Auto|Scroll|Mouse|None", "What a finger does here: Auto scrolls what scrolls and is a mouse elsewhere, Scroll always scrolls, Mouse is a mouse (no scrolling), None ignores fingers (a surface for the pen)."),
	B("Touch.Gestures", "false: two fingers, on the screen or a trackpad, neither zoom nor pan the widget."),
	B("Pen.Mouse", "false: the pen is not a mouse here; only the pen callback of the application sees it."),
};

/* ---- fingers, pens and trackpads in a window ---- */

static const rvn_schema_attribute_t g_window_touch[] = {
	B("Touch.MouseEmulation", "A finger drives the widgets as the mouse does: a tap clicks, a drag drags (default true)."),
	N("Touch.Slop", "How far a finger moves before a press becomes a drag or a scroll (default 8)."),
	N("Touch.LongPress", "Seconds a finger rests before the press is a right click; 0 turns it off (default 0.5)."),
	B("Touch.Inertia", "Flicked content keeps going and slows down (default true)."),
	N("Touch.Deceleration", "How fast flicked content slows down, units per second per second (default 2400)."),
	B("Touch.Gestures", "Two fingers pinch to zoom and pan (default true)."),
	B("Touch.PalmRejection", "Fingers are ignored while a pen is near the screen (default true)."),
	B("Pen.MouseEmulation", "The pen drives the widgets as the mouse does; its barrel button is the right button (default true)."),
	N("Pen.PressureCurve", "The pressure is the one of the pen to this power: 1 as it is, below 1 softer, above 1 harder."),
	N("Pen.PressureThreshold", "Pressure, 0 to 1, the tip needs before it presses (default 0)."),
	B("Trackpad.Gestures", "The pinch and the rotation of a trackpad zoom what zooms, as two fingers on the screen do (default true)."),
	B("Trackpad.SwipeNavigation", "Three or more fingers swiped across a trackpad go back or forward, as the side buttons of a mouse (default true)."),
	N("Trackpad.SwipeDistance", "How far the fingers of a swipe go across before it navigates (default 120)."),
};

/* ---- colors and metrics of the theme, for one widget and what it contains ---- */

static const rvn_schema_attribute_t g_colors[] = {
	K("Color.Window", ""), K("Color.Panel", ""), K("Color.Border", ""), K("Color.Text", ""), K("Color.TextDisabled", ""),
	K("Color.TextPlaceholder", ""), K("Color.Accent", ""), K("Color.AccentHovered", ""), K("Color.AccentPressed", ""),
	K("Color.AccentText", ""), K("Color.Control", ""), K("Color.ControlHovered", ""), K("Color.ControlPressed", ""),
	K("Color.ControlBorder", ""), K("Color.Input", ""), K("Color.Focus", ""), K("Color.Selection", ""), K("Color.RowHovered", ""),
	K("Color.Track", ""), K("Color.Handle", ""), K("Color.HandleHovered", ""), K("Color.Separator", ""), K("Color.Header", ""),
	K("Color.HeaderHovered", ""), K("Color.TabActive", ""), K("Color.Popup", ""), K("Color.PopupBorder", ""), K("Color.Shadow", ""),
	K("Color.Tooltip", ""), K("Color.TooltipText", ""), K("Color.Switch", ""), K("Color.Success", ""), K("Color.Warning", ""),
	K("Color.Danger", ""), K("Color.Link", ""), K("Color.Code", ""),
};

static const rvn_schema_attribute_t g_metrics[] = {
	N("Metric.FontSize", ""), N("Metric.CornerRadius", ""), N("Metric.BorderWidth", ""), N("Metric.ControlHeight", ""),
	N("Metric.PaddingX", ""), N("Metric.PaddingY", ""), N("Metric.Spacing", ""), N("Metric.ScrollbarSize", ""),
	N("Metric.RowHeight", ""), N("Metric.Indent", ""), N("Metric.SplitterSize", ""), N("Metric.CheckSize", ""),
	N("Metric.SliderHandleSize", ""), N("Metric.PopupRadius", ""), N("Metric.GlassFrost", ""), N("Metric.GlassRefraction", ""),
	N("Metric.GlassSaturation", ""), N("Metric.GlassRim", ""), N("Metric.GlassShadow", ""), N("Metric.BlurRadius", ""),
};

/* ---- tags ---- */

static const rvn_schema_attribute_t g_window[] = {
	T("Title", "Title of the window."),
	N("Width", "Width of the content, in logical units."),
	N("Height", "Height of the content."),
	N("MinWidth", "The user cannot make the window narrower."),
	N("MinHeight", "The user cannot make the window shorter."),
	B("Resizable", "false fixes the size."),
	B("CustomHeader", "No title bar from the system: a HeaderBar of the interface replaces it."),
	B("Borderless", "No frame at all."),
	B("Maximized", "Starts maximized."),
	B("Hidden", "Starts hidden, until Rwindow_show."),
	B("IsSplashScreen", "A splash screen: no frame, a fixed size, centered on the screen, above the other windows, out of the taskbar."),
	K("ClearColor", "Color behind the content."),
	F("Background", "Image behind the content (what glass refracts)."),
	B("Transparent", "The window can be seen through where nothing is drawn (needs a compositing window system)."),
	C("Backdrop", "None|Transparent|Blur|Acrylic|Mica", "What the system shows behind a window that is seen through: the desktop as it is, blurred, or a material of Windows 11."),
	C("Theme", "Default|Dark|Light|GlassDark|GlassLight|HighContrastDark|HighContrastLight", "Theme of this window only (its content, island, menus and background): a name, or a theme file; the other windows keep the one of the application. Default: the one of the application."),
};

static const rvn_schema_attribute_t g_box[] = { N("Spacing", "Space between the children.") };
static const rvn_schema_attribute_t g_grid[] = {
	T("Columns", "Share of the width of each column: \"0,1,2\" (0 = the size of its content)."),
	T("Rows", "Share of the height of each row."),
	N("Spacing", "Space between the cells."),
};
static const rvn_schema_attribute_t g_uniform_grid[] = {
	N("Columns", "Number of columns; 0 fits as many as the width allows."),
	V("MinCellSize", "\"width,height\" every cell has at least."),
	N("Spacing", "Space between the cells."),
};
static const rvn_schema_attribute_t g_border[] = {
	V("Padding", "Space between the border and its content."),
	K("Background", "Color of the surface."),
	K("BorderColor", "Color of the outline."),
	N("BorderWidth", "Width of the outline."),
	V("CornerRadius", "One number, or \"top-left,top-right,bottom-right,bottom-left\"."),
	V("Shadow", "\"blur\" or \"blur,x,y\"."),
	T("Effect", "An effect as its background: aurora, shimmer, neon, ripple, plasma, grain, spotlight, mesh, or a .spv file (raven_shader.h)."),
	K("EffectColor", "First color of the effect."),
	K("EffectColor2", "Second color of the effect."),
	V("EffectParams", "The four parameters of the effect: \"a,b,c,d\" (fewer keep the defaults of the others)."),
	B("EffectAnimate", "Draws frames while it is shown (the animated effects do by default)."),
	B("EffectPointer", "The effect follows the pointer: where it is, over the border and pressing it (true by default)."),
	B("EffectBackdrop", "The .spv effect reads what is behind it (R_SHADER_BACKDROP)."),
};
static const rvn_schema_attribute_t g_size_box[] = { V("AspectRatio", "Width over height: \"1.5\" or \"16,9\".") };
static const rvn_schema_attribute_t g_scroll_box[] = {
	C("Orientation", ORIENTATION, "The way the content scrolls and is stacked."),
	B("ScrollBoth", "Scrolls both ways."),
	N("Spacing", "Space between the children."),
};
static const rvn_schema_attribute_t g_splitter[] = {
	C("Orientation", ORIENTATION, "Horizontal puts the panes side by side."),
	N("MinPaneSize", "The smallest a pane can be dragged to."),
};
static const rvn_schema_attribute_t g_switcher[] = { N("Index", "Which child is shown, from 0.") };
static const rvn_schema_attribute_t g_tab_view[] = { N("ActiveTab", "Which tab is shown, from 0."), A("OnChanged", "Action fired when the tab changes.") };
static const rvn_schema_attribute_t g_expander[] = {
	T("Title", "Text of the header."),
	B("Expanded", "The content is shown."),
	A("OnChanged", "Action fired when it opens or closes."),
};
static const rvn_schema_attribute_t g_label[] = {
	T("Text", "The text."),
	N("TextSize", "Size of the text; 0 uses the theme."),
	C("TextAlign", "Left|Center|Right", "Where the lines sit across the label."),
	B("Wrap", "Long lines continue on the next one."),
	K("Color", "Color of the text."),
	F("Font", "Font file, or \"monospace\"."),
	B("Markdown", "The text is Markdown: headings, emphasis, lists, quotes, code, tables, links."),
	B("Selectable", "The text can be selected with the mouse and copied, as on a web page."),
};
static const rvn_schema_attribute_t g_button[] = {
	T("Text", "The text of the button."),
	C("Style", "Default|Primary|Flat", "Primary is the button of the main action; Flat has no surface until hovered."),
	A("OnClicked", "Action fired by a click."),
};
static const rvn_schema_attribute_t g_checkbox[] = {
	T("Text", "The text beside the box."),
	B("Checked", "Its state."),
	C("Style", "Check|Radio|Switch", "What it looks like."),
	A("OnChanged", "Action fired when the state changes."),
};
static const rvn_schema_attribute_t g_slider[] = {
	N("Min", "Lowest value."),
	N("Max", "Highest value."),
	N("Value", "The value."),
	N("Step", "The value moves by this much; 0 moves freely."),
	C("Orientation", ORIENTATION, "The way the slider lies."),
	A("OnChanged", "Action fired when the value changes."),
};
static const rvn_schema_attribute_t g_progress_bar[] = {
	N("Value", "How far, from 0 to 1."),
};
static const rvn_schema_attribute_t g_separator[] = { C("Orientation", ORIENTATION, "Horizontal is a line across.") };
static const rvn_schema_attribute_t g_image[] = {
	F("Source", "Image file."),
	C("Stretch", STRETCH, "How the image fills its place."),
	K("Tint", "Color the image is multiplied by."),
	N("CornerRadius", "Rounds the corners."),
	V("Region", "Part of the image shown: \"minu,minv,maxu,maxv\" from 0 to 1."),
	B("TintText", "Tinted with the color of the text around it: white icons that follow the theme."),
};
static const rvn_schema_attribute_t g_text_input[] = {
	T("Text", "The text."),
	T("Placeholder", "Shown, dimmed, while the input is empty."),
	B("Password", "Shows dots."),
	B("ReadOnly", "Can be selected and copied, not changed."),
	N("MaxLength", "Most characters accepted; 0 for no limit."),
	A("OnChanged", "Action fired at each change."),
	A("OnCommitted", "Action fired by Enter."),
};
static const rvn_schema_attribute_t g_spin_box[] = {
	N("Min", "Lowest value."),
	N("Max", "Highest value."),
	N("Value", "The value."),
	N("Step", "What the arrows add."),
	N("Decimals", "Digits shown after the point."),
	A("OnChanged", "Action fired when the value changes."),
};
static const rvn_schema_attribute_t g_combo_box[] = {
	L("Items", "The choices, separated by '|'."),
	N("Selected", "Which one is chosen, from 0; -1 for none."),
	A("OnChanged", "Action fired when the choice changes."),
};
static const rvn_schema_attribute_t g_list_view[] = {
	L("Items", "The rows, separated by '|'."),
	N("Selected", "Which row is selected, from 0; -1 for none."),
	A("OnChanged", "Action fired when the selection changes."),
	A("OnActivated", "Action fired by a double click or Enter."),
};
static const rvn_schema_attribute_t g_table[] = {
	L("Columns", "The headers of the columns, separated by '|' (or Column elements: Header, Width, Fill, Sortable, Align)."),
	N("Selected", "Which row is selected, from 0; -1 for none."),
	N("SortColumn", "The column the rows are sorted by, from 0; -1 for none."),
	B("SortDescending", "Sorted from the largest to the smallest."),
	B("ShowHeader", "Shows the row of the headers."),
	B("GridLines", "Lines between the rows and the columns."),
	B("Striped", "Every other row a little darker."),
	A("OnActivated", "Action fired by a double click or Enter."),
};
static const rvn_schema_attribute_t g_date_picker[] = {
	T("Date", "The date, \"YYYY-MM-DD\" or Today."),
	T("Min", "The earliest date that can be chosen, \"YYYY-MM-DD\"."),
	T("Max", "The latest date that can be chosen, \"YYYY-MM-DD\"."),
	C("FirstDayOfWeek", "Sunday|Monday|Saturday", "The day the weeks of the calendar start with (or 0 to 6)."),
	T("Format", "How the date is written: YYYY, MM, M, DD, D, MMMM, MMM, dddd, ddd; default YYYY-MM-DD."),
	A("OnChanged", "Action fired when the user chooses a date."),
};
static const rvn_schema_attribute_t g_time_picker[] = {
	T("Time", "The time, \"HH:MM\" or \"HH:MM:SS\"."),
	B("ShowSeconds", "Shows the seconds."),
	N("MinuteStep", "Step of the minutes for the arrows and the wheel."),
	A("OnChanged", "Action fired when the user changes the time."),
};
static const rvn_schema_attribute_t g_toast[] = {
	T("Text", "A message shown when the window opens."),
	N("Duration", "Seconds a message written here stays; 0: until hidden."),
	C("Kind", "Info|Success|Warning|Error", "The color of the dot before the message."),
	T("ActionText", "A button at the end of the message (\"Undo\")."),
	A("OnClicked", "Action fired by the button of the message."),
};
static const rvn_schema_attribute_t g_breadcrumb[] = {
	L("Items", "The items, separated by '|' (or Item elements); the last one is the current place."),
	A("OnClicked", "Action fired when the user clicks an item."),
};
static const rvn_schema_attribute_t g_rating[] = {
	N("Value", "The score, from 0 to Max."),
	N("Max", "Number of stars."),
	B("AllowHalf", "Half stars can be chosen."),
	B("ReadOnly", "Shown only."),
	N("StarSize", "Size of a star."),
	A("OnChanged", "Action fired when the user changes the score."),
};
static const rvn_schema_attribute_t g_pagination[] = {
	N("PageCount", "Number of pages."),
	N("Page", "The current page, from 0."),
	N("Siblings", "Page numbers shown on each side of the current one."),
	A("OnChanged", "Action fired when the user goes to another page."),
};
static const rvn_schema_attribute_t g_tree_view[] = {
	A("OnChanged", "Action fired when the selection changes."),
	A("OnActivated", "Action fired by a double click or Enter."),
};
static const rvn_schema_attribute_t g_tile_view[] = {
	N("TileWidth", "Width of a tile."),
	C("Layout", "Tiles|Rows", "Tiles: a grid. Rows: one item per line, as wide as the view, the icon on the left."),
	N("TileHeight", "Height of a tile, name included."),
	N("IconSize", "Size of the icon of a tile."),
	N("Spacing", "Space between the tiles."),
	T("EmptyText", "Text shown when there is no tile."),
	T("DragType", "Type given to Rdrag_begin when tiles are dragged; empty: no drag."),
	A("OnActivated", "Action fired by a double click or Enter."),
};
static const rvn_schema_attribute_t g_menu_bar[] = {
	B("Native", "false keeps the bar in the window on macOS too, instead of the menu bar of the system."),
};
static const rvn_schema_attribute_t g_menu[] = { T("Title", "Text of the menu in its bar, or in the menu that holds it.") };
static const rvn_schema_attribute_t g_menu_item[] = {
	T("Text", "Text of the item."),
	T("Shortcut", "Keys shown beside it and that fire it: \"Ctrl+S\"."),
	B("Checkable", "Clicking it checks and unchecks it."),
	B("Checked", "Its state."),
	B("Separator", "A line instead of an item."),
	A("OnClicked", "Action fired by the item."),
};
static const rvn_schema_attribute_t g_viewport[] = {
	C("Stretch", STRETCH, "How the picture fills its place."),
	B("FlipY", "Turns the picture upside down (some renderers draw that way)."),
	B("Realtime", "Drawn again at every frame."),
	F("Source", "Image shown until the application draws."),
};
static const rvn_schema_attribute_t g_text_editor[] = {
	T("Text", "The text."),
	C("Language", "None|C|C++|C#|Java|JavaScript|TypeScript|Python|Rust|GLSL|JSON|CSS|XML|HTML", "Colors the text and helps while typing."),
	B("LineNumbers", "Shows the line numbers in a margin."),
	B("Wrap", "Long lines continue on the next row."),
	B("ReadOnly", "Can be scrolled, selected and copied, not changed."),
	T("Placeholder", "Shown, dimmed, while the editor is empty."),
	N("TabSize", "Width of a tab, in spaces."),
	B("TabSpaces", "The Tab key writes spaces."),
	B("Completion", "Proposes words while typing."),
	B("AutoPairs", "Closes brackets and quotes, indents after them."),
	N("TextSize", "Size of the text; 0 uses the theme."),
	N("Zoom", "Zoom of the text, 1 = 100 % (0.5 .. 4): Ctrl + wheel changes it."),
	B("Zoomable", "Ctrl + wheel, Ctrl + '+' / '-' / 0 zoom the text (on by default)."),
	F("Font", "Font file, or \"monospace\"."),
	B("Markdown", "The text is Markdown, styled where it is typed (the same as Language=\"Markdown\")."),
	A("OnChanged", "Action fired at each change."),
};
static const rvn_schema_attribute_t g_header_bar[] = {
	T("Title", "Text of the bar."),
	C("Buttons", "All|None|Close|Close,Minimize|Close,Maximize", "The window buttons the bar shows."),
	C("ButtonSide", "Right|Left", "Where the window buttons are (default: the side of the system, whatever the look)."),
	C("TitleAlign", "Center|Left", "Where the title is."),
	B("Background", "false leaves the bar without a surface, over what is behind."),
};
static const rvn_schema_attribute_t g_file_browser[] = {
	T("Directory", "Folder shown first."),
	C("Mode", "Open|Save|Folder", "What is being chosen."),
	T("Filter", "Patterns of the files shown: \"*.png;*.jpg\"."),
	T("FileName", "Save: the name proposed."),
	T("DefaultExtension", "Save: added to a name typed without one."),
	B("ShowHidden", "Shows the hidden files."),
	B("Multiple", "Open: several files can be chosen."),
	B("Controls", "false hides the name, the filter and the buttons."),
	B("Places", "false hides the list of places."),
};
static const rvn_schema_attribute_t g_dock_area[] = {
	B("Movable", "false keeps the panels where they are."),
	T("Layout", "An arrangement saved with Rdock_area_save_layout."),
	B("CloseHides", "The close button of a tab hides its panel (Rdock_area_open_panel brings it back) instead of deleting it."),
	B("Tearable", "false: a tab dragged out of the area does not take its panel to a window of its own."),
};
static const rvn_schema_attribute_t g_island[] = {
	B("Expanded", "Unfolded."),
	B("ExpandOnHover", "Unfolds while the pointer is over it."),
	B("ExpandOnClick", "A click unfolds it and folds it back."),
	B("Attached", "Hangs from the top edge, like a notch, instead of floating below it."),
	B("Notch", "On a screen with a camera housing (a MacBook), lives in it, outside of the window: it stays there when the window moves. Elsewhere it stays in the window."),
};
static const rvn_schema_attribute_t g_background_blur[] = {
	C("Style", "Plain|Frosted|Glass|Native", "Plain only blurs; Frosted adds a veil and grain; Glass bends light at its rim; Native shows the desktop, blurred by the system, through a window that is seen through."),
	N("Radius", "How far the blur spreads; 0 uses the theme."),
	K("Tint", "Color laid over what is blurred; its alpha is its strength."),
	N("Saturation", "Colors of what is behind: 1 unchanged, 0 gray, more than 1 vivid."),
	N("Grain", "Fine noise, from 0 to 1 (what keeps a blur from banding)."),
	N("Shadow", "Blur of the shadow under the panel; 0 = none."),
	V("Padding", "Space around the child."),
	N("CornerRadius", "Radius of the corners."),
};
static const rvn_schema_attribute_t g_color_picker[] = {
	K("Color", "The color."),
	K("Previous", "The color shown next to it, to compare."),
	B("Alpha", "Shows the opacity strip; false makes colors opaque."),
	B("Popup", "A swatch the size of a button, that opens the picker when clicked."),
	B("Compact", "Only the square, the hues and the opacity."),
	C("Numbers", "RGB|HSV", "The numbers shown under the square."),
	T("Swatches", "Colors proposed under the picker: \"#FF453A, #0A84FF, white\"."),
	A("OnChanged", "Action fired when the user changes the color."),
};
static const rvn_schema_attribute_t g_timeline[] = {
	N("Duration", "Length of the sequence, in seconds."),
	N("Time", "Where the playhead is, in seconds."),
	N("FrameRate", "Frames per second: times snap to frames and show as seconds:frames; 0 = none."),
	B("Snapping", "Items snap to frames, to the playhead and to each other."),
	N("ViewStart", "First second shown."),
	N("ViewEnd", "Last second shown."),
	B("Loop", "Playing starts again at the end."),
	B("Playing", "The playhead runs."),
	N("HeaderWidth", "Width of the names of the tracks; 0 fits the longest."),
	N("TrackHeight", "Height of the tracks that have none of their own; 0 follows the theme."),
	N("RulerHeight", "Height of the ruler of time; 0 follows the theme."),
	A("OnChanged", "Action fired when the user moves, stretches or removes something."),
};
static const rvn_schema_attribute_t g_block_editor[] = {
	N("Zoom", "Size of the canvas: 1 = life size."),
	B("Palette", "The column of blocks to drag from, on the left."),
	A("OnChanged", "Action fired when the user snaps, moves, edits or deletes blocks."),
};
static const rvn_schema_attribute_t g_node_graph[] = {
	N("Zoom", "Size of the canvas: 1 = life size."),
	N("CenterX", "What the middle of the widget shows, across the canvas."),
	N("CenterY", "What the middle of the widget shows, down the canvas."),
	B("Grid", "The dots of the canvas."),
	C("Flow", "Auto|true|false", "The pulses that run along the links: while the pointer is over the graph, always, or never."),
	B("ReadOnly", "The user can look, select and move the view, but not change the graph."),
	B("FrameAll", "Starts with every node in view."),
	A("OnChanged", "Action fired when the user links, unlinks, moves or removes something."),
};

typedef struct rvn_schema_tag_t {
	const char* tag;
	const rvn_schema_attribute_t* attributes;
	int count;
	const char* description;
} rvn_schema_tag_t;

#define TAG(tag, attributes, description) { tag, attributes, (int)RVN_COUNT_OF(attributes), description }
#define TAG_PLAIN(tag, description) { tag, NULL, 0, description }

static const rvn_schema_tag_t g_tags[] = {
	TAG("Window", g_window, "A window; it contains one element, and optionally an Island and Shortcut elements."),
	TAG("VBox", g_box, "Stacks its children from top to bottom."),
	TAG("HBox", g_box, "Lines its children up from left to right."),
	TAG("Grid", g_grid, "Places its children in rows and columns (Slot.Column, Slot.Row)."),
	TAG("UniformGrid", g_uniform_grid, "Cells of the same size, filled in order."),
	TAG_PLAIN("Overlay", "Puts its children on top of each other."),
	TAG("Border", g_border, "A surface with padding around one child."),
	TAG("SizeBox", g_size_box, "Gives one child a size or a shape."),
	TAG_PLAIN("Spacer", "Empty space."),
	TAG("ScrollBox", g_scroll_box, "Scrolls content that is larger than its place."),
	TAG("Splitter", g_splitter, "Panes separated by bars the user drags."),
	TAG("WrapBox", g_box, "Lines its children up and continues on the next line."),
	TAG_PLAIN("Canvas", "Places its children freely (Slot.Position, Slot.Size, Slot.Anchors)."),
	TAG("Switcher", g_switcher, "Shows one of its children."),
	TAG("TabView", g_tab_view, "Its children as tabs (Slot.Title)."),
	TAG("Expander", g_expander, "A header that shows and hides one child."),
	TAG("Label", g_label, "Text."),
	TAG("Button", g_button, "A button, with a text or with one child."),
	TAG("Checkbox", g_checkbox, "A box to check, a radio button or a switch."),
	TAG("Slider", g_slider, "A value chosen by dragging."),
	TAG("ProgressBar", g_progress_bar, "How far something is."),
	TAG("Image", g_image, "A picture."),
	TAG("TextInput", g_text_input, "One line of text to type."),
	TAG("SpinBox", g_spin_box, "A number to type or to step."),
	TAG("ComboBox", g_combo_box, "One choice in a list that drops down."),
	TAG("Separator", g_separator, "A line between things."),
	TAG("ListView", g_list_view, "Rows of text, one selected; any number of them."),
	TAG("TreeView", g_tree_view, "Rows inside rows, made of Item elements."),
	TAG("TileView", g_tile_view, "A grid of tiles, an icon over a name, given by the application (Rtile_view_set_items)."),
	TAG("MenuBar", g_menu_bar, "A bar of Menu elements; on macOS, shown in the menu bar of the system."),
	TAG("Menu", g_menu, "A menu: MenuItem, Separator and Menu elements."),
	TAG("MenuItem", g_menu_item, "An item of a menu."),
	TAG("Viewport", g_viewport, "A place the application draws in with its own renderer."),
	TAG_PLAIN("Custom", "A widget painted and measured by the application."),
	TAG("TextEditor", g_text_editor, "Text of several lines: notes, code."),
	TAG("HeaderBar", g_header_bar, "The title bar of a window with a custom header; holds any widgets."),
	TAG("FileBrowser", g_file_browser, "Folders and files to choose from."),
	TAG("DockArea", g_dock_area, "Panels the user arranges by dragging their tabs."),
	TAG("Island", g_island, "A pill at the top of the window that unfolds: first child folded, second unfolded."),
	TAG("BackgroundBlur", g_background_blur, "A panel that shows what is behind it out of focus, around one child."),
	TAG("ColorPicker", g_color_picker, "Chooses a color: square, hues, opacity, code, numbers, swatches."),
	TAG("Timeline", g_timeline, "A sequencer: Track elements holding Clip and Key elements, and Marker elements."),
	TAG("BlockEditor", g_block_editor, "Blocks that snap into each other (Scratch): Kind elements (Name, Label, Shape, Category) and Category elements (Name, Color)."),
	TAG("Table", g_table, "Rows of cells under columns that can be resized and sorted: Column and Row elements; any number of rows."),
	TAG("DatePicker", g_date_picker, "A date, chosen in a calendar that drops down."),
	TAG("TimePicker", g_time_picker, "A time of the day: hours, minutes, optionally seconds."),
	TAG("Toast", g_toast, "A short message that rises over the content for a few seconds (in an Overlay)."),
	TAG("Breadcrumb", g_breadcrumb, "The path to the current place: links separated by chevrons."),
	TAG("Rating", g_rating, "A score shown and chosen with stars."),
	TAG("Pagination", g_pagination, "Page numbers with previous and next."),
	TAG("NodeGraph", g_node_graph, "Nodes linked by wires: Node elements (Input, Output, any widget), Link and Type elements."),
};

static const rvn_schema_tag_t* rvn_schema_tag(const char* tag) {
	const rvn_widget_class_t* cls;
	size_t i;
	if (!tag) return NULL;
	for (i = 0; i < RVN_COUNT_OF(g_tags); i++) {
		if (rvn_stricmp(g_tags[i].tag, tag) == 0) return &g_tags[i];
	}
	/* another name of a class ("TextBlock", "Notch") */
	cls = rvn_widget_class_by_name(tag);
	if (!cls) return NULL;
	for (i = 0; i < RVN_COUNT_OF(g_tags); i++) {
		if (strcmp(g_tags[i].tag, cls->name) == 0) return &g_tags[i];
	}
	return NULL;
}

static int rvn_schema_is_window(const rvn_schema_tag_t* tag) {
	return tag == &g_tags[0];
}

/* ---------------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------------- */

int Rxml_get_tag_count(void) {
	return (int)RVN_COUNT_OF(g_tags);
}

const char* Rxml_get_tag_name(int index) {
	if (index < 0 || index >= (int)RVN_COUNT_OF(g_tags)) return NULL;
	return g_tags[index].tag;
}

const char* Rxml_get_tag_description(const char* tag) {
	const rvn_schema_tag_t* entry = rvn_schema_tag(tag);
	return entry ? entry->description : "";
}

int Rxml_get_tag_children(const char* tag) {
	const rvn_schema_tag_t* entry = rvn_schema_tag(tag);
	const rvn_widget_class_t* cls;
	if (!entry) return 2;
	if (rvn_schema_is_window(entry)) return 1;
	cls = rvn_widget_class_by_name(entry->tag);
	if (!cls) return 2;
	if (cls->flags & RVN_CF_NO_CHILDREN) return 0;
	return (cls->flags & RVN_CF_SINGLE_CHILD) ? 1 : 2;
}

/* The parameters of a tag declared with <Widget>. */
static int rvn_schema_parameter_count(const char* tag) {
	int count = 0;
	while (rvn_xml_component_parameter(tag, count, NULL, NULL)) count++;
	return count;
}

int Rxml_get_attribute_count(const char* tag) {
	const rvn_schema_tag_t* entry = rvn_schema_tag(tag);
	int shared = (int)(RVN_COUNT_OF(g_widget) + RVN_COUNT_OF(g_slot) + RVN_COUNT_OF(g_touch) + RVN_COUNT_OF(g_colors) + RVN_COUNT_OF(g_metrics));
	if (!tag) return 0;
	if (entry) return rvn_schema_is_window(entry) ? entry->count + (int)RVN_COUNT_OF(g_window_touch) : entry->count + shared;
	return rvn_schema_parameter_count(tag) + shared;
}

int Rxml_get_attribute_info(const char* tag, int index, RXmlAttributeInfo* info) {
	static const struct { const rvn_schema_attribute_t* attributes; int count; const char* group; } shared[] = {
		{ g_widget, (int)RVN_COUNT_OF(g_widget), "Widget" },
		{ g_slot, (int)RVN_COUNT_OF(g_slot), "Slot" },
		{ g_touch, (int)RVN_COUNT_OF(g_touch), "Touch" },
		{ g_colors, (int)RVN_COUNT_OF(g_colors), "Colors" },
		{ g_metrics, (int)RVN_COUNT_OF(g_metrics), "Metrics" }
	};
	const rvn_schema_tag_t* entry = rvn_schema_tag(tag);
	const rvn_schema_attribute_t* attribute = NULL;
	const char* group = NULL;
	size_t i;

	if (info) memset(info, 0, sizeof(*info));
	if (!tag || !info || index < 0) return R_FALSE;
	if (entry) {
		if (index < entry->count) {
			attribute = &entry->attributes[index];
			group = entry->tag;
		} else if (rvn_schema_is_window(entry)) {
			index -= entry->count;
			if (index >= (int)RVN_COUNT_OF(g_window_touch)) return R_FALSE;
			attribute = &g_window_touch[index];
			group = "Touch";
		}
		index -= entry->count;
	} else {
		const char* name;
		const char* default_value;
		if (rvn_xml_component_parameter(tag, index, &name, &default_value)) {
			info->name = name;
			info->type = R_XML_TEXT;
			info->group = tag;
			info->description = default_value;
			return R_TRUE;
		}
		index -= rvn_schema_parameter_count(tag);
	}
	for (i = 0; !attribute && i < RVN_COUNT_OF(shared); i++) {
		if (index < shared[i].count) {
			attribute = &shared[i].attributes[index];
			group = shared[i].group;
		}
		index -= shared[i].count;
	}
	if (!attribute) return R_FALSE;
	info->name = attribute->name;
	info->type = attribute->type;
	info->choices = attribute->choices;
	info->group = group;
	info->description = attribute->description;
	return R_TRUE;
}

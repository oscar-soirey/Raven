# Raven XML reference

A window, or any part of one, can be described in XML and loaded with

```c
Rid window = Rwindow_create_from_xml_file("editor.xml");   /* <Window> root  */
Rid panel  = Rwidget_create_from_xml(text, strlen(text)); /* any widget root */
```

The XML is only a way to *build* the tree: once loaded, the widgets are
ordinary handles. Find them with `Rwidget_find(window, "name")` and use the
C API on them. Anything the XML can express can also be done from C, and one
attribute can be set at run time with `Rwidget_set_attribute(widget, "Text", "Hello")`.

```xml
<Window Title="Tool" Width="800" Height="500">
    <Shortcut Keys="Ctrl+S" Action="file.save"/>
    <VBox Spacing="8" Slot.Padding="12">
        <Label TextSize="18">Settings</Label>
        <HBox Spacing="8">
            <TextInput Name="path" Placeholder="Folder..." Slot.Fill="1"/>
            <Button Text="Save" Style="Primary" OnClicked="file.save"/>
        </HBox>
        <ListView Name="files" Slot.Fill="1"/>
    </VBox>
</Window>
```

## Rules

- Tags are widget types. Tag and attribute names are not case sensitive.
- The parser handles elements, attributes, text, comments, CDATA, the five
  predefined entities (`&amp; &lt; &gt; &quot; &apos;`) and numeric character
  references. Files are UTF-8. There is no DTD or namespace.
- `<` and `&` are written `&lt;` and `&amp;` inside attribute values, as XML
  requires: `If="{Count &lt; 3}"`.
- The text inside an element is its `Text` attribute: `<Label>Hello</Label>`
  is `<Label Text="Hello"/>`.
- Children are created first, then the attributes of the parent are applied,
  so `Selected="1"` or `ActiveTab="2"` can refer to children.
- An unknown tag or attribute, or an invalid value, raises a warning through
  the error callback (with line and column) and loading continues.
- Paths (`Source`, `Font`) are relative to the XML file, or to
  `Rxml_set_base_path` when loading from memory. A file is loaded once and
  shared by every widget that names it.

## Value types

| Type | Syntax |
|---|---|
| number | `12`, `0.5`, `-3` (always with a dot, whatever the locale) |
| boolean | `true` / `false`, `yes` / `no`, `1` / `0` |
| color | `#RGB`, `#RGBA`, `#RRGGBB`, `#RRGGBBAA`, or a name: `transparent white black red green blue yellow orange cyan magenta gray` |
| margin | `8` (all sides), `8,4` (horizontal, vertical), `1,2,3,4` (left, top, right, bottom) |
| shortcut | `Ctrl+S`, `Ctrl+Shift+Z`, `F5`, `Alt+Enter`. Modifiers: `Ctrl`, `Shift`, `Alt`, `Super`. Keys: a letter, a digit, `F1`..`F12`, `Space Escape Enter Tab Backspace Insert Delete Left Right Up Down PageUp PageDown Home End` |

Lengths are in logical units: they are multiplied by the scale of the monitor.

## `<Window>`

Only as the root, with `Rwindow_create_from_xml[File]`. It has one child
widget, the content, optionally an `<Island>` that floats at its top, and any
number of `<Shortcut>` and `<Widget>` declarations.

| Attribute | Value |
|---|---|
| `Title` | text |
| `Width`, `Height` | client size (default 800 x 600) |
| `MinWidth`, `MinHeight` | smallest size the user can give the window |
| `Resizable` | boolean (default true) |
| `Borderless`, `Maximized`, `Hidden` | boolean |
| `ClearColor` | color behind the content (default: the theme color `Window`) |
| `Background` | path of an image drawn behind the content, scaled to cover the window. With a glass theme it replaces the wallpaper of the theme |
| `CustomHeader` | boolean: no title bar from the system; a `HeaderBar` in the content replaces it (see below). The window keeps its shadow, its resizing edges and its snapping |
| `Transparent` | boolean: the window can be seen through. Where nothing is drawn, what is behind the window on the screen shows. Needs a window system that composes windows (Windows 10 and 11; X11 with a compositor) |
| `Backdrop` | what the system does to what shows through a `Transparent` window: `Transparent` (nothing), `Blur`, `Acrylic`, `Mica` (Windows 11), `None`. Giving it implies `Transparent` |
| `Theme` | the theme of the window: a name (a look: `RavenDark`, `WindowsLight`, `MacOSDark`, `GnomeLight`, `NativeDark`..., the old `Dark`, `Light`, `GlassDark`, `GlassLight`; or one given by `Rtheme_load`) or a theme file. See [THEMES.md](THEMES.md) |

`<Shortcut Keys="Ctrl+S" Action="file.save"/>` fires an action when the keys
are pressed anywhere in the window.

## Attributes of every widget

| Attribute | Value |
|---|---|
| `Name` (or `Id`) | name for `Rwidget_find` |
| `Width`, `Height` | fixed size, overrides the size the widget asks for |
| `MinWidth`, `MinHeight`, `MaxWidth`, `MaxHeight` | limits of the size the widget asks for |
| `Visibility` | `Visible`, `Hidden` (keeps its place), `Collapsed` (takes no place) |
| `Enabled` | boolean; a disabled widget disables its children |
| `Opacity` | 0..1, applies to the children too |
| `Clip` | boolean: children are clipped to the bounds of the widget |
| `HitTestable` | boolean: false lets the mouse through to what is behind |
| `Focusable` | boolean |
| `Tooltip` | text shown when the cursor rests on the widget |
| `Material` | what the surface of the widget, and of what it contains, is made of: `Solid`, `Glass` (clear), `FrostedGlass`, `TintedGlass`, or `Default` (the material of the theme) |
| `Class` | style classes, separated by spaces: the styles of the theme that name one of them apply to the widget (`Class="card danger"`) |
| `Theme` | the theme of the widget and of what it contains: a name or a theme file; `Default` goes back to what is around |
| `Appear` | transition played each time the widget appears: `None Fade Pop Zoom FlyUp FlyDown FlyLeft FlyRight Drop SlideLeft SlideRight` |
| `AppearDelay` | seconds before the transition starts; growing delays make a group arrive one after the other |
| `WindowDrag` | boolean: in a window with `CustomHeader`, dragging the widget moves the window |
| `Cursor` | `Arrow IBeam Hand Crosshair ResizeH ResizeV ResizeNWSE ResizeNESW Move NotAllowed` |
| `Action`, `OnClicked`, `OnChanged`, `OnCommitted`, `OnActivated` | name of the action fired by the main event of the widget (see below) |
| `Color.<Role>` | overrides a theme color for this widget, e.g. `Color.Accent="#E5484D"` |
| `Metric.<Name>` | overrides a theme metric for this widget, e.g. `Metric.CornerRadius="0"` |

Color roles: `Window Panel Border Text TextDisabled TextPlaceholder Accent
AccentHovered AccentPressed AccentText Control ControlHovered ControlPressed
ControlBorder Input Focus Selection RowHovered Track Handle HandleHovered
Separator Header HeaderHovered TabActive Popup PopupBorder Shadow Tooltip
TooltipText Switch`. `Color.Text` is inherited by the children.

Metrics: `FontSize CornerRadius BorderWidth ControlHeight PaddingX PaddingY
Spacing ScrollbarSize RowHeight Indent SplitterSize CheckSize SliderHandleSize
PopupRadius`.

With a glass theme (`Rtheme_apply(R_THEME_GLASS_DARK)` or
`R_THEME_GLASS_LIGHT`, see the README) the same tags and attributes describe
glass controls. The alpha of a color is then the strength of the tint of the
glass: `Color.Control="#FF375F60"` is a pink glass button, `#FF375F` an opaque
one.

### Actions

A widget has one action name; the five `On...` spellings are synonyms kept
for readability. The action fires on the main event of the widget:

| Widget | Fires when |
|---|---|
| Button, MenuItem | clicked |
| Checkbox, Slider, SpinBox, ComboBox, TabView, Expander | the value changes |
| TextInput | Enter is pressed |
| ListView, TreeView | a row is activated (double click or Enter) |

```c
static void on_save(Rid sender, const char* action, void* user_data) { ... }
Raction_bind("file.save", on_save, NULL);
/* or one function for everything that has no binding of its own */
Raction_set_default_handler(on_any_action, NULL);
```

Callbacks set from C work alongside actions and cover the other events:
every keystroke of a text input (`Rtext_input_set_changed_callback`), the
selection of a list or tree (`Rlist_view_set_selection_changed_callback`,
`Rtree_view_set_selection_changed_callback`), raw input (`Rwidget_set_event_callback`).

## Slot attributes

A slot belongs to the child and is read by its parent panel (Slate). Each
panel uses the attributes that make sense for it.

| Attribute | Value | Used by |
|---|---|---|
| `Slot.Padding` | margin around the child | every panel, and the window for its content |
| `Slot.HAlign` | `Fill` (default), `Left`, `Center`, `Right` | VBox, Grid, Overlay, Border, SizeBox, ScrollBox, Switcher, window content |
| `Slot.VAlign` | `Fill` (default), `Top`, `Center`, `Bottom` | HBox, Grid, Overlay, Border, SizeBox, ScrollBox, Switcher, window content |
| `Slot.Fill` | number: share of the free space along the panel (`1` = one share). Without it the child gets the size it asks for | VBox, HBox; pane size in Splitter |
| `Slot.Auto` | `true`: back to the size the child asks for | VBox, HBox |
| `Slot.MaxSize` | limit of a filling child along the panel | VBox, HBox |
| `Slot.Column`, `Slot.Row` | cell, from 0 | Grid |
| `Slot.ColumnSpan`, `Slot.RowSpan` | number of cells covered (default 1) | Grid |
| `Slot.Anchors` | `x,y` pins to a point of the parent, `minX,minY,maxX,maxY` stretches between two; fractions 0..1 | Canvas |
| `Slot.Position` | `x,y` offset from the anchor | Canvas |
| `Slot.Size` | `width,height` (when the anchors do not stretch) | Canvas |
| `Slot.Alignment` | `x,y` pivot of the child, 0..1 (`0.5,0.5` centers it on its position) | Canvas |
| `Slot.Title` | text of the tab | TabView, DockArea |
| `Slot.Dock` | `Left`, `Right`, `Top`, `Bottom`: the side the panel takes; `Tab`: a tab beside the panel named by `Slot.DockTo` | DockArea |
| `Slot.DockTo` | name of the panel it is placed against; without it, the whole area | DockArea |
| `Slot.DockSize` | share of the place it is cut from, 0..1 (0: a half, or a quarter of the whole area) | DockArea |
| `Slot.Closable` | `false` removes the close button of its tab | DockArea |

## Panels

| Tag (aliases) | Children | Attributes |
|---|---|---|
| `VBox` (`VerticalBox`) | stacked top to bottom | `Spacing` |
| `HBox` (`HorizontalBox`) | left to right | `Spacing` |
| `Grid` (`GridPanel`) | placed by `Slot.Column` / `Slot.Row` | `Spacing`; `Columns`, `Rows`: one entry per track, `auto` or a fill share, e.g. `Columns="auto,1,2"` |
| `UniformGrid` (`UniformGridPanel`) | cells of equal size, filled row by row | `Columns` (children fill rows of this many cells; 0 = each child goes to its `Slot.Column` / `Slot.Row`), `MinCellSize` (`size` or `width,height`), `Spacing` |
| `Overlay` | stacked on top of each other, last on top | |
| `Border` | one child, with a background | `Padding`, `Background`, `BorderColor`, `BorderWidth`, `CornerRadius` (`r` or `topLeft,topRight,bottomRight,bottomLeft`), `Shadow` (`blur` or `blur,offsetX,offsetY`), `Material` (`Solid`, or `Glass`: a panel that blurs and refracts what is behind it, tinted by `Background` when given) |
| `SizeBox` (`Box`) | one child with a forced size (`Width`, `Height`...) | `AspectRatio` (width / height) |
| `Spacer` | none: empty space | `Width`, `Height`, or `Slot.Fill` |
| `ScrollBox` | scrolled content | `Orientation` (`Vertical` default), `ScrollBoth`, `Spacing` |
| `Splitter` | panes separated by draggable handles | `Orientation` (`Horizontal` = side by side, default), `MinPaneSize` |
| `WrapBox` | left to right, wrapping to the next line | `Spacing` |
| `Canvas` (`CanvasPanel`) | free placement with anchors | |
| `Switcher` (`WidgetSwitcher`) | shows one child at a time | `Index` |
| `TabView` (`Tabs`) | one tab per child (`Slot.Title`) | `ActiveTab` |
| `Expander` (`ExpandableArea`) | one child under a collapsible header | `Title`, `Expanded` |
| `DockArea` | each child is a panel with a tab, moved by the user (see the slot attributes); the first child fills the area, the others are placed around it | `Movable` (`false`: the panels stay where they are), `Layout` (an arrangement saved with `Rdock_area_save_layout`) |
| `HeaderBar` (`TitleBar`) | the title bar of a window with `CustomHeader`: children are packed from the left, those with `Slot.HAlign="Right"` from the right, before the window buttons | `Title` (default: the title of the window), `Buttons` (`All`, `None`, or a list: `Close,Minimize,Maximize`), `ButtonSide` (`Right` or `Left`), `TitleAlign` (`Center` or `Left`), `Background` (`false`: no surface) |
| `BackgroundBlur` | one child, over a panel that shows what is behind it out of focus | `Style`: `Plain` (only blurred), `Frosted` (a veil and a fine grain, default), `Glass` (bent at its rim, as the glass material), `Native` (opens the window so that the system blurs the desktop behind it; in a window that is not seen through, the same as `Frosted`); `Radius` (0 = the theme), `Tint`, `Saturation`, `Grain` (0..1), `Shadow`, `Padding`, `CornerRadius` |
| `Island` (`Notch`, `DynamicIsland`) | first child: folded; second child: unfolded. As a child of `<Window>`, beside the content, it floats at the top of the window (`Slot.Padding` is its distance from the edge) | `Expanded`, `ExpandOnHover` (default true), `ExpandOnClick` (default true), `Attached` (square upper corners: a notch) |

## Controls

| Tag (aliases) | Attributes |
|---|---|
| `Label` (`Text`, `TextBlock`) | `Text`, `TextSize` (or `FontSize`), `Font` (path of a .ttf), `TextAlign` (`Left Center Right`), `Wrap`, `Color`, `Markdown` (`true`: the text is Markdown, see below) |
| `Button` | `Text`, `Style` (`Default`, `Primary`, `Flat`) |
| `Checkbox` (`CheckBox`) | `Text`, `Checked`, `Style` (`Check`, `Radio`, `Switch`). Radio boxes of the same parent are exclusive |
| `Slider` | `Min`, `Max`, `Value`, `Step`, `Orientation` |
| `ProgressBar` | `Value` (0..1) |
| `SpinBox` | `Min`, `Max`, `Value`, `Step`, `Decimals` |
| `TextInput` (`TextBox`, `EditableText`) | `Text`, `Placeholder`, `Password`, `ReadOnly`, `MaxLength` |
| `ComboBox` | `Selected` (index), items as children `<Item Text="..."/>` or `Items="A|B|C"` |
| `ListView` | `Selected`, items as children `<Item>` or `Items="A|B|C"` (large lists: `Rlist_view_set_virtual_items`) |
| `TreeView` | nested `<Item Text="..." Expanded="true">` children |
| `Image` | `Source` (path of an image), `Tint`, `Stretch` (`Fill Fit Cover None`), `Region` (`u0,v0,u1,v1`), `CornerRadius` |
| `Separator` | `Orientation` (`Horizontal` line by default) |
| `MenuBar` | `<Menu>` children |
| `Menu` | `Text`; children `<MenuItem>`, `<Separator/>` and nested `<Menu>` |
| `MenuItem` | `Text`, `Shortcut`, `Checkable`, `Checked` |
| `Viewport` | `Source` (image shown until the application sets a texture), `Stretch`, `FlipY`, `Realtime` |
| `Custom` | drawn by the application: `Rcustom_set_paint_callback` |
| `TextEditor` (`TextArea`) | `Text` (or the text inside the element), `Language` (`C`, `C++`, `C#`, `Java`, `JavaScript`, `TypeScript`, `Python`, `Rust`, `GLSL`, `JSON`, `CSS`, `XML`, `HTML`, one declared with `Rlanguage_register`, or `None`), `LineNumbers`, `Wrap`, `ReadOnly`, `Placeholder`, `TabSize`, `TabSpaces`, `Completion` (`false`: proposes words only on Ctrl+Space), `AutoPairs` (`false`: does not close brackets and quotes), `TextSize`, `Font`, `Markdown` (`true`: the same as `Language="Markdown"`: the text stays as typed, styled in place) |
| `FileBrowser` | `Directory`, `Mode` (`Open`, `Save`, `Folder`), `Filter` (`*.png;*.jpg`), `FileName`, `DefaultExtension`, `ShowHidden`, `Multiple`, `Controls` (`false` hides the name, the filter and the buttons), `Places` (`false` hides the list of places) |

A `MenuItem` shortcut works without any `<Shortcut>` element: the window
looks for it in its menus when the keys are pressed.

## Tools

| Tag | Attributes and elements |
|---|---|
| `ColorPicker` | `Color`, `Previous` (the color shown beside it, to compare), `Alpha` (`false`: no opacity), `Popup` (`true`: a swatch that opens the picker), `Compact` (`true`: no code, no numbers), `Numbers` (`RGB` or `HSV`), `Swatches` (`"#FF453A, #0A84FF, white"`), `OnChanged` |
| `Timeline` | `Duration` (seconds), `Time`, `FrameRate` (0: none), `Snapping`, `ViewStart`, `ViewEnd`, `Loop`, `Playing`, `HeaderWidth` (0: fits the names), `TrackHeight` (0: follows the theme), `RulerHeight` (0: follows the theme), `OnChanged`. Contains `<Track>` and `<Marker>` elements |
| `<Track>` | `Name`, `Color`, `Height`, `Locked`, `Muted`. Contains `<Clip>` and `<Key>` elements |
| `<Clip>` | `Start`, `Length` or `End`, `Label` (or the text of the element), `Color`, `Selected` |
| `<Key>` | `Time`, `Label`, `Color` |
| `<Marker>` | `Time`, `Label`, `Color`: a flag on the ruler |
| `NodeGraph` | `Zoom`, `CenterX`, `CenterY`, `Grid` (`false`: no dots), `Flow` (`Auto`: the pulses along the links run while the pointer is over the graph; `true`: always; `false`: never), `ReadOnly`, `FrameAll` (`true`: starts with every node in view), `OnChanged`. Contains `<Node>`, `<Link>` and `<Type>` elements |
| `<Node>` | `Name` (for the links and `Rnode_graph_find_node`), `Title`, `X`, `Y`, `Color` (of its header), `Selected`. Contains `<Input>` and `<Output>` elements, and any widgets: they are shown inside the node (several are stacked) |
| `<Input>`, `<Output>` | `Name`, `Label`, `Type` (free text; pins of different types do not link, a pin without a type links to any) |
| `<Link>` | `From="node.output"`, `To="node.input"`, after the nodes it links |
| `<Type>` | `Name`, `Color`: the color of the pins of a type and of their links (without it, Raven picks one from the name) |

```xml
<NodeGraph Name="graph">
    <Type Name="number" Color="#64D2FF"/>
    <Node Name="noise" Title="Noise" X="40" Y="60" Color="#FF9F0A">
        <Output Name="value" Label="Value" Type="number"/>
        <Slider Value="0.4" Width="150"/>
    </Node>
    <Node Name="blend" Title="Blend" X="320" Y="90">
        <Input Name="a" Label="A" Type="number"/>
    </Node>
    <Link From="noise.value" To="blend.a"/>
</NodeGraph>

<Timeline Duration="12" FrameRate="24">
    <Track Name="Camera" Color="#0A84FF">
        <Clip Start="0" Length="3.5" Label="Wide shot"/>
        <Key Time="6"/>
    </Track>
    <Marker Time="4" Label="Intro"/>
</Timeline>
```

## Markdown

`Markdown="true"` on a `Label` renders its text: headings (`#` to `######`),
**strong**, *emphasis*, ~~struck~~, `code`, links, lists (bulleted, numbered,
nested, with check boxes), quotes, code blocks (colored when they name a
language of the text editor), tables, rules, line breaks and images. Images on
a line of their own use the usual `![description](path)` syntax; `path` can be
relative to the XML file or an `http://` / `https://` URL. Remote images load
asynchronously (requires `curl` on macOS/Linux and WinHTTP on Windows). Inside
an XML element the text may be indented with the document; `<![CDATA[ ... ]]>`
avoids escaping `<` and `&`.

A link `[text](https://...)` opens in the browser of the system;
`[text](action:name)` fires the action `name`; `Rlabel_set_link_callback`
receives them all instead. Bold and italic are drawn from the regular font
unless the application gives real ones (`Rfont_set_variant`).

On a `TextEditor`, the same flag keeps the text as typed and styles it where
it stands (headings larger in color, strong and emphasis in bold and italic,
code and links in their colors).

## Files that are loaded again when they change

A window or a widget created from a file (`Rwindow_create_from_xml_file`,
`Rwidget_create_from_xml_file`), the files it includes, the files of declared
tags (`Rxml_register_widgets_from_file`) and the theme files are watched
while the application runs. When one is saved, the interface follows:

- an element whose attributes changed keeps its widget (its handle, its
  callbacks, its state: scroll, text typed, selection) and receives the new
  values;
- elements are matched by `Name`, else by content, else by position among
  those of the same tag; new ones are built, those that are gone are deleted;
- a widget that manages its own content (`ComboBox`, `Timeline`, `NodeGraph`,
  menus...) is built again when the elements inside it change;
- a file with a mistake changes nothing, and the error is reported.

`Rxml_set_reload_callback` tells after each reload (the place to look widgets
up again with `Rwidget_find`); `Rxml_set_live_reload(R_FALSE)`, or the
environment variable `RAVEN_LIVE_RELOAD=0`, turns the watching off. Documents
given as text (`Rwindow_create_from_xml`) are not files and are not watched.

## Tags made by code

A library or an application can give a tag to a widget it builds itself:

```c
static Rid make_gauge(const char* tag, void* user) { return Rwidget_create(R_WIDGET_CUSTOM); /* + callbacks */ }
static int gauge_attribute(Rid widget, const char* name, const char* value, void* user) {
    if (strcmp(name, "Level") != 0) return R_FALSE;      /* not mine: Raven handles Name, Width, Slot... */
    /* ... */
    return R_TRUE;
}
Rxml_register_tag("Gauge", make_gauge, gauge_attribute, NULL);      /* then: <Gauge Level="0.7" Width="120"/> */
```

`<WebView>` of the optional library `raven_web` is such a tag: see
[WEB.md](WEB.md).

## Declaring tags

A document can declare its own tags. A `<Widget>` names a piece of interface,
with parameters, and is then used like any widget of the library:

```xml
<Widget Name="Field" Label="Name" Hint="" Wide="false">
    <HBox Spacing="8">
        <Label Text="{Label}:" Width="{Wide ? 140 : 90}"/>
        <TextInput Placeholder="{Hint}" Slot.Fill="1"/>
        <Children/>
    </HBox>
</Widget>

<Field Label="Email" Hint="you@example.com" Name="email"/>
<Field Label="Password" Wide="true"><Button Text="Show"/></Field>
```

- The attributes of `<Widget>` other than `Name` are its **parameters**, with
  their default values. It contains exactly one element: what the tag stands for.
- Inside it, `{...}` is computed and replaced by its value. `{{` and `}}`
  write a brace.
- Where the tag is used, the attributes that are parameters give their
  values; the others (`Name`, `Slot.Fill`, `OnClicked`, `Material`...) go to
  the element the tag stands for. The text inside the tag is its `Text`.
- `<Children/>` is replaced by the children given where the tag is used;
  `<Children Name="footer"/>` by those marked `Into="footer"`.
- A declaration is known from the moment it is read until the library shuts
  down, in every document loaded afterwards. Declaring a name again replaces
  it. It cannot take the name of a tag of the library.
- A tag can use the tags declared before it, but not itself.

| Element or attribute | |
|---|---|
| `<Widget Name="..." Param="default" ...>` | declares a tag |
| `<Widgets>` | the root of a file that only holds declarations |
| `<Children/>`, `<Children Name="x"/>` | where the children given to the tag go |
| `Into="x"` | on a child given to a tag: goes to `<Children Name="x"/>` |
| `If="..."` | on any element: kept when the value is true |
| `<If Test="...">` | keeps what it contains when the test is true |
| `<Repeat Count="3" Var="i">` | repeats what it contains; `{i}` is 0, 1, 2 |
| `<ForEach Items="a\|b\|c" Var="x" Index="n" Separator="\|">` | repeats for each text of a list; `{x}` is the text, `{n}` its index |
| `<Include Source="file.xml"/>` | brings in another file: its declarations, and its element if it has one |

Expressions: numbers, texts between single quotes, parameters and variables;
`+ - * / %` (`+` joins texts), `== != < <= > >=`, `&& || !`, `a ? b : c`,
parentheses; `min(a, b)`, `max(a, b)`, `round`, `floor`, `ceil`, `abs`,
`len(text)`, `upper`, `lower`, `empty(text)`, `default(value, fallback)`;
`{Instance}` is a number that differs for each use of a tag (to build unique
names). False is `false`, `0` and the empty text. `{...}` is only
computed inside a `<Widget>`, a `<Repeat>` or a `<ForEach>`: elsewhere braces
are ordinary characters.

From C: `Rxml_register_widgets(text, size)` and
`Rxml_register_widgets_from_file(path)` read declarations ahead of the
documents that use them; `Rxml_get_registered_widget_name` lists them.
`examples/components` shows all of this.

## Documents and tools

`Rxml_document_parse` reads a text into a tree of nodes that can be changed,
written back (`Rxml_document_to_string`, comments kept) and built
(`Rxml_document_create_widget`, `Rxml_document_create_window`). A widget built
from a document knows its node (`Rwidget_get_xml_node`) and a node its widget
(`Rxml_node_get_widget`). `Rxml_get_tag_name`, `Rxml_get_attribute_count` and
`Rxml_get_attribute_info` list the tags of the library and, for each, its
attributes with the kind of value they take: the tables of this page as data.
The designer (`raven_designer`) is built on these functions.

## A complete example

`examples/editor/editor.xml` is a full editor layout: menu bar, tool bar,
nested splitters, tree, tabs with a viewport and a custom widget, a property
grid and a log.

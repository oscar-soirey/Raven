# Looks and themes

How a widget is drawn comes from two layers:

- a **look**: the shapes of the controls, how they show their states, how
  they move, and the colors, sizes and font that go with them. Raven has its
  own (flat, glass, high contrast) and looks that imitate the platforms
  (Windows 11, macOS, GNOME). One look for the application: `Rlook_set`.
- a **theme**, laid over the look: about forty **colors** (roles, not
  widgets: "accent", "panel", "text"...), twenty **metrics** (font size,
  corner radius, height of a control...), a **material** (solid or one of the
  three kinds of glass), a **font**, the colors of **code**, and **styles**:
  values for one type of widget, for a class, or for both; for a **part** of
  a widget (the thumb of a slider) in some **states** (hovered, pressed...).

The rest of this page is how to choose a look and how to write themes.
`examples/themes` shows all of it, and its files can be edited while it runs.

![The themes example](images/themes.png)

*`examples/themes`: the theme `ocean.xml` for the application, a card that
keeps the theme `paper.xml`, another one in dark glass; on the right, a
Markdown label and a Markdown editor.*

## Looks

```c
Rlook_set(R_LOOK_WINDOWS_LIGHT);   /* Windows 11, light                           */
Rlook_set(R_LOOK_NATIVE_DARK);     /* the look of the platform the program runs on */
RLook look = Rlook_get();
```

| Look | |
|---|---|
| `R_LOOK_RAVEN_DARK`, `R_LOOK_RAVEN_LIGHT` | Raven, flat and dense, for tools. The look at start |
| `R_LOOK_RAVEN_GLASS_DARK`, `R_LOOK_RAVEN_GLASS_LIGHT` | Raven in glass: translucent capsules that refract what is behind them |
| `R_LOOK_RAVEN_HIGH_CONTRAST_DARK`, `..._LIGHT` | black and white, thick borders, larger text (`Raccessibility_set_high_contrast`) |
| `R_LOOK_WINDOWS_DARK`, `R_LOOK_WINDOWS_LIGHT` | Windows 11 (Fluent) |
| `R_LOOK_MACOS_DARK`, `R_LOOK_MACOS_LIGHT` | macOS Sequoia (Aqua) |
| `R_LOOK_MACOS_GLASS_DARK`, `R_LOOK_MACOS_GLASS_LIGHT` | macOS 26 Tahoe (Liquid Glass) |
| `R_LOOK_GNOME_DARK`, `R_LOOK_GNOME_LIGHT` | GNOME (libadwaita), the desktop of most Linux distributions |
| `R_LOOK_NATIVE_DARK`, `R_LOOK_NATIVE_LIGHT` | Windows on Windows, macOS on macOS (macOS Glass from macOS 26 on), GNOME on Linux and the other systems |

The look of Windows imitates WinUI closely, without using it: the controls
of Windows 11 (buttons with their darker bottom edge, check boxes, radio
buttons, switches, fields with their line of the accent, combo boxes, number
boxes with their buttons, sliders, thin progress bars, scrollbars that widen
under the pointer, menus and flyouts, tabs, lists and trees with the pill of
the selection, tooltips, expanders, the buttons of the title bar), their
colors and sizes, and motion without bounce. On Windows it also takes from
the system:

- the **accent color** the user chose (its shades, as Windows 11 uses them),
- the **font**, Segoe UI, and the icons of Segoe Fluent Icons (Segoe MDL2
  Assets on Windows 10) for the chevrons, check marks and window buttons,
- **reduced motion**, when animations are turned off in the settings.

(The font and the accent come from the system the program runs on only: the
look of Windows shown on a Mac keeps the font of Raven and its own blue.)

The look of **macOS** imitates AppKit (Sequoia): white push buttons with a
hairline and a little shadow, the default button in the accent, small check
boxes and radio buttons, `NSSwitch`, fields with a ring of the accent around
them, pop-up buttons with their two arrows on a square of the accent,
steppers, segmented controls for the tabs, tables whose selection is the
accent, menus whose entry under the pointer is the accent, overlay
scrollbars, disclosure triangles, the traffic lights on the left of the
title bar. Nothing changes under the pointer, as on the Mac. On macOS it
takes the accent color of the System Settings, San Francisco, and reduced
motion.

The look of **macOS Glass** is the one of macOS 26 (Tahoe), with Liquid
Glass. The controls are those of macOS, larger (24 high) and rounder, without
the lines of Aqua: push buttons on a soft shadow, the default button a plain
accent, fields as round as the buttons, a capsule for the segmented control
of the tabs, rows of lists and of menus with rounder corners. Glass is where
the system puts it, on what floats above the content: header bars and the
buttons on them (capsules of glass, the primary one tinted with the accent),
menus, drop-down lists, tooltips and sheets. The knobs of sliders and
switches are white capsules that turn into a lens of clear glass while they
are held. The content itself (fields, lists, text, the buttons of a form)
stays opaque, as Apple asks. `R_LOOK_NATIVE_...` gives it on macOS 26 and
later, and the look of Sequoia before. Its names in theme files:
`MacOSGlassDark`, `MacOSGlassLight` (and `Tahoe`, `LiquidGlass`).

The sizes are measured on the system (Apple publishes none but the corners
of windows): a theme file can correct any of them (`<Metrics CornerRadius="10"/>`).

The look of **GNOME** imitates libadwaita: flat rounded controls of the color
of the text seen through, the suggested action in the accent, entries with
a ring of the accent inside them, round check boxes and switches,
`AdwToggleGroup` for the tabs, boxed lists, popover menus with large
corners, round window buttons on a grey header bar. On Linux it takes the
accent color of GNOME 47 and later (`accent-color`), the font of the desktop
and its size (`font-name`, found with fontconfig), and "Reduce Animation".
It needs no library: these settings are read with `gsettings` and
`fc-match` when they are there.

The looks of the platforms are opaque, but for the glass macOS Glass draws
where macOS puts it: the materials belong to the looks of Raven.
`Rtheme_set_material` and `Material="..."` in a theme file are ignored there
(with a warning); a material given to one widget (`Rwidget_set_material`,
`Material=` in the interface) is still honored.

`Rtheme_apply(R_THEME_DARK)` and the other `R_THEME_...` of Raven 0.2 still
work: they are the looks of Raven (`R_THEME_DARK` is `R_LOOK_RAVEN_DARK`).
They replace the theme file in use; `Rlook_set` keeps it (see below).

## From C

```c
Rlook_set(R_LOOK_RAVEN_DARK);                              /* a look                                    */
Rtheme_set_accent(0.13f, 0.83f, 0.93f);                    /* the accent and every shade that follows   */
Rtheme_set_color(R_COLOR_PANEL, 0.07f, 0.10f, 0.17f, 1);   /* one color                                 */
Rtheme_set_metric(R_METRIC_CORNER_RADIUS, 10);             /* one metric                                */
Rtheme_set_material(R_MATERIAL_FROSTED_GLASS);             /* what surfaces are made of                 */
```

`Rtheme_set_accent` derives the hovered and pressed shades, the focus
outline, the selection and the links from one color. `Rtheme_set_dark`
tells whether the theme is a dark one (it decides the tint of glass, the
default colors of code and the wallpaper of the glass themes).

For one widget and what it contains: `Rwidget_set_color`,
`Rwidget_set_metric`, `Rwidget_set_material`, or in XML `Color.Accent="#F00"`,
`Metric.CornerRadius="4"`, `Material="Glass"`.

## Styles and classes

A style gives values to the widgets of a type, of a class, or of both:

```c
Rtheme_set_style_metric("Button", NULL, R_METRIC_CORNER_RADIUS, 16);          /* every button         */
Rtheme_set_style_color(NULL, "danger", R_COLOR_ACCENT, 0.98f, 0.44f, 0.52f, 1); /* the class "danger" */
Rtheme_set_style_color("Label", "eyebrow", R_COLOR_TEXT, 0.13f, 0.83f, 0.93f, 1);
Rtheme_set_style_material(NULL, "card", R_MATERIAL_TINTED_GLASS);
```

A widget has any number of classes, separated by spaces: `Class="card
danger"` in XML, `Rwidget_set_class`, `Rwidget_add_class`,
`Rwidget_remove_class`, `Rwidget_has_class` in C. Changing the classes of a
widget changes its look at once, which makes them a simple way to show a
state (`Rwidget_add_class(field, "invalid")`).

When several styles give the same value to a widget, a style with a type and
a class wins over one with a class, which wins over one with a type; between
equals, the last one set wins. What was given to the widget itself
(`Rwidget_set_color`, `Color.*`) wins over every style.

Colors and metrics are inherited by what a widget contains only when they
are set on the widget (`Rwidget_set_color`); a style applies to the widgets
it names, not to their children, except the color of text, which every
widget takes from the nearest ancestor that has one.

## Theme files

```xml
<Theme Name="Ocean" Look="RavenDark" Accent="#22D3EE" CornerRadius="10">
    <Colors Window="#0B1220" Panel="#111A2C" Text="#DCE7F5" Separator="#FFFFFF1A"/>
    <Metrics ControlHeight="32" Spacing="8"/>
    <Glass Frost="0.8" Refraction="1.2" Saturation="1.1" Rim="1" Shadow="1"/>
    <Syntax Keyword="#7DD3FC" String="#86EFAC" Comment="#64748B" Heading="#22D3EE"/>

    <Style Widget="Button" CornerRadius="16"/>
    <Style Class="danger" Accent="#FB7185" AccentHovered="#FDA4AF" AccentPressed="#E11D48"/>
    <Style Class="card" Panel="#1B2742" CornerRadius="18" Material="Solid"/>
    <Style Widget="Label" Class="eyebrow" Text="#22D3EE" FontSize="12"/>
</Theme>
```

| On `<Theme>` | |
|---|---|
| `Name` | the name the theme is kept under (default: the name of the file) |
| `Look` | the look it starts from, and imposes: `RavenDark`, `RavenLight`, `RavenGlassDark`, `RavenGlassLight`, `RavenHighContrastDark`, `RavenHighContrastLight`, `WindowsDark`, `WindowsLight`, `MacOSDark`, `MacOSLight`, `MacOSGlassDark`, `MacOSGlassLight`, `GnomeDark`, `GnomeLight`, `NativeDark`, `NativeLight` (the names of Raven 0.2 work too: `Dark`, `GlassLight`...). Without `Look` (nor `Base`), the theme is laid over the look in use and **follows it**: `Rlook_set` reads it again over the new look |
| `Base` | the theme it starts from: a theme loaded before, or a look by its old name (`Dark`, `Light`, `GlassDark`, `GlassLight`). Kept for Raven 0.2; `Look` is the new name |
| `Motion` | `Springy` (Raven: things bounce and squash) or `Smooth` (the platforms: short transitions that settle at once). Each look has its own |
| `Accent` | the accent color, and the shades derived from it |
| `Material` | `Solid`, `Glass`, `FrostedGlass`, `TintedGlass` |
| `Dark` | `true` / `false`, when the base does not say it right |
| `Font` | a font file, relative to the theme file |
| `Wallpaper` | what is behind the content of the windows (what glass refracts): an image file, `Dark` or `Light` (the soft ones of the glass themes), colors to blend (`"#1E3A8A,#7C3AED,#0F766E"`), or `None` |
| any color or metric | `Panel="#111A2C"`, `CornerRadius="10"`: the same as inside `<Colors>` and `<Metrics>` |

| Element | |
|---|---|
| `<Colors>` | one attribute per role: `Window Panel Border Text TextDisabled TextPlaceholder Accent AccentHovered AccentPressed AccentText Control ControlHovered ControlPressed ControlBorder Input Focus Selection RowHovered Track Handle HandleHovered Separator Header HeaderHovered TabActive Popup PopupBorder Shadow Tooltip TooltipText Switch Success Warning Danger Link Code` |
| `<Metrics>` | `FontSize CornerRadius BorderWidth ControlHeight PaddingX PaddingY Spacing ScrollbarSize RowHeight Indent SplitterSize CheckSize SliderHandleSize PopupRadius BlurRadius` |
| `<Glass>` | `Frost Refraction Saturation Rim Shadow`: multipliers of what the glass of the theme does (1 = as designed, 0 = none) |
| `<Syntax>` | colors of code in the text editor: `Comment String Number Keyword Type Function Tag Attribute Punctuation Directive Heading Strong Emphasis Link` |
| `<Style Widget="..." Class="..." Part="..." State="...">` | colors, metrics and `Material` for the widgets of that type and / or class; properties of a part of them, in some states (below) |

## Parts and states

A style can reach a part of a widget, in some states, and give it
properties. It is how a theme changes the look of a control without code:

```xml
<Theme Name="Tweaks" Accent="#E8590C">
    <!-- buttons of the class "danger": red, white text, darker when pressed -->
    <Style Widget="Button" Class="danger" Background="#C42B1C" Foreground="#FFFFFF"/>
    <Style Widget="Button" Class="danger" State="Pressed" Background="#A12317"/>
    <!-- the core of the thumb of the sliders grows under the pointer -->
    <Style Widget="Slider" Part="Knob" State="Hovered" Size="16"/>
    <!-- switches that are on: green -->
    <Style Widget="Checkbox" Part="Track" State="Checked" Background="#2F9E44" BorderColor="#2F9E44"/>
    <!-- the pill of the selected rows, everywhere -->
    <Style Part="Indicator" Background="#2F9E44"/>
</Theme>
```

| Property | |
|---|---|
| `Background` | the color it is filled with (`None`: nothing) |
| `Foreground` (or `Text`) | its text, marks and glyphs |
| `BorderColor`, `BorderWidth` | its outline |
| `BorderTopColor`, `BorderBottomColor`, `BorderBottomWidth` | its top and bottom edges, when they differ (the controls of Windows have a darker bottom edge; a focused field, a thicker line of the accent) |
| `CornerRadius` | its corners |
| `Size` | the size of a part that has one of its own: a knob, a dot, a pill |
| `Opacity` | 0..1 |
| `ShadowColor`, `ShadowSize` | the shadow it drops (popups) |

| `Part` | of | |
|---|---|---|
| (none) | every widget | the widget itself |
| `Box`, `Mark` | Checkbox | a check box and its check mark |
| `Radio`, `Dot` | Checkbox (`Style="Radio"`) | a radio button and its dot |
| `Track`, `Knob` | Checkbox (`Style="Switch"`) | a switch and its knob |
| `Track`, `Fill`, `Thumb`, `Knob` | Slider | the rail, what fills it, the thumb, its core (Windows) |
| `Track`, `Fill` | ProgressBar | |
| `Arrow` | ComboBox, SpinBox, TreeView, Expander | the chevrons, and the buttons of a spin box |
| `Popup`, `Item`, `Indicator` | ComboBox, Menu, MenuBar, ListView, TreeView | the flyout, its rows, the pill of the selected one |
| `Tab`, `Indicator`, `Header` | TabView | the tabs, the pill of the active one, the bar |
| `Header` | Expander | its header |
| `Scrollbar`, `ScrollThumb` | everything that scrolls | |
| `Tooltip` | every widget | its tooltip |
| `CaptionButton`, `CloseButton` | HeaderBar | the buttons of the window |
| `Focus` | every widget | the ring of the keyboard focus |

`State` lists states that must all hold: `Hovered`, `Pressed`, `Focused`,
`Disabled`, `Checked` (a check box, a switch, a radio button that is on),
`Selected` (a row, a tab), `Open` (a combo box, an expander, a menu), and
the styles of a button: `Primary`, `Flat`.

The look gives each part its values in each state; the styles that match are
laid over them, from the least specific to the most specific: a type counts
1, a class or a state counts 2 (as in CSS); between equals, the last one
written wins. Hovered and pressed are eased: a color given to `Hovered`
fades in as the ones of the look do.

Without `Part` nor `State`, a style also gives colors and metrics to the
widgets it names (`Accent`, `CornerRadius`...: the metrics keep their
meaning); with a `Part` or a `State`, it only gives properties.

Parts and states are read from theme files; in C, a theme can be given as a
text (`Rtheme_load_from_memory`).

A mistake in a file (an unknown role, a color that is not one) is reported
with its line and the rest of the file is used.

```c
const char* name = Rtheme_load("themes/ocean.xml");   /* "Ocean": kept under its name, not applied   */
Rtheme_use("Ocean");                                   /* the theme of the application                */
Rtheme_save("my-theme.xml", "Mine");                   /* the theme in use, with what was changed     */
```

`Rtheme_get_count` and `Rtheme_get_name_at` list the themes, the six
built-in ones of Raven 0.2 included. `Rtheme_use` also takes the name of a
look (`"WindowsDark"`), which then replaces the theme in use.

## A theme for a part of the window

```xml
<Border Theme="Paper" Padding="18"> ... </Border>        <!-- a theme by name          -->
<Border Theme="themes/paper.xml"> ... </Border>          <!-- or straight from a file  -->
<Window Theme="Ocean" ...>
```

`Rwidget_set_theme(widget, "Paper")` does the same from C. The widget and
everything inside it use that theme, whatever the theme of the application
becomes; popups and menus opened from there wear it too. The island of a
window uses this: it is always dark, as the notch of a phone is, unless it
is told `Theme="Default"`.

## Files that are loaded again when they change

Theme files, like the interface files, are watched while the application
runs: save the file and every widget that uses the theme follows.
`Rxml_set_live_reload(R_FALSE)` (or the environment variable
`RAVEN_LIVE_RELOAD=0`) turns it off for a shipped application;
`Rxml_set_reload_callback` tells when a file was loaded again, and
`Rxml_check_files()` checks now instead of at the next tick (five times a
second). A file with a mistake changes nothing.

## What a widget author needs to know

Widgets read the theme through `rvn_widget_color(widget, role)` and
`rvn_widget_metric(widget, metric)`: what was set on the widget, else what
the styles of its theme give it, else its theme (its own, the one of an
ancestor, or the one of the application). The match of the styles for a
widget is computed once and kept until the styles or its classes change.
From outside the library, `Rwidget_get_color` and `Rwidget_get_metric` give
the same answer to custom widgets.

A widget is drawn in parts (`src/widget/look.c`). For each part and its
states, it asks `rvn_part_look` (or `rvn_part_look_eased`, which blends the
states with the springs of the widget) for a `rvn_part_look_t`: a function of
the widget gives the base of its look (from the colors and metrics of the
theme, and from `rvn_widget_family`: the shapes of Raven or of Windows),
then the styles of the theme for that part are laid over it.
`rvn_paint_part` draws the result. `src/widgets/common.c` holds the colors of
Fluent that are not color roles (`rvn_fluent`) and the looks the widgets of
Windows share (buttons, fields, flyouts, rows); `src/widgets/platform_looks.c`
those of macOS and GNOME, and the `rvn_platform_...` functions that pick the
one of the platform of a widget.

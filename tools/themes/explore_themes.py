
from raven import *
import os


# --------------------------------------------------
# Configuration
# --------------------------------------------------

THEME_DIR = os.path.dirname(os.path.abspath(__file__))

THEMES = {
    "Ocean": "ocean.xml",
    "Midnight": "midnight.xml",
    "Nord": "nord.xml",
    "Paper": "paper.xml",
    "Frost": "frost.xml",
    "Dracula": "dracula.xml",
    "Ember": "ember.xml",
    "Forest": "forest.xml",
    "Monokai": "monokai.xml",
    "Sakura": "sakura.xml",
    "Solarized Dark": "solarized-dark.xml"
}

window = None
status_label = None
layer_label = None

selected_tool = "Move"
loaded_themes = set()


# --------------------------------------------------
# Theme management
# --------------------------------------------------

def load_themes():
    loaded_themes.clear()

    for name, filename in THEMES.items():
        path = os.path.join(THEME_DIR, filename)

        if not os.path.isfile(path):
            print(f"[Theme] Missing file: {path}")
            continue

        try:
            Rtheme_load(path)

            loaded_themes.add(name)
            print(f"[Theme] Loaded: {name}")

        except Exception as error:
            print(f"[Theme] Failed to load {name}: {error}")


def use_theme(sender, action, user):
    index = Rcombo_box_get_selected(sender)

    if index < 0:
        return

    name = Rcombo_box_get_item_text(sender, index)

    if name not in loaded_themes:
        Rlabel_set_text(
            status_label,
            f"Theme unavailable: {name}"
        )
        return

    try:
        Rtheme_use(name)

        Rlabel_set_text(
            status_label,
            f"Theme: {name}"
        )

    except Exception as error:
        print(f"[Theme] Could not activate {name}: {error}")

        Rlabel_set_text(
            status_label,
            f"Failed to activate: {name}"
        )


def change_accent(sender, action, user):
    color = Rcolor_picker_get_color(sender)

    if not isinstance(color, (tuple, list)) or len(color) < 3:
        return

    r, g, b = color[:3]

    Rtheme_set_accent(r, g, b)

    Rlabel_set_text(
        status_label,
        f"Accent: #{int(r * 255):02X}"
        f"{int(g * 255):02X}"
        f"{int(b * 255):02X}"
    )


def save_theme(sender, action, user):
    try:
        success = Rtheme_save(
            os.path.join(THEME_DIR, "custom-theme.xml"),
            "Custom"
        )

        if success:
            Rlabel_set_text(
                status_label,
                "Saved: custom-theme.xml"
            )
        else:
            Rlabel_set_text(
                status_label,
                "Could not save theme"
            )

    except Exception as error:
        print(f"[Theme] Save failed: {error}")

        Rlabel_set_text(
            status_label,
            "Theme save failed"
        )


# --------------------------------------------------
# Toolbar
# --------------------------------------------------

def select_tool(sender, action, user):
    global selected_tool

    if not action:
        return

    selected_tool = (
        action.split(".")[-1]
        .replace("_", " ")
        .title()
    )

    Rlabel_set_text(
        layer_label,
        f"Active tool: {selected_tool}"
    )

    Rlabel_set_text(
        status_label,
        f"Tool selected: {selected_tool}"
    )


# --------------------------------------------------
# Layers
# --------------------------------------------------

def select_layer(sender, action, user):
    index = Rcombo_box_get_selected(sender)

    if index < 0:
        return

    name = Rcombo_box_get_item_text(sender, index)

    Rlabel_set_text(
        layer_label,
        f"Selected layer: {name}"
    )

    Rlabel_set_text(
        status_label,
        f"Layer selected: {name}"
    )


# --------------------------------------------------
# Action bindings
# --------------------------------------------------

def bind_actions():
    Raction_bind("theme.use", use_theme, None)
    Raction_bind("theme.accent", change_accent, None)
    Raction_bind("theme.save", save_theme, None)
    Raction_bind("layer.select", select_layer, None)

    for tool in (
        "move",
        "select",
        "eyedropper",
        "brush",
        "eraser",
        "fill",
    ):
        Raction_bind(
            f"tool.{tool}",
            select_tool,
            None
        )


# --------------------------------------------------
# Interface
# --------------------------------------------------

UI = """
<Window
    Title="Raven Theme Studio"
    Width="1440"
    Height="900"
    MinWidth="1050"
    MinHeight="650"
    CustomHeader="true">

    <VBox Spacing="0">

        <!-- Custom title bar -->
        <HeaderBar Title="" Background="false">
            <Label Text="RAVEN" TextSize="16"/>
            <Label Text="THEME STUDIO" Opacity="0.55"/>
            <Spacer Slot.Fill="1"/>
            <Button Text="Save theme"
                Style="Primary"
                OnClicked="theme.save"/>
        </HeaderBar>

        <!-- Main menu -->
        <HBox Spacing="14" Slot.Padding="12,6">
            <Button Text="File" Style="Flat"/>
            <Button Text="Edit" Style="Flat"/>
            <Button Text="Image" Style="Flat"/>
            <Button Text="Layer" Style="Flat"/>
            <Button Text="View" Style="Flat"/>
            <Button Text="Window" Style="Flat"/>

            <Spacer Slot.Fill="1"/>

            <Label Text="Theme"/>

            <ComboBox
                Name="theme_selector"
                Items="Ocean|Midnight|Nord|Paper|Frost|Dracula|Ember|Forest|Monokai|Sakura|Solarized Dark"
                Selected="0"
                Width="150"
                OnChanged="theme.use"/>
        </HBox>

        <Separator/>

        <!-- Main workspace -->
        <HBox Spacing="0" Slot.Fill="1">

            <!-- Toolbar -->
            <VBox
                Width="66"
                Spacing="8"
                Slot.Padding="8,12">

                <Button Text="Move"
                    Tooltip="Move tool"
                    OnClicked="tool.move"/>

                <Button Text="Select"
                    Tooltip="Selection tool"
                    OnClicked="tool.select"/>

                <Button Text="Picker"
                    Tooltip="Eyedropper"
                    OnClicked="tool.eyedropper"/>

                <Separator/>

                <Button Text="Brush"
                    Tooltip="Brush tool"
                    OnClicked="tool.brush"/>

                <Button Text="Erase"
                    Tooltip="Eraser"
                    OnClicked="tool.eraser"/>

                <Button Text="Fill"
                    Tooltip="Fill tool"
                    OnClicked="tool.fill"/>

                <Spacer Slot.Fill="1"/>

                <Label Text="TOOLS"
                    TextSize="10"
                    Opacity="0.5"/>
            </VBox>

            <Separator Orientation="Vertical"/>

            <!-- Main workspace -->
            <VBox Spacing="0" Slot.Fill="1">

                <HBox Spacing="12" Slot.Padding="12,8">

                    <Label Text="Untitled Theme"
                        TextSize="15"/>

                    <Label Text="• Unsaved"
                        Opacity="0.55"/>

                    <Spacer Slot.Fill="1"/>

                    <Button Text="Fit"
                        Style="Flat"/>

                    <Button Text="100%"
                        Style="Flat"/>

                </HBox>

                <Separator/>

                <!-- Canvas -->
                <VBox
                    Spacing="12"
                    Slot.Fill="1"
                    Slot.Padding="24">

                    <Border Padding="24" Slot.Fill="1">

                        <VBox Spacing="18">

                            <Label Text="LIVE PREVIEW"
                                TextSize="11"
                                Opacity="0.6"/>

                            <Label Text="Design your interface"
                                TextSize="28"/>

                            <Label
                                Text="Preview how Raven widgets look with the current theme."
                                Wrap="true"
                                Opacity="0.75"/>

                            <Separator/>

                            <Label Text="Buttons"
                                TextSize="17"/>

                            <HBox Spacing="10">

                                <Button Text="Primary"
                                    Style="Primary"/>

                                <Button Text="Default"/>

                                <Button Text="Flat"
                                    Style="Flat"/>

                            </HBox>

                            <Label Text="Controls"
                                TextSize="17"/>

                            <HBox Spacing="12">

                                <Checkbox Text="Checkbox"
                                    Checked="true"/>

                                <Checkbox Text="Switch"
                                    Style="Switch"
                                    Checked="true"/>

                            </HBox>

                            <Slider Value="0.65"/>

                            <HBox Spacing="10">

                                <TextInput
                                    Placeholder="Type something..."
                                    Slot.Fill="1"/>

                                <ComboBox
                                    Items="Option A|Option B|Option C"
                                    Selected="0"
                                    Width="150"/>

                            </HBox>

                            <ProgressBar Value="0.7"/>

                            <Spacer Slot.Fill="1"/>

                            <Label Text="Raven GUI • Theme preview"
                                Opacity="0.5"/>

                        </VBox>

                    </Border>

                </VBox>

            </VBox>

            <Separator Orientation="Vertical"/>

            <!-- Right inspector -->
            <VBox Width="285" Spacing="0">

                <HBox Slot.Padding="14,12">

                    <Label Text="Properties"
                        TextSize="17"/>

                </HBox>

                <Separator/>

                <VBox
                    Spacing="10"
                    Slot.Padding="14,14">

                    <!-- Layers -->
                    <Label Text="LAYERS"
                        TextSize="11"
                        Opacity="0.6"/>

                    <ComboBox
                        Name="layer_selector"
                        Items="Background|Canvas|Controls|Typography|Accent"
                        Selected="2"
                        OnChanged="layer.select"/>

                    <Label Name="layer_info"
                        Text="Selected layer: Controls"
                        Wrap="true"
                        Opacity="0.7"/>

                    <Separator/>

                    <!-- Appearance -->
                    <Label Text="APPEARANCE"
                        TextSize="11"
                        Opacity="0.6"/>

                    <Label Text="Accent color"/>

                    <ColorPicker
                        Name="accent_picker"
                        Color="#38BDF8"
                        Alpha="false"
                        OnChanged="theme.accent"
                        Swatches="#38BDF8,#A78BFA,#88C0D0,#2563EB,#34D399,#F87171,#FBBF24,#FFFFFF"/>

                    <Label Text="Theme file"
                        Slot.Padding="0,8,0,0"/>

                    <Label Text="XML with live reload"
                        Opacity="0.6"/>

                    <Separator/>

                    <!-- Metrics -->
                    <Label Text="METRICS"
                        TextSize="11"
                        Opacity="0.6"/>

                    <Label Text="Corner radius"/>

                    <Slider Value="0.4"/>

                    <Label Text="Control spacing"/>

                    <Slider Value="0.5"/>

                    <Spacer Slot.Fill="1"/>

                    <Label Text="Raven Theme Studio"
                        Opacity="0.45"/>

                </VBox>

            </VBox>

        </HBox>

        <Separator/>

        <!-- Status bar -->
        <HBox Spacing="12" Slot.Padding="10,6">

            <Label Name="status"
                Text="Ready"
                Opacity="0.75"/>

            <Spacer Slot.Fill="1"/>

            <Label Text="RGB"
                Opacity="0.55"/>

            <Label Text="100%"
                Opacity="0.55"/>

        </HBox>

    </VBox>

</Window>
"""


# --------------------------------------------------
# Application
# --------------------------------------------------

def main():
    global window
    global status_label
    global layer_label

    Rinit(R_BACKEND_VULKAN)

    try:
        load_themes()
        bind_actions()

        window = Rwindow_create_from_xml(UI)

        if window == R_INVALID_ID:
            return

        status_label = Rwidget_find(window, "status")
        layer_label = Rwidget_find(window, "layer_info")

        if "Ocean" in loaded_themes:
            Rtheme_use("Ocean")

        elif loaded_themes:
            Rtheme_use(next(iter(loaded_themes)))

        Rwindow_show(window)

        while Rwait_frame(-1.0):
            pass

    finally:
        Rshutdown()


if __name__ == "__main__":
    main()

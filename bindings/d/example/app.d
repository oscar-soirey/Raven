// A window described in XML, driven from D.
//
//   RAVEN_LIB_DIR=/path/to/lib dub run --root=example
//
// `-- --test` runs without a window system and without a GPU, clicks its own
// button and checks what happened: it is how the build of Raven checks this
// binding.

import core.stdc.stdlib : exit;
import std.algorithm : canFind;
import std.stdio : writeln;
import std.string : toStringz;

import raven;

enum xml = `
<Window Title="Raven from D" Width="360" Height="200">
    <VBox Spacing="10" Slot.Padding="16">
        <Label Name="title" Text="Hello from D" TextSize="20"/>
        <TextInput Name="name" Placeholder="Your name"/>
        <HBox Spacing="8">
            <Button Name="greet" Text="Greet" Style="Primary" OnClicked="greet" Slot.Fill="1"/>
            <Button Text="Quit" OnClicked="quit"/>
        </HBox>
    </VBox>
</Window>`;

void check(bool ok, string what, size_t line = __LINE__)
{
    if (!ok)
    {
        writeln("d binding: failed at line ", line, ": ", what);
        exit(1);
    }
}

void main(string[] args)
{
    bool test = args.canFind("--test");
    RInitDesc desc;
    desc.backend = test ? R_BACKEND_NULL : R_BACKEND_VULKAN;
    desc.headless = test;
    if (!Rinit_ex(&desc))
    {
        writeln("cannot start Raven");
        exit(1);
    }

    Rid window = windowFromXml(xml);
    Rid title = widgetFind(window, "title");
    Rid name = widgetFind(window, "name");
    int clicks = 0;

    actionBind("greet", (Rid sender, string action) {
        clicks++;
        Rlabel_set_text(title, toStringz("Hello, " ~ text(Rtext_input_get_text(name)) ~ "!"));
    });
    actionBind("quit", (Rid sender, string action) { Rquit(0); });

    if (!test)
    {
        while (Rwait_frame(-1)) {}
        Rshutdown();
        return;
    }

    Rdraw_frame();
    check(text(Rwindow_get_title(window)) == "Raven from D", "the title of the window");
    check(Rwidget_get_type(title) == R_WIDGET_LABEL, "the type of a widget");
    Rtext_input_set_text(name, "Zoé");
    float x, y, width, height;
    Rwidget_get_rect(widgetFind(window, "greet"), &x, &y, &width, &height);
    check(width > 0 && height > 0, "the button is laid out");
    Rwindow_inject_mouse_move(window, x + width / 2, y + height / 2);
    Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 1);
    Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 0);
    check(clicks == 1, "the click calls the delegate");
    check(text(Rlabel_get_text(title)) == "Hello, Zoé!", "the delegate changed the label");

    // structures, and a mistake described in one
    RMemoryStats stats;
    Rmemory_get_stats(&stats);
    check(stats.allocation_count > 0, "a structure filled by the library");
    RXmlError error;
    string broken = "<A><B></A>";
    check(Rxml_document_parse(broken.ptr, broken.length, &error) == R_INVALID_ID, "a broken document");
    check(error.line == 1, "the line of the mistake");
    check(text(error.message).canFind("does not match"), "the message of the mistake");

    // documents
    Rid document = Rxml_document_create("VBox");
    Rid root = Rxml_document_get_root(document);
    Rid label = Rxml_node_add_child(root, "Label", -1);
    Rxml_node_set_attribute(label, "Text", "a < b");
    check(text(Rxml_document_to_string(document)).canFind(`Text="a &lt; b"`), "a document written as XML");
    Rid built = Rxml_document_create_widget(document);
    check(Rwidget_get_xml_node(Rwidget_get_child(built, 0)) == label, "the widgets of a document");
    Rwidget_destroy(built);
    Rxml_document_destroy(document);

    // a timer, and the errors of the library
    int fired = 0;
    timerStart(0, false, () { fired++; });
    string[] errors;
    errorSetCallback((RError code, RSeverity severity, string detail) { errors ~= detail; });
    Rwidget_destroy(R_INVALID_ID);
    check(errors.length > 0, "the error callback");
    errorSetCallback(null);
    for (int frame = 0; frame < 10 && fired == 0; frame++)
        Rdraw_frame();
    check(fired == 1, "the timer");

    Raction_fire("quit", R_INVALID_ID);
    check(!Ris_running(), "an action fired by its name");
    Rshutdown();
    writeln("d binding: ok");
}

// A window described in XML, driven from Java.
//
//   javac -d classes src/main/java/raven/R.java
//   RAVEN_LIBRARY=/path/to/libraven.so java --enable-native-access=ALL-UNNAMED -cp classes example/Example.java
//
// `--test` runs without a window system and without a GPU, clicks its own
// button and checks what happened: it is how the build of Raven checks this
// binding.

import static raven.R.*;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class Example {
    static final String XML = """
        <Window Title="Raven from Java" Width="360" Height="200">
            <VBox Spacing="10" Slot.Padding="16">
                <Label Name="title" Text="Hello from Java" TextSize="20"/>
                <TextInput Name="name" Placeholder="Your name"/>
                <HBox Spacing="8">
                    <Button Name="greet" Text="Greet" Style="Primary" OnClicked="greet" Slot.Fill="1"/>
                    <Button Text="Quit" OnClicked="quit"/>
                </HBox>
            </VBox>
        </Window>
        """;

    static void check(boolean condition, Object what) {
        if (!condition) throw new AssertionError(String.valueOf(what));
    }

    public static void main(String[] args) throws Exception {
        boolean test = Arrays.asList(args).contains("--test");
        int started = test
            ? Rinit_ex(new RInitDesc().backend(R_BACKEND_NULL).headless(1))
            : Rinit(R_BACKEND_VULKAN);
        if (started == 0) {
            System.out.println("cannot start Raven");
            System.exit(1);
        }

        List<String> errors = new ArrayList<>();
        List<Integer> codes = new ArrayList<>();
        Rerror_set_callback((code, severity, detail) -> { codes.add(code); errors.add(detail); });

        int window = Rwindow_create_from_xml(XML);
        int title = Rwidget_find(window, "title");
        int name = Rwidget_find(window, "name");
        List<String> clicks = new ArrayList<>();

        Raction_bind("greet", (sender, action, user) -> {
            clicks.add(action);
            String who = Rtext_input_get_text(name);
            Rlabel_set_text(title, "Hello, " + (who == null || who.isEmpty() ? "you" : who) + "!");
        });
        Raction_bind("quit", (sender, action, user) -> Rquit(0));

        if (!test) {
            while (Rwait_frame(-1.0) != 0) { }
            Rshutdown();
            return;
        }

        // ---- the binding, checked
        Rdraw_frame();
        check(Rwindow_get_title(window).equals("Raven from Java"), Rwindow_get_title(window));
        check(Rwidget_get_type(title) == R_WIDGET_LABEL, "type");
        Rtext_input_set_text(name, "Zoé");
        float[] x = {0}, y = {0}, width = {0}, height = {0};
        Rwidget_get_rect(Rwidget_find(window, "greet"), x, y, width, height);
        check(width[0] > 0 && height[0] > 0, "rect");
        Rwindow_inject_mouse_move(window, x[0] + width[0] / 2, y[0] + height[0] / 2);
        Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 1);
        Rwindow_inject_mouse_button(window, R_MOUSE_LEFT, 0);
        check(clicks.equals(List.of("greet")), clicks);
        check(Rlabel_get_text(title).equals("Hello, Zoé!"), Rlabel_get_text(title));
        // values written through pointers
        int[] w = {0}, h = {0};
        Rwindow_get_size(window, w, h);
        check(w[0] == 360 && h[0] == 200, w[0] + "x" + h[0]);
        // structures
        RMemoryStats stats = new RMemoryStats();
        Rmemory_get_stats(stats);
        check(stats.allocation_count() > 0, "stats");
        // documents, and a mistake reported in a structure
        RXmlError error = new RXmlError();
        check(Rxml_document_parse("<A><B></A>", error) == R_INVALID_ID, "parse");
        check(error.line() == 1 && error.message().contains("does not match"), error.message());
        int document = Rxml_document_parse("<VBox><Label Text='x'/></VBox>", null);
        int root = Rxml_document_get_root(document);
        check(Rxml_node_get_tag(root).equals("VBox") && Rxml_node_get_child_count(root) == 1, "tree");
        check(Rxml_node_get_attribute(root, "Nope") == null, "attribute");
        Rxml_node_set_attribute(Rxml_node_get_child(root, 0), "Text", "é & <");
        check(Rxml_document_to_string(document).contains("Text=\"é &amp; &lt;\""), Rxml_document_to_string(document));
        // a callback that returns a text
        int listed = Rwidget_create(R_WIDGET_LIST_VIEW);
        Rlist_view_set_virtual_items(listed, 3, (view, index, user) -> "row " + index);
        check(Rlist_view_get_item_text(listed, 2).equals("row 2"), Rlist_view_get_item_text(listed, 2));
        Rwidget_destroy(listed);
        // an error of the caller reaches the callback as text
        Rwidget_set_tooltip(0x12345, "x");
        check(!errors.isEmpty() && codes.get(codes.size() - 1) == R_ERROR_INVALID_ID
            && errors.get(errors.size() - 1).contains("Rwidget_set_tooltip"), errors);
        // files: bytes in, bytes out
        check(Rfile_write("raven_java_test.bin", new byte[] {'a', 0, 'b'}) != 0, "write");
        check(Arrays.equals(Rfile_read("raven_java_test.bin"), new byte[] {'a', 0, 'b'}), "read");
        Files.delete(Path.of("raven_java_test.bin"));
        Raction_fire("quit", R_INVALID_ID);
        check(Ris_running() == 0, "running");
        Rshutdown();
        System.out.println("java binding: ok");
    }
}

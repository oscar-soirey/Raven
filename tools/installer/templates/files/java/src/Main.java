// The program. Its interface is main.xml, loaded when it starts:
// save main.xml while it runs and the window follows.
import static raven.R.*;

public class Main {
    static int window;
    static int clicks;

    public static void main(String[] args) {
        if (Rinit(R_BACKEND_VULKAN) == 0) System.exit(1);
        Rlocale_load_csv("strings.csv");
        Rlocale_set_language(Rlocale_get_system_language());
        // OnClicked="hello.clicked" in main.xml fires this action
        Raction_bind("hello.clicked", (sender, action, user) -> {
            clicks++;
            // the widget with Name="status"
            Rlabel_set_text(Rwidget_find(window, "status"), Rtr_plural("clicks", clicks));
        });
        window = Rwindow_create_from_xml_file("main.xml");
        while (Rdraw_frame() != 0) { }
        Rshutdown();
    }
}

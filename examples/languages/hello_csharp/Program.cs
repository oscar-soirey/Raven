// hello_csharp: the program. Its interface is main.xml, loaded when it starts:
// save main.xml while it runs and the window follows. F5 builds and runs it.
using System;
using Raven;

public static class Program
{
    static uint window;
    static int clicks;

    // OnClicked="hello.clicked" in main.xml fires this action
    static void OnHello(uint sender, IntPtr action, IntPtr userData)
    {
        clicks++;
        // the widget with Name="status"
        R.Rlabel_set_text(R.Rwidget_find(window, "status"), R.Rtr_plural("clicks", clicks));
    }

    public static int Main()
    {
        if (R.Rinit(RBackend.R_BACKEND_VULKAN) == 0) return 1;
        R.Rlocale_load_csv("strings.csv");
        R.Rlocale_set_language(R.Rlocale_get_system_language());
        R.Raction_bind("hello.clicked", R.Keep<RActionCallback>(OnHello), IntPtr.Zero);
        window = R.Rwindow_create_from_xml_file("main.xml");
        while (R.Rdraw_frame() != 0) { }
        R.Rshutdown();
        return 0;
    }
}

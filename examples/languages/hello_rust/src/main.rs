// hello_rust: the program. Its interface is main.xml, loaded when it starts:
// save main.xml while it runs and the window follows. F5 builds and runs it.
use raven::*;

fn main() {
    unsafe {
        if Rinit(R_BACKEND_VULKAN) == 0 {
            return;
        }
        Rlocale_load_csv(cstr("strings.csv").as_ptr());
        Rlocale_set_language(Rlocale_get_system_language());
    }
    let window = unsafe { Rwindow_create_from_xml_file(cstr("main.xml").as_ptr()) };
    let mut clicks = 0;
    // OnClicked="hello.clicked" in main.xml fires this action
    action_bind("hello.clicked", move |_sender, _action| {
        clicks += 1;
        // the widget with Name="status"
        unsafe { Rlabel_set_text(widget_find(window, "status"), Rtr_plural(cstr("clicks").as_ptr(), clicks)) };
    });
    unsafe {
        while Rdraw_frame() != 0 {}
        Rshutdown();
    }
}

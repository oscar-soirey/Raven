// hello_zig: the program. Its interface is main.xml, loaded when it starts:
// save main.xml while it runs and the window follows. F5 builds and runs it.
const r = @import("raven");

var window: r.Rid = r.R_INVALID_ID;
var clicks: c_int = 0;

// OnClicked="hello.clicked" in main.xml fires this action
fn onHello(sender: r.Rid, action: ?[*:0]const u8, user_data: ?*anyopaque) callconv(.C) void {
    _ = sender;
    _ = action;
    _ = user_data;
    clicks += 1;
    // the widget with Name="status"
    r.Rlabel_set_text(r.Rwidget_find(window, "status"), r.Rtr_plural("clicks", clicks));
}

pub fn main() void {
    if (r.Rinit(r.R_BACKEND_VULKAN) == 0) return;
    _ = r.Rlocale_load_csv("strings.csv");
    _ = r.Rlocale_set_language(r.Rlocale_get_system_language());
    r.Raction_bind("hello.clicked", onHello, null);
    window = r.Rwindow_create_from_xml_file("main.xml");
    while (r.Rdraw_frame() != 0) {}
    r.Rshutdown();
}

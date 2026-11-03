# hello_python: the program. Its interface is main.xml, loaded when it starts:
# save main.xml while it runs and the window follows. F5 runs it.
import sys

import raven as r

window = r.R_INVALID_ID
clicks = 0


def on_hello(sender, action, user_data):
    """OnClicked="hello.clicked" in main.xml fires this action."""
    global clicks
    clicks += 1
    # the widget with Name="status"
    r.Rlabel_set_text(r.Rwidget_find(window, "status"), r.Rtr_plural("clicks", clicks))


if __name__ == "__main__":
    if not r.Rinit(r.R_BACKEND_VULKAN):
        sys.exit(1)
    r.Rlocale_load_csv("strings.csv")
    r.Rlocale_set_language(r.Rlocale_get_system_language())
    r.Raction_bind("hello.clicked", on_hello)
    window = r.Rwindow_create_from_xml_file("main.xml")
    while r.Rdraw_frame():
        pass
    r.Rshutdown()

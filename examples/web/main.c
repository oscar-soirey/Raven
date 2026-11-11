#include <Raven.h>
#include <raven/raven_emscripten.h>
#include <stdio.h>
#include <emscripten.h>
#include <string.h>

EM_JS(void, startup_error, (void), {
    const node = document.getElementById('status');
    if (node) {
        node.hidden = false;
        node.textContent = 'Raven could not start. This application requires WebGL 2.';
    }
})
static Rid input, status;
static int count;
static void clicked(Rid button, void* user) {
    char text[80];
    (void)button; (void)user;
    count++;
    snprintf(text, sizeof(text), "Button pressed %d time%s",
             count, count == 1 ? "" : "s");
    Rlabel_set_text(status, text);
}
static void imported(int success, const char* result, void* user) {
    (void)user;
    Rlabel_set_text(status, success ? result : "File import cancelled");
}
static void import_file(Rid button, void* user) {
    (void)button; (void)user;
    Rweb_import_file("", imported, NULL);
}
static void saved(int success, const char* result, void* user) {
    (void)result; (void)user;
    Rlabel_set_text(status, success ? "Saved in this browser" : "Save failed");
}
static void mounted(int success, const char* result, void* user) {
    (void)result; (void)user;
    if (success) {
        FILE* file = fopen("/persistent/note.txt", "wb");
        const char* text = Rtext_input_get_text(input);
        if (file) { fwrite(text, 1, strlen(text), file); fclose(file); }
        else { Rlabel_set_text(status, "Unable to write note"); return; }
        Rweb_sync_persistent(saved, NULL);
    } else Rlabel_set_text(status, "Browser storage unavailable");
}
static void save_note(Rid button, void* user) {
    (void)button; (void)user;
    Rweb_mount_persistent(mounted, NULL);
}
int main(void) {
    Rid window, frame, scroll, root, title, image, button, actions;
    const char* svg =
        "<svg xmlns='http://www.w3.org/2000/svg' width='96' height='96'>"
        "<rect width='96' height='96' rx='20' fill='#222a44'/>"
        "<path d='M20 66L34 24H62L76 66L48 50Z' fill='#849cff'/>"
        "<circle cx='48' cy='36' r='5' fill='white'/></svg>";
    if (!Rinit(0)) { startup_error(); return 1; }
    window = Rwindow_create("Raven in the browser", 800, 600, 0);
    frame = Rwidget_create(R_WIDGET_BORDER);
    Rborder_set_padding(frame, 24, 24, 24, 24);
    scroll = Rwidget_create(R_WIDGET_SCROLL_BOX);
    root = Rwidget_create(R_WIDGET_VBOX);
    Rpanel_set_spacing(root, 12);
    title = Rlabel_create("Raven in your browser");
    Rlabel_set_text_size(title, 26);
    Rwidget_add_child(root, title);
    Rwidget_add_child(root, Rlabel_create("The same C widgets, rendered with WebGL 2."));
    image = Rwidget_create(R_WIDGET_IMAGE);
    Rimage_set_texture(image, Rtexture_create(svg, strlen(svg)));
    Rwidget_set_size(image, 96, 96);
    Rwidget_add_child(root, image);
    input = Rwidget_create(R_WIDGET_TEXT_INPUT);
    Rwidget_set_accessible_name(input, "Your note");
    Rtext_input_set_text(input, "Edit this note, then save it in this browser.");
    Rwidget_add_child(root, input);
    actions = Rwidget_create(R_WIDGET_HBOX);
    Rpanel_set_spacing(actions, 8);
    button = Rbutton_create("Click me");
    Rbutton_set_clicked_callback(button, clicked, NULL);
    Rwidget_add_child(actions, button);
    button = Rbutton_create("Import a file");
    Rbutton_set_clicked_callback(button, import_file, NULL);
    Rwidget_add_child(actions, button);
    button = Rbutton_create("Save note");
    Rbutton_set_clicked_callback(button, save_note, NULL);
    Rwidget_add_child(actions, button);
    Rwidget_add_child(root, actions);
    status = Rlabel_create("Ready. Try the controls, resize the page, or scroll.");
    Rwidget_add_child(root, status);
    for (int i = 1; i <= 24; i++) {
        char text[96];
        snprintf(text, sizeof(text), "Scroll row %02d — mouse wheel, trackpad, or touch", i);
        Rwidget_add_child(root, Rlabel_create(text));
    }
    Rwidget_add_child(scroll, root);
    Rwidget_add_child(frame, scroll);
    Rwindow_set_content(window, frame);
    Rwindow_show(window);
    Rweb_start();
    return 0;
}

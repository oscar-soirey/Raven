/* Browser integration fixture: real widgets and GPU capture, no injected UI input. */
#include <Raven.h>
#include <raven/raven_emscripten.h>
#include <emscripten.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GLES3/gl3.h>

static Rid heading, window, input, button, image, texture = R_INVALID_ID;
static Rid secondary = R_INVALID_ID, border, import_button, scroll, editor;
static int clicks, resets, errors, io_status, drops;
static char io_result[256];

static void clicked(Rid id, void* user) {
    char text[64];
    (void)id; (void)user;
    snprintf(text, sizeof(text), "Clicks: %d", ++clicks);
    Rbutton_set_text(button, text);
}
static void reset(void* user) { (void)user; resets++; }
static void error(RError code, RSeverity severity, const char* text) {
    (void)code;
    if (severity >= R_SEVERITY_ERROR) { errors++; fprintf(stderr, "%s\n", text); }
}
static void io_done(int success, const char* result, void* user) {
    (void)user; io_status = success ? 1 : -1;
    snprintf(io_result, sizeof(io_result), "%s", result);
}
EMSCRIPTEN_KEEPALIVE void test_unfocused_label(void) {
    Rlabel_set_text(heading, "Updated without focus");
}
EMSCRIPTEN_KEEPALIVE const char* test_text(void) { return Rtext_input_get_text(input); }
EMSCRIPTEN_KEEPALIVE int test_clicks(void) { return clicks; }
EMSCRIPTEN_KEEPALIVE int test_resets(void) { return resets; }
EMSCRIPTEN_KEEPALIVE int test_errors(void) { return errors; }
EMSCRIPTEN_KEEPALIVE int test_io_status(void) { return io_status; }
EMSCRIPTEN_KEEPALIVE const char* test_io_result(void) { return io_result; }
EMSCRIPTEN_KEEPALIVE int test_width(void) {
    int width; Rwindow_get_framebuffer_size(window, &width, NULL); return width;
}
EMSCRIPTEN_KEEPALIVE void test_persist(int save) {
    io_status = 0;
    if (save) {
        FILE* f = fopen("/persistent/check.txt", "wb");
        if (f) { fwrite("saved", 1, 5, f); fclose(f); }
        Rweb_sync_persistent(io_done, NULL);
    } else Rweb_mount_persistent(io_done, NULL);
}
EMSCRIPTEN_KEEPALIVE int test_saved(void) {
    char text[6] = {0};
    FILE* f = fopen("/persistent/check.txt", "rb");
    if (!f) return 0;
    (void)fread(text, 1, 5, f); fclose(f);
    return strcmp(text, "saved") == 0;
}
EMSCRIPTEN_KEEPALIVE void test_import(void) {
    io_status = 0; Rweb_import_file(".txt", io_done, NULL);
}
static void import_clicked(Rid id, void* user) {
    (void)id; (void)user; test_import();
}
EMSCRIPTEN_KEEPALIVE double test_import_y(void) {
    float x,y,w,h; Rwidget_get_rect(import_button,&x,&y,&w,&h); return y+h/2;
}
EMSCRIPTEN_KEEPALIVE int test_import_contents(void) {
    char text[9] = {0};
    FILE* file = fopen(io_result, "rb");
    if (!file) return 0;
    (void)fread(text, 1, 8, file); fclose(file);
    return strcmp(text, "imported") == 0;
}
EMSCRIPTEN_KEEPALIVE void test_download(void) {
    FILE* f = fopen("/export.txt", "wb");
    if (f) { fwrite("exported", 1, 8, f); fclose(f); }
    Rweb_download_file("/export.txt", "raven.txt");
}
EMSCRIPTEN_KEEPALIVE void test_image(const char* url) {
    texture = Rtexture_create_from_file_async(url);
    Rimage_set_texture(image, texture);
}
EMSCRIPTEN_KEEPALIVE int test_image_ready(void) {
    int w = 0, h = 0;
    if (texture == R_INVALID_ID || !Rtexture_is_ready(texture)) return 0;
    Rtexture_get_size(texture, &w, &h); return w > 0 && h > 0;
}
static void primitives(Rid widget, RPainter* p, float w, float h, void* user) {
    (void)widget; (void)w; (void)h; (void)user;
    Rdraw_rect(p,10,10,60,40,0,1,0,0,1);
    Rdraw_line(p,100,30,160,30,10,0,1,0,1);
    Rdraw_triangle(p,10,80,70,80,40,130,0,0,1,1);
    Rdraw_push_clip_rect(p,100,80,20,50);
    Rdraw_rect(p,100,80,60,50,0,1,1,0,1);
    Rdraw_pop_clip_rect(p);
    Rdraw_rect(p,10,160,120,50,0,0,0,1,1);
    Rdraw_glass(p,20,170,80,30,4,0,0,0,0);
}
static void viewport_draw(const RViewportDrawInfo* info, void* user) {
    (void)info; (void)user;
    glClearColor(0,0,0,1); glClear(GL_COLOR_BUFFER_BIT);
    /* Raven must restore UI state after application drawing. */
    glEnable(GL_RASTERIZER_DISCARD);
    glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
    glScissor(0,0,1,1);
    {
        const GLenum none = GL_NONE;
        glDrawBuffers(1, &none);
        glReadBuffer(GL_NONE);
    }
}
EMSCRIPTEN_KEEPALIVE double test_button_y(void) {
    float x,y,w,h; Rwidget_get_rect(button,&x,&y,&w,&h); return y+h/2;
}
EMSCRIPTEN_KEEPALIVE int test_pixels(void) {
    Rid test = Rwindow_create("Pixels",320,240,0);
    Rid custom = Rwidget_create(R_WIDGET_CUSTOM);
    Rid overlay = Rwidget_create(R_WIDGET_OVERLAY);
    Rid viewport = Rwidget_create(R_WIDGET_VIEWPORT);
    int width=0,height=0, result=1;
    const int points[][5] = {
        {30,30,255,0,0}, {130,30,0,255,0}, {40,95,0,0,255},
        {110,100,255,255,0}, {140,100,0,0,0}, {60,185,0,0,255}
    };
    uint8_t* pixels;
    float scale = Rwindow_get_scale(test);
    size_t i;
    Rwindow_set_clear_color(test,0,0,0,1);
    Rcustom_set_paint_callback(custom,primitives,NULL);
    Rviewport_set_draw_callback(viewport,viewport_draw,NULL);
    Rwidget_add_child(overlay,viewport); Rwidget_add_child(overlay,custom);
    Rwindow_set_content(test,overlay); Rwindow_show(test);
    pixels=Rwindow_capture(test,&width,&height);
    if (!pixels) result=0;
    else for (i=0;i<sizeof(points)/sizeof(points[0]);i++) {
        int x=(int)(points[i][0]*scale), y=(int)(points[i][1]*scale), c;
        if (x>=width || y>=height) { result=0; break; }
        for(c=0;c<3;c++) {
            int actual=pixels[((size_t)y*(size_t)width+(size_t)x)*4+(size_t)c];
            if (abs(actual-points[i][c+2])>20) result=0;
        }
    }
    Rpixels_free(pixels); Rwindow_destroy(test); return result;
}
EMSCRIPTEN_KEEPALIVE double test_scroll_y(void) {
    float x,y,w,h; Rwidget_get_rect(scroll,&x,&y,&w,&h); return y+h/2;
}
EMSCRIPTEN_KEEPALIVE double test_scroll_offset(void) {
    float y=0; Rscroll_box_get_offset(scroll,NULL,&y); return y;
}
static int dropped(Rid id, const RDropEvent* event, void* user) {
    (void)id; (void)user;
    if(event->phase==R_DROP_DROPPED) drops++;
    return R_TRUE;
}
EMSCRIPTEN_KEEPALIVE int test_drops(void) { return drops; }
EMSCRIPTEN_KEEPALIVE int test_capture(void) {
    int w = 0, h = 0;
    uint8_t* pixels = Rwindow_capture(window, &w, &h);
    int result = pixels && w > 0 && h > 0;
    if (pixels) Rpixels_free(pixels);
    return result;
}
EMSCRIPTEN_KEEPALIVE void test_effect(int index) {
    Rid shader;
    RShaderLook look;
    if (index < 0)
        shader = Rweb_shader_create(
            "vec4 raven_effect(RavenEffect fx) { return vec4(0,1,0,1); }", 0);
    else shader = Reffect_get((REffect)(R_EFFECT_AURORA + index));
    Rshader_look_defaults(shader, &look);
    Rborder_set_effect(border, shader, &look);
}
EMSCRIPTEN_KEEPALIVE int test_secondary(int close) {
    if (close) {
        Rwindow_destroy(secondary); secondary = R_INVALID_ID; return 1;
    }
    secondary = Rwindow_create("Second window", 280, 180, 0);
    if (secondary == R_INVALID_ID) return 0;
    Rwindow_set_content(secondary, Rlabel_create("Second canvas"));
    Rwindow_set_position(secondary, 400, 200);
    Rwindow_show(secondary); return 1;
}
EMSCRIPTEN_KEEPALIVE void test_multiline(int close) {
    if (close) { Rwindow_destroy(secondary); secondary=R_INVALID_ID; return; }
    secondary=Rwindow_create("Editor",300,200,0);
    editor=Rwidget_create(R_WIDGET_TEXT_EDITOR);
    Rwidget_set_accessible_name(editor,"Multiline editor");
    Rtext_editor_set_text(editor,"line one");
    Rwindow_set_content(secondary,editor); Rwindow_show(secondary);
}
EMSCRIPTEN_KEEPALIVE const char* test_multiline_text(void) {
    return Rtext_editor_get_text(editor);
}
EMSCRIPTEN_KEEPALIVE void test_secondary_state(int state) {
    if(state==1) Rwindow_maximize(secondary);
    else if(state==2) Rwindow_minimize(secondary);
    else if(state==3) Rwindow_begin_move(secondary);
    else Rwindow_restore(secondary);
}
EMSCRIPTEN_KEEPALIVE void test_quit(void) { Rquit(0); }

int main(void) {
    Rid root, blur, label;
    Rerror_set_callback(error);
    if (!Rinit(0)) return 1;
    Rset_device_reset_callback(reset, NULL);
    window = Rwindow_create("Web integration", 800, 600, 0);
    root = Rwidget_create(R_WIDGET_VBOX);
    Rpanel_set_spacing(root, 8);
    label = Rlabel_create("Raven browser integration"); heading = label;
    input = Rwidget_create(R_WIDGET_TEXT_INPUT);
    Rwidget_set_accessible_name(input, "Test input");
    Rtext_input_set_text(input, "initial");
    button = Rbutton_create("Clicks: 0");
    Rbutton_set_clicked_callback(button, clicked, NULL);
    border = Rwidget_create(R_WIDGET_BORDER);
    Rborder_set_padding(border, 12, 12, 12, 12);
    Rwidget_add_child(border, Rlabel_create("Effect panel"));
    image = Rwidget_create(R_WIDGET_IMAGE);
    blur = Rbackground_blur_create();
    Rwidget_add_child(blur, Rlabel_create("Backdrop panel"));
    Rwidget_add_child(root, label);
    Rwidget_add_child(root, input);
    Rwidget_add_child(root, button);
    Rwidget_add_child(root, border);
    Rwidget_add_child(root, blur);
    import_button = Rbutton_create("Import");
    Rbutton_set_clicked_callback(import_button, import_clicked, NULL);
    Rwidget_add_child(root, import_button);
    Rwidget_add_child(root, image);
    scroll = Rwidget_create(R_WIDGET_SCROLL_BOX);
    Rwidget_set_size(scroll,0,100);
    for (int i=0;i<30;i++) Rwidget_add_child(scroll,Rlabel_create("Scroll row"));
    Rwidget_add_child(root,scroll);
    Rwindow_set_drop_callback(window,dropped,NULL);
    Rwindow_set_content(window, root);
    Rwindow_show(window);
    Rweb_start();
    return 0;
}

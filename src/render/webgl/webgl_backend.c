/* WebGL 2 renderer. All GL calls run on the browser's UI thread. */
#include "../renderer.h"
#include "../../core/context.h"
#include "raven/raven_emscripten.h"
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <stddef.h>
#include "shaders.h"
#include "effects.h"

typedef struct rvn_gl_surface_t {
    Rid window;
    uintptr_t native;
    struct rvn_gl_surface_t* next;
} rvn_gl_surface_t;

typedef struct rvn_gl_texture_t {
    GLuint name;
    GLenum format;
    RFilterType filter;
} rvn_gl_texture_t;

static struct {
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
    GLuint program, vao, buffer, white;
    GLuint backdrop, blur[2], blur_fbo, blur_program;
    uint32_t width, height;
    rvn_gl_texture_t* textures;
    uint32_t capacity;
    GLuint* shaders;
    uint32_t shader_capacity;
    struct rvn_gl_surface_t* surfaces;
} g_gl;

static GLuint rvn_gl_compile(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    GLint ok;
    char log[2048];
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok) return shader;
    glGetShaderInfoLog(shader, sizeof(log), NULL, log);
    rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "WebGL shader: %s", log);
    glDeleteShader(shader);
    return 0;
}

static void rvn_gl_shutdown(void);

static int rvn_gl_init(const RBackendInitInfo* info) {
    EmscriptenWebGLContextAttributes attrs;
    GLuint vert, frag;
    GLint ok;
    uint32_t white = 0xffffffffu;
    RVN_UNUSED(info);
    memset(&g_gl, 0, sizeof(g_gl));
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.majorVersion = 2;
    attrs.alpha = EM_TRUE;
    attrs.depth = EM_FALSE;
    attrs.stencil = EM_FALSE;
    attrs.antialias = EM_FALSE;
    attrs.premultipliedAlpha = EM_TRUE;
    g_gl.context = emscripten_webgl_create_context("#raven-render-canvas", &attrs);
    if (g_gl.context <= 0) return R_FALSE;
    emscripten_webgl_make_context_current(g_gl.context);
    vert = rvn_gl_compile(GL_VERTEX_SHADER, rvn_webgl_vert);
    frag = rvn_gl_compile(GL_FRAGMENT_SHADER, rvn_webgl_frag);
    if (!vert || !frag) {
        glDeleteShader(vert);
        glDeleteShader(frag);
        emscripten_webgl_destroy_context(g_gl.context);
        memset(&g_gl, 0, sizeof(g_gl));
        return R_FALSE;
    }
    g_gl.program = glCreateProgram();
    glAttachShader(g_gl.program, vert);
    glAttachShader(g_gl.program, frag);
    glLinkProgram(g_gl.program);
    glDeleteShader(vert);
    glDeleteShader(frag);
    glGetProgramiv(g_gl.program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(g_gl.program, sizeof(log), NULL, log);
        rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR, "WebGL link: %s", log);
        glDeleteProgram(g_gl.program);
        emscripten_webgl_destroy_context(g_gl.context);
        memset(&g_gl, 0, sizeof(g_gl));
        return R_FALSE;
    }
    {
        const char* vs =
            "#version 300 es\n"
            "out vec2 uv; void main() {"
            "vec2 p = vec2(float((gl_VertexID << 1) & 2),"
            "float(gl_VertexID & 2)); uv=p;"
            "gl_Position=vec4(p*2.0-1.0,0,1);}";
        const char* fs =
            "#version 300 es\nprecision highp float;\n"
            "in vec2 uv; uniform sampler2D image; uniform vec2 step_size;"
            "out vec4 color; void main() {"
            "color=texture(image,uv)*0.227027;"
            "color+=(texture(image,uv+step_size*1.384615)"
            "+texture(image,uv-step_size*1.384615))*0.316216;"
            "color+=(texture(image,uv+step_size*3.230769)"
            "+texture(image,uv-step_size*3.230769))*0.070270;}";
        GLuint a = rvn_gl_compile(GL_VERTEX_SHADER, vs);
        GLuint b = rvn_gl_compile(GL_FRAGMENT_SHADER, fs);
        g_gl.blur_program = glCreateProgram();
        glAttachShader(g_gl.blur_program, a);
        glAttachShader(g_gl.blur_program, b);
        glLinkProgram(g_gl.blur_program);
        glDeleteShader(a); glDeleteShader(b);
        glGetProgramiv(g_gl.blur_program, GL_LINK_STATUS, &ok);
        if (!ok) { rvn_gl_shutdown(); return R_FALSE; }
        glGenTextures(1, &g_gl.backdrop);
        glGenTextures(2, g_gl.blur);
        glGenFramebuffers(1, &g_gl.blur_fbo);
    }
    glGenVertexArrays(1, &g_gl.vao);
    glGenBuffers(1, &g_gl.buffer);
    glGenTextures(1, &g_gl.white);
    glBindTexture(GL_TEXTURE_2D, g_gl.white);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, &white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return R_TRUE;
}

/* Release names while the context is still lost, before restoration invalidates
 * the old WebGL object identities. Emscripten also drops its JS table entries. */
static void rvn_gl_release_objects(void) {
    uint32_t i;
    if (g_gl.context <= 0) return;
    emscripten_webgl_make_context_current(g_gl.context);
    for (i = 0; i < g_gl.capacity; i++) {
        if (g_gl.textures[i].name) glDeleteTextures(1, &g_gl.textures[i].name);
        g_gl.textures[i].name = 0;
    }
    for (i = 0; i < g_gl.shader_capacity; i++) {
        if (g_gl.shaders[i]) glDeleteProgram(g_gl.shaders[i]);
        g_gl.shaders[i] = 0;
    }
    glDeleteTextures(1, &g_gl.backdrop);
    glDeleteTextures(2, g_gl.blur);
    glDeleteFramebuffers(1, &g_gl.blur_fbo);
    glDeleteProgram(g_gl.blur_program);
    glDeleteTextures(1, &g_gl.white);
    glDeleteBuffers(1, &g_gl.buffer);
    glDeleteVertexArrays(1, &g_gl.vao);
    glDeleteProgram(g_gl.program);
    g_gl.backdrop = g_gl.blur[0] = g_gl.blur[1] = g_gl.blur_fbo = 0;
    g_gl.blur_program = g_gl.white = g_gl.buffer = g_gl.vao = g_gl.program = 0;
}

void rvn_webgl_context_lost(void) { rvn_gl_release_objects(); }

static void rvn_gl_shutdown(void) {
    rvn_gl_release_objects();
    while (g_gl.surfaces) {
        rvn_gl_surface_t* surface = g_gl.surfaces;
        g_gl.surfaces = surface->next; rvn_free(surface);
    }
    rvn_free(g_gl.textures);
    rvn_free(g_gl.shaders);
    if (g_gl.context > 0) emscripten_webgl_destroy_context(g_gl.context);
    memset(&g_gl, 0, sizeof(g_gl));
}

static int rvn_gl_surface(Rid window, const RNativeWindow* native,
                          Ruint width, Ruint height) {
    rvn_gl_surface_t* surface = rvn_calloc(sizeof(*surface));
    RVN_UNUSED(width); RVN_UNUSED(height);
    surface->window = window; surface->native = native->window;
    surface->next = g_gl.surfaces; g_gl.surfaces = surface;
    return R_TRUE;
}

static void rvn_gl_destroy_surface(Rid window) {
    rvn_gl_surface_t** link = &g_gl.surfaces;
    while (*link && (*link)->window != window) link = &(*link)->next;
    if (*link) {
        rvn_gl_surface_t* surface = *link;
        *link = surface->next; rvn_free(surface);
    }
}

static rvn_gl_texture_t* rvn_gl_texture(Rid id) {
    uint32_t index = R_ID_INDEX(id);
    return index < g_gl.capacity ? &g_gl.textures[index] : NULL;
}

static void rvn_gl_filter(Rid id, RFilterType filter) {
    rvn_gl_texture_t* texture = rvn_gl_texture(id);
    GLint value = filter == R_FILTER_NEAREST ? GL_NEAREST : GL_LINEAR;
    if (!texture || !texture->name) return;
    texture->filter = filter;
    glBindTexture(GL_TEXTURE_2D, texture->name);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    filter == R_FILTER_MIPMAP ? GL_LINEAR_MIPMAP_LINEAR : value);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, value);
    if (filter == R_FILTER_MIPMAP) glGenerateMipmap(GL_TEXTURE_2D);
}

static int rvn_gl_create_texture(Rid id, const RTextureDesc* desc,
                                const void* pixels) {
    uint32_t index = R_ID_INDEX(id);
    rvn_gl_texture_t* texture;
    if (index >= g_gl.capacity) {
        uint32_t capacity = index + 64;
        rvn_gl_texture_t* items = rvn_calloc(capacity * sizeof(*items));
        if (g_gl.capacity)
            memcpy(items, g_gl.textures, g_gl.capacity * sizeof(*items));
        rvn_free(g_gl.textures);
        g_gl.textures = items;
        g_gl.capacity = capacity;
    }
    texture = &g_gl.textures[index];
    if (texture->name) glDeleteTextures(1, &texture->name);
    glGenTextures(1, &texture->name);
    texture->format = desc->format == R_TEXTURE_ALPHA8 ? GL_RED : GL_RGBA;
    glBindTexture(GL_TEXTURE_2D, texture->name);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0,
                 texture->format == GL_RED ? GL_R8 : GL_RGBA8,
                 (GLsizei)desc->width, (GLsizei)desc->height, 0,
                 texture->format, GL_UNSIGNED_BYTE, pixels);
    /* WebGL has no texture swizzles: the fragment shader handles glyph masks. */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    rvn_gl_filter(id, desc->filter);
    return glGetError() == GL_NO_ERROR;
}

static void rvn_gl_update_texture(Rid id, Ruint x, Ruint y,
                                  Ruint width, Ruint height,
                                  const void* pixels) {
    rvn_gl_texture_t* texture = rvn_gl_texture(id);
    if (!texture || !texture->name) return;
    glBindTexture(GL_TEXTURE_2D, texture->name);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, (GLint)x, (GLint)y,
                    (GLsizei)width, (GLsizei)height,
                    texture->format, GL_UNSIGNED_BYTE, pixels);
    if (texture->filter == R_FILTER_MIPMAP) glGenerateMipmap(GL_TEXTURE_2D);
}

static void rvn_gl_destroy_texture(Rid id) {
    rvn_gl_texture_t* texture = rvn_gl_texture(id);
    if (!texture) return;
    glDeleteTextures(1, &texture->name);
    memset(texture, 0, sizeof(*texture));
}

/* WebGL 2 has no base-instance draw: shift the attributes per command. */
static void rvn_gl_attributes(uint32_t first) {
    const size_t offsets[] = {
        offsetof(RDrawInstance, rect), offsetof(RDrawInstance, uv),
        offsetof(RDrawInstance, clip), offsetof(RDrawInstance, radii),
        offsetof(RDrawInstance, color), offsetof(RDrawInstance, border_color),
        offsetof(RDrawInstance, border_width), offsetof(RDrawInstance, flags)
    };
    GLuint i;
    for (i = 0; i < 8; i++) {
        const void* offset = (const void*)((size_t)first * sizeof(RDrawInstance)
                                           + offsets[i]);
        glEnableVertexAttribArray(i);
        if (i == 7)
            glVertexAttribIPointer(i, 1, GL_UNSIGNED_INT,
                                   sizeof(RDrawInstance), offset);
        else
            glVertexAttribPointer(i, i == 6 ? 1 : 4,
                                  (i == 4 || i == 5) ? GL_UNSIGNED_BYTE : GL_FLOAT,
                                  (i == 4 || i == 5) ? GL_TRUE : GL_FALSE,
                                  sizeof(RDrawInstance), offset);
        glVertexAttribDivisor(i, 1);
    }
}

static void rvn_gl_state(const RDrawData* data, GLuint program) {
    emscripten_webgl_make_context_current(g_gl.context);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    {
        const GLenum back = GL_BACK;
        /* The default framebuffer remembers these across viewport callbacks. */
        glDrawBuffers(1, &back);
        glReadBuffer(GL_BACK);
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glViewport(0, 0, (GLsizei)data->framebuffer_width,
                (GLsizei)data->framebuffer_height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glDisable(GL_SAMPLE_COVERAGE);
    glDisable(GL_RASTERIZER_DISCARD);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glUseProgram(program);
    glUniform2f(glGetUniformLocation(program, "pc.scale"),
                2.0f / data->framebuffer_width,
                -2.0f / data->framebuffer_height);
    glUniform2f(glGetUniformLocation(program, "pc.translate"), -1, 1);
    glUniform2fv(glGetUniformLocation(program, "pc.pointer"), 1, data->pointer);
    glUniform1i(glGetUniformLocation(program, "u_texture"), 0);
    glUniform1i(glGetUniformLocation(program, "u_backdrop"), 1);
    glUniform1i(glGetUniformLocation(program, "u_backdrop_blur"), 2);
    glBindSampler(0, 0); glBindSampler(1, 0); glBindSampler(2, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_gl.backdrop);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, g_gl.blur[1]);
    glBindVertexArray(g_gl.vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_gl.buffer);
    glActiveTexture(GL_TEXTURE0);
}

static void rvn_gl_backdrop(const RDrawData* data) {
    unsigned i;
    GLuint names[] = {g_gl.backdrop, g_gl.blur[0], g_gl.blur[1]};
    if (g_gl.width != data->framebuffer_width ||
        g_gl.height != data->framebuffer_height) {
        g_gl.width = data->framebuffer_width;
        g_gl.height = data->framebuffer_height;
        for (i = 0; i < 3; i++) {
            glBindTexture(GL_TEXTURE_2D, names[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)g_gl.width,
                         (GLsizei)g_gl.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_gl.backdrop);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
                        (GLsizei)g_gl.width, (GLsizei)g_gl.height);
    glBindFramebuffer(GL_FRAMEBUFFER, g_gl.blur_fbo);
    glDisable(GL_BLEND);
    glUseProgram(g_gl.blur_program);
    glUniform1i(glGetUniformLocation(g_gl.blur_program, "image"), 0);
    for (i = 0; i < 2; i++) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, g_gl.blur[i], 0);
        glBindTexture(GL_TEXTURE_2D, i ? g_gl.blur[0] : g_gl.backdrop);
        glUniform2f(glGetUniformLocation(g_gl.blur_program, "step_size"),
                     i ? 0 : 4.0f / g_gl.width,
                     i ? 4.0f / g_gl.height : 0);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    rvn_gl_state(data, g_gl.program);
}

static void rvn_gl_frame(const RDrawData* data, RCapture* capture) {
    uint32_t i;
    if (!data->framebuffer_width || !data->framebuffer_height ||
        emscripten_is_webgl_context_lost(g_gl.context)) return;
    {
        int width, height;
        emscripten_get_canvas_element_size("#raven-render-canvas", &width, &height);
        if (width != (int)data->framebuffer_width ||
            height != (int)data->framebuffer_height)
            emscripten_set_canvas_element_size("#raven-render-canvas",
                (int)data->framebuffer_width, (int)data->framebuffer_height);
    }
    emscripten_webgl_make_context_current(g_gl.context);
    rvn_gl_state(data, g_gl.program);
    glClearColor(data->clear_color[0], data->clear_color[1],
                  data->clear_color[2], data->clear_color[3]);
    glClear(GL_COLOR_BUFFER_BIT);
    glBufferData(GL_ARRAY_BUFFER,
                  data->instance_count * sizeof(RDrawInstance),
                  data->instances, GL_STREAM_DRAW);
    for (i = 0; i < data->command_count; i++) {
        const RDrawCommand* command = &data->commands[i];
        rvn_gl_texture_t* texture;
        GLuint program = g_gl.program;
        if (command->type == R_DRAW_CALLBACK) {
            RViewportDrawInfo info;
            if (!command->callback || command->rect[2] <= 0 ||
                command->rect[3] <= 0 || command->clip[2] <= 0 ||
                command->clip[3] <= 0) continue;
            memset(&info, 0, sizeof(info));
            info.backend = R_BACKEND_WEBGL;
            info.viewport = command->viewport; info.window = data->window;
            info.x = command->rect[0]; info.y = command->rect[1];
            info.width = command->rect[2]; info.height = command->rect[3];
            info.clip_x = rvn_maxi(command->clip[0], 0);
            info.clip_y = rvn_maxi(command->clip[1], 0);
            info.clip_width = rvn_mini(command->clip[0] + command->clip[2],
                (int)data->framebuffer_width) - info.clip_x;
            info.clip_height = rvn_mini(command->clip[1] + command->clip[3],
                (int)data->framebuffer_height) - info.clip_y;
            if (info.clip_width <= 0 || info.clip_height <= 0) continue;
            info.framebuffer_width = (int)data->framebuffer_width;
            info.framebuffer_height = (int)data->framebuffer_height;
            info.frame_index = data->frame_index;
            glViewport(info.x, info.framebuffer_height - info.y - info.height,
                        info.width, info.height);
            glEnable(GL_SCISSOR_TEST);
            glScissor(info.clip_x,
                       info.framebuffer_height - info.clip_y - info.clip_height,
                       info.clip_width, info.clip_height);
            if (command->callback) command->callback(&info, command->user_data);
            rvn_gl_state(data, g_gl.program);
            continue;
        }
        if (command->type == R_DRAW_BACKDROP) {
            rvn_gl_backdrop(data);
            continue;
        }
        if (command->type == R_DRAW_SHADER &&
            R_ID_INDEX(command->shader) < g_gl.shader_capacity &&
            g_gl.shaders[R_ID_INDEX(command->shader)])
            program = g_gl.shaders[R_ID_INDEX(command->shader)];
        rvn_gl_state(data, program);
        if (command->first_instance > data->instance_count ||
            command->instance_count > data->instance_count - command->first_instance)
            continue;
        texture = rvn_gl_texture(command->texture);
        glBindTexture(GL_TEXTURE_2D,
                       texture && texture->name ? texture->name : g_gl.white);
        glUniform1i(glGetUniformLocation(program, "u_alpha_only"),
                     texture && texture->format == GL_RED);
        glBlendFunc(command->type == R_DRAW_ERASE ? GL_ZERO : GL_ONE,
                    GL_ONE_MINUS_SRC_ALPHA);
        rvn_gl_attributes(command->first_instance);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4,
                               (GLsizei)command->instance_count);
    }
    if (capture) {
        size_t row = (size_t)data->framebuffer_width * 4;
        uint32_t y;
        uint8_t* temp = rvn_alloc(row);
        capture->width = (int)data->framebuffer_width;
        capture->height = (int)data->framebuffer_height;
        capture->pixels = Rpixels_alloc(row * data->framebuffer_height);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, capture->width, capture->height,
                      GL_RGBA, GL_UNSIGNED_BYTE, capture->pixels);
        for (y = 0; y < data->framebuffer_height / 2; y++) {
            uint8_t* a = capture->pixels + y * row;
            uint8_t* b = capture->pixels
                         + (data->framebuffer_height - y - 1) * row;
            memcpy(temp, a, row);
            memcpy(a, b, row);
            memcpy(b, temp, row);
        }
        rvn_free(temp);
    }
    {
        rvn_gl_surface_t* surface;
        for (surface = g_gl.surfaces; surface; surface = surface->next) {
            if (surface->window != data->window) continue;
            EM_ASM({
                const state = Module.ravenWindows.get($0);
                if (!state) return;
                const c = state.canvas;
                const source = Module.ravenRenderCanvas;
                if (c.width !== source.width) c.width = source.width;
                if (c.height !== source.height) c.height = source.height;
                const context = c.getContext('2d');
                context.globalCompositeOperation = 'copy';
                context.drawImage(source, 0, 0);
            }, surface->native);
            break;
        }
    }

}

static int rvn_gl_create_shader(Rid id, const uint32_t* spirv, size_t size) {
    rvn_shader_t* shader = rvn_shader_get(id);
    const char* source = NULL;
    GLuint vert, frag, program;
    GLint ok;
    uint32_t index = R_ID_INDEX(id);
    char* combined;
    size_t common_size, source_size;
    RVN_UNUSED(spirv); RVN_UNUSED(size);
    if (!shader) return R_FALSE;
    if (shader->source_glsl) source = shader->source_glsl;
    else if (shader->effect >= 0 &&
             (unsigned)shader->effect < RVN_COUNT_OF(rvn_webgl_effects))
        source = rvn_webgl_effects[shader->effect];
    if (!source) {
        rvn_error(R_ERROR_BACKEND, R_SEVERITY_ERROR,
                  "WebGL cannot load SPIR-V: use Rweb_shader_create with GLSL");
        return R_FALSE;
    }
    common_size = strlen(rvn_webgl_effect_common);
    source_size = strlen(source);
    combined = rvn_alloc(common_size + source_size + 1);
    memcpy(combined, rvn_webgl_effect_common, common_size);
    memcpy(combined + common_size, source, source_size + 1);
    vert = rvn_gl_compile(GL_VERTEX_SHADER, rvn_webgl_vert);
    frag = rvn_gl_compile(GL_FRAGMENT_SHADER, combined);
    rvn_free(combined);
    if (!vert || !frag) {
        glDeleteShader(vert); glDeleteShader(frag);
        return R_FALSE;
    }
    program = glCreateProgram();
    glAttachShader(program, vert); glAttachShader(program, frag);
    glLinkProgram(program);
    glDeleteShader(vert); glDeleteShader(frag);
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) { glDeleteProgram(program); return R_FALSE; }
    if (index >= g_gl.shader_capacity) {
        uint32_t capacity = index + 32;
        GLuint* programs = rvn_calloc(capacity * sizeof(*programs));
        if (g_gl.shader_capacity)
            memcpy(programs, g_gl.shaders,
                    g_gl.shader_capacity * sizeof(*programs));
        rvn_free(g_gl.shaders);
        g_gl.shaders = programs; g_gl.shader_capacity = capacity;
    }
    if (g_gl.shaders[index]) glDeleteProgram(g_gl.shaders[index]);
    g_gl.shaders[index] = program;
    return R_TRUE;
}
static void rvn_gl_destroy_shader(Rid id) {
    uint32_t index = R_ID_INDEX(id);
    if (index >= g_gl.shader_capacity) return;
    glDeleteProgram(g_gl.shaders[index]); g_gl.shaders[index] = 0;
}

Rid Rweb_shader_create(const char* source, Ruint flags) {
    rvn_shader_t* shader;
    RVN_API_GUARD(R_INVALID_ID);
    if (g_rvn.backend != R_BACKEND_WEBGL) {
        rvn_error(R_ERROR_INVALID_OPERATION, R_SEVERITY_ERROR,
                  "Rweb_shader_create requires the WebGL backend");
        return R_INVALID_ID;
    }
    if (!source || !source[0]) return R_INVALID_ID;
    shader = rvn_calloc(sizeof(*shader));
    shader->flags = flags; shader->effect = -1;
    shader->source_glsl = rvn_strdup(source);
    shader->id = rvn_pool_add(&g_rvn.shaders, shader);
    if (shader->id == R_INVALID_ID) {
        rvn_free(shader->source_glsl); rvn_free(shader); return R_INVALID_ID;
    }
    if (!rvn_gl_create_shader(shader->id, NULL, 0)) {
        rvn_pool_remove(&g_rvn.shaders, shader->id);
        rvn_free(shader->source_glsl); rvn_free(shader); return R_INVALID_ID;
    }
    shader->created = R_TRUE;
    return shader->id;
}

static void rvn_gl_idle(void) {
    if (g_gl.context > 0 && !emscripten_is_webgl_context_lost(g_gl.context))
        glFinish();
}

const RBackendVTable* rvn_backend_webgl(void) {
    static const RBackendVTable table = {
        .name = "WebGL 2",
        .Init = rvn_gl_init,
        .Shutdown = rvn_gl_shutdown,
        .CreateSurface = rvn_gl_surface,
        .DestroySurface = rvn_gl_destroy_surface,
        .CreateTexture = rvn_gl_create_texture,
        .UpdateTexture = rvn_gl_update_texture,
        .SetTextureFilter = rvn_gl_filter,
        .DestroyTexture = rvn_gl_destroy_texture,
        .RenderFrame = rvn_gl_frame,
        .WaitIdle = rvn_gl_idle,
        .CreateShader = rvn_gl_create_shader,
        .DestroyShader = rvn_gl_destroy_shader
    };
    return &table;
}

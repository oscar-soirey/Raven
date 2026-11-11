/* DOM adapter for Raven. Browser callbacks execute on the Raven UI thread. */
#include "../platform.h"
#include "../../widget/widget.h"
#include "raven/raven_accessibility.h"
#include <emscripten.h>
#include <emscripten/html5.h>

struct rvn_pwindow_t {
    int width, height, x, y;
    float scale;
    void* user;
    rvn_pwindow_state_t state;
    int primary;
    int transparent;
    struct rvn_pwindow_t* next;
};
static rvn_pwindow_t* g_window;
static rvn_pwindow_t* g_windows;
static rvn_pevent_handler_t g_handler;
static char* g_clipboard;

EM_JS(void, rvn_dom_attach, (rvn_pwindow_t* handle, int primary,
                            int width, int height, Rid window_id), {
    const canvas = primary ? (Module.canvas || document.getElementById('canvas')) :
                             document.createElement('canvas');
    if (!canvas) throw new Error('Raven requires a canvas with id="canvas"');
    const originalId = canvas.id;
    const originalStyle = canvas.style.cssText;
    const originalTabIndex = canvas.tabIndex;
    canvas.id = primary ? 'canvas' : 'raven-canvas-' + handle;
    if (!primary) {
        Object.assign(canvas.style, {position:'absolute',left:'40px',top:'40px',
            width:width+'px',height:height+'px',boxShadow:'0 8px 40px #0008'});
        document.body.appendChild(canvas);
    }
    canvas.tabIndex = 0;
    canvas.style.touchAction = 'none';
    canvas.style.visibility = 'hidden';
    const controller = new AbortController();
    const signal = controller.signal;
    const listen = (target, event, fn, options = {}) =>
        target.addEventListener(event, e => {
            _rvn_web_target(handle); fn(e);
        }, Object.assign({signal}, options));
    const textarea = document.createElement('textarea');
    textarea.setAttribute('aria-label', 'Raven text input');
    textarea.autocapitalize = 'off';
    textarea.spellcheck = false;
    Object.assign(textarea.style, {position:'fixed', left:'0', top:'0',
        width:'1px', height:'1px', opacity:'0', pointerEvents:'none'});
    document.body.appendChild(textarea);
    const state = {controller, textarea, canvas, window_id, visible:false,
                   originalId, originalStyle, originalTabIndex};
    Module.ravenWindows.set(handle, state);
    if (primary) Module.ravenDOM = state;
    const mods = e => (e.shiftKey ? 1 : 0) | (e.ctrlKey ? 2 : 0)
                     | (e.altKey ? 4 : 0) | (e.metaKey ? 8 : 0);
    const sendText = text => {
        if (!text) return;
        const ptr = stringToNewUTF8(text);
        _rvn_web_text(ptr);
        _free(ptr);
    };
    let composing = false;
    let committed = null;
    listen(textarea, 'compositionstart', () => { composing = true; });
    listen(textarea, 'compositionend', e => {
        composing = false; committed = e.data; sendText(e.data); textarea.value = "";
    });
    listen(textarea, 'input', e => {
        if (!composing && !e.isComposing) {
            if (committed !== null && e.data === committed) {
                committed = null; textarea.value = ""; return;
            }
            committed = null;
            sendText(e.data || textarea.value); textarea.value = "";
        }
    });
    listen(textarea, 'beforeinput', e => {
        const code = e.inputType === 'deleteContentBackward' ? 'Backspace' :
                     e.inputType === 'deleteContentForward' ? 'Delete' :
                     e.inputType === 'insertLineBreak' ? 'Enter' : null;
        if (code && !e.isComposing) {
            const ptr = stringToNewUTF8(code);
            _rvn_web_key(ptr, 1, 0, 0); _rvn_web_key(ptr, 0, 0, 0);
            _free(ptr); e.preventDefault();
        }
    });
    const key = (e, pressed) => {
        // Let the browser finish IME composition before forwarding editing keys.
        if (composing || e.isComposing || e.keyCode === 229) return;
        if ((e.ctrlKey || e.metaKey) && e.code === 'KeyV') return;
        const ptr = stringToNewUTF8(e.code);
        _rvn_web_key(ptr, pressed, mods(e), e.repeat ? 1 : 0);
        _free(ptr);
        _rvn_web_target(handle);
        if (e.code === 'Tab')
            (_rvn_web_wants_text() ? textarea : canvas).focus({preventScroll:true});
        if (['Tab','Backspace','Delete','Enter','Escape','ArrowLeft',
             'ArrowRight','ArrowUp','ArrowDown','Home','End','PageUp',
             'PageDown'].includes(e.code)) e.preventDefault();
    };
    for (const target of [canvas, textarea]) {
        listen(target, 'keydown', e => key(e, 1));
        listen(target, 'keyup', e => key(e, 0));
        listen(target, 'focus', () => _rvn_web_focus(1));
        listen(target, 'blur', () => {
            queueMicrotask(() => {
                if (Module.ravenWindows.get(handle) !== state) return;
                _rvn_web_target(handle);
                if (document.activeElement !== canvas &&
                    document.activeElement !== textarea &&
                    !document.activeElement.closest('#raven-access-window-' + window_id))
                    _rvn_web_focus(0);
            });
        });
        for (const event of ['copy', 'cut']) listen(target, event, e => {
            const ptr = _rvn_web_clipboard_pointer();
            if (ptr) e.clipboardData.setData('text/plain', UTF8ToString(ptr));
            e.preventDefault();
        });
        listen(target, 'paste', e => {
            const text = e.clipboardData.getData('text/plain');
            const ptr = stringToNewUTF8(text);
            _rvn_web_clipboard(ptr); _free(ptr);
            sendText(text); e.preventDefault();
        });
    }
    const pointer = (e, phase) => {
        state.pointerX = e.clientX; state.pointerY = e.clientY;
        if (state.drag && phase !== 0) {
            const drag = state.drag;
            const dx = e.clientX-drag.x, dy = e.clientY-drag.y;
            let left=drag.left, top=drag.top, width=drag.width, height=drag.height;
            if (drag.edge === -1) { left+=dx; top+=dy; }
            else {
                const west=[2,6,8].includes(drag.edge), east=[3,7,9].includes(drag.edge);
                const north=[4,6,7].includes(drag.edge), south=[5,8,9].includes(drag.edge);
                if (west) { width=Math.max(drag.minWidth,drag.width-dx); left+=drag.width-width; }
                if (east) width=Math.max(drag.minWidth,drag.width+dx);
                if (north) { height=Math.max(drag.minHeight,drag.height-dy); top+=drag.height-height; }
                if (south) height=Math.max(drag.minHeight,drag.height+dy);
            }
            Object.assign(canvas.style,{left:left+'px',top:top+'px',
                width:width+'px',height:height+'px'});
            _rvn_web_window_moved(left*(window.devicePixelRatio||1),
                                  top*(window.devicePixelRatio||1));
            if (phase===2 || phase===3) state.drag=null;
            // The initiating press reached Raven; its release must do so too.
            if (phase===1) { e.preventDefault(); return; }
        }
        const rect = canvas.getBoundingClientRect();
        const x = (e.clientX - rect.left) * canvas.width / rect.width;
        const y = (e.clientY - rect.top) * canvas.height / rect.height;
        if (phase === 0) {
            canvas.setPointerCapture(e.pointerId);
            canvas.style.zIndex = state.topmost ? '2000000000' :
                String(Module.ravenZ = (Module.ravenZ || 1) + 1);
        }
        _rvn_web_pointer(phase, e.pointerType === 'touch' ? 1 :
            e.pointerType === 'pen' ? 2 : 0, e.pointerId, x, y,
            e.button, e.buttons, e.pressure, e.tiltX, e.tiltY, mods(e));
        if (phase === 0) {
            _rvn_web_target(handle);
            (_rvn_web_wants_text() ? textarea : canvas).focus({preventScroll:true});
        }
        if (phase === 2 || phase === 3) {
            if (canvas.hasPointerCapture(e.pointerId))
                canvas.releasePointerCapture(e.pointerId);
        }
        e.preventDefault();
    };
    listen(canvas, 'pointerdown', e => pointer(e, 0));
    listen(canvas, 'pointermove', e => pointer(e, 1));
    listen(canvas, 'pointerup', e => pointer(e, 2));
    listen(canvas, 'pointercancel', e => pointer(e, 3));
    listen(canvas, 'pointerleave', e => {
        if (e.pointerType === 'mouse') _rvn_web_mouse_leave();
    });
    listen(canvas, 'contextmenu', e => e.preventDefault());
    listen(canvas, 'wheel', e => {
        const factor = e.deltaMode === 1 ? 1 : e.deltaMode === 2
                       ? canvas.clientHeight / 48 : 1 / 48;
        _rvn_web_wheel(-e.deltaX * factor, -e.deltaY * factor,
                       e.deltaMode === 0 ? 1 : 0, mods(e));
        e.preventDefault();
    }, {passive:false});
    const ownsFocus = target => {
        const tree = document.getElementById('raven-access-window-' + window_id);
        return target === canvas || target === textarea || (tree && tree.contains(target));
    };
    listen(document, 'focusin', e => _rvn_web_focus(ownsFocus(e.target) ? 1 : 0));
    listen(window, 'blur', () => _rvn_web_focus(0));
    listen(window, 'focus', () => {
        if (ownsFocus(document.activeElement)) _rvn_web_focus(1);
    });
    const dropPoint = e => {
        const rect = canvas.getBoundingClientRect();
        return [(e.clientX-rect.left)*canvas.width/rect.width,
                (e.clientY-rect.top)*canvas.height/rect.height];
    };
    listen(canvas, 'dragover', e => {
        const [x,y] = dropPoint(e);
        if (_rvn_web_drag(x,y,0)) e.preventDefault();
    });
    listen(canvas, 'dragleave', e => {
        const [x,y] = dropPoint(e); _rvn_web_drag(x,y,1);
    });
    listen(canvas, 'drop', async e => {
        e.preventDefault();
        const [x,y] = dropPoint(e);
        const text = e.dataTransfer.getData('text/plain');
        const files = Array.from(e.dataTransfer.files);
        let total = 0;
        const paths = [];
        for (const file of files) {
            total += file.size;
            if (total > 33554432) return;
        }
        try {
            for (let i=0;i<files.length;i++) {
                const bytes = new Uint8Array(await files[i].arrayBuffer());
                if (Module.ravenWindows.get(handle) !== state) return;
                FS.mkdirTree('/uploads');
                const name = files[i].name.replace(/[^a-zA-Z0-9._-]/g, '_');
                const path = '/uploads/drop-' + Date.now() + '-' + i + '-' + name;
                FS.writeFile(path,bytes); paths.push(path);
            }
            if (Module.ravenWindows.get(handle) !== state) return;
            _rvn_web_target(handle);
            const p=stringToNewUTF8(paths.join("\n")), t=stringToNewUTF8(text);
            _rvn_web_drop(x,y,p,t); _free(p); _free(t);
        } catch (error) { console.error('Raven file drop: ' + error); }
    });
    const resize = () => {
        _rvn_web_target(handle);
        const rect = canvas.getBoundingClientRect();
        const scale = window.devicePixelRatio || 1;
        const width = Math.max(1, Math.round(rect.width * scale));
        const height = Math.max(1, Math.round(rect.height * scale));
        if (canvas.width !== width || canvas.height !== height) {
            canvas.width = width; canvas.height = height;
        }
        _rvn_web_resize(width, height, scale);
    };
    const observer = new ResizeObserver(resize);
    observer.observe(canvas);
    state.observer = observer;
    listen(window, 'resize', resize);
    listen(document, 'visibilitychange', () => {
        if (document.hidden) _rvn_web_focus(0);
        else { resize(); _rvn_web_expose(); }
    });
    state.resize = resize;
    _rvn_web_target(handle);
    const initial = canvas.getBoundingClientRect();
    _rvn_web_window_moved(initial.left*(window.devicePixelRatio||1),
                          initial.top*(window.devicePixelRatio||1));
})

EM_JS(void, rvn_dom_detach, (rvn_pwindow_t* handle), {
    const state = Module.ravenWindows.get(handle);
    if (!state) return;
    state.controller.abort(); state.observer.disconnect(); state.textarea.remove();
    if (state.canvas.id !== 'canvas') state.canvas.remove();
    else {
        const context = state.canvas.getContext('2d');
        if (context) context.clearRect(0,0,state.canvas.width,state.canvas.height);
        state.canvas.id = state.originalId;
        state.canvas.style.cssText = state.originalStyle;
        state.canvas.tabIndex = state.originalTabIndex;
    }
    Module.ravenWindows.delete(handle);
    if (Module.ravenDOM === state) Module.ravenDOM = null;
})

EM_JS(void, rvn_dom_init, (void), {
    Module.ravenWindows = new Map();
    Module.ravenLifecycle = {originalTitle:document.title};
    const canvas = document.createElement('canvas');
    canvas.id = 'raven-render-canvas';
    canvas.hidden = true;
    document.body.appendChild(canvas);
    Module.ravenRenderCanvas = canvas;
    canvas.addEventListener('webglcontextlost', e => {
        e.preventDefault(); _rvn_web_context_lost();
    });
    canvas.addEventListener('webglcontextrestored', () => _rvn_web_context_restored());
})

EM_JS(void, rvn_dom_shutdown, (void), {
    if (Module.ravenRenderCanvas) Module.ravenRenderCanvas.remove();
    delete Module.ravenRenderCanvas;
    Module.ravenDOM = null;
    if (Module.ravenLifecycle) {
        for (const input of (Module.ravenLifecycle.inputs || [])) input.remove();
        document.title = Module.ravenLifecycle.originalTitle;
    }
    delete Module.ravenLifecycle;
    for (const id of ['raven-accessibility', 'raven-announcement']) {
        const node = document.getElementById(id); if (node) node.remove();
    }
})

EMSCRIPTEN_KEEPALIVE void rvn_web_target(rvn_pwindow_t* handle) {
    rvn_pwindow_t* window;
    g_window = NULL;
    for (window = g_windows; window; window = window->next)
        if (window == handle) { g_window = window; break; }
}

static void rvn_web_emit(rvn_pevent_t* event) {
    if (!g_window || !g_handler) return;
    event->user = g_window->user;
    g_handler(event);
}

EMSCRIPTEN_KEEPALIVE int rvn_web_wants_text(void) {
    rvn_window_t* window = g_window ? g_window->user : NULL;
    return window ? Rwindow_wants_text_input(window->id) : R_FALSE;
}

EMSCRIPTEN_KEEPALIVE const char* rvn_web_clipboard_pointer(void) {
    return g_clipboard;
}

EMSCRIPTEN_KEEPALIVE void rvn_web_window_moved(int x, int y) {
    if (g_window) { g_window->x = x; g_window->y = y; }
}

EMSCRIPTEN_KEEPALIVE void rvn_web_resize(int width, int height, float scale) {
    rvn_pevent_t event = {0};
    if (!g_window) return;
    if (g_window->width == width && g_window->height == height &&
        g_window->scale == scale) return;
    g_window->width = width;
    g_window->height = height;
    g_window->scale = scale;
    event.type = RVN_PEVENT_RESIZE;
    event.width = width; event.height = height; event.scale = scale;
    rvn_web_emit(&event);
}

#if defined(RAVEN_BACKEND_WEBGL_ENABLED)
void rvn_webgl_context_lost(void);
#endif
EMSCRIPTEN_KEEPALIVE void rvn_web_context_lost(void) {
#if defined(RAVEN_BACKEND_WEBGL_ENABLED)
    rvn_webgl_context_lost();
#endif
}

EMSCRIPTEN_KEEPALIVE void rvn_web_context_restored(void) {
    Rbackend_device_lost();
}

EMSCRIPTEN_KEEPALIVE void rvn_web_expose(void) {
    rvn_pevent_t event = {0};
    event.type = RVN_PEVENT_EXPOSE;
    rvn_web_emit(&event);
}

EMSCRIPTEN_KEEPALIVE void rvn_web_focus(int focused) {
    rvn_pevent_t event = {0};
    event.type = RVN_PEVENT_FOCUS;
    event.focused = focused;
    rvn_web_emit(&event);
}

void rvn_web_access_focus_window(Rid id) {
    rvn_pwindow_t* window;
    for (window = g_windows; window; window = window->next) {
        rvn_window_t* owner = window->user;
        if (owner && owner->id == id) {
            g_window = window; rvn_web_focus(R_TRUE); return;
        }
    }
}

EMSCRIPTEN_KEEPALIVE void rvn_web_clipboard(const char* text) {
    rvn_str_set(&g_clipboard, text);
}

EMSCRIPTEN_KEEPALIVE void rvn_web_text(const char* text) {
    rvn_pevent_t event = {0};
    const uint8_t* p = (const uint8_t*)text;
    event.type = RVN_PEVENT_TEXT;
    while (*p) {
        uint32_t c = *p++;
        int count = c < 128 ? 0 : c < 224 ? 1 : c < 240 ? 2 : 3;
        c &= count == 0 ? 127 : count == 1 ? 31 : count == 2 ? 15 : 7;
        while (count-- && *p) c = (c << 6) | (*p++ & 63);
        event.codepoint = c;
        rvn_web_emit(&event);
    }
}

EMSCRIPTEN_KEEPALIVE void rvn_web_key(const char* code, int pressed,
                                      int modifiers, int repeat) {
    static const struct { const char* code; RKey key; } keys[] = {
        {"Escape", R_KEY_ESCAPE}, {"Enter", R_KEY_ENTER}, {"Tab", R_KEY_TAB},
        {"Backspace", R_KEY_BACKSPACE}, {"Insert", R_KEY_INSERT},
        {"Delete", R_KEY_DELETE}, {"ArrowRight", R_KEY_RIGHT},
        {"ArrowLeft", R_KEY_LEFT}, {"ArrowDown", R_KEY_DOWN},
        {"ArrowUp", R_KEY_UP}, {"PageUp", R_KEY_PAGE_UP},
        {"PageDown", R_KEY_PAGE_DOWN}, {"Home", R_KEY_HOME}, {"End", R_KEY_END},
        {"ShiftLeft", R_KEY_LEFT_SHIFT}, {"ShiftRight", R_KEY_RIGHT_SHIFT},
        {"ControlLeft", R_KEY_LEFT_CTRL}, {"ControlRight", R_KEY_RIGHT_CTRL},
        {"AltLeft", R_KEY_LEFT_ALT}, {"AltRight", R_KEY_RIGHT_ALT},
        {"MetaLeft", R_KEY_LEFT_SUPER}, {"MetaRight", R_KEY_RIGHT_SUPER},
        {"Space", R_KEY_SPACE}, {"Minus", R_KEY_MINUS}, {"Equal", R_KEY_EQUAL},
        {"BracketLeft", R_KEY_LEFT_BRACKET}, {"BracketRight", R_KEY_RIGHT_BRACKET},
        {"Backslash", R_KEY_BACKSLASH}, {"Semicolon", R_KEY_SEMICOLON},
        {"Quote", R_KEY_APOSTROPHE}, {"Comma", R_KEY_COMMA},
        {"Period", R_KEY_PERIOD}, {"Slash", R_KEY_SLASH}, {"Backquote", R_KEY_GRAVE}
    };
    rvn_pevent_t event = {0};
    size_t i;
    event.type = RVN_PEVENT_KEY;
    event.pressed = pressed; event.modifiers = (uint32_t)modifiers;
    event.repeat = repeat;
    if (strncmp(code, "Key", 3) == 0 && strlen(code) == 4)
        event.key = (RKey)code[3];
    else if (strncmp(code, "Digit", 5) == 0 && strlen(code) == 6)
        event.key = (RKey)code[5];
    else if (code[0] == 'F' && atoi(code + 1) >= 1 && atoi(code + 1) <= 12)
        event.key = (RKey)(R_KEY_F1 + atoi(code + 1) - 1);
    else for (i = 0; i < RVN_COUNT_OF(keys); i++)
        if (strcmp(code, keys[i].code) == 0) { event.key = keys[i].key; break; }
    rvn_web_emit(&event);
}

EMSCRIPTEN_KEEPALIVE void rvn_web_pointer(int phase, int kind, int id,
        float x, float y, int button, int buttons, float pressure,
        float tilt_x, float tilt_y, int modifiers) {
    rvn_pevent_t event = {0};
    event.fx = x; event.fy = y; event.x = (int)x; event.y = (int)y;
    event.modifiers = (uint32_t)modifiers;
    event.pressure = pressure;
    if (kind == 1) {
        event.type = RVN_PEVENT_TOUCH;
        event.touch_id = id;
        event.phase = phase == 0 ? R_TOUCH_DOWN : phase == 1 ? R_TOUCH_MOVE :
                      phase == 2 ? R_TOUCH_UP : R_TOUCH_CANCEL;
    } else if (kind == 2) {
        event.type = RVN_PEVENT_PEN;
        event.phase = phase == 0 ? R_PEN_DOWN :
                      phase == 1 ? (buttons ? R_PEN_MOVE : R_PEN_HOVER) :
                      phase == 2 ? R_PEN_UP : R_PEN_LEAVE;
        event.pen_buttons = (buttons & 2) ? R_PEN_BUTTON_BARREL : 0;
        event.tilt_x = tilt_x; event.tilt_y = tilt_y;
        event.eraser = button == 5;
    } else {
        event.type = phase == 1 ? RVN_PEVENT_MOUSE_MOVE : RVN_PEVENT_MOUSE_BUTTON;
        event.button = button == 1 ? R_MOUSE_MIDDLE :
                       button == 2 ? R_MOUSE_RIGHT : button;
        event.pressed = phase == 0;
    }
    rvn_web_emit(&event);
}

EMSCRIPTEN_KEEPALIVE int rvn_web_drag(int x, int y, int leaving) {
    rvn_pevent_t event = {0};
    int accepted = R_FALSE;
    event.type = RVN_PEVENT_DRAG; event.x = x; event.y = y;
    event.leaving = leaving; event.accepted = &accepted;
    rvn_web_emit(&event);
    return accepted;
}
EMSCRIPTEN_KEEPALIVE void rvn_web_drop(int x, int y, char* paths,
                                      const char* text) {
    rvn_pevent_t event = {0};
    const char** items = NULL;
    char* p = paths;
    uint32_t count = 0, capacity = 0;
    while (p && *p) {
        char* end = strchr(p, '\n');
        if (end) *end = 0;
        RVN_ARRAY_RESERVE(items, capacity, count + 1);
        items[count++] = p;
        p = end ? end + 1 : NULL;
    }
    event.type = RVN_PEVENT_DROP; event.x = x; event.y = y;
    event.drop_paths = items; event.drop_count = (int)count;
    event.drop_text = text;
    rvn_web_emit(&event);
    rvn_free(items);
}

EMSCRIPTEN_KEEPALIVE void rvn_web_mouse_leave(void) {
    rvn_pevent_t event = {0};
    event.type = RVN_PEVENT_MOUSE_LEAVE; rvn_web_emit(&event);
}

EMSCRIPTEN_KEEPALIVE void rvn_web_wheel(float x, float y, int precise,
                                       int modifiers) {
    rvn_pevent_t event = {0};
    event.type = RVN_PEVENT_MOUSE_WHEEL;
    event.wheel_x = x; event.wheel_y = y; event.wheel_precise = precise;
    event.modifiers = (uint32_t)modifiers;
    rvn_web_emit(&event);
}

static int rvn_web_init(rvn_pevent_handler_t handler,
                         rvn_pregion_handler_t regions) {
    RVN_UNUSED(regions);
    g_handler = handler;
    rvn_dom_init();
    return R_TRUE;
}
void rvn_web_services_shutdown(void);
static void rvn_web_shutdown(void) {
    rvn_web_services_shutdown();
    rvn_dom_shutdown();
    rvn_free(g_clipboard); g_clipboard = NULL;
    g_handler = NULL;
}
static void* rvn_web_display(void) { return NULL; }
static void rvn_web_title(rvn_pwindow_t* window, const char* title);
static rvn_pwindow_t* rvn_web_create(const rvn_pwindow_desc_t* desc) {
    rvn_pwindow_t* window = rvn_calloc(sizeof(*window));
    window->primary = !g_windows;
    window->user = desc->user;
    window->transparent = (desc->flags & R_WINDOW_TRANSPARENT) != 0;
    window->scale = (float)emscripten_get_device_pixel_ratio();
    if (window->scale <= 0) window->scale = 1;
    window->width = (int)(desc->width * window->scale);
    window->height = (int)(desc->height * window->scale);
    window->next = g_windows;
    g_windows = window;
    g_window = window;
    rvn_dom_attach(window, window->primary, desc->width, desc->height,
                   ((rvn_window_t*)desc->user)->id);
    rvn_web_title(window, desc->title);
    return window;
}
static void rvn_web_destroy(rvn_pwindow_t* window) {
    rvn_window_t* owner = window->user;
    rvn_pwindow_t** link = &g_windows;
    if (owner) EM_ASM({
        const node = document.getElementById('raven-access-window-' + $0);
        if (node) node.remove();
    }, owner->id);
    rvn_dom_detach(window);
    while (*link && *link != window) link = &(*link)->next;
    if (*link) *link = window->next;
    if (g_window == window) g_window = NULL;
    rvn_free(window);
}
static void rvn_web_title(rvn_pwindow_t* window, const char* title) {
    if (window->primary)
        EM_ASM({ document.title = UTF8ToString($0); }, title ? title : "");
}
static void rvn_web_size(rvn_pwindow_t* window, int width, int height) {
    EM_ASM({
        const c = Module.ravenWindows.get($2).canvas;
        c.style.width = $0 + 'px'; c.style.height = $1 + 'px';
    }, width / window->scale, height / window->scale, window);
}
static void rvn_web_get_size(rvn_pwindow_t* window, int* w, int* h) {
    *w = window->width; *h = window->height;
}
static void rvn_web_min_size(rvn_pwindow_t* window, int w, int h) {
    EM_ASM({
        const c = Module.ravenWindows.get($0).canvas;
        c.style.minWidth = $1 + 'px'; c.style.minHeight = $2 + 'px';
    }, window, w / window->scale, h / window->scale);
}
static void rvn_web_position(rvn_pwindow_t* window, int x, int y) {
    window->x = x; window->y = y;
    if (!window->primary) EM_ASM({
        const c = Module.ravenWindows.get($2).canvas;
        c.style.left = $0 + 'px'; c.style.top = $1 + 'px';
    }, x / window->scale, y / window->scale, window);
}
static void rvn_web_get_position(rvn_pwindow_t* window, int* x, int* y) {
    *x = window->x; *y = window->y;
}
static float rvn_web_scale(rvn_pwindow_t* window) { return window->scale; }
static void rvn_web_show(rvn_pwindow_t* window, int visible) {
    RVN_UNUSED(window);
    EM_ASM({
        const state = Module.ravenWindows.get($1);
        state.visible = !!$0;
        state.canvas.style.visibility = $0 ? 'visible' : 'hidden';
        const tree = document.getElementById('raven-access-window-' + state.window_id);
        if (tree) tree.hidden = !$0;
    }, visible, window);
}
EM_JS(void, rvn_dom_state, (rvn_pwindow_t* handle, int state), {
    const s=Module.ravenWindows.get(handle), c=s.canvas;
    if (state===2) {
        if (!s.restoreStyle) s.restoreStyle=c.style.cssText;
        Object.assign(c.style,{position:'absolute',left:'0',top:'0',
            width:'100vw',height:'100vh',visibility:'visible'});
    } else if (state===1) c.style.visibility='hidden';
    else {
        if (s.restoreStyle) { c.style.cssText=s.restoreStyle; s.restoreStyle=null; }
        c.style.visibility='visible';
    }
    s.visible = state !== 1;
    const tree = document.getElementById('raven-access-window-' + s.window_id);
    if (tree) tree.hidden = !s.visible;
    s.resize();
})
static void rvn_web_state(rvn_pwindow_t* window, rvn_pwindow_state_t state) {
    rvn_pevent_t event = {0};
    if (window->state == state) return;
    window->state = state;
    rvn_dom_state(window, state);
    event.type = RVN_PEVENT_STATE; event.user = window->user; event.state = state;
    g_handler(&event);
}
static rvn_pwindow_state_t rvn_web_get_state(rvn_pwindow_t* window) {
    return window->state;
}
EM_JS(void, rvn_dom_begin_drag, (rvn_pwindow_t* handle, int edge), {
    const s=Module.ravenWindows.get(handle), c=s.canvas;
    const rect=c.getBoundingClientRect(), style=getComputedStyle(c);
    s.drag={x:s.pointerX||0,y:s.pointerY||0,left:rect.left+scrollX,
        top:rect.top+scrollY,width:rect.width,height:rect.height,edge,
        minWidth:parseFloat(style.minWidth)||1,minHeight:parseFloat(style.minHeight)||1};
})
static void rvn_web_move(rvn_pwindow_t* window) {
    if (!window->primary) rvn_dom_begin_drag(window, -1);
}
static void rvn_web_resize_begin(rvn_pwindow_t* window, rvn_pregion_t edge) {
    if (!window->primary) rvn_dom_begin_drag(window, (int)edge);
}
static void rvn_web_cursor(rvn_pwindow_t* window, RCursor cursor) {
    static const char* names[] = {"default", "text", "pointer", "crosshair",
        "ew-resize", "ns-resize", "nwse-resize", "nesw-resize", "move",
        "not-allowed", "none"};
    unsigned index = (unsigned)(cursor - R_CURSOR_ARROW);
    RVN_UNUSED(window);
    EM_ASM({ Module.ravenWindows.get($1).canvas.style.cursor = UTF8ToString($0); },
           names[index < RVN_COUNT_OF(names) ? index : 0], window);
}
static int rvn_web_opacity(rvn_pwindow_t* window, float opacity) {
    EM_ASM({ Module.ravenWindows.get($0).canvas.style.opacity = $1; },
           window, opacity);
    return R_TRUE;
}
static int rvn_web_topmost(rvn_pwindow_t* window, int topmost) {
    EM_ASM({
        const state = Module.ravenWindows.get($0);
        state.topmost = !!$1;
        state.canvas.style.zIndex = $1 ? '2000000000' :
            String(Module.ravenZ = (Module.ravenZ || 1) + 1);
    }, window, topmost);
    return R_TRUE;
}
static void rvn_web_center(rvn_pwindow_t* window) {
    if (window->primary) return;
    EM_ASM({
        const canvas = Module.ravenWindows.get($0).canvas;
        const rect = canvas.getBoundingClientRect();
        const x = Math.max(0, (innerWidth - rect.width) / 2);
        const y = Math.max(0, (innerHeight - rect.height) / 2);
        canvas.style.left = x + 'px'; canvas.style.top = y + 'px';
        _rvn_web_target($0);
        _rvn_web_window_moved(x * (devicePixelRatio || 1), y * (devicePixelRatio || 1));
    }, window);
}

static void rvn_web_native(rvn_pwindow_t* window, RNativeWindow* native) {
    RVN_UNUSED(window);
    memset(native, 0, sizeof(*native));
    native->system = R_WINDOW_SYSTEM_EMSCRIPTEN;
    native->window = (uintptr_t)window;
    native->transparent = window->transparent;
}
static rvn_pwindow_t* rvn_web_child(rvn_pwindow_t* parent,
        int x, int y, int w, int h, void* user) {
    RVN_UNUSED(parent); RVN_UNUSED(x); RVN_UNUSED(y);
    RVN_UNUSED(w); RVN_UNUSED(h); RVN_UNUSED(user);
    return NULL;
}
static void rvn_web_child_rect(rvn_pwindow_t* child,
        int x, int y, int w, int h, int visible) {
    RVN_UNUSED(child); RVN_UNUSED(x); RVN_UNUSED(y);
    RVN_UNUSED(w); RVN_UNUSED(h); RVN_UNUSED(visible);
}
static void rvn_web_pump(double timeout) {
    rvn_pwindow_t* window;
    RVN_UNUSED(timeout);
    EM_ASM({ for (const state of Module.ravenWindows.values()) state.resize(); });
    for (window = g_windows; window; window = window->next) {
        rvn_window_t* owner = window->user;
        int x, y, width, height;
        if (!owner || !Rwindow_get_text_input_rect(owner->id, &x, &y, &width, &height))
            continue;
        EM_ASM({
            const state = Module.ravenWindows.get($0);
            if (!state) return;
            const rect = state.canvas.getBoundingClientRect();
            const scale = state.canvas.width / rect.width;
            state.textarea.style.left = (rect.left + $1 / scale) + 'px';
            state.textarea.style.top = (rect.top + $2 / scale) + 'px';
            state.textarea.style.height = Math.max(1, $3 / scale) + 'px';
        }, window, x, y, height);
    }
}
static void rvn_web_wake(void) {}
static void rvn_web_clipboard_set(const char* text) {
    rvn_str_set(&g_clipboard, text);
    EM_ASM({
        if (navigator.clipboard)
            navigator.clipboard.writeText(UTF8ToString($0)).catch(() => {});
    }, text ? text : "");
}
static char* rvn_web_clipboard_get(void) { return rvn_strdup(g_clipboard); }

void rvn_web_a11y_event(rvn_pwindow_t* window, int event, uint64_t node);
void rvn_web_a11y_announce(rvn_pwindow_t* window, const char* text, int assertive);
int rvn_web_a11y_active(void);
void rvn_web_a11y_settings(RAccessibilitySettings* settings);

const rvn_platform_t* rvn_platform_emscripten(void) {
    static const rvn_platform_t table = {
        .name = "emscripten", .system = R_WINDOW_SYSTEM_EMSCRIPTEN,
        .init = rvn_web_init, .shutdown = rvn_web_shutdown,
        .native_display = rvn_web_display,
        .window_create = rvn_web_create, .window_destroy = rvn_web_destroy,
        .window_set_title = rvn_web_title, .window_set_size = rvn_web_size,
        .window_get_size = rvn_web_get_size, .window_set_min_size = rvn_web_min_size,
        .window_set_position = rvn_web_position,
        .window_get_position = rvn_web_get_position,
        .window_get_scale = rvn_web_scale, .window_show = rvn_web_show,
        .window_set_state = rvn_web_state, .window_get_state = rvn_web_get_state,
        .window_begin_move = rvn_web_move,
        .window_begin_resize = rvn_web_resize_begin,
        .window_set_cursor = rvn_web_cursor, .window_get_native = rvn_web_native,
        .window_set_opacity = rvn_web_opacity, .window_set_topmost = rvn_web_topmost,
        .window_center = rvn_web_center,
        .child_create = rvn_web_child, .child_set_rect = rvn_web_child_rect,
        .pump = rvn_web_pump, .wake = rvn_web_wake,
        .clipboard_set = rvn_web_clipboard_set,
        .clipboard_get = rvn_web_clipboard_get,
        .a11y_event = rvn_web_a11y_event, .a11y_announce = rvn_web_a11y_announce,
        .a11y_active = rvn_web_a11y_active, .a11y_settings = rvn_web_a11y_settings
    };
    return &table;
}

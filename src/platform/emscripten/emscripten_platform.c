/* DOM adapter for Raven. Browser callbacks execute on the Raven UI thread. */
#include "../platform.h"
#include "../web_input.h"
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

/* The UTF-16 units the textarea holds on each side of the selection of a TextInput. */
#define RVN_WEB_SURROUNDING 512

/*
 * What the page was told of the focused field (rvn_web_field_sync): its mode
 * and attributes, and for a TextInput the part of its text the textarea
 * holds. The page sends its edits relative to that part.
 */
static struct {
    rvn_pwindow_t* window;
    Rid focus;
    rvn_web_field_hints_t hints;
    /* the part of the text in the textarea (NULL: not sent yet), where it starts, its selection */
    char* mirror;
    int offset;
    int start16, end16;
    /* the page composes: nothing is sent until it ends */
    int composing;
    /* a composition is shown in the field */
    int composition;
    /* counts what was sent to the page: an answer about an older text is not applied */
    uint32_t serial;
    /* an edit of the page is being applied: the page is told once it is done */
    int applying;
} g_field;

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
    // The elements the keyboard types into: a textarea, and a password input
    // for secret fields. 16px: iOS zooms into a focused field with a smaller font.
    const makeInput = (tag, label) => {
        const node = document.createElement(tag);
        if (tag === 'input') node.type = 'password';
        node.setAttribute('aria-label', label);
        node.setAttribute('autocapitalize', 'off');
        node.setAttribute('autocorrect', 'off');
        node.setAttribute('autocomplete', tag === 'input' ? 'current-password' : 'off');
        node.spellcheck = false;
        Object.assign(node.style, {position:'fixed', left:'0', top:'0', width:'1px',
            height:'1px', opacity:'0', pointerEvents:'none', fontSize:'16px', padding:'0',
            border:'0', margin:'0', resize:'none', overflow:'hidden'});
        document.body.appendChild(node);
        return node;
    };
    const textarea = makeInput('textarea', 'Raven text input');
    const secure = makeInput('input', 'Raven password input');
    // mode: 0 no text, 1 the textarea mirrors the text around the caret of a
    // TextInput, 2 password, 3 keys and typed text (TextEditor, SpinBox).
    const field = {mode:0, mirror:"", start:0, end:0, composing:false, data:"", committed:null};
    const state = {controller, textarea, secure, field, canvas, window_id, visible:false,
                   originalId, originalStyle, originalTabIndex,
                   insets:[0,0,0,0], keyboard:0};
    Module.ravenWindows.set(handle, state);
    if (primary) Module.ravenDOM = state;
    const ours = node => node === textarea || node === secure;
    state.focusInput = () => {
        const target = field.mode === 2 ? secure : field.mode ? textarea : canvas;
        if (document.activeElement !== target) target.focus({preventScroll:true});
    };
    const mods = e => (e.shiftKey ? 1 : 0) | (e.ctrlKey ? 2 : 0)
                     | (e.altKey ? 4 : 0) | (e.metaKey ? 8 : 0);
    const sendText = text => {
        if (!text) return;
        const ptr = stringToNewUTF8(text);
        _rvn_web_text(ptr);
        _free(ptr);
    };
    const pressKey = code => {
        const ptr = stringToNewUTF8(code);
        _rvn_web_key(ptr, 1, 0, 0); _rvn_web_key(ptr, 0, 0, 0);
        _free(ptr);
    };
    const compose = (text, start, end) => {
        const ptr = text ? stringToNewUTF8(text) : 0;
        _rvn_web_field_compose(ptr, start, end);
        if (ptr) _free(ptr);
    };
    // The textarea went from field.mirror to value (UTF-16 selection start..end):
    // the changed range is sent as one replacement, attributed to the caret
    // (it starts at `at` at the latest: by default where the selection was or is).
    const apply = (value, start, end, at) => {
        const from = field.mirror;
        if (value === from) {
            if (start !== field.start || end !== field.end) {
                field.start = start; field.end = end;
                _rvn_web_field_select(start, end);
            }
            return false;
        }
        const limit = Math.min(from.length, value.length,
                               at === undefined ? Math.min(field.start, start) : at);
        let head = 0, tail = 0;
        while (head < limit && from.charCodeAt(head) === value.charCodeAt(head)) head++;
        if (head > 0 && (from.charCodeAt(head - 1) & 0xFC00) === 0xD800) head--;
        while (tail < from.length - head && tail < value.length - head &&
               from.charCodeAt(from.length - 1 - tail) ===
               value.charCodeAt(value.length - 1 - tail)) tail++;
        if (tail > 0 && (from.charCodeAt(from.length - tail) & 0xFC00) === 0xDC00) tail--;
        field.mirror = value; field.start = start; field.end = end;
        const ptr = stringToNewUTF8(value.slice(head, value.length - tail));
        _rvn_web_field_edit(head, from.length - tail - head, ptr, start, end);
        _free(ptr);
        return true;
    };
    // Where the composition `data` is in the textarea: around the caret.
    const composedAt = (value, data, caret) => {
        if (!data) return -1;
        let at = caret - data.length;
        if (at >= 0 && value.substr(at, data.length) === data) return at;
        at = value.indexOf(data, Math.max(0, caret - data.length));
        return at >= 0 && at <= caret ? at : -1;
    };
    const beginComposition = () => {
        if (field.composing) return;
        field.composing = true; field.data = ""; field.committed = null;
        _rvn_web_field_composing(1);
    };
    // An update of the composition: the field holds the text without it (a
    // word composed again leaves the field), the composition is drawn at the caret.
    const composing = data => {
        field.data = data = data == null ? field.data : data;
        if (field.mode !== 1) return compose(data, data.length, data.length);
        const value = textarea.value;
        const at = composedAt(value, data, textarea.selectionEnd);
        let start = data.length, end = data.length;
        if (at >= 0) {
            apply(value.slice(0, at) + value.slice(at + data.length), at, at, at);
            start = Math.max(0, Math.min(data.length, textarea.selectionStart - at));
            end = Math.max(0, Math.min(data.length, textarea.selectionEnd - at));
        }
        compose(data, start, end);
    };
    const endComposition = data => {
        field.composing = false;
        _rvn_web_field_composing(0);
        if (field.mode !== 1) {
            field.committed = data;
            if (data) sendText(data); else compose(null, 0, 0);
            textarea.value = "";
            return;
        }
        compose(null, 0, 0);
        field.committed = null;
        if (!apply(textarea.value, textarea.selectionStart, textarea.selectionEnd) && data) {
            // (the textarea did not change: the event did not come from an input method)
            field.committed = data;
            sendText(data);
        }
    };
    for (const node of [textarea, secure]) {
        listen(node, 'compositionstart', () => beginComposition());
        // (the textarea changes after compositionupdate: a mirrored field follows its input events)
        listen(node, 'compositionupdate', e => {
            field.data = e.data;
            if (field.composing && field.mode !== 1) composing(e.data);
        });
        listen(node, 'compositionend', e => endComposition(e.data));
        listen(node, 'beforeinput', e => {
            const type = e.inputType;
            if (type === 'historyUndo' || type === 'historyRedo') {
                // Raven undoes on its own shortcut: the history of the textarea is not the field
                e.preventDefault(); return;
            }
            if (e.isComposing || field.composing) return;
            const mirror = node === textarea && field.mode === 1;
            const code = type === 'insertLineBreak' || type === 'insertParagraph' ? 'Enter' :
                         mirror ? null :
                         type === 'deleteContentBackward' ? 'Backspace' :
                         type === 'deleteContentForward' ? 'Delete' : null;
            if (code) { pressKey(code); e.preventDefault(); }
        });
    }
    listen(textarea, 'input', e => {
        if (e.isComposing || field.composing) {
            beginComposition(); composing(e.data); return;
        }
        if (field.mode !== 1) {
            if (field.committed !== null && e.data === field.committed) {
                field.committed = null; textarea.value = ""; return;
            }
            field.committed = null;
            sendText(e.data || textarea.value); textarea.value = "";
            return;
        }
        if (field.committed !== null && e.data === field.committed &&
            textarea.value === field.mirror) { field.committed = null; return; }
        field.committed = null;
        if (!apply(textarea.value, textarea.selectionStart, textarea.selectionEnd) &&
            !e.inputType && e.data)
            sendText(e.data);   // (an event without an edit of the textarea)
    });
    listen(secure, 'input', e => {
        // never kept in the page: each input goes to Raven and the element is emptied
        const value = secure.value;
        secure.value = "";
        if (e.isComposing || field.composing) return;
        if (e.data && e.inputType && e.inputType !== 'insertReplacementText') sendText(e.data);
        else if (value || e.data) {
            // a password manager filled the field
            const ptr = stringToNewUTF8(value || e.data);
            _rvn_web_field_fill(ptr); _free(ptr);
        }
    });
    // The selection moved in the textarea (the cursor keys of an on-screen keyboard, its menu).
    const selected = () => {
        if (field.mode !== 1 || field.composing || document.activeElement !== textarea ||
            textarea.value !== field.mirror) return;
        apply(field.mirror, textarea.selectionStart, textarea.selectionEnd);
    };
    listen(document, 'selectionchange', selected);
    listen(textarea, 'selectionchange', selected);
    const key = (e, pressed) => {
        // Let the browser finish IME composition before forwarding editing keys.
        if (field.composing || e.isComposing || e.keyCode === 229) return;
        if ((e.ctrlKey || e.metaKey) && e.code === 'KeyV') return;
        const ptr = stringToNewUTF8(e.code);
        _rvn_web_key(ptr, pressed, mods(e), e.repeat ? 1 : 0);
        _free(ptr);
        _rvn_web_target(handle);
        if (e.code === 'Tab') state.focusInput();
        if (['Tab','Backspace','Delete','Enter','Escape','ArrowLeft',
             'ArrowRight','ArrowUp','ArrowDown','Home','End','PageUp',
             'PageDown'].includes(e.code)) e.preventDefault();
        // Shortcuts Raven applies to the field itself: the textarea must not apply
        // them to its copy too (Control+A moves to the line start in a macOS textarea).
        else if (pressed && ours(e.target) && (e.ctrlKey || e.metaKey) &&
                 ['KeyA','KeyZ','KeyY'].includes(e.code)) e.preventDefault();
    };
    for (const target of [canvas, textarea, secure]) {
        listen(target, 'keydown', e => key(e, 1));
        listen(target, 'keyup', e => key(e, 0));
        listen(target, 'focus', () => { _rvn_web_focus(1); state.measure(); });
        listen(target, 'blur', () => {
            queueMicrotask(() => {
                if (Module.ravenWindows.get(handle) !== state) return;
                _rvn_web_target(handle);
                state.measure();
                if (document.activeElement !== canvas && !ours(document.activeElement) &&
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
        // (a tap focuses a field when the finger leaves it: the keyboard comes in the gesture)
        if (phase === 0 || phase === 2) {
            _rvn_web_target(handle);
            if (phase === 0 || field.mode) state.focusInput();
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
        return target === canvas || ours(target) || (tree && tree.contains(target));
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
    // What covers the canvas: the safe area of the screen (env(safe-area-inset-*),
    // with viewport-fit=cover) and the on-screen keyboard, which shrinks the
    // visual viewport while a field has the focus. In device pixels.
    const measure = () => {
        if (typeof getComputedStyle !== 'function') return;
        const scale = window.devicePixelRatio || 1;
        const rect = canvas.getBoundingClientRect();
        let probe = Module.ravenSafeArea;
        if (!probe || !probe.isConnected) {
            probe = Module.ravenSafeArea = document.createElement('div');
            probe.setAttribute('aria-hidden', 'true');
            probe.style.cssText = 'position:fixed;left:0;top:0;width:0;height:0;' +
                'visibility:hidden;pointer-events:none;padding:env(safe-area-inset-top) ' +
                'env(safe-area-inset-right) env(safe-area-inset-bottom) env(safe-area-inset-left)';
            document.body.appendChild(probe);
        }
        const style = getComputedStyle(probe);
        const safe = ['Top','Right','Bottom','Left'].map(side =>
            parseFloat(style['padding' + side]) || 0);
        const width = document.documentElement.clientWidth || innerWidth;
        const height = innerHeight;
        // (only the part of each band the canvas reaches into)
        const band = (inset, overlap) => inset > 0 ? Math.min(inset, Math.max(0, overlap)) : 0;
        const insets = [band(safe[0], safe[0] - rect.top),
                        band(safe[1], rect.right - (width - safe[1])),
                        band(safe[2], rect.bottom - (height - safe[2])),
                        band(safe[3], safe[3] - rect.left)].map(v => Math.round(v * scale));
        let keyboard = 0;
        const viewport = window.visualViewport;
        if (viewport && field.mode && ours(document.activeElement) &&
            Math.abs((viewport.scale || 1) - 1) < 0.01) {
            const covered = rect.bottom - (viewport.offsetTop + viewport.height);
            // (a few pixels are toolbars coming and going, not a keyboard)
            if (covered > 60) keyboard = Math.round(Math.min(covered, rect.height) * scale);
        }
        if (keyboard === state.keyboard && insets.every((v, i) => v === state.insets[i])) return;
        state.insets = insets; state.keyboard = keyboard;
        _rvn_web_target(handle);
        _rvn_web_insets();
    };
    state.measure = measure;
    const observer = new ResizeObserver(() => { resize(); measure(); });
    observer.observe(canvas);
    state.observer = observer;
    listen(window, 'resize', () => { resize(); measure(); });
    if (window.visualViewport) {
        listen(window.visualViewport, 'resize', measure);
        listen(window.visualViewport, 'scroll', measure);
    }
    listen(document, 'visibilitychange', () => {
        if (document.hidden) _rvn_web_focus(0);
        else { resize(); measure(); _rvn_web_expose(); }
    });
    state.resize = resize;
    measure();
    _rvn_web_target(handle);
    const initial = canvas.getBoundingClientRect();
    _rvn_web_window_moved(initial.left*(window.devicePixelRatio||1),
                          initial.top*(window.devicePixelRatio||1));
})

EM_JS(void, rvn_dom_detach, (rvn_pwindow_t* handle), {
    const state = Module.ravenWindows.get(handle);
    if (!state) return;
    state.controller.abort(); state.observer.disconnect(); state.textarea.remove();
    state.secure.remove();
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
    if (Module.ravenSafeArea) Module.ravenSafeArea.remove();
    delete Module.ravenSafeArea;
    for (const id of ['raven-accessibility', 'raven-announcement']) {
        const node = document.getElementById(id); if (node) node.remove();
    }
})

EMSCRIPTEN_KEEPALIVE void rvn_web_target(rvn_pwindow_t* handle) {
    rvn_pwindow_t* window;
    /* (every DOM event comes through here first: the loop may be asleep) */
    rvn_web_loop_poke();
    g_window = NULL;
    for (window = g_windows; window; window = window->next)
        if (window == handle) { g_window = window; break; }
}

static void rvn_web_field_sync(rvn_pwindow_t* window);
EMSCRIPTEN_KEEPALIVE void rvn_web_text(const char* text);

static void rvn_web_emit(rvn_pevent_t* event) {
    rvn_pwindow_t* window = g_window;
    if (!window || !g_handler) return;
    event->user = window->user;
    g_handler(event);
    /* what may have moved the focus, the caret or the text: the page follows at once */
    switch (event->type) {
        case RVN_PEVENT_KEY: case RVN_PEVENT_TEXT: case RVN_PEVENT_MOUSE_BUTTON:
        case RVN_PEVENT_FOCUS: case RVN_PEVENT_PEN:
            rvn_web_field_sync(window);
            break;
        case RVN_PEVENT_TOUCH:
            if (event->phase != R_TOUCH_MOVE) rvn_web_field_sync(window);
            break;
        default:
            break;
    }
}

/* ---------------------------------------------------------------------------
 *  The text field: the textarea (or password input) the keyboard types into
 * ------------------------------------------------------------------------- */

EM_JS(void, rvn_dom_field, (rvn_pwindow_t* handle, int mode, const char* inputmode,
                           const char* enterkeyhint, const char* autocomplete,
                           const char* autocapitalize, int spellcheck, int read_only), {
    const state = Module.ravenWindows.get(handle);
    if (!state) return;
    const field = state.field;
    const focused = document.activeElement === state.textarea ||
                    document.activeElement === state.secure;
    Object.assign(field, {mode, mirror:"", start:0, end:0, composing:false, committed:null});
    state.textarea.value = ""; state.secure.value = "";
    const node = mode === 2 ? state.secure : state.textarea;
    const set = (name, value) => value ? node.setAttribute(name, value) : node.removeAttribute(name);
    set('inputmode', UTF8ToString(inputmode));
    set('enterkeyhint', UTF8ToString(enterkeyhint));
    set('autocomplete', UTF8ToString(autocomplete));
    set('autocapitalize', UTF8ToString(autocapitalize));
    node.spellcheck = !!spellcheck;
    node.setAttribute('autocorrect', spellcheck ? 'on' : 'off');
    node.readOnly = !!read_only;
    // the keyboard follows the focus of Raven: another kind of field, or none (it goes away)
    // (unless the page moved the focus elsewhere meanwhile: another canvas, its field)
    if (focused) queueMicrotask(() => {
        if (Module.ravenWindows.get(handle) === state &&
            (document.activeElement === state.textarea ||
             document.activeElement === state.secure)) state.focusInput();
    });
})

EM_JS(void, rvn_dom_field_text, (rvn_pwindow_t* handle, const char* text, int length,
                                int start, int end), {
    const state = Module.ravenWindows.get(handle);
    if (!state) return;
    const field = state.field, node = state.textarea;
    const value = length ? UTF8ToString(text, length) : "";
    Object.assign(field, {mirror:value, start, end});
    if (node.value !== value) node.value = value;
    if (node.selectionStart !== start || node.selectionEnd !== end)
        node.setSelectionRange(start, end);
})

static Rid rvn_web_core(rvn_pwindow_t* window) {
    rvn_window_t* owner = window ? window->user : NULL;
    return owner && !owner->dead ? owner->id : R_INVALID_ID;
}

static void rvn_web_emit_to(rvn_pwindow_t* window, rvn_pevent_t* event) {
    rvn_pwindow_t* previous = g_window;
    g_window = window;
    rvn_web_emit(event);
    g_window = previous;
}

/* The composition shown in the field ends (before an edit of the field). */
static void rvn_web_field_end_composition(void) {
    rvn_pevent_t event = {0};
    if (!g_field.composition || !g_field.window) return;
    g_field.composition = R_FALSE;
    event.type = RVN_PEVENT_COMPOSITION;
    rvn_web_emit_to(g_field.window, &event);
}

static void rvn_web_field_forget(void) {
    g_field.serial++;
    rvn_free(g_field.mirror);
    g_field.mirror = NULL;
    g_field.offset = 0;
    g_field.start16 = g_field.end16 = 0;
}

/*
 * The page follows the focus of `window`: the mode and attributes of the
 * field when they change, and for a TextInput the text around its selection
 * when it changed (not while the page composes).
 */
static void rvn_web_field_sync(rvn_pwindow_t* window) {
    rvn_window_t* owner = window ? window->user : NULL;
    rvn_web_field_hints_t hints;
    RTextInputInfo info;
    rvn_web_field_kind_t kind = RVN_WEB_KIND_OTHER;
    int wants, has_info;
    Rid core = rvn_web_core(window), focus;
    if (core == R_INVALID_ID || g_field.applying) return;
    wants = Rwindow_wants_text_input(core);
    has_info = wants && Rwindow_get_text_input_info(core, &info);
    if (wants && owner->focus) {
        RWidgetType type = owner->focus->cls->type;
        kind = type == R_WIDGET_TEXT_INPUT ? RVN_WEB_KIND_TEXT_INPUT :
               type == R_WIDGET_TEXT_EDITOR ? RVN_WEB_KIND_TEXT_EDITOR :
               type == R_WIDGET_SPIN_BOX ? RVN_WEB_KIND_SPIN_BOX : RVN_WEB_KIND_OTHER;
    }
    rvn_web_field_hints(wants, kind, has_info ? &info : NULL, &hints);
    focus = wants && owner->focus ? owner->focus->id : R_INVALID_ID;
    if (window != g_field.window || focus != g_field.focus || hints.mode != g_field.hints.mode ||
        hints.spellcheck != g_field.hints.spellcheck || hints.read_only != g_field.hints.read_only ||
        strcmp(hints.inputmode, g_field.hints.inputmode) != 0 ||
        strcmp(hints.enterkeyhint, g_field.hints.enterkeyhint) != 0 ||
        strcmp(hints.autocomplete, g_field.hints.autocomplete) != 0 ||
        strcmp(hints.autocapitalize, g_field.hints.autocapitalize) != 0) {
        /* (the textarea of the window the focus left takes no text any more) */
        if (g_field.window && window != g_field.window && g_field.hints.mode != RVN_WEB_FIELD_NONE)
            rvn_dom_field(g_field.window, RVN_WEB_FIELD_NONE, "text", "", "off", "off", 0, 0);
        g_field.window = window;
        g_field.focus = focus;
        g_field.hints = hints;
        g_field.composing = R_FALSE;
        g_field.composition = R_FALSE;
        rvn_web_field_forget();
        rvn_dom_field(window, hints.mode, hints.inputmode, hints.enterkeyhint, hints.autocomplete,
                      hints.autocapitalize, hints.spellcheck, hints.read_only);
    }
    if (hints.mode == RVN_WEB_FIELD_MIRROR && !g_field.composing) {
        const char* text = Rwindow_get_text_input_text(core);
        rvn_web_surrounding_t part;
        int start, end;
        if (!text || !Rwindow_get_text_input_selection(core, &start, &end)) return;
        rvn_web_surrounding(text, start, end, RVN_WEB_SURROUNDING, &part);
        if (g_field.mirror && part.offset == g_field.offset && part.start16 == g_field.start16 &&
            part.end16 == g_field.end16 && (int)strlen(g_field.mirror) == part.length &&
            memcmp(g_field.mirror, text + part.offset, (size_t)part.length) == 0) return;
        rvn_free(g_field.mirror);
        g_field.mirror = rvn_strndup(text + part.offset, (size_t)part.length);
        g_field.offset = part.offset;
        g_field.start16 = part.start16;
        g_field.end16 = part.end16;
        g_field.serial++;
        rvn_dom_field_text(window, g_field.mirror, part.length, part.start16, part.end16);
    }
}

/* The page's textarea is the window of the field it was last sent: R_TRUE when Raven still has that text there. */
static int rvn_web_field_current(Rid core, const char** text) {
    int length, mirror;
    *text = Rwindow_get_text_input_text(core);
    if (!*text || !g_field.mirror) return R_FALSE;
    length = (int)strlen(*text);
    mirror = (int)strlen(g_field.mirror);
    return g_field.offset + mirror <= length &&
           memcmp(*text + g_field.offset, g_field.mirror, (size_t)mirror) == 0;
}

/* Selects UTF-16 [start16, end16) of the mirrored text in the field. */
static void rvn_web_field_select_mirror(Rid core, int start16, int end16) {
    int length = (int)strlen(g_field.mirror), start, end, now_start, now_end;
    start = g_field.offset + rvn_utf16_to_utf8(g_field.mirror, length, start16);
    end = g_field.offset + rvn_utf16_to_utf8(g_field.mirror, length, end16);
    g_field.start16 = start16;
    g_field.end16 = end16;
    if (Rwindow_get_text_input_selection(core, &now_start, &now_end) &&
        (now_start != rvn_mini(start, end) || now_end != rvn_maxi(start, end)))
        Rwindow_set_text_input_selection(core, start, end);
}

/* Text that typing can insert: no control characters. */
static int rvn_web_typeable(const char* text) {
    const unsigned char* p = (const unsigned char*)text;
    if (!*p) return R_FALSE;
    for (; *p; p++) if (*p < 0x20 || *p == 0x7F) return R_FALSE;
    return R_TRUE;
}

/*
 * The page's textarea changed: UTF-16 units [head, head + removed) of the
 * text it had were replaced by `inserted`, and its selection is [start16,
 * end16) of its new text. An insertion at the selection is typed (text
 * events, as a hardware keyboard types); anything else replaces the range.
 */
EMSCRIPTEN_KEEPALIVE void rvn_web_field_edit(int head, int removed, const char* inserted,
                                             int start16, int end16) {
    rvn_pwindow_t* window = g_field.window;
    Rid core = rvn_web_core(window);
    const char* text;
    int length, from, to, start, end, typed;
    uint32_t serial;
    char* mirror;
    if (core == R_INVALID_ID || g_window != window || g_field.hints.mode != RVN_WEB_FIELD_MIRROR) return;
    rvn_web_field_end_composition();
    if (!rvn_web_field_current(core, &text)) {
        /* the field changed since the page was told (the application): what was typed goes at the caret */
        if (inserted[0] && Rwindow_get_text_input_selection(core, &start, &end)) {
            if (rvn_web_typeable(inserted)) rvn_web_text(inserted);
            else Rwindow_replace_text_input(core, start, end, inserted);
        }
        rvn_web_field_forget();
        rvn_web_field_sync(window);
        return;
    }
    length = (int)strlen(g_field.mirror);
    from = rvn_utf16_to_utf8(g_field.mirror, length, head);
    to = rvn_utf16_to_utf8(g_field.mirror, length, head + removed);
    if (to < from) to = from;
    typed = Rwindow_get_text_input_selection(core, &start, &end) && start == g_field.offset + from &&
            end == g_field.offset + to && rvn_web_typeable(inserted);
    /* the textarea has this now (set first: the text events below send what differs from it) */
    mirror = rvn_alloc((size_t)(length - (to - from)) + strlen(inserted) + 1);
    memcpy(mirror, g_field.mirror, (size_t)from);
    strcpy(mirror + from, inserted);
    strcat(mirror, g_field.mirror + to);
    rvn_free(g_field.mirror);
    g_field.mirror = mirror;
    serial = g_field.serial;
    if (typed) {
        /* (one sync once all of it is typed, not one per character) */
        g_field.applying = R_TRUE;
        rvn_web_text(inserted);
        g_field.applying = R_FALSE;
    } else if (to > from || inserted[0]) {
        Rwindow_replace_text_input(core, g_field.offset + from, g_field.offset + to, inserted);
    }
    /* (the text callbacks may have moved the focus or changed the text, or the part sent moved: the sync sends it all) */
    if (serial == g_field.serial && rvn_web_core(g_field.window) == core && rvn_web_field_current(core, &text))
        rvn_web_field_select_mirror(core, start16, end16);
    rvn_web_field_sync(window);
}

/* The selection moved in the page's textarea. */
EMSCRIPTEN_KEEPALIVE void rvn_web_field_select(int start16, int end16) {
    rvn_pwindow_t* window = g_field.window;
    Rid core = rvn_web_core(window);
    const char* text;
    if (core == R_INVALID_ID || g_window != window || g_field.hints.mode != RVN_WEB_FIELD_MIRROR) return;
    if (rvn_web_field_current(core, &text)) rvn_web_field_select_mirror(core, start16, end16);
    rvn_web_field_sync(window);
}

/* The page composes: the composition `text` (NULL or "": none) with its selection in UTF-16 units. */
EMSCRIPTEN_KEEPALIVE void rvn_web_field_compose(const char* text, int start16, int end16) {
    rvn_pevent_t event = {0};
    int length;
    if (!g_window || g_window != g_field.window) return;
    if (!text || !text[0]) {
        rvn_web_field_end_composition();
        return;
    }
    length = (int)strlen(text);
    event.type = RVN_PEVENT_COMPOSITION;
    event.composition = text;
    event.composition_start = rvn_utf16_to_utf8(text, length, rvn_mini(start16, end16));
    event.composition_end = rvn_utf16_to_utf8(text, length, rvn_maxi(start16, end16));
    g_field.composition = R_TRUE;
    rvn_web_emit_to(g_field.window, &event);
}

EMSCRIPTEN_KEEPALIVE void rvn_web_field_composing(int composing) {
    if (g_window != g_field.window) return;
    g_field.composing = composing != 0;
    /* (what changed while it composed is sent now) */
    if (!composing) rvn_web_field_sync(g_field.window);
}

/* The password input was filled at once (a password manager): it replaces the whole field. */
EMSCRIPTEN_KEEPALIVE void rvn_web_field_fill(const char* text) {
    Rid core = rvn_web_core(g_field.window);
    const char* current;
    if (core == R_INVALID_ID || g_window != g_field.window) return;
    current = Rwindow_get_text_input_text(core);
    if (current) Rwindow_replace_text_input(core, 0, (int)strlen(current), text);
    else rvn_web_text(text);
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
    rvn_web_loop_poke();
#if defined(RAVEN_BACKEND_WEBGL_ENABLED)
    rvn_webgl_context_lost();
#endif
}

EMSCRIPTEN_KEEPALIVE void rvn_web_context_restored(void) {
    rvn_web_loop_poke();
    Rbackend_device_lost();
}

/* The safe area or the on-screen keyboard over the canvas changed (state.measure). */
EMSCRIPTEN_KEEPALIVE void rvn_web_insets(void) {
    rvn_pevent_t event = {0};
    event.type = RVN_PEVENT_INSETS;
    rvn_web_emit(&event);
}

static void rvn_web_safe_area(rvn_pwindow_t* window, float insets[4]) {
    int i;
    for (i = 0; i < 4; i++) insets[i] = (float)EM_ASM_DOUBLE({
        const state = Module.ravenWindows.get($0);
        return state ? state.insets[$1] : 0;
    }, window, i);
}

static float rvn_web_keyboard_height(rvn_pwindow_t* window) {
    return (float)EM_ASM_DOUBLE({
        const state = Module.ravenWindows.get($0);
        return state ? state.keyboard : 0;
    }, window);
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
    rvn_free(g_field.mirror);
    memset(&g_field, 0, sizeof(g_field));
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
    if (g_field.window == window) {
        rvn_web_field_forget();
        memset(&g_field.hints, 0, sizeof(g_field.hints));
        g_field.window = NULL;
        g_field.focus = R_INVALID_ID;
        g_field.composing = g_field.composition = R_FALSE;
    }
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
        /* (at the caret: the candidates of an input method go there) */
        EM_ASM({
            const state = Module.ravenWindows.get($0);
            if (!state) return;
            const rect = state.canvas.getBoundingClientRect();
            const scale = state.canvas.width / rect.width;
            for (const node of [state.textarea, state.secure]) {
                node.style.left = (rect.left + $1 / scale) + 'px';
                node.style.top = (rect.top + $2 / scale) + 'px';
                node.style.height = Math.max(1, $3 / scale) + 'px';
            }
        }, window, x, y, height);
    }
    /* what the application changed in the focused field since (a timer, a call from JavaScript) */
    {
        rvn_pwindow_t* field = NULL;
        for (window = g_windows; window; window = window->next) {
            rvn_window_t* owner = window->user;
            if (owner && !owner->dead && owner->active && (!field || window == g_field.window)) field = window;
        }
        if (field) rvn_web_field_sync(field);
    }
}
static void rvn_web_wake(void) { rvn_web_loop_poke(); }
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
        .a11y_active = rvn_web_a11y_active, .a11y_settings = rvn_web_a11y_settings,
        .window_get_safe_area = rvn_web_safe_area,
        .window_get_keyboard_height = rvn_web_keyboard_height
    };
    return &table;
}

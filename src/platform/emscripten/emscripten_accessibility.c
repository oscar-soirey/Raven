#include "../platform.h"
#include "../../core/accessibility.h"
#include <emscripten.h>

EM_JS(void, rvn_web_access_begin, (Rid window), {
    let root = document.getElementById('raven-accessibility');
    if (!root) {
        root = document.createElement('div'); root.id = 'raven-accessibility';
        Object.assign(root.style, {position:'absolute', width:'1px', height:'1px',
            overflow:'hidden', clipPath:'inset(50%)', whiteSpace:'nowrap'});
        document.body.appendChild(root);
    }
    let subtree = document.getElementById('raven-access-window-' + window);
    if (!subtree) {
        subtree = document.createElement('div');
        subtree.id = 'raven-access-window-' + window; root.appendChild(subtree);
    }
    for (const node of subtree.querySelectorAll('[data-raven-node]'))
        node.dataset.seen = 'false';
    subtree.dataset.window = window;
    for (const state of Module.ravenWindows.values())
        if (state.window_id === window) subtree.hidden = !state.visible;
    Module.ravenAccessRoot = subtree;
})

EM_JS(void, rvn_web_access_node, (uint32_t lo, uint32_t hi,
        uint32_t parent_lo, uint32_t parent_hi, const char* role,
        const char* name, const char* description, const char* value,
        uint32_t states, double number, double minimum, double maximum), {
    const id = 'raven-node-' + hi + '-' + lo;
    const rawRole = UTF8ToString(role);
    const roleName = states & 262144 ? 'row' : states & 8192 ? 'textbox' : rawRole === 'table' ? 'grid' : rawRole;
    const tag = roleName === 'textbox' ? (states & 512 ? 'textarea' : 'input') : 'div';
    let node = document.getElementById(id);
    if (node && node.tagName !== tag.toUpperCase()) {
        node.remove(); node = null;
    }
    const signature = JSON.stringify([roleName,UTF8ToString(name),
        UTF8ToString(description),UTF8ToString(value),states,number,minimum,maximum]);
    const parent = document.getElementById(
        'raven-node-' + parent_hi + '-' + parent_lo) || Module.ravenAccessRoot;
    if (node) node.dataset.focused = states & 2 ? 'true' : 'false';
    if (node && node.ravenSignature === signature) {
        node.dataset.seen = 'true';
        if (node.parentNode !== parent) parent.appendChild(node);
        return;
    }
    if (!node) {
        node = document.createElement(tag);
        node.id = id; node.dataset.ravenNode = 'true';
        node.addEventListener('focus', () => _rvn_web_access_act(lo, hi, 5));
        node.addEventListener('click', event => {
            if (node.dataset.disabled === 'true' ||
                (node.dataset.invokable !== 'true' &&
                 node.dataset.checkable !== 'true')) return;
            event.stopPropagation();
            _rvn_web_access_act(lo, hi, node.dataset.checkable === 'true' ? 2 : 1);
        });
        if (roleName === 'textbox') node.addEventListener('input', event => {
            event.stopPropagation();
            const ptr = stringToNewUTF8(node.value);
            _rvn_web_access_value(lo, hi, ptr); _free(ptr);
        });
        node.addEventListener('keydown', e => {
            if (roleName === 'textbox') { e.stopPropagation(); return; }
            if ((e.key === 'Enter' || e.key === ' ') &&
                (node.dataset.invokable === 'true' ||
                 node.dataset.checkable === 'true')) {
                _rvn_web_access_act(lo, hi,
                    node.dataset.checkable === 'true' ? 2 : 1);
                e.preventDefault(); e.stopPropagation();
            } else if (node.hasAttribute('aria-valuenow') &&
                       ['ArrowUp','ArrowRight','ArrowDown','ArrowLeft',
                        'Home','End'].includes(e.key)) {
                const min = Number(node.getAttribute('aria-valuemin'));
                const max = Number(node.getAttribute('aria-valuemax'));
                const current = Number(node.getAttribute('aria-valuenow'));
                const next = e.key === 'Home' ? min : e.key === 'End' ? max :
                    current + (e.key === 'ArrowUp' || e.key === 'ArrowRight' ? 1 : -1)
                    * Math.max((max-min)/100, 0.01);
                _rvn_web_access_number(lo, hi, Math.max(min,Math.min(max,next)));
                e.preventDefault(); e.stopPropagation();
            }
        });
    }
    node.dataset.focused = states & 2 ? 'true' : 'false';
    node.ravenSignature = signature;
    node.dataset.seen = 'true';
    node.dataset.disabled = states & 4 ? 'true' : 'false';
    node.dataset.checkable = states & 16 ? 'true' : 'false';
    node.dataset.invokable = states & 16384 ? 'true' : 'false';
    node.setAttribute('role', roleName);
    node.setAttribute('aria-label', UTF8ToString(name));
    const desc = UTF8ToString(description);
    if (desc) node.setAttribute('aria-description', desc);
    else node.removeAttribute('aria-description');
    if (roleName === 'textbox') {
        if (tag === 'input') node.type = states & 2048 ? 'password' : 'text';
        node.value = UTF8ToString(value);
        node.readOnly = !!(states & 1024);
        node.disabled = !!(states & 4);
    }
    for (const name of ['aria-disabled','aria-checked','aria-pressed','aria-expanded',
        'aria-selected','aria-readonly','aria-multiline','aria-modal','aria-multiselectable',
        'aria-valuenow','aria-valuemin','aria-valuemax']) node.removeAttribute(name);
    if (states & 4) node.setAttribute('aria-disabled', 'true');
    if (states & 16) node.setAttribute(roleName === 'button' ? 'aria-pressed' : 'aria-checked',
        states & 8 ? 'true' : 'false');
    if (states & 64) node.setAttribute('aria-expanded', states & 32 ? 'true' : 'false');
    if (states & 256) node.setAttribute('aria-selected', states & 128 ? 'true' : 'false');
    if (states & 1024) node.setAttribute('aria-readonly', 'true');
    if (states & 512) node.setAttribute('aria-multiline', 'true');
    if (states & 65536) node.setAttribute('aria-modal', 'true');
    if (states & 131072) node.setAttribute('aria-multiselectable', 'true');
    if (states & 4096) {
        node.setAttribute('aria-valuenow', number);
        node.setAttribute('aria-valuemin', minimum);
        node.setAttribute('aria-valuemax', maximum);
    }
    if ((states & 1) && !(states & 4)) node.tabIndex = 0;
    else node.removeAttribute('tabindex');
    if (node.parentNode !== parent) parent.appendChild(node);

})

EM_JS(void, rvn_web_access_end, (void), {
    for (const node of Module.ravenAccessRoot.querySelectorAll('[data-seen="false"]'))
        node.remove();
    const root = Module.ravenAccessRoot;
    const focused = root.querySelector('[data-focused="true"]');
    for (const state of Module.ravenWindows.values()) {
        if (state.window_id !== Number(root.dataset.window)) continue;
        if (focused) state.canvas.setAttribute('aria-activedescendant', focused.id);
        else state.canvas.removeAttribute('aria-activedescendant');
    }
})

EMSCRIPTEN_KEEPALIVE int rvn_web_access_value(uint32_t lo, uint32_t hi,
                                             const char* text) {
    return rvn_a11y_set_value(((uint64_t)hi << 32) | lo, text);
}
EMSCRIPTEN_KEEPALIVE int rvn_web_access_number(uint32_t lo, uint32_t hi,
                                              double value) {
    return rvn_a11y_set_number(((uint64_t)hi << 32) | lo, value);
}

void rvn_web_access_focus_window(Rid window);
EMSCRIPTEN_KEEPALIVE int rvn_web_access_act(uint32_t lo, uint32_t hi,
                                            int action) {
    rvn_a11y_id_t id = ((uint64_t)hi << 32) | lo;
    if (action == RVN_A11Y_FOCUS) rvn_web_access_focus_window(rvn_a11y_window(id));
    return rvn_a11y_act(id, (rvn_a11y_action_t)action);
}

static const char* rvn_web_role(RAccessibleRole role) {
    switch (role) {
        case R_ROLE_WINDOW: return "application";
        case R_ROLE_TEXT: return "paragraph";
        case R_ROLE_HEADING: return "heading";
        case R_ROLE_LINK: return "link";
        case R_ROLE_BUTTON: case R_ROLE_TOGGLE_BUTTON: return "button";
        case R_ROLE_CHECKBOX: return "checkbox";
        case R_ROLE_RADIO_BUTTON: return "radio";
        case R_ROLE_SLIDER: return "slider";
        case R_ROLE_PROGRESS_BAR: return "progressbar";
        case R_ROLE_SPIN_BUTTON: return "spinbutton";
        case R_ROLE_TEXT_FIELD: return "textbox";
        case R_ROLE_DOCUMENT: return "document";
        case R_ROLE_COMBO_BOX: return "combobox";
        case R_ROLE_LIST: return "list";
        case R_ROLE_LIST_ITEM: return "listitem";
        case R_ROLE_TREE: return "tree";
        case R_ROLE_TREE_ITEM: return "treeitem";
        case R_ROLE_TABLE: return "table";
        case R_ROLE_MENU_BAR: return "menubar";
        case R_ROLE_MENU: return "menu";
        case R_ROLE_MENU_ITEM: return "menuitem";
        case R_ROLE_TAB_LIST: return "tablist";
        case R_ROLE_TAB: return "tab";
        case R_ROLE_TAB_PANEL: return "tabpanel";
        case R_ROLE_TOOLBAR: return "toolbar";
        case R_ROLE_STATUS_BAR: return "status";
        case R_ROLE_DIALOG: return "dialog";
        case R_ROLE_ALERT: return "alert";
        case R_ROLE_IMAGE: return "img";
        case R_ROLE_SEPARATOR: return "separator";
        case R_ROLE_SCROLL_BAR: return "scrollbar";
        default: return "group";
    }
}

static void rvn_web_access_tree(rvn_a11y_id_t root) {
    typedef struct rvn_web_access_item_t {
        rvn_a11y_id_t id, parent;
    } rvn_web_access_item_t;
    rvn_web_access_item_t* stack = NULL;
    uint32_t used = 0, capacity = 0;
    RVN_ARRAY_RESERVE(stack, capacity, 1);
    stack[used++] = (rvn_web_access_item_t){root, 0};
    while (used) {
        rvn_web_access_item_t item = stack[--used];
        rvn_a11y_node_t node;
        int count, i;
        if (!rvn_a11y_describe(item.id, &node)) continue;
        rvn_web_access_node((uint32_t)item.id, (uint32_t)(item.id >> 32),
            (uint32_t)item.parent, (uint32_t)(item.parent >> 32),
            rvn_web_role(node.role), node.name ? node.name : "",
            node.description ? node.description : "",
            node.value ? node.value : "", node.states,
            node.number, node.minimum, node.maximum);
        count = rvn_a11y_child_count(item.id);
        for (i = count - 1; i >= 0; i--) {
            rvn_a11y_id_t child = rvn_a11y_child(item.id, i);
            RVN_ARRAY_RESERVE(stack, capacity, used + 1);
            stack[used++] = (rvn_web_access_item_t){child, item.id};
        }
    }
    rvn_free(stack);
}

void rvn_web_a11y_event(rvn_pwindow_t* window, int event, uint64_t node) {
    RVN_UNUSED(window); RVN_UNUSED(event);
    rvn_web_access_begin(rvn_a11y_window(node));
    rvn_web_access_tree(rvn_a11y_window(node));
    rvn_web_access_end();
}
void rvn_web_a11y_announce(rvn_pwindow_t* window, const char* text,
                          int assertive) {
    RVN_UNUSED(window);
    EM_ASM({
        let live = document.getElementById('raven-announcement');
        if (!live) {
            live = document.createElement('div'); live.id = 'raven-announcement';
            Object.assign(live.style, {position:'absolute',width:'1px',
                height:'1px',overflow:'hidden',clipPath:'inset(50%)'});
            document.body.appendChild(live);
        }
        live.setAttribute('aria-live', $1 ? 'assertive' : 'polite');
        live.textContent = UTF8ToString($0);
    }, text ? text : "", assertive);
}
int rvn_web_a11y_active(void) { return R_TRUE; }
void rvn_web_a11y_settings(RAccessibilitySettings* settings) {
    memset(settings, 0, sizeof(*settings));
    settings->text_scale = 1;
    settings->reduce_motion = EM_ASM_INT({
        return matchMedia('(prefers-reduced-motion: reduce)').matches;
    });
    settings->high_contrast = EM_ASM_INT({
        return matchMedia('(forced-colors: active)').matches ||
               matchMedia('(prefers-contrast: more)').matches;
    });
}

/* Exercise the actual browser adapter bodies without a browser or Wasm build. */
const fs = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const path = require('node:path');
const platform = fs.readFileSync(path.join(__dirname,
    '../../src/platform/emscripten/emscripten_platform.c'), 'utf8');
const accessibility = fs.readFileSync(path.join(__dirname,
    '../../src/platform/emscripten/emscripten_accessibility.c'), 'utf8');
function body(source, name) {
    return source.match(new RegExp('EM_JS\\([^\\n]*' + name +
        ',[\\s\\S]*?, \\{\\n([\\s\\S]*?)\\n\\}\\)'))[1];
}
class Target {
    constructor(tag = 'div') {
        this.events = {}; this.style = {}; this.dataset = {}; this.attrs = {};
        this.tagName = tag.toUpperCase(); this.id = 'canvas';
        this.tabIndex = -1; this.width = 200; this.height = 100;
        this.value = ''; this.selectionStart = 0; this.selectionEnd = 0;
        this.isConnected = true;
    }
    setSelectionRange(start, end) { this.selectionStart = start; this.selectionEnd = end; }
    addEventListener(name, fn) { this.events[name] = fn; }
    setAttribute(name, value) { this.attrs[name] = value; }
    removeAttribute(name) {
        delete this.attrs[name];
        if (name === 'tabindex') delete this.tabIndex;
    }
    appendChild(node) { node.parentNode = this; nodes.set(node.id, node); }
    getBoundingClientRect() { return {left:0, top:0, right:200, bottom:100, width:200, height:100}; }
    setPointerCapture() { this.capture = true; }
    hasPointerCapture() { return this.capture; }
    releasePointerCapture() { this.capture = false; }
    focus() {}
    querySelectorAll() { return []; }
    querySelector() {
        return [...nodes.values()].find(n => n.dataset.focused === 'true');
    }
}
const nodes = new Map(), canvas = new Target('canvas');
const document = new Target(), window = new Target();
document.createElement = tag => new Target(tag);
document.getElementById = id => nodes.get(id);
document.body = new Target(); document.activeElement = canvas;
document.documentElement = {clientWidth:200};
// a phone: notch and home indicator, a visual viewport the keyboard shrinks
const viewport = new Target(); viewport.offsetTop = 0; viewport.height = 100; viewport.scale = 1;
window.visualViewport = viewport;
const safeArea = {paddingTop:'47px', paddingRight:'0px', paddingBottom:'34px', paddingLeft:'0px'};
const calls = [], queued = [];
const context = {
    Module: {canvas, ravenWindows:new Map()}, document, window,
    AbortController, ResizeObserver:class { observe() {} },
    stringToNewUTF8:x => x, UTF8ToString:x => x, _free() {},
    queueMicrotask:fn => queued.push(fn),
    getComputedStyle:() => safeArea, innerWidth:200, innerHeight:100
};
for (const name of ['target','text','key','focus','pointer','window_moved',
    'mouse_leave','wheel','drag','drop','resize','expose','insets','field_edit',
    'field_select','field_compose','field_composing','field_fill'])
    context['_rvn_web_' + name] = (...args) => calls.push([name, ...args]);
context._rvn_web_wants_text = () => false;
vm.createContext(context);
function run(source, name, params, args) {
    context.args = args;
    return vm.runInContext('(function(' + params + '){' + body(source, name) +
        '})(...args)', context);
}
run(platform, 'rvn_dom_attach', 'handle,primary,width,height,window_id',
    [1, 1, 200, 100, 9]);
const state = context.Module.ravenWindows.get(1);
assert.deepEqual(Array.from(state.insets), [47,0,34,0], 'safe area where the canvas reaches the notch');
const event = {clientX:10, clientY:20, pointerId:3, pointerType:'mouse',
    button:0, buttons:1, pressure:0, preventDefault() {}};
canvas.events.pointerdown(event);
state.drag = {x:10,y:20,left:0,top:0,width:200,height:100,edge:-1};
canvas.events.pointermove({...event, clientX:15});
canvas.events.pointerup({...event, buttons:0});
assert.deepEqual(calls.filter(c => c[0] === 'pointer').map(c => c[1]), [0,2]);
assert.equal(canvas.capture, false);
const before = calls.filter(c => c[0] === 'key').length;
state.textarea.events.compositionstart({});
state.textarea.events.keydown({code:'Enter', isComposing:true,
    preventDefault() { throw Error('IME commit prevented'); }});
assert.equal(calls.filter(c => c[0] === 'key').length, before);
state.textarea.events.compositionend({data:''});
// The textarea of a TextInput: edits become replaced ranges, attributed to the caret.
const textarea = state.textarea;
const edits = () => calls.filter(c => c[0] === 'field_edit').map(c => c.slice(1));
const composes = () => calls.filter(c => c[0] === 'field_compose').map(c => c.slice(1));
const clear = () => { calls.length = 0; };
run(platform, 'rvn_dom_field',
    'handle,mode,inputmode,enterkeyhint,autocomplete,autocapitalize,spellcheck,read_only',
    [1, 1, 'email', 'next', 'email', 'off', 0, 0]);
assert.equal(textarea.attrs.inputmode, 'email');
assert.equal(textarea.attrs.enterkeyhint, 'next');
assert.equal(textarea.spellcheck, false);
run(platform, 'rvn_dom_field_text', 'handle,text,length,start,end', [1, 'teh cat', 7, 3, 3]);
assert.equal(textarea.value, 'teh cat');
assert.deepEqual([textarea.selectionStart, textarea.selectionEnd], [3, 3]);
const input = (type, value, caret, data = null, isComposing = false) => {
    textarea.value = value; textarea.setSelectionRange(caret, caret);
    textarea.events.input({inputType:type, data, isComposing});
};
clear(); input('insertReplacementText', 'the cat', 3, 'the');
assert.deepEqual(edits(), [[1, 2, 'he', 3, 3]], 'autocorrection: the middle replaced');
clear(); input('deleteContentBackward', 'th cat', 2);
assert.deepEqual(edits(), [[2, 1, '', 2, 2]], 'backspace at the caret');
clear(); input('insertText', 'thh cat', 3, 'h');
assert.deepEqual(edits(), [[2, 0, 'h', 3, 3]], 'a repeated letter goes at the caret');
document.activeElement = textarea;
clear(); textarea.setSelectionRange(1, 3); textarea.events.selectionchange({});
assert.deepEqual(calls.filter(c => c[0] === 'field_select'), [['field_select', 1, 3]]);
// A word composed again: removed from the field, shown as the composition.
clear(); textarea.events.compositionstart({});
input('insertCompositionText', 'thh cat', 7, 'cat', true);
assert.deepEqual(edits(), [[4, 3, '', 4, 4]]);
assert.deepEqual(composes(), [['cat', 3, 3]]);
clear(); input('insertCompositionText', 'thh cart', 8, 'cart', true);
assert.deepEqual(edits(), [], 'the field without the composition is unchanged');
assert.deepEqual(composes(), [['cart', 4, 4]]);
clear(); textarea.events.compositionend({data:'cart'});
assert.deepEqual(composes(), [[0, 0, 0]]);
assert.deepEqual(edits(), [[4, 0, 'cart', 8, 8]], 'the commit typed at the caret');
// Characters out of the BMP are never cut in two.
run(platform, 'rvn_dom_field_text', 'handle,text,length,start,end', [1, 'a\u{1F600}', 5, 3, 3]);
clear(); input('insertText', 'a\u{1F600}\u{1F601}', 5, '\u{1F601}');
assert.deepEqual(edits(), [[3, 0, '\u{1F601}', 5, 5]]);
// The keyboard comes while the textarea has the focus: what it covers of the canvas.
clear(); viewport.height = 30; viewport.events.resize({});
assert.equal(state.keyboard, 70);
assert.ok(calls.some(c => c[0] === 'insets'));
viewport.height = 100; viewport.events.resize({});
assert.equal(state.keyboard, 0);
document.activeElement = canvas;
state.textarea.events.blur({});
context.Module.ravenWindows.set(1, {});
const count = calls.length; queued.forEach(fn => fn());
assert.equal(calls.length, count);
context.Module.ravenWindows.set(1, state);
run(accessibility, 'rvn_web_access_begin', 'window', [9]);
const root = context.Module.ravenAccessRoot;
assert.equal(root.hidden, true, 'new hidden window must hide its ARIA tree');
function render(states, description = '') {
    run(accessibility, 'rvn_web_access_node',
        'lo,hi,parent_lo,parent_hi,role,name,description,value,states,number,minimum,maximum',
        [1,0,0,0,'button','Name',description,'',states,0,0,0]);
}
render(3, 'Old description');
let node = nodes.get('raven-node-0-1');
assert.equal(node.tabIndex, 0);
run(accessibility, 'rvn_web_access_end', '', []);
assert.equal(canvas.attrs['aria-activedescendant'], node.id);
render(0);
run(accessibility, 'rvn_web_access_end', '', []);
assert.equal(node.attrs['aria-description'], undefined);
assert.equal(node.tabIndex, undefined);
assert.equal(canvas.attrs['aria-activedescendant'], undefined);
run(platform, 'rvn_dom_state', 'handle,state', [1,2]);
assert.equal(root.hidden, false);
run(platform, 'rvn_dom_state', 'handle,state', [1,1]);
assert.equal(root.hidden, true);
console.log('Platform regressions passed: drag, IME, text field edits and composition, ' +
            'keyboard and safe area, lifecycle, ARIA focus and visibility');

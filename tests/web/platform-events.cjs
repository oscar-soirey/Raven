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
    }
    addEventListener(name, fn) { this.events[name] = fn; }
    setAttribute(name, value) { this.attrs[name] = value; }
    removeAttribute(name) {
        delete this.attrs[name];
        if (name === 'tabindex') delete this.tabIndex;
    }
    appendChild(node) { node.parentNode = this; nodes.set(node.id, node); }
    getBoundingClientRect() { return {left:0, top:0, width:200, height:100}; }
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
const calls = [], queued = [];
const context = {
    Module: {canvas, ravenWindows:new Map()}, document, window,
    AbortController, ResizeObserver:class { observe() {} },
    stringToNewUTF8:x => x, UTF8ToString:x => x, _free() {},
    queueMicrotask:fn => queued.push(fn)
};
for (const name of ['target','text','key','focus','pointer','window_moved',
    'mouse_leave','wheel','drag','drop','resize','expose'])
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
console.log('Platform regressions passed: drag, IME, lifecycle, ARIA focus and visibility');

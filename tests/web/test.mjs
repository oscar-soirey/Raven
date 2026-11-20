/* Run against a served RAVEN_BUILD_WEB_TESTS build.
 * RAVEN_WEB_URL defaults to http://127.0.0.1:8765/raven_web_test.html.
 * RAVEN_BROWSER optionally selects an installed Chromium executable. */
import assert from 'node:assert/strict';
import {browserDiagnostics} from './diagnostics.mjs';
import {readFile} from 'node:fs/promises';
import { chromium, firefox, webkit } from 'playwright';
const engine = process.env.RAVEN_BROWSER_ENGINE || 'chromium';

const browser = await ({chromium,firefox,webkit}[engine]).launch({
  executablePath: process.env.RAVEN_BROWSER || undefined,
  headless: true,
  firefoxUserPrefs: {'webgl.force-enabled':true},
  args: engine === 'chromium' ? ['--no-sandbox', '--use-gl=angle', '--use-angle=swiftshader',
         '--enable-unsafe-swiftshader'] : [],
});
const context = await browser.newContext({
  ...(process.env.RAVEN_WEB_BYPASS_PROXY === '1' ? {
    proxy:{server:'http://127.0.0.1:9',bypass:'127.0.0.1,localhost'}
  } : {}),
  viewport:{width:800,height:600},
                                          hasTouch:engine === 'chromium',deviceScaleFactor:2});
const page = await context.newPage();
// Arm picker interception before interaction; enabling it at keypress can race.
page.on('filechooser',()=>{});
const diagnose = browserDiagnostics(page);
if (engine === 'firefox') {
  // Headless Firefox emits native 'cancel' before Playwright setFiles delivers
  // 'change', even for a plain HTML input. Ignore that automation-only event.
  await page.addInitScript(() => document.addEventListener('cancel', event => {
    if (event.target.matches('input[type=file]')) event.stopImmediatePropagation();
  }, true));
}
// A visual viewport the test can shrink, as an on-screen keyboard does on a phone.
await page.addInitScript(() => {
  const viewport = new EventTarget();
  viewport.keyboard = 0;
  Object.defineProperties(viewport, {
    offsetLeft:{get:() => 0}, offsetTop:{get:() => 0}, scale:{get:() => 1},
    width:{get:() => innerWidth}, height:{get:() => innerHeight - viewport.keyboard}});
  Object.defineProperty(window, 'visualViewport', {value:viewport, configurable:true});
  window.ravenTestViewport = viewport;
});
const errors = [];
page.on('pageerror', error => errors.push(error.stack || error.message));
page.on('console', message => {
  if (message.type() === 'error' && !message.text().includes('404'))
    errors.push(message.text());
});
const call = (name, args = [], types = [], result = 'number') =>
  page.evaluate(({name,args,types,result}) =>
    Module.ccall(name, result, types, args), {name,args,types,result});
const wait = async fn => {
  for (let i=0;i<100;i++) {
    if (await fn()) return;
    await page.waitForTimeout(50);
  }
  throw new Error('Timed out waiting for browser state');
};
try {
  await page.goto(process.env.RAVEN_WEB_URL ||
                  'http://127.0.0.1:8765/raven_web_test.html');
  await wait(() => page.locator('[role=textbox]').count().then(n=>n===1));
  assert.equal(await call('test_width'),1600,'device pixel ratio');
  // An idle page asks the browser for no frames; input and timers wake it.
  // (Module.ravenLoop.frames counts them; reading it calls nothing in Raven.)
  const frames = () => page.evaluate(() => Module.ravenLoop.frames);
  const settle = async () => {
    let last = await frames();
    for (let i=0;i<40;i++) {
      await page.waitForTimeout(250);
      const now = await frames();
      if (now === last) return now;
      last = now;
    }
    throw new Error('The page never stopped drawing');
  };
  let idle = await settle();
  await page.waitForTimeout(2000);
  assert.equal(await frames(), idle, 'no frame while idle');
  await page.mouse.move(300,300);
  await wait(async () => (await frames()) > idle);
  idle = await settle();
  await page.evaluate(() => Module.ccall('test_timer',null,['number'],[0.6]));
  const asked = await frames();
  await page.getByLabel('Timer fired',{exact:true}).waitFor({state:'attached'});
  assert.ok((await frames()) - asked < 10, 'a timer wakes the loop when it is due, not every frame');
  await settle();
  const input = page.locator('[role=textbox]');
  assert.equal(await input.getAttribute('aria-label'),'Test input');
  // Use visible canvas coordinates; the ARIA mirror is not the visual widget.
  await page.mouse.click(100,40);
  await page.keyboard.press('Control+a');
  await page.keyboard.type('Browser text');
  await wait(async () => (await call('test_text',[],[],'string'))==='Browser text');
  await page.keyboard.press('Backspace');
  assert.equal(await call('test_text',[],[],'string'),'Browser tex');
  await page.locator('textarea').evaluate(node => {
    node.dispatchEvent(new CompositionEvent('compositionstart',{data:""}));
    node.dispatchEvent(new InputEvent('input',{data:'仮',isComposing:true}));
    node.dispatchEvent(new CompositionEvent('compositionend',{data:'日本語'}));
    node.dispatchEvent(new InputEvent('input',{data:'日本語',isComposing:false}));
  });
  assert.equal(await call('test_text',[],[],'string'),'Browser tex日本語');
  await page.locator('textarea').evaluate(node => {
    const data = new DataTransfer(); data.setData('text/plain',' paste');
    const event = new Event('paste',{cancelable:true});
    Object.defineProperty(event,'clipboardData',{value:data});
    node.dispatchEvent(event);
  });
  assert.equal(await call('test_text',[],[],'string'),'Browser tex日本語 paste');
  // The textarea holds the text around the caret, for the predictive text and
  // corrections of on-screen keyboards; their edits replace ranges of the field.
  const area = page.locator('textarea');
  const text = () => call('test_text',[],[],'string');
  const areaState = () => area.evaluate(node => ({value:node.value, start:node.selectionStart,
    end:node.selectionEnd, spellcheck:node.spellcheck, autocorrect:node.getAttribute('autocorrect'),
    autocapitalize:node.getAttribute('autocapitalize')}));
  assert.deepEqual(await areaState(), {value:'Browser tex日本語 paste', start:20, end:20,
    spellcheck:true, autocorrect:'on', autocapitalize:'sentences'}, 'surrounding text and hints');
  const edit = (type, value, caret, data = null) => area.evaluate((node, [type, value, caret, data]) => {
    const before = new InputEvent('beforeinput',{inputType:type,data,cancelable:true,bubbles:true});
    if (!node.dispatchEvent(before)) return false;
    node.value = value; node.setSelectionRange(caret,caret);
    node.dispatchEvent(new InputEvent('input',{inputType:type,data,bubbles:true}));
    return true;
  }, [type, value, caret, data]);
  const select = async (start, end, bytes) => {
    await area.evaluate((node, [start, end]) => node.setSelectionRange(start, end), [start, end]);
    await wait(async () => (await call('test_selection')) === bytes[0]*10000+bytes[1]);
  };
  assert.ok(await edit('insertReplacementText','Browser tex日本語 pasta',20,'pasta'));
  assert.equal(await text(),'Browser tex日本語 pasta','autocorrect replacement');
  assert.ok(await edit('deleteWordBackward','Browser tex日本語',14));
  assert.equal(await text(),'Browser tex日本語','word deleted backward');
  await select(7,7,[7,7]);
  assert.ok(await edit('insertText','Browser! tex日本語',8,'!'));
  assert.equal(await text(),'Browser! tex日本語','typed in the middle');
  assert.ok(await edit('deleteContentBackward','Browser tex日本語',7),'the keyboard deletes in the mirror');
  assert.equal(await text(),'Browser tex日本語','deleted backward');
  assert.equal(await call('test_selection'),70007);
  // A word composed again (Android keyboards): it leaves the field while composed.
  await select(11,11,[11,11]);
  await area.evaluate(node => {
    node.dispatchEvent(new CompositionEvent('compositionstart',{data:''}));
    node.dispatchEvent(new InputEvent('input',{inputType:'insertCompositionText',data:'tex',isComposing:true}));
  });
  assert.equal(await text(),'Browser 日本語','the recomposed word left the field');
  assert.equal(await call('test_composition',[],[],'string'),'tex');
  await area.evaluate(node => {
    node.value = 'Browser text日本語'; node.setSelectionRange(12,12);
    node.dispatchEvent(new InputEvent('input',{inputType:'insertCompositionText',data:'text',isComposing:true}));
  });
  assert.equal(await call('test_composition',[],[],'string'),'text');
  await area.evaluate(node => node.dispatchEvent(new CompositionEvent('compositionend',{data:'text'})));
  assert.equal(await text(),'Browser text日本語','composition committed');
  assert.equal(await call('test_composition',[],[],'string'),'');
  // Raven moves the selection or changes the text: the textarea follows.
  await call('test_select',[0,7],['number','number']);
  await wait(async () => { const s = await areaState(); return s.start === 0 && s.end === 7; });
  await call('test_set_text',['Fresh 😀 text'],['string']);
  await wait(async () => (await areaState()).value === 'Fresh 😀 text');
  await select(9,9,[11,11]);
  assert.ok(await edit('insertText','Fresh 😀 !text',10,'!'));
  assert.equal(await text(),'Fresh 😀 !text','UTF-16 offsets after an emoji');
  // The on-screen keyboard covers the bottom of the canvas: Raven lays out above it.
  await page.evaluate(() => { ravenTestViewport.keyboard = 250;
    ravenTestViewport.dispatchEvent(new Event('resize')); });
  await wait(async () => (await call('test_keyboard')) === 250);
  await page.evaluate(() => { ravenTestViewport.keyboard = 0;
    ravenTestViewport.dispatchEvent(new Event('resize')); });
  await wait(async () => (await call('test_keyboard')) === 0);
  await page.keyboard.press('Tab'); await page.keyboard.press('Enter');
  assert.equal(await call('test_clicks'),1,'keyboard activation');
  if (engine === 'chromium') {
    await page.touchscreen.tap(100,await call('test_button_y'));
    assert.equal(await call('test_clicks'),2,'touch activation');
  }
  await wait(async () => await page.getByRole('button',{
    name:engine === 'chromium' ? 'Clicks: 2' : 'Clicks: 1',exact:true
  }).count() === 1);
  await call('test_unfocused_label');
  await wait(async () => await page.getByLabel('Updated without focus',{exact:true}).count()===1);
  await input.evaluate(node => {
    node.value='Accessible editing';
    node.dispatchEvent(new Event('input',{bubbles:true}));
  });
  assert.equal(await call('test_text',[],[],'string'),'Accessible editing');
  await input.evaluate(node => {node.focus();node.setSelectionRange(node.value.length,node.value.length);});
  await page.keyboard.type('!');
  await wait(async () => (await call('test_text',[],[],'string'))==='Accessible editing!');
  for (let i=0;i<8;i++) {
    await call('test_effect',[i],['number']);
    await page.waitForTimeout(60);
    assert.equal(await call('test_errors'),0,'built-in effect '+i);
  }
  await call('test_effect',[-1],['number']);
  assert.equal(await call('test_capture'),1,'GPU capture');
  assert.equal(await call('test_pixels'),1,'primitive colors, clipping and glass');
  await page.mouse.move(200,await call('test_scroll_y'));
  await page.mouse.wheel(0,500);
  await wait(async () => (await call('test_scroll_offset'))>0);
  await page.locator('#canvas').evaluate(canvas => {
    const data=new DataTransfer(); data.setData('text/plain','drop text');
    const event=new Event('drop',{cancelable:true});
    Object.defineProperties(event,{
      dataTransfer:{value:data},clientX:{value:200},clientY:{value:200}});
    canvas.dispatchEvent(event);
  });
  assert.equal(await call('test_drops'),1,'browser text drop');
  await call('test_image',[new URL('asset.svg',page.url()).href],['string']);
  await wait(async () => (await call('test_image_ready'))===1);
  assert.equal(await call('test_secondary',[0],['number']),1);
  await wait(() => page.locator('canvas[id^=raven-canvas-]').count().then(n=>n===1));
  await page.waitForTimeout(100);
  const secondary=page.locator('canvas[id^=raven-canvas-]');
  await call('test_secondary_state',[1],['number']);
  assert.equal(Math.round((await secondary.boundingBox()).width),800,'maximize');
  await call('test_secondary_state',[2],['number']);
  assert.equal(await secondary.isVisible(),false,'minimize');
  await call('test_secondary_state',[0],['number']);
  const before=await secondary.boundingBox();
  assert.equal(Math.round(before.width),280,'restore');
  await page.mouse.move(before.x+40,before.y+40); await page.mouse.down();
  await call('test_secondary_state',[3],['number']);
  await page.mouse.move(before.x+70,before.y+60); await page.mouse.up();
  const after=await secondary.boundingBox();
  assert.equal(Math.round(after.x-before.x),30,'window move x');
  assert.equal(Math.round(after.y-before.y),20,'window move y');
  assert.equal(await call('test_secondary',[1],['number']),1);
  assert.equal(await page.locator('canvas[id^=raven-canvas-]').count(),0);
  await call('test_multiline',[0],['number']);
  const editor=page.getByRole('textbox',{name:'Multiline editor'});
  await editor.waitFor({state:'attached'});
  await editor.evaluate(node => {node.focus();node.setSelectionRange(node.value.length,node.value.length);});
  await page.keyboard.press('Enter'); await page.keyboard.type('line two');
  await wait(async () => (await call('test_multiline_text',[],[],'string'))==='line one\nline two');
  await call('test_multiline',[1],['number']);
  // A password field types into a password input that never keeps the text;
  // an email field asks for the email keyboard, without corrections.
  await call('test_fields',[0],['number']);
  const fields=page.locator('canvas[id^=raven-canvas-]');
  await wait(() => fields.count().then(n=>n===1));
  await page.waitForTimeout(100);
  const fieldBox=await fields.boundingBox();
  const clickField=async which => page.mouse.click(
    fieldBox.x+await call('test_field_x',[which],['number']),
    fieldBox.y+await call('test_field_y',[which],['number']));
  const focused=() => page.evaluate(() => { const node=document.activeElement;
    return {tag:node.tagName,type:node.type,value:node.value,
      inputmode:node.getAttribute('inputmode'),autocomplete:node.getAttribute('autocomplete'),
      enterkeyhint:node.getAttribute('enterkeyhint'),spellcheck:node.spellcheck,
      autocorrect:node.getAttribute('autocorrect')}; });
  await clickField(1);
  await wait(async () => (await focused()).type==='password');
  await page.keyboard.type('s3cret');
  await wait(async () => (await call('test_field_text',[1],['number'],'string'))==='s3cret');
  const secret=await focused();
  assert.equal(secret.value,'','the password is not kept in the page');
  assert.equal(secret.autocomplete,'current-password');
  assert.equal(secret.spellcheck,false);
  await clickField(2);
  await wait(async () => (await focused()).tag==='TEXTAREA');
  await page.keyboard.type('me@example.org');
  await wait(async () => (await call('test_field_text',[2],['number'],'string'))==='me@example.org');
  const mail=await focused();
  assert.deepEqual([mail.value,mail.inputmode,mail.autocomplete,mail.enterkeyhint,mail.spellcheck,
    mail.autocorrect],['me@example.org','email','email','next',false,'off'],'email field hints');
  await call('test_fields',[1],['number']);
  await call('test_persist',[0],['number']);
  await wait(async () => (await call('test_io_status'))!==0);
  assert.equal(await call('test_io_status'),1,'mount persistent filesystem');
  await call('test_persist',[1],['number']);
  await wait(async () => (await call('test_io_status'))!==0);
  assert.equal(await call('test_io_status'),1,'flush persistent filesystem');
  const chooserPromise = page.waitForEvent('filechooser');
  await page.mouse.click(100,await call('test_import_y'));
  await (await chooserPromise).setFiles({
    name:'sample.txt',mimeType:'text/plain',buffer:Buffer.from('imported')
  });
  await wait(async () => (await call('test_io_status'))!==0);
  assert.equal(await call('test_io_status'),1,'file import: '+
               await call('test_io_result',[],[],'string'));
  assert.equal(await call('test_import_contents'),1,'imported bytes');
  const downloadPromise = page.waitForEvent('download');
  await call('test_download');
  const download = await downloadPromise;
  assert.equal(download.suggestedFilename(),'raven.txt');
  assert.equal(await readFile(await download.path(),'utf8'),'exported','downloaded bytes');
  await page.evaluate(() => {
    const gl=Module.ravenRenderCanvas.getContext('webgl2');
    const extension=gl.getExtension('WEBGL_lose_context');
    extension.loseContext();
    setTimeout(()=>extension.restoreContext(),100);
  });
  await wait(async () => (await call('test_resets'))===1);
  assert.equal(await call('test_capture'),1,'capture after recovery');
  assert.equal(await call('test_image_ready'),1,'image survives recovery');
  assert.equal(await call('test_errors'),0);
  await page.setViewportSize({width:640,height:480});
  await wait(async () => (await call('test_width'))===1280);
  await page.screenshot({path:process.env.RAVEN_WEB_SCREENSHOT ||
                             '/tmp/raven-web-verified.png'});
  assert.deepEqual(errors,[]);
  await page.reload();
  await wait(() => page.locator('[role=textbox]').count().then(n=>n===1));
  await call('test_persist',[0],['number']);
  await wait(async () => (await call('test_io_status'))!==0);
  assert.equal(await call('test_saved'),1,'IndexedDB survives page reload');
  await call('test_quit');
  await wait(() => page.locator('#raven-render-canvas').count().then(n=>n===0));
  assert.equal(await page.locator('textarea').count(),0,'shutdown removes listeners/input');
  assert.deepEqual(errors,[]);
  console.log('Browser integration checks passed.');
} catch(error) {
  await diagnose();
  throw error;
} finally {
  await browser.close();
}

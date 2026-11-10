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
const diagnose = browserDiagnostics(page);
if (engine === 'firefox') {
  // Headless Firefox emits native 'cancel' before Playwright setFiles delivers
  // 'change', even for a plain HTML input. Ignore that automation-only event.
  await page.addInitScript(() => document.addEventListener('cancel', event => {
    if (event.target.matches('input[type=file]')) event.stopImmediatePropagation();
  }, true));
}
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

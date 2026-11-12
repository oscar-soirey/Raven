import assert from 'node:assert/strict';
import {browserDiagnostics} from './diagnostics.mjs';
import {readFile} from 'node:fs/promises';
import {chromium, firefox, webkit} from 'playwright';
const engine = process.env.RAVEN_BROWSER_ENGINE || 'chromium';
const browser = await ({chromium,firefox,webkit}[engine]).launch({
  executablePath: process.env.RAVEN_BROWSER || undefined,
  headless:true,
  firefoxUserPrefs:{'webgl.force-enabled':true},
  args:engine==='chromium' ? ['--no-sandbox','--use-gl=angle',
    '--use-angle=swiftshader','--enable-unsafe-swiftshader'] : []
});
const base = new URL('.',process.env.RAVEN_WEB_URL ||
  'http://127.0.0.1:8765/raven_web_test.html');
const names = ['hello','editor','glass','notepad','animations','workspace',
  'components','memory','studio','themes','overlay','effects','plot','music'];
let diagnose=async()=>{};
try {
  const context = await browser.newContext({viewport:{width:1000,height:800},
    ...(process.env.RAVEN_WEB_BYPASS_PROXY==='1' ? {
      proxy:{server:'http://127.0.0.1:9',bypass:'127.0.0.1,localhost'}
    } : {})});
  if(engine==='firefox') {
    // Firefox's automated picker cancels before Playwright delivers setFiles.
    await context.addInitScript(()=>document.addEventListener('cancel',event=>{
      if(event.target.matches('input[type=file]')) event.stopImmediatePropagation();
    },true));
  }
  const gallery = await context.newPage();
  diagnose=browserDiagnostics(gallery);
  await gallery.goto(new URL('index.html',base).href);
  assert.equal(await gallery.locator('nav a').count(),names.length);
  await gallery.close();
  for (const name of names) {
    const page = await context.newPage();
    // Arm picker interception before interaction; enabling it at keypress can race.
    page.on('filechooser',()=>{});
    diagnose=browserDiagnostics(page);
    const errors = [];
    page.on('pageerror',error=>errors.push(error.message));
    page.on('console',message=>{
      if(message.type()==='error' && !message.text().includes('404'))
        errors.push(message.text());
    });
    page.on('response',response=>{
      if(response.status()>=400) errors.push(`${response.status()} ${response.url()}`);
    });
    await page.goto(new URL(`raven_${name}.html`,base).href);
    await page.waitForFunction(()=>window.Module?.ravenRenderCanvas &&
      document.querySelector('[role]:not([role=status])'),null,{timeout:30000});
    await page.waitForTimeout(500);
    const colors = await page.locator('#canvas').evaluate(canvas=>{
      const pixels=canvas.getContext('2d').getImageData(0,0,canvas.width,canvas.height).data;
      const colors=new Set();
      for(let i=0;i<pixels.length;i+=64)
        colors.add(`${pixels[i]},${pixels[i+1]},${pixels[i+2]},${pixels[i+3]}`);
      return colors.size;
    });
    assert.ok(colors>8,`${name}: rendered visible UI (${colors} colors)`);
    assert.deepEqual(errors,[],`${name}: browser errors`);
    await page.mouse.move(200,200);
    await page.mouse.wheel(0,100);
    await page.setViewportSize({width:800,height:600});
    await page.waitForTimeout(100);
    assert.deepEqual(errors,[],`${name}: interaction errors`);
    if(name==='hello') {
      await page.getByRole('button',{name:'Click me',exact:true}).evaluate(node=>node.click());
      await page.getByLabel('Clicked 1 time',{exact:true}).waitFor({state:'attached'});
      await page.getByRole('button',{name:'Quit',exact:true}).evaluate(node=>node.click());
      await page.waitForFunction(()=>!document.querySelector('#raven-render-canvas'));
      assert.equal(await page.locator('textarea').count(),0,'hello shutdown cleanup');
    }
    if(name==='notepad') {
      await page.locator('#canvas').focus();
      const chooser=page.waitForEvent('filechooser');
      await page.keyboard.press('Control+o');
      await (await chooser).setFiles({name:'note.txt',mimeType:'text/plain',
        buffer:Buffer.from('Browser example note')});
      await page.waitForFunction(()=>Array.from(document.querySelectorAll('[role=textbox]'))
        .some(node=>node.value==='Browser example note'));
      await page.locator('#canvas').focus();
      const download=page.waitForEvent('download');
      await page.keyboard.press('Control+s');
      const saved=await download;
      assert.ok(saved.suggestedFilename().endsWith('note.txt'));
      assert.equal(await readFile(await saved.path(),'utf8'),'Browser example note');
    }
    assert.deepEqual(errors,[],`${name}: lifecycle/file errors`);
    console.log(`Example passed: ${name}`);
    await page.close();
  }
  const automated=await context.newPage();
  diagnose=browserDiagnostics(automated);
  await automated.route('**/raven_hello.html',async route=>{
    const response=await route.fetch();
    const source=await response.text();
    assert.match(source,/var\s+Module\s*=\s*\{/,'example shell Module initializer');
    const html=source.replace(/var\s+Module\s*=\s*\{/,
      "var Module = { preRun:[function(){ENV.RAVEN_EXAMPLE_FRAMES='3';}],");
    await route.fulfill({response,body:html});
  });
  const report=automated.waitForEvent('console',{
    predicate:message=>message.text().startsWith('last frame:'),timeout:30000});
  await automated.goto(new URL('raven_hello.html',base).href);
  await report;
  await automated.waitForFunction(()=>!document.querySelector('#raven-render-canvas') &&
    !Module.ravenLifecycle);
  assert.equal(await automated.locator('textarea').count(),0,'automated shutdown cleanup');
  await automated.close();
  console.log('Automated example lifecycle passed.');
} catch(error) {
  await diagnose();
  throw error;
} finally { await browser.close(); }

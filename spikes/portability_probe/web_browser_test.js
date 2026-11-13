// Runs a Raven web build (examples/hello built with Emscripten) in headless Chromium and checks that it draws
// and answers the mouse, the keyboard and touch.
//
//   node web_browser_test.js <dir with raven_hello.html> [screenshot.png]
//
// Needs Playwright (require('playwright'), with Chromium). WebGL 2 comes from SwiftShader when there is no GPU.
const path = require('path');
const http = require('http');
const fs = require('fs');
const { chromium } = require('playwright');

const dir = path.resolve(process.argv[2] || '.');
const shot = process.argv[3];
const types = { '.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm', '.data': 'application/octet-stream' };

const server = http.createServer((req, res) => {
  const file = path.join(dir, decodeURIComponent(req.url.split('?')[0]));
  fs.readFile(file, (err, data) => {
    if (err) { res.writeHead(404); res.end(); return; }
    res.writeHead(200, { 'content-type': types[path.extname(file)] || 'application/octet-stream' });
    res.end(data);
  });
});

let failures = 0;

// Where the blue "Click me" button is, in CSS pixels of the page: found in a screenshot, because the size of the UI
// depends on the device (a touch device gets larger controls)
async function findBlueButton(page, context) {
  const png = (await page.screenshot()).toString('base64');
  const helper = await context.newPage();
  const box = await helper.evaluate(async (data) => {
    const image = new Image();
    image.src = 'data:image/png;base64,' + data;
    await image.decode();
    const canvas = document.createElement('canvas');
    canvas.width = image.width; canvas.height = image.height;
    const g = canvas.getContext('2d'); g.drawImage(image, 0, 0);
    const d = g.getImageData(0, 0, canvas.width, canvas.height).data;
    let x0 = 1e9, y0 = 1e9, x1 = -1, y1 = -1;
    for (let y = 0; y < canvas.height; y++) for (let x = 0; x < canvas.width; x++) {
      const i = (y * canvas.width + x) * 4;
      if (d[i] < 80 && d[i + 1] > 110 && d[i + 1] < 150 && d[i + 2] > 225) { x0 = Math.min(x0, x); y0 = Math.min(y0, y); x1 = Math.max(x1, x); y1 = Math.max(y1, y); }
    }
    return { x0, y0, x1, y1, width: image.width };
  }, png);
  await helper.close();
  const factor = box.width / page.viewportSize().width;
  return { x: (box.x0 + box.x1) / 2 / factor, y: (box.y0 + box.y1) / 2 / factor };
}
function check(ok, what) { console.log((ok ? 'ok   ' : 'FAIL ') + what); if (!ok) failures++; }

(async () => {
  await new Promise((r) => server.listen(0, r));
  const base = `http://127.0.0.1:${server.address().port}`;
  const url = `${base}/raven_hello.html`;
  const browser = await chromium.launch({ args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'] });
  const context = await browser.newContext({ viewport: { width: 640, height: 400 }, deviceScaleFactor: 2, hasTouch: true });
  const page = await context.newPage();
  const logs = [];
  page.on('console', (m) => logs.push(m.text()));
  page.on('pageerror', (e) => logs.push('pageerror: ' + e.message));
  await page.goto(url);
  await page.waitForTimeout(2500);

  const size = await page.evaluate(() => ({ w: document.getElementById('canvas').width, h: document.getElementById('canvas').height }));
  check(size.w === 1280 && size.h === 800, `canvas follows the page in physical pixels (${size.w}x${size.h})`);
  if (shot) await page.screenshot({ path: shot });
  await page.screenshot({ path: '/tmp/raven_web_first.png' });

  // find the button by clicking where hello puts it (centered); the label text changes on click
  const text = async () => (await page.screenshot()).length; // placeholder to wait for a frame
  const before = await page.screenshot();
  const button = await findBlueButton(page, context);
  check(button.x > 0, `the button is found at ${Math.round(button.x)},${Math.round(button.y)}`);
  await page.mouse.move(button.x, button.y);
  await page.mouse.down(); await page.mouse.up();
  await page.waitForTimeout(500);
  const afterClick = await page.screenshot();
  check(Buffer.compare(before, afterClick) !== 0, 'a mouse click changes what is shown');

  await page.touchscreen.tap(button.x, button.y);
  await page.waitForTimeout(500);
  const afterTap = await page.screenshot();
  check(Buffer.compare(afterClick, afterTap) !== 0, 'a tap changes what is shown');
  fs.writeFileSync('/tmp/raven_web_after.png', afterTap);

  // text: a tap on the text field raises the (hidden) field that gets the soft keyboard; what is typed, accents too, and pasted arrives
  const focused = () => page.evaluate(() => (document.activeElement && document.activeElement.tagName) || '');
  const beforeTyping = await page.screenshot();
  // the text field is above the buttons: tap upwards from them until the hidden field gets the focus
  let field = false;
  for (let up = 30; up <= 110 && !field; up += 10) {
    await page.touchscreen.tap(button.x, button.y - up);
    await page.waitForTimeout(250);
    field = (await focused()) === 'TEXTAREA';
  }
  check(field, 'a tap on a text field focuses the hidden input field (soft keyboard)');
  await page.keyboard.type('Héllo €');
  await page.waitForTimeout(500);
  const typed = await page.screenshot();
  check(Buffer.compare(beforeTyping, typed) !== 0, 'typed text, accents included, appears');
  await context.grantPermissions(['clipboard-read', 'clipboard-write']);
  await page.evaluate(() => navigator.clipboard.writeText('PASTED'));
  await page.keyboard.press('Control+V');
  await page.waitForTimeout(500);
  const pasted = await page.screenshot();
  check(Buffer.compare(typed, pasted) !== 0, 'Ctrl+V pastes the clipboard');
  await page.touchscreen.tap(320, 100);
  await page.waitForTimeout(400);
  check((await focused()) !== 'TEXTAREA', 'a tap elsewhere releases the field (keyboard hides)');

  await page.setViewportSize({ width: 800, height: 500 });
  await page.waitForTimeout(800);
  const resized = await page.evaluate(() => ({ w: document.getElementById('canvas').width, h: document.getElementById('canvas').height }));
  check(resized.w === 1600 && resized.h === 1000, `a resized page resizes the canvas (${resized.w}x${resized.h})`);

  // the glass example: the blur and refraction passes draw, and a swipe of a finger scrolls the page under the glass
  if (fs.existsSync(path.join(dir, 'raven_glass.html'))) {
    const glass = await context.newPage();
    const glassLogs = [];
    glass.on('console', (m) => glassLogs.push(m.text()));
    glass.on('pageerror', (e) => glassLogs.push('pageerror: ' + e.message));
    await glass.setViewportSize({ width: 900, height: 600 });
    await glass.goto(`${base}/raven_glass.html`);
    await glass.waitForTimeout(3500);
    const first = await glass.screenshot();
    // a flat page compresses to a few KB; the colored cards, text and glass do not
    check(first.length > 30000, `glass example draws its colored cards and glass (screenshot of ${first.length} bytes)`);
    const cdp = await context.newCDPSession(glass);
    const touch = (type, y) => cdp.send('Input.dispatchTouchEvent', { type, touchPoints: type === 'touchEnd' ? [] : [{ x: 560, y, id: 1 }] });
    await touch('touchStart', 420);
    for (let y = 420; y >= 200; y -= 20) { await touch('touchMove', y); await glass.waitForTimeout(16); }
    await touch('touchEnd', 200);
    await glass.waitForTimeout(600);
    const scrolled = await glass.screenshot();
    check(Buffer.compare(first, scrolled) !== 0, 'a swipe of a finger scrolls the page');
    const glassErrors = glassLogs.filter((l) => /error|abort|pageerror|exception/i.test(l) && !/GPU stall|swiftshader/i.test(l));
    check(glassErrors.length === 0, 'no errors in the console (glass)' + (glassErrors.length ? ': ' + glassErrors.slice(0, 3).join(' | ') : ''));
    fs.writeFileSync('/tmp/raven_web_glass.png', scrolled);
  }

  // every other example that was built: it starts, draws something and reports nothing
  for (const name of ['editor', 'animations', 'components', 'studio', 'themes', 'effects']) {
    if (!fs.existsSync(path.join(dir, `raven_${name}.html`))) continue;
    const other = await context.newPage();
    const otherLogs = [];
    other.on('console', (m) => otherLogs.push(m.text()));
    other.on('pageerror', (e) => otherLogs.push('pageerror: ' + e.message));
    await other.setViewportSize({ width: 1100, height: 700 });
    await other.goto(`${base}/raven_${name}.html`);
    await other.waitForTimeout(3500);
    const picture = await other.screenshot();
    const bad = otherLogs.filter((l) => /error|abort|pageerror|exception/i.test(l) && !/GPU stall|swiftshader/i.test(l));
    check(picture.length > 20000 && bad.length === 0, `${name} example starts and draws (${picture.length} bytes)` + (bad.length ? ': ' + bad.slice(0, 2).join(' | ') : ''));
    await other.close();
  }

  const errors = logs.filter((l) => /error|abort|pageerror|exception/i.test(l) && !/GPU stall|swiftshader/i.test(l));
  check(errors.length === 0, 'no errors in the console' + (errors.length ? ': ' + errors.slice(0, 3).join(' | ') : ''));
  if (logs.length) console.log('console:\n  ' + logs.slice(0, 12).join('\n  '));
  await browser.close();
  server.close();
  process.exit(failures ? 1 : 0);
})().catch((e) => { console.error(e); process.exit(2); });

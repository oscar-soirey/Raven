// Preserve startup failures (especially unavailable WebGL) in CI logs.
export function browserDiagnostics(page) {
  const events=[];
  const record=(kind,text)=>{
    events.push({kind,text});
    if(events.length>50) events.shift();
  };
  page.on('console',message=>record(message.type(),message.text()));
  page.on('pageerror',error=>record('pageerror',error.stack || error.message));
  page.on('requestfailed',request=>record('requestfailed',
    `${request.url()}: ${request.failure()?.errorText}`));
  page.on('response',response=>{
    if(response.status()>=400) record('http',`${response.status()} ${response.url()}`);
  });
  return async()=>{
    let state;
    try {
      state=await page.evaluate(()=>{
        const canvas=document.createElement('canvas');
        const failures=[];
        canvas.addEventListener('webglcontextcreationerror',event=>failures.push(event.statusMessage));
        const gl=canvas.getContext('webgl2');
        return {url:location.href,status:document.querySelector('#status')?.textContent,
          textboxes:document.querySelectorAll('[role=textbox]').length,
          runtimeStarted:typeof Module!=='undefined' && !!Module.ravenLifecycle,
          webgl2:!!gl,renderer:gl?.getParameter(gl.RENDERER),failures};
      });
    } catch(error) { state={unavailable:error.message}; }
    console.error('Browser failure diagnostics:',JSON.stringify({
      engine:process.env.RAVEN_BROWSER_ENGINE || 'chromium',
      display:process.env.DISPLAY || '(unset)',state,events},null,2));
  };
}

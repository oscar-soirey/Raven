(function(){
  var side = document.getElementById('side');
  var btn = document.getElementById('menu');
  function set(open){ side.classList.toggle('open', open); btn.setAttribute('aria-expanded', open ? 'true' : 'false'); }
  btn.addEventListener('click', function(){ set(!side.classList.contains('open')); });
  document.addEventListener('keydown', function(e){ if (e.key === 'Escape') set(false); });
  side.addEventListener('click', function(e){ if (e.target.closest('a')) set(false); });

  var links = [].slice.call(side.querySelectorAll('.sections a[href^="#"]'));
  var targets = links.map(function(a){ return document.getElementById(decodeURIComponent(a.getAttribute('href').slice(1))); });
  var current = -1, ticking = false;
  function spy(){
    ticking = false;
    var idx = -1;
    for (var i = 0; i < targets.length; i++) {
      if (targets[i] && targets[i].getBoundingClientRect().top <= 120) idx = i; else if (targets[i]) break;
    }
    if (idx === current) return;
    if (current >= 0) links[current].classList.remove('on');
    current = idx;
    if (idx >= 0) { links[idx].classList.add('on'); links[idx].scrollIntoView({ block: 'nearest' }); }
  }
  window.addEventListener('scroll', function(){ if (!ticking) { ticking = true; requestAnimationFrame(spy); } }, { passive: true });
  spy();
})();

(function(){
  function copy(text, btn){
    function done(){ btn.textContent = 'Copied!'; setTimeout(function(){ btn.textContent = 'Copy'; }, 1500); }
    function fallback(){
      var t = document.createElement('textarea'); t.value = text; t.style.cssText = 'position:fixed;opacity:0';
      document.body.appendChild(t); t.select();
      try { document.execCommand('copy'); done(); } catch (e) { btn.textContent = 'Ctrl+C'; }
      document.body.removeChild(t);
    }
    if (navigator.clipboard && window.isSecureContext) navigator.clipboard.writeText(text).then(done, fallback); else fallback();
  }
  [].forEach.call(document.querySelectorAll('.doc pre:not(.sig)'), function(pre){
    if (pre.parentNode.classList.contains('codebox')) return;
    var box = document.createElement('div'), b = document.createElement('button');
    box.className = 'codebox';
    b.type = 'button'; b.className = 'copy'; b.textContent = 'Copy'; b.setAttribute('aria-label', 'Copy the code');
    b.addEventListener('click', function(){ copy(pre.textContent, b); });
    pre.parentNode.insertBefore(box, pre); box.appendChild(pre); box.appendChild(b);
  });
})();

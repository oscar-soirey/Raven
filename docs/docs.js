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

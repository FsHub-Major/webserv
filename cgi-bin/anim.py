#!/usr/bin/env python3
"""HTML page that pulls frames from /cgi-bin/anim_frame.py and animates a circle on a canvas.
This demonstrates CGI-generated dynamic frames used to animate in the browser.
"""
print("Content-Type: text/html")
print()
print("""<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <title>CGI Animation Demo</title>
  <style>
    body { font-family: Arial, sans-serif; background: #0f172a; color: #e5e7eb; }
    .container { margin: 20px; }
    canvas { background: #0b1220; border: 1px solid #222; display:block; }
    .info { margin-top: 10px; color: #9ca3af; }
    .controls { margin-top: 8px; }
    button { padding: 6px 10px; }
  </style>
</head>
<body>
  <div class="container">
    <h1>CGI Animation Demo</h1>
    <p class="info">This page fetches a JSON "frame" from <code>/cgi-bin/anim_frame.py</code> every 150ms and animates a circle.</p>
    <canvas id="c" width="600" height="200"></canvas>
    <div class="controls">
      <button id="start">Start</button>
      <button id="stop">Stop</button>
    </div>
    <div class="info">Frame: <span id="frame">-</span> Timestamp: <span id="ts">-</span></div>
  </div>

<script>
(function(){
  var canvas = document.getElementById('c');
  var ctx = canvas.getContext('2d');
  var running = false;
  var timer = null;

  function drawFrame(data) {
    ctx.clearRect(0,0,canvas.width,canvas.height);
    // draw a trail background
    ctx.fillStyle = 'rgba(255,255,255,0.02)';
    ctx.fillRect(0,0,canvas.width,canvas.height);

    // circle
    ctx.beginPath();
    ctx.arc(data.x, data.y, 20, 0, Math.PI*2);
    ctx.fillStyle = data.color;
    ctx.fill();

    document.getElementById('frame').textContent = data.frame;
    document.getElementById('ts').textContent = new Date(data.ts*1000).toLocaleTimeString();
  }

  function fetchAndDraw(){
    fetch('/cgi-bin/anim_frame.py', {cache: 'no-store'})
      .then(function(r){ return r.json(); })
      .then(function(data){ drawFrame(data); })
      .catch(function(e){ console.error('frame fetch error', e); });
  }

  document.getElementById('start').addEventListener('click', function(){
    if (running) return; running = true; fetchAndDraw(); timer = setInterval(fetchAndDraw, 150);
  });
  document.getElementById('stop').addEventListener('click', function(){ if (timer) clearInterval(timer); running = false; });

  // auto-start
  document.getElementById('start').click();
})();
</script>

</body>
</html>""")

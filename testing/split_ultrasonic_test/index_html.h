const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <meta name="theme-color" content="#0d0e10">
  <title>ParkAssist</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&family=JetBrains+Mono:wght@400;500;700&display=swap');
    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
    :root {
      --bg:      #0d0e10;
      --surface: #13141a;
      --border:  #252730;
      --text:    #e2e4ea;
      --muted:   #5a5d6e;
      --safe:    #22c55e;
      --warn:    #f59e0b;
      --danger:  #ef4444;
      --accent:  #6366f1;
      --cam:     #0ea5e9;
    }
    html, body {
      height: 100%; background: var(--bg); color: var(--text);
      font-family: 'Inter', system-ui, sans-serif;
      overflow: hidden; -webkit-tap-highlight-color: transparent;
    }
    @media (orientation: portrait) { #app { display: none !important; } #rot { display: flex !important; } }
    #rot {
      display: none; position: fixed; inset: 0;
      flex-direction: column; align-items: center; justify-content: center;
      gap: 12px; background: var(--bg); color: var(--muted); font-size: .9rem;
    }
    #app {
      display: grid; width: 100vw; height: 100vh;
      grid-template-columns: 1fr 390px;
      grid-template-rows: 50px 1fr 40px;
    }

    /* Header */
    header {
      grid-column: 1/-1; display: flex; align-items: center;
      gap: 12px; padding: 0 18px;
      background: var(--surface); border-bottom: 1px solid var(--border);
    }
    .logo { font-weight: 700; font-size: 1rem; }
    .logo em { color: var(--accent); font-style: normal; }
    .badge {
      font-size: 9px; font-weight: 700; letter-spacing: .08em; padding: 2px 7px;
      border-radius: 4px; background: rgba(99,102,241,.15); color: var(--accent);
    }
    .pill {
      display: inline-flex; align-items: center; gap: 5px;
      padding: 3px 9px; border-radius: 999px;
      font-size: 11px; color: var(--muted); background: var(--bg);
    }
    .pill .dot { width: 6px; height: 6px; border-radius: 50%; background: var(--muted); }
    .pill.on  .dot { background: var(--safe); }
    .pill.off .dot { background: var(--danger); }
    .pill.spin .dot { background: var(--warn); animation: blink .7s ease-in-out infinite; }
    @keyframes blink { 50%{ opacity:.2; } }
    .hdr-r { margin-left: auto; display: flex; gap: 12px; font-size: 11px; color: var(--muted); align-items: center; }

    /* Camera panel */
    .cam-panel {
      position: relative; background: #000;
      display: flex; align-items: center; justify-content: center; overflow: hidden;
    }
    #cam-img { width: 100%; height: 100%; object-fit: contain; display: none; }
    .cam-ph {
      position: absolute; inset: 0;
      display: flex; flex-direction: column; align-items: center; justify-content: center;
      gap: 10px; color: var(--muted); font-size: 12px; text-align: center; padding: 20px;
    }
    .cam-ph svg { opacity: .3; }
    .cam-ph.hide { display: none; }
    .live-badge {
      position: absolute; top: 10px; left: 50%; transform: translateX(-50%);
      font-size: 10px; font-weight: 700; letter-spacing: .1em;
      padding: 3px 10px; border-radius: 4px;
      background: rgba(0,0,0,.65); color: var(--muted);
    }
    .live-badge.on { color: var(--safe); }

    /* Sidebar */
    .sidebar {
      display: flex; flex-direction: column;
      background: var(--surface); border-left: 1px solid var(--border); overflow-y: auto;
    }
    .block { padding: 13px 16px; border-bottom: 1px solid var(--border); }
    .block-title { font-size: 9px; font-weight: 700; letter-spacing: .1em; text-transform: uppercase; color: var(--muted); margin-bottom: 11px; }

    /* Distance bars */
    .dist-row { display: flex; align-items: center; gap: 8px; margin-bottom: 7px; }
    .dist-row:last-child { margin-bottom: 0; }
    .dist-label { width: 40px; font-size: 10px; color: var(--muted); }
    .bar-track { flex: 1; height: 7px; background: var(--bg); border-radius: 4px; overflow: hidden; }
    .bar-fill { height: 100%; border-radius: 4px; transition: width .1s, background .15s; }
    .dist-val { width: 58px; text-align: right; font-size: 12px; font-weight: 600; font-family: 'JetBrains Mono', monospace; transition: color .15s; }

    /* Angle indicator */
    .angle-row { display: flex; align-items: center; gap: 8px; margin-top: 10px; padding-top: 10px; border-top: 1px dashed var(--border); }
    .angle-track { flex: 1; height: 10px; background: var(--bg); border-radius: 4px; position: relative; }
    .angle-track::before { content:''; position:absolute; left:50%; top:0; bottom:0; width:2px; background:var(--border); transform:translateX(-50%); border-radius:1px; }
    .angle-needle { position:absolute; top:1px; bottom:1px; width:8px; border-radius:2px; left:50%; transform:translate(-50%,0); transition:left .1s, background .15s; background:var(--accent); }
    .angle-val { width: 52px; text-align: right; font-size: 12px; font-family: 'JetBrains Mono', monospace; transition: color .15s; }

    /* Closest strip */
    .closest-strip {
      display: flex; align-items: baseline; gap: 8px;
      padding: 10px 16px; background: var(--bg); border-top: 1px solid var(--border);
    }
    .cl-label { font-size: 10px; color: var(--muted); }
    .cl-val { font-size: 30px; font-weight: 700; font-family: 'JetBrains Mono', monospace; transition: color .15s; }
    .cl-unit { font-size: 11px; color: var(--muted); }
    .threat { margin-left: auto; font-size: 11px; font-weight: 700; letter-spacing: .05em; }

    /* Top-view map */
    .map-wrap { flex: 1; display: flex; align-items: center; justify-content: center; background: var(--bg); border-radius: 8px; margin-top: 8px; padding: 6px; }
    #topview { display: block; width: 100%; max-width: 360px; height: auto; border-radius: 6px; }

    /* Log */
    .log-wrap { flex: 1; min-height: 80px; display: flex; flex-direction: column; padding: 11px 16px; }
    #event-log { flex: 1; overflow-y: auto; font-size: 10px; display: flex; flex-direction: column; gap: 3px; }
    .log-line { display: flex; gap: 7px; align-items: baseline; color: var(--muted); }
    .log-line .t { flex-shrink: 0; font-family: 'JetBrains Mono', monospace; opacity: .5; }
    .log-line .tag { flex-shrink: 0; padding: 1px 5px; border-radius: 3px; font-size: 9px; font-weight: 700; text-transform: uppercase; }
    .tag.sys   { background: rgba(90,93,110,.18);  color: var(--muted); }
    .tag.alert { background: rgba(239,68,68,.14);  color: var(--danger); }
    .tag.cam   { background: rgba(14,165,233,.12); color: var(--cam); }

    /* Footer */
    footer {
      grid-column: 1/-1; display: flex; align-items: center;
      padding: 0 16px; gap: 14px; font-size: 10px; color: var(--muted);
      background: var(--surface); border-top: 1px solid var(--border);
    }
    footer .right { margin-left: auto; font-family: 'JetBrains Mono', monospace; }
  </style>
</head>
<body>

<div id="rot">
  <svg width="44" height="44" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
    <rect x="5" y="2" width="14" height="20" rx="2"/><circle cx="12" cy="17" r="1"/>
  </svg>
  Rotate to landscape
</div>

<div id="app">
  <header>
    <span class="logo">Park<em>Assist</em></span>
    <span class="badge">ULTRASONIC TEST</span>
    <span class="pill" id="pill-sensor"><span class="dot"></span>Sensors</span>
    <span class="pill" id="pill-cam"><span class="dot"></span>Camera</span>
    <span class="hdr-r">
      <span id="fps">-- fps</span>
      <span id="clock">--:--:--</span>
    </span>
  </header>

  <!-- Camera -->
  <div class="cam-panel">
    <div class="cam-ph" id="cam-ph">
      <svg width="52" height="52" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.2">
        <rect x="2" y="7" width="15" height="11" rx="2"/>
        <polygon points="17 9 22 6 22 18 17 15"/>
        <circle cx="9" cy="12" r="2.5"/>
      </svg>
      Connecting to ESP32-CAM<br>
      <small style="opacity:.5">192.168.4.2 / stream</small>
    </div>
    <img id="cam-img" alt="Camera">
    <span class="live-badge" id="live-badge">● LIVE</span>
  </div>

  <!-- Sidebar -->
  <aside class="sidebar">

    <!-- Sensor bars -->
    <div class="block">
      <div class="block-title">Sensors</div>
      <div class="dist-row">
        <span class="dist-label">Left</span>
        <div class="bar-track"><div class="bar-fill" id="bar-L" style="width:0%;background:var(--muted)"></div></div>
        <span class="dist-val" id="val-L" style="color:var(--muted)">--</span>
      </div>
      <div class="dist-row">
        <span class="dist-label">Right</span>
        <div class="bar-track"><div class="bar-fill" id="bar-R" style="width:0%;background:var(--muted)"></div></div>
        <span class="dist-val" id="val-R" style="color:var(--muted)">--</span>
      </div>
      <div class="dist-row">
        <span class="dist-label">Fused</span>
        <div class="bar-track"><div class="bar-fill" id="bar-dist" style="width:0%;background:var(--muted)"></div></div>
        <span class="dist-val" id="val-dist" style="color:var(--muted)">--</span>
      </div>
      <div class="angle-row">
        <span class="dist-label">Angle</span>
        <div class="angle-track"><div class="angle-needle" id="needle"></div></div>
        <span class="angle-val" id="val-angle" style="color:var(--muted)">0°</span>
      </div>
    </div>

    <!-- Top-view map -->
    <div class="block" style="flex:1;display:flex;flex-direction:column;">
      <div class="block-title">Proximity Map</div>
      <div class="map-wrap" style="flex:1;">
        <canvas id="topview" width="360" height="280"></canvas>
      </div>
    </div>

    <!-- Log -->
    <div class="log-wrap">
      <div class="block-title">Events</div>
      <div id="event-log"></div>
    </div>

    <!-- Closest strip -->
    <div class="closest-strip">
      <span class="cl-label">Closest</span>
      <span class="cl-val" id="cl-val" style="color:var(--muted)">--</span>
      <span class="cl-unit">cm</span>
      <span class="threat" id="threat" style="color:var(--muted)">--</span>
    </div>

  </aside>

  <footer>
    <span>Sensor WS: <span id="ws-status">--</span></span>
    <span>Camera: <span id="cam-status">--</span></span>
    <span class="right">192.168.4.1 | cam: 192.168.4.2</span>
  </footer>
</div>

<script>
(function () {
  'use strict';
  const MAX_CM  = 250, WARN_CM = 80, STOP_CM = 30;

  const SENSOR_IP  = window.location.hostname || '192.168.4.1';
  const WS_URL     = 'ws://' + SENSOR_IP + ':81';
  const STREAM_URL = 'http://192.168.4.2/stream';

  // ── Audio beeper ─────────────────────────────────────────────────────────────
  const AC = new (window.AudioContext || window.webkitAudioContext)();
  let lastBeep = 0;
  document.addEventListener('click', () => { if (AC.state === 'suspended') AC.resume(); }, { once: true });
  function beep(dist) {
    if (dist > WARN_CM || AC.state === 'suspended') return;
    const now = Date.now(), gap = Math.max(80, (dist / WARN_CM) * 900);
    if (now - lastBeep < gap) return;
    lastBeep = now;
    const o = AC.createOscillator(), g = AC.createGain();
    o.type = 'sine';
    o.frequency.setValueAtTime(900 - dist * 5, AC.currentTime);
    g.gain.setValueAtTime(0.07, AC.currentTime);
    g.gain.exponentialRampToValueAtTime(0.001, AC.currentTime + 0.08);
    o.connect(g); g.connect(AC.destination);
    o.start(); o.stop(AC.currentTime + 0.08);
  }

  // ── Helpers ───────────────────────────────────────────────────────────────────
  function log(tag, msg) {
    const el = document.getElementById('event-log');
    const line = document.createElement('div');
    line.className = 'log-line';
    const t = new Date().toTimeString().slice(0, 8);
    line.innerHTML = `<span class="t">${t}</span><span class="tag ${tag}">${tag}</span><span>${msg}</span>`;
    el.insertBefore(line, el.firstChild);
    while (el.children.length > 60) el.removeChild(el.lastChild);
  }
  function pill(id, s) {
    document.getElementById(id).className =
      'pill ' + (s === true ? 'on' : s === false ? 'off' : 'spin');
  }
  function col(d) {
    if (d <= STOP_CM) return '#ef4444';
    if (d <= WARN_CM) return '#f59e0b';
    return '#22c55e';
  }
  function setBar(barId, valId, cm) {
    const bar = document.getElementById(barId), val = document.getElementById(valId);
    if (!cm || cm <= 0) {
      bar.style.width = '0%'; bar.style.background = 'var(--muted)';
      val.textContent = '--'; val.style.color = 'var(--muted)'; return;
    }
    bar.style.width = Math.min(100, (cm / MAX_CM) * 100) + '%';
    bar.style.background = col(cm);
    val.textContent = Math.round(cm) + ' cm';
    val.style.color = col(cm);
  }

  // ── Top-view map ──────────────────────────────────────────────────────────────
  let lastSensor = { valid: false, dist: 0, angle: 0, dL: 0, dR: 0 };

  function drawMap() {
    const c = document.getElementById('topview');
    const ctx = c.getContext('2d');
    const W = c.width, H = c.height, cx = W / 2, baseY = H - 36, maxR = baseY - 16;

    ctx.fillStyle = '#0d0e10'; ctx.fillRect(0, 0, W, H);

    // Range rings
    ctx.setLineDash([3, 6]); ctx.lineWidth = 1;
    [60, 120, 180, 240].forEach(cm => {
      const r = (Math.min(cm, MAX_CM) / MAX_CM) * maxR;
      ctx.beginPath(); ctx.arc(cx, baseY, r, Math.PI, 0);
      ctx.strokeStyle = '#1e2233'; ctx.stroke();
      ctx.fillStyle = '#343660'; ctx.font = '8px Inter,sans-serif'; ctx.textAlign = 'left';
      ctx.fillText(cm + 'cm', cx + r + 3, baseY - 2);
    });
    ctx.setLineDash([]);

    // Centre line
    ctx.beginPath(); ctx.moveTo(cx, 0); ctx.lineTo(cx, baseY);
    ctx.strokeStyle = '#1a1c2a'; ctx.lineWidth = 1; ctx.stroke();

    // Individual sensor dots at their physical offsets
    [{ d: lastSensor.dL, ang: -10 }, { d: lastSensor.dR, ang: 10 }].forEach(({ d, ang }) => {
      if (!d || d <= 0 || d > MAX_CM) return;
      const rad = ang * Math.PI / 180;
      const r   = (d / MAX_CM) * maxR;
      const ox  = cx + Math.sin(rad) * r;
      const oy  = baseY - Math.cos(rad) * r;
      ctx.beginPath(); ctx.arc(ox, oy, 5, 0, Math.PI * 2);
      ctx.fillStyle = col(d); ctx.fill();
      ctx.fillStyle = 'rgba(255,255,255,.55)';
      ctx.font = '8px JetBrains Mono,monospace';
      ctx.textAlign = ang < 0 ? 'right' : 'left';
      ctx.fillText(Math.round(d), ox + (ang < 0 ? -8 : 8), oy + 3);
    });

    // Fused obstacle + FOV cone
    if (lastSensor.valid && lastSensor.dist > 0) {
      const dist  = Math.min(lastSensor.dist, MAX_CM);
      const rad   = lastSensor.angle * Math.PI / 180;
      const r     = (dist / MAX_CM) * maxR;
      const span  = 30 * Math.PI / 180;
      const rgb   = dist <= STOP_CM ? '239,68,68' : dist <= WARN_CM ? '245,158,11' : '34,197,94';

      // Cone fill
      const grd = ctx.createRadialGradient(cx, baseY, 0, cx, baseY - r * .5, r);
      grd.addColorStop(0, `rgba(${rgb},.04)`);
      grd.addColorStop(1, `rgba(${rgb},.18)`);
      ctx.beginPath();
      ctx.moveTo(cx, baseY);
      ctx.arc(cx, baseY, r, Math.PI / 2 + rad - span, Math.PI / 2 + rad + span);
      ctx.closePath();
      ctx.fillStyle = grd; ctx.fill();
      ctx.strokeStyle = `rgba(${rgb},.4)`; ctx.lineWidth = 1.2; ctx.stroke();

      // Fused dot
      const ox = cx + Math.sin(rad) * r;
      const oy = baseY - Math.cos(rad) * r;
      ctx.beginPath(); ctx.arc(ox, oy, 8, 0, Math.PI * 2);
      ctx.fillStyle = `rgb(${rgb})`; ctx.fill();
      ctx.fillStyle = '#fff';
      ctx.font = 'bold 9px JetBrains Mono,monospace'; ctx.textAlign = 'center';
      ctx.fillText(Math.round(dist), ox, oy + 3);
    }

    // Car
    ctx.beginPath();
    if (ctx.roundRect) ctx.roundRect(cx - 22, baseY - 11, 44, 22, 5);
    else ctx.rect(cx - 22, baseY - 11, 44, 22);
    ctx.fillStyle = '#6366f1'; ctx.fill();
    ctx.fillStyle = '#fff'; ctx.font = 'bold 9px Inter,sans-serif'; ctx.textAlign = 'center';
    ctx.fillText('YOU', cx, baseY + 4);
    ctx.textAlign = 'left';
  }

  // ── Sensor data handler ───────────────────────────────────────────────────────
  function onData(data) {
    const dL    = (data.dL   > 0 && data.dL   <= MAX_CM) ? data.dL   : null;
    const dR    = (data.dR   > 0 && data.dR   <= MAX_CM) ? data.dR   : null;
    const dist  = (data.dist > 0 && data.dist <= MAX_CM) ? data.dist : null;
    const angle = data.angle || 0;
    const valid = !!data.valid;

    setBar('bar-L', 'val-L', dL);
    setBar('bar-R', 'val-R', dR);
    setBar('bar-dist', 'val-dist', dist);

    const needle  = document.getElementById('needle');
    const angEl   = document.getElementById('val-angle');
    const needleC = Math.abs(angle) > 20 ? '#ef4444' : Math.abs(angle) > 8 ? '#f59e0b' : '#6366f1';
    needle.style.left       = Math.max(4, Math.min(94, 50 + (angle / 45) * 46)) + '%';
    needle.style.background = needleC;
    angEl.textContent       = (angle >= 0 ? '+' : '') + angle.toFixed(1) + '°';
    angEl.style.color       = needleC;

    const clVal  = document.getElementById('cl-val');
    const threat = document.getElementById('threat');
    if (!valid || dist == null) {
      clVal.textContent = '--'; clVal.style.color = 'var(--muted)';
      threat.textContent = '--'; threat.style.color = 'var(--muted)';
    } else {
      const d = Math.round(dist);
      clVal.textContent = d; clVal.style.color = col(d);
      if (d <= STOP_CM) {
        threat.textContent = 'STOP!'; threat.style.color = '#ef4444';
        if (!window._stopLogged) { log('alert', 'STOP — ' + d + ' cm'); window._stopLogged = true; }
      } else {
        window._stopLogged = false;
        threat.textContent = d <= WARN_CM ? 'CAUTION' : 'CLEAR';
        threat.style.color = d <= WARN_CM ? '#f59e0b' : '#22c55e';
      }
      beep(d);
    }

    lastSensor = { valid, dist: dist || 0, angle, dL: dL || 0, dR: dR || 0 };
    drawMap();
  }

  // ── WebSocket — auto-connect + auto-retry ────────────────────────────────────
  function connectSensor() {
    pill('pill-sensor', 'spin');
    document.getElementById('ws-status').textContent = 'Connecting…';
    const ws = new WebSocket(WS_URL);
    ws.onopen    = () => { pill('pill-sensor', true); document.getElementById('ws-status').textContent = 'OK'; log('sys', 'Sensors connected'); };
    ws.onmessage = (ev) => { try { onData(JSON.parse(ev.data)); } catch(_){} };
    ws.onclose   = () => { pill('pill-sensor', false); document.getElementById('ws-status').textContent = 'Retry…'; log('alert', 'Sensor disconnected'); setTimeout(connectSensor, 3000); };
    ws.onerror   = () => pill('pill-sensor', false);
  }

  // ── Camera — auto-connect + auto-retry ───────────────────────────────────────
  let fpsCount = 0, fpsTime = Date.now();
  function connectCam() {
    const img = document.getElementById('cam-img'), ph = document.getElementById('cam-ph'), badge = document.getElementById('live-badge');
    pill('pill-cam', 'spin');
    document.getElementById('cam-status').textContent = 'Connecting…';
    img.onload  = () => {
      img.style.display = 'block'; ph.classList.add('hide'); badge.classList.add('on');
      pill('pill-cam', true); document.getElementById('cam-status').textContent = 'OK';
      fpsCount++;
      const now = Date.now();
      if (now - fpsTime >= 1000) { document.getElementById('fps').textContent = fpsCount + ' fps'; fpsCount = 0; fpsTime = now; }
    };
    img.onerror = () => {
      img.style.display = 'none'; ph.classList.remove('hide'); badge.classList.remove('on');
      pill('pill-cam', false); document.getElementById('cam-status').textContent = 'Unavailable';
      setTimeout(connectCam, 4000);
    };
    img.src = STREAM_URL + '?t=' + Date.now();
  }

  setInterval(() => { document.getElementById('clock').textContent = new Date().toTimeString().slice(0, 8); }, 1000);
  drawMap();
  connectSensor();
  connectCam();
  log('sys', 'ParkAssist Ultrasonic Test booted');
})();
</script>
</body>
</html>
)rawliteral";

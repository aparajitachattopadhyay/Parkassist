/**
 * ParkAssist – script.js  (fresh rewrite)
 * ─────────────────────────────────────────────────────────────────
 *  DEMO MODE  → When sensor ESP32 WebSocket (ws://192.168.4.1:81)
 *               and/or camera MJPEG (http://192.168.4.2/stream)
 *               cannot be reached, the UI shows:
 *                 • vdo.mp4 looped in the camera panel
 *                 • simulated (animated) sensor readings
 *
 *  LIVE MODE  → When connected it shows real MJPEG + real WebSocket
 *               sensor data. Demo video & simulated data are hidden.
 *
 *  Transition is automatic: reconnects every few seconds.
 * ─────────────────────────────────────────────────────────────────
 */

(function () {
  'use strict';

  /* ── Constants ───────────────────────────────────────────────── */
  const MAX_CM   = 250;
  const STOP_CM  = 30;
  const WARN_CM  = 80;

  /* ESP32 addresses (split architecture) */
  const SENSOR_HOST = '192.168.4.1';
  const CAM_HOST    = '192.168.4.2';

  /* URLs (Default to ESP32) */
  let WS_URL     = 'ws://'   + SENSOR_HOST + ':81';
  let STREAM_URL = 'http://' + CAM_HOST    + '/stream';
  let CONFIG_URL = 'http://' + SENSOR_HOST + '/config';

  const HOST = window.location.hostname;
  const isLocal = (HOST === 'localhost' || HOST === '127.0.0.1' || window.location.protocol === 'file:');

  // If hitting the page from localhost/127.0.0.1 or file://, assume test_stream.py backend
  if (isLocal) {
    STREAM_URL = 'http://127.0.0.1:5000/stream';
    CONFIG_URL = 'http://127.0.0.1:5000/config';
  }

  /* ── Application State ───────────────────────────────────────── */
  const state = {
    mode            : 'demo',   // 'demo' | 'live'
    wsConnected     : false,
    camConnected    : false,
    aiConnected     : false,
    lastDetections  : [],
    fpsCount        : 0,
    fpsTime         : Date.now(),
    demoTimer       : null,
    demoPhase       : 0,        // 0-3 scenario index
    demoTick        : 0,
    wsRetryTimer    : null,
    camRetryTimer   : null,
    aiRetryTimer    : null,
    ws              : null,
    camImg          : null,
  };

  /* ── Demo sensor scenarios (loop through them) ───────────────── */
  const demoScenarios = [
    // scenario 0 – object approaching from left
    {
      usL: 45, usR: 130, label: 'Object left',
      detections: [
        { label: 'car',    confidence: 0.88, x: 0.08, y: 0.32, w: 0.18, h: 0.22 },
        { label: 'person', confidence: 0.75, x: 0.28, y: 0.28, w: 0.07, h: 0.20 },
      ],
    },
    // scenario 1 – object dead-ahead close
    {
      usL: 28, usR: 30, label: 'STOP zone',
      detections: [
        { label: 'car',  confidence: 0.94, x: 0.30, y: 0.25, w: 0.40, h: 0.35 },
        { label: 'bus',  confidence: 0.81, x: 0.10, y: 0.20, w: 0.20, h: 0.30 },
      ],
    },
    // scenario 2 – object right
    {
      usL: 200, usR: 48, label: 'Object right',
      detections: [
        { label: 'motorcycle', confidence: 0.82, x: 0.65, y: 0.35, w: 0.12, h: 0.18 },
        { label: 'car',       confidence: 0.91, x: 0.74, y: 0.30, w: 0.20, h: 0.25 },
      ],
    },
    // scenario 3 – clear
    {
      usL: 220, usR: 210, label: 'Clear',
      detections: [],
    },
  ];

  /* ── Audio ───────────────────────────────────────────────────── */
  const audioCtx = new (window.AudioContext || window.webkitAudioContext)();
  let lastBeep = 0;

  function playBeep(dist) {
    if (dist > WARN_CM) return;
    if (audioCtx.state === 'suspended') return;
    const now = Date.now();
    const interval = Math.max(80, (dist / WARN_CM) * 900);
    if (now - lastBeep < interval) return;
    lastBeep = now;

    const osc  = audioCtx.createOscillator();
    const gain = audioCtx.createGain();
    osc.type = 'sine';
    osc.frequency.setValueAtTime(900 - dist * 4, audioCtx.currentTime);
    gain.gain.setValueAtTime(0.09, audioCtx.currentTime);
    gain.gain.exponentialRampToValueAtTime(0.001, audioCtx.currentTime + 0.08);
    osc.connect(gain);
    gain.connect(audioCtx.destination);
    osc.start();
    osc.stop(audioCtx.currentTime + 0.08);
  }

  document.addEventListener('click', () => {
    if (audioCtx.state === 'suspended') audioCtx.resume();
  }, { once: true });

  /* ── DOM Helpers ─────────────────────────────────────────────── */
  const $ = id => document.getElementById(id);

  function setPill(id, status) {
    // status: true | false | 'connecting'
    $('pill-' + id).className = 'pill ' +
      (status === true ? 'on' : status === false ? 'off' : 'connecting');
  }

  function log(tag, msg) {
    const el = $('event-log');
    const line = document.createElement('div');
    line.className = 'log-line';
    const t = new Date().toTimeString().slice(3, 8);
    line.innerHTML =
      `<span class="t">${t}</span>` +
      `<span class="tag ${tag}">${tag}</span>` +
      `<span>${msg}</span>`;
    el.insertBefore(line, el.firstChild);
    while (el.children.length > 60) el.removeChild(el.lastChild);
  }

  /* ── Mode management ─────────────────────────────────────────── */
  function setMode(mode) {
    if (state.mode === mode) return;
    state.mode = mode;
    const app = $('app');
    app.classList.toggle('demo-mode', mode === 'demo');
    app.classList.toggle('live-mode', mode === 'live');

    const liveB = $('live-badge');
    if (mode === 'live') {
      liveB.textContent = 'LIVE';
      liveB.className = 'live-badge on';
      $('mode-label').textContent = 'LIVE MODE';
      $('mode-label').style.color = 'var(--safe)';
      // hide demo video, show live img
      $('demo-video').style.display = 'none';
      log('sys', 'Switched to LIVE mode');
    } else {
      liveB.textContent = 'DEMO';
      liveB.className = 'live-badge demo';
      $('mode-label').textContent = 'DEMO MODE';
      $('mode-label').style.color = 'var(--warn)';
      // show demo video
      $('demo-video').style.display = '';
      $('demo-video').play().catch(() => {});
      log('demo', 'No hardware – demo mode active');
    }
  }

  function checkOverallMode() {
    const shouldBeLive = state.wsConnected || state.camConnected;
    setMode(shouldBeLive ? 'live' : 'demo');
  }

  /* ── Display helpers ─────────────────────────────────────────── */
  function colorForDist(d) {
    if (d <= STOP_CM) return '#ef4444'; // danger
    if (d <= WARN_CM) return '#f59e0b'; // warn
    return '#22c55e';                   // safe
  }

  function updateSensorBox(key, cmVal) {
    const valEl = $('sv-' + key);
    const barEl = $('sb-' + key);
    if (cmVal == null || cmVal < 0) {
      valEl.textContent = '-- cm';
      valEl.style.color = 'var(--muted)';
      barEl.style.width = '0%';
      barEl.style.background = 'var(--muted)';
      return;
    }
    const clamped = Math.min(cmVal, MAX_CM);
    valEl.textContent = Math.round(cmVal) + ' cm';
    valEl.style.color = colorForDist(cmVal);
    barEl.style.width  = (100 - (clamped / MAX_CM) * 100) + '%';  // fuller = closer
    barEl.style.background = colorForDist(cmVal);
  }

  function updateFusedBar(cmVal) {
    const bar = $('bar-final');
    const val = $('val-final');
    if (cmVal == null || cmVal < 0) {
      val.textContent = '-- cm';
      bar.style.width = '0%';
      bar.style.background = 'var(--muted)';
      return;
    }
    val.textContent = Math.round(cmVal) + ' cm';
    bar.style.width = Math.min(100, (cmVal / MAX_CM) * 100) + '%';
    bar.style.background = colorForDist(cmVal);
  }

  function updateAngle(angleDeg) {
    const needle = $('angle-needle');
    const pct = 50 + (angleDeg / 45) * 48;
    needle.style.left = Math.max(4, Math.min(96, pct)) + '%';
    const col = Math.abs(angleDeg) > 20 ? 'var(--danger)'
               : Math.abs(angleDeg) > 8  ? 'var(--warn)'
               : 'var(--accent)';
    needle.style.background = col;
    $('angle-val').textContent = (angleDeg >= 0 ? '+' : '') + angleDeg.toFixed(1) + '°';
    $('angle-val').style.color = col;
  }

  // Track last proximity zone so we only log on transitions
  let lastZone = null;

  function updateClosest(distCm, isValid) {
    const cv  = $('closest-val');
    const thr = $('threat');
    if (!isValid || distCm == null) {
      cv.textContent = '--'; cv.style.color = 'var(--muted)';
      thr.textContent = '--'; thr.style.color = 'var(--muted)';
      if (lastZone !== null) { lastZone = null; }
      return;
    }
    const d = Math.round(distCm);
    cv.textContent = d; cv.style.color = colorForDist(d);
    let zone;
    if (d <= STOP_CM)      { thr.textContent = 'STOP';    thr.style.color = 'var(--danger)'; zone = 'STOP';    }
    else if (d <= WARN_CM) { thr.textContent = 'CAUTION'; thr.style.color = 'var(--warn)';   zone = 'CAUTION'; }
    else                   { thr.textContent = 'CLEAR';   thr.style.color = 'var(--safe)';   zone = 'CLEAR';   }

    if (zone !== lastZone) {
      lastZone = zone;
      if (zone === 'STOP')    log('alert', '🛑 STOP! Object at ' + d + ' cm');
      else if (zone === 'CAUTION') log('alert', '⚠️ Caution — ' + d + ' cm');
      else                    log('sys',   '✅ Path clear (' + d + ' cm)');
    }
  }

  /* ── Sensor fusion ───────────────────────────────────────────── */
  function fuse(sensors) {
    // sensors: [ { val, ang }, ... ]  (ang in degrees)
    const valid = sensors.filter(s => s.val != null && s.val > 0 && s.val <= MAX_CM);
    if (!valid.length) return { dist: null, angle: 0 };

    let sumW = 0, sumX = 0, sumY = 0;
    valid.forEach(s => {
      const rad = s.ang * Math.PI / 180;
      const x = s.val * Math.sin(rad);
      const y = s.val * Math.cos(rad);
      const w = 1 / (s.val * s.val);
      sumW += w; sumX += x * w; sumY += y * w;
    });
    const ax = sumX / sumW, ay = sumY / sumW;
    return {
      dist  : Math.sqrt(ax * ax + ay * ay),
      angle : Math.atan2(ax, ay) * 180 / Math.PI,
    };
  }

  /* ── Apply a full sensor reading to the UI ───────────────────── */
  function applySensorData(data) {
    // data fields: usL, usR (or legacy dA/dB)
    const usL = data.usL != null ? data.usL : (data.dA != null ? data.dA : null);
    const usR = data.usR != null ? data.usR : (data.dB != null ? data.dB : null);

    updateSensorBox('usL', usL);
    updateSensorBox('usR', usR);

    const sensors = [
      { val: usL, ang: -20 },
      { val: usR, ang:  20 },
    ];
    const { dist, angle } = fuse(sensors);

    // Fallback to pre-fused values from firmware
    const finalDist  = dist  != null ? dist  : (data.dist  != null ? data.dist  : null);
    const finalAngle = angle != null ? angle : (data.angle != null ? data.angle : 0);

    updateFusedBar(finalDist);
    updateAngle(finalAngle);
    updateClosest(finalDist, data.valid !== false && finalDist != null);

    if (finalDist != null && finalDist <= WARN_CM) playBeep(finalDist);

    drawTopView({ finalDist, finalAngle, valid: finalDist != null }, state.lastDetections);
  }

  /* ── TOP-VIEW CANVAS ─────────────────────────────────────────── */
  const shapeColors = {
    person   : '#ef4444',
    car      : '#eab308',
    bus      : '#eab308',
    bicycle  : '#22c55e',
    motorcycle: '#22c55e',
    cat      : '#6366f1',
    dog      : '#6366f1',
  };

  function drawShape(ctx, type, x, y, size, color) {
    ctx.fillStyle = color;
    ctx.beginPath();
    if (type === 'person') {
      ctx.arc(x, y, size, 0, Math.PI * 2);
    } else if (type === 'car' || type === 'bus') {
      const rx = x - size * 1.3, ry = y - size * 0.9;
      const rw = size * 2.6, rh = size * 1.8, r = 3;
      // polyfill for roundRect
      ctx.moveTo(rx + r, ry);
      ctx.lineTo(rx + rw - r, ry);
      ctx.quadraticCurveTo(rx + rw, ry, rx + rw, ry + r);
      ctx.lineTo(rx + rw, ry + rh - r);
      ctx.quadraticCurveTo(rx + rw, ry + rh, rx + rw - r, ry + rh);
      ctx.lineTo(rx + r, ry + rh);
      ctx.quadraticCurveTo(rx, ry + rh, rx, ry + rh - r);
      ctx.lineTo(rx, ry + r);
      ctx.quadraticCurveTo(rx, ry, rx + r, ry);
    } else {
      ctx.moveTo(x, y - size);
      ctx.lineTo(x + size * 0.9, y + size * 0.8);
      ctx.lineTo(x - size * 0.9, y + size * 0.8);
    }
    ctx.fill();
    ctx.fillStyle = '#fff';
    ctx.font = '9px Inter, sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText(type, x, y + size + 12);
    ctx.textAlign = 'left';
  }

  function drawTopView(sensor, detections) {
    const c = $('topview-canvas');
    if (!c) return;
    const ctx = c.getContext('2d');
    const W = c.width, H = c.height;

    // Background
    ctx.fillStyle = '#0d0e10';
    ctx.fillRect(0, 0, W, H);

    // Grid
    ctx.strokeStyle = '#1c1d22';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(W / 2, 0); ctx.lineTo(W / 2, H);
    for (let i = H * .2; i < H; i += H * .2) {
      ctx.moveTo(0, i); ctx.lineTo(W, i);
    }
    ctx.stroke();

    // Range arcs from car
    const cx = W / 2, baseY = H - 30;
    ctx.setLineDash([3, 5]);
    ctx.lineWidth = 1;
    [STOP_CM, WARN_CM, MAX_CM].forEach((r, i) => {
      ctx.strokeStyle = i === 0 ? 'rgba(239,68,68,.35)'
                      : i === 1 ? 'rgba(245,158,11,.25)'
                      :           'rgba(99,102,241,.15)';
      const arcR = (r / MAX_CM) * (baseY - 20);
      ctx.beginPath();
      ctx.arc(cx, baseY, arcR, Math.PI, 0);
      ctx.stroke();
    });
    ctx.setLineDash([]);

    // User car
    ctx.fillStyle = '#3b82f6';
    ctx.beginPath();
    const carX = cx - 22, carY = baseY - 10, carW = 44, carH = 22, carR = 4;
    ctx.moveTo(carX + carR, carY);
    ctx.lineTo(carX + carW - carR, carY);
    ctx.quadraticCurveTo(carX + carW, carY, carX + carW, carY + carR);
    ctx.lineTo(carX + carW, carY + carH - carR);
    ctx.quadraticCurveTo(carX + carW, carY + carH, carX + carW - carR, carY + carH);
    ctx.lineTo(carX + carR, carY + carH);
    ctx.quadraticCurveTo(carX, carY + carH, carX, carY + carH - carR);
    ctx.lineTo(carX, carY + carR);
    ctx.quadraticCurveTo(carX, carY, carX + carR, carY);
    ctx.fill();
    ctx.fillStyle = '#fff';
    ctx.font = 'bold 10px Inter, sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText('YOU', cx, baseY + 4);
    ctx.textAlign = 'left';

    const maxR = baseY - 20;

    function plotPoint(dist, angleDeg) {
      if (dist == null || dist <= 0 || dist > MAX_CM) return;
      const rad = angleDeg * Math.PI / 180;
      const r   = (dist / MAX_CM) * maxR;
      const ox  = cx + Math.sin(rad) * r;
      const oy  = baseY - Math.cos(rad) * r;

      // Glow aura
      const grd = ctx.createRadialGradient(ox, oy, 0, ox, oy, 14);
      grd.addColorStop(0, colorForDist(dist));
      grd.addColorStop(1, 'transparent');
      // Simple solid dot with glow
      const cHex = colorForDist(dist);
      ctx.shadowColor = cHex;
      ctx.shadowBlur = 10;
      ctx.fillStyle  = cHex;
      ctx.beginPath();
      ctx.arc(ox, oy, 6, 0, Math.PI * 2);
      ctx.fill();
      ctx.shadowBlur = 0;

      ctx.fillStyle = 'rgba(255,255,255,.75)';
      ctx.font = '9px JetBrains Mono, monospace';
      ctx.fillText(Math.round(dist) + 'cm', ox + 9, oy + 3);
    }

    if (sensor.valid && sensor.finalDist != null) {
      plotPoint(sensor.finalDist, sensor.finalAngle);
    }

    // AI detections
    if (detections && detections.length) {
      const fallback = sensor.finalDist != null ? sensor.finalDist : 100;
      detections.forEach(d => {
        const objAng = (d.x - 0.5) * 60;
        const rad = objAng * Math.PI / 180;
        const r   = (fallback / MAX_CM) * maxR;
        const ox  = cx + Math.sin(rad) * r;
        const oy  = baseY - Math.cos(rad) * r;
        const col = shapeColors[d.label] || '#6366f1';
        drawShape(ctx, d.label, ox, oy, 11, col);
      });
    }
  }

  /* ── Overlay canvas (bounding boxes on camera / demo video) ─── */
  function getVideoRect() {
    // Returns the rendered pixel rect of the active media element
    // inside the camera-wrap, respecting object-fit.
    const wrap = document.querySelector('.camera-wrap');
    const cW = wrap.offsetWidth, cH = wrap.offsetHeight;

    // In demo mode the <video> uses object-fit:cover → fills entire wrap
    if (state.mode === 'demo') {
      return { left: 0, top: 0, width: cW, height: cH };
    }

    // In live mode the <img> uses object-fit:contain → may have letterboxing
    const img = $('cam-img');
    if (!img || !img.naturalWidth) return { left: 0, top: 0, width: cW, height: cH };
    const scale = Math.min(cW / img.naturalWidth, cH / img.naturalHeight);
    const rW = img.naturalWidth  * scale;
    const rH = img.naturalHeight * scale;
    return {
      left  : (cW - rW) / 2,
      top   : (cH - rH) / 2,
      width : rW,
      height: rH,
    };
  }

  function drawOverlay(detections) {
    const canvas = $('overlay');
    const wrap   = document.querySelector('.camera-wrap');
    canvas.width  = wrap.offsetWidth;
    canvas.height = wrap.offsetHeight;
    const ctx = canvas.getContext('2d');
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    if (!detections || !detections.length) return;

    const rect = getVideoRect();

    detections.forEach(d => {
      // Normalised coords → pixel coords inside the actual media rect
      const x = rect.left + d.x * rect.width;
      const y = rect.top  + d.y * rect.height;
      const w = d.w * rect.width;
      const h = d.h * rect.height;
      const col = shapeColors[d.label] || '#6366f1';

      // Box
      ctx.shadowColor = col; ctx.shadowBlur = 8;
      ctx.strokeStyle = col; ctx.lineWidth = 2;
      ctx.strokeRect(x, y, w, h);
      ctx.shadowBlur  = 0;

      // Label pill
      const label = d.label + ' ' + Math.round((d.confidence || 0) * 100) + '%';
      ctx.font = 'bold 11px Inter, sans-serif';
      const tw = ctx.measureText(label).width;
      ctx.fillStyle = col;
      ctx.fillRect(x - 1, y - 20, tw + 10, 20);
      ctx.fillStyle = '#fff';
      ctx.fillText(label, x + 4, y - 6);
    });
  }

  /* ═════════════════════════════════════════════════════════════
     DEMO MODE  –  simulated sensor walk
     ═════════════════════════════════════════════════════════════ */
  function startDemoLoop() {
    if (state.demoTimer) return;          // already running
    state.demoTick = 0;
    state.demoPhase = 0;

    state.demoTimer = setInterval(() => {
      if (state.mode === 'live') return;  // paused in live mode

      const sc = demoScenarios[state.demoPhase];

      // Add small oscillation to values so they look "live"
      function jitter(v) { return v + (Math.random() - 0.5) * 8; }

      const data = {
        usL    : Math.max(2, jitter(sc.usL)),
        usR    : Math.max(2, jitter(sc.usR)),
        valid  : true,
      };

      applySensorData(data);

      // Draw demo bounding boxes on the video panel
      if (sc.detections && sc.detections.length) {
        state.lastDetections = sc.detections;
        drawOverlay(sc.detections);
        $('obj-count').textContent = sc.detections.length;
      } else {
        state.lastDetections = [];
        drawOverlay([]);
        $('obj-count').textContent = 0;
      }

      // Advance phase every ~5 s (10 ticks × 500 ms)
      state.demoTick++;
      if (state.demoTick >= 10) {
        state.demoTick = 0;
        const nextPhase = (state.demoPhase + 1) % demoScenarios.length;
        state.demoPhase = nextPhase;
        const nextSc = demoScenarios[nextPhase];
        log('demo', '📍 ' + nextSc.label);
        // Log each detected object in the new scenario
        (nextSc.detections || []).forEach(det => {
          log('detect', det.label + ' detected (' + Math.round(det.confidence * 100) + '%)');
        });
      }
    }, 500);
  }

  /* ═════════════════════════════════════════════════════════════
     LIVE MODE  –  WebSocket (sensor data from ESP32)
     ═════════════════════════════════════════════════════════════ */
  function connectWs() {
    if (state.ws && (state.ws.readyState === WebSocket.CONNECTING ||
                     state.ws.readyState === WebSocket.OPEN)) return;

    setPill('sensor', 'connecting');
    $('ws-status').textContent = 'Connecting…';

    let ws;
    try { ws = new WebSocket(WS_URL); }
    catch (e) { scheduleWsRetry(); return; }

    state.ws = ws;

    ws.onopen = () => {
      state.wsConnected = true;
      setPill('sensor', true);
      $('ws-status').textContent = 'OK';
      log('sys', 'Sensor ESP32 connected');
      checkOverallMode();
    };

    ws.onmessage = (ev) => {
      try {
        const d = JSON.parse(ev.data);
        if (state.mode === 'live') {
          applySensorData(d);
        }
        if (d.detections) {
          // Log newly-appeared object labels
          const prevLabels = state.lastDetections.map(x => x.label).sort().join(',');
          const newLabels  = d.detections.map(x => x.label).sort().join(',');
          if (newLabels !== prevLabels && d.detections.length > 0) {
            d.detections.forEach(det => {
              log('detect', det.label + ' detected (' + Math.round((det.confidence||0)*100) + '%)');
            });
          }
          state.lastDetections = d.detections;
          drawOverlay(d.detections);
          $('obj-count').textContent = d.detections.length;
          setPill('ai', true);
        }
      } catch (_) {}
    };

    ws.onclose = ws.onerror = () => {
      state.wsConnected = false;
      setPill('sensor', false);
      $('ws-status').textContent = 'Offline';
      checkOverallMode();
      scheduleWsRetry();
    };
  }

  function scheduleWsRetry() {
    clearTimeout(state.wsRetryTimer);
    state.wsRetryTimer = setTimeout(connectWs, 4000);
  }

  /* ═════════════════════════════════════════════════════════════
     LIVE MODE  –  MJPEG camera stream (ESP32-CAM)
     ═════════════════════════════════════════════════════════════ */
  function connectCam() {
    setPill('cam', 'connecting');
    const img         = $('cam-img');
    const placeholder = $('cam-placeholder');
    const badge       = $('live-badge');

    // Attempt with a tiny probe image first so we don't lock
    // the main <img> on a dead stream for too long.
    const probe = new Image();
    probe.onload = () => {
      // Stream is reachable — switch to real img
      img.onload = () => {
        state.camConnected = true;
        setPill('cam', true);
        placeholder.classList.add('hide');
        state.fpsCount++;
        const now = Date.now();
        if (now - state.fpsTime >= 1000) {
          $('fps').textContent = state.fpsCount + ' fps';
          state.fpsCount = 0;
          state.fpsTime  = now;
        }
        drawOverlay(state.lastDetections);
        checkOverallMode();
      };
      img.onerror = () => {
        onCamFail();
      };
      img.style.display = 'block';
      img.src = STREAM_URL + '?t=' + Date.now();
      badge.classList.add('on');
      badge.textContent = 'LIVE';
    };
    probe.onerror = () => {
      onCamFail();
    };
    probe.src = 'http://' + CAM_HOST + '/capture?t=' + Date.now();
  }

  function onCamFail() {
    state.camConnected = false;
    setPill('cam', false);
    $('live-badge').classList.remove('on');
    checkOverallMode();
    clearTimeout(state.camRetryTimer);
    state.camRetryTimer = setTimeout(connectCam, 5000);
  }

  /* ═════════════════════════════════════════════════════════════
     LIVE MODE  –  AI detection WebSocket (Python backend)
     ═════════════════════════════════════════════════════════════ */
  function connectAi() {
    fetch(CONFIG_URL, { signal: AbortSignal.timeout(2000) })
      .then(r => r.json())
      .then(cfg => {
        const url = cfg && cfg.detectionWs;
        if (!url) { setPill('ai', false); return; }
        setPill('ai', 'connecting');
        const aiWs = new WebSocket(url);
        aiWs.onopen  = () => { state.aiConnected = true;  setPill('ai', true);  log('sys', 'AI detection active'); };
        aiWs.onmessage = (ev) => {
          try {
            const d = JSON.parse(ev.data);
            if (d.detections) {
              state.lastDetections = d.detections;
              drawOverlay(d.detections);
              $('obj-count').textContent = d.detections.length;
            }
          } catch (_) {}
        };
        aiWs.onclose = aiWs.onerror = () => {
          state.aiConnected = false;
          setPill('ai', false);
          clearTimeout(state.aiRetryTimer);
          state.aiRetryTimer = setTimeout(connectAi, 6000);
        };
      })
      .catch(() => {
        setPill('ai', false);
        clearTimeout(state.aiRetryTimer);
        state.aiRetryTimer = setTimeout(connectAi, 6000);
      });
  }

  /* ── Clock & FPS ─────────────────────────────────────────────── */
  setInterval(() => {
    $('clock').textContent = new Date().toTimeString().slice(0, 8);
  }, 1000);

  /* ── Resize overlay on window resize ─────────────────────────── */
  window.addEventListener('resize', () => {
    drawTopView({ finalDist: null, finalAngle: 0, valid: false }, []);
    drawOverlay(state.lastDetections);
  });

  /* ── Init ────────────────────────────────────────────────────── */
  $('device-addr').textContent = isLocal ? 'Local AI Streamer' : SENSOR_HOST;

  // Start in demo mode immediately
  setMode('demo');
  $('app').classList.add('demo-mode');

  // Ensure demo video plays (browser policy requires interaction or autoplay)
  const vid = $('demo-video');
  vid.muted = true;
  vid.play().catch(() => {
    // Autoplay blocked; wait for user click
    document.addEventListener('click', () => vid.play(), { once: true });
  });

  // Start demo data loop
  startDemoLoop();

  // Blank top-view until data arrives
  drawTopView({ finalDist: null, finalAngle: 0, valid: false }, []);

  // Begin hardware connection attempts in background
  connectWs();
  connectCam();
  connectAi();

})();

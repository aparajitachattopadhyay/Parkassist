/*
 * ParkAssist — Frontend JavaScript
 * Served at http://192.168.4.1/script.js
 *
 * FIX vs original frontend/script.js:
 *   STREAM_URL now correctly points to 192.168.4.2/stream (ESP32-CAM)
 *   instead of 192.168.4.1/stream (sensor ESP32 does not host a camera).
 *   Everything else is identical to the main frontend script.
 */

#pragma once

const char script_js[] PROGMEM = R"rawliteral(
(function () {
  const MAX_CM = 250;
  const STOP_CM = 30;
  const WARN_CM = 60;

  const audioCtx = new (window.AudioContext || window.webkitAudioContext)();
  let lastBeep = 0;
  function playBeep(dist) {
    if (dist > 60) return;
    if (audioCtx.state === 'suspended') return;
    const now = Date.now();
    const interval = Math.max(100, (dist / 60) * 1000);
    if (now - lastBeep < interval) return;
    lastBeep = now;

    const osc = audioCtx.createOscillator();
    const gain = audioCtx.createGain();
    osc.type = 'sine';
    osc.frequency.setValueAtTime(800 - (dist * 5), audioCtx.currentTime);
    gain.gain.setValueAtTime(0.1, audioCtx.currentTime);
    gain.gain.exponentialRampToValueAtTime(0.001, audioCtx.currentTime + 0.1);
    osc.connect(gain);
    gain.connect(audioCtx.destination);
    osc.start();
    osc.stop(audioCtx.currentTime + 0.1);
  }

  document.addEventListener('click', function() {
    if (audioCtx.state === 'suspended') {
      audioCtx.resume();
    }
  }, { once: true });

  function host() {
    const h = window.location.hostname;
    if (h && h !== 'localhost' && h !== '127.0.0.1') return h;
    return '192.168.4.1';
  }

  const HOST = host();
  const WS_URL     = 'ws://' + HOST + ':81';

  // ── FIX: Camera is on the ESP32-CAM (192.168.4.2), NOT the sensor ESP32. ──
  // When served from the real AP (192.168.4.1), always point cam at .2.
  // When developing locally, fall back to the same host for easy testing.
  const CAM_IP     = (HOST === '192.168.4.1') ? '192.168.4.2' : HOST;
  const STREAM_URL = 'http://' + CAM_IP + '/stream';
  // ─────────────────────────────────────────────────────────────────────────

  const CONFIG_URL = 'http://' + HOST + '/config';

  const state = { ws: false, cam: false, ai: false, fpsCount: 0, fpsTime: Date.now(), lastDetections: [] };

  function setPill(id, status) {
    const el = document.getElementById(id);
    el.className = 'pill ' + (status === true ? 'on' : status === false ? 'off' : 'connecting');
  }

  function log(tag, msg) {
    const el = document.getElementById('event-log');
    const line = document.createElement('div');
    line.className = 'log-line';
    const t = new Date().toTimeString().slice(3, 8);
    line.innerHTML = '<span class="t">' + t + '</span><span class="tag ' + tag + '">' + tag + '</span><span>' + msg + '</span>';
    el.insertBefore(line, el.firstChild);
    while (el.children.length > 50) el.removeChild(el.lastChild);
  }

  function colorForDist(d) {
    if (d <= STOP_CM) return 'var(--danger)';
    if (d <= WARN_CM) return 'var(--warn)';
    return 'var(--safe)';
  }

  function updateBar(barId, valId, cm) {
    const bar = document.getElementById(barId);
    const val = document.getElementById(valId);
    if (cm == null || cm < 0) {
      val.textContent = '-- cm';
      bar.style.width = '0%';
      bar.style.background = 'var(--muted)';
      return;
    }
    val.textContent = Math.round(cm) + ' cm';
    bar.style.width = Math.min(100, (cm / MAX_CM) * 100) + '%';
    bar.style.background = colorForDist(cm);
  }

  function updateSensor(data) {
    const usL = data.usL != null ? data.usL : data.dA;
    const usR = data.usR != null ? data.usR : data.dB;
    const lidarL = data.lidarL != null ? data.lidarL : data.dA;
    const lidarR = data.lidarR != null ? data.lidarR : data.dB;

    let sensors = [
      { val: usL, ang: -35 },
      { val: lidarL, ang: -10 },
      { val: lidarR, ang: 10 },
      { val: usR, ang: 35 }
    ];

    let validSensors = sensors.filter(s => s.val != null && s.val > 0 && s.val <= MAX_CM);

    let finalDist = null;
    let finalAngle = 0;

    if (validSensors.length > 0) {
      let sumW = 0, sumX = 0, sumY = 0;
      validSensors.forEach(s => {
        let rad = s.ang * Math.PI / 180;
        let x = s.val * Math.sin(rad);
        let y = s.val * Math.cos(rad);
        let w = 1.0 / (s.val * s.val);
        sumW += w;
        sumX += x * w;
        sumY += y * w;
      });
      let avgX = sumX / sumW;
      let avgY = sumY / sumW;

      finalDist = Math.sqrt(avgX * avgX + avgY * avgY);
      finalAngle = Math.atan2(avgX, avgY) * 180 / Math.PI;
    } else if (data.dist != null) {
      finalDist = data.dist;
      finalAngle = data.angle != null ? data.angle : 0;
    }

    updateBar('bar-final', 'val-final', finalDist);

    if (finalDist && finalDist <= WARN_CM) {
      playBeep(finalDist);
    }

    const needle = document.getElementById('angle-needle');
    const pct = 50 + (finalAngle / 45) * 48;
    needle.style.left = Math.max(5, Math.min(95, pct)) + '%';
    needle.style.background = Math.abs(finalAngle) > 20 ? 'var(--danger)' : Math.abs(finalAngle) > 8 ? 'var(--warn)' : 'var(--accent)';
    document.getElementById('angle-val').textContent = (finalAngle >= 0 ? '+' : '') + finalAngle.toFixed(1) + '\u00b0';
    document.getElementById('angle-val').style.color = needle.style.background;

    const cv = document.getElementById('closest-val');
    const thr = document.getElementById('threat');
    if (!data.valid || finalDist == null) {
      cv.textContent = '--';
      cv.style.color = 'var(--muted)';
      thr.textContent = '--';
      thr.style.color = 'var(--muted)';
    } else {
      const d = Math.round(finalDist);
      cv.textContent = d;
      cv.style.color = colorForDist(d);
      if (d <= STOP_CM) { thr.textContent = 'STOP'; thr.style.color = 'var(--danger)'; }
      else if (d <= WARN_CM) { thr.textContent = 'CAUTION'; thr.style.color = 'var(--warn)'; }
      else { thr.textContent = 'CLEAR'; thr.style.color = 'var(--safe)'; }
    }

    drawTopView({ finalDist, finalAngle, valid: data.valid }, state.lastDetections);
  }

  const shapeColors = { person: '#ef4444', car: '#eab308', bus: '#eab308', bicycle: '#22c55e', motorcycle: '#22c55e', cat: '#3b82f6', dog: '#3b82f6' };

  function drawShape(ctx, type, x, y, size, color) {
    ctx.fillStyle = color;
    ctx.beginPath();
    if (type === 'person') {
       ctx.arc(x, y, size, 0, Math.PI*2);
    } else if (type === 'car' || type === 'bus') {
       ctx.rect(x - size*1.2, y - size, size*2.4, size*2);
    } else {
       ctx.moveTo(x, y - size);
       ctx.lineTo(x + size, y + size);
       ctx.lineTo(x - size, y + size);
    }
    ctx.fill();
    ctx.fillStyle = '#fff';
    ctx.font = '10px sans-serif';
    ctx.fillText(type, x - 12, y + size + 10);
  }

  function drawTopView(sensor, detections) {
    const c = document.getElementById('topview-canvas');
    if (!c) return;
    const ctx = c.getContext('2d');
    const w = c.width, h = c.height;
    ctx.fillStyle = '#1a1b1e';
    ctx.fillRect(0, 0, w, h);
    ctx.strokeStyle = '#2d2e33';
    ctx.lineWidth = 1;
    ctx.strokeRect(0, 0, w, h);

    ctx.beginPath();
    ctx.moveTo(w/2, 0); ctx.lineTo(w/2, h);
    for(let i=h/5; i<h; i+=h/5) { ctx.moveTo(0, i); ctx.lineTo(w, i); }
    ctx.stroke();

    const cx = w / 2, baseY = h - 35;

    ctx.fillStyle = '#3b82f6';
    ctx.beginPath();
    ctx.roundRect(cx - 24, baseY - 12, 48, 24, 4);
    ctx.fill();
    ctx.fillStyle = '#fff';
    ctx.font = '11px sans-serif';
    ctx.fillText('CAR', cx - 11, baseY + 3);

    const maxR = baseY - 20;

    function plotSensor(dist, angleDeg) {
        if (dist == null || dist <= 0 || dist > MAX_CM) return;
        const rad = angleDeg * Math.PI / 180;
        const Y_dist = (dist / MAX_CM) * maxR;
        const ox = cx + Math.sin(rad) * Y_dist;
        const oy = baseY - 12 - Math.cos(rad) * Y_dist;

        ctx.fillStyle = colorForDist(dist);
        ctx.beginPath();
        ctx.arc(ox, oy, 6, 0, Math.PI * 2);
        ctx.fill();
        ctx.fillStyle = 'rgba(255,255,255,0.7)';
        ctx.font = '9px sans-serif';
        ctx.fillText(Math.round(dist), ox + 8, oy + 3);
    }

    if (sensor.valid && sensor.finalDist != null) {
        plotSensor(sensor.finalDist, sensor.finalAngle);
    }

    if (detections && detections.length > 0) {
        const fallbackDist = sensor.finalDist != null ? sensor.finalDist : 100;
        detections.forEach(d => {
            const objAngle = (d.x - 0.5) * 60;
            const rad = objAngle * Math.PI / 180;
            const Y_dist = (fallbackDist / MAX_CM) * maxR;
            const ox = cx + Math.sin(rad) * Y_dist;
            const oy = baseY - 12 - Math.cos(rad) * Y_dist;
            const col = shapeColors[d.label] || '#3b82f6';
            drawShape(ctx, d.label, ox, oy, 12, col);
        });
    }
  }

  function drawOverlay(detections) {
    const canvas = document.getElementById('overlay');
    const img = document.getElementById('cam-img');
    if (!img.offsetWidth) return;
    canvas.width = img.offsetWidth;
    canvas.height = img.offsetHeight;
    const ctx = canvas.getContext('2d');
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    (detections || []).forEach(function (d) {
      const x = d.x * canvas.width, y = d.y * canvas.height, w = d.w * canvas.width, h = d.h * canvas.height;
      const col = shapeColors[d.label] || '#3b82f6';
      ctx.strokeStyle = col;
      ctx.lineWidth = 2;
      ctx.strokeRect(x, y, w, h);
      ctx.fillStyle = col;
      ctx.font = '11px sans-serif';
      ctx.fillText(d.label + ' ' + Math.round((d.confidence || 0) * 100) + '%', x, y - 4);
    });
  }

  function connectWs() {
    setPill('pill-sensor', 'connecting');
    document.getElementById('ws-status').textContent = 'Connecting';
    const ws = new WebSocket(WS_URL);
    ws.onopen = function () {
      state.ws = true;
      setPill('pill-sensor', true);
      document.getElementById('ws-status').textContent = 'OK';
      log('sys', 'Sensor connected');
    };
    ws.onmessage = function (ev) {
      try {
        const d = JSON.parse(ev.data);
        updateSensor(d);
        if (d.detections && d.detections.length) {
          state.lastDetections = d.detections;
          drawOverlay(d.detections);
          document.getElementById('obj-count').textContent = d.detections.length;
        }
      } catch (_) {}
    };
    ws.onclose = function () {
      state.ws = false;
      setPill('pill-sensor', false);
      document.getElementById('ws-status').textContent = 'Retry';
      setTimeout(connectWs, 3000);
    };
    ws.onerror = function () {
      setPill('pill-sensor', false);
    };
  }

  function connectCam() {
    setPill('pill-cam', 'connecting');
    const img = document.getElementById('cam-img');
    const placeholder = document.getElementById('cam-placeholder');
    const badge = document.getElementById('live-badge');
    img.onload = function () {
      img.style.display = 'block';
      placeholder.classList.add('hide');
      badge.classList.add('on');
      state.cam = true;
      setPill('pill-cam', true);
      state.fpsCount++;
      var now = Date.now();
      if (now - state.fpsTime >= 1000) {
        document.getElementById('fps').textContent = state.fpsCount + ' fps';
        state.fpsCount = 0;
        state.fpsTime = now;
      }
      drawOverlay(state.lastDetections);
    };
    img.onerror = function () {
      img.style.display = 'none';
      placeholder.classList.remove('hide');
      placeholder.textContent = 'Camera unavailable. Retrying\u2026';
      state.cam = false;
      setPill('pill-cam', false);
      badge.classList.remove('on');
      setTimeout(connectCam, 4000);
    };
    img.src = STREAM_URL;
  }

  function connectAi() {
    fetch(CONFIG_URL).then(function (r) { return r.json(); }).then(function (cfg) {
      const url = cfg && cfg.detectionWs;
      if (!url) {
        setPill('pill-ai', false);
        return;
      }
      setPill('pill-ai', 'connecting');
      const ws = new WebSocket(url);
      ws.onopen = function () {
        state.ai = true;
        setPill('pill-ai', true);
        log('sys', 'AI detection connected');
      };
      ws.onmessage = function (ev) {
        try {
          const d = JSON.parse(ev.data);
          if (d.detections) {
            state.lastDetections = d.detections;
            drawOverlay(d.detections);
            document.getElementById('obj-count').textContent = d.detections.length;
          }
        } catch (_) {}
      };
      ws.onclose = function () {
        state.ai = false;
        setPill('pill-ai', false);
        setTimeout(connectAi, 5000);
      };
    }).catch(function () {
      setPill('pill-ai', false);
    });
  }

  setInterval(function () {
    document.getElementById('clock').textContent = new Date().toTimeString().slice(0, 8);
  }, 1000);

  document.getElementById('device-addr').textContent = HOST;
  connectWs();
  connectCam();
  connectAi();
})();
)rawliteral";

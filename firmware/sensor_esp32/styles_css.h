/*
 * ParkAssist — Stylesheet
 * Served at http://192.168.4.1/styles.css
 */

#pragma once

const char styles_css[] PROGMEM = R"rawliteral(
    * { box-sizing: border-box; margin: 0; padding: 0; }
    :root {
      --bg: #111113;
      --surface: #1a1b1e;
      --border: #2d2e33;
      --text: #e4e4e7;
      --muted: #71717a;
      --safe: #22c55e;
      --warn: #eab308;
      --danger: #ef4444;
      --accent: #3b82f6;
    }
    html, body { height: 100%; background: var(--bg); color: var(--text); font: 14px/1.4 system-ui, sans-serif; overflow: hidden; -webkit-tap-highlight-color: transparent; }
    @media (orientation: portrait) {
      #app { display: none !important; }
      #rotate-msg { display: flex !important; }
    }
    #rotate-msg {
      display: none;
      position: fixed; inset: 0;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      gap: 1rem;
      background: var(--bg);
      color: var(--muted);
      font-size: 1.1rem;
    }
    #rotate-msg svg { opacity: 0.6; }

    #app {
      display: grid;
      width: 100vw;
      height: 100vh;
      grid-template-columns: 1fr 400px;
      grid-template-rows: 48px 1fr 40px;
    }
    header {
      grid-column: 1 / -1;
      display: flex;
      align-items: center;
      padding: 0 16px;
      gap: 16px;
      background: var(--surface);
      border-bottom: 1px solid var(--border);
    }
    .logo { font-weight: 700; font-size: 1.1rem; color: var(--text); }
    .logo span { color: var(--accent); }
    .pill {
      display: inline-flex;
      align-items: center;
      gap: 6px;
      padding: 4px 10px;
      border-radius: 999px;
      font-size: 11px;
      color: var(--muted);
      background: var(--bg);
    }
    .pill .dot { width: 6px; height: 6px; border-radius: 50%; background: var(--muted); }
    .pill.on .dot { background: var(--safe); }
    .pill.off .dot { background: var(--danger); }
    .pill.connecting .dot { background: var(--warn); animation: pulse 0.8s ease-in-out infinite; }
    @keyframes pulse { 50% { opacity: 0.4; } }
    .header-right { margin-left: auto; display: flex; align-items: center; gap: 12px; font-size: 12px; color: var(--muted); }

    .camera-wrap {
      position: relative;
      background: #000;
      overflow: hidden;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    .camera-wrap img { width: 100%; height: 100%; object-fit: contain; display: none; }
    .camera-wrap canvas { position: absolute; inset: 0; width: 100%; height: 100%; pointer-events: none; }
    .camera-placeholder {
      position: absolute;
      inset: 0;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      color: var(--muted);
      font-size: 13px;
    }
    .camera-placeholder.hide { display: none; }
    .live-badge {
      position: absolute;
      top: 10px;
      left: 50%;
      transform: translateX(-50%);
      padding: 4px 12px;
      border-radius: 4px;
      font-size: 11px;
      font-weight: 600;
      background: rgba(0,0,0,0.6);
      color: var(--muted);
    }
    .live-badge.on { color: var(--safe); }

    .sidebar {
      display: flex;
      flex-direction: column;
      background: var(--surface);
      border-left: 1px solid var(--border);
      overflow-y: auto;
    }
    .block { padding: 12px 16px; border-bottom: 1px solid var(--border); }
    .block-title { font-size: 10px; font-weight: 600; text-transform: uppercase; letter-spacing: 0.08em; color: var(--muted); margin-bottom: 10px; }
    
    .distance-row { display: flex; align-items: center; gap: 10px; margin-bottom: 8px; }
    .distance-row:last-child { margin-bottom: 0; }
    .distance-label { width: 36px; font-size: 11px; color: var(--muted); }
    .distance-bar-wrap { flex: 1; height: 8px; background: var(--bg); border-radius: 4px; overflow: hidden; }
    .distance-bar { height: 100%; border-radius: 4px; transition: width 0.15s, background 0.2s; }
    .distance-val { width: 48px; text-align: right; font-size: 12px; font-weight: 500; }
    
    .angle-row { display: flex; align-items: center; gap: 10px; margin-top: 10px; padding-top: 10px; border-top: 1px dashed var(--border); }
    .angle-bar-wrap { flex: 1; height: 10px; background: var(--bg); border-radius: 4px; position: relative; }
    .angle-bar-wrap::before { content: ''; position: absolute; left: 50%; top: 0; bottom: 0; width: 2px; background: var(--border); transform: translateX(-50%); }
    .angle-needle { position: absolute; top: 1px; bottom: 1px; width: 8px; background: var(--accent); border-radius: 2px; left: 50%; transform: translate(-50%, 0); transition: left 0.15s; }
    .angle-val { width: 52px; text-align: right; font-size: 12px; }

    .map-wrap {
      flex: 1;
      display: flex;
      align-items: center;
      justify-content: center;
      background: var(--bg);
      border-radius: 8px;
      margin-top: 8px;
      padding: 10px;
    }
    #topview-canvas { 
      border-radius: 8px; 
      display: block; 
      width: 100%; 
      max-width: 360px;
      height: auto; 
    }

    .closest-strip {
      display: flex;
      align-items: baseline;
      gap: 8px;
      padding: 12px 16px;
      background: var(--bg);
      border-top: 1px solid var(--border);
    }
    .closest-label { font-size: 11px; color: var(--muted); }
    .closest-value { font-size: 28px; font-weight: 700; transition: color 0.2s; }
    .closest-unit { font-size: 12px; color: var(--muted); }
    .threat { margin-left: auto; font-size: 12px; font-weight: 700; }

    .log-wrap { flex: 1; min-height: 120px; overflow: hidden; display: flex; flex-direction: column; padding: 12px 16px; }
    #event-log { flex: 1; overflow-y: auto; font-size: 11px; display: flex; flex-direction: column; gap: 4px; padding-right: 4px; }
    .log-line { display: flex; gap: 8px; align-items: baseline; color: var(--muted); }
    .log-line .t { flex-shrink: 0; }
    .log-line .tag { flex-shrink: 0; padding: 2px 6px; border-radius: 4px; font-size: 10px; font-weight: 600; }
    .log-line .tag.detect { background: rgba(34,197,94,0.15); color: var(--safe); }
    .log-line .tag.alert { background: rgba(239,68,68,0.15); color: var(--danger); }
    .log-line .tag.sys { background: rgba(113,113,122,0.2); color: var(--muted); }

    footer {
      grid-column: 1 / -1;
      display: flex;
      align-items: center;
      padding: 0 16px;
      gap: 16px;
      font-size: 11px;
      color: var(--muted);
      background: var(--surface);
      border-top: 1px solid var(--border);
    }
    footer .device { margin-left: auto; }
)rawliteral";

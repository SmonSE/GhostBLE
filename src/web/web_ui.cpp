#include "web_ui.h"

// ------------------------------------------------------------
//  Room words for environment name detection
// ------------------------------------------------------------
const std::vector<std::string> roomWords = {
    "wohnzimmer", "küche", "kueche", "bad",
    "schlafzimmer", "office", "living",
    "bedroom", "kitchen", "bath",
    "arbeitszimmer", "gästezimmer", "garage", "büro"
};

// ------------------------------------------------------------
//  GhostBLE Web UI
//  Split-view: Live BLE device cards (top) + trace log (bottom)
//  WebSocket message types:
//    { "type": "device", "data": { ... } }  → device card
//    { "type": "log",    "data": "..."    }  → trace log line
//    { "type": "config", ... }               → settings sync
// ------------------------------------------------------------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>GhostBLE</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Orbitron:wght@400;700;900&display=swap');

    :root {
      --bg:        #080c10;
      --bg2:       #0d1520;
      --bg3:       #111d2e;
      --green:     #00ff88;
      --green-dim: #00cc66;
      --cyan:      #00d4ff;
      --red:       #ff3366;
      --amber:     #ffaa00;
      --text:      #c8d8e8;
      --text-dim:  #5a7a9a;
      --border:    #1a3050;
      --card-bg:   #0a1828;
      --scan:      #00ff88;
      --radius:    6px;
    }

    * { box-sizing: border-box; margin: 0; padding: 0; }

    body {
      background: var(--bg);
      color: var(--text);
      font-family: 'Share Tech Mono', monospace;
      font-size: 13px;
      height: 100dvh;
      display: flex;
      flex-direction: column;
      overflow: hidden;
    }

    /* ── Header ── */
    header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 10px 16px;
      background: var(--bg2);
      border-bottom: 1px solid var(--border);
      flex-shrink: 0;
    }

    .logo {
      font-family: 'Orbitron', sans-serif;
      font-size: 18px;
      font-weight: 900;
      color: var(--green);
      letter-spacing: 3px;
      text-shadow: 0 0 20px rgba(0,255,136,0.5);
    }

    .logo span { color: var(--cyan); }

    .status-bar {
      display: flex;
      gap: 16px;
      align-items: center;
      font-size: 11px;
      color: var(--text-dim);
    }

    .status-dot {
      width: 7px; height: 7px;
      border-radius: 50%;
      background: var(--text-dim);
      display: inline-block;
      margin-right: 5px;
    }

    .status-dot.active {
      background: var(--green);
      box-shadow: 0 0 8px var(--green);
      animation: pulse 1.5s infinite;
    }

    @keyframes pulse {
      0%, 100% { opacity: 1; }
      50%       { opacity: 0.4; }
    }

    .stat-item { display: flex; align-items: center; gap: 3px; }
    .stat-val  { color: var(--cyan); font-weight: bold; }

    /* ── Main layout ── */
    .main {
      display: flex;
      flex-direction: column;
      flex: 1;
      overflow: hidden;
      gap: 0;
    }

    /* ── Device section ── */
    .section-header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 6px 16px;
      background: var(--bg2);
      border-bottom: 1px solid var(--border);
      font-size: 10px;
      color: var(--text-dim);
      letter-spacing: 2px;
      text-transform: uppercase;
      flex-shrink: 0;
    }

    .section-header .label {
      color: var(--cyan);
      font-family: 'Orbitron', sans-serif;
      font-size: 9px;
    }

    #devices-wrap {
      flex: 0 0 auto;
      height: 42%;
      overflow-y: auto;
      padding: 10px 12px;
      background: var(--bg);
    }

    #devices-wrap::-webkit-scrollbar { width: 4px; }
    #devices-wrap::-webkit-scrollbar-track { background: var(--bg2); }
    #devices-wrap::-webkit-scrollbar-thumb { background: var(--border); border-radius: 2px; }

    .devices-grid {
      display: grid;
      grid-template-columns: repeat(auto-fill, minmax(220px, 1fr));
      gap: 8px;
    }

    .empty-state {
      text-align: center;
      color: var(--text-dim);
      padding: 30px;
      font-size: 12px;
      letter-spacing: 2px;
    }

    .empty-state::before {
      content: '◈';
      display: block;
      font-size: 28px;
      color: var(--border);
      margin-bottom: 8px;
    }

    /* ── Device card ── */
    .device-card {
      background: var(--card-bg);
      border: 1px solid var(--border);
      border-radius: var(--radius);
      padding: 10px 12px;
      position: relative;
      overflow: hidden;
      animation: cardIn 0.25s ease;
      transition: border-color 0.2s;
    }

    .device-card:hover { border-color: var(--cyan); }

    .device-card::before {
      content: '';
      position: absolute;
      top: 0; left: 0; right: 0;
      height: 2px;
      background: var(--card-accent, var(--green));
    }

    @keyframes cardIn {
      from { opacity: 0; transform: translateY(6px); }
      to   { opacity: 1; transform: translateY(0); }
    }

    .card-top {
      display: flex;
      justify-content: space-between;
      align-items: flex-start;
      margin-bottom: 6px;
    }

    .card-name {
      font-family: 'Orbitron', sans-serif;
      font-size: 11px;
      color: var(--green);
      font-weight: 700;
      max-width: 150px;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
    }

    .card-rssi {
      font-size: 11px;
      font-weight: bold;
      padding: 1px 6px;
      border-radius: 3px;
      background: var(--rssi-bg, #1a2a1a);
      color: var(--rssi-color, var(--green));
    }

    .card-ghost {
      font-size: 10px;
      color: var(--text-dim);
      margin-bottom: 5px;
      letter-spacing: 1px;
    }

    .card-ghost span { color: var(--cyan); }

    .card-meta {
      display: flex;
      flex-wrap: wrap;
      gap: 4px;
      margin-top: 5px;
    }

    .tag {
      font-size: 9px;
      padding: 1px 5px;
      border-radius: 2px;
      letter-spacing: 1px;
      text-transform: uppercase;
    }

    .tag-connectable { background: #0a2a1a; color: var(--green-dim); border: 1px solid #0f3a20; }
    .tag-beacon      { background: #1a1a0a; color: var(--amber);     border: 1px solid #2a2a0a; }
    .tag-pwn         { background: #2a0a1a; color: var(--red);       border: 1px solid #3a1020; }
    .tag-notify      { background: #0a1a2a; color: var(--cyan);      border: 1px solid #0a2a3a; }
    .tag-sus         { background: #2a1a0a; color: #ff6600;          border: 1px solid #3a2a0a; }

    .rssi-bar {
      margin-top: 6px;
      height: 2px;
      background: var(--bg3);
      border-radius: 1px;
      overflow: hidden;
    }

    .rssi-bar-fill {
      height: 100%;
      border-radius: 1px;
      background: var(--rssi-color, var(--green));
      transition: width 0.3s ease;
    }

    /* ── Divider ── */
    .divider {
      height: 1px;
      background: var(--border);
      flex-shrink: 0;
      position: relative;
    }

    .divider-handle {
      position: absolute;
      left: 50%;
      top: -8px;
      transform: translateX(-50%);
      background: var(--bg2);
      border: 1px solid var(--border);
      border-radius: 3px;
      padding: 2px 10px;
      font-size: 9px;
      color: var(--text-dim);
      cursor: ns-resize;
      letter-spacing: 2px;
    }

    /* ── Log section ── */
    #log-wrap {
      flex: 1;
      overflow-y: auto;
      padding: 8px 12px 8px;
      background: var(--bg);
      font-size: 11px;
      line-height: 1.7;
    }

    #log-wrap::-webkit-scrollbar { width: 4px; }
    #log-wrap::-webkit-scrollbar-track { background: var(--bg2); }
    #log-wrap::-webkit-scrollbar-thumb { background: var(--border); border-radius: 2px; }

    .log-line {
      display: flex;
      gap: 8px;
      padding: 1px 0;
      border-bottom: 1px solid rgba(26,48,80,0.3);
      animation: logIn 0.15s ease;
    }

    @keyframes logIn {
      from { opacity: 0; }
      to   { opacity: 1; }
    }

    .log-ts   { color: var(--text-dim); flex-shrink: 0; font-size: 10px; }
    .log-text { color: var(--text); word-break: break-all; flex: 1; }

    .log-text.scan     { color: var(--green); }
    .log-text.security { color: var(--red); }
    .log-text.beacon   { color: var(--amber); }
    .log-text.gatt     { color: var(--cyan); }
    .log-text.system   { color: var(--text-dim); }
    .log-text.notify   { color: #cc88ff; }
    .log-text.privacy  { color: #ff8888; }

    /* ── Settings panel ── */
    .settings-panel {
      position: fixed;
      top: 0; right: 0;
      width: 280px;
      height: 100%;
      background: var(--bg2);
      border-left: 1px solid var(--border);
      padding: 16px;
      transform: translateX(100%);
      transition: transform 0.25s ease;
      z-index: 100;
      overflow-y: auto;
    }

    .settings-panel.open { transform: translateX(0); }

    .settings-panel h3 {
      font-family: 'Orbitron', sans-serif;
      font-size: 12px;
      color: var(--cyan);
      letter-spacing: 3px;
      margin-bottom: 16px;
    }

    .field { margin-bottom: 12px; }
    .field label { display: block; font-size: 10px; color: var(--text-dim); margin-bottom: 4px; letter-spacing: 1px; }
    .field input {
      width: 100%;
      background: var(--bg3);
      border: 1px solid var(--border);
      color: var(--text);
      padding: 6px 8px;
      font-family: 'Share Tech Mono', monospace;
      font-size: 12px;
      border-radius: var(--radius);
      outline: none;
    }
    .field input:focus { border-color: var(--cyan); }

    .btn {
      background: transparent;
      border: 1px solid var(--green);
      color: var(--green);
      padding: 6px 14px;
      font-family: 'Share Tech Mono', monospace;
      font-size: 11px;
      letter-spacing: 2px;
      border-radius: var(--radius);
      cursor: pointer;
      transition: background 0.15s;
    }

    .btn:hover { background: rgba(0,255,136,0.1); }
    .btn-icon {
      background: none;
      border: none;
      color: var(--text-dim);
      cursor: pointer;
      font-size: 14px;
      padding: 2px 6px;
      border-radius: 3px;
      transition: color 0.15s;
    }
    .btn-icon:hover { color: var(--cyan); }

    .cfg-status { font-size: 10px; color: var(--green-dim); margin-top: 8px; min-height: 14px; }

    /* ── Scan filter bar ── */
    .filter-bar {
      display: flex;
      gap: 8px;
      align-items: center;
    }

    .filter-bar input {
      background: var(--bg3);
      border: 1px solid var(--border);
      color: var(--text);
      padding: 3px 8px;
      font-family: 'Share Tech Mono', monospace;
      font-size: 11px;
      border-radius: var(--radius);
      outline: none;
      width: 100px;
    }

    .filter-bar input:focus { border-color: var(--cyan); }

    #overlay {
      position: fixed; inset: 0;
      background: rgba(0,0,0,0.5);
      display: none;
      z-index: 99;
    }
    #overlay.visible { display: block; }
  </style>
</head>
<body>

<!-- ── Header ── -->
<header>
  <div class="logo">GHOST<span>BLE</span></div>
  <div class="status-bar">
    <div class="stat-item"><span class="status-dot" id="ws-dot"></span><span id="ws-status">OFFLINE</span></div>
    <div class="stat-item">SPT <span class="stat-val" id="stat-spt">0</span></div>
    <div class="stat-item">SUS <span class="stat-val" id="stat-sus" style="color:var(--red)">0</span></div>
    <div class="stat-item">BCN <span class="stat-val" id="stat-bcn" style="color:var(--amber)">0</span></div>
    <div class="stat-item">PWN <span class="stat-val" id="stat-pwn" style="color:var(--red)">0</span></div>
  </div>
</header>

<!-- ── Devices ── -->
<div class="section-header">
  <span class="label">Live BLE Devices</span>
  <div class="filter-bar">
    <input id="filter-input" placeholder="filter name" oninput="applyFilter()">
    <button class="btn-icon" onclick="clearDevices()" title="Clear">✕</button>
    <button class="btn-icon" onclick="toggleSettings()" title="Settings">⚙</button>
  </div>
</div>

<div id="devices-wrap">
  <div id="devices" class="devices-grid">
    <div class="empty-state">SCANNING FOR DEVICES</div>
  </div>
</div>

<!-- ── Divider ── -->
<div class="divider">
  <div class="divider-handle">▼ TRACE LOG ▼</div>
</div>

<!-- ── Log ── -->
<div class="section-header">
  <span class="label">Trace Log</span>
  <div style="display:flex;gap:8px;align-items:center;">
    <span id="log-count" style="color:var(--text-dim);font-size:10px;">0 lines</span>
    <button class="btn-icon" onclick="clearLog()" title="Clear log">✕</button>
  </div>
</div>

<div id="log-wrap"></div>

<!-- ── Settings panel ── -->
<div id="overlay" onclick="toggleSettings()"></div>
<div class="settings-panel" id="settings">
  <h3>⚙ SETTINGS</h3>
  <div class="field">
    <label>DEVICE NAME</label>
    <input id="cfg-name" maxlength="20" placeholder="NibBLEs">
  </div>
  <div class="field">
    <label>FACE</label>
    <input id="cfg-face" maxlength="20" placeholder="(◕‿◕)">
  </div>
  <div class="field">
    <label>WIFI SSID</label>
    <input id="cfg-ssid" maxlength="32" placeholder="GhostBLE">
  </div>
  <div class="field">
    <label>WIFI PASSWORD</label>
    <input id="cfg-pass" type="password" maxlength="63">
  </div>
  <button class="btn" onclick="saveConfig()">SAVE  →  REBOOT</button>
  <div class="cfg-status" id="cfg-status"></div>
</div>

<script>
  // ── State ──
  const MAX_DEVICES  = 50;
  const MAX_LOG_LINES = 200;

  let devices   = {};   // ghost_id → device object
  let logLines  = 0;
  let filterStr = '';
  let wsUrl     = `ws://${location.host}/ws`;
  let ws;

  // Stats
  let stats = { spt: 0, sus: 0, bcn: 0, pwn: 0 };

  // ── WebSocket ──
  function connect() {
    ws = new WebSocket(wsUrl);

    ws.onopen = () => {
      setWsStatus(true);
      ws.send('GET_CONFIG');
    };

    ws.onclose = () => {
      setWsStatus(false);
      setTimeout(connect, 3000);  // auto-reconnect
    };

    ws.onerror = () => ws.close();

    ws.onmessage = (event) => {
      let msg;
      try {
        msg = JSON.parse(event.data);
      } catch {
        // Plain text → treat as log line (backward compat with old firmware)
        addLogLine(event.data, 'system');
        return;
      }

      if (msg.type === 'device') {
        upsertDevice(msg.data);
      } else if (msg.type === 'log') {
        addLogLine(msg.data, classifyLog(msg.data));
      } else if (msg.type === 'stats') {
        updateStats(msg.data);
      } else if (msg.type === 'config') {
        applyConfig(msg.data);
      } else if (msg.data) {
        // Fallback: raw string in msg.data
        addLogLine(String(msg.data), 'system');
      }
    };
  }

  function setWsStatus(ok) {
    document.getElementById('ws-dot').className    = 'status-dot' + (ok ? ' active' : '');
    document.getElementById('ws-status').textContent = ok ? 'LIVE' : 'OFFLINE';
  }

  // ── Device rendering ──
  function rssiColor(rssi) {
    if (rssi >= -60) return { color: '#00ff88', bg: '#0a2a1a' };
    if (rssi >= -75) return { color: '#ffaa00', bg: '#1a1a0a' };
    return               { color: '#ff3366', bg: '#2a0a0a' };
  }

  function rssiPercent(rssi) {
    // Map -30 (100%) to -100 (0%)
    return Math.max(0, Math.min(100, ((rssi + 100) / 70) * 100));
  }

  function upsertDevice(d) {
    // Evict oldest if at capacity
    const ids = Object.keys(devices);
    if (ids.length >= MAX_DEVICES && !devices[d.ghost_id]) {
      delete devices[ids[0]];
    }

    d.ts = Date.now();
    devices[d.ghost_id] = d;
    renderDevices();
  }

  function renderDevices() {
    const container = document.getElementById('devices');
    const filtered  = Object.values(devices)
      .filter(d => !filterStr ||
        (d.name  && d.name.toLowerCase().includes(filterStr))  ||
        (d.mac   && d.mac.toLowerCase().includes(filterStr))   ||
        (d.ghost_id && d.ghost_id.toLowerCase().includes(filterStr))
      )
      .sort((a, b) => b.rssi - a.rssi);  // strongest signal first

    if (filtered.length === 0) {
      container.innerHTML = '<div class="empty-state">SCANNING FOR DEVICES</div>';
      return;
    }

    container.innerHTML = filtered.map(d => {
      const rc      = rssiColor(d.rssi);
      const pct     = rssiPercent(d.rssi);
      const name    = d.name || d.mac || 'Unknown';
      const ghostId = d.ghost_id || '—';
      const manuf   = d.manufacturer ? `<span style="color:var(--text-dim)">${esc(d.manufacturer)}</span> · ` : '';

      const tags = [
        d.connectable  ? '<span class="tag tag-connectable">CONN</span>' : '',
        d.is_beacon    ? '<span class="tag tag-beacon">iBeacon</span>'   : '',
        d.is_pwnbeacon ? '<span class="tag tag-pwn">PWN</span>'          : '',
        d.has_notify   ? '<span class="tag tag-notify">NOTIFY</span>'    : '',
        d.suspicious   ? '<span class="tag tag-sus">SUS</span>'          : '',
      ].join('');

      const accentColor = d.suspicious ? '#ff3366' : d.is_pwnbeacon ? '#cc44ff' : rc.color;

      return `<div class="device-card" style="--card-accent:${accentColor}">
        <div class="card-top">
          <div class="card-name" title="${esc(d.mac || '')}">${esc(name)}</div>
          <div class="card-rssi" style="--rssi-color:${rc.color};--rssi-bg:${rc.bg}">${d.rssi} dBm</div>
        </div>
        <div class="card-ghost">${manuf}Ghost <span>#${esc(ghostId)}</span></div>
        <div class="card-meta">${tags}</div>
        <div class="rssi-bar"><div class="rssi-bar-fill" style="width:${pct}%;--rssi-color:${rc.color}"></div></div>
      </div>`;
    }).join('');
  }

  function clearDevices() {
    devices = {};
    renderDevices();
  }

  function applyFilter() {
    filterStr = document.getElementById('filter-input').value.toLowerCase().trim();
    renderDevices();
  }

  // ── Log rendering ──
  function classifyLog(line) {
    const l = line.toLowerCase();
    if (l.includes('security') || l.includes('[high]') || l.includes('sus')) return 'security';
    if (l.includes('beacon') || l.includes('ibeacon') || l.includes('pwnbeacon')) return 'beacon';
    if (l.includes('gatt') || l.includes('connected') || l.includes('uuid')) return 'gatt';
    if (l.includes('notify') || l.includes('heart rate') || l.includes('temperature')) return 'notify';
    if (l.includes('privacy') || l.includes('exposure') || l.includes('tracking')) return 'privacy';
    if (l.includes('scan') || l.includes('device found') || l.includes('spotted')) return 'scan';
    return 'system';
  }

  function addLogLine(text, cls) {
    const wrap = document.getElementById('log-wrap');

    // Trim oldest lines if over limit
    while (wrap.children.length >= MAX_LOG_LINES) {
      wrap.removeChild(wrap.firstChild);
    }

    const now = new Date();
    const ts  = now.toTimeString().slice(0, 8);

    const div = document.createElement('div');
    div.className = 'log-line';
    div.innerHTML = `<span class="log-ts">${ts}</span><span class="log-text ${cls}">${esc(text)}</span>`;
    wrap.appendChild(div);

    // Auto-scroll if near bottom
    const threshold = 60;
    const atBottom  = wrap.scrollHeight - wrap.scrollTop - wrap.clientHeight < threshold;
    if (atBottom) wrap.scrollTop = wrap.scrollHeight;

    logLines++;
    document.getElementById('log-count').textContent = logLines + ' lines';
  }

  function clearLog() {
    document.getElementById('log-wrap').innerHTML = '';
    logLines = 0;
    document.getElementById('log-count').textContent = '0 lines';
  }

  // ── Stats ──
  function updateStats(data) {
    if (data.spotted  !== undefined) { stats.spt = data.spotted;   document.getElementById('stat-spt').textContent = stats.spt; }
    if (data.sus      !== undefined) { stats.sus = data.sus;       document.getElementById('stat-sus').textContent = stats.sus; }
    if (data.beacons  !== undefined) { stats.bcn = data.beacons;   document.getElementById('stat-bcn').textContent = stats.bcn; }
    if (data.pwn      !== undefined) { stats.pwn = data.pwn;       document.getElementById('stat-pwn').textContent = stats.pwn; }
  }

  // ── Settings ──
  function toggleSettings() {
    document.getElementById('settings').classList.toggle('open');
    document.getElementById('overlay').classList.toggle('visible');
  }

  function applyConfig(data) {
    if (data.name) document.getElementById('cfg-name').value = data.name;
    if (data.face) document.getElementById('cfg-face').value = data.face;
    if (data.ssid) document.getElementById('cfg-ssid').value = data.ssid;
  }

  function saveConfig() {
    const fields = [
      { id: 'cfg-name', key: 'SET_NAME' },
      { id: 'cfg-face', key: 'SET_FACE' },
      { id: 'cfg-ssid', key: 'SET_SSID' },
      { id: 'cfg-pass', key: 'SET_PASS' },
    ];

    fields.forEach(f => {
      const val = document.getElementById(f.id).value.trim();
      if (val) ws.send(`${f.key}:${val}`);
    });

    document.getElementById('cfg-status').textContent = 'Saved. Reboot to apply.';
  }

  // ── Utility ──
  function esc(str) {
    return String(str)
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;');
  }

  // ── Boot ──
  connect();
</script>
</body>
</html>
)rawliteral";

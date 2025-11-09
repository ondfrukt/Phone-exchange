'use strict';

document.addEventListener('DOMContentLoaded', () => {
  const $lines  = document.getElementById('lines');
  const $status = document.getElementById('status');

  // --- Debug UI ---
  const $dbgShk = document.getElementById('dbg-shk');
  const $dbgLm  = document.getElementById('dbg-lm');
  const $dbgWs  = document.getElementById('dbg-ws');
  const $dbgLa  = document.getElementById('dbg-la');
  const $dbgMt  = document.getElementById('dbg-mt');

  const $dbgBtn = document.getElementById('dbg-save');
  const $dbgMsg = document.getElementById('dbg-status');
  const $restartBtn = document.getElementById('dbg-restart');

  const dbgToggle = document.getElementById('dbg-toggle');
  const dbgBody = document.getElementById('dbg-body');

  // --- Console UI (receive-only) ---
  const $consoleLog = document.getElementById('console-log');

  // Bitmask representing active/inactive lines (0..255)
  let activeMask = 0;
  // Cache of line objects for quick lookup and rendering: [{id, status}, ...]
  let linesCache = [];

  // Client-side console buffer to limit DOM updates and keep scroll performant
  const consoleMaxLines = 500;
  const consoleLines = [];

  let autoScrollConsole = true;

  // Helpers
  const isActive  = (id) => ((activeMask >> id) & 1) === 1;
  const setStatus = (t) => { $status.textContent = t; };
  const setDbgMsg = (t) => { $dbgMsg.textContent = t; };
  let restartInProgress = false;

  // Update the visibility and content of the status cell for a given line
  // depending on whether the line is active. This keeps sensitive or
  // unneeded info hidden when the line is inactive.
  function updateStatusVisibility(lineId){
    const sCell = document.getElementById(`line-${lineId}-status`);
    if (!sCell) return;
    if (isActive(lineId)) {
      // Show cached status if available
      const cached = linesCache.find(x => x.id === lineId);
      sCell.textContent = cached ? cached.status : '';
      sCell.removeAttribute('aria-hidden');
    } else {
      // Hide content when inactive for accessibility and clarity
      sCell.textContent = '';
      sCell.setAttribute('aria-hidden', 'true');
    }
  }

  // Build or update a single line in the DOM, including the badge element,
  // ids and keyboard/click handlers. This is idempotent: calling with an
  // existing id will update the row instead of duplicating it.
  function upsertLine(lineId, status){
    let row = document.getElementById('line-'+lineId);
    if (!row){
      row = document.createElement('div');
      row.className = 'row';
      row.id = 'line-' + lineId;
      row.innerHTML = `
        <span class="k">Linje ${lineId}</span>
        <span class="v" id="line-${lineId}-status"></span>
        <span class="badge" id="line-${lineId}-active"
              role="button" tabindex="0"
              title="Activate / Inactivate"></span>
      `;
      $lines.appendChild(row);

      // Attach clicks and keyboard handling only to the badge element so the
      // rest of the row remains static and non-interactive.
      const aCell = row.querySelector(`#line-${lineId}-active`);
      aCell.classList.add('clickable');
      aCell.addEventListener('click', () => toggleLineActive(lineId, aCell));
      aCell.addEventListener('keydown', (e) => {
        if (e.key === 'Enter' || e.key === ' ') {
          e.preventDefault(); toggleLineActive(lineId, aCell);
        }
      });
    }

    // Update or insert to the in-memory cache for fast future updates
    const idx = linesCache.findIndex(x => x.id === lineId);
    if (idx >= 0) linesCache[idx].status = status;
    else linesCache.push({ id: lineId, status });

    // Status cell is shown only if the line is active
    updateStatusVisibility(lineId);

    // Update badge appearance based on current activeMask
    updateActiveCell(lineId);
  }

  // Update a single active-badge element's appearance and ARIA state
  // based on the activeMask. This keeps a consistent visual state.
  function updateActiveCell(lineId){
    const aCell = document.getElementById(`line-${lineId}-active`);
    if (!aCell) return;
    const a = isActive(lineId);
    aCell.textContent = a ? 'Active' : 'Inactive';
    aCell.classList.toggle('ok', a);
    aCell.classList.toggle('no', !a);
    aCell.setAttribute('aria-pressed', a ? 'true' : 'false');
  }

  // Render all lines (clears the container and rebuilds using upsertLine).
  // This is used for full snapshots from the server.
  function renderAll(lines){
    linesCache = lines.map(l => ({ id:l.id, status:l.status }));
    $lines.innerHTML = '';
    for (const l of linesCache) upsertLine(l.id, l.status);
  }

  // Toggle a line's active state via the API. The badge element receives a
  // 'working' class while the request is in progress to give user feedback.
  async function toggleLineActive(lineId, badgeEl){
    try{
      badgeEl.classList.add('working');
      const body = new URLSearchParams({ line:String(lineId) }).toString();
      const r = await fetch('/api/active/toggle', {
        method:'POST',
        headers:{ 'Content-Type':'application/x-www-form-urlencoded' },
        body
      });
      if (!r.ok) throw new Error('HTTP ' + r.status);
      const d = await r.json(); // {mask, active:[...]}
      if (typeof d.mask === 'number'){
        activeMask = d.mask|0;
        // Update all cached lines' badges and status visibility
        linesCache.forEach(l => { updateActiveCell(l.id); updateStatusVisibility(l.id); });
      }
    } catch(e){
      console.warn('toggleLineActive error', e);
      setStatus('Kunde inte uppdatera masken (se konsol).'); // UI message left as-is
    } finally {
      badgeEl.classList.remove('working');
    }
  }

  // Console: format timestamp and append a new console line to the client buffer.
  // We keep the DOM updates limited by maintaining consoleLines and writing the
  // combined textNode. This reduces reflow when many messages arrive.
  function formatTime(ts) {
    try {
      const dt = new Date(ts);
      return dt.toLocaleTimeString();
    } catch {
      return '';
    }
  }

  function appendConsoleLine(obj) {
    const time = obj.ts ? formatTime(obj.ts) : '';
    const src = obj.source ? `[${obj.source}] ` : '';
    const text = obj.text || '';
    const line = `${time} ${src}${text}`;

    consoleLines.push(line);
    if (consoleLines.length > consoleMaxLines) consoleLines.shift();

    // Single DOM write for the whole buffer
    $consoleLog.textContent = consoleLines.join('\n');

    if (autoScrollConsole) {
      // Keep console scrolled to bottom when user hasn't manually scrolled up
      $consoleLog.scrollTop = $consoleLog.scrollHeight;
    }
  }

  // Load debug levels from server on startup. These are the numeric debug
  // settings shown in the debug UI.
  async function loadDebug() {
    try{
      const r = await fetch('/api/debug');
      if (!r.ok) throw new Error('HTTP '+r.status);
      const d = await r.json(); // {shk,lm,ws,la,mt}
      if (typeof d.shk === 'number') $dbgShk.value = String(d.shk|0);
      if (typeof d.lm  === 'number') $dbgLm.value  = String(d.lm|0);
      if (typeof d.ws  === 'number') $dbgWs.value  = String(d.ws|0);
      if (typeof d.la  === 'number') $dbgLa.value  = String(d.la|0);
      if (typeof d.mt  === 'number') $dbgMt.value  = String(d.mt|0);
    } catch(e){
      setDbgMsg('Could not read debug levels');
      console.warn(e);
    }
  }

  // Persist debug levels to the server. UI shows a short working state.
  async function saveDebug() {
    try{
      $dbgBtn.classList.add('working');
      setDbgMsg('Sparar…'); // UI message left as-is
      const body = new URLSearchParams({
        shk: $dbgShk.value,
        lm: $dbgLm.value,
        ws: $dbgWs.value,
        la: $dbgLa.value,
        mt: $dbgMt.value
      }).toString();
      const r = await fetch('/api/debug/set', {
        method:'POST',
        headers:{ 'Content-Type':'application/x-www-form-urlencoded' },
        body
      });
      if (!r.ok) throw new Error('HTTP '+r.status);
      await r.json();
      setDbgMsg('Sparat.');
    } catch(e){
      setDbgMsg('Misslyckades att spara.');
      console.warn(e);
    } finally {
      $dbgBtn.classList.remove('working');
      setTimeout(()=> setDbgMsg(''), 1500);
    }
  }

  // Restart the device via API with a confirmation prompt. We set a local
  // flag while a restart is in progress so the UI can reflect that if needed.
  async function restartDevice() {
    if (!confirm('Are you sure you want to restart the device?')) return;
    try{
      const r = await fetch('/api/restart', { method:'POST' });
      if (!r.ok) throw new Error('HTTP '+r.status);
      restartInProgress = true;
      setDbgMsg('Restarting…');
    } catch(e){
      restartInProgress = false;
      setStatus('Kunde inte starta om enheten (se konsol).'); // UI message left as-is
      console.warn(e);
    }
  }

  $dbgBtn?.addEventListener('click', saveDebug);
  $restartBtn?.addEventListener('click', restartDevice);

  // Toggle debug UI visibility and load current debug values when opened.
  dbgToggle?.addEventListener('click', () => {
    if (!dbgBody) return;
    const opened = dbgBody.style.display !== 'none';
    if (opened) {
      dbgBody.style.display = 'none';
      dbgToggle.textContent = 'Show';
      dbgToggle.setAttribute('aria-expanded', 'false');
    } else {
      dbgBody.style.display = '';
      dbgToggle.textContent = 'Hide';
      dbgToggle.setAttribute('aria-expanded', 'true');
      loadDebug();
    }
  });

  // Track manual scroll to disable auto-scroll when user scrolls up.
  $consoleLog?.addEventListener('scroll', () => {
    if (!$consoleLog) return;
    const nearBottom = ($consoleLog.scrollTop + $consoleLog.clientHeight) >= ($consoleLog.scrollHeight - 20);
    autoScrollConsole = nearBottom;
  });

  // ---- Initial load ----
  // 1) Full status list (snapshot)
  fetch('/api/status')
    .then(r => r.json())
    .then(d => { if (d && Array.isArray(d.lines)) renderAll(d.lines); })
    .catch(()=>{});

  // 2) Current activeMask
  fetch('/api/active')
    .then(r => r.json())
    .then(d => {
      if (d && typeof d.mask === 'number') {
        activeMask = d.mask|0;
        linesCache.forEach(l => { updateActiveCell(l.id); updateStatusVisibility(l.id); });
      }
    })
    .catch(()=>{});

  // ---- SSE (Server-Sent Events) ----
  // Persistent connection used to receive live updates from the server.
  const es = new EventSource('/events');
  es.onopen  = () => setStatus('Live: Connected');
  es.onerror = () => setStatus('Live: Problem with the connection!');

  // Generic full snapshot message (fallback) — payload: { lines:[...] }
  // Using onmessage for non-named events.
  es.onmessage = ev => {
    try {
      const d = JSON.parse(ev.data);
      if (d && Array.isArray(d.lines)) {
        linesCache = d.lines.map(l => ({ id:l.id, status:l.status }));
        for (const l of linesCache) upsertLine(l.id, l.status);
      }
    } catch {}
  };

  // Named SSE event for a single line status delta — payload: { line, status }
  es.addEventListener('lineStatus', ev => {
    try {
      const d = JSON.parse(ev.data);
      if (!('line' in d) || !('status' in d)) return;
      if (typeof d.line !== 'number' || typeof d.status !== 'string') return;
      const i = linesCache.findIndex(x => x.id === d.line);
      if (i >= 0) linesCache[i].status = d.status;
      else linesCache.push({ id:d.line, status:d.status });
      upsertLine(d.line, d.status);
    } catch {}
  });

  // Named SSE event for active mask updates — payload: { mask }
  es.addEventListener('activeMask', ev => {
    try{
      const d = JSON.parse(ev.data);
      if (typeof d.mask === 'number'){
        activeMask = d.mask|0;
        linesCache.forEach(l => { updateActiveCell(l.id); updateStatusVisibility(l.id); });
      }
    } catch {}
  });

  // Debug levels updated via SSE — keeps UI in sync with device
  es.addEventListener('debug', ev => {
    try{
      const d = JSON.parse(ev.data);
      if (typeof d.shk === 'number') $dbgShk.value = String(d.shk|0);
      if (typeof d.lm  === 'number') $dbgLm.value  = String(d.lm|0);
      if (typeof d.ws  === 'number') $dbgWs.value  = String(d.ws|0);
      if (typeof d.la  === 'number') $dbgLa.value  = String(d.la|0);
      if (typeof d.mt  === 'number') $dbgMt.value  = String(d.mt|0);
    } catch {}
  });

  // Console SSE (receive-only stream of log-like messages)
  es.addEventListener('console', ev => {
    try {
      const d = JSON.parse(ev.data);
      appendConsoleLine(d);
    } catch (e) {
      console.warn('Invalid console SSE', e);
    }
  });

  window.addEventListener('beforeunload', () => { try { es.close(); } catch {} });

  // Load debug levels at startup
  loadDebug();
});
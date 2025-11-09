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
  // Cache of line objects for quick lookup and rendering: [{id, status, phone}, ...]
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

  function updatePhoneInput(entry){
    if (!entry) return;
    const input = document.getElementById(`line-${entry.id}-phone`);
    if (!input) return;
    if (document.activeElement === input) return;
    input.value = entry.phone || '';
  }

  // Build or update a single line entry in cache and DOM.
  function upsertLine(line){
    if (!line) return;
    const lineId = typeof line.id === 'number' ? (line.id | 0) : 0;

    let entry = linesCache.find(x => x.id === lineId);
    if (!entry) {
      entry = { id: lineId, status: '', phone: '' };
      linesCache.push(entry);
    }

    if (typeof line.status === 'string') entry.status = line.status;
    if (typeof line.phone === 'string') entry.phone = line.phone;

    let row = document.getElementById('line-' + lineId);
    if (!row) {
      row = document.createElement('div');
      row.className = 'row';
      row.id = 'line-' + lineId;
      row.innerHTML = `
        <span class="k">Linje ${lineId}</span>
        <span class="v" id="line-${lineId}-status"></span>
        <div class="phone-editor">
          <input type="text" id="line-${lineId}-phone" inputmode="tel" autocomplete="tel"
                 placeholder="Phone number" maxlength="32" />
          <button class="badge clickable" id="line-${lineId}-save" type="button">Spara</button>
        </div>
        <button class="badge" id="line-${lineId}-active" type="button" title="Activate / Inactivate"></button>
      `;
      $lines.appendChild(row);

      const aCell = row.querySelector(`#line-${lineId}-active`);
      if (aCell) {
        aCell.classList.add('clickable');
        aCell.addEventListener('click', () => toggleLineActive(lineId, aCell));
      }

      const saveBtn = row.querySelector(`#line-${lineId}-save`);
      const phoneInput = row.querySelector(`#line-${lineId}-phone`);
      if (saveBtn) {
        saveBtn.addEventListener('click', () => persistPhoneNumber(lineId, saveBtn));
      }
      if (phoneInput) {
        phoneInput.addEventListener('keydown', (e) => {
          if (e.key === 'Enter') {
            e.preventDefault();
            persistPhoneNumber(lineId, saveBtn);
          }
        });
      }
    }

    updateStatusVisibility(lineId);
    updatePhoneInput(entry);
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
    linesCache = [];
    $lines.innerHTML = '';
    if (!Array.isArray(lines)) return;
    for (const l of lines) upsertLine(l);
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

  async function persistPhoneNumber(lineId, buttonEl){
    const input = document.getElementById(`line-${lineId}-phone`);
    if (!input) return;
    const btn = buttonEl || document.getElementById(`line-${lineId}-save`);
    const originalText = btn ? btn.textContent : '';

    const value = input.value.trim();
    if (value.length > 32) {
      if (btn) {
        btn.textContent = 'För långt';
        setTimeout(() => { if (btn) btn.textContent = originalText; }, 2000);
      }
      setStatus('Telefonnumret är för långt (max 32 tecken).');
      return;
    }

    try{
      if (btn) {
        btn.classList.add('working');
        btn.disabled = true;
      }

      const body = new URLSearchParams({
        line: String(lineId),
        phone: value
      }).toString();

      const r = await fetch('/api/line/phone', {
        method:'POST',
        headers:{ 'Content-Type':'application/x-www-form-urlencoded' },
        body
      });

      if (!r.ok) {
        let msg = 'HTTP ' + r.status;
        try {
          const data = await r.json();
          if (data && typeof data.error === 'string') msg = data.error;
        } catch {}
        throw new Error(msg);
      }

      const entry = linesCache.find(x => x.id === lineId);
      if (entry) entry.phone = value;
      if (btn) {
        btn.textContent = 'Sparat';
        setTimeout(() => { if (btn) btn.textContent = originalText; }, 1500);
      }
    } catch(e){
      console.warn('persistPhoneNumber error', e);
      let friendly = 'Kunde inte spara telefonnumret.';
      if (e && typeof e.message === 'string') {
        if (e.message.includes('phone too long')) friendly = 'Telefonnumret är för långt (max 32 tecken).';
        else if (e.message.includes('invalid characters')) friendly = 'Telefonnumret innehåller ogiltiga tecken.';
        else if (!e.message.startsWith('HTTP')) friendly += ` (${e.message})`;
      }
      setStatus(friendly);
      if (btn) {
        btn.textContent = 'Fel';
        setTimeout(() => { if (btn) btn.textContent = originalText; }, 2000);
      }
    } finally {
      if (btn) {
        btn.classList.remove('working');
        btn.disabled = false;
      }
      const entry = linesCache.find(x => x.id === lineId);
      updatePhoneInput(entry);
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
      if (d && Array.isArray(d.lines)) renderAll(d.lines);
    } catch {}
  };

  // Named SSE event for a single line status delta — payload: { line, status }
  es.addEventListener('lineStatus', ev => {
    try {
      const d = JSON.parse(ev.data);
      if (!('line' in d) || !('status' in d)) return;
      if (typeof d.line !== 'number' || typeof d.status !== 'string') return;
      upsertLine({ id: d.line, status: d.status });
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
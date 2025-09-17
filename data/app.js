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

  let activeMask = 0;                 // bitmask 0..255
  let linesCache = [];                // [{id, status}]

  const isActive  = (id) => ((activeMask >> id) & 1) === 1;
  const setStatus = (t) => { $status.textContent = t; };
  const setDbgMsg = (t) => { $dbgMsg.textContent = t; };

  // Bygg/uppdatera EN rad, inkl. badge, IDs och handlers
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

      // koppla klick endast på badgen
      const aCell = row.querySelector(`#line-${lineId}-active`);
      aCell.classList.add('clickable');
      aCell.addEventListener('click', () => toggleLineActive(lineId, aCell));
      aCell.addEventListener('keydown', (e) => {
        if (e.key === 'Enter' || e.key === ' ') {
          e.preventDefault(); toggleLineActive(lineId, aCell);
        }
      });
    }

    // statuscell
    const sCell = document.getElementById(`line-${lineId}-status`);
    if (sCell) sCell.textContent = status;

    // badge enligt mask
    updateActiveCell(lineId);
  }

  // Uppdatera EN aktiv-badge utifrån activeMask (endast EN version)
  function updateActiveCell(lineId){
    const aCell = document.getElementById(`line-${lineId}-active`);
    if (!aCell) return;
    const a = isActive(lineId);
    aCell.textContent = a ? 'Active' : 'Inactive';
    aCell.classList.toggle('ok', a);
    aCell.classList.toggle('no', !a);
    aCell.setAttribute('aria-pressed', a ? 'true' : 'false');
  }

  // Rendera alla (rensar och bygger via upsertLine)
  function renderAll(lines){
    linesCache = lines.map(l => ({ id:l.id, status:l.status }));
    $lines.innerHTML = '';
    for (const l of linesCache) upsertLine(l.id, l.status);
  }

  // Toggle via API
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
        linesCache.forEach(l => updateActiveCell(l.id));
      }
      // SSE "activeMask" uppdaterar dessutom andra klienter
    } catch(e){
      console.warn('toggleLineActive error', e);
      setStatus('Kunde inte uppdatera masken (se konsol).');
    } finally {
      badgeEl.classList.remove('working');
    }
  }

  async function loadDebug() {
    try{
      const r = await fetch('/api/debug');
      if (!r.ok) throw new Error('HTTP '+r.status);
      const d = await r.json(); // {shk,lm,ws}
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

  async function saveDebug() {
    try{
      $dbgBtn.classList.add('working');
      setDbgMsg('Sparar…');
      const body = new URLSearchParams({
        shk: $dbgShk.value, lm: $dbgLm.value, ws: $dbgWs.value
      }).toString();
      const r = await fetch('/api/debug/set', {
        method:'POST',
        headers:{ 'Content-Type':'application/x-www-form-urlencoded' },
        body
      });
      if (!r.ok) throw new Error('HTTP '+r.status);
      await r.json();
      // UI kommer dessutom att uppdateras av SSE "debug"
      setDbgMsg('Sparat.');
    } catch(e){
      setDbgMsg('Misslyckades att spara.');
      console.warn(e);
    } finally {
      $dbgBtn.classList.remove('working');
      setTimeout(()=> setDbgMsg(''), 1500);
    }
  }

  async function restartDevice() {
    if (!confirm('Are you sure you want to restart the device?')) return;
    try{
      const r = await fetch('/api/restart', { method:'POST' });
      if (!r.ok) throw new Error('HTTP '+r.status);
      await r.json();
    } catch(e){
      setStatus('Kunde inte starta om enheten (se konsol).');
      console.warn(e);
    }
  }

  async function restartDevice() {
    if (!confirm('Are you sure you want to restart the device?')) return;
    try{
      const r = await fetch('/api/restart', { method:'POST' });
      if (!r.ok) throw new Error('HTTP '+r.status);
      // Svara gärna i UI
      setDbgMsg('Restarting…');
      // Efter detta tappar sidan kontakt när ESP32 startar om
    } catch(e){
      setStatus('Kunde inte starta om enheten (se konsol).');
      console.warn(e);
    }
  }

  $dbgBtn?.addEventListener('click', saveDebug);
  $restartBtn?.addEventListener('click', restartDevice);

  // ---- Initial inläsning ----
  // 1) Full statuslista
  fetch('/api/status')
    .then(r => r.json())
    .then(d => { if (d && Array.isArray(d.lines)) renderAll(d.lines); })
    .catch(()=>{});

  // 2) Aktuell activeMask
  fetch('/api/active')
    .then(r => r.json())
    .then(d => {
      if (d && typeof d.mask === 'number') {
        activeMask = d.mask|0;
        linesCache.forEach(l => updateActiveCell(l.id));
      }
    })
    .catch(()=>{});

  // ---- SSE (Server-Sent Events) ----
  const es = new EventSource('/events');
  es.onopen  = () => setStatus('Live: Connected');
  es.onerror = () => setStatus('Live: Problem with the connection!');

  // Full snapshot: { lines:[...] }
  es.onmessage = ev => {
    try {
      const d = JSON.parse(ev.data);
      if (d && Array.isArray(d.lines)) {
        linesCache = d.lines.map(l => ({ id:l.id, status:l.status }));
        for (const l of linesCache) upsertLine(l.id, l.status);
      }
    } catch {}
  };

  // Delta: { line, status }
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

  // Delta: { mask }
  es.addEventListener('activeMask', ev => {
    try{
      const d = JSON.parse(ev.data);
      if (typeof d.mask === 'number'){
        activeMask = d.mask|0;
        linesCache.forEach(l => updateActiveCell(l.id));
      }
    } catch {}
  });

  // Debug-nivåer via SSE
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

  window.addEventListener('beforeunload', () => { try { es.close(); } catch {} });

  // Läs in debugnivåer vid start
  loadDebug();
});
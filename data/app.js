'use strict';

document.addEventListener('DOMContentLoaded', () => {
  const $lines  = document.getElementById('lines');
  const $status = document.getElementById('status');

  // Bitmask representing active/inactive lines (0..255)
  let activeMask = 0;
  // Cache of line objects for quick lookup and rendering: [{id, status, phone, name}, ...]
  let linesCache = [];

  // Helpers
  const isActive  = (id) => ((activeMask >> id) & 1) === 1;
  const setStatus = (t) => { $status.textContent = t; };

  // Update the visibility and content of the status cell for a given line
  // depending on whether the line is active. This keeps sensitive or
  // unneeded info hidden when the line is inactive.
  function updateStatusVisibility(lineId){
    const sCell = document.getElementById(`line-${lineId}-status`);
    const row = document.getElementById(`line-${lineId}`);
    const phoneEditor = row ? row.querySelector('.phone-editor') : null;
    const phoneInput = phoneEditor ? phoneEditor.querySelector(`#line-${lineId}-phone`) : null;
    const saveBtn = phoneEditor ? phoneEditor.querySelector(`#line-${lineId}-save`) : null;

    // Status cell show/hide (som tidigare)
    if (sCell) {
      if (isActive(lineId)) {
        const cached = linesCache.find(x => x.id === lineId);
        sCell.textContent = cached ? cached.status : '';
        sCell.removeAttribute('aria-hidden');
      } else {
        sCell.textContent = '';
        sCell.setAttribute('aria-hidden', 'true');
      }
    }

    // Dölj eller visa phone-editor när linjen är inaktiv/aktiv
    // (använd .hidden-klass i stället för display:none så att layouten behåller sin höjd)
    if (phoneEditor) {
      if (isActive(lineId)) {
        phoneEditor.classList.remove('hidden');
        phoneEditor.removeAttribute('aria-hidden');
        if (saveBtn) saveBtn.disabled = false;
        if (phoneInput) phoneInput.removeAttribute('aria-hidden');
      } else {
        phoneEditor.classList.add('hidden');
        phoneEditor.setAttribute('aria-hidden', 'true');
        if (saveBtn) saveBtn.disabled = true;
        if (phoneInput) phoneInput.setAttribute('aria-hidden', 'true');
      }
    }
  }

  function updatePhoneInput(entry){
    if (!entry) return;
    const input = document.getElementById(`line-${entry.id}-phone`);
    if (!input) return;
    if (document.activeElement === input) return;
    input.value = entry.phone || '';
  }

  function updateNameInput(entry){
    if (!entry) return;
    const input = document.getElementById(`line-${entry.id}-name`);
    if (!input) return;
    if (document.activeElement === input) return;
    input.value = entry.name || '';
  }

  function updateLineLabel(entry){
    if (!entry) return;
    const label = document.getElementById(`line-${entry.id}-label`);
    if (!label) return;
    const cleanName = (entry.name || '').trim();
    label.textContent = cleanName.length > 0 ? `${cleanName} (Linje ${entry.id})` : `Linje ${entry.id}`;
  }

  // Build or update a single line entry in cache and DOM.
  function upsertLine(line){
    if (!line) return;
    const lineId = typeof line.id === 'number' ? (line.id | 0) : 0;

    let entry = linesCache.find(x => x.id === lineId);
    if (!entry) {
      entry = { id: lineId, status: '', phone: '', name: '' };
      linesCache.push(entry);
    }

    if (typeof line.status === 'string') entry.status = line.status;
    if (typeof line.phone === 'string') entry.phone = line.phone.trim();
    if (typeof line.name === 'string') entry.name = line.name.trim();

    let row = document.getElementById('line-' + lineId);
    if (!row) {
      row = document.createElement('div');
      row.className = 'row';
      row.id = 'line-' + lineId;
      row.innerHTML = `
        <div class="k line-col">
          <span class="line-label" id="line-${lineId}-label">Linje ${lineId}</span>
          <div class="name-editor">
            <input type="text" id="line-${lineId}-name" autocomplete="off"
                   placeholder="Name" maxlength="32" />
            <button class="btn btn-primary btn-sm" id="line-${lineId}-name-save" type="button">Save Name</button>
          </div>
        </div>
        <span class="v" id="line-${lineId}-status"></span>
        <div class="phone-editor">
          <input type="text" id="line-${lineId}-phone" inputmode="tel" autocomplete="tel"
                 placeholder="Phone number" maxlength="32" />
          <button class="btn btn-primary btn-sm" id="line-${lineId}-save" type="button">Save</button>
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
      const nameSaveBtn = row.querySelector(`#line-${lineId}-name-save`);
      const nameInput = row.querySelector(`#line-${lineId}-name`);
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
      if (nameSaveBtn) {
        nameSaveBtn.addEventListener('click', () => persistLineName(lineId, nameSaveBtn));
      }
      if (nameInput) {
        nameInput.addEventListener('keydown', (e) => {
          if (e.key === 'Enter') {
            e.preventDefault();
            persistLineName(lineId, nameSaveBtn);
          }
        });
      }
    }

    updateStatusVisibility(lineId);
    updatePhoneInput(entry);
    updateNameInput(entry);
    updateLineLabel(entry);
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

    if (value.length > 0) {
      const duplicate = linesCache.some(entry => {
        if (!entry || entry.id === lineId) return false;
        if (typeof entry.phone !== 'string') return false;
        const current = entry.phone.trim();
        return current.length > 0 && current === value;
      });
      if (duplicate) {
        setStatus('Telefonnumret används redan av en annan linje.');
        if (btn) {
          btn.textContent = 'Dublett';
          setTimeout(() => { if (btn) btn.textContent = originalText; }, 2000);
        }
        return;
      }
    }
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
        else if (e.message.includes('phone already in use')) friendly = 'Telefonnumret används redan av en annan linje.';
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

  async function persistLineName(lineId, buttonEl){
    const input = document.getElementById(`line-${lineId}-name`);
    if (!input) return;
    const btn = buttonEl || document.getElementById(`line-${lineId}-name-save`);
    const originalText = btn ? btn.textContent : '';

    const value = input.value.trim();
    if (value.length > 32) {
      if (btn) {
        btn.textContent = 'Too long';
        setTimeout(() => { if (btn) btn.textContent = originalText; }, 2000);
      }
      setStatus('Namnet är för långt (max 32 tecken).');
      return;
    }

    try {
      if (btn) {
        btn.classList.add('working');
        btn.disabled = true;
      }

      const body = new URLSearchParams({
        line: String(lineId),
        name: value
      }).toString();

      const r = await fetch('/api/line/name', {
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
      if (entry) {
        entry.name = value;
        updateLineLabel(entry);
      }
      if (btn) {
        btn.textContent = 'Saved';
        setTimeout(() => { if (btn) btn.textContent = originalText; }, 1500);
      }
    } catch (e) {
      console.warn('persistLineName error', e);
      let friendly = 'Kunde inte spara namnet.';
      if (e && typeof e.message === 'string') {
        if (e.message.includes('name too long')) friendly = 'Namnet är för långt (max 32 tecken).';
        else if (e.message.includes('invalid characters')) friendly = 'Namnet innehåller ogiltiga tecken.';
        else if (!e.message.startsWith('HTTP')) friendly += ` (${e.message})`;
      }
      setStatus(friendly);
      if (btn) {
        btn.textContent = 'Error';
        setTimeout(() => { if (btn) btn.textContent = originalText; }, 2000);
      }
    } finally {
      if (btn) {
        btn.classList.remove('working');
        btn.disabled = false;
      }
      const entry = linesCache.find(x => x.id === lineId);
      updateNameInput(entry);
      updateLineLabel(entry);
    }
  }

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

  window.addEventListener('beforeunload', () => { try { es.close(); } catch {} });
});

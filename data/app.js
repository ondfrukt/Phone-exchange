'use strict';

document.addEventListener('DOMContentLoaded', () => {
  const $lines  = document.getElementById('lines');
  const $status = document.getElementById('status');

  let activeMask = 0;
  let linesCache = [];
  let statusTimer = null;

  const isActive  = (id) => ((activeMask >> id) & 1) === 1;
  const clearStatus = () => {
    if (statusTimer) {
      clearTimeout(statusTimer);
      statusTimer = null;
    }
    if ($status) $status.textContent = '';
  };
  const setStatus = (t) => { if ($status) $status.textContent = t || ''; };
  const showError = (msg) => {
    setStatus(msg);
    if (statusTimer) clearTimeout(statusTimer);
    statusTimer = setTimeout(() => {
      if ($status && $status.textContent === msg) $status.textContent = '';
      statusTimer = null;
    }, 6000);
  };

  function renderHeader() {
    if (!$lines) return;
    const existing = document.getElementById('line-header');
    if (existing) return;

    const head = document.createElement('div');
    head.className = 'row row-head';
    head.id = 'line-header';
    head.innerHTML = `
      <span class="col-head col-head-center">Line</span>
      <span class="col-head col-head-left">Name</span>
      <span class="col-head col-head-left">Phone</span>
      <span class="col-head col-head-center">Status</span>
      <span class="col-head col-head-center">Active</span>
    `;
    $lines.appendChild(head);
  }

  function updateStatusVisibility(lineId){
    const sCell = document.getElementById(`line-${lineId}-status`);
    if (!sCell) return;

    if (isActive(lineId)) {
      const cached = linesCache.find(x => x.id === lineId);
      sCell.textContent = cached ? cached.status : '';
      sCell.removeAttribute('aria-hidden');
    } else {
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

  function updateNameInput(entry){
    if (!entry) return;
    const input = document.getElementById(`line-${entry.id}-name`);
    if (!input) return;
    if (document.activeElement === input) return;
    input.value = entry.name || '';
  }

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
        <span class="line-id" id="line-${lineId}-id">${lineId}</span>
        <div class="name-editor">
          <input type="text" id="line-${lineId}-name" autocomplete="off"
                 placeholder="Name" maxlength="32" />
        </div>
        <div class="phone-editor">
          <input type="text" id="line-${lineId}-phone" inputmode="tel" autocomplete="tel"
                 placeholder="Phone number" maxlength="32" />
        </div>
        <span class="v" id="line-${lineId}-status"></span>
        <button class="badge" id="line-${lineId}-active" type="button" title="Activate / Inactivate"></button>
      `;
      $lines.appendChild(row);

      const aCell = row.querySelector(`#line-${lineId}-active`);
      if (aCell) {
        aCell.classList.add('clickable');
        aCell.addEventListener('click', () => toggleLineActive(lineId, aCell));
      }

      const phoneInput = row.querySelector(`#line-${lineId}-phone`);
      const nameInput = row.querySelector(`#line-${lineId}-name`);

      if (phoneInput) {
        phoneInput.addEventListener('blur', () => { void persistPhoneNumber(lineId); });
        phoneInput.addEventListener('keydown', (e) => {
          if (e.key === 'Enter') {
            e.preventDefault();
            phoneInput.blur();
          }
        });
      }

      if (nameInput) {
        nameInput.addEventListener('blur', () => { void persistLineName(lineId); });
        nameInput.addEventListener('keydown', (e) => {
          if (e.key === 'Enter') {
            e.preventDefault();
            nameInput.blur();
          }
        });
      }
    }

    updateStatusVisibility(lineId);
    updatePhoneInput(entry);
    updateNameInput(entry);
    updateActiveCell(lineId);
  }

  function updateActiveCell(lineId){
    const aCell = document.getElementById(`line-${lineId}-active`);
    if (!aCell) return;
    const a = isActive(lineId);
    aCell.textContent = a ? 'Active' : 'Inactive';
    aCell.classList.toggle('ok', a);
    aCell.classList.toggle('no', !a);
    aCell.setAttribute('aria-pressed', a ? 'true' : 'false');
  }

  function renderAll(lines){
    linesCache = [];
    $lines.innerHTML = '';
    renderHeader();
    if (!Array.isArray(lines)) return;
    for (const l of lines) upsertLine(l);
  }

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
      const d = await r.json();
      if (typeof d.mask === 'number'){
        activeMask = d.mask|0;
        linesCache.forEach(l => { updateActiveCell(l.id); updateStatusVisibility(l.id); });
      }
      clearStatus();
    } catch(e){
      console.warn('toggleLineActive error', e);
      showError('Kunde inte uppdatera linjens aktiv-status.');
    } finally {
      badgeEl.classList.remove('working');
    }
  }

  async function persistPhoneNumber(lineId){
    const input = document.getElementById(`line-${lineId}-phone`);
    if (!input) return;

    const entry = linesCache.find(x => x.id === lineId);
    const current = entry && typeof entry.phone === 'string' ? entry.phone.trim() : '';
    const value = input.value.trim();

    if (value === current) return;

    if (value.length > 0) {
      const duplicate = linesCache.some(item => {
        if (!item || item.id === lineId) return false;
        if (typeof item.phone !== 'string') return false;
        const existing = item.phone.trim();
        return existing.length > 0 && existing === value;
      });
      if (duplicate) {
        showError('Telefonnumret anvands redan av en annan linje.');
        input.value = current;
        return;
      }
    }

    if (value.length > 32) {
      showError('Telefonnumret ar for langt (max 32 tecken).');
      input.value = current;
      return;
    }

    try{
      input.disabled = true;
      const body = new URLSearchParams({ line: String(lineId), phone: value }).toString();

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

      if (entry) entry.phone = value;
      clearStatus();
    } catch(e){
      console.warn('persistPhoneNumber error', e);
      let friendly = 'Kunde inte spara telefonnumret.';
      if (e && typeof e.message === 'string') {
        if (e.message.includes('phone too long')) friendly = 'Telefonnumret ar for langt (max 32 tecken).';
        else if (e.message.includes('invalid characters')) friendly = 'Telefonnumret innehaller ogiltiga tecken.';
        else if (e.message.includes('phone already in use')) friendly = 'Telefonnumret anvands redan av en annan linje.';
      }
      showError(friendly);
      input.value = current;
    } finally {
      input.disabled = false;
      updatePhoneInput(entry);
    }
  }

  async function persistLineName(lineId){
    const input = document.getElementById(`line-${lineId}-name`);
    if (!input) return;

    const entry = linesCache.find(x => x.id === lineId);
    const current = entry && typeof entry.name === 'string' ? entry.name.trim() : '';
    const value = input.value.trim();

    if (value === current) return;

    if (value.length > 32) {
      showError('Namnet ar for langt (max 32 tecken).');
      input.value = current;
      return;
    }

    try {
      input.disabled = true;
      const body = new URLSearchParams({ line: String(lineId), name: value }).toString();

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

      if (entry) entry.name = value;
      clearStatus();
    } catch (e) {
      console.warn('persistLineName error', e);
      let friendly = 'Kunde inte spara namnet.';
      if (e && typeof e.message === 'string') {
        if (e.message.includes('name too long')) friendly = 'Namnet ar for langt (max 32 tecken).';
        else if (e.message.includes('invalid characters')) friendly = 'Namnet innehaller ogiltiga tecken.';
      }
      showError(friendly);
      input.value = current;
    } finally {
      input.disabled = false;
      updateNameInput(entry);
    }
  }

  fetch('/api/status')
    .then(r => r.json())
    .then(d => { if (d && Array.isArray(d.lines)) renderAll(d.lines); })
    .catch(()=>{});

  fetch('/api/active')
    .then(r => r.json())
    .then(d => {
      if (d && typeof d.mask === 'number') {
        activeMask = d.mask|0;
        linesCache.forEach(l => { updateActiveCell(l.id); updateStatusVisibility(l.id); });
      }
    })
    .catch(()=>{});

  const es = new EventSource('/events');
  es.onopen = () => {};
  es.onerror = () => {};

  es.onmessage = ev => {
    try {
      const d = JSON.parse(ev.data);
      if (d && Array.isArray(d.lines)) renderAll(d.lines);
    } catch {}
  };

  es.addEventListener('lineStatus', ev => {
    try {
      const d = JSON.parse(ev.data);
      if (!('line' in d) || !('status' in d)) return;
      if (typeof d.line !== 'number' || typeof d.status !== 'string') return;
      upsertLine({ id: d.line, status: d.status });
    } catch {}
  });

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

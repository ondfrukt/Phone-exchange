'use strict';

document.addEventListener('DOMContentLoaded', () => {
  const $lines = document.getElementById('lines');
  const $status = document.getElementById('status');
  const $connectionsView = document.getElementById('connections-view');

  let activeMask = 0;
  let linesCache = [];

  const isActive = (id) => ((activeMask >> id) & 1) === 1;
  const setStatus = (t) => { if ($status) $status.textContent = t || ''; };

  function applyStatusPadStyle(el, statusText) {
    if (!el) return;
    const status = (statusText || '').trim();
    const key = status.toLowerCase();
    el.textContent = status;
    el.className = 'status-pad';

    if (key === 'idle' || key === 'abandoned') return void el.classList.add('state-idle');
    if (key === 'connected') return void el.classList.add('state-connected');
    if (key === 'incoming' || key === 'ringing') return void el.classList.add('state-ringing');
    if (key === 'ready' || key === 'dialing' || key === 'pulsedialing' || key === 'tonedialing') return void el.classList.add('state-ready');
    if (key === 'busy' || key === 'fail' || key === 'timeout' || key === 'disconnected') return void el.classList.add('state-alert');
    el.classList.add('state-other');
  }

  function updateConnectionsView() {
    if (!$connectionsView) return;

    const pairs = new Set();
    for (const line of linesCache) {
      if (!line || typeof line.id !== 'number') continue;
      if (line.status !== 'Connected') continue;
      if (typeof line.outgoingTo !== 'number' || line.outgoingTo < 0 || line.outgoingTo > 7) continue;

      const a = Math.min(line.id, line.outgoingTo);
      const b = Math.max(line.id, line.outgoingTo);
      if (a === b) continue;
      pairs.add(`${a}-${b}`);
    }

    if (pairs.size === 0) {
      $connectionsView.textContent = 'No active connections';
      return;
    }

    $connectionsView.textContent = Array.from(pairs)
      .sort()
      .map((p) => {
        const [a, b] = p.split('-');
        return `Line ${a} <-> Line ${b}`;
      })
      .join('  |  ');
  }

  function updateStatusVisibility(lineId) {
    const sCell = document.getElementById(`line-${lineId}-status`);
    const row = document.getElementById(`line-${lineId}`);
    const editors = row ? row.querySelector('.line-editors') : null;

    if (sCell) {
      if (isActive(lineId)) {
        const cached = linesCache.find((x) => x.id === lineId);
        applyStatusPadStyle(sCell, cached ? cached.status : '');
        sCell.removeAttribute('aria-hidden');
      } else {
        sCell.textContent = '';
        sCell.className = 'status-pad hidden';
        sCell.setAttribute('aria-hidden', 'true');
      }
    }

    if (editors) {
      if (isActive(lineId)) {
        editors.classList.remove('hidden');
        editors.removeAttribute('aria-hidden');
      } else {
        editors.classList.add('hidden');
        editors.setAttribute('aria-hidden', 'true');
      }
    }
  }

  function updatePhoneInput(entry) {
    if (!entry) return;
    const input = document.getElementById(`line-${entry.id}-phone`);
    if (!input || document.activeElement === input) return;
    input.value = entry.phone || '';
  }

  function updateNameInput(entry) {
    if (!entry) return;
    const input = document.getElementById(`line-${entry.id}-name`);
    if (!input || document.activeElement === input) return;
    input.value = entry.name || '';
  }

  function updateLineLabel(entry) {
    if (!entry) return;
    const label = document.getElementById(`line-${entry.id}-label`);
    if (!label) return;
    const cleanName = (entry.name || '').trim();
    label.textContent = cleanName.length > 0 ? `${cleanName} (Line ${entry.id})` : `Line ${entry.id}`;
  }

  function isDuplicatePhone(lineId, value) {
    const normalized = (value || '').trim();
    if (!normalized) return false;
    return linesCache.some((entry) => {
      if (!entry || entry.id === lineId) return false;
      const current = typeof entry.phone === 'string' ? entry.phone.trim() : '';
      return current === normalized;
    });
  }

  function isDuplicateName(lineId, value) {
    const normalized = (value || '').trim().toLowerCase();
    if (!normalized) return false;
    return linesCache.some((entry) => {
      if (!entry || entry.id === lineId) return false;
      const current = typeof entry.name === 'string' ? entry.name.trim().toLowerCase() : '';
      return current === normalized;
    });
  }

  async function persistPhoneNumber(lineId) {
    const input = document.getElementById(`line-${lineId}-phone`);
    if (!input) return;
    const value = input.value.trim();

    if (!/^\d*$/.test(value)) {
      throw new Error('Phone number may only contain digits.');
    }
    if (isDuplicatePhone(lineId, value)) {
      throw new Error('Phone number already used by another line.');
    }

    const body = new URLSearchParams({ line: String(lineId), phone: value }).toString();
    const r = await fetch('/api/line/phone', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body
    });

    if (!r.ok) {
      let msg = `HTTP ${r.status}`;
      try {
        const data = await r.json();
        if (data && typeof data.error === 'string') msg = data.error;
      } catch {}
      throw new Error(msg);
    }

    const entry = linesCache.find((x) => x.id === lineId);
    if (entry) entry.phone = value;
  }

  async function persistLineName(lineId) {
    const input = document.getElementById(`line-${lineId}-name`);
    if (!input) return;
    const value = input.value.trim();

    if (isDuplicateName(lineId, value)) {
      throw new Error('Name already used by another line.');
    }

    const body = new URLSearchParams({ line: String(lineId), name: value }).toString();
    const r = await fetch('/api/line/name', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body
    });

    if (!r.ok) {
      let msg = `HTTP ${r.status}`;
      try {
        const data = await r.json();
        if (data && typeof data.error === 'string') msg = data.error;
      } catch {}
      throw new Error(msg);
    }

    const entry = linesCache.find((x) => x.id === lineId);
    if (entry) {
      entry.name = value;
      updateLineLabel(entry);
    }
  }

  async function saveFieldOnBlur(lineId, field) {
    try {
      if (field === 'name') await persistLineName(lineId);
      else await persistPhoneNumber(lineId);
      setStatus('');
    } catch (e) {
      const entry = linesCache.find((x) => x.id === lineId);
      if (field === 'name') updateNameInput(entry);
      else updatePhoneInput(entry);
      setStatus((e && e.message) ? e.message : `Failed to save ${field} for line ${lineId}.`);
    }
  }

  function updateActiveCell(lineId) {
    const aCell = document.getElementById(`line-${lineId}-active`);
    if (!aCell) return;
    const a = isActive(lineId);
    aCell.textContent = a ? 'Active' : 'Inactive';
    aCell.classList.toggle('ok', a);
    aCell.classList.toggle('no', !a);
    aCell.classList.add('clickable');
    aCell.setAttribute('aria-pressed', a ? 'true' : 'false');
  }

  async function toggleLineActive(lineId, badgeEl) {
    try {
      badgeEl.classList.add('working');
      badgeEl.disabled = true;

      const body = new URLSearchParams({ line: String(lineId) }).toString();
      const r = await fetch('/api/active/toggle', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body
      });

      if (!r.ok) throw new Error(`HTTP ${r.status}`);
      const d = await r.json();

      if (typeof d.mask === 'number') {
        activeMask = d.mask | 0;
        linesCache.forEach((l) => {
          updateActiveCell(l.id);
          updateStatusVisibility(l.id);
        });
      }
    } catch (e) {
      setStatus('Could not update active/inactive.');
    } finally {
      badgeEl.classList.remove('working');
      badgeEl.disabled = false;
    }
  }

  function upsertLine(line) {
    if (!line) return;
    const lineId = typeof line.id === 'number' ? (line.id | 0) : 0;

    let entry = linesCache.find((x) => x.id === lineId);
    if (!entry) {
      entry = { id: lineId, status: '', phone: '', name: '', incomingFrom: -1, outgoingTo: -1 };
      linesCache.push(entry);
    }

    if (typeof line.status === 'string') entry.status = line.status;
    if (typeof line.phone === 'string') entry.phone = line.phone.trim();
    if (typeof line.name === 'string') entry.name = line.name.trim();
    if (typeof line.incomingFrom === 'number') entry.incomingFrom = line.incomingFrom;
    if (typeof line.outgoingTo === 'number') entry.outgoingTo = line.outgoingTo;

    let row = document.getElementById(`line-${lineId}`);
    if (!row) {
      row = document.createElement('div');
      row.className = 'row';
      row.id = `line-${lineId}`;
      row.innerHTML = `
        <div class="k line-col">
          <span class="line-label" id="line-${lineId}-label">Line ${lineId}</span>
          <div class="line-editors">
            <input type="text" id="line-${lineId}-name" autocomplete="off" placeholder="Name" maxlength="32" />
            <input type="text" id="line-${lineId}-phone" inputmode="numeric" pattern="[0-9]*" autocomplete="tel" placeholder="Phone number" maxlength="32" />
          </div>
        </div>
        <span class="status-pad" id="line-${lineId}-status"></span>
        <button class="badge clickable active-toggle" id="line-${lineId}-active" type="button" title="Activate / Inactivate"></button>
      `;

      $lines.appendChild(row);

      const aCell = row.querySelector(`#line-${lineId}-active`);
      if (aCell) aCell.addEventListener('click', () => toggleLineActive(lineId, aCell));

      const phoneInput = row.querySelector(`#line-${lineId}-phone`);
      const nameInput = row.querySelector(`#line-${lineId}-name`);

      phoneInput?.addEventListener('input', () => {
        const filtered = phoneInput.value.replace(/\D+/g, '');
        if (filtered !== phoneInput.value) phoneInput.value = filtered;
      });
      phoneInput?.addEventListener('blur', () => saveFieldOnBlur(lineId, 'phone'));
      nameInput?.addEventListener('blur', () => saveFieldOnBlur(lineId, 'name'));

      phoneInput?.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') {
          e.preventDefault();
          phoneInput.blur();
        }
      });
      nameInput?.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') {
          e.preventDefault();
          nameInput.blur();
        }
      });
    }

    updateStatusVisibility(lineId);
    updatePhoneInput(entry);
    updateNameInput(entry);
    updateLineLabel(entry);
    updateActiveCell(lineId);
  }

  function renderAll(lines) {
    linesCache = [];
    $lines.innerHTML = '';
    if (!Array.isArray(lines)) return;
    for (const l of lines) upsertLine(l);
    updateConnectionsView();
  }

  fetch('/api/status')
    .then((r) => r.json())
    .then((d) => {
      if (d && Array.isArray(d.lines)) renderAll(d.lines);
    })
    .catch(() => {});

  fetch('/api/active')
    .then((r) => r.json())
    .then((d) => {
      if (d && typeof d.mask === 'number') {
        activeMask = d.mask | 0;
        linesCache.forEach((l) => {
          updateActiveCell(l.id);
          updateStatusVisibility(l.id);
        });
      }
    })
    .catch(() => {});

  const es = new EventSource('/events');

  es.onmessage = (ev) => {
    try {
      const d = JSON.parse(ev.data);
      if (d && Array.isArray(d.lines)) renderAll(d.lines);
    } catch {}
  };

  es.addEventListener('lineStatus', (ev) => {
    try {
      const d = JSON.parse(ev.data);
      if (!('line' in d) || !('status' in d)) return;
      if (typeof d.line !== 'number' || typeof d.status !== 'string') return;
      upsertLine({
        id: d.line,
        status: d.status,
        incomingFrom: typeof d.incomingFrom === 'number' ? d.incomingFrom : -1,
        outgoingTo: typeof d.outgoingTo === 'number' ? d.outgoingTo : -1
      });
      updateConnectionsView();
    } catch {}
  });

  es.addEventListener('activeMask', (ev) => {
    try {
      const d = JSON.parse(ev.data);
      if (typeof d.mask === 'number') {
        activeMask = d.mask | 0;
        linesCache.forEach((l) => {
          updateActiveCell(l.id);
          updateStatusVisibility(l.id);
        });
      }
    } catch {}
  });

  window.addEventListener('beforeunload', () => {
    try { es.close(); } catch {}
  });
});

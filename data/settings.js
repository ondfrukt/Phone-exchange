'use strict';

document.addEventListener('DOMContentLoaded', () => {
  // Hjälpfunktioner för ms <-> sekunder konvertering
  const msToSec = (ms) => (ms / 1000).toFixed(2);
  const secToMs = (sec) => Math.round(parseFloat(sec) * 1000);
  
  // --- Debug UI ---
  const $dbgShk = document.getElementById('dbg-shk');
  const $dbgLm  = document.getElementById('dbg-lm');
  const $dbgWs  = document.getElementById('dbg-ws');
  const $dbgLa  = document.getElementById('dbg-la');
  const $dbgMt  = document.getElementById('dbg-mt');
  const $dbgTr  = document.getElementById('dbg-tr');
  const $dbgTg  = document.getElementById('dbg-tg');
  const $dbgRg  = document.getElementById('dbg-rg');
  const $dbgMcp = document.getElementById('dbg-mcp');
  const $dbgI2c = document.getElementById('dbg-i2c');
  const $dbgIm  = document.getElementById('dbg-im');

  const $dbgBtn = document.getElementById('dbg-save');
  const $dbgMsg = document.getElementById('dbg-status');
  const $restartBtn = document.getElementById('dbg-restart');

  const $toneEnabled = document.getElementById('tone-enabled');
  const $toneEnabledLabel = document.getElementById('tone-enabled-label');

  // --- Console UI (receive-only) ---
  const $consoleLog = document.getElementById('console-log');

  // Client-side console buffer to limit DOM updates and keep scroll performant
  const consoleMaxLines = 500;
  const consoleLines = [];

  let autoScrollConsole = true;

  // --- Ring Test UI ---
  const $ringLineSelect = document.getElementById('ring-line-select');
  const $ringTestBtn = document.getElementById('ring-test-btn');
  const $ringStopBtn = document.getElementById('ring-stop-btn');
  const $ringStatus = document.getElementById('ring-status');

  // --- Ring Settings UI ---
  const $ringLength = document.getElementById('ring-length');
  const $ringPause = document.getElementById('ring-pause');
  const $ringIterations = document.getElementById('ring-iterations');
  const $ringSaveBtn = document.getElementById('ring-save');
  const $ringSettingsStatus = document.getElementById('ring-settings-status');

  // --- SHK/Ring Detection Settings UI ---
  const $burstTickMs = document.getElementById('burst-tick-ms');
  const $hookStableMs = document.getElementById('hook-stable-ms');
  const $hookStableConsec = document.getElementById('hook-stable-consec');
  const $shkSaveBtn = document.getElementById('shk-save');
  const $shkSettingsStatus = document.getElementById('shk-settings-status');

  // --- Timer Settings UI ---
  const $timerReady = document.getElementById('timer-ready');
  const $timerDialing = document.getElementById('timer-dialing');
  const $timerRinging = document.getElementById('timer-ringing');
  const $timerPulseDialing = document.getElementById('timer-pulse-dialing');
  const $timerToneDialing = document.getElementById('timer-tone-dialing');
  const $timerFail = document.getElementById('timer-fail');
  const $timerDisconnected = document.getElementById('timer-disconnected');
  const $timerTimeout = document.getElementById('timer-timeout');
  const $timerBusy = document.getElementById('timer-busy');
  const $timerSaveBtn = document.getElementById('timer-save');
  const $timerStatus = document.getElementById('timer-status');

  // Bitmask representing active/inactive lines (0..255)
  let activeMask = 0;

  // Helpers
  const isActive  = (id) => ((activeMask >> id) & 1) === 1;
  const setDbgMsg = (t) => { if ($dbgMsg) $dbgMsg.textContent = t; };
  const setRingStatus = (t) => { if ($ringStatus) $ringStatus.textContent = t; };
  const setRingSettingsStatus = (t) => { if ($ringSettingsStatus) $ringSettingsStatus.textContent = t; };
  const setShkSettingsStatus = (t) => { if ($shkSettingsStatus) $shkSettingsStatus.textContent = t; };
  const setTimerStatus = (t) => { if ($timerStatus) $timerStatus.textContent = t; };

  const setToneEnabledUi = (enabled) => {
    if ($toneEnabled) $toneEnabled.checked = !!enabled;
    if ($toneEnabledLabel) $toneEnabledLabel.textContent = enabled ? 'Enabled' : 'Disabled';
  };

  // Update the ring line selector with active lines
  function updateRingLineSelector() {
    if (!$ringLineSelect) return;
    
    // Clear existing options
    $ringLineSelect.innerHTML = '';
    
    // Add options for active lines
    let hasActive = false;
    for (let i = 0; i < 8; i++) {
      if (isActive(i)) {
        const option = document.createElement('option');
        option.value = i;
        option.textContent = `Line ${i}`;
        $ringLineSelect.appendChild(option);
        hasActive = true;
      }
    }
    
    // If no active lines, show placeholder
    if (!hasActive) {
      const option = document.createElement('option');
      option.value = '';
      option.textContent = '-- No active lines --';
      $ringLineSelect.appendChild(option);
      if ($ringTestBtn) $ringTestBtn.disabled = true;
    } else {
      if ($ringTestBtn) $ringTestBtn.disabled = false;
    }
  }

  // Load debug levels from server
  async function loadDebug() {
    try{
      const r = await fetch('/api/debug');
      if (!r.ok) throw new Error('HTTP '+r.status);
      const d = await r.json();
      if (typeof d.shk === 'number') $dbgShk.value = String(d.shk|0);
      if (typeof d.lm  === 'number') $dbgLm.value  = String(d.lm|0);
      if (typeof d.ws  === 'number') $dbgWs.value  = String(d.ws|0);
      if (typeof d.la  === 'number') $dbgLa.value  = String(d.la|0);
      if (typeof d.mt  === 'number') $dbgMt.value  = String(d.mt|0);
      if (typeof d.tr  === 'number') $dbgTr.value  = String(d.tr|0);
      if (typeof d.tg  === 'number') $dbgTg.value  = String(d.tg|0);
      if (typeof d.rg  === 'number') $dbgRg.value  = String(d.rg|0);
      if (typeof d.mcp === 'number') $dbgMcp.value = String(d.mcp|0);
      if (typeof d.i2c === 'number') $dbgI2c.value = String(d.i2c|0);
      if (typeof d.im  === 'number') $dbgIm.value  = String(d.im|0);
    } catch(e){
      setDbgMsg('Could not read debug levels');
      console.warn(e);
    }
  }

  // Save debug levels to server
  async function saveDebug() {
    try{
      $dbgBtn.classList.add('working');
      setDbgMsg('Saving…');
      const body = new URLSearchParams({
        shk: $dbgShk.value,
        lm: $dbgLm.value,
        ws: $dbgWs.value,
        la: $dbgLa.value,
        mt: $dbgMt.value,
        tr: $dbgTr.value,
        tg: $dbgTg.value,
        rg: $dbgRg.value,
        mcp: $dbgMcp.value,
        i2c: $dbgI2c.value,
        im: $dbgIm.value
      }).toString();
      const r = await fetch('/api/debug/set', {
        method:'POST',
        headers:{ 'Content-Type':'application/x-www-form-urlencoded' },
        body
      });
      if (!r.ok) throw new Error('HTTP '+r.status);
      await r.json();
      setDbgMsg('Saved.');
    } catch(e){
      setDbgMsg('Failed to save.');
      console.warn(e);
    } finally {
      $dbgBtn.classList.remove('working');
      setTimeout(()=> setDbgMsg(''), 1500);
    }
  }

  // Load tone generator state
  async function loadToneGenerator() {
    try {
      const r = await fetch('/api/tone-generator');
      if (!r.ok) throw new Error('HTTP ' + r.status);
      const d = await r.json();
      if (typeof d.enabled === 'boolean') setToneEnabledUi(d.enabled);
      else if (typeof d.enabled === 'number') setToneEnabledUi(Boolean(d.enabled));
    } catch (e) {
      console.warn('Could not read tone generator state', e);
    }
  }

  // Save tone generator state
  async function saveToneGenerator() {
    if (!$toneEnabled) return;
    try {
      $toneEnabled.disabled = true;
      const body = new URLSearchParams({ enabled: $toneEnabled.checked ? '1' : '0' }).toString();
      const r = await fetch('/api/tone-generator/set', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body
      });
      if (!r.ok) throw new Error('HTTP ' + r.status);
      const d = await r.json();
      if ('enabled' in d) setToneEnabledUi(!!d.enabled);
      setDbgMsg('Tone generator updated.');
      setTimeout(() => setDbgMsg(''), 1500);
    } catch (e) {
      console.warn('Could not update tone generator', e);
      setDbgMsg('Failed to update tone generator.');
      $toneEnabled.checked = !$toneEnabled.checked;
    } finally {
      $toneEnabled.disabled = false;
    }
  }

  // Load ring settings from server
  async function loadRingSettings() {
    try {
      const r = await fetch('/api/settings/ring');
      if (!r.ok) throw new Error('HTTP ' + r.status);
      const d = await r.json();
      if (typeof d.ringLengthMs === 'number') $ringLength.value = msToSec(d.ringLengthMs);
      if (typeof d.ringPauseMs === 'number') $ringPause.value = msToSec(d.ringPauseMs);
      if (typeof d.ringIterations === 'number') $ringIterations.value = d.ringIterations;
    } catch (e) {
      console.warn('Could not read ring settings', e);
      setRingSettingsStatus('Could not read ring settings');
    }
  }

  // Save ring settings to server
  async function saveRingSettings() {
    try {
      $ringSaveBtn.classList.add('working');
      setRingSettingsStatus('Saving…');
      const body = new URLSearchParams({
        ringLengthMs: secToMs($ringLength.value),
        ringPauseMs: secToMs($ringPause.value),
        ringIterations: $ringIterations.value
      }).toString();
      const r = await fetch('/api/settings/ring', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body
      });
      if (!r.ok) throw new Error('HTTP ' + r.status);
      await r.json();
      setRingSettingsStatus('Saved.');
    } catch (e) {
      setRingSettingsStatus('Failed to save.');
      console.warn(e);
    } finally {
      $ringSaveBtn.classList.remove('working');
      setTimeout(() => setRingSettingsStatus(''), 1500);
    }
  }

  // Load SHK/ring-detection settings from server
  async function loadShkSettings() {
    try {
      const r = await fetch('/api/settings/shk');
      if (!r.ok) throw new Error('HTTP ' + r.status);
      const d = await r.json();
      if (typeof d.burstTickMs === 'number') $burstTickMs.value = msToSec(d.burstTickMs);
      if (typeof d.hookStableMs === 'number') $hookStableMs.value = msToSec(d.hookStableMs);
      if (typeof d.hookStableConsec === 'number') $hookStableConsec.value = d.hookStableConsec;
    } catch (e) {
      console.warn('Could not read SHK settings', e);
      setShkSettingsStatus('Could not read SHK settings');
    }
  }

  // Save SHK/ring-detection settings to server
  async function saveShkSettings() {
    try {
      $shkSaveBtn.classList.add('working');
      setShkSettingsStatus('Saving…');
      const body = new URLSearchParams({
        burstTickMs: secToMs($burstTickMs.value),
        hookStableMs: secToMs($hookStableMs.value),
        hookStableConsec: $hookStableConsec.value
      }).toString();
      const r = await fetch('/api/settings/shk', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body
      });
      if (!r.ok) throw new Error('HTTP ' + r.status);
      await r.json();
      setShkSettingsStatus('Saved.');
    } catch (e) {
      setShkSettingsStatus('Failed to save.');
      console.warn(e);
    } finally {
      $shkSaveBtn.classList.remove('working');
      setTimeout(() => setShkSettingsStatus(''), 1500);
    }
  }

  // Load timer settings from server
  async function loadTimerSettings() {
    try {
      const r = await fetch('/api/settings/timers');
      if (!r.ok) throw new Error('HTTP ' + r.status);
      const d = await r.json();
      if (typeof d.timer_Ready === 'number') $timerReady.value = msToSec(d.timer_Ready);
      if (typeof d.timer_Dialing === 'number') $timerDialing.value = msToSec(d.timer_Dialing);
      if (typeof d.timer_Ringing === 'number') $timerRinging.value = msToSec(d.timer_Ringing);
      if (typeof d.timer_pulsDialing === 'number') $timerPulseDialing.value = msToSec(d.timer_pulsDialing);
      if (typeof d.timer_toneDialing === 'number') $timerToneDialing.value = msToSec(d.timer_toneDialing);
      if (typeof d.timer_fail === 'number') $timerFail.value = msToSec(d.timer_fail);
      if (typeof d.timer_disconnected === 'number') $timerDisconnected.value = msToSec(d.timer_disconnected);
      if (typeof d.timer_timeout === 'number') $timerTimeout.value = msToSec(d.timer_timeout);
      if (typeof d.timer_busy === 'number') $timerBusy.value = msToSec(d.timer_busy);
    } catch (e) {
      console.warn('Could not read timer settings', e);
      setTimerStatus('Could not read timer settings');
    }
  }

  // Save timer settings to server
  async function saveTimerSettings() {
    try {
      $timerSaveBtn.classList.add('working');
      setTimerStatus('Saving…');
      const body = new URLSearchParams({
        timer_Ready: secToMs($timerReady.value),
        timer_Dialing: secToMs($timerDialing.value),
        timer_Ringing: secToMs($timerRinging.value),
        timer_pulsDialing: secToMs($timerPulseDialing.value),
        timer_toneDialing: secToMs($timerToneDialing.value),
        timer_fail: secToMs($timerFail.value),
        timer_disconnected: secToMs($timerDisconnected.value),
        timer_timeout: secToMs($timerTimeout.value),
        timer_busy: secToMs($timerBusy.value)
      }).toString();
      const r = await fetch('/api/settings/timers', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body
      });
      if (!r.ok) throw new Error('HTTP ' + r.status);
      await r.json();
      setTimerStatus('Saved.');
    } catch (e) {
      setTimerStatus('Failed to save.');
      console.warn(e);
    } finally {
      $timerSaveBtn.classList.remove('working');
      setTimeout(() => setTimerStatus(''), 1500);
    }
  }

  // Test ring on selected line
  async function testRing() {
    const line = $ringLineSelect.value;
    if (!line || line === '') {
      setRingStatus('Please select a line');
      return;
    }

    try {
      $ringTestBtn.classList.add('working');
      $ringTestBtn.disabled = true;
      setRingStatus('Starting ring test...');
      
      const body = new URLSearchParams({ line }).toString();
      const r = await fetch('/api/ring/test', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body
      });
      
      if (!r.ok) throw new Error('HTTP ' + r.status);
      const d = await r.json();
      
      setRingStatus(`Ring test started on line ${line}`);
      $ringStopBtn.disabled = false;
    } catch (e) {
      setRingStatus('Failed to start ring test: ' + e.message);
      console.warn(e);
    } finally {
      $ringTestBtn.classList.remove('working');
      $ringTestBtn.disabled = false;
      setTimeout(() => setRingStatus(''), 3000);
    }
  }

  // Stop ring test
  async function stopRing() {
    try {
      $ringStopBtn.classList.add('working');
      $ringStopBtn.disabled = true;
      setRingStatus('Stopping ring...');
      
      const r = await fetch('/api/ring/stop', {
        method: 'POST'
      });
      
      if (!r.ok) throw new Error('HTTP ' + r.status);
      
      setRingStatus('Ring stopped');
      $ringStopBtn.disabled = true;
    } catch (e) {
      setRingStatus('Failed to stop ring: ' + e.message);
      console.warn(e);
    } finally {
      $ringStopBtn.classList.remove('working');
      setTimeout(() => setRingStatus(''), 3000);
    }
  }

  // Restart device
  async function restartDevice() {
    if (!confirm('Are you sure you want to restart the device?')) return;
    try{
      const r = await fetch('/api/restart', { method:'POST' });
      if (!r.ok) throw new Error('HTTP '+r.status);
      setDbgMsg('Restarting…');
    } catch(e){
      setDbgMsg('Could not restart device');
      console.warn(e);
    }
  }

  // Console: format timestamp and append a new console line to the client buffer.
  // We keep the DOM updates limited by maintaining consoleLines and writing the
  // combined textNode. This reduces reflow when many messages arrive.
  function formatTime(ts) {
    try {
      // Timestamp från backend är i sekunder (Unix timestamp) om >1000000000,
      // annars är det gamla millis() format
      const dt = (ts > 1000000000) ? new Date(ts * 1000) : new Date(ts);
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
    if ($consoleLog) {
      $consoleLog.textContent = consoleLines.join('\n');

      if (autoScrollConsole) {
        // Keep console scrolled to bottom when user hasn't manually scrolled up
        $consoleLog.scrollTop = $consoleLog.scrollHeight;
      }
    }
  }

  // Track manual scroll to disable auto-scroll when user scrolls up.
  $consoleLog?.addEventListener('scroll', () => {
    if (!$consoleLog) return;
    const nearBottom = ($consoleLog.scrollTop + $consoleLog.clientHeight) >= ($consoleLog.scrollHeight - 20);
    autoScrollConsole = nearBottom;
  });

  // Event listeners
  $dbgBtn?.addEventListener('click', saveDebug);
  $restartBtn?.addEventListener('click', restartDevice);
  $toneEnabled?.addEventListener('change', saveToneGenerator);
  $ringTestBtn?.addEventListener('click', testRing);
  $ringStopBtn?.addEventListener('click', stopRing);
  $ringSaveBtn?.addEventListener('click', saveRingSettings);
  $shkSaveBtn?.addEventListener('click', saveShkSettings);
  $timerSaveBtn?.addEventListener('click', saveTimerSettings);

  // Load current activeMask
  fetch('/api/active')
    .then(r => r.json())
    .then(d => {
      if (d && typeof d.mask === 'number') {
        activeMask = d.mask|0;
        updateRingLineSelector();
      }
    })
    .catch(()=>{});

  // ---- SSE (Server-Sent Events) ----
  const es = new EventSource('/events');
  
  // Named SSE event for active mask updates
  es.addEventListener('activeMask', ev => {
    try{
      const d = JSON.parse(ev.data);
      if (typeof d.mask === 'number'){
        activeMask = d.mask|0;
        updateRingLineSelector();
      }
    } catch {}
  });

  // Tone generator updates via SSE
  es.addEventListener('toneGen', ev => {
    try {
      const d = JSON.parse(ev.data);
      if ('enabled' in d) setToneEnabledUi(!!d.enabled);
    } catch {}
  });

  // Debug levels updated via SSE
  es.addEventListener('debug', ev => {
    try{
      const d = JSON.parse(ev.data);
      if (typeof d.shk === 'number') $dbgShk.value = String(d.shk|0);
      if (typeof d.lm  === 'number') $dbgLm.value  = String(d.lm|0);
      if (typeof d.ws  === 'number') $dbgWs.value  = String(d.ws|0);
      if (typeof d.la  === 'number') $dbgLa.value  = String(d.la|0);
      if (typeof d.mt  === 'number') $dbgMt.value  = String(d.mt|0);
      if (typeof d.tr  === 'number') $dbgTr.value  = String(d.tr|0);
      if (typeof d.tg  === 'number') $dbgTg.value  = String(d.tg|0);
      if (typeof d.rg  === 'number') $dbgRg.value  = String(d.rg|0);
      if (typeof d.mcp === 'number') $dbgMcp.value = String(d.mcp|0);
      if (typeof d.i2c === 'number') $dbgI2c.value = String(d.i2c|0);
      if (typeof d.im  === 'number') $dbgIm.value  = String(d.im|0);
    } catch {}
  });

  // Console messages via SSE
  es.addEventListener('console', ev => {
    try {
      const d = JSON.parse(ev.data);
      appendConsoleLine(d);
    } catch {}
  });

  window.addEventListener('beforeunload', () => { try { es.close(); } catch {} });

  // Initial load
  loadDebug();
  loadToneGenerator();
  loadRingSettings();
  loadShkSettings();
  loadTimerSettings();
});

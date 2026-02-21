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
  const $dbgLac = document.getElementById('dbg-lac');

  const $dbgBtn = document.getElementById('dbg-save');
  const $dbgMsg = document.getElementById('dbg-status');
  const $restartBtn = document.getElementById('dbg-restart');
  const $wifiChangeBtn = document.getElementById('wifi-change-btn');
  const $factoryResetBtn = document.getElementById('factory-reset-btn');
  const $deviceStatus = document.getElementById('device-status');
  const $devHostname = document.getElementById('dev-hostname');
  const $devMac = document.getElementById('dev-mac');
  const $devIp = document.getElementById('dev-ip');
  const $devSsid = document.getElementById('dev-ssid');
  const $devWifiStatus = document.getElementById('dev-wifi-status');
  const $devMqttStatus = document.getElementById('dev-mqtt-status');

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

  // --- ToneReader Settings UI ---
  const $dtmfDebounceMs = document.getElementById('dtmf-debounce-ms');
  const $dtmfMinToneMs = document.getElementById('dtmf-min-tone-ms');
  const $dtmfStdStableMs = document.getElementById('dtmf-std-stable-ms');
  const $tmuxScanDwellMinMs = document.getElementById('tmux-scan-dwell-min-ms');
  const $toneReaderSaveBtn = document.getElementById('tonereader-save');
  const $toneReaderStatus = document.getElementById('tonereader-status');

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

  // --- MQTT Settings UI ---
  const $mqttEnabled = document.getElementById('mqtt-enabled');
  const $mqttEnabledLabel = document.getElementById('mqtt-enabled-label');
  const $mqttHost = document.getElementById('mqtt-host');
  const $mqttPort = document.getElementById('mqtt-port');
  const $mqttUsername = document.getElementById('mqtt-username');
  const $mqttPassword = document.getElementById('mqtt-password');
  const $mqttClientId = document.getElementById('mqtt-client-id');
  const $mqttBaseTopic = document.getElementById('mqtt-base-topic');
  const $mqttQos = document.getElementById('mqtt-qos');
  const $mqttRetain = document.getElementById('mqtt-retain');
  const $mqttRetainLabel = document.getElementById('mqtt-retain-label');
  const $mqttSaveBtn = document.getElementById('mqtt-save');
  const $mqttStatus = document.getElementById('mqtt-status');

  function setupSettingsNavigation() {
    const main = document.querySelector('main.wrap');
    const header = document.querySelector('.page-header');
    if (!main || !header) return;

    const panels = Array.from(main.querySelectorAll('section.card'));
    if (!panels.length) return;

    const desiredOrder = [
      'Device Control',
      'Debug Levels',
      'Ring Test',
      'Ring Settings',
      'Hook Detection & Burst',
      'ToneReader Settings',
      'Timer Settings',
      'MQTT Settings',
      'Console'
    ];

    const panelIdByTitle = {
      'Debug Levels': 'panel-debug',
      'Ring Test': 'panel-ring-test',
      'Ring Settings': 'panel-ring-settings',
      'Hook Detection & Burst': 'panel-hook-burst',
      'ToneReader Settings': 'panel-tonereader',
      'Timer Settings': 'panel-timers',
      'MQTT Settings': 'panel-mqtt',
      'Device Control': 'panel-device',
      'Console': 'panel-console'
    };

    const byTitle = new Map();
    panels.forEach((panel) => {
      const heading = panel.querySelector('h2');
      const title = (heading?.textContent || '').trim();
      if (!title) return;
      byTitle.set(title, panel);
    });

    const orderedPanels = [];
    desiredOrder.forEach((title) => {
      const panel = byTitle.get(title);
      if (!panel) return;
      panel.id = panelIdByTitle[title] || panel.id;
      panel.classList.add('settings-panel');
      orderedPanels.push({ title, panel });
      byTitle.delete(title);
    });

    byTitle.forEach((panel, title) => {
      panel.id = panel.id || `panel-${title.toLowerCase().replace(/[^a-z0-9]+/g, '-')}`;
      panel.classList.add('settings-panel');
      orderedPanels.push({ title, panel });
    });

    const layout = document.createElement('div');
    layout.className = 'settings-layout';

    const sidebar = document.createElement('aside');
    sidebar.className = 'settings-sidebar card';
    const sidebarTitle = document.createElement('h2');
    sidebarTitle.className = 'settings-sidebar-title';
    sidebarTitle.textContent = 'Sections';
    sidebar.appendChild(sidebarTitle);

    const content = document.createElement('div');
    content.className = 'settings-content';

    orderedPanels.forEach(({ title, panel }) => {
      const tab = document.createElement('button');
      tab.type = 'button';
      tab.className = 'settings-tab';
      tab.dataset.target = panel.id;
      tab.textContent = title;
      sidebar.appendChild(tab);
      content.appendChild(panel);
    });

    layout.appendChild(sidebar);
    layout.appendChild(content);
    main.appendChild(layout);

    function setActivePanel(targetId) {
      const tabs = layout.querySelectorAll('.settings-tab');
      const settingsPanels = layout.querySelectorAll('.settings-panel');
      tabs.forEach((tab) => tab.classList.toggle('active', tab.dataset.target === targetId));
      settingsPanels.forEach((panel) => panel.classList.toggle('active', panel.id === targetId));
    }

    layout.addEventListener('click', (event) => {
      const tab = event.target.closest('.settings-tab');
      if (!tab) return;
      setActivePanel(tab.dataset.target);
      history.replaceState(null, '', `#${tab.dataset.target}`);
    });

    const hashTarget = location.hash ? location.hash.replace('#', '') : '';
    const firstId = orderedPanels[0]?.panel.id || '';
    const initialId = orderedPanels.some(({ panel }) => panel.id === hashTarget) ? hashTarget : firstId;
    if (initialId) setActivePanel(initialId);
  }

  // Bitmask representing active/inactive lines (0..255)
  let activeMask = 0;

  // Helpers
  const isActive  = (id) => ((activeMask >> id) & 1) === 1;
  const setDbgMsg = (t) => { if ($dbgMsg) $dbgMsg.textContent = t; };
  const setDeviceStatus = (t) => { if ($deviceStatus) $deviceStatus.textContent = t; };
  const setRingStatus = (t) => { if ($ringStatus) $ringStatus.textContent = t; };
  const setRingSettingsStatus = (t) => { if ($ringSettingsStatus) $ringSettingsStatus.textContent = t; };
  const setShkSettingsStatus = (t) => { if ($shkSettingsStatus) $shkSettingsStatus.textContent = t; };
  const setToneReaderStatus = (t) => { if ($toneReaderStatus) $toneReaderStatus.textContent = t; };
  const setTimerStatus = (t) => { if ($timerStatus) $timerStatus.textContent = t; };
  const setMqttStatus = (t) => { if ($mqttStatus) $mqttStatus.textContent = t; };

  const setToneEnabledUi = (enabled) => {
    if ($toneEnabled) $toneEnabled.checked = !!enabled;
    if ($toneEnabledLabel) $toneEnabledLabel.textContent = enabled ? 'Enabled' : 'Disabled';
  };

  const setMqttUiState = (enabled, retain) => {
    if ($mqttEnabled) $mqttEnabled.checked = !!enabled;
    if ($mqttEnabledLabel) $mqttEnabledLabel.textContent = enabled ? 'Enabled' : 'Disabled';
    if ($mqttRetain) $mqttRetain.checked = !!retain;
    if ($mqttRetainLabel) $mqttRetainLabel.textContent = retain ? 'On' : 'Off';
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
      if (typeof d.lac === 'number') $dbgLac.value = String(d.lac|0);
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
        im: $dbgIm.value,
        lac: $dbgLac.value
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

  // Load ToneReader settings from server
  async function loadToneReaderSettings() {
    try {
      const r = await fetch('/api/settings/tone-reader');
      if (!r.ok) throw new Error('HTTP ' + r.status);
      const d = await r.json();
      if (typeof d.dtmfDebounceMs === 'number') $dtmfDebounceMs.value = d.dtmfDebounceMs;
      if (typeof d.dtmfMinToneDurationMs === 'number') $dtmfMinToneMs.value = d.dtmfMinToneDurationMs;
      if (typeof d.dtmfStdStableMs === 'number') $dtmfStdStableMs.value = d.dtmfStdStableMs;
      if (typeof d.tmuxScanDwellMinMs === 'number') $tmuxScanDwellMinMs.value = d.tmuxScanDwellMinMs;
    } catch (e) {
      console.warn('Could not read ToneReader settings', e);
      setToneReaderStatus('Could not read ToneReader settings');
    }
  }

  // Save ToneReader settings to server
  async function saveToneReaderSettings() {
    try {
      $toneReaderSaveBtn.classList.add('working');
      setToneReaderStatus('Saving…');
      const body = new URLSearchParams({
        dtmfDebounceMs: $dtmfDebounceMs.value,
        dtmfMinToneDurationMs: $dtmfMinToneMs.value,
        dtmfStdStableMs: $dtmfStdStableMs.value,
        tmuxScanDwellMinMs: $tmuxScanDwellMinMs.value
      }).toString();
      const r = await fetch('/api/settings/tone-reader', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body
      });
      if (!r.ok) throw new Error('HTTP ' + r.status);
      await r.json();
      setToneReaderStatus('Saved.');
    } catch (e) {
      setToneReaderStatus('Failed to save.');
      console.warn(e);
    } finally {
      $toneReaderSaveBtn.classList.remove('working');
      setTimeout(() => setToneReaderStatus(''), 1500);
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

  // Load MQTT settings from server
  async function loadMqttSettings() {
    try {
      const r = await fetch('/api/settings/mqtt');
      if (!r.ok) throw new Error('HTTP ' + r.status);
      const d = await r.json();
      setMqttUiState(!!d.enabled, !!d.retain);
      if (typeof d.host === 'string') $mqttHost.value = d.host;
      if (typeof d.port === 'number') $mqttPort.value = d.port;
      if (typeof d.username === 'string') $mqttUsername.value = d.username;
      if (typeof d.password === 'string') $mqttPassword.value = d.password;
      if (typeof d.clientId === 'string') $mqttClientId.value = d.clientId;
      if (typeof d.baseTopic === 'string') $mqttBaseTopic.value = d.baseTopic;
      if (typeof d.qos === 'number') $mqttQos.value = String(d.qos);
    } catch (e) {
      console.warn('Could not read MQTT settings', e);
      setMqttStatus('Could not read MQTT settings');
    }
  }

  // Save MQTT settings to server
  async function saveMqttSettings() {
    try {
      $mqttSaveBtn.classList.add('working');
      setMqttStatus('Saving…');
      const body = new URLSearchParams({
        enabled: $mqttEnabled.checked ? '1' : '0',
        host: ($mqttHost.value || '').trim(),
        port: ($mqttPort.value || '1883').trim(),
        username: ($mqttUsername.value || '').trim(),
        password: ($mqttPassword.value || '').trim(),
        clientId: ($mqttClientId.value || '').trim(),
        baseTopic: ($mqttBaseTopic.value || '').trim(),
        qos: ($mqttQos.value || '0').trim(),
        retain: $mqttRetain.checked ? '1' : '0'
      }).toString();
      const r = await fetch('/api/settings/mqtt', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body
      });
      if (!r.ok) throw new Error('HTTP ' + r.status);
      const d = await r.json();
      setMqttUiState(!!d.enabled, !!d.retain);
      setMqttStatus('Saved.');
    } catch (e) {
      setMqttStatus('Failed to save.');
      console.warn(e);
    } finally {
      $mqttSaveBtn.classList.remove('working');
      setTimeout(() => setMqttStatus(''), 1500);
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
    const line = $ringLineSelect.value;
    if (!line || line === '') {
      setRingStatus('Please select a line');
      return;
    }

    try {
      $ringStopBtn.classList.add('working');
      $ringStopBtn.disabled = true;
      setRingStatus('Stopping ring...');
      
      const body = new URLSearchParams({ line }).toString();
      const r = await fetch('/api/ring/stop', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body
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
      setDeviceStatus('Restarting...');
    } catch(e){
      setDeviceStatus('Could not restart device');
      console.warn(e);
    }
  }

  async function loadDeviceInfo() {
    try {
      const r = await fetch('/api/info');
      if (!r.ok) throw new Error('HTTP ' + r.status);
      const d = await r.json();
      if ($devHostname) $devHostname.textContent = d.hostname || '-';
      if ($devMac) $devMac.textContent = d.mac || '-';
      if ($devIp) $devIp.textContent = d.ip || '-';
      if ($devSsid) $devSsid.textContent = d.ssid || '(not connected)';
      if ($devWifiStatus) $devWifiStatus.textContent = d.wifiConnected ? 'Connected' : 'Disconnected';
      if ($devMqttStatus) $devMqttStatus.textContent = d.mqttEnabled ? 'Enabled' : 'Disabled';
    } catch (e) {
      console.warn('Could not read device info', e);
      setDeviceStatus('Could not read device info');
    }
  }

  async function changeWifiSettings() {
    if (!confirm('This will erase WiFi credentials and restart the device. Continue?')) return;
    try {
      setDeviceStatus('Starting WiFi reset...');
      const r = await fetch('/api/wifi/reset', { method: 'POST' });
      if (!r.ok) throw new Error('HTTP ' + r.status);
      setDeviceStatus('WiFi settings erased. Device restarting...');
    } catch (e) {
      console.warn(e);
      setDeviceStatus('Failed to reset WiFi settings');
    }
  }

  async function factoryReset() {
    if (!confirm('Factory reset will erase all settings and WiFi credentials, then restart. Continue?')) return;
    try {
      setDeviceStatus('Starting factory reset...');
      const r = await fetch('/api/factory-reset', { method: 'POST' });
      if (!r.ok) throw new Error('HTTP ' + r.status);
      setDeviceStatus('Factory reset started. Device restarting...');
    } catch (e) {
      console.warn(e);
      setDeviceStatus('Failed to start factory reset');
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
  $wifiChangeBtn?.addEventListener('click', changeWifiSettings);
  $factoryResetBtn?.addEventListener('click', factoryReset);
  $toneEnabled?.addEventListener('change', saveToneGenerator);
  $ringTestBtn?.addEventListener('click', testRing);
  $ringStopBtn?.addEventListener('click', stopRing);
  $ringSaveBtn?.addEventListener('click', saveRingSettings);
  $shkSaveBtn?.addEventListener('click', saveShkSettings);
  $toneReaderSaveBtn?.addEventListener('click', saveToneReaderSettings);
  $timerSaveBtn?.addEventListener('click', saveTimerSettings);
  $mqttSaveBtn?.addEventListener('click', saveMqttSettings);
  $mqttEnabled?.addEventListener('change', () => setMqttUiState($mqttEnabled.checked, $mqttRetain.checked));
  $mqttRetain?.addEventListener('change', () => setMqttUiState($mqttEnabled.checked, $mqttRetain.checked));

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
      if (typeof d.lac === 'number') $dbgLac.value = String(d.lac|0);
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
  setupSettingsNavigation();
  loadDebug();
  loadToneGenerator();
  loadRingSettings();
  loadShkSettings();
  loadToneReaderSettings();
  loadTimerSettings();
  loadMqttSettings();
  loadDeviceInfo();
  setInterval(loadDeviceInfo, 5000);
});

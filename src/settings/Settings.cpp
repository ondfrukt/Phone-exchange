#include "settings.h"

Settings::Settings() {
  resetDefaults();        // ensure fields have initial values
}

void Settings::adjustActiveLines() {
  // Adjust activeLinesMask to take physical MCP connections into account
  uint8_t userMask = activeLinesMask;
  if (mcpSlic1Present && !mcpSlic2Present) {
    // Only SLIC1 exists: allow only lines 0–3
    allowMask = 0b00001111;
  } else if (!mcpSlic1Present && mcpSlic2Present) {
    // Only SLIC2 exists: allow only lines 4–7
    allowMask = 0b11110000;
  } else {
    // Both exist: allow all lines
    allowMask = 0b11111111;
  }
  // Mask the user's mask so disallowed lines are always inactive
  activeLinesMask = userMask & allowMask;
}

void Settings::resetDefaults() {
  // Save only *settings*, not runtime status
  activeLinesMask       = 0b11111111;

  // -- Debug levels ---
  debugSHKLevel         = 0; // Debug for SHKService
  debugLmLevel          = 0; // Debug for LineManager
  debugWSLevel          = 0; // Debug for WebServer
  debugLALevel          = 0; // Debug for LineAction
  debugMTLevel          = 0; // Debug for MT8816Driver
  debugTRLevel          = 0; // Debug for ToneReader
  debugMCPLevel         = 0; // Debug for MCPDriver
  debugI2CLevel         = 0; // Debug for I2C
  debugTonGenLevel      = 0; // Debug for ToneGenerator
  debugRGLevel          = 0; // Debug for RingGenerator
  debugIMLevel          = 0; // Debug for InterruptManager
  debugLAC              = 0; // Debug for LineAudioConnections

  toneGeneratorEnabled  = true;
  pulseAdjustment       = 1;

  // SHK detection settings
  burstTickMs           = 2;    // Time for a burst tick
  hookStableMs          = 50;   // Time for stable hook state
  hookStableConsec      = 10;   // Number of consecutive stable readings for hook state
  pulsGlitchMs          = 2;    // Max glitch time for pulse dialing
  pulseDebounceMs       = 25;   // Debounce time for line state changes
  pulseLowMaxMs         = 75;   // Maximum pulse low duration

  // --- Pulse dialing settings ---
  digitGapMinMs         = 600;  // Minimum gap between digits
  globalPulseTimeoutMs  = 500;  // Global pulse timeout
  highMeansOffHook      = true; // High signal means off-hook state

  // --- Ringing settings ---
  // Typical sequence is 1s ring, 5s pause
  ringLengthMs          = 1000;  // Length of ringing signal in ms
  ringPauseMs           = 5000; // Pause between rings in ms

  ringIterations        = 5;    // Iterations of ringing signal

  // --- ToneReader (DTMF) settings ---
  dtmfDebounceMs        = 100;   // Debounce time for DTMF detection (minimum time between detections of the same digit)
  dtmfMinToneDurationMs = 25;    // Minimum duration of a valid DTMF tone (ms)
  dtmfStdStableMs       = 15;    // Stability time for STD signal during DTMF detection (ms)
  tmuxScanDwellMinMs    = 35;    // Minimum time to dwell on a line during TMUX scanning (ms)

  // --- Timers ---
  timer_Ready           = 240000;
  timer_Dialing         = 30000;    // TODO: Verify whether this timer is still used.
  timer_Ringing         = 30000;
  timer_incomming       = 30000;
  timer_pulsDialing     = 3000;
  timer_toneDialing     = 3000;
  timer_fail            = 30000;
  timer_disconnected    = 30000;
  timer_timeout         = 30000;
  timer_busy            = 30000;


  // --- Phone numbers ---
  for (int i = 0; i < 8; ++i) {
    linePhoneNumbers[i] = String(i);
    lineNames[i] = "";
  }

  // Runtime flags kept false here; set by MCPDriver::begin()
  mcpSlic1Present = mcpSlic2Present = mcpMainPresent = mcpMt8816Present = false;
  Serial.println("Settings reset to defaults");
  util::UIConsole::log("Settings reset to defaults", "Settings");
}

bool Settings::load() {
  Preferences prefs;
  if (!prefs.begin(kNamespace, true)) {
    Serial.println("Settings: No readable NVS namespace found, saving defaults.");
    util::UIConsole::log("No readable NVS namespace found, saving defaults.", "Settings");
    save();
    return false;
  }
  uint16_t v = prefs.getUShort("ver", 0);
  const bool supportedVersion = (v == kVersion || v == 3);
  const bool needsUpgrade = (v != kVersion) && supportedVersion;
  bool ok = supportedVersion;
  if (ok) {
    activeLinesMask       = prefs.getUChar ("activeMask", activeLinesMask);
    pulseDebounceMs        = prefs.getUShort("pulseDebounceMs", pulseDebounceMs);

    // --- Debug levels ---
    debugSHKLevel         = prefs.getUChar ("debugLevel", debugSHKLevel);
    debugLmLevel          = prefs.getUChar ("debugLm",    debugLmLevel);
    debugWSLevel          = prefs.getUChar ("debugWs",    debugWSLevel);
    debugLALevel          = prefs.getUChar ("debugLa",    debugLALevel);
    debugMTLevel          = prefs.getUChar ("debugMt",    debugMTLevel);
    debugTRLevel          = prefs.getUChar ("debugTr",    debugTRLevel);
    debugMCPLevel         = prefs.getUChar ("debugMCP",   debugMCPLevel);
    debugI2CLevel         = prefs.getUChar ("debugI2C",   debugI2CLevel);
    debugTonGenLevel      = prefs.getUChar ("debugTonGen", debugTonGenLevel);
    debugRGLevel          = prefs.getUChar ("debugRG",     debugRGLevel);
    debugIMLevel          = prefs.getUChar ("debugIM",     debugIMLevel);
    debugLAC              = prefs.getUChar ("debugLAC",    debugLAC);
  

    // --- Other settings ---    
    burstTickMs           = prefs.getUInt ("burstTickMs",       burstTickMs);
    hookStableMs          = prefs.getUInt ("hookStableMs",      hookStableMs);
    hookStableConsec      = prefs.getUChar("hookStbCnt",        hookStableConsec);
    pulsGlitchMs          = prefs.getUInt ("pulsGlitchMs",      pulsGlitchMs);
    pulseLowMaxMs         = prefs.getUInt ("pulseLowMaxMs",     pulseLowMaxMs);
    digitGapMinMs         = prefs.getUInt ("digitGapMinMs",     digitGapMinMs);
    globalPulseTimeoutMs  = prefs.getUInt ("globalPulseTO",     globalPulseTimeoutMs);
    highMeansOffHook      = prefs.getBool ("hiOffHook",         highMeansOffHook);
    toneGeneratorEnabled  = prefs.getBool ("toneGenEn",         toneGeneratorEnabled);

    ringLengthMs          = prefs.getUInt ("ringLengthMs",      ringLengthMs);
    ringPauseMs           = prefs.getUInt ("ringPauseMs",       ringPauseMs);
    ringIterations        = prefs.getUInt ("ringIterations",    ringIterations);

    // --- ToneReader (DTMF) settings ---
    dtmfDebounceMs        = prefs.getUInt ("dtmfDebounce",      dtmfDebounceMs);
    dtmfMinToneDurationMs = prefs.getUInt ("dtmfMinTone",       dtmfMinToneDurationMs);
    dtmfStdStableMs       = prefs.getUInt ("dtmfStdStable",     dtmfStdStableMs);
    tmuxScanDwellMinMs    = prefs.getUInt ("tmuxDwellMin",      tmuxScanDwellMinMs);

    // --- Timers ---
    timer_Ready           = prefs.getUInt ("timerReady",        timer_Ready);
    timer_Dialing         = prefs.getUInt ("timerDialing",      timer_Dialing);
    timer_Ringing         = prefs.getUInt ("timerRinging",      timer_Ringing);
    timer_incomming       = prefs.getUInt ("timerIncomming",    timer_incomming);
    timer_pulsDialing     = prefs.getUInt ("tmrPulsDial",       timer_pulsDialing);
    timer_toneDialing     = prefs.getUInt ("tmrToneDial",       timer_toneDialing);
    timer_fail            = prefs.getUInt ("timerFail",         timer_fail);
    timer_disconnected    = prefs.getUInt ("tmrDisconn",        timer_disconnected);
    timer_timeout         = prefs.getUInt ("timerTimeout",      timer_timeout);
    timer_busy            = prefs.getUInt ("timerBusy",         timer_busy);
    

    // --- Phone numbers ---
    for (int i = 0; i < 8; ++i) {
      String key = String("linePhone") + i;
      linePhoneNumbers[i] = prefs.getString(key.c_str(), linePhoneNumbers[i]);
      key = String("lineName") + i;
      lineNames[i] = prefs.getString(key.c_str(), lineNames[i]);
    }
  }
  prefs.end();
  if (!ok || needsUpgrade) save();
  return ok;
}

void Settings::save() const {
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) return;
  prefs.putUShort("ver", kVersion);

  // --- General settings ---
  prefs.putUChar ("activeMask", activeLinesMask);
  prefs.putUShort("pulseDebounceMs", pulseDebounceMs);

  // --- Debug levels ---
  prefs.putUChar ("debugLevel",           debugSHKLevel);
  prefs.putUChar ("debugLm",              debugLmLevel);
  prefs.putUChar ("debugWs",              debugWSLevel);
  prefs.putUChar ("debugLa",              debugLALevel);
  prefs.putUChar ("debugMt",              debugMTLevel);
  prefs.putUChar ("debugTr",              debugTRLevel);
  prefs.putUChar ("debugMCP",             debugMCPLevel);
  prefs.putUChar ("debugI2C",             debugI2CLevel);
  prefs.putUChar ("debugTonGen",          debugTonGenLevel);
  prefs.putUChar ("debugRG",              debugRGLevel);
  prefs.putUChar ("debugIM",              debugIMLevel);
  prefs.putUChar ("debugLAC",             debugLAC);

  // --- Other settings ---
  prefs.putUInt ("burstTickMs",           burstTickMs);
  prefs.putUInt ("hookStableMs",          hookStableMs);
  prefs.putUChar("hookStbCnt",            hookStableConsec);
  prefs.putUInt ("pulsGlitchMs",          pulsGlitchMs);
  prefs.putUInt ("pulseLowMaxMs",         pulseLowMaxMs);
  prefs.putUInt ("digitGapMinMs",         digitGapMinMs);
  prefs.putUInt ("globalPulseTO",         globalPulseTimeoutMs);
  prefs.putBool ("hiOffHook",             highMeansOffHook);
  prefs.putBool ("toneGenEn",             toneGeneratorEnabled);

  prefs.putUInt ("ringLengthMs",          ringLengthMs);
  prefs.putUInt ("ringPauseMs",           ringPauseMs);
  prefs.putUInt ("ringIterations",        ringIterations);

  // --- ToneReader (DTMF) settings ---
  prefs.putUInt ("dtmfDebounce",          dtmfDebounceMs);
  prefs.putUInt ("dtmfMinTone",           dtmfMinToneDurationMs);
  prefs.putUInt ("dtmfStdStable",         dtmfStdStableMs);
  prefs.putUInt ("tmuxDwellMin",          tmuxScanDwellMinMs);


  // --- Timers ---
  prefs.putUInt ("timerReady",            timer_Ready);
  prefs.putUInt ("timerDialing",          timer_Dialing);
  prefs.putUInt ("timerRinging",          timer_Ringing);
  prefs.putUInt ("timerIncomming",        timer_incomming);
  prefs.putUInt ("tmrPulsDial",           timer_pulsDialing);
  prefs.putUInt ("tmrToneDial",           timer_toneDialing);
  prefs.putUInt ("timerFail",             timer_fail);
  prefs.putUInt ("tmrDisconn",            timer_disconnected);
  prefs.putUInt ("timerTimeout",          timer_timeout);
  prefs.putUInt ("timerBusy",             timer_busy);

  // --- Phone numbers ---
  for (int i = 0; i < 8; ++i) {
    String key = String("linePhone") + i;
    prefs.putString(key.c_str(), linePhoneNumbers[i]);
    key = String("lineName") + i;
    prefs.putString(key.c_str(), lineNames[i]);
  }

  prefs.end();
}

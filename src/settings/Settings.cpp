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
  debugSHKLevel         = 0;
  debugLmLevel          = 0;
  debugWSLevel          = 0;
  debugLALevel          = 0;
  debugMTLevel          = 0;
  debugTRLevel          = 0;
  debugMCPLevel         = 0;
  debugI2CLevel         = 0;
  debugTonGenLevel      = 0;
  debugRGLevel          = 0;
  debugIMLevel          = 0;

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
  ringLengthMs          = 1000;  // Length of ringing signal in ms
  ringPauseMs           = 5000; // Pause between rings in ms
  ringIterations        = 2;    // Iterations of ringing signal

  // --- ToneReader (DTMF) settings ---
  dtmfDebounceMs        = 150;   // 150ms debounce between same digit detections
  dtmfMinToneDurationMs = 40;    // Minimum 40ms tone duration (MT8870 spec: 40ms typical)
  dtmfStdStableMs       = 5;     // STD signal must be stable for 5ms before reading

  // --- Timers ---
  timer_Ready           = 240000;
  timer_Dialing         = 5000;
  timer_Ringing         = 12000;
  timer_incomming       = 12000;
  timer_pulsDialing     = 3000;
  timer_toneDialing     = 3000;
  timer_fail            = 30000;
  timer_disconnected    = 60000;
  timer_timeout         = 60000;
  timer_busy            = 30000;


  // --- Phone numbers ---
  for (int i = 0; i < 8; ++i) {
    linePhoneNumbers[i] = String(i);
  }

  // Runtime flags kept false here; set by MCPDriver::begin()
  mcpSlic1Present = mcpSlic2Present = mcpMainPresent = mcpMt8816Present = false;
  Serial.println("Settings reset to defaults");
  util::UIConsole::log("Settings reset to defaults", "Settings");
}

bool Settings::load() {
  Preferences prefs;
  if (!prefs.begin(kNamespace, true)) {
    Serial.println("No NVS namespace found for settings");
    util::UIConsole::log("No NVS namespace found for settings", "Settings");
    return false;
  }
  uint16_t v = prefs.getUShort("ver", 0);
  bool ok = (v == kVersion);
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

    // --- Timers ---
    timer_Ready           = prefs.getUInt ("timerReady",        timer_Ready);
    timer_Dialing         = prefs.getUInt ("timerDialing",      timer_Dialing);
    timer_Ringing         = prefs.getUInt ("timerRinging",      timer_Ringing);
    timer_incomming       = prefs.getUInt ("timerIncomming",    timer_incomming);
    timer_pulsDialing     = prefs.getUInt ("timerPulsDialing",  timer_pulsDialing);
    timer_toneDialing     = prefs.getUInt ("timerToneDialing",  timer_toneDialing);
    timer_fail            = prefs.getUInt ("timerFail",         timer_fail);
    timer_disconnected    = prefs.getUInt ("timerDisconnected", timer_disconnected);
    timer_timeout         = prefs.getUInt ("timerTimeout",      timer_timeout);
    timer_busy            = prefs.getUInt ("timerBusy",         timer_busy);
    

    // --- Phone numbers ---
    for (int i = 0; i < 8; ++i) {
      String key = String("linePhone") + i;
      linePhoneNumbers[i] = prefs.getString(key.c_str(), linePhoneNumbers[i]);
    }
  }
  prefs.end();
  if (!ok) save();
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


  // --- Timers ---
  prefs.putUInt ("timerReady",            timer_Ready);
  prefs.putUInt ("timerDialing",          timer_Dialing);
  prefs.putUInt ("timerRinging",          timer_Ringing);
  prefs.putUInt ("timerIncomming",        timer_incomming);
  prefs.putUInt ("timerPulsDialing",      timer_pulsDialing);
  prefs.putUInt ("timerToneDialing",      timer_toneDialing);
  prefs.putUInt ("timerFail",             timer_fail);
  prefs.putUInt ("timerDisconnected",     timer_disconnected);
  prefs.putUInt ("timerTimeout",          timer_timeout);
  prefs.putUInt ("timerBusy",             timer_busy);

  // --- Phone numbers ---
  for (int i = 0; i < 8; ++i) {
    String key = String("linePhone") + i;
    prefs.putString(key.c_str(), linePhoneNumbers[i]);
  }

  prefs.end();
}

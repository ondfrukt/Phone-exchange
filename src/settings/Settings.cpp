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

  toneGeneratorEnabled  = true;
  pulseAdjustment       = 1;

  // --- Other settings ---
  burstTickMs           = 2;
  hookStableMs          = 50;
  hookStableConsec      = 2;
  pulseGlitchMs         = 2;
  debounceMs            = 25;
  pulseLowMaxMs         = 150;
  digitGapMinMs         = 600;
  globalPulseTimeoutMs  = 500;
  highMeansOffHook      = true;

  // --- Timers ---
  timer_Ready           = 240000;
  timer_Dialing         = 5000;
  timer_Ringing         = 10000;
  timer_pulsDialing     = 3000;
  timer_toneDialing     = 3000;
  timer_fail            = 30000;
  timer_disconnected    = 60000;
  timer_timeout         = 60000;
  timer_busy            = 30000;

  // --- Phone numbers ---
  for (auto &num : linePhoneNumbers) {
    num = "";
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
    debounceMs            = prefs.getUShort("debounceMs", debounceMs);

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

    // --- Other settings ---    
    burstTickMs           = prefs.getUInt ("burstTickMs",          burstTickMs);
    hookStableMs          = prefs.getUInt ("hookStableMs",         hookStableMs);
    hookStableConsec      = prefs.getUChar("hookStbCnt",           hookStableConsec);
    pulseGlitchMs         = prefs.getUInt ("pulseGlitchMs",        pulseGlitchMs);
    pulseLowMaxMs         = prefs.getUInt ("pulseLowMaxMs",        pulseLowMaxMs);
    digitGapMinMs         = prefs.getUInt ("digitGapMinMs",        digitGapMinMs);
    globalPulseTimeoutMs  = prefs.getUInt ("globalPulseTO",        globalPulseTimeoutMs);
    highMeansOffHook      = prefs.getBool ("hiOffHook",            highMeansOffHook);
    toneGeneratorEnabled  = prefs.getBool ("toneGenEn",            toneGeneratorEnabled);

    // --- Timers ---
    timer_Ready           = prefs.getUInt ("timerReady",        timer_Ready);
    timer_Dialing         = prefs.getUInt ("timerDialing",      timer_Dialing);
    timer_Ringing         = prefs.getUInt ("timerRinging",      timer_Ringing);
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
  prefs.putUShort("debounceMs", debounceMs);

  // --- Debug levels ---
  prefs.putUChar ("debugLevel", debugSHKLevel);
  prefs.putUChar ("debugLm",    debugLmLevel);
  prefs.putUChar ("debugWs",    debugWSLevel);
  prefs.putUChar ("debugLa",    debugLALevel);
  prefs.putUChar ("debugMt",    debugMTLevel);
  prefs.putUChar ("debugTr",    debugTRLevel);
  prefs.putUChar ("debugMCP",   debugMCPLevel);
  prefs.putUChar ("debugI2C",   debugI2CLevel);
  prefs.putUChar ("debugTonGen", debugTonGenLevel);

  // --- Other settings ---
  prefs.putUInt ("burstTickMs",          burstTickMs);
  prefs.putUInt ("hookStableMs",         hookStableMs);
  prefs.putUChar("hookStbCnt",           hookStableConsec);
  prefs.putUInt ("pulseGlitchMs",        pulseGlitchMs);
  prefs.putUInt ("pulseLowMaxMs",        pulseLowMaxMs);
  prefs.putUInt ("digitGapMinMs",        digitGapMinMs);
  prefs.putUInt ("globalPulseTO",        globalPulseTimeoutMs);
  prefs.putBool ("hiOffHook",            highMeansOffHook);
  prefs.putBool ("toneGenEn",            toneGeneratorEnabled);

  // --- Timers ---
  prefs.putUInt ("timerReady",        timer_Ready);
  prefs.putUInt ("timerDialing",      timer_Dialing);
  prefs.putUInt ("timerRinging",      timer_Ringing);
  prefs.putUInt ("timerPulsDialing",  timer_pulsDialing);
  prefs.putUInt ("timerToneDialing",  timer_toneDialing);
  prefs.putUInt ("timerFail",         timer_fail);
  prefs.putUInt ("timerDisconnected", timer_disconnected);
  prefs.putUInt ("timerTimeout",      timer_timeout);
  prefs.putUInt ("timerBusy",         timer_busy);

  // --- Phone numbers ---
  for (int i = 0; i < 8; ++i) {
    String key = String("linePhone") + i;
    prefs.putString(key.c_str(), linePhoneNumbers[i]);
  }

  prefs.end();
}

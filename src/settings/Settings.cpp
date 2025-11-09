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
  debugSHKLevel         = 1;
  debugLmLevel          = 0;
  debugWSLevel          = 0;
  debugLALevel          = 0;
  debugMTLevel          = 0;
  debugMCPLevel         = 0;

  pulseAdjustment       = 1;

  burstTickMs           = 2;
  hookStableMs          = 50;
  hookStableConsec      = 2;
  pulseGlitchMs         = 2;
  debounceMs            = 25;
  pulseLowMaxMs         = 150;
  digitGapMinMs         = 600;
  globalPulseTimeoutMs  = 500;
  highMeansOffHook      = true;

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
    debugMCPLevel         = prefs.getUChar ("debugMCP",   debugMCPLevel);
    debugI2CLevel         = prefs.getUChar ("debugI2C",   debugI2CLevel);

    // --- Other settings ---    
    burstTickMs           = prefs.getUInt ("burstTickMs",          burstTickMs);
    hookStableMs          = prefs.getUInt ("hookStableMs",         hookStableMs);
    hookStableConsec      = prefs.getUChar("hookStbCnt",           hookStableConsec);
    pulseGlitchMs         = prefs.getUInt ("pulseGlitchMs",        pulseGlitchMs);
    pulseLowMaxMs         = prefs.getUInt ("pulseLowMaxMs",        pulseLowMaxMs);
    digitGapMinMs         = prefs.getUInt ("digitGapMinMs",        digitGapMinMs);
    globalPulseTimeoutMs  = prefs.getUInt ("globalPulseTO",        globalPulseTimeoutMs);
    highMeansOffHook      = prefs.getBool ("hiOffHook",            highMeansOffHook);

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

  prefs.putUChar ("activeMask", activeLinesMask);
  prefs.putUShort("debounceMs", debounceMs);

  // --- Debug levels ---
  prefs.putUChar ("debugLevel", debugSHKLevel);
  prefs.putUChar ("debugLm",    debugLmLevel);
  prefs.putUChar ("debugWs",    debugWSLevel);
  prefs.putUChar ("debugLa",    debugLALevel);
  prefs.putUChar ("debugMt",    debugMTLevel);
  prefs.putUChar ("debugMCP",   debugMCPLevel);
  prefs.putUChar ("debugI2C",   debugI2CLevel);

  // --- Other settings ---
  prefs.putUInt ("burstTickMs",          burstTickMs);
  prefs.putUInt ("hookStableMs",         hookStableMs);
  prefs.putUChar("hookStbCnt",           hookStableConsec);
  prefs.putUInt ("pulseGlitchMs",        pulseGlitchMs);
  prefs.putUInt ("pulseLowMaxMs",        pulseLowMaxMs);
  prefs.putUInt ("digitGapMinMs",        digitGapMinMs);
  prefs.putUInt ("globalPulseTO",        globalPulseTimeoutMs);
  prefs.putBool ("hiOffHook",            highMeansOffHook);

  for (int i = 0; i < 8; ++i) {
    String key = String("linePhone") + i;
    prefs.putString(key.c_str(), linePhoneNumbers[i]);
  }

  prefs.end();
}

#include "settings.h"

Settings::Settings() {
  resetDefaults();        // se till att fälten har startvärden
}


void Settings::adjustActiveLines() {
  // Justera activeLinesMask så den tar hänsyn till fysiska MCP-anslutningar
  uint8_t userMask = activeLinesMask;
  if (mcpSlic1Present && !mcpSlic2Present) {
    // Bara SLIC1 finns: tillåt endast linjer 0–3
    allowMask = 0b00001111;
  } else if (!mcpSlic1Present && mcpSlic2Present) {
    // Bara SLIC2 finns: tillåt endast linjer 4–7
    allowMask = 0b11110000;
  } else {
    // Båda finns: tillåt alla linjer
    allowMask = 0b11111111;
  }
  // Maskera användarens mask så att otillåtna linjer alltid blir inaktiva
  activeLinesMask = userMask & allowMask;
}

void Settings::resetDefaults() {
  // Spara bara *inställningar*, inte körtidsstatus
  activeLinesMask       = 0b11110000;
  //debounceMs            = 25;

  // -- Debug levels ---
  debugSHKLevel         = 1;
  debugLmLevel          = 0;
  debugWSLevel          = 0;

  pulseAdjustment       = 1;

  burstTickMs           = 2;
  hookStableMs          = 50;
  hookStableConsec      = 2;
  pulseGlitchMs         = 2;
  pulseLowMaxMs         = 150;
  digitGapMinMs         = 600;
  globalPulseTimeoutMs  = 500;
  highMeansOffHook      = true;

  // körtidsflaggor hålls false här; sätts av MCPDriver::begin()
  mcpSlic1Present = mcpSlic2Present = mcpMainPresent = mcpMt8816Present = false;
  Serial.println("Settings reset to defaults");
}

bool Settings::load() {
  Preferences prefs;
  if (!prefs.begin(kNamespace, /*read only*/true)) {
    Serial.println("No NVS namespace found for settings");
    return false;
  }
  
  uint16_t v = prefs.getUShort("ver", 0);
  bool ok = (v == kVersion);
  if (ok) {
    activeLinesMask       = prefs.getUChar ("activeMask", activeLinesMask);
    debounceMs            = prefs.getUShort("debounceMs", debounceMs);
    debugSHKLevel         = prefs.getUChar ("debugLevel", debugSHKLevel);

    burstTickMs           = prefs.getUInt ("burstTickMs",          burstTickMs);
    hookStableMs          = prefs.getUInt ("hookStableMs",         hookStableMs);
    hookStableConsec      = prefs.getUChar("hookStbCnt",     hookStableConsec);
    pulseGlitchMs         = prefs.getUInt ("pulseGlitchMs",        pulseGlitchMs);
    pulseLowMaxMs         = prefs.getUInt ("pulseLowMaxMs",        pulseLowMaxMs);
    digitGapMinMs         = prefs.getUInt ("digitGapMinMs",        digitGapMinMs);
    globalPulseTimeoutMs  = prefs.getUInt ("globalPulseTO",        globalPulseTimeoutMs);
    highMeansOffHook      = prefs.getBool ("hiOffHook",     highMeansOffHook);
  }
  prefs.end();
  if (!ok) save();
  return ok;
}

void Settings::save() const {
  Preferences prefs;
  if (!prefs.begin(kNamespace, /*ro*/false)) return;
  prefs.putUShort("ver", kVersion);

  prefs.putUChar ("activeMask", activeLinesMask);
  prefs.putUShort("debounceMs", debounceMs);
  prefs.putUChar ("debugSHKLevel", debugSHKLevel);

  prefs.putUInt ("burstTickMs",          burstTickMs);
  prefs.putUInt ("hookStableMs",         hookStableMs);
  prefs.putUChar("hookStbCnt",     hookStableConsec);
  prefs.putUInt ("pulseGlitchMs",        pulseGlitchMs);
  prefs.putUInt ("pulseLowMaxMs",        pulseLowMaxMs);
  prefs.putUInt ("digitGapMinMs",        digitGapMinMs);
  prefs.putUInt ("globalPulseTO",        globalPulseTimeoutMs);
  prefs.putBool ("hiOffHook",     highMeansOffHook);

  prefs.end();
}

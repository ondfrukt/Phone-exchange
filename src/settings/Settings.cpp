#include "settings.h"

Settings::Settings() {
  resetDefaults();        // se till att fälten har startvärden
}

void Settings::resetDefaults() {
  // Spara bara *inställningar*, inte körtidsstatus
  activeLinesMask       = 0x00;
  debounceMs            = 25;

  burstTickMs           = 2;
  hookStableMs          = 10;
  hookStableConsec      = 2;
  pulseGlitchMs         = 2;
  pulseLowMaxMs         = 80;
  digitGapMinMs         = 180;
  globalPulseTimeoutMs  = 500;
  highMeansOffHook      = true;

  // körtidsflaggor hålls false här; sätts av MCPDriver::begin()
  mcpSlic1Present = mcpSlic2Present = mcpMainPresent = mcpMt8816Present = false;
}

bool Settings::load() {
  Preferences prefs;
  if (!prefs.begin(kNamespace, /*ro*/true)) return false;
  uint16_t v = prefs.getUShort("ver", 0);
  bool ok = (v == kVersion);
  if (ok) {
    activeLinesMask       = prefs.getUChar ("activeMask", activeLinesMask);
    debounceMs            = prefs.getUShort("debounceMs", debounceMs);

    burstTickMs           = prefs.getUInt ("burstTickMs",          burstTickMs);
    hookStableMs          = prefs.getUInt ("hookStableMs",         hookStableMs);
    hookStableConsec      = prefs.getUChar("hookStableConsec",     hookStableConsec);
    pulseGlitchMs         = prefs.getUInt ("pulseGlitchMs",        pulseGlitchMs);
    pulseLowMaxMs         = prefs.getUInt ("pulseLowMaxMs",        pulseLowMaxMs);
    digitGapMinMs         = prefs.getUInt ("digitGapMinMs",        digitGapMinMs);
    globalPulseTimeoutMs  = prefs.getUInt ("globalPulseTO",        globalPulseTimeoutMs);
    highMeansOffHook      = prefs.getBool ("highMeansOffHook",     highMeansOffHook);
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

  prefs.putUInt ("burstTickMs",          burstTickMs);
  prefs.putUInt ("hookStableMs",         hookStableMs);
  prefs.putUChar("hookStableConsec",     hookStableConsec);
  prefs.putUInt ("pulseGlitchMs",        pulseGlitchMs);
  prefs.putUInt ("pulseLowMaxMs",        pulseLowMaxMs);
  prefs.putUInt ("digitGapMinMs",        digitGapMinMs);
  prefs.putUInt ("globalPulseTO",        globalPulseTimeoutMs);
  prefs.putBool ("highMeansOffHook",     highMeansOffHook);

  prefs.end();
}

#include "Settings.h"

// Resets defaults settings
void Settings::resetDefaults() {
    debounceMs = 25;
    logging    = true;
    volume     = 5;
    activeLines = 0x0F; // Default to 4 active lines 0-3)
}

// Loading settings from NVS memory there is any
bool Settings::load() {
    Preferences prefs;
    if (!prefs.begin(kNamespace, /*readOnly=*/true)) {
        return false;
    }

    uint16_t ver = prefs.getUShort("ver", 0);
    bool hasData = (ver == kVersion);
    if (hasData) {
        debounceMs  = prefs.getUShort("debounceMs", debounceMs);
        logging     = prefs.getBool ("logging",    logging);
        volume      = prefs.getUChar("volume",     volume);
        activeLines = prefs.getUChar("activeLines", activeLines);
    }
    prefs.end();

    if (!hasData) {
        save();
    }
    return hasData;
}

// Saving settings into NVS memory
void Settings::save() const {
    Preferences prefs;
    if (!prefs.begin(kNamespace, /*readOnly=*/false)) {
        return;
    }
    prefs.putUShort("ver",        kVersion);
    prefs.putUShort("debounceMs", debounceMs);
    prefs.putBool ("logging",     logging);
    prefs.putUChar("volume",      volume);
    prefs.putUChar("activeLines", activeLines);
    prefs.end();
}

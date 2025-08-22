#include "Settings.h"

// ---- Defaults ----
void Settings::resetDefaults() {
    debounceMs = 25;
    logging    = true;
    volume     = 5;
}

// ---- Load ----
bool Settings::load() {
    Preferences prefs;
    if (!prefs.begin(kNamespace, /*readOnly=*/true)) {
        return false;
    }

    uint16_t ver = prefs.getUShort("ver", 0);
    bool hasData = (ver == kVersion);
    if (hasData) {
        debounceMs = prefs.getUShort("debounceMs", debounceMs);
        logging    = prefs.getBool ("logging",    logging);
        volume     = prefs.getUChar("volume",     volume);
    }
    prefs.end();

    if (!hasData) {
        // skriv defaults första gången
        save();
    }
    return hasData;
}

// ---- Save ----
void Settings::save() const {
    Preferences prefs;
    if (!prefs.begin(kNamespace, /*readOnly=*/false)) {
        return;
    }
    prefs.putUShort("ver",        kVersion);
    prefs.putUShort("debounceMs", debounceMs);
    prefs.putBool ("logging",     logging);
    prefs.putUChar("volume",      volume);
    prefs.end();
}

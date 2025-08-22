#include "Settings.h"

// ---- Konstruktor ----
Settings::Settings() {
    resetDefaults();  // initiera med defaultvärden
}

// ---- Defaults ----
void Settings::resetDefaults() {
    debounceMs_ = 25;
    logging_    = true;
    volume_     = 5;
}

// ---- Load ----
bool Settings::load(const char* ns) {
    Preferences prefs;
    if (!prefs.begin(ns, /*readOnly=*/true)) {
        return false;
    }

    uint16_t ver = prefs.getUShort("ver", 0);
    bool hasData = (ver == kVersion);
    if (hasData) {
        debounceMs_ = prefs.getUShort("debounceMs", debounceMs_);
        logging_    = prefs.getBool ("logging",    logging_);
        volume_     = prefs.getUChar("volume",     volume_);
    }
    prefs.end();

    if (!hasData) {
        // om inget fanns → skriv in defaults
        save(ns);
    }
    return hasData;
}

// ---- Save ----
void Settings::save(const char* ns) const {
    Preferences prefs;
    if (!prefs.begin(ns, /*readOnly=*/false)) {
        return;
    }
    prefs.putUShort("ver",        kVersion);
    prefs.putUShort("debounceMs", debounceMs_);
    prefs.putBool ("logging",     logging_);
    prefs.putUChar("volume",      volume_);
    prefs.end();
}

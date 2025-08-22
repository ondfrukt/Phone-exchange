#include "settings.h"

// ======================
// Begin / End
// ======================
bool SettingsManager::begin(const char* ns) {
    return prefs_.begin(ns, /*readOnly=*/false);
}

void SettingsManager::end() {
    prefs_.end();
}

SettingsManager::~SettingsManager() {
    prefs_.end();
}

// ======================
// Load & Save
// ======================
void SettingsManager::load(Settings& s) {
    // Läs ut varje variabel, behåll default om ej satt
    s.debounceMs = prefs_.getUShort("debounceMs", s.debounceMs);
    s.logging    = prefs_.getBool("logging",    s.logging);
    s.volume     = prefs_.getUChar("volume",    s.volume);

    // Här kan du lägga fler .getXXXX för andra variabler
}

void SettingsManager::save(const Settings& s) {
    prefs_.putUShort("debounceMs", s.debounceMs);
    prefs_.putBool("logging",      s.logging);
    prefs_.putUChar("volume",      s.volume);

    // Här kan du lägga fler .putXXXX för andra variabler
}

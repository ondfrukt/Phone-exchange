#pragma once
#include <Arduino.h>
#include <Preferences.h>

struct Settings {
    uint16_t debounceMs = 25;
    bool     logging    = true;
    uint8_t  volume     = 5;

    void load(const char* ns = "app") {
        Preferences prefs;
        prefs.begin(ns, true); // readOnly
        debounceMs = prefs.getUShort("debounceMs", debounceMs);
        logging    = prefs.getBool("logging", logging);
        volume     = prefs.getUChar("volume", volume);
        prefs.end();
    }

    void save(const char* ns = "app") const {
        Preferences prefs;
        prefs.begin(ns, false); // writeable
        prefs.putUShort("debounceMs", debounceMs);
        prefs.putBool("logging", logging);
        prefs.putUChar("volume", volume);
        prefs.end();
    }
};

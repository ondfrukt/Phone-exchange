#pragma once
#include <Arduino.h>
#include <Preferences.h>

struct Settings {
  // --- Versionering av inställningar i flash ---
  static constexpr uint16_t kVersion = 1;   // öka om du ändrar structens layout
  // --------------------------------------------

  // --- DINA INSTÄLLNINGAR (med defaults) ---
  uint16_t debounceMs = 25;
  bool     logging    = true;
  uint8_t  volume     = 5;
  // Lägg till fler fält här...

  // Återställ till defaultvärden (praktiskt från meny/CLI)
  void resetDefaults() {
    debounceMs = 25;
    logging    = true;
    volume     = 5;
    // uppdatera nya fält här om du lägger till
  }

  // Ladda från NVS. Om inget finns eller versionen saknas/fel:
  // - behåll defaults
  // - spara defaults till NVS
  // Returnerar true om befintliga värden laddades, false om defaults användes.
  bool load(const char* ns = "app") {
    Preferences prefs;
    if (!prefs.begin(ns, /*readOnly=*/true)) {
      // Kunde inte öppna – behåll defaults
      return false;
    }

    // Kolla version – om saknas/fel: första start eller layout ändrad
    uint16_t ver = prefs.getUShort("ver", 0);
    bool hasData = (ver == kVersion);
    if (hasData) {
      // Läs värden (behåller defaults om nyckel saknas)
      debounceMs = prefs.getUShort("debounceMs", debounceMs);
      logging    = prefs.getBool ("logging",    logging);
      volume     = prefs.getUChar("volume",     volume);
    }
    prefs.end();

    if (!hasData) {
      // Skriv våra defaults + version så de finns till nästa boot
      save(ns);
    }
    return hasData;
  }

  // Spara aktuella värden till NVS
  void save(const char* ns = "app") const {
    Preferences prefs;
    if (!prefs.begin(ns, /*readOnly=*/false)) {
      return;
    }
    prefs.putUShort("ver",        kVersion);
    prefs.putUShort("debounceMs", debounceMs);
    prefs.putBool ("logging",     logging);
    prefs.putUChar("volume",      volume);
    prefs.end();
  }
};

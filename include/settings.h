#pragma once
#include <Arduino.h>
#include <Preferences.h>

class Settings {
public:
    // Constructor
    // resetDefaults() in the constructor ensure that we have values on every variable, even if load is not used in setup()
    Settings() { resetDefaults(); }

    // Golabal members
    void resetDefaults();
    bool load();         // laddar från NVS, skriver defaults om inget finns
    void save() const;   // sparar till NVS

    // ---- Publika fält ----
    uint8_t activeLines;
    uint16_t debounceMs;
    bool     logging;
    uint8_t  volume;
    

private:
    // Namespace för NVS
    static constexpr const char* kNamespace = "myapp";

    // Version (öka om du ändrar structens layout)
    static constexpr uint16_t kVersion = 1;
};

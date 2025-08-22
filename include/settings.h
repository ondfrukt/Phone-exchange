#pragma once
#include <Arduino.h>
#include <Preferences.h>

// ======================
// Struct med inställningar
// ======================
struct Settings {
    uint16_t debounceMs = 25;  // exempelvärde
    bool     logging    = true;
    uint8_t  volume     = 5;
    // Lägg till fler inställningar här...
};

// ======================
// Manager-klass för load/save
// ======================
class SettingsManager {
public:
    SettingsManager() = default;

    // Initiera Preferences (namespace kan vara valfritt, "app" t.ex.)
    bool begin(const char* ns = "app");

    // Ladda inställningar från NVS till RAM
    void load(Settings& s);

    // Spara aktuella inställningar till NVS
    void save(const Settings& s);

    // Stäng Preferences (valfritt att kalla, sker i destructor också)
    void end();

    ~SettingsManager();

private:
    Preferences prefs_;
};

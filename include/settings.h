#pragma once
#include <Arduino.h>
#include <Preferences.h>

class Settings {
public:
    // ---- Konstruktor ----
    Settings();

    // ---- Offentliga metoder ----
    void resetDefaults();                // återställ standardvärden
    bool load(const char* ns = "app");   // läs från NVS, default om saknas
    void save(const char* ns = "app") const;

    // ---- Getters / setters ----
    uint16_t debounceMs() const { return debounceMs_; }
    void setDebounceMs(uint16_t ms) { debounceMs_ = ms; }

    bool logging() const { return logging_; }
    void setLogging(bool on) { logging_ = on; }

    uint8_t volume() const { return volume_; }
    void setVolume(uint8_t v) { volume_ = v; }

private:
    static constexpr uint16_t kVersion = 1;

    uint16_t debounceMs_;
    bool     logging_;
    uint8_t  volume_;
};

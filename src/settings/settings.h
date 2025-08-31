#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <stdint.h>

class Settings {
public:
  // === Singletonåtkomst ===
  static Settings& instance() {
    static Settings s;   // "Meyers singleton" – skapas vid första anrop
    return s;
  }

  // Justera activeLinesMask så den tar hänsyn till fysiska MCP-anslutningar
  void adjustActiveLines();

  // Golabal members
  void resetDefaults();   // Sets default values if no values are stored in NVS
  bool load();            // Loads saved settings from NVS if any. Returns true if successful
  void save() const;      // Saves current settings to NVS

  // ---- Publika fält ----
  uint8_t activeLinesMask;    // Bitmask for active lines (1-4)
  uint16_t debounceMs;    // Debounce time for line state changes

  // ---- Settings (justera efter din settings-klass/konstanter) ----
  uint32_t burstTickMs;           // Time for a burst tick
  uint32_t hookStableMs;          // Time for stable hook state
  uint8_t hookStableConsec;       // Number of consecutive stable readings for hook state
  uint32_t pulseGlitchMs;         // Max glitch time for pulse dialing
  uint32_t pulseLowMaxMs;         // Max low time for pulse dialing
  uint32_t digitGapMinMs;         // Min gap time between digits for pulse dialing
  uint32_t globalPulseTimeoutMs;  // Global timeout for pulse dialing
  bool highMeansOffHook;          // True if high signal means off-hook

  uint8_t allowMask;

  // ---- MCP statuses (Are not saved into NVS, just for runtime use) ----
  bool mcpSlic1Present = false;
  bool mcpSlic2Present = false;
  bool mcpMainPresent  = false;
  bool mcpMt8816Present= false;

  inline uint8_t activeLinesCount() const {
    uint8_t x = activeLinesMask, c = 0;
    while (x) { c += (x & 1u); x >>= 1; }
    return c;
  }

    inline bool isLineActive(uint8_t i) const { return (activeLinesMask >> i) & 1u; }

private:
  // === Singleton-skydd ===
  Settings();                                // gör ctor privat (definiera i .cpp eller inline)
  Settings(const Settings&) = delete;
  Settings& operator=(const Settings&) = delete;

  static constexpr const char* kNamespace = "myapp";
  static constexpr uint16_t kVersion = 1;    // höj om layout ändras
};

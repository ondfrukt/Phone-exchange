#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <stdint.h>
#include "util/UIConsole.h"

class Settings {
public:
  // === Singleton access ===
  static Settings& instance() {
    static Settings s;   // "Meyers singleton" - created on first call
    return s;
  }

  // Adjust activeLinesMask to account for physical MCP connections
  void adjustActiveLines();

  // Global members
  void resetDefaults();   // Sets default values if no values are stored in NVS
  bool load();            // Loads saved settings from NVS if any. Returns true if successful
  void save() const;      // Saves current settings to NVS

  // ---- Public fields ----
  uint8_t activeLinesMask;        // Bitmask for active lines (1-4)
  uint16_t debounceMs;            // Debounce time for line state changes

  // ---- Debugging ----
  uint8_t debugSHKLevel;          // 0=none, 1=low, 2=high
  uint8_t debugLmLevel;           // 0=none, 1=low, 2=high
  uint8_t debugWSLevel;           // 0=none, 1=low, 2=high
  uint8_t debugLALevel;           // 0=none, 1=low, 2=high
  uint8_t debugMTLevel;           // 0=none, 1=low, 2=high
  uint8_t debugMCPLevel;          // 0=none, 1=low, 2=high

  uint8_t debugI2CLevel;         // 0=none, 1=low, 2=high


  uint8_t pulseAdjustment;         // Pulse adjustment (1 means 1 pulse = 0, 2 pulses = 1, etc.)

  // ---- Settings (adjust according to your settings class/constants) ----
  uint32_t burstTickMs;           // Time for a burst tick
  uint32_t hookStableMs;          // Time for stable hook state
  uint8_t hookStableConsec;       // Number of consecutive stable readings for hook state
  uint32_t pulseGlitchMs;         // Max glitch time for pulse dialing
  uint32_t pulseLowMaxMs;         // Max low time for pulse dialing
  uint32_t digitGapMinMs;         // Min gap time between digits for pulse dialing
  uint32_t globalPulseTimeoutMs;  // Global timeout for pulse dialing
  bool highMeansOffHook;          // True if high signal means off-hook

  uint8_t allowMask;

  // ---- Line metadata ----
  String linePhoneNumbers[8];     // Configured phone number for each line

  // ---- MCP statuses (not saved to NVS, runtime only) ----
  bool mcpSlic1Present = false;
  bool mcpSlic2Present = false;
  bool mcpMainPresent  = false;
  bool mcpMt8816Present= false;

  // Timers
  unsigned long timer_Ready = 240000;
  unsigned long timer_Dialing = 5000;
  unsigned long timer_Ringing = 10000;
  unsigned long timer_pulsDialing = 3000;
  unsigned long timer_toneDialing = 3000;
  unsigned long timer_fail = 30000;
  unsigned long timer_disconnected = 60000;
  unsigned long timer_timeout = 60000;
  unsigned long timer_busy = 30000;

  inline uint8_t activeLinesCount() const {
    uint8_t x = activeLinesMask, c = 0;
    while (x) { c += (x & 1u); x >>= 1; }
    return c;
  }

  inline bool isLineActive(uint8_t i) const { return (activeLinesMask >> i) & 1u; }

private:
  // === Singleton protection ===
  Settings();                                // make ctor private (define in .cpp or inline)
  Settings(const Settings&) = delete;
  Settings& operator=(const Settings&) = delete;

  static constexpr const char* kNamespace = "myapp";
  static constexpr uint16_t kVersion = 1;    // increase if layout changes
};

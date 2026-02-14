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
  uint16_t pulseDebounceMs;            // Debounce time for line state changes
  String linePhoneNumbers[8];     // Stored phone number per line

  // ---- Debugging ----
  uint8_t debugSHKLevel;          // 0=none, 1=low, 2=high Debug level for SHK service
  uint8_t debugLmLevel;           // 0=none, 1=low, 2=high Debug level for Line Manager
  uint8_t debugWSLevel;           // 0=none, 1=low, 2=high Debug level for Web Server
  uint8_t debugLALevel;           // 0=none, 1=low, 2=high Debug level for Line Adapter
  uint8_t debugMTLevel;           // 0=none, 1=low, 2=high Debug level for MT8816 service
  uint8_t debugTRLevel;           // 0=none, 1=low, 2=high Debug level for TonReader service
  uint8_t debugMCPLevel;          // 0=none, 1=low, 2=high Debug level for MCP service
  uint8_t debugTonGenLevel;       // 0=none, 1=low, 2=high Debug level for ToneGenerator
  uint8_t debugRGLevel;           // 0=none, 1=low, 2=high Debug level for RingGenerator
  uint8_t debugIMLevel;           // 0=none, 1=low, 2=high Debug level for InterruptManager
  uint8_t debugLAC;               // 0=none, 1=low, 2=high Debug level for LineAudioConnections   

  uint8_t debugI2CLevel;         // 0=none, 1=low, 2=high

  bool    toneGeneratorEnabled = true; // Enable tone generators
  uint8_t pulseAdjustment;         // Pulse adjustment (1 means 1 pulse = 0, 2 pulses = 1, etc.)

  // ---- Settings (adjust according to your settings class/constants) ----
  uint32_t burstTickMs;           // Time for a burst tick
  uint32_t hookStableMs;          // Time for stable hook state
  uint8_t hookStableConsec;       // Number of consecutive stable readings for hook state
  uint32_t pulsGlitchMs;              // Max glitch time for pulse dialing
  uint32_t pulseLowMaxMs;         // Max low time for pulse dialing
  uint32_t digitGapMinMs;         // Min gap time between digits for pulse dialing
  uint32_t globalPulseTimeoutMs;  // Global timeout for pulse dialing
  bool highMeansOffHook;          // True if high signal means off-hook

  uint32_t ringLengthMs;          // Length of ringing signal in ms
  uint32_t ringPauseMs;           // Pause between rings in ms
  uint32_t ringIterations;        // Iterations of ringing signal

  // ---- ToneReader (DTMF) settings ----
  uint32_t dtmfDebounceMs;        // Minimum time between detections of the same digit (ms)
  uint32_t dtmfMinToneDurationMs; // Minimum tone duration to accept as valid (ms)
  uint32_t dtmfStdStableMs;       // Minimum time STD signal must be stable before reading (ms)
  uint32_t tmuxScanDwellMinMs;    // Minimum TMUX dwell time per scanned line (ms)

  uint8_t allowMask;

  // ---- MCP statuses (not saved to NVS, runtime only) ----
  bool mcpSlic1Present = false;
  bool mcpSlic2Present = false;
  bool mcpMainPresent  = false;
  bool mcpMt8816Present= false;

  // Timers
  unsigned long timer_Ready;
  unsigned long timer_Dialing;
  unsigned long timer_Ringing;
  unsigned long timer_incomming;
  unsigned long timer_pulsDialing;
  unsigned long timer_toneDialing;
  unsigned long timer_fail;
  unsigned long timer_disconnected;
  unsigned long timer_timeout;
  unsigned long timer_busy;

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
  static constexpr uint16_t kVersion = 3;    // increase if layout changes
};

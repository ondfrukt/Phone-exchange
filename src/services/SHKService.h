#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include "config.h"
#include "LineManager.h"
#include "drivers/InterruptManager.h"
#include "drivers/MCPDriver.h"
#include "settings.h"
#include "model/Types.h"

// Forward declaration to avoid circular dependency
class RingGenerator;

class SHKService {
public:
  SHKService(LineManager& lineManager, InterruptManager& interruptManager, MCPDriver& mcpDriver, Settings& settings);

  // Set RingGenerator reference (called after construction to avoid circular dependency).
  // Must be called before SHKService.update() is used to enable context-aware filtering.
  void setRingGenerator(RingGenerator* ringGenerator);

  // Kallas när appen sett att MCP rapporterat ändringar (bitmask per linje)
  void notifyLinesPossiblyChanged(uint32_t changedMask, uint32_t nowMs);

  // Anropa från app.loop()
  bool needsTick(uint32_t nowMs) const;
  bool tick(uint32_t nowMs);
  void update();

private:
  struct PerLine {
    // Hook-filter (kandidatnivå)
    bool     hookCand = true;
    uint32_t hookCandSince = 0;
    uint8_t  hookCandConsec = 0;

    // Pulsdetektor
    enum class PDState : uint8_t { Idle, InPulse, BetweenPulses } pdState = PDState::Idle;
    bool     fastLevel = true;   // snabbfiltrerad nivå till PD
    bool     lastRaw   = true;
    uint32_t rawChangeMs = 0;    // för glitchfilter
    uint32_t lowStartMs  = 0;    // start på låg-del
    uint32_t lastEdgeMs  = 0;    // senaste godkända stigande kant
    uint8_t  pulseCountWork = 0; // pulser i pågående siffra
    uint32_t blockUntilMs = 0;
  };

  // Hjälp
  char mapPulseToDigit_(uint8_t count) const;

  inline bool rawToOffHook_(bool rawHigh) const {
    // SHK=1 => OffHook om settings_.highMeansOffHook == true
    return settings_.highMeansOffHook ? rawHigh : !rawHigh;
  }

  // I/O
  uint32_t readShkMask_() const;

  // Logik
  void updateHookFilter_(int idx, bool rawHigh, uint32_t nowMs);
  void setStableHook_(int index, bool offHook, bool rawHigh, uint32_t nowMs);
  void updatePulseDetector_(int idx, bool rawHigh, uint32_t nowMs);
  bool pulseModeAllowed_(const LineHandler& line) const;
  void pulseFalling_(int idx, uint32_t nowMs);
  void pulseRising_(int idx, uint32_t nowMs);
  void emitDigitAndReset_(int idx, bool rawHigh, uint32_t nowMs);
  void resetPulseState_(int idx);
  void resyncFast_(int idx, bool rawHigh, uint32_t nowMs);

private:
  LineManager& lineManager_;
  InterruptManager& interruptManager_;
  MCPDriver&   mcpDriver_;
  Settings&    settings_;
  RingGenerator* ringGenerator_ = nullptr;

  std::vector<PerLine> lineState_;

  uint32_t activeMask_ = 0;
  bool     burstActive_     = false;
  uint32_t burstNextTickAtMs_ = 0;
  std::size_t maxPhysicalLines_ = 8;

};

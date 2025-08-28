// include/telephony/SHKService.h
#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include "LineManager.h"
#include "drivers/MCPDriver.h"
#include "settings.h"
#include "model/Types.h"

class SHKService {
public:
  SHKService(LineManager& lineManager, MCPDriver& mcpDriver, Settings& settings);

  // Kallas när appen sett att MCP rapporterat ändringar (bitmask per linje)
  void notifyLinesPossiblyChanged(uint32_t changedMask, uint32_t nowMs);

  // Anropa från app.loop()
  bool needsTick(uint32_t nowMs) const;
  bool tick(uint32_t nowMs);

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
  };

  // Hjälp
  static inline char mapPulseCountToDigit(uint8_t count) {
    uint8_t p = count % 10;
    return (p == 0) ? '0' : static_cast<char>('0' + p);
  }
  inline bool rawToOffHook_(bool rawHigh) const {
    // SHK=1 => OffHook om settings_.highMeansOffHook == true
    return settings_.highMeansOffHook ? rawHigh : !rawHigh;
  }

  // I/O
  uint32_t readShkMask_() const;

  // Logik
  void updateHookFilter_(int idx, bool rawHigh, uint32_t nowMs);
  void setStableHook_(int idx, bool offHook);

  void updatePulseDetector_(int idx, bool rawHigh, uint32_t nowMs);
  bool pulseModeAllowed_(const LineHandler& line) const;

  void pulseFalling_(int idx, uint32_t nowMs);
  void pulseRising_(int idx, uint32_t nowMs);
  void emitDigitAndReset_(int idx);
  void resetPulseState_(int idx);

private:
  LineManager& lineManager_;
  MCPDriver&   mcpDriver_;
  Settings&    settings_;

  std::vector<PerLine> lineState_;
  std::size_t  activeLines_ = 0;

  uint32_t activeMask_ = 0;
  bool     burstActive_     = false;
  uint32_t burstNextTickAtMs_ = 0;
  std::size_t maxPhysicalLines_ = 4;

};

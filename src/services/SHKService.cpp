// src/telephony/SHKService.cpp
#include "SHKService.h"
#include "config.h"


// Constructor. Takes references to LineManager, MCPDriver and Settings instances.
SHKService::SHKService(LineManager& lineManager, MCPDriver& mcpDriver, Settings& settings)
: lineManager_(lineManager), mcpDriver_(mcpDriver), settings_(settings) {
  
  // Reads active lines from settings, ensures at least one line is active
  maxPhysicalLines_ = cfg::mcp::SHK_LINE_COUNT;
  lineState_.resize(maxPhysicalLines_);
  activeLines_ = settings_.activeLinesMask;
  if (activeLines_ > maxPhysicalLines_) activeLines_ = maxPhysicalLines_;

  // Initial read of SHK states. Assumes stable at startup.
  uint32_t raw = readShkMask_();
  for (std::size_t i=0;i<activeLines_;++i) {
    bool rawHigh = (raw >> i) & 0x1;
    auto& s = lineState_[i];
    s.hookCand  = rawHigh;
    s.fastLevel = rawHigh;
    s.lastRaw   = rawHigh;

    auto& line = lineManager_.getLine((int)i);
    bool offHook = rawToOffHook_(rawHigh);
    line.SHK = offHook;
    line.previousHookStatus = line.currentHookStatus;
    line.currentHookStatus  = offHook ? model::HookStatus::Off : model::HookStatus::On;
  }
}

// Member to notifi when MCP reports changes (bitmask per line)
void SHKService::notifyLinesPossiblyChanged(uint32_t changedMask, uint32_t nowMs) {
  std::size_t a = settings_.activeLinesMask;
  if (a > maxPhysicalLines_) a = maxPhysicalLines_;
  uint32_t allowMask = (a==32) ? 0xFFFFFFFFu : ((1u << a) - 1u);

  changedMask &= allowMask;
  if (!changedMask) return;

  activeMask_   |= changedMask;
  burstActive_   = true;
  if (nowMs >= burstNextTickAtMs_) burstNextTickAtMs_ = nowMs;
}


// Chek if it's time for a tick (returns true if so)
bool SHKService::needsTick(uint32_t nowMs) const {
  return burstActive_ && (nowMs >= burstNextTickAtMs_);
}

// Perform a tick if needed (returns true if tick was done)
// A tick processes all active lines (hook filtering and pulse detection)
// If no lines remain active after processing, the service goes idle.
// Otherwise, it schedules the next tick after settings_.burstTickMs
bool SHKService::tick(uint32_t nowMs) {

  activeLines_ = settings_.activeLinesMask;
  if (activeLines_ > maxPhysicalLines_) activeLines_ = maxPhysicalLines_;

  if (!burstActive_ || nowMs < burstNextTickAtMs_) return false;

  uint32_t rawMask = readShkMask_();
  uint32_t nextActiveMask = 0;

  for (std::size_t lineIndex = 0; lineIndex < maxPhysicalLines_; ++lineIndex) {
    // Bearbeta endast de logiskt aktiva
    if (lineIndex >= activeLines_) {
      resetPulseState_(static_cast<int>(lineIndex));
      continue;
    }

    // ev. require att LineManager säger att linjen är aktiv
    auto& line = lineManager_.getLine(static_cast<int>(lineIndex));
    if (!line.lineActive) {
      resetPulseState_(static_cast<int>(lineIndex));
      continue;
    }

    bool rawHigh = (rawMask >> lineIndex) & 0x1U;
    updateHookFilter_(static_cast<int>(lineIndex), rawHigh, nowMs);
    updatePulseDetector_(static_cast<int>(lineIndex), rawHigh, nowMs);

    const auto& s = lineState_[lineIndex];
    bool hookUnstable =
      ((settings_.hookStableConsec > 0 && s.hookCandConsec < settings_.hookStableConsec) ||
       ((nowMs - s.hookCandSince) < settings_.hookStableMs));
    bool pdActive = (s.pdState != PerLine::PDState::Idle);

    if (hookUnstable || pdActive) nextActiveMask |= (1u << lineIndex);
  }

  activeMask_ = nextActiveMask;
  if (activeMask_ == 0) {
    burstActive_ = false;
  } else {
    burstNextTickAtMs_ = nowMs + settings_.burstTickMs;
  }

  return true;
}

// Read SHK pin states from MCP and return as bitmask
uint32_t SHKService::readShkMask_() const {
  // 1) Samla distinkta adresser som faktiskt förekommer i tabellen
  uint8_t addrs[2]  = {0}; // vi har max två SLIC
  uint16_t gpio[2]  = {0};
  bool have[2]      = {false,false};
  int addrCount = 0;

  auto findOrAdd = [&](uint8_t a)->int {
    for (int i=0;i<addrCount;++i) if (addrs[i]==a) return i;
    if (addrCount < 2) { addrs[addrCount]=a; return addrCount++; }
    return -1;
  };

  for (std::size_t i=0;i<maxPhysicalLines_; ++i) {
    (void)findOrAdd(cfg::mcp::SHK_LINE_ADDR[i]);
  }

  // 2) Läs varje unik adress EN gång (16-bit GPIO)
  for (int k=0; k<addrCount; ++k) {
    uint16_t g = 0;
    if (mcpDriver_.readGpioAB16(addrs[k], g)) {
      have[k] = true;
      gpio[k] = g;
    } else {
      have[k] = false; // bank saknas/offline
    }
  }

  // 3) Bygg mask per global linje
  uint32_t mask = 0;
  for (std::size_t i=0;i<maxPhysicalLines_; ++i) {
    uint8_t addr = cfg::mcp::SHK_LINE_ADDR[i];
    uint8_t pin  = cfg::mcp::SHK_PINS[i];

    int bank = -1;
    for (int k=0;k<addrCount;++k) if (addrs[k]==addr) { bank=k; break; }
    if (bank<0 || !have[bank]) continue;       // MCP saknas → skippa linjen

    bool val = ( (gpio[bank] >> pin) & 0x1 ) != 0;
    if (val) mask |= (1u << i);
  }
  return mask;
}

// ---------------- Hook-filter ----------------
// Updates hook filter state for line 'idx' with new raw reading 'rawHigh' at time 'nowMs'
void SHKService::updateHookFilter_(int idx, bool rawHigh, uint32_t nowMs) {
  auto& s = lineState_[idx];

  // Spåra kandidatnivå och hur länge den stått stabil
  if (s.hookCand != rawHigh) {
    s.hookCand = rawHigh;
    s.hookCandSince = nowMs;
    s.hookCandConsec = 1;
  } else if (s.hookCandConsec < 255) {
    s.hookCandConsec++;
  }

  bool timeOk   = (nowMs - s.hookCandSince) >= settings_.hookStableMs;
  bool consecOk = (settings_.hookStableConsec == 0) || (s.hookCandConsec >= settings_.hookStableConsec);
  if (timeOk && consecOk) {
    bool offHook = rawToOffHook_(s.hookCand);
    setStableHook_(idx, offHook);
  }
}

void SHKService::setStableHook_(int lineIndex, bool offHook) {
  auto& line = lineManager_.getLine(lineIndex);

  line.SHK = offHook; // 1 = off-hook
  line.previousHookStatus = line.currentHookStatus;
  line.currentHookStatus  = offHook ? model::HookStatus::Off : model::HookStatus::On;

  // Om vi gick till OnHook -> nollställ pulsdelen
  if (!offHook) {
    resetPulseState_(lineIndex);
  }
}

// ---------------- Pulsdetektor ----------------
bool SHKService::pulseModeAllowed_(const LineHandler& line) const {
  if (line.currentHookStatus != model::HookStatus::Off) return false;
  return (line.currentLineStatus == model::LineStatus::Ready ||
          line.currentLineStatus == model::LineStatus::PulseDialing);
}

void SHKService::updatePulseDetector_(int idx, bool rawHigh, uint32_t nowMs) {
  auto& s = lineState_[idx];
  auto& line = lineManager_.getLine(idx);

  // Kör bara i Ready/PulseDialing och OffHook
  if (!pulseModeAllowed_(line)) {
    resetPulseState_(idx);
    line.gap = line.edge ? (nowMs - line.edge) : 0;
    return;
  }

  // Glitch-filter (små spikar ignoreras) baserat på settings_.pulseGlitchMs
  if (rawHigh != s.lastRaw) { s.lastRaw = rawHigh; s.rawChangeMs = nowMs; }
  bool accept = (nowMs - s.rawChangeMs) >= settings_.pulseGlitchMs;

  if (accept && (rawHigh != s.fastLevel)) {
    s.fastLevel = rawHigh;

    if (!s.fastLevel) {
      // Hög -> Låg : start puls
      pulseFalling_(idx, nowMs);
    } else {
      // Låg -> Hög : slut puls
      pulseRising_(idx, nowMs);
    }
  }

  // Siffra klar (digit-gap)
  if (s.pdState == PerLine::PDState::BetweenPulses) {
    uint32_t sinceRise = nowMs - s.lastEdgeMs;
    if (sinceRise >= settings_.digitGapMinMs) {
      emitDigitAndReset_(idx);
      return;
    }
  }

  // Timeout-skydd för hängande sekvens
  if (s.pdState != PerLine::PDState::Idle) {
    uint32_t since = nowMs - s.lastEdgeMs;
    if (since >= settings_.globalPulseTimeoutMs) {
      resetPulseState_(idx);
      return;
    }
  }

  // Uppdatera gap för diag
  line.gap = line.edge ? (nowMs - line.edge) : 0;
}

void SHKService::pulseFalling_(int idx, uint32_t nowMs) {
  auto& s = lineState_[idx];
  auto& line = lineManager_.getLine(idx);

  if (s.pdState == PerLine::PDState::Idle || s.pdState == PerLine::PDState::BetweenPulses) {
    s.pdState = PerLine::PDState::InPulse;
    s.lowStartMs = nowMs;

    line.pulsing = true;
    line.pulsingFlag = true;
    line.edge = nowMs;
  }
}

void SHKService::pulseRising_(int idx, uint32_t nowMs) {
  auto& s = lineState_[idx];
  auto& line = lineManager_.getLine(idx);

  if (s.pdState == PerLine::PDState::InPulse) {
    uint32_t lowDur = nowMs - s.lowStartMs; // låg-bredd
    s.lastEdgeMs = nowMs;                    // stigande kant

    // Validera puls-lågtid: min = settings_.debounceMs, max = settings_.pulseLowMaxMs
    if (lowDur >= settings_.debounceMs && lowDur <= settings_.pulseLowMaxMs) {
      if (s.pulseCountWork < 200) s.pulseCountWork++;
      line.pulsCount = s.pulseCountWork;    // synligt för diagnos
    } else {
      resetPulseState_(idx);
      return;
    }

    s.pdState = PerLine::PDState::BetweenPulses;
    line.pulsing = false;                   // lågdel avslutad
    line.edge = nowMs;
  }
}

void SHKService::emitDigitAndReset_(int idx) {
  auto& s = lineState_[idx];
  auto& line = lineManager_.getLine(idx);

  if (s.pulseCountWork > 0) {
    char d = mapPulseCountToDigit(s.pulseCountWork); // 10→'0'
    line.dialedDigits += d; // lägg direkt i LineHandler
  }
  resetPulseState_(idx);
}

void SHKService::resetPulseState_(int idx) {
  auto& s = lineState_[idx];
  auto& line = lineManager_.getLine(idx);

  s.pdState = PerLine::PDState::Idle;
  s.pulseCountWork = 0;
  s.lowStartMs = 0;
  s.lastEdgeMs = 0;

  line.pulsing = false;
  line.pulsingFlag = false;
}

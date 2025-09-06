// src/telephony/SHKService.cpp
#include "SHKService.h"

// Constructor. Takes references to LineManager, MCPDriver and Settings instances.
SHKService::SHKService(LineManager& lineManager, MCPDriver& mcpDriver, Settings& settings)
: lineManager_(lineManager), mcpDriver_(mcpDriver), settings_(settings) {
  
  // Reads active lines from settings, ensures at least one line is active
  maxPhysicalLines_ = cfg::mcp::SHK_LINE_COUNT;
  
  activeLines_ = 0;
  for (std::size_t i = 0; i < maxPhysicalLines_; ++i) {
    if ((settings_.activeLinesMask >> i) & 1u) {
      activeLines_ = i + 1;
    }
  }
  
  lineState_.resize(maxPhysicalLines_);

  // Initial read of SHK states. Assumes stable at startup.
  uint32_t raw = readShkMask_();
  for (std::size_t i = 0; i < maxPhysicalLines_; ++i) {
    auto& line = lineManager_.getLine((int)i);
    if(!line.lineActive) continue;
    bool rawHigh = (raw >> i) & 0x1;
    auto& s = lineState_[i];
    s.hookCand  = rawHigh;
    s.fastLevel = rawHigh;
    s.lastRaw   = rawHigh;

    bool offHook = rawToOffHook_(rawHigh);
    line.SHK = offHook;
    line.previousHookStatus = line.currentHookStatus;
    line.currentHookStatus  = offHook ? model::HookStatus::Off : model::HookStatus::On;
  }
}

// Member to notifi when MCP reports changes (bitmask per line)
void SHKService::notifyLinesPossiblyChanged(uint32_t changedMask, uint32_t nowMs) {
  uint32_t allowMask = settings_.activeLinesMask & settings_.allowMask;
  changedMask &= allowMask;
  if (!changedMask) return;

  activeMask_ |= changedMask;
  burstActive_ = true;
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

  activeLines_ = 0;
  for (std::size_t i = 0; i < maxPhysicalLines_; ++i) {
    if ((settings_.activeLinesMask >> i) & 1u) {
      activeLines_ = i + 1;
    }
  }

  if (!burstActive_ || nowMs < burstNextTickAtMs_) return false;

  uint32_t rawMask = readShkMask_();
  uint32_t nextActiveMask = 0;

  for (std::size_t lineIndex = 0; lineIndex < maxPhysicalLines_; ++lineIndex) {
    // Bearbeta bara linjer som nyss ändrats/är aktiva i burst…
    if ((activeMask_ & (1u << lineIndex)) == 0) {
      // …men nollställ gärna ev. pulstillstånd:
      resetPulseState_(static_cast<int>(lineIndex));
      continue;
    }

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

    // Fortsätt bara ticka de linjer som ännu inte “lagt sig”
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
// Läs SHK‑ingångar och returnera rå bitmask (1 = ingång hög)
uint32_t SHKService::readShkMask_() const {
  uint32_t mask = 0;

  // 1) Bygg en lista över SLIC‑adresser som behövs utifrån allowMask
  uint8_t addrs[2] = {0};
  uint16_t gpio[2] = {0};
  bool have[2] = {false, false};
  int addrCount = 0;

  auto findOrAdd = [&](uint8_t a) -> int {
      for (int i = 0; i < addrCount; ++i) {
          if (addrs[i] == a) return i;
      }
      if (addrCount < 2) {
          addrs[addrCount] = a;
          return addrCount++;
      }
      return -1;
  };

  // Samla bara adresser för linjer som är tillåtna (allowMask)
  for (std::size_t i = 0; i < maxPhysicalLines_; ++i) {
      if ((settings_.allowMask & (1u << i)) != 0) {
          (void)findOrAdd(cfg::mcp::SHK_LINE_ADDR[i]);
      }
  }

  // 2) Läs varje unik SLIC‑adress om den faktiskt finns
  for (int k = 0; k < addrCount; ++k) {
      uint8_t a = addrs[k];
      bool present = true;
      // Koppla adresser till närvaroflaggor
      if (a == cfg::mcp::MCP_SLIC1_ADDRESS && !settings_.mcpSlic1Present) {
          present = false;
      }
      if (a == cfg::mcp::MCP_SLIC2_ADDRESS && !settings_.mcpSlic2Present) {
          present = false;
      }
      if (!present) {
          have[k] = false;       // hoppa över läsning om kretsen saknas
          continue;
      }
      uint16_t g = 0;
      if (mcpDriver_.readGpioAB16(a, g)) {
          have[k] = true;
          gpio[k] = g;
      } else {
          // om läsningen misslyckas betraktas banken som saknad
          have[k] = false;
      }
  }
  // 3) Bygg rå mask för varje tillåten linje
  for (std::size_t i = 0; i < maxPhysicalLines_; ++i) {
      if ((settings_.allowMask & (1u << i)) == 0) {
          continue;                       // linjen är inte tillåten
      }
      uint8_t addr = cfg::mcp::SHK_LINE_ADDR[i];
      uint8_t pin  = cfg::mcp::SHK_PINS[i];
      // hitta index för aktuell adress i addrs[]
      int bank = -1;
      for (int k = 0; k < addrCount; ++k) {
          if (addrs[k] == addr) {
              bank = k;
              break;
          }
      }
      // hoppa över linjen om banken saknas/offline
      if (bank < 0 || !have[bank]) {
          continue;
      }
      bool val = ((gpio[bank] >> pin) & 0x1U) != 0U;   // läs bit från 16‑bitars GPIO
      if (val) {
          mask |= (1u << i);  // sätt bit i rå mask om nivån är hög
      }
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

    // Om vi går till OnHook: committa siffra om vi var mellan pulser
    if (!offHook) { // OnHook
      if (s.pdState == PerLine::PDState::BetweenPulses && s.pulseCountWork > 0) {
        emitDigitAndReset_(idx, rawHigh, nowMs);
      } else if (s.pdState == PerLine::PDState::InPulse) {
        // Avbruten puls på lägg-på → kasta den
        resetPulseState_(idx);
      }
    }
    setStableHook_(idx, offHook, rawHigh, nowMs);
  }
}

void SHKService::setStableHook_(int index, bool offHook, bool rawHigh, uint32_t nowMs) {
  auto& line = lineManager_.getLine(index);

  model::HookStatus newHook = offHook ? model::HookStatus::Off : model::HookStatus::On;
  if (newHook != line.currentHookStatus) {
    line.currentHookStatus = newHook;
    line.SHK = offHook;
    lineManager_.setStatus(index, offHook ? model::LineStatus::Ready : model::LineStatus::Idle);

    // resync fast först NÄR hook-läget faktiskt har bytts:
    resyncFast_(index, rawHigh, nowMs);

    if (!offHook) resetPulseState_(index);  // gick till OnHook
  }
}

// ---------------- Pulsdetektor ----------------
bool SHKService::pulseModeAllowed_(const LineHandler& line) const {
  if (line.currentHookStatus != model::HookStatus::Off) return false;
  return (line.currentLineStatus == model::LineStatus::Ready ||
          line.currentLineStatus == model::LineStatus::PulseDialing);
}

void SHKService::updatePulseDetector_(int idx, bool rawHigh, uint32_t nowMs) {
  
  if (settings_.debugSHKLevel >= 2){
    Serial.printf("SHKService: Line %d updatePulseDetector_ rawHigh=%d\n", (int)idx, (int)rawHigh);
  }

  auto& s   = lineState_[idx];
  auto& line = lineManager_.getLine(idx);

  if (nowMs < s.blockUntilMs) {
    // hoppa helt över pulsdelen en liten stund efter förra siffran
    return;
  }

  // 1) Kör bara i rätt läge. Men: avbryt inte en pågående puls.
  if (!pulseModeAllowed_(line)) {
    if (s.pdState == PerLine::PDState::BetweenPulses && s.pulseCountWork > 0) {
      emitDigitAndReset_(idx, rawHigh, nowMs);                  // committa ev. siffra vid t.ex. OnHook
      return;
    }
    if (s.pdState != PerLine::PDState::InPulse) {
      resetPulseState_(idx);
      line.gap = line.edge ? (nowMs - line.edge) : 0;
      return;
    }
    // Är vi mitt i en puls? Låt den få slutföras (ingen reset här).
  }

  // 2) Glitchfilter
  if (rawHigh != s.lastRaw) { s.lastRaw = rawHigh; s.rawChangeMs = nowMs; }
  bool accept = (nowMs - s.rawChangeMs) >= settings_.pulseGlitchMs;

  // 3) Kantdetektering (OBS: rätt ordning och INTE beroende av debug)
  if (accept && (rawHigh != s.fastLevel)) {
    s.fastLevel = rawHigh;

    if (!s.fastLevel) {
      // High -> Low = start av puls
      pulseFalling_(idx, nowMs);
    } else {
      // Low -> High = slut av puls
      pulseRising_(idx, nowMs);
    }
  }   // <-- VIKTIG: stäng kant-blocket här!

  // 4) Digit-gap: körs varje tick (inte bara när kant sker)
  if (s.pdState == PerLine::PDState::BetweenPulses) {
    uint32_t sinceRise = nowMs - s.lastEdgeMs;
    if (settings_.debugSHKLevel >= 2) {
      Serial.printf("SHKService: L%d gap=%u ms (thr=%u)\n",
                    idx, (unsigned)sinceRise, (unsigned)settings_.digitGapMinMs);
    }
    if (sinceRise >= settings_.digitGapMinMs) {
      emitDigitAndReset_(idx, rawHigh, nowMs);
      return;
    }
  }

  // 5) Timeout: state-medveten
  if (s.pdState == PerLine::PDState::InPulse) {
    uint32_t since = nowMs - s.lowStartMs;     // hur länge "lågt" pågår
    if (since >= settings_.globalPulseTimeoutMs) {
      resetPulseState_(idx);                   // avbruten/trasig puls
      return;
    }
  } else if (s.pdState == PerLine::PDState::BetweenPulses) {
    uint32_t since = nowMs - s.lastEdgeMs;     // hur länge sedan sist stigande kant
    if (since >= settings_.globalPulseTimeoutMs) {
      emitDigitAndReset_(idx, rawHigh, nowMs);                 // committa siffran ändå
      return;
    }
  }

  // 6) Diag
  line.gap = line.edge ? (nowMs - line.edge) : 0;
}

void SHKService::pulseFalling_(int idx, uint32_t nowMs) {
  auto& line = lineManager_.getLine(idx);
  if (line.currentHookStatus != model::HookStatus::Off) {
    return; // På OnHook ska inga pulser startas
  }
  auto& s = lineState_[idx];
  if (s.pdState == PerLine::PDState::Idle || s.pdState == PerLine::PDState::BetweenPulses) {
    s.pdState   = PerLine::PDState::InPulse;
    s.lowStartMs = nowMs;
    s.lastEdgeMs = nowMs;
    if (settings_.debugSHKLevel >= 2)
      Serial.printf("SHKService: Line %d pulse falling \n", idx);
  }
}

void SHKService::pulseRising_(int idx, uint32_t nowMs) {
  auto& s = lineState_[idx];
  auto& line = lineManager_.getLine(idx);

  if (s.pdState == PerLine::PDState::InPulse) {
    uint32_t lowDur = nowMs - s.lowStartMs; // låg-bredd
    s.lastEdgeMs = nowMs;                   // stigande kant
    if (settings_.debugSHKLevel >= 2) {
      Serial.printf("SHKService: Line %d pulse low duration %d ms\n", (int)idx, (int)lowDur);
    }

    // Validera puls-lågtid: min = settings_.debounceMs, max = settings_.pulseLowMaxMs
    if (lowDur >= settings_.debounceMs && lowDur <= settings_.pulseLowMaxMs) {
      
      if (s.pulseCountWork == 0 &&
          line.currentHookStatus == model::HookStatus::Off &&
          line.currentLineStatus == model::LineStatus::Ready) {
        lineManager_.setStatus(idx, model::LineStatus::PulseDialing);
      }
      s.pulseCountWork += 1;
      s.pdState     = PerLine::PDState::BetweenPulses;
      s.lastEdgeMs  = nowMs;           // starta digit-gap-mätning

    if (settings_.debugSHKLevel >= 1) {
      Serial.printf("SHKService: Line %d pulsCountWork %d \n", (int)idx, (int)s.pulseCountWork);
    }

    } else {
      resetPulseState_(idx);
      return;
    }
  }
}

void SHKService::emitDigitAndReset_(int idx, bool rawHigh, uint32_t nowMs) {
  auto& s = lineState_[idx];
  auto& line = lineManager_.getLine(idx);

  if (s.pulseCountWork > 0) {
    char d = mapPulseToDigit_(s.pulseCountWork); // 10→'0'
    line.dialedDigits += d; // lägg direkt i LineHandler
    Serial.printf("SHKService: Line %d digit '%c' (pulses=%d)\n", (int)idx, d, (int)s.pulseCountWork);
    Serial.printf("SHKService: Line %d dialedDigits now: %s\n", (int)idx, line.dialedDigits.c_str());
  }
  resetPulseState_(idx);
  s.blockUntilMs = nowMs + 80;
  resyncFast_(idx, rawHigh, nowMs);
}

void SHKService::resetPulseState_(int idx) {
  auto& s = lineState_[idx];
  auto& line = lineManager_.getLine(idx);

  s.pdState = PerLine::PDState::Idle;
  s.pulseCountWork = 0;
  s.lowStartMs = 0;
  s.lastEdgeMs = 0;

}

void SHKService::resyncFast_(int idx, bool rawHigh, uint32_t nowMs) {
  auto& s = lineState_[idx];
  s.lastRaw     = rawHigh;
  s.fastLevel   = rawHigh;
  s.rawChangeMs = nowMs;
}

char SHKService::mapPulseToDigit_(uint8_t count) const {
  uint8_t p = count % 10;               // p ∈ {0..9} där 0 betyder “10 pulser”
  if (settings_.pulseAdjustment == 1) {
    // Svensk typ: 0 = 1 puls, 1 = 2 pulser, ..., 9 = 10 pulser
    // => digit = (p == 0) ? 9 : (p - 1)
    uint8_t d = (p == 0) ? 9 : (p - 1);
    return static_cast<char>('0' + d);
  } else {
    // Standard (decadic): 1..9 => 1..9, 10 => 0
    return (p == 0) ? '0' : static_cast<char>('0' + p);
  }
}
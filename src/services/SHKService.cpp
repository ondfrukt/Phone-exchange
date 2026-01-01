#include "SHKService.h"

// Constants for hook detection during pulse dialing
namespace {
  // Additional margin time (ms) added to pulseLowMaxMs when pulse detector is active.
  // This ensures hook changes are distinguished from pulse low states.
  // With pulseLowMaxMs=150ms and kPulseMarginMs=50ms, we require 200ms stability
  // during pulse dialing, which is well beyond the maximum pulse duration (150ms).
  constexpr uint32_t kPulseMarginMs = 50;
}

// Constructor: Initializes SHKService with references to LineManager, InterruptManager, MCPDriver, and Settings.
SHKService::SHKService(LineManager& lineManager, InterruptManager& interruptManager, MCPDriver& mcpDriver, Settings& settings, RingGenerator& ringGenerator)
: lineManager_(lineManager), interruptManager_(interruptManager), mcpDriver_(mcpDriver), settings_(settings), ringGenerator_(ringGenerator){

  // Set maximum number of physical lines.
  maxPhysicalLines_ = cfg::mcp::SHK_LINE_COUNT;

  // Prepare state storage for each line.
  lineState_.resize(maxPhysicalLines_);

  // Initial read of SHK states; assumes stable at startup.
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

// Notifies SHKService when MCP reports changes (bitmask per line).
void SHKService::notifyLinesPossiblyChanged(uint32_t changedMask, uint32_t nowMs, bool value) {
  uint32_t allowMask = settings_.activeLinesMask & settings_.allowMask;
  changedMask &= allowMask;
  if (!changedMask) return;

  if (settings_.debugSHKLevel >= 2) {
    Serial.printf("SHKService: notifyLinesPossiblyChanged mask=0x%X at %u ms, value=%d\n", changedMask, nowMs, value);
    Serial.flush();  // Ensure immediate output
    util::UIConsole::log("notifyLinesPossiblyChanged mask=0x" + String(changedMask, HEX) + " at " + String(nowMs) + " ms", "SHKService");
  }

  activeMask_ |= changedMask;
  burstActive_ = true;
  if (nowMs >= burstNextTickAtMs_) burstNextTickAtMs_ = nowMs;
}

// Checks if it's time for a tick (returns true if so).
bool SHKService::needsTick(uint32_t nowMs) const {
  return burstActive_ && (nowMs >= burstNextTickAtMs_);
}

// Processes a tick if needed (returns true if tick was done).
// A tick processes all active lines (hook filtering and pulse detection).
// If no lines remain active after processing, the service goes idle.
// Otherwise, schedules the next tick after settings_.burstTickMs.
bool SHKService::tick(uint32_t nowMs) {

  if (!burstActive_ || nowMs < burstNextTickAtMs_) return false;

  uint32_t rawMask = readShkMask_();
  uint32_t nextActiveMask = 0;

  for (std::size_t lineIndex = 0; lineIndex < maxPhysicalLines_; ++lineIndex) {
    // Process only lines that have recently changed or are active in burst.
    if ((activeMask_ & (1u << lineIndex)) == 0) {
      // Reset pulse state for inactive lines.
      resetPulseState_(static_cast<int>(lineIndex));
      continue;
    }

    auto& line = lineManager_.getLine(static_cast<int>(lineIndex));
    if (!line.lineActive) {
      resetPulseState_(static_cast<int>(lineIndex));
      continue;
    }

    bool rawHigh = (rawMask >> lineIndex) & 0x1U;
    uint32_t hookStableMs = settings_.hookStableMs;
    updateHookFilter_(static_cast<int>(lineIndex), rawHigh, nowMs, hookStableMs);
    updatePulseDetector_(static_cast<int>(lineIndex), rawHigh, nowMs);

    const auto& s = lineState_[lineIndex];

    bool hookUnstable =
      ((settings_.hookStableConsec > 0 && s.hookCandConsec < settings_.hookStableConsec) ||
      ((nowMs - s.hookCandSince) < hookStableMs));
    bool pdActive = (s.pdState != PerLine::PDState::Idle);

    // Continue ticking lines that are not yet stable.
    if (hookUnstable || pdActive) nextActiveMask |= (1u << lineIndex);
  }
  activeMask_ = nextActiveMask;
  if (activeMask_ == 0) {
    burstActive_ = false;
    if (settings_.debugSHKLevel >= 2) {
      Serial.println("SHKService: burst finished, going idle");
      Serial.flush();  // Ensure immediate output
      util::UIConsole::log("burst finished, going idle", "SHKService");
    }
  } else {
    burstNextTickAtMs_ = nowMs + settings_.burstTickMs;
  }

  return true;
}

// Handles interrupts and triggers line change notifications.
void SHKService::update() {
  // Poll all SLIC1 events
  while (true) {
    IntResult r = interruptManager_.pollEventByAddress(cfg::mcp::MCP_SLIC1_ADDRESS);
    if (!r.hasEvent) break;

    // Ignore SHK changes during ringing due to a interference error.
    if (ringGenerator_.lineStates_[r.line].state == model::RingState::RingToggling) {
      continue;
    }

    if (r.line < 8) {
      uint32_t mask = (1u << r.line);
      notifyLinesPossiblyChanged(mask, millis(), r.level);
      yield();
    }
  }
  
  // Poll all SLIC2 events
  while (true) {
    IntResult r = interruptManager_.pollEventByAddress(cfg::mcp::MCP_SLIC2_ADDRESS);
    if (!r.hasEvent) break;

    // Ignore SHK changes during ringing due to a interference error.
    if (ringGenerator_.lineStates_[r.line].state == model::RingState::RingToggling) {
      continue;
    }

    if (r.line < 8) {
      uint32_t mask = (1u << r.line);
      notifyLinesPossiblyChanged(mask, millis(), r.level);
      yield();
    }
  }

  uint32_t nowMs = millis();
  if (needsTick(nowMs)) {
    tick(nowMs); // Updates hook status and pulses.
  }
}

// Reads SHK pin states from MCP and returns as bitmask (1 = input high).
uint32_t SHKService::readShkMask_() const {
  uint32_t mask = 0;

  // Build a list of required SLIC addresses based on allowMask.
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

  // Collect addresses for allowed lines only.
  for (std::size_t i = 0; i < maxPhysicalLines_; ++i) {
    if ((settings_.allowMask & (1u << i)) != 0) {
      (void)findOrAdd(cfg::mcp::SHK_LINE_ADDR[i]);
    }
  }

  // Read each unique SLIC address if present.
  for (int k = 0; k < addrCount; ++k) {
    uint8_t a = addrs[k];
    bool present = true;
    // Check presence flags for addresses.
    if (a == cfg::mcp::MCP_SLIC1_ADDRESS && !settings_.mcpSlic1Present) {
      present = false;
    }
    if (a == cfg::mcp::MCP_SLIC2_ADDRESS && !settings_.mcpSlic2Present) {
      present = false;
    }
    if (!present) {
      have[k] = false; // Skip reading if chip is missing.
      continue;
    }
    uint16_t g = 0;
    if (mcpDriver_.readGpioAB16(a, g)) {
      have[k] = true;
      gpio[k] = g;
    } else {
      // If reading fails, treat bank as missing.
      have[k] = false;
    }
  }

  // Build raw mask for each allowed line.
  for (std::size_t i = 0; i < maxPhysicalLines_; ++i) {
    if ((settings_.allowMask & (1u << i)) == 0) {
      continue; // Line not allowed.
    }
    uint8_t addr = cfg::mcp::SHK_LINE_ADDR[i];
    uint8_t pin  = cfg::mcp::SHK_PINS[i];
    // Find index for current address in addrs[].
    int bank = -1;
    for (int k = 0; k < addrCount; ++k) {
      if (addrs[k] == addr) {
        bank = k;
        break;
      }
    }
    // Skip line if bank is missing/offline.
    if (bank < 0 || !have[bank]) {
      continue;
    }
    bool val = ((gpio[bank] >> pin) & 0x1U) != 0U; // Read bit from 16-bit GPIO.
    if (val) {
      mask |= (1u << i); // Set bit in raw mask if level is high.
    }
  }
  return mask;
}

// ---------------- Hook Filter ----------------
// Updates hook filter state for line 'idx' with new raw reading 'rawHigh' at time 'nowMs'.
void SHKService::updateHookFilter_(int idx, bool rawHigh, uint32_t nowMs, uint32_t hookStableMs) {
  auto& s = lineState_[idx];

  // Track candidate level and how long it has been stable.
  if (s.hookCand != rawHigh) {
    s.hookCand = rawHigh;
    s.hookCandSince = nowMs;
    s.hookCandConsec = 1;
  } else if (s.hookCandConsec < 255) {
    s.hookCandConsec++;
  }

  // During pulse dialing, require longer stability time to distinguish between
  // short pulses (which can be up to pulseLowMaxMs) and actual hook changes.
  uint32_t requiredStableMs = hookStableMs;
  if (s.pdState != PerLine::PDState::Idle) {
    // Use pulseLowMaxMs + margin to ensure pulses are not mistaken for hook changes
    requiredStableMs = settings_.pulseLowMaxMs + kPulseMarginMs;
  }

  bool timeOk   = (nowMs - s.hookCandSince) >= requiredStableMs;
  bool consecOk = (settings_.hookStableConsec == 0) || (s.hookCandConsec >= settings_.hookStableConsec);
  if (timeOk && consecOk) {
    bool offHook = rawToOffHook_(s.hookCand);

    if (settings_.debugSHKLevel >= 2) {
      Serial.printf("SHKService: L%d stable hook %s (raw=%d) after %u ms\n",idx, offHook ? "OffHook" : "OnHook", rawHigh ? 1 : 0, nowMs - s.hookCandSince);
      Serial.flush();  // Ensure immediate output
      util::UIConsole::log("L" + String(idx) + " stable hook " + (offHook ? "OffHook" : "OnHook") + " (raw=" + String(rawHigh ? 1 : 0) + ") at " + String(nowMs) + " ms", "SHKService");
    }
    setStableHook(idx, offHook, rawHigh, nowMs);
  }
}

// Sets stable hook status for a line and updates related state.
void SHKService::setStableHook(int index, bool offHook, bool rawHigh, uint32_t nowMs) {
  auto& line = lineManager_.getLine(index);


  model::HookStatus newHook = offHook ? model::HookStatus::Off : model::HookStatus::On;
  if (newHook != line.currentHookStatus) {
    line.currentHookStatus = newHook;
    line.SHK = offHook;
    lineManager_.setStatus(index, offHook ? model::LineStatus::Ready : model::LineStatus::Idle);

    // Resync fast only when hook state actually changes.
    resyncFast_(index, rawHigh, nowMs);

    if (!offHook) resetPulseState_(index);  // Transitioned to OnHook.
  }
}

// ---------------- Pulse Detector ----------------
// Checks if pulse mode is allowed for the given line.
bool SHKService::pulseModeAllowed_(const LineHandler& line) const {
  if (line.currentHookStatus != model::HookStatus::Off) return false;
  return (line.currentLineStatus == model::LineStatus::Ready ||
          line.currentLineStatus == model::LineStatus::PulseDialing);
}

// Updates pulse detector state for line 'idx' with new raw reading 'rawHigh' at time 'nowMs'.
void SHKService::updatePulseDetector_(int idx, bool rawHigh, uint32_t nowMs) {

  auto& s   = lineState_[idx];
  auto& line = lineManager_.getLine(idx);

  // Skip pulse detection for a short time after last digit.
  if (nowMs < s.blockUntilMs) {
    return;
  }

  // Only run in correct mode, but do not interrupt an ongoing pulse.
  if (!pulseModeAllowed_(line)) {
    if (s.pdState == PerLine::PDState::BetweenPulses && s.pulseCountWork > 0) {
      emitDigitAndReset_(idx, rawHigh, nowMs); // Commit digit e.g. on OnHook.
      return;
    }
    if (s.pdState != PerLine::PDState::InPulse) {
      resetPulseState_(idx);
      line.gap = line.edge ? (nowMs - line.edge) : 0;
      return;
    }
    // If in pulse, let it finish (no reset here).
  }

  // Glitch filter.
  if (rawHigh != s.lastRaw) { s.lastRaw = rawHigh; s.rawChangeMs = nowMs; }
  bool accept = (nowMs - s.rawChangeMs) >= settings_.pulseGlitchMs;

  // Edge detection (correct order, not debug-dependent).
  if (accept && (rawHigh != s.fastLevel)) {
    s.fastLevel = rawHigh;

    if (!s.fastLevel) {
      // High → Low = start of pulse.
      pulseFalling_(idx, nowMs);
    } else {
      // Low → High = end of pulse.
      pulseRising_(idx, nowMs);
    }
  } // End edge block.

  // Digit gap: runs every tick (not just on edge).
  if (s.pdState == PerLine::PDState::BetweenPulses) {
    uint32_t sinceRise = nowMs - s.lastEdgeMs;
    if (sinceRise >= settings_.digitGapMinMs) {
      emitDigitAndReset_(idx, rawHigh, nowMs);
      return;
    }
  }

  // Timeout: state-aware.
  if (s.pdState == PerLine::PDState::InPulse) {
    uint32_t since = nowMs - s.lowStartMs; // Duration of "low".
    if (since >= settings_.globalPulseTimeoutMs) {
      resetPulseState_(idx); // Interrupted/broken pulse.
      return;
    }
  } else if (s.pdState == PerLine::PDState::BetweenPulses) {
    uint32_t since = nowMs - s.lastEdgeMs; // Time since last rising edge.
    if (since >= settings_.globalPulseTimeoutMs) {
      emitDigitAndReset_(idx, rawHigh, nowMs); // Commit digit anyway.
      return;
    }
  }

  // Diagnostics.
  line.gap = line.edge ? (nowMs - line.edge) : 0;
}

// Handles falling edge (start of pulse) for line 'idx'.
void SHKService::pulseFalling_(int idx, uint32_t nowMs) {
  auto& line = lineManager_.getLine(idx);
  if (line.currentHookStatus != model::HookStatus::Off) {
    return; // No pulses should start on OnHook.
  }

  auto& s = lineState_[idx];
  if (s.pdState == PerLine::PDState::Idle || s.pdState == PerLine::PDState::BetweenPulses) {
    s.pdState   = PerLine::PDState::InPulse;
    s.lowStartMs = nowMs;
    s.lastEdgeMs = nowMs;
    lineManager_.resetLineTimer(idx); // Reset line timer on pulse start.

    if (settings_.debugSHKLevel >= 2){
      Serial.printf("SHKService: Line %d pulse falling \n", idx);
      Serial.flush();  // Ensure immediate output
      util::UIConsole::log("Line " + String(idx) + " pulse falling", "SHKService");
    }
  }
}

// Handles rising edge (end of pulse) for line 'idx'.
void SHKService::pulseRising_(int idx, uint32_t nowMs) {
  auto& s = lineState_[idx];
  auto& line = lineManager_.getLine(idx);

  if (s.pdState == PerLine::PDState::InPulse) {
    uint32_t lowDur = nowMs - s.lowStartMs; // Pulse low duration.
    s.lastEdgeMs = nowMs;                   // Rising edge.
    if (settings_.debugSHKLevel >= 2) {
      Serial.printf("SHKService: Line %d pulse low duration %d ms\n", (int)idx, (int)lowDur);
      Serial.flush();  // Ensure immediate output
      util::UIConsole::log("SHKService: Line " + String(idx) + " pulse low duration " + String(lowDur) + " ms", "SHKService");
    }

    // Validate pulse low time: min = debounceMs, max = pulseLowMaxMs.
    if (lowDur >= settings_.debounceMs && lowDur <= settings_.pulseLowMaxMs) {

      if (s.pulseCountWork == 0 &&
          line.currentHookStatus == model::HookStatus::Off &&
          line.currentLineStatus == model::LineStatus::Ready) {lineManager_.setStatus(idx, model::LineStatus::PulseDialing);
      }
      s.pulseCountWork += 1;
      s.pdState     = PerLine::PDState::BetweenPulses;
      s.lastEdgeMs  = nowMs; // Start digit gap measurement.

      if (settings_.debugSHKLevel >= 2) {
        Serial.printf("SHKService: Line %d pulsCountWork %d \n", (int)idx, (int)s.pulseCountWork);
        Serial.flush();  // Ensure immediate output
        util::UIConsole::log("SHKService: Line " + String(idx) + " pulseCountWork " + String(s.pulseCountWork), "SHKService");
      }

    } else {
      resetPulseState_(idx);
      return;
    }
  }
}

// Emits digit and resets pulse state for line 'idx'.
void SHKService::emitDigitAndReset_(int idx, bool rawHigh, uint32_t nowMs) {
  auto& s = lineState_[idx];
  auto& line = lineManager_.getLine(idx);

  if (s.pulseCountWork > 0) {
    char d = mapPulseToDigit_(s.pulseCountWork); // 10 → '0'
    line.dialedDigits += d; // Add digit directly to LineHandler.
    line.lineTimerEnd = nowMs + settings_.timer_toneDialing; // Reset timer

    if (settings_.debugSHKLevel >= 1) {
      Serial.printf("SHKService: Line %d digit '%c' (pulses=%d)\n", (int)idx, d, (int)s.pulseCountWork);
      Serial.flush();  // Ensure immediate output
      util::UIConsole::log("Line " + String(idx) + " digit '" + String(d) + "' (pulses=" + String(s.pulseCountWork) + ")", "SHKService");
    }
    Serial.printf("SHKService: Line %d dialedDigits now: %s\n", (int)idx, line.dialedDigits.c_str());
    Serial.flush();  // Ensure immediate output
    util::UIConsole::log("Line " + String(idx) + " dialedDigits now: " + line.dialedDigits, "SHKService");
  }
  lineManager_.setLineTimer(idx, settings_.timer_pulsDialing);
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

// Resynchronizes fast-level state for line 'idx'.
void SHKService::resyncFast_(int idx, bool rawHigh, uint32_t nowMs) {
  auto& s = lineState_[idx];
  s.lastRaw     = rawHigh;
  s.fastLevel   = rawHigh;
  s.rawChangeMs = nowMs;
}

// Maps pulse count to digit (10 pulses → '0').
char SHKService::mapPulseToDigit_(uint8_t count) const {
  uint8_t p = count % 10; // p ∈ {0..9}, where 0 means “10 pulses”
  if (settings_.pulseAdjustment == 1) {
    // Swedish type: 0 = 1 pulse, 1 = 2 pulses, ..., 9 = 10 pulses
    // => digit = (p == 0) ? 9 : (p - 1)
    uint8_t d = (p == 0) ? 9 : (p - 1);
    return static_cast<char>('0' + d);
  } else {
    // Standard (decadic): 1..9 → 1..9, 10 → 0
    return (p == 0) ? '0' : static_cast<char>('0' + p);
  }
}
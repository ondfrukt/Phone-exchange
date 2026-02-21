#include "services/ToneReader.h"
#include "services/LineManager.h"

ToneReader::ToneReader(InterruptManager& interruptManager, MCPDriver& mcpDriver, Settings& settings, LineManager& lineManager)
  : interruptManager_(interruptManager), mcpDriver_(mcpDriver), settings_(settings), lineManager_(lineManager) {}


void ToneReader::activate()
{ 
  if (settings_.debugTRLevel >= 1) {
    Serial.println(F("ToneReader:         Activating MT8870 power"));
    util::UIConsole::log("ToneReader:         Activating MT8870 power", "ToneReader");
  }
  isActive = true;
  mcpDriver_.digitalWriteMCP(cfg::mcp::MCP_MAIN_ADDRESS, cfg::mcp::PWDN_MT8870 , false);
  
  // Small delay to let MT8870 stabilize after power on
  delay(10);
  
  // Reset all state variables to ensure clean start
  lastStdLevel_ = false;
  stdRisingEdgePending_ = false;
  stdRisingEdgeTime_ = 0;
  for (int i = 0; i < 8; ++i) {
    lastDtmfTimeByLine_[i] = 0;
    lastDtmfNibbleByLine_[i] = INVALID_DTMF_NIBBLE;
  }
  scanCursor_ = -1;
  currentScanLine_ = -1;
  stdLineIndex_ = -1;
  lastTmuxSwitchAtMs_ = 0;
  scanPauseLogged_ = false;
  lastLoggedScanMask_ = 0xFF;
  
  // Clear any pending STD interrupts that might have accumulated
  int clearedCount = 0;
  while (true) {
    IntResult ir = interruptManager_.pollEvent(cfg::mcp::MCP_MAIN_ADDRESS, cfg::mcp::STD);
    if (!ir.hasEvent) break;
    clearedCount++;
  }
  
  if (settings_.debugTRLevel >= 2) {
    Serial.print(F("ToneReader:         State reset after activation, cleared "));
    Serial.print(clearedCount);
    Serial.println(F(" pending interrupts"));
    util::UIConsole::log("State reset after activation, cleared " + String(clearedCount) + " pending interrupts", "ToneReader");
  }
}


void ToneReader::deactivate()
{
  if (settings_.debugTRLevel >= 1) {
    Serial.println(F("ToneReader:         Deactivating MT8870 power"));
    util::UIConsole::log("ToneReader:         Deactivating MT8870 power", "ToneReader");
  }
  isActive = false;
  mcpDriver_.digitalWriteMCP(cfg::mcp::MCP_MAIN_ADDRESS, cfg::mcp::PWDN_MT8870 , true);
  
  // Reset state variables when deactivating
  lastStdLevel_ = false;
  stdRisingEdgePending_ = false;
  stdRisingEdgeTime_ = 0;
  currentScanLine_ = -1;
  stdLineIndex_ = -1;
  scanPauseLogged_ = false;
  lastLoggedScanMask_ = 0xFF;
}

void ToneReader::update() {
  if (!isActive) {
    return;
  }

  // Note: We now rely on interrupt-driven events from InterruptManager
  // instead of direct GPIO polling for more reliable edge detection
  
  unsigned long now = millis();
  uint8_t scanMaskNow = lineManager_.toneScanMask;
  uint8_t activeScanLines = 0;
  for (int i = 0; i < 8; ++i) {
    if (scanMaskNow & (1u << i)) {
      activeScanLines++;
    }
  }
  unsigned long requiredStdStableMs = settings_.dtmfStdStableMs;
  if (activeScanLines > 1 && requiredStdStableMs > 8) {
    requiredStdStableMs = 8;
  }
  
  // Check if we have a pending rising edge that needs stability verification
  if (stdRisingEdgePending_) {
    unsigned long timeSinceRising = now - stdRisingEdgeTime_;
    
    // Check if STD signal has been stable for the required duration
    if (timeSinceRising >= requiredStdStableMs) {
      if (settings_.debugTRLevel >= 2) {
        Serial.print(F("ToneReader:         STD stable for "));
        Serial.print(timeSinceRising);
        Serial.print(F("ms (required="));
        Serial.print(requiredStdStableMs);
        Serial.println(F("ms) - attempting to read DTMF nibble"));
        util::UIConsole::log("ToneReader:         STD stable for " + String(timeSinceRising) + 
                            "ms (required=" + String(requiredStdStableMs) + "ms) - attempting to read",
                            "ToneReader");
      }
      
      stdRisingEdgePending_ = false;
      
      // Only process DTMF if we know which line generated the current STD cycle.
      int idx = stdLineIndex_;
      if (idx < 0 || idx >= 8) {
        if (settings_.debugTRLevel >= 1) {
          Serial.println(F("ToneReader:         Ignoring DTMF - no scanned line associated with STD"));
          util::UIConsole::log("Ignoring DTMF - no scanned line associated with STD", "ToneReader");
        }
        return;
      }
      
      uint8_t nibble = 0;
      if (readDtmfNibble(nibble)) {
        
        // Filter out spurious nibble=0x0 (all Q pins LOW) - this is never a valid DTMF tone
        // These often appear as ghost interrupts when MT8870 starts up or from noise
        if (nibble == 0x0) {
          if (settings_.debugTRLevel >= 1) {
            Serial.println(F("ToneReader:         Ignoring spurious nibble=0x0 (not a valid DTMF tone)"));
            util::UIConsole::log("Ignoring spurious nibble=0x0", "ToneReader");
          }
          return; // Don't update debounce variables, completely ignore this interrupt
        }
        
        // Check debouncing: ignore if same digit detected within debounce period
        // Use unsigned subtraction which handles millis() rollover correctly
        unsigned long timeSinceLastDtmf = now - lastDtmfTimeByLine_[idx];
        bool isSameDigit = (nibble == lastDtmfNibbleByLine_[idx]);
        bool withinDebounceWindow = (timeSinceLastDtmf < settings_.dtmfDebounceMs);
        bool isDuplicate = isSameDigit && withinDebounceWindow;
        
        if (settings_.debugTRLevel >= 2) {
          Serial.print(F("ToneReader:         Debounce check - timeSince="));
          Serial.print(timeSinceLastDtmf);
          Serial.print(F("ms isSameDigit="));
          Serial.print(isSameDigit ? F("YES") : F("NO"));
          Serial.print(F(" withinWindow="));
          Serial.print(withinDebounceWindow ? F("YES") : F("NO"));
          Serial.print(F(" isDuplicate="));
          Serial.println(isDuplicate ? F("YES") : F("NO"));
        }
        
        if (!isDuplicate) {
          lastDtmfTimeByLine_[idx] = now;
          lastDtmfNibbleByLine_[idx] = nibble;
          
          char ch = decodeDtmf(nibble);
          if (settings_.debugTRLevel >= 2) {
            Serial.print(F("ToneReader:         DECODED - nibble=0x"));
            Serial.print(nibble, HEX);
            Serial.print(F(" => char='"));
            Serial.print(ch);
            Serial.println('\'');
            util::UIConsole::log("ToneReader DECODED: nibble=0x" + String(nibble, HEX) +
                                     " => '" + String(ch) + "'",
                                 "ToneReader");
          }

          if (ch != '\0') {
            lineManager_.lastLineReady = idx;

            // Only set status to ToneDialing if the line is currently in Ready state and we have a valid digit
            if (idx >= 0 && lineManager_.getLine(idx).currentLineStatus == model::LineStatus::Ready) {
              lineManager_.setStatus(idx, model::LineStatus::ToneDialing);
            }
            lineManager_.resetLineTimer(idx);

            // To avoid strange signals thats detected as dtomf tones, only accept tones when line is in Ready or ToneDialing state
            if (idx >= 0 && (lineManager_.getLine(idx).currentLineStatus == model::LineStatus::Ready || 
                     lineManager_.getLine(idx).currentLineStatus == model::LineStatus::ToneDialing)) {
              auto& line = lineManager_.getLine(idx);
              line.dialedDigits += ch;
              Serial.print(MAGENTA);
              Serial.print(F("ToneReader:         Added to line "));
              Serial.print(idx);
              Serial.print(F(" digit='"));
              Serial.print(ch);
              Serial.print(F("' dialedDigits=\""));
              Serial.print(line.dialedDigits);
              Serial.println('"');
              Serial.print(COLOR_RESET);
              util::UIConsole::log("ToneReader:         line " + String(idx) + " +=" + String(ch) + 
                  " dialedDigits=\"" + line.dialedDigits + "\"", "ToneReader");
            } else if (idx >= 0) {
              if (settings_.debugTRLevel >= 1) {
                Serial.println(F("ToneReader:         WARNING - Line not in Ready/ToneDialing state"));
                util::UIConsole::log("WARNING - Line not in valid state for DTMF", "ToneReader");
              }
            } else {
              if (settings_.debugTRLevel >= 1) {
                Serial.println(F("ToneReader:         WARNING - No valid scanned line to store digit"));
                util::UIConsole::log("WARNING - No valid scanned line to store digit", "ToneReader");
              }
            }
          } else {
            if (settings_.debugTRLevel >= 1) {
              Serial.print(F("ToneReader:         WARNING - Decoded character is NULL (invalid nibble=0x"));
              Serial.print(nibble, HEX);
              Serial.println(F(")"));
              util::UIConsole::log("WARNING - Decoded character is NULL (invalid nibble=0x" + 
                        String(nibble, HEX) + ")", "ToneReader");
            }
          }
        } else if (settings_.debugTRLevel >= 1) {
          Serial.print(F("ToneReader:         Duplicate ignored (debouncing) nibble=0x"));
          Serial.print(nibble, HEX);
          Serial.print(F(" time="));
          Serial.print(timeSinceLastDtmf);
          Serial.println(F("ms"));
          util::UIConsole::log("Duplicate ignored (debouncing) nibble=0x" + 
                              String(nibble, HEX) + " time=" + String(timeSinceLastDtmf) + "ms", 
                              "ToneReader");
        }
      } else {
        if (settings_.debugTRLevel >= 1) {
          Serial.println(F("ToneReader:         ERROR - Failed to read DTMF nibble from MCP"));
          util::UIConsole::log("ERROR - Failed to read nibble", "ToneReader");
        }
      }
    }
  }
  
  // Hantera MAIN-interrupts (bl.a. MT8870 STD). Töm alla väntande events.
  while (true) {
    IntResult ir = interruptManager_.pollEvent(cfg::mcp::MCP_MAIN_ADDRESS, cfg::mcp::STD);
    if (!ir.hasEvent) break;

    if (settings_.debugTRLevel >= 2) {
      Serial.print(F("ToneReader:         STD interrupt detected - addr=0x"));
      Serial.print(ir.i2c_addr, HEX);
      Serial.print(F(" pin="));
      Serial.print(ir.pin);
      Serial.print(F(" level="));
      Serial.println(ir.level ? F("HIGH") : F("LOW"));
      util::UIConsole::log("ToneReader:         STD INT 0x" + String(ir.i2c_addr, HEX) +
                               " pin=" + String(ir.pin) +
                               " level=" + String(ir.level ? "HIGH" : "LOW"),
                           "ToneReader");
    }
    // Detect rising edge (LOW -> HIGH transition)
    bool risingEdge = ir.level && !lastStdLevel_;
    bool fallingEdge = !ir.level && lastStdLevel_;
    
    if (settings_.debugTRLevel >= 2) {
      Serial.print(F("ToneReader:         Edge detection - lastStdLevel_="));
      Serial.print(lastStdLevel_ ? F("HIGH") : F("LOW"));
      Serial.print(F(" currentLevel="));
      Serial.print(ir.level ? F("HIGH") : F("LOW"));
      Serial.print(F(" risingEdge="));
      Serial.print(risingEdge ? F("YES") : F("NO"));
      Serial.print(F(" fallingEdge="));
      Serial.println(fallingEdge ? F("YES") : F("NO"));
    }
    
    lastStdLevel_ = ir.level;
    
    // STD blir hög när en giltig ton detekterats. Läs nibbeln på rising edge.
    if (risingEdge) {
      const unsigned long sinceSwitchMs = millis() - lastTmuxSwitchAtMs_;
      if (sinceSwitchMs < TMUX_POST_SWITCH_GUARD_MS) {
        if (settings_.debugTRLevel >= 2) {
          Serial.print(F("ToneReader:         Rising edge ignored (post-switch guard), sinceSwitch="));
          Serial.print(sinceSwitchMs);
          Serial.println(F("ms"));
          util::UIConsole::log("Rising edge ignored (post-switch guard), sinceSwitch=" +
                                   String(sinceSwitchMs) + "ms",
                               "ToneReader");
        }
        continue;
      }

      if (settings_.debugTRLevel >= 1) {
        Serial.println(F("ToneReader:         Rising edge detected - marking for stability check"));
        util::UIConsole::log("ToneReader:         Rising edge detected - marking for stability check", "ToneReader");
      }
      
      // Debug: Read Q pins immediately at rising edge
      if (settings_.debugTRLevel >= 2) {
        uint16_t gpioAB = 0;
        if (mcpDriver_.readGpioAB16(cfg::mcp::MCP_MAIN_ADDRESS, gpioAB)) {
          uint8_t q1 = (gpioAB >> cfg::mcp::Q1) & 0x1;
          uint8_t q2 = (gpioAB >> cfg::mcp::Q2) & 0x1;
          uint8_t q3 = (gpioAB >> cfg::mcp::Q3) & 0x1;
          uint8_t q4 = (gpioAB >> cfg::mcp::Q4) & 0x1;
          Serial.print(F("ToneReader:         Q-pins at rising edge: Q4="));
          Serial.print(q4);
          Serial.print(F(" Q3="));
          Serial.print(q3);
          Serial.print(F(" Q2="));
          Serial.print(q2);
          Serial.print(F(" Q1="));
          Serial.println(q1);
        }
      }
      
      // Mark the rising edge and record the time
      // We'll process it after verifying STD is stable for the configured duration
      stdRisingEdgePending_ = true;
      stdRisingEdgeTime_ = millis();
      stdLineIndex_ = currentScanLine_;
      lineManager_.lastLineReady = stdLineIndex_;
      if (settings_.debugTRLevel >= 2) {
        Serial.print(F("ToneReader:         Scan PAUSED - locked to line "));
        Serial.println(stdLineIndex_);
        util::UIConsole::log("Scan PAUSED - locked to line " + String(stdLineIndex_), "ToneReader");
      }
      
    } else if (fallingEdge) {
      // If we get a falling edge before processing the rising edge, validate duration
      // This helps filter very short glitches
      unsigned long toneDuration = 0;
      if (stdRisingEdgePending_) {
        toneDuration = millis() - stdRisingEdgeTime_;
        if (settings_.debugTRLevel >= 1) {
          Serial.print(F("ToneReader:         Falling edge - STD was HIGH for "));
          Serial.print(toneDuration);
          Serial.println(F("ms"));
        }
        if (toneDuration < settings_.dtmfMinToneDurationMs) {
          if (settings_.debugTRLevel >= 1) {
            Serial.print(F("ToneReader:         Tone too short ("));
            Serial.print(toneDuration);
            Serial.print(F("ms < "));
            Serial.print(settings_.dtmfMinToneDurationMs);
            Serial.println(F("ms) - ignoring"));
            util::UIConsole::log("Tone too short (" + String(toneDuration) + 
                                "ms < " + String(settings_.dtmfMinToneDurationMs) + "ms) - ignoring", 
                                "ToneReader");
          }
          // Cancel the pending tone - don't process it
          stdRisingEdgePending_ = false;
          stdLineIndex_ = -1;
          if (settings_.debugTRLevel >= 2) {
            Serial.println(F("ToneReader:         Scan RESUMED - tone rejected (too short)"));
            util::UIConsole::log("Scan RESUMED - tone rejected (too short)", "ToneReader");
          }
          // Skip line timer and logging below, continue to next event
          continue;
        }
        // Tone was long enough, but already processed if stability check passed
        stdRisingEdgePending_ = false;
      }
      
      // Only set timer if line is in ToneDialing state (meaning a valid digit was processed)
      if (stdLineIndex_ >= 0) {
        auto& line = lineManager_.getLine(stdLineIndex_);
        if (line.currentLineStatus == model::LineStatus::ToneDialing) {
          lineManager_.setLineTimer(stdLineIndex_, settings_.timer_toneDialing);
        } else if (settings_.debugTRLevel >= 2) {
          Serial.println(F("ToneReader:         Falling edge - line not in ToneDialing, no timer set"));
          util::UIConsole::log("Falling edge - line not in ToneDialing, no timer set", "ToneReader");
        }
      } else if (settings_.debugTRLevel >= 1) {
        Serial.println(F("ToneReader:         Falling edge - no scanned line associated with STD"));
        util::UIConsole::log("Falling edge - no scanned line associated with STD", "ToneReader");
      }
      stdLineIndex_ = -1;
      if (settings_.debugTRLevel >= 2) {
        Serial.println(F("ToneReader:         Scan RESUMED - STD returned LOW"));
        util::UIConsole::log("Scan RESUMED - STD returned LOW", "ToneReader");
      }
      
      if (settings_.debugTRLevel >= 1) {
        Serial.println(F("ToneReader:         Falling edge detected (STD went LOW)"));
        util::UIConsole::log("Falling edge detected (STD LOW)", "ToneReader");
      }
    }
  }

  // Fallback lock by level is only safe when scanning a single line.
  // With multiple active scan lines it can mis-attribute a tone to the wrong line.
  const bool allowEarlyLock = (activeScanLines <= 1);
  if (!stdRisingEdgePending_ && stdLineIndex_ < 0 && currentScanLine_ >= 0 &&
      allowEarlyLock &&
      (now - lastTmuxSwitchAtMs_) >= TMUX_POST_SWITCH_GUARD_MS) {
    uint16_t gpioAB = 0;
    if (mcpDriver_.readGpioAB16(cfg::mcp::MCP_MAIN_ADDRESS, gpioAB)) {
      const bool stdHigh = (gpioAB & (1u << cfg::mcp::STD)) != 0;
      if (stdHigh) {
        stdRisingEdgePending_ = true;
        stdRisingEdgeTime_ = now;
        stdLineIndex_ = currentScanLine_;
        lastStdLevel_ = true;
        if (settings_.debugTRLevel >= 2) {
          Serial.print(F("ToneReader:         Scan EARLY-LOCK - STD HIGH on line "));
          Serial.println(stdLineIndex_);
          util::UIConsole::log("Scan EARLY-LOCK - STD HIGH on line " + String(stdLineIndex_),
                               "ToneReader");
        }
      }
    }
  }
  else if (!allowEarlyLock && settings_.debugTRLevel >= 2) {
    static bool loggedEarlyLockDisabled = false;
    if (!loggedEarlyLockDisabled) {
      Serial.println(F("ToneReader:         EARLY-LOCK disabled while scanning multiple lines"));
      util::UIConsole::log("EARLY-LOCK disabled while scanning multiple lines", "ToneReader");
      loggedEarlyLockDisabled = true;
    }
  }

  // Fallback: release lock if STD dropped LOW without a captured falling edge.
  if (lastStdLevel_ && !stdRisingEdgePending_) {
    uint16_t gpioAB = 0;
    if (mcpDriver_.readGpioAB16(cfg::mcp::MCP_MAIN_ADDRESS, gpioAB)) {
      const bool stdHigh = (gpioAB & (1u << cfg::mcp::STD)) != 0;
      if (!stdHigh) {
        if (stdLineIndex_ >= 0) {
          auto& line = lineManager_.getLine(stdLineIndex_);
          if (line.currentLineStatus == model::LineStatus::ToneDialing) {
            lineManager_.setLineTimer(stdLineIndex_, settings_.timer_toneDialing);
          }
        }
        stdLineIndex_ = -1;
        lastStdLevel_ = false;
        if (settings_.debugTRLevel >= 2) {
          Serial.println(F("ToneReader:         Scan EARLY-RELEASE - STD LOW"));
          util::UIConsole::log("Scan EARLY-RELEASE - STD LOW", "ToneReader");
        }
      }
    }
  }

  // Scan only when STD is idle and no tone is pending verification.
  // This prevents changing TMUX channel before queued STD edges are processed.
  if (!stdRisingEdgePending_ && !lastStdLevel_) {
    if (scanPauseLogged_ && settings_.debugTRLevel >= 2) {
      Serial.println(F("ToneReader:         Scan loop active again"));
      util::UIConsole::log("Scan loop active again", "ToneReader");
    }
    scanPauseLogged_ = false;
    toneScan();
  } else if (!scanPauseLogged_ && settings_.debugTRLevel >= 2) {
    Serial.print(F("ToneReader:         Scan HOLD - pending="));
    Serial.print(stdRisingEdgePending_ ? F("1") : F("0"));
    Serial.print(F(" stdLevel="));
    Serial.println(lastStdLevel_ ? F("1") : F("0"));
    util::UIConsole::log("Scan HOLD - pending=" + String(stdRisingEdgePending_ ? 1 : 0) +
                         " stdLevel=" + String(lastStdLevel_ ? 1 : 0), "ToneReader");
    scanPauseLogged_ = true;
  }
}

bool ToneReader::readDtmfNibble(uint8_t& nibble) {
  if (settings_.debugTRLevel >= 2) {
    Serial.println(F("ToneReader:         Reading DTMF nibble from MCP GPIO..."));
    util::UIConsole::log("Reading DTMF nibble from MCP GPIO...", "ToneReader");
  }
  
  // Läs aktuell GPIO-status (STD är hög nu => MT8870 håller data stabil)
  uint16_t gpioAB = 0;
  if (!mcpDriver_.readGpioAB16(cfg::mcp::MCP_MAIN_ADDRESS, gpioAB)) {
    if (settings_.debugTRLevel >= 1) {
      Serial.println(F("ToneReader:         ERROR - Failed to read GPIO from MCP_MAIN"));
      util::UIConsole::log("ERROR - Failed to read GPIO from MCP_MAIN", "ToneReader");
    }
    return false;
  }

  if (settings_.debugTRLevel >= 2) {
    Serial.print(F("ToneReader:         Raw GPIOAB=0b"));
    for (int i = 15; i >= 0; i--) {
      Serial.print((gpioAB & (1 << i)) ? '1' : '0');
    }
    Serial.print(F(" (0x"));
    Serial.print(gpioAB, HEX);
    Serial.println(F(")"));
    util::UIConsole::log("Raw GPIOAB=0b" + String(gpioAB, BIN) +
                             " (0x" + String(gpioAB, HEX) + ")",
                         "ToneReader");
  }

  // Q1..Q4 i cfg::mcp är pinindex 0..15 i 16-bitarsordet (A=0..7, B=8..15)
  const bool q1 = (gpioAB & (1u << cfg::mcp::Q1)) != 0; // LSB
  const bool q2 = (gpioAB & (1u << cfg::mcp::Q2)) != 0;
  const bool q3 = (gpioAB & (1u << cfg::mcp::Q3)) != 0;
  const bool q4 = (gpioAB & (1u << cfg::mcp::Q4)) != 0; // MSB

  nibble = (static_cast<uint8_t>(q4) << 3) |
           (static_cast<uint8_t>(q3) << 2) |
           (static_cast<uint8_t>(q2) << 1) |
           (static_cast<uint8_t>(q1) << 0);

  if (settings_.debugTRLevel >= 1) {
    Serial.print(F("ToneReader:         MT8870 pins - Q4="));
    Serial.print(q4);
    Serial.print(F(" Q3="));
    Serial.print(q3);
    Serial.print(F(" Q2="));
    Serial.print(q2);
    Serial.print(F(" Q1="));
    Serial.print(q1);
    Serial.print(F(" => nibble=0b"));
    for (int i = 3; i >= 0; i--) {
      Serial.print((nibble & (1 << i)) ? '1' : '0');
    }
    Serial.print(F(" (0x"));
    Serial.print(nibble, HEX);
    Serial.println(F(")"));
    util::UIConsole::log("ToneReader:         Q4=" + String(q4) + " Q3=" + String(q3) +
                             " Q2=" + String(q2) + " Q1=" + String(q1) + 
                             " nibble=0x" + String(nibble, HEX),
                         "ToneReader");
  }
  return true;
}

char ToneReader::decodeDtmf(uint8_t nibble) {
  if (nibble < 16) return dtmf_map_[nibble];
  return '\0';
}

void ToneReader::toneScan(){
  const uint8_t scanMask = lineManager_.toneScanMask;

  if (settings_.debugTRLevel >= 2 && scanMask != lastLoggedScanMask_) {
    Serial.print(F("ToneReader:         Scan mask changed -> 0b"));
    Serial.println(scanMask, BIN);
    util::UIConsole::log("Scan mask changed -> 0b" + String(scanMask, BIN), "ToneReader");
    lastLoggedScanMask_ = scanMask;
  }

  if (scanMask == 0) {
    currentScanLine_ = -1;
    return;
  }

  const unsigned long now = millis();
  const unsigned long recommendedDwell =
      settings_.dtmfMinToneDurationMs + settings_.dtmfStdStableMs + 8;
  const unsigned long configuredMinDwell =
      (settings_.tmuxScanDwellMinMs < 1) ? 1 : settings_.tmuxScanDwellMinMs;
  const unsigned long dwellMs =
      (recommendedDwell > configuredMinDwell) ? recommendedDwell : configuredMinDwell;

  if ((now - lastTmuxSwitchAtMs_) < dwellMs && currentScanLine_ >= 0) {
    return;
  }

  int nextLine = -1;
  for (int offset = 1; offset <= 8; ++offset) {
    const int candidate = (scanCursor_ + offset) & 0x07;
    if ((scanMask & (1u << candidate)) != 0) {
      nextLine = candidate;
      break;
    }
  }

  if (nextLine < 0) {
    currentScanLine_ = -1;
    return;
  }

  const auto& line = lineManager_.getLine(nextLine);
  const uint8_t tmuxSel =
      static_cast<uint8_t>((line.tmuxAddress[0] ? 0x4u : 0x0u) |
                           (line.tmuxAddress[1] ? 0x2u : 0x0u) |
                           (line.tmuxAddress[2] ? 0x1u : 0x0u));
  if (!mcpDriver_.writeMainTmuxAddress(tmuxSel)) {
    return;
  }

  scanCursor_ = nextLine;
  currentScanLine_ = nextLine;
  lastTmuxSwitchAtMs_ = now;

  (void)dwellMs; // Kept for tuning logic; scan-line switch logging intentionally reduced.
}

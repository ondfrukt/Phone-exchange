#include "RingGenerator.h"

RingGenerator::RingGenerator(MCPDriver& mcpDriver, Settings& settings, LineManager& lineManager)
    : mcpDriver_(mcpDriver), settings_(settings), lineManager_(lineManager) {}

RingGenerator::RingState RingGenerator::getRingState(uint8_t lineNumber) const {
  // Map the ring state stored in LineHandler
  // We use ringStateStartTime as a marker: if it's 0, the state is Idle
  auto& line = lineManager_.getLine(lineNumber);
  
  // Check the linesRinging bitmask to determine state
  bool isRinging = (lineManager_.linesRinging & (1 << lineNumber)) != 0;
  if (!isRinging) {
    return RingState::RingIdle;
  }
  
  // If we're in the middle of a ring cycle, we're either toggling or paused
  // We determine this by checking if we're still within the ring length period
  unsigned long elapsed = millis() - line.ringStateStartTime;
  if (elapsed < settings_.ringLengthMs) {
    return RingState::RingToggling;
  } else {
    return RingState::RingPause;
  }
}

void RingGenerator::setRingState(uint8_t lineNumber, RingState state) {
  auto& line = lineManager_.getLine(lineNumber);
  
  if (state == RingState::RingIdle) {
    // Clear the ringing bit
    lineManager_.linesRinging &= ~(1 << lineNumber);
    line.ringStateStartTime = 0;
  } else {
    // Set the ringing bit
    lineManager_.linesRinging |= (1 << lineNumber);
    // Note: ringStateStartTime should be set by the caller when entering a new state
  }
}

void RingGenerator::generateRingSignal(uint8_t lineNumber) {
  if (lineNumber >= cfg::mcp::SHK_LINE_COUNT) {
    if (settings_.debugRGLevel >= 1) {
      Serial.println("[RingGenerator] Invalid line number: " + String(lineNumber));
    }
    return;
  }

  if (lineManager_.getLine(lineNumber).currentLineStatus != model::LineStatus::Idle) {
    if (settings_.debugRGLevel >= 1) {
      Serial.println("[RingGenerator] Line " + String(lineNumber) + " is not Idle");
    }
    return;
  }

  // Start ringing for the specified line
  auto& line = lineManager_.getLine(lineNumber);
  line.ringCurrentIteration = 0;
  line.ringStateStartTime = millis();
  line.ringLastFRToggleTime = millis();
  line.ringFRPinState = false;
  
  // Set the ringing bit in LineManager
  setRingState(lineNumber, RingState::RingToggling);

  // Determine which MCP address to use for this line
  uint8_t mcpAddr = (lineNumber < 4) ? cfg::mcp::MCP_SLIC1_ADDRESS : cfg::mcp::MCP_SLIC2_ADDRESS;
  
  // Set RM pin HIGH to activate ring mode
  uint8_t rmPin = cfg::mcp::RM_PINS[lineNumber];
  mcpDriver_.digitalWriteMCP(mcpAddr, rmPin, HIGH);

  if (settings_.debugRGLevel >= 1) {
    Serial.println("[RingGenerator] Started ringing for line " + String(lineNumber) + 
                   " (RM pin " + String(rmPin) + " on MCP 0x" + String(mcpAddr, HEX) + ")");
  }
}

void RingGenerator::stopRinging() {
  // Stop all ringing lines
  for (uint8_t lineNumber = 0; lineNumber < cfg::mcp::SHK_LINE_COUNT; lineNumber++) {
    stopRingingLine(lineNumber);
  }
}

void RingGenerator::stopRingingLine(uint8_t lineNumber) {
  if (lineNumber >= cfg::mcp::SHK_LINE_COUNT) {
    return;
  }

  RingState state = getRingState(lineNumber);
  if (state == RingState::RingIdle) {
    return;
  }

  // Determine which MCP address to use for this line
  uint8_t mcpAddr = (lineNumber < 4) ? cfg::mcp::MCP_SLIC1_ADDRESS : cfg::mcp::MCP_SLIC2_ADDRESS;
  
  // Set both FR and RM pins LOW
  uint8_t frPin = cfg::mcp::FR_PINS[lineNumber];
  uint8_t rmPin = cfg::mcp::RM_PINS[lineNumber];
  
  mcpDriver_.digitalWriteMCP(mcpAddr, frPin, LOW);
  mcpDriver_.digitalWriteMCP(mcpAddr, rmPin, LOW);

  setRingState(lineNumber, RingState::RingIdle);

  if (settings_.debugRGLevel >= 1) {
    Serial.println("[RingGenerator] Stopped ringing for line " + String(lineNumber));
  }
}

void RingGenerator::update() {
  unsigned long currentTime = millis();

  // Process each line independently using the linesRinging bitmask
  for (uint8_t lineNumber = 0; lineNumber < cfg::mcp::SHK_LINE_COUNT; lineNumber++) {
    // Skip lines that are not ringing
    if ((lineManager_.linesRinging & (1 << lineNumber)) == 0) {
      continue;
    }

    auto& line = lineManager_.getLine(lineNumber);
    RingState state = getRingState(lineNumber);

    if (state == RingState::RingIdle) {
      continue;
    }

    // Check if the phone has been picked up (hook status changed to Off)
    // Only stop ringing if the physical hook is off, not just status changes
    if (line.currentHookStatus == model::HookStatus::Off) {
      if (settings_.debugRGLevel >= 1) {
        Serial.println("[RingGenerator] Line " + String(lineNumber) + 
                      " phone picked up (hook off), stopping ring");
      }
      stopRingingLine(lineNumber);
      continue;
    }

    uint8_t mcpAddr = (lineNumber < 4) ? cfg::mcp::MCP_SLIC1_ADDRESS : cfg::mcp::MCP_SLIC2_ADDRESS;
    uint8_t frPin = cfg::mcp::FR_PINS[lineNumber];

    switch (state) {
      case RingState::RingToggling: {
        // Toggle FR pin at 20 Hz (50ms period: 25ms HIGH, 25ms LOW)
        if (currentTime - line.ringLastFRToggleTime >= 25) {
          line.ringFRPinState = !line.ringFRPinState;
          mcpDriver_.digitalWriteMCP(mcpAddr, frPin, line.ringFRPinState);
          line.ringLastFRToggleTime = currentTime;
          if (settings_.debugRGLevel >= 2) {
            Serial.println("toggling FR pin to " + String(line.ringFRPinState) + " on Line " + String(lineNumber));
          }
        }

        // Check if ring signal duration has elapsed
        if (currentTime - line.ringStateStartTime >= settings_.ringLengthMs) {
          // Stop FR pin toggling, set it LOW
          mcpDriver_.digitalWriteMCP(mcpAddr, frPin, LOW);
          line.ringFRPinState = false;

          line.ringCurrentIteration++;
          
          if (line.ringCurrentIteration >= settings_.ringIterations) {
            // All iterations complete, stop ringing this line
            stopRingingLine(lineNumber);
            if (settings_.debugRGLevel >= 2) {
              Serial.println("[RingGenerator] Line " + String(lineNumber) + 
                           " completed all " + String(settings_.ringIterations) + " ring iterations");
            }
          } else {
            // Move to pause state
            line.ringStateStartTime = currentTime;
            if (settings_.debugRGLevel >= 2) {
              Serial.println("[RingGenerator] Line " + String(lineNumber) + 
                           " ring iteration " + String(line.ringCurrentIteration) + 
                           " complete, pausing for " + String(settings_.ringPauseMs) + "ms");
            }
          }
        }
        break;
      }

      case RingState::RingPause: {
        // Check if pause duration has elapsed
        if (currentTime - line.ringStateStartTime >= settings_.ringPauseMs) {
          // Start next ring signal
          line.ringStateStartTime = currentTime;
          line.ringLastFRToggleTime = currentTime;
          line.ringFRPinState = false;
          
          if (settings_.debugRGLevel >= 2) {
            Serial.println("[RingGenerator] Line " + String(lineNumber) + 
                         " starting ring iteration " + String(line.ringCurrentIteration + 1));
          }
        }
        break;
      }

      case RingState::RingIdle:
        // Nothing to do
        break;
    }
  }
}
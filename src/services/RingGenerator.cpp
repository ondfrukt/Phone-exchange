#include "RingGenerator.h"

RingGenerator::RingGenerator(MCPDriver& mcpDriver, Settings& settings, LineManager& lineManager)
    : mcpDriver_(mcpDriver), settings_(settings), lineManager_(lineManager) {}

void RingGenerator::generateRingSignal(uint8_t lineNumber) {
  if (lineNumber >= 8) {
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
  auto& lineState = lineStates_[lineNumber];
  lineState.currentIteration = 0;
  lineState.state = RingState::RingToggling;
  lineState.stateStartTime = millis();
  lineState.lastFRToggleTime = millis();
  lineState.frPinState = false;

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
  for (uint8_t lineNumber = 0; lineNumber < 8; lineNumber++) {
    stopRingingLine(lineNumber);
  }
}

void RingGenerator::stopRingingLine(uint8_t lineNumber) {
  if (lineNumber >= 8) {
    return;
  }

  auto& lineState = lineStates_[lineNumber];
  if (lineState.state == RingState::RingIdle) {
    return;
  }

  // Determine which MCP address to use for this line
  uint8_t mcpAddr = (lineNumber < 4) ? cfg::mcp::MCP_SLIC1_ADDRESS : cfg::mcp::MCP_SLIC2_ADDRESS;
  
  // Set both FR and RM pins LOW
  uint8_t frPin = cfg::mcp::FR_PINS[lineNumber];
  uint8_t rmPin = cfg::mcp::RM_PINS[lineNumber];
  
  mcpDriver_.digitalWriteMCP(mcpAddr, frPin, LOW);
  mcpDriver_.digitalWriteMCP(mcpAddr, rmPin, LOW);

  lineState.state = RingState::RingIdle;

  if (settings_.debugRGLevel >= 1) {
    Serial.println("[RingGenerator] Stopped ringing for line " + String(lineNumber));
  }
}

void RingGenerator::update() {
  unsigned long currentTime = millis();

  // Process each line independently
  for (uint8_t lineNumber = 0; lineNumber < 8; lineNumber++) {
    auto& lineState = lineStates_[lineNumber];

    if (lineState.state == RingState::RingIdle) {
      continue;
    }

    // Check if the line status has changed from Idle (e.g., phone picked up)
    // If so, stop ringing this line immediately
    if (lineManager_.getLine(lineNumber).currentLineStatus != model::LineStatus::Idle) {
      if (settings_.debugRGLevel >= 1) {
        Serial.println("[RingGenerator] Line " + String(lineNumber) + 
                      " status changed from Idle, stopping ring");
      }
      stopRingingLine(lineNumber);
      continue;
    }

    uint8_t mcpAddr = (lineNumber < 4) ? cfg::mcp::MCP_SLIC1_ADDRESS : cfg::mcp::MCP_SLIC2_ADDRESS;
    uint8_t frPin = cfg::mcp::FR_PINS[lineNumber];

    switch (lineState.state) {
      case RingState::RingToggling: {
        // Toggle FR pin at 20 Hz (50ms period: 25ms HIGH, 25ms LOW)
        if (currentTime - lineState.lastFRToggleTime >= 25) {
          lineState.frPinState = !lineState.frPinState;
          mcpDriver_.digitalWriteMCP(mcpAddr, frPin, lineState.frPinState);
          lineState.lastFRToggleTime = currentTime;
          if (settings_.debugRGLevel >= 2) {
            Serial.println("toggling FR pin to " + String(lineState.frPinState) + " on Line " + String(lineNumber));
          }
        }

        // Check if ring signal duration has elapsed
        if (currentTime - lineState.stateStartTime >= settings_.ringLengthMs) {
          // Stop FR pin toggling, set it LOW
          mcpDriver_.digitalWriteMCP(mcpAddr, frPin, LOW);
          lineState.frPinState = false;

          lineState.currentIteration++;
          
          if (lineState.currentIteration >= settings_.ringIterations) {
            // All iterations complete, stop ringing this line
            stopRingingLine(lineNumber);
            if (settings_.debugRGLevel >= 2) {
              Serial.println("[RingGenerator] Line " + String(lineNumber) + 
                           " completed all " + String(settings_.ringIterations) + " ring iterations");
            }
          } else {
            // Move to pause state
            lineState.state = RingState::RingPause;
            lineState.stateStartTime = currentTime;
            if (settings_.debugRGLevel >= 2) {
              Serial.println("[RingGenerator] Line " + String(lineNumber) + 
                           " ring iteration " + String(lineState.currentIteration) + 
                           " complete, pausing for " + String(settings_.ringPauseMs) + "ms");
            }
          }
        }
        break;
      }

      case RingState::RingPause: {
        // Check if pause duration has elapsed
        if (currentTime - lineState.stateStartTime >= settings_.ringPauseMs) {
          // Start next ring signal
          lineState.state = RingState::RingToggling;
          lineState.stateStartTime = currentTime;
          lineState.lastFRToggleTime = currentTime;
          lineState.frPinState = false;
          
          if (settings_.debugRGLevel >= 2) {
            Serial.println("[RingGenerator] Line " + String(lineNumber) + 
                         " starting ring iteration " + String(lineState.currentIteration + 1));
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
#include "RingGenerator.h"
#include "services/LineManager.h"

RingGenerator::RingGenerator(MCPDriver& mcpDriver, Settings& settings, LineManager& lineManager)
    : mcpDriver_(mcpDriver), settings_(settings), lineManager_(lineManager) {}

void RingGenerator::generateRingSignal(uint8_t lineNumber) {
  
  // Validate line number
  if (lineNumber >= cfg::mcp::SHK_LINE_COUNT) {
    if (settings_.debugRGLevel >= 1) {
      Serial.println("[RingGenerator] Invalid line number: " + String(lineNumber));
    }
    return;
  }

  // Check if line is currently Idle. Only ring if Idle
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

  if (settings_.debugRGLevel >= 2) {
    Serial.println("RingGenerator: Line " + String(lineNumber) + " set to RingToggling");
  }

  lineState.stateStartTime = millis();
  lineState.lastFRToggleTime = millis();
  lineState.frPinState = false;
  lineState.rmPinState = false;

  // Determine which MCP address to use for this line
  uint8_t mcpAddr = (lineNumber < 4) ? cfg::mcp::MCP_SLIC1_ADDRESS : cfg::mcp::MCP_SLIC2_ADDRESS;
  uint8_t rmPin = cfg::mcp::RM_PINS[lineNumber];
  
  if (settings_.debugRGLevel >= 1) {
    Serial.println("RingGenerator: Started ringing for line " + String(lineNumber));
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
    Serial.println("RingGenerator: Stopped ringing for line " + String(lineNumber));
  }
}

void RingGenerator::update() {
  unsigned long currentTime = millis();

  // Process each line independently
  for (uint8_t lineNumber = 0; lineNumber < cfg::mcp::SHK_LINE_COUNT; lineNumber++) {
    auto& lineState = lineStates_[lineNumber];

    if (lineState.state == RingState::RingIdle) {
      continue;
    }

    // Check if the line status has changed from Idle (e.g., phone picked up)
    // If so, stop ringing this line immediately
    if (lineManager_.getLine(lineNumber).currentLineStatus != model::LineStatus::Idle) {
      if (settings_.debugRGLevel >= 1) {
        Serial.println("RingGenerator: Line " + String(lineNumber) + 
                      " status changed from Idle, stopping ring");
      }
      stopRingingLine(lineNumber);
      continue;
    }

    uint8_t mcpAddr = (lineNumber < 4) ? cfg::mcp::MCP_SLIC1_ADDRESS : cfg::mcp::MCP_SLIC2_ADDRESS;
    uint8_t frPin = cfg::mcp::FR_PINS[lineNumber];

    switch (lineState.state) {
      case RingState::RingToggling: {

        // Set RM pin HIGH to activate ring mode
        if (!lineState.rmPinState) {
          mcpDriver_.digitalWriteMCP(mcpAddr, cfg::mcp::RM_PINS[lineNumber], HIGH);
          lineState.rmPinState = true;
        }

        // Toggle FR pin at 20 Hz (50ms period: 25ms HIGH, 25ms LOW)
        if (currentTime - lineState.lastFRToggleTime >= 25) {
          lineState.frPinState = !lineState.frPinState;
          mcpDriver_.digitalWriteMCP(mcpAddr, frPin, lineState.frPinState);
          lineState.lastFRToggleTime = currentTime;
          if (settings_.debugRGLevel >= 2) {
            Serial.println("RingGenerator: Toggling FR pin to " + String(lineState.frPinState) + " on Line " + String(lineNumber));
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
              Serial.println("RingGenerator: Line " + String(lineNumber) + 
                           " completed all " + String(settings_.ringIterations) + " ring iterations");
            }
          } else {
            // Move to pause state
            lineState.state = RingState::RingPause;
            lineState.stateStartTime = currentTime;
            if (settings_.debugRGLevel >= 2) {
              Serial.println("RingGenerator: Line " + String(lineNumber) + 
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
          lineState.rmPinState = false;

          if (settings_.debugRGLevel >= 2) {
            Serial.println("RingGenerator: Line " + String(lineNumber) + 
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
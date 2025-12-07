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

  // Start ringing for the specified line
  activeLineNumber_ = lineNumber;
  currentIteration_ = 0;
  state_ = RingState::RingSignal;
  stateStartTime_ = millis();
  lastFRToggleTime_ = millis();
  frPinState_ = false;

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
  if (state_ == RingState::Idle) {
    return;
  }

  // Determine which MCP address to use for the active line
  uint8_t mcpAddr = (activeLineNumber_ < 4) ? cfg::mcp::MCP_SLIC1_ADDRESS : cfg::mcp::MCP_SLIC2_ADDRESS;
  
  // Set both FR and RM pins LOW
  uint8_t frPin = cfg::mcp::FR_PINS[activeLineNumber_];
  uint8_t rmPin = cfg::mcp::RM_PINS[activeLineNumber_];
  
  mcpDriver_.digitalWriteMCP(mcpAddr, frPin, LOW);
  mcpDriver_.digitalWriteMCP(mcpAddr, rmPin, LOW);

  state_ = RingState::Idle;

  if (settings_.debugRGLevel >= 1) {
    Serial.println("[RingGenerator] Stopped ringing for line " + String(activeLineNumber_));
  }
}

void RingGenerator::update() {
  if (state_ == RingState::Idle) {
    return;
  }

  unsigned long currentTime = millis();
  uint8_t mcpAddr = (activeLineNumber_ < 4) ? cfg::mcp::MCP_SLIC1_ADDRESS : cfg::mcp::MCP_SLIC2_ADDRESS;
  uint8_t frPin = cfg::mcp::FR_PINS[activeLineNumber_];

  switch (state_) {
    case RingState::RingSignal: {
      // Toggle FR pin at 20 Hz (50ms period: 25ms HIGH, 25ms LOW)
      if (currentTime - lastFRToggleTime_ >= 25) {
        frPinState_ = !frPinState_;
        mcpDriver_.digitalWriteMCP(mcpAddr, frPin, frPinState_);
        lastFRToggleTime_ = currentTime;
      }

      // Check if ring signal duration has elapsed
      if (currentTime - stateStartTime_ >= settings_.ringLengthMs) {
        // Stop FR pin toggling, set it LOW
        mcpDriver_.digitalWriteMCP(mcpAddr, frPin, LOW);
        frPinState_ = false;

        currentIteration_++;
        
        if (currentIteration_ >= settings_.ringIterations) {
          // All iterations complete, stop ringing
          stopRinging();
          if (settings_.debugRGLevel >= 2) {
            Serial.println("[RingGenerator] Completed all " + String(settings_.ringIterations) + " ring iterations");
          }
        } else {
          // Move to pause state
          state_ = RingState::RingPause;
          stateStartTime_ = currentTime;
          if (settings_.debugRGLevel >= 2) {
            Serial.println("[RingGenerator] Ring iteration " + String(currentIteration_) + 
                         " complete, pausing for " + String(settings_.ringPauseMs) + "ms");
          }
        }
      }
      break;
    }

    case RingState::RingPause: {
      // Check if pause duration has elapsed
      if (currentTime - stateStartTime_ >= settings_.ringPauseMs) {
        // Start next ring signal
        state_ = RingState::RingSignal;
        stateStartTime_ = currentTime;
        lastFRToggleTime_ = currentTime;
        frPinState_ = false;
        
        if (settings_.debugRGLevel >= 2) {
          Serial.println("[RingGenerator] Starting ring iteration " + String(currentIteration_ + 1));
        }
      }
      break;
    }

    case RingState::Idle:
      // Nothing to do
      break;
  }
}
#include "RingGenerator.h"

RingGenerator::RingGenerator(MCPDriver& mcpDriver, Settings& settings, LineManager& lineManager)
    : mcpDriver_(mcpDriver), settings_(settings), lineManager_(lineManager) {}

// Start generating ring signal for a specific line
void RingGenerator::generateRingSignal(uint8_t lineIndex) {
  if (lineIndex >= 8) {
    if (settings_.debugRGLevel >= 1) {
      Serial.print(F("RingGenerator: Invalid line index: "));
      Serial.println(lineIndex);
    }
    return;
  }

  RingState& state = ringStates_[lineIndex];
  
  // If already ringing, ignore
  if (state.isRinging) {
    if (settings_.debugRGLevel >= 2) {
      Serial.print(F("RingGenerator: Line "));
      Serial.print(lineIndex);
      Serial.println(F(" already ringing"));
    }
    return;
  }

  if (settings_.debugRGLevel >= 1) {
    Serial.print(F("RingGenerator: Starting ring signal for line "));
    Serial.println(lineIndex);
  }

  // Initialize state
  state.isRinging = true;
  state.currentIteration = 0;
  state.inRingPhase = true;
  state.frPinState = false;
  
  // Start the first ring phase
  startRingingPhase_(lineIndex);
}

// Stop ringing for a specific line
void RingGenerator::stopRinging(uint8_t lineIndex) {
  if (lineIndex >= 8) return;

  RingState& state = ringStates_[lineIndex];
  
  if (!state.isRinging) return;

  if (settings_.debugRGLevel >= 1) {
    Serial.print(F("RingGenerator: Stopping ring signal for line "));
    Serial.println(lineIndex);
  }

  // Deactivate ring mode
  setRMPin_(lineIndex, false);
  setFRPin_(lineIndex, false);
  
  // Reset state
  state.isRinging = false;
  state.currentIteration = 0;
  state.frPinState = false;
}

// Non-blocking update function - call this regularly from main loop
void RingGenerator::update() {
  unsigned long now = millis();
  
  // Check each line
  for (uint8_t i = 0; i < 8; i++) {
    RingState& state = ringStates_[i];
    
    if (!state.isRinging) continue;
    
    if (state.inRingPhase) {
      // During ring phase: toggle FR pin at 20 Hz (50ms period = toggle every 25ms)
      // 20 Hz = 1000ms / 20 = 50ms period
      const unsigned long FR_TOGGLE_INTERVAL = 25; // Toggle every 25ms for 20 Hz
      
      if (now - state.lastFRToggleTime >= FR_TOGGLE_INTERVAL) {
        state.frPinState = !state.frPinState;
        setFRPin_(i, state.frPinState);
        state.lastFRToggleTime = now;
      }
      
      // Check if ring phase is complete
      if (now - state.phaseStartTime >= settings_.ringLengthMs) {
        // Move to pause phase
        startPausePhase_(i);
      }
    } else {
      // During pause phase: FR pin should be LOW, just wait for pause duration
      if (now - state.phaseStartTime >= settings_.ringPauseMs) {
        // Increment iteration
        state.currentIteration++;
        
        if (state.currentIteration >= settings_.ringIterations) {
          // All iterations complete, stop ringing
          stopRinging(i);
        } else {
          // Start next ring phase
          startRingingPhase_(i);
        }
      }
    }
  }
}

// Start the ringing phase
void RingGenerator::startRingingPhase_(uint8_t lineIndex) {
  RingState& state = ringStates_[lineIndex];
  unsigned long now = millis();
  
  if (settings_.debugRGLevel >= 2) {
    Serial.print(F("RingGenerator: Line "));
    Serial.print(lineIndex);
    Serial.print(F(" - Starting ring phase, iteration "));
    Serial.println(state.currentIteration);
  }
  
  state.inRingPhase = true;
  state.phaseStartTime = now;
  state.lastFRToggleTime = now;
  state.frPinState = false;
  
  // Activate ring mode
  setRMPin_(lineIndex, true);
  setFRPin_(lineIndex, false);
}

// Start the pause phase
void RingGenerator::startPausePhase_(uint8_t lineIndex) {
  RingState& state = ringStates_[lineIndex];
  unsigned long now = millis();
  
  if (settings_.debugRGLevel >= 2) {
    Serial.print(F("RingGenerator: Line "));
    Serial.print(lineIndex);
    Serial.println(F(" - Starting pause phase"));
  }
  
  state.inRingPhase = false;
  state.phaseStartTime = now;
  
  // During pause, keep RM high but FR low
  setRMPin_(lineIndex, true);
  setFRPin_(lineIndex, false);
  state.frPinState = false;
}

// Set RM pin state for a line
void RingGenerator::setRMPin_(uint8_t lineIndex, bool state) {
  if (lineIndex >= 8) return; // Safety check
  
  uint8_t slicAddr = getSlicAddress_(lineIndex);
  uint8_t rmPin = cfg::mcp::RM_PINS[lineIndex];
  
  if (settings_.debugRGLevel >= 2) {
    Serial.print(F("RingGenerator: Line "));
    Serial.print(lineIndex);
    Serial.print(F(" RM pin "));
    Serial.print(rmPin);
    Serial.print(F(" -> "));
    Serial.println(state ? "HIGH" : "LOW");
  }
  
  mcpDriver_.digitalWriteMCP(slicAddr, rmPin, state);
}

// Set FR pin state for a line
void RingGenerator::setFRPin_(uint8_t lineIndex, bool state) {
  if (lineIndex >= 8) return; // Safety check
  
  uint8_t slicAddr = getSlicAddress_(lineIndex);
  uint8_t frPin = cfg::mcp::FR_PINS[lineIndex];
  
  mcpDriver_.digitalWriteMCP(slicAddr, frPin, state);
}

// Get SLIC I2C address for a line
uint8_t RingGenerator::getSlicAddress_(uint8_t lineIndex) {
  if (lineIndex >= 8) return cfg::mcp::MCP_SLIC1_ADDRESS; // Safety fallback
  return cfg::mcp::SHK_LINE_ADDR[lineIndex];
}
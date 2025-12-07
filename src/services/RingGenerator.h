#pragma once
#include <Arduino.h>
#include "config.h"
#include "drivers/MCPDriver.h"
#include "services/LineManager.h"
#include "settings.h"


class RingGenerator {
  public:
    RingGenerator(MCPDriver& mcpDriver, Settings& settings, LineManager& lineManager);
    void update();
    void generateRingSignal(uint8_t lineIndex);
    void stopRinging(uint8_t lineIndex);

  private:
    MCPDriver& mcpDriver_;
    Settings& settings_;
    LineManager& lineManager_;

    // Ringing state per line
    struct RingState {
      bool isRinging = false;
      unsigned long phaseStartTime = 0;    // When current phase (ring/pause) started
      unsigned long lastFRToggleTime = 0;  // Last time FR pin was toggled
      bool frPinState = false;             // Current state of FR pin
      uint32_t currentIteration = 0;       // Current ring iteration
      bool inRingPhase = true;             // true = ringing, false = pausing
    };
    
    RingState ringStates_[8];              // One state per line (0-7)
    
    // Helper methods
    void startRingingPhase_(uint8_t lineIndex);
    void startPausePhase_(uint8_t lineIndex);
    void setRMPin_(uint8_t lineIndex, bool state);
    void setFRPin_(uint8_t lineIndex, bool state);
    uint8_t getSlicAddress_(uint8_t lineIndex);
};
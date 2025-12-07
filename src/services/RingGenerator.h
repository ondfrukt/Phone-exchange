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
    void generateRingSignal(uint8_t lineNumber);
    void stopRinging();

  private:
    MCPDriver& mcpDriver_;
    Settings& settings_;
    LineManager& lineManager_;

    // Ringing state machine
    enum class RingState {
      Idle,
      RingSignal,  // Generating ring signal (FR toggling)
      RingPause    // Pause between rings
    };

    RingState state_ = RingState::Idle;
    uint8_t activeLineNumber_ = 0;
    uint32_t currentIteration_ = 0;
    unsigned long stateStartTime_ = 0;
    unsigned long lastFRToggleTime_ = 0;
    bool frPinState_ = false; // Current state of FR pin
};
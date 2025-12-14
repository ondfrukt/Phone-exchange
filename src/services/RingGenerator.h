#pragma once
#include <Arduino.h>
#include "config.h"
#include "drivers/MCPDriver.h"
#include "settings.h"
#include "model/Types.h"

class LineManager;

class RingGenerator {
  public:
    RingGenerator(MCPDriver& mcpDriver, Settings& settings, LineManager& lineManager);
    void update();
    void generateRingSignal(uint8_t lineNumber);
    void stopRinging();
    void stopRingingLine(uint8_t lineNumber);

    MCPDriver& mcpDriver_;
    Settings& settings_;
    LineManager& lineManager_;

    // Per-line ringing state
    struct LineRingState {
      RingState state = RingState::RingIdle;
      uint32_t currentIteration = 0;
      unsigned long stateStartTime = 0;
      unsigned long lastFRToggleTime = 0;
      bool frPinState = false; // Current state of FR pin
      bool rmPinState = false; // Current state of RM pin
    };

    LineRingState lineStates_[cfg::mcp::SHK_LINE_COUNT]; // State for each line
};
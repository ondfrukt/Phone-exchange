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
    void stopRingingLine(uint8_t lineNumber);
    bool isLineRinging(uint8_t lineNumber) const;

  private:
    MCPDriver& mcpDriver_;
    Settings& settings_;
    LineManager& lineManager_;

    // Ringing state machine
    enum class RingState {
      RingIdle,
      RingToggling,  // Generating ring signal (FR toggling)
      RingPause      // Pause between rings
    };

    // Per-line ringing state
    struct LineRingState {
      RingState state = RingState::RingIdle;
      uint32_t currentIteration = 0;
      unsigned long stateStartTime = 0;
      unsigned long lastFRToggleTime = 0;
      bool frPinState = false; // Current state of FR pin
    };

    LineRingState lineStates_[cfg::mcp::SHK_LINE_COUNT]; // State for each line
};
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

    // Helper to get/set ring state from LineHandler
    RingState getRingState(uint8_t lineNumber) const;
    void setRingState(uint8_t lineNumber, RingState state);
};
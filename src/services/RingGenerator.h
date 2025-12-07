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
    void generatRingingSignal();
    void updateRinging();

  private:
    MCPDriver& mcpDriver_;
    Settings& settings_;
    LineManager& lineManager_;

    // Ringing state
    bool isRinging_ = false;
    unsigned long ringStartTime_ = 0;
    unsigned long lastRingToggleTime_ = 0;
    bool ringState_ = false; // true = ON, false = OFF
};
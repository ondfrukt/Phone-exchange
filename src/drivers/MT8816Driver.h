#pragma once
#include <Arduino.h>
#include "config.h"
#include "settings/settings.h"
#include "MCPDriver.h"


class MT8816Driver {
  public:
    MT8816Driver(MCPDriver& mcpDriver, Settings& settings);
    void begin();
    void SetConnection(uint8_t x, uint8_t y, bool state);
    void setAddress(uint8_t x, uint8_t y);
  private:
    void strobe();
    void reset();
    MCPDriver& mcpDriver_;
    Settings& settings_;
};
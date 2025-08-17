#pragma once

#include <Adafruit_MCP23X17.h>
#include "config.h"

class MCPDriver {
  public:
    void begin();
    void setPin(uint8_t addr, uint8_t pin, bool state);
    void readPin(uint8_t addr, uint8_t pin);

  private:
    Adafruit_MCP23X17 mcpMain;
    Adafruit_MCP23X17 mcpMT8816;
    Adafruit_MCP23X17 mcpSlic1;
    Adafruit_MCP23X17 mcpSlic2;
};

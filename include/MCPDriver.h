#pragma once

#include <Adafruit_MCP23X17.h>
#include "config.h"

class MCPDriver {
  public:
    void begin();
    // Getters till de olika MCP:erna
    Adafruit_MCP23X17& mainDev()  { return mcpMain;  }
    Adafruit_MCP23X17& mt8816Dev(){ return mcpMT8816;}
    Adafruit_MCP23X17& slic1Dev(){ return mcpSlic1; }
    Adafruit_MCP23X17& slic2Dev(){ return mcpSlic2; }
    void setPin(uint8_t addr, uint8_t pin, bool state);
    void readPin(uint8_t addr, uint8_t pin);

  private:
    Adafruit_MCP23X17 mcpMain;
    Adafruit_MCP23X17 mcpMT8816;
    Adafruit_MCP23X17 mcpSlic1;
    Adafruit_MCP23X17 mcpSlic2;
};

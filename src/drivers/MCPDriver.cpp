#include "MCPDriver.h"
using namespace cfg;

void MCPDriver::SetMCPPinModes(Adafruit_MCP23X17& mcp, const mcp::PinModeEntry (&list)[16]) {
  for (uint8_t pin = 0; pin < 16; ++pin) {
    mcp.pinMode(pin, list[pin].mode);
    if (list[pin].mode == OUTPUT) {
      mcp.digitalWrite(pin, list[pin].initial ? HIGH : LOW);
    }
  }
}

bool MCPDriver::begin() {
  // Initiera I²C (du kan ta bort om du gör detta i setup())
  Wire.begin(i2c::SDA_PIN, i2c::SCL_PIN);
  pinMode(cfg::mcp::MCP_SLIC_INT_1_PIN, INPUT_PULLUP);
  pinMode(cfg::mcp::MCP_SLIC_INT_2_PIN, INPUT_PULLUP);
  pinMode(cfg::mcp::MCP_MAIN_INT_PIN, INPUT_PULLUP);

  // Initiera alla MCP:er
  if (!setupMain())    return false;
  if (!setupMT8816())  return false;
  if (!setupSlics())   return false;

  return true;
}

bool MCPDriver::setupMain() {
  if (!mcpMain.begin_I2C(mcp::MCP_MAIN_ADDRESS))
    return false;
  SetMCPPinModes(mcpMain, mcp::MCP_MAIN);
  return true;
}

bool MCPDriver::setupMT8816() {
  if (!mcpMT8816.begin_I2C(mcp::MCP_MT8816_ADDRESS))
    return false;
  SetMCPPinModes(mcpMT8816, mcp::MCP_MT8816);
  return true;
}

bool MCPDriver::setupSlics() {
  if (!mcpSlic1.begin_I2C(mcp::MCP_SLIC1_ADDRESS))
    return false;
  SetMCPPinModes(mcpSlic1, mcp::MCP_SLIC);
  mcpSlic1.setupInterrupts(/*mirror*/ true, /*openDrain*/ true, /*activeLow*/ true);
  enableSlicShkInterrupts_(mcpSlic1);
  return true;
}

void MCPDriver::enableSlicShkInterrupts_(Adafruit_MCP23X17& mcp) {
  // Aktivera CHANGE-interrupt på alla SHK-pinnar från din config
  // (lista innehåller 4 MCP-pinnar: 4,5,8,11)
  for (uint8_t i = 0; i < 4; ++i) {            // 4 st definierade nu
    uint8_t pin = cfg::mcp::SHK_PINS[i];       // :contentReference[oaicite:3]{index=3}
    mcp.setupInterruptPin(pin, CHANGE);
  }
}
#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>

// Framåtdeklaration (för att slippa cirkelberoende i headers)
class MCPInterruptService;

namespace mcp_reg {
// MCP23X17 registeradresser (Bank=0, standard)
static constexpr uint8_t INTF_A   = 0x0E;
static constexpr uint8_t INTF_B   = 0x0F;
static constexpr uint8_t INTCAP_A = 0x10;
static constexpr uint8_t INTCAP_B = 0x11;
}

// Resultat från en samlad INT-hantering
struct IntResult {
  bool    hasEvent = false; // true om någon källa hittades
  uint8_t pin      = 0;     // 0..15 (MCP-pinindex)
  bool    level    = false; // nivå vid event (latched/last enligt API)
};

class MCPDriver {
public:
  // Init av dina MCP:er (anpassa efter din befintliga kod)
  bool begin();

  // Samlade entry-points – en per MCP du lyssnar på
  IntResult handleMainInterrupt(MCPInterruptService& isrSvc);
  IntResult handleSlic1Interrupt(MCPInterruptService& isrSvc);
  IntResult handleMT8816Interrupt(MCPInterruptService& isrSvc);

private:
  // Gemensam intern hanterare (för att undvika duplikatkod)
  IntResult handleInterrupt_(MCPInterruptService& isrSvc,
                             Adafruit_MCP23X17& mcp,
                             uint8_t i2c_addr);

  // Fallback om ditt Adafruit-lib saknar getLastInterruptPin()
  IntResult readIntfIntcap_(uint8_t i2c_addr);

  // Low-level I2C helpers
  uint8_t  readReg8_(uint8_t i2c_addr, uint8_t reg);
  void     readRegPair16_(uint8_t i2c_addr, uint8_t regA, uint16_t& out16);

private:
  // Dina MCP-instansobjekt (anpassa namn efter din kod)
  Adafruit_MCP23X17 mcpMain;
  Adafruit_MCP23X17 mcpSlic1;
  Adafruit_MCP23X17 mcpMT8816;
};

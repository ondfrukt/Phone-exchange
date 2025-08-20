#include "MCPDriver.h"
#include "MCPInterruptService.h"
#include "config.h"   // antas innehålla cfg::mcp::{MCP_MAIN_ADDRESS, MCP_SLIC1_ADDRESS, MCP_MT8816_ADDRESS}

using namespace mcp_reg;

bool MCPDriver::begin() {
  // Init I2C (om du inte gör det annorstädes)
  Wire.begin();

  // Initiera chips – anpassa efter dina behov/konfig
  if (!mcpMain.begin_I2C(cfg::mcp::MCP_MAIN_ADDRESS))   return false;
  if (!mcpSlic1.begin_I2C(cfg::mcp::MCP_SLIC1_ADDRESS)) return false;
  if (!mcpMT8816.begin_I2C(cfg::mcp::MCP_MT8816_ADDRESS)) return false;

  // Här kan du sätta pinModes/interruptinställningar osv. om du vill.
  return true;
}

// Publika entry-points
IntResult MCPDriver::handleMainInterrupt(MCPInterruptService& isrSvc) {
  return handleInterrupt_(isrSvc, mcpMain,  cfg::mcp::MCP_MAIN_ADDRESS);
}
IntResult MCPDriver::handleSlic1Interrupt(MCPInterruptService& isrSvc) {
  return handleInterrupt_(isrSvc, mcpSlic1, cfg::mcp::MCP_SLIC1_ADDRESS);
}
IntResult MCPDriver::handleMT8816Interrupt(MCPInterruptService& isrSvc) {
  return handleInterrupt_(isrSvc, mcpMT8816, cfg::mcp::MCP_MT8816_ADDRESS);
}

// Gemensam hanterare
IntResult MCPDriver::handleInterrupt_(MCPInterruptService& isrSvc,
                                      Adafruit_MCP23X17& mcp,
                                      uint8_t i2c_addr)
{
  IntResult out;

  // Säkerställ att din ESP-flagga faktiskt är satt
  if (!isrSvc.flag_) return out;

  // Försök high-level först (räcker för din applikation)
  // Notera: getLastInterruptPin() returnerar -1 om ingen.
  int pin = mcp.getLastInterruptPin();
  if (pin >= 0) {
    out.hasEvent = true;
    out.pin      = static_cast<uint8_t>(pin);
    out.level    = mcp.getLastInterruptPinValue(); // nivå vid event (latched enligt lib)

    // Nollställ ESP-flaggan sist
    isrSvc.clearFlag();
    return out;
  }

  // Fallback: råregister (INTF -> INTCAP) för källa + kvittering
  out = readIntfIntcap_(i2c_addr);

  // Nollställ ESP-flaggan sist
  isrSvc.clearFlag();
  return out;
}

// --- Low-level helpers ---
uint8_t MCPDriver::readReg8_(uint8_t i2c_addr, uint8_t reg) {
  Wire.beginTransmission(i2c_addr);
  Wire.write(reg);
  Wire.endTransmission(false);      // repeated start
  Wire.requestFrom((int)i2c_addr, 1);
  if (Wire.available()) return Wire.read();
  return 0;
}

void MCPDriver::readRegPair16_(uint8_t i2c_addr, uint8_t regA, uint16_t& out16) {
  uint8_t a = readReg8_(i2c_addr, regA);
  uint8_t b = readReg8_(i2c_addr, regA + 1);
  out16 = (uint16_t(b) << 8) | uint16_t(a);
}

// Läs INTF (orsaker), sedan INTCAP (snapshot + kvittering). Plocka en (lägsta) pin.
IntResult MCPDriver::readIntfIntcap_(uint8_t i2c_addr) {
  IntResult out;

  uint16_t intf = 0, cap = 0;
  readRegPair16_(i2c_addr, INTF_A,   intf);   // vilka pinnar triggat
  readRegPair16_(i2c_addr, INTCAP_A, cap);    // latched nivåer + kvittering

  if (intf == 0) return out;

  // Din applikation: samtidighet låg -> ta lägsta satta bit
  uint8_t pin = __builtin_ctz(intf);          // index för lägsta 1-bit
  out.hasEvent = true;
  out.pin      = pin;
  out.level    = (cap >> pin) & 0x1;
  return out;
}

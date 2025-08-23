#pragma once
#include <Arduino.h>
#include <Wire.h> 
#include <Adafruit_MCP23X17.h>



// Framåtdeklaration av config-namespace.
// Själva definierade värdena (adresser, ESP-GPIO) kommer från din config.h i .cpp-filen.
namespace cfg { namespace mcp {
  extern const uint8_t MCP_MAIN_ADDRESS;
  extern const uint8_t MCP_SLIC1_ADDRESS;
  extern const uint8_t MCP_MT8816_ADDRESS;

  extern const int MCP_MAIN_INT_PIN;     // ESP32-GPIO som läser INT från "Main"-MCP
  extern const int MCP_SLIC_INT_1_PIN;   // ESP32-GPIO som läser INT från SLIC-MCP
  extern const int MCP_SLIC_INT_2_PIN;   // Ifall du har en tredje INT-lina (ex. MT8816)
}}

// ======= Returnpaket för interrupt-händelser =======
struct IntResult {
  bool    hasEvent = false;  // om något fanns att hämta
  uint8_t line     = 255;    // 0..7, 255 = okänd linje
  uint8_t pin      = 255;    // 0..15, 255 = ogiltig
  bool    level    = false;  // nivå enligt INTCAP/getLastInterruptValue()
  uint8_t i2c_addr = 0x00;   // vilken MCP
};

class MCPDriver {
public:
  MCPDriver() = default;

  // Initiera alla MCP:er, lägg deras GPIO-lägen, INT-egenskaper och
  // koppla ESP32-interrupts (fallande flank; open-drain).
  bool begin();

  // ===== Basala GPIO-funktioner =====
  bool digitalWriteMCP(uint8_t i2c_addr, uint8_t pin, bool value);
  bool digitalReadMCP (uint8_t i2c_addr, uint8_t pin, bool& out);

  // Snabbhjälp för kända kretsar
  inline Adafruit_MCP23X17& mainChip()   { return mcpMain_;   }
  inline Adafruit_MCP23X17& slic1Chip()  { return mcpSlic1_;  }
  inline Adafruit_MCP23X17& mt8816Chip() { return mcpMT8816_; }

  // ===== Loop-hanterare (pollas från loop()) =====
  IntResult handleMainInterrupt();
  IntResult handleSlic1Interrupt();
  IntResult handleMT8816Interrupt();

private:
  // ===== ISR-thunks (sätter endast flaggor) =====
  static void IRAM_ATTR isrMainThunk (void* arg);
  static void IRAM_ATTR isrSlic1Thunk(void* arg);
  static void IRAM_ATTR isrMT8816Thunk(void* arg);

  // Gemensam interrupt-hantering
  IntResult handleInterrupt_(volatile bool& flag, Adafruit_MCP23X17& mcp, uint8_t i2c_addr);

  // Fallback om biblioteket saknar/inte rapporterar pin/value korrekt
  IntResult readIntfIntcapFallback_(uint8_t i2c_addr);

  // Låg-nivå I2C registerläsning
  uint8_t  readReg8_(uint8_t i2c_addr, uint8_t reg);
  void     readRegPair16_(uint8_t i2c_addr, uint8_t regA, uint16_t& out16);

  // Setup-steg
  bool setupMain_();
  bool setupSlics_();
  bool setupMT8816_();

  // Aktivera INT på SLIC-SHK-pinnarna, CHANGE-trigger
  void enableSlicShkInterrupts_(Adafruit_MCP23X17& mcp);

  // Applicera pinlägen från cfg::mcp::MCP_* arrayer
  bool applyPinModes_(Adafruit_MCP23X17& mcp, const uint8_t (&modes)[16], const bool (&initial)[16]);

private:
  Adafruit_MCP23X17 mcpMain_;
  Adafruit_MCP23X17 mcpSlic1_;
  Adafruit_MCP23X17 mcpMT8816_;

  volatile bool mainIntFlag_   = false;
  volatile bool slic1IntFlag_  = false;
  volatile bool mt8816IntFlag_ = false;

  int8_t mapSlicPinToLine_(uint8_t pin) const;
};
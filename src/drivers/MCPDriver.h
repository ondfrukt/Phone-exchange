#pragma once
#include <Arduino.h>
#include <Wire.h> 
#include <Adafruit_MCP23X17.h>
#include "settings/settings.h"
#include "config.h"

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

  bool haveMain_;
  bool haveSlic1_;
  bool haveSlic2_;
  bool haveMT8816_;

  // Initiera alla MCP:er, lägg deras GPIO-lägen, INT-egenskaper och
  // koppla ESP32-interrupts (fallande flank; open-drain).
  bool begin();

  // ===== Basala GPIO-funktioner =====
  bool digitalWriteMCP(uint8_t i2c_addr, uint8_t pin, bool value);
  bool digitalReadMCP (uint8_t i2c_addr, uint8_t pin, bool& out);

  bool readGpioAB16(uint8_t i2c_addr, uint16_t& out16);

  // Snabbhjälp för kända kretsar
  inline Adafruit_MCP23X17& mainChip()   { return mcpMain_;   }
  inline Adafruit_MCP23X17& slic1Chip()  { return mcpSlic1_;  }
  inline Adafruit_MCP23X17& slic2Chip()  { return mcpSlic2_;  }
  inline Adafruit_MCP23X17& mt8816Chip() { return mcpMT8816_; }

  // ===== Loop-hanterare (pollas från loop()) =====
  IntResult handleMainInterrupt();
  IntResult handleSlic1Interrupt();
  IntResult handleSlic2Interrupt();
  IntResult handleMT8816Interrupt();

private:
  // ===== ISR-thunks (sätter endast flaggor) =====
  static void IRAM_ATTR isrMainThunk (void* arg);
  static void IRAM_ATTR isrSlic1Thunk(void* arg);
  static void IRAM_ATTR isrSlic2Thunk(void* arg);
  static void IRAM_ATTR isrMT8816Thunk(void* arg);

  // Gemensam interrupt-hantering
  IntResult handleInterrupt_(volatile bool& flag, Adafruit_MCP23X17& mcp, uint8_t i2c_addr);

  // Fallback om biblioteket saknar/inte rapporterar pin/value korrekt
  IntResult readIntfIntcapFallback_(uint8_t i2c_addr);

  // Låg-nivå I2C registerläsning
  uint8_t  readReg8_(uint8_t i2c_addr, uint8_t reg);
  void     readRegPair16_(uint8_t i2c_addr, uint8_t regA, uint16_t& out16);

  // Aktivera INT på SLIC-SHK-pinnarna, CHANGE-trigger
  void enableSlicShkInterrupts_(uint8_t i2cAddr, Adafruit_MCP23X17& mcp);

  // Aktivera INT på MAIN, ex. knapp på GPB0
  void enableMainInterrupts_(uint8_t i2cAddr, Adafruit_MCP23X17& mcp);

  // Applicera pinlägen från cfg::mcp::MCP_* arrayer
  bool applyPinModes_(Adafruit_MCP23X17& mcp, const uint8_t (&modes)[16], const bool (&initial)[16]);

  Adafruit_MCP23X17 mcpMain_;
  Adafruit_MCP23X17 mcpSlic1_;
  Adafruit_MCP23X17 mcpSlic2_;
  Adafruit_MCP23X17 mcpMT8816_;

  volatile bool mainIntFlag_   = false;
  volatile bool slic1IntFlag_  = false;
  volatile bool slic2IntFlag_  = false;
  volatile bool mt8816IntFlag_ = false;

  int8_t mapSlicPinToLine_(uint8_t addr, uint8_t pin) const;

  // === [NYTT] Säkra I2C-hjälpare (deklarationer) ===
  bool writeReg8_(uint8_t addr, uint8_t reg, uint8_t val);
  bool readReg8_OK_(uint8_t addr, uint8_t reg, uint8_t& out);
  bool readRegPair16_OK_(uint8_t addr, uint8_t regA, uint16_t& out16);

  // Prova att initiera en MCP och returnera true om den svarar
  bool probeMcp_(Adafruit_MCP23X17& mcp, uint8_t addr);

};

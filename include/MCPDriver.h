#pragma once
#include <Arduino.h>
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

// ------------------------------
// Litet paket med resultatdata från ett hanterat interrupt.
// hasEvent = true om något fanns att hämta
// pin      = vilken MCP-pin (0..15) som orsakade interrupt
// level    = nivå vid event (latched/“last” enligt biblioteket)
// ------------------------------
struct IntEvent {
  bool    hasEvent = false;
  uint8_t pin      = 0;
  bool    level    = false;
};

class MCPDriver {
public:
  MCPDriver() = default;

  // Sätter upp MCP-kretsar och kopplar in ESP32-interrupts.
  // OBS: Vi anropar INTE Wire.begin() här, eftersom du gör det någon annanstans.
  bool begin();

  // Anropas från din loop(). Varje metod:
  // - returnerar IntEvent om en händelse fångats sedan sist
  // - kvitterar händelsen i MCP:n
  // - nollställer intern ESP-flagga
  IntEvent handleMainInterrupt();
  IntEvent handleSlic1Interrupt();
  IntEvent handleMT8816Interrupt();

  // (Valfritt) Exponera MCP:erna om du behöver dem externt
  Adafruit_MCP23X17& mainChip()   { return mcpMain_;   }
  Adafruit_MCP23X17& slic1Chip()  { return mcpSlic1_;  }
  Adafruit_MCP23X17& mt8816Chip() { return mcpMT8816_; }

private:
  // ===== ESP32 ISR “thunks” =====
  // Dessa är statiska små C-kompatibla funktioner (thunks) som får 'this' via arg,
  // castar tillbaka till MCPDriver* och sätter respektive flagga. De gör INGENTING annat.
  static void IRAM_ATTR isrMainThunk(void* arg);
  static void IRAM_ATTR isrSlic1Thunk(void* arg);
  static void IRAM_ATTR isrMT8816Thunk(void* arg);

  // Gemensam funktion som gör själva jobbet för en instans (minskar duplikatkod)
  IntEvent handleInterrupt_(volatile bool& flag,
                            Adafruit_MCP23X17& mcp,
                            uint8_t i2c_addr);

  // Fallback om biblioteket saknar getLastInterruptPin()/Value()
  IntEvent readIntfIntcapFallback_(uint8_t i2c_addr);

  // Låg-nivå registerläsning (I2C). Wire.begin() gör du själv på annat ställe.
  uint8_t  readReg8_(uint8_t i2c_addr, uint8_t reg);
  void     readRegPair16_(uint8_t i2c_addr, uint8_t regA, uint16_t& out16);

  // Hjälpmetoder för MCP-intern setup (anpassa efter ditt projekt)
  bool setupMain_();
  bool setupSlics_();
  bool setupMT8816_();
  void enableSlicShkInterrupts_(Adafruit_MCP23X17& mcp);

private:
  // ===== MCP-instansobjekt =====
  Adafruit_MCP23X17 mcpMain_;
  Adafruit_MCP23X17 mcpSlic1_;
  Adafruit_MCP23X17 mcpMT8816_;

  // ===== ESP-flaggor som ISR sätter =====
  // 'volatile' = kan ändras “när som helst” av ISR:n.
  volatile bool mainIntFlag_   = false;
  volatile bool slic1IntFlag_  = false;
  volatile bool mt8816IntFlag_ = false;
};

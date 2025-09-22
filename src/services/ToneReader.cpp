#include "services/ToneReader.h"

ToneReader::ToneReader(MCPDriver& mcpDriver, Settings& settings, LineManager& lineManager)
  : mcpDriver_(mcpDriver), settings_(settings), lineManager_(lineManager) {}

void ToneReader::update() {
  // Hantera MAIN-interrupts (bl.a. MT8870 STD). Töm alla väntande events.
  while (true) {
    IntResult ir = mcpDriver_.handleMainInterrupt();
    if (!ir.hasEvent) break;

    // Vi bryr oss bara om STD-pinnen från MT8870
    if (ir.i2c_addr == cfg::mcp::MCP_MAIN_ADDRESS && ir.pin == cfg::mcp::STD) {
      // STD blir hög när en giltig ton detekterats. Läs nibbeln på rising edge.
      if (ir.level) {
        uint8_t nibble = 0;
        if (readDtmfNibble(nibble)) {
          char ch = decodeDtmf(nibble);
          if (settings_.debugMTLevel >= 2) {
            Serial.print(F("DTMF: nibble=0x"));
            if (nibble < 16) Serial.print(nibble, HEX); else Serial.print('?');
            Serial.print(F(" => '"));
            Serial.print(ch);
            Serial.println('\'');
          }

          if (ch != '\0') {
            int idx = lineManager_.lastLineReady; // "senast aktiv" (Ready)
            if (idx >= 0) {
              auto& line = lineManager_.getLine(idx);
              line.dialedDigits += ch;

              if (settings_.debugMTLevel >= 1) {
                Serial.print(F("DTMF: line "));
                Serial.print(idx);
                Serial.print(F(" +="));
                Serial.println(ch);
              }
            } else if (settings_.debugMTLevel >= 1) {
              Serial.println(F("DTMF: ingen lastLineReady att lagra på"));
            }
          }
        } else if (settings_.debugMTLevel >= 1) {
          Serial.println(F("DTMF: kunde inte läsa nibble"));
        }
      } else {
        // Falling edge på STD – ignoreras (giltig data läses vid high)
      }
    }
  }
}

bool ToneReader::readDtmfNibble(uint8_t& nibble) {
  // Läs aktuell GPIO-status (STD är hög nu => MT8870 håller data stabil)
  uint16_t gpioAB = 0;
  if (!mcpDriver_.readGpioAB16(cfg::mcp::MCP_MAIN_ADDRESS, gpioAB)) return false;

  // Q1..Q4 i cfg::mcp är pinindex 0..15 i 16-bitarsordet (A=0..7, B=8..15)
  const bool q1 = (gpioAB & (1u << cfg::mcp::Q1)) != 0; // LSB
  const bool q2 = (gpioAB & (1u << cfg::mcp::Q2)) != 0;
  const bool q3 = (gpioAB & (1u << cfg::mcp::Q3)) != 0;
  const bool q4 = (gpioAB & (1u << cfg::mcp::Q4)) != 0; // MSB

  nibble = (static_cast<uint8_t>(q4) << 3) |
           (static_cast<uint8_t>(q3) << 2) |
           (static_cast<uint8_t>(q2) << 1) |
           (static_cast<uint8_t>(q1) << 0);
  return true;
}

char ToneReader::decodeDtmf(uint8_t nibble) {
  if (nibble < 16) return dtmf_map_[nibble];
  return '\0';
}
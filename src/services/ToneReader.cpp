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
      if (settings_.debugTRLevel >= 1) {
        Serial.print(F("DTMF: interrupt addr=0x"));
        Serial.print(ir.i2c_addr, HEX);
        Serial.print(F(" pin="));
        Serial.print(ir.pin);
        Serial.print(F(" level="));
        Serial.println(ir.level ? F("HIGH") : F("LOW"));
        util::UIConsole::log("DTMF INT 0x" + String(ir.i2c_addr, HEX) +
                                 " pin=" + String(ir.pin) +
                                 " level=" + String(ir.level ? "HIGH" : "LOW"),
                             "ToneReader");
      }
      // Detect rising edge (LOW -> HIGH transition)
      bool risingEdge = ir.level && !lastStdLevel_;
      lastStdLevel_ = ir.level;
      
      // STD blir hög när en giltig ton detekterats. Läs nibbeln på rising edge.
      if (risingEdge) {
        unsigned long now = millis();
        uint8_t nibble = 0;
        
        if (readDtmfNibble(nibble)) {
          // Check debouncing: ignore if same digit detected within debounce period
          // Use unsigned subtraction which handles millis() rollover correctly
          unsigned long timeSinceLastDtmf = now - lastDtmfTime_;
          bool isSameDigit = (nibble == lastDtmfNibble_);
          bool withinDebounceWindow = (timeSinceLastDtmf < DTMF_DEBOUNCE_MS);
          bool isDuplicate = isSameDigit && withinDebounceWindow;
          
          if (!isDuplicate) {
            lastDtmfTime_ = now;
            lastDtmfNibble_ = nibble;
            
            char ch = decodeDtmf(nibble);
            if (settings_.debugTRLevel >= 2) {
              Serial.print(F("DTMF: nibble=0x"));
              if (nibble < 16) Serial.print(nibble, HEX); else Serial.print('?');
              Serial.print(F(" => '"));
              Serial.print(ch);
              Serial.println('\'');
              util::UIConsole::log("DTMF: nibble=0x" + String(nibble, HEX) +
                                       " => '" + String(ch) + "'",
                                   "ToneReader");
            }

            if (ch != '\0') {
              int idx = lineManager_.lastLineReady; // "senast aktiv" (Ready)
              if (idx >= 0) {
                auto& line = lineManager_.getLine(idx);
                line.dialedDigits += ch;

                if (settings_.debugTRLevel >= 1) {
                  Serial.print(F("DTMF: line "));
                  Serial.print(idx);
                  Serial.print(F(" +="));
                  Serial.println(ch);
                  util::UIConsole::log("DTMF: line " + String(idx) + " +=" + String(ch), "ToneReader");
                }
              } else if (settings_.debugTRLevel >= 1) {
                Serial.println(F("DTMF: ingen lastLineReady att lagra på"));
                util::UIConsole::log("DTMF: ingen lastLineReady att lagra på", "ToneReader");
              }
            }
          } else if (settings_.debugTRLevel >= 2) {
            Serial.print(F("DTMF: duplicate ignored (debouncing) nibble=0x"));
            Serial.print(nibble, HEX);
            Serial.print(F(" time="));
            Serial.print(timeSinceLastDtmf);
            Serial.println(F("ms"));
            util::UIConsole::log("DTMF: duplicate ignored (debouncing) nibble=0x" + 
                                String(nibble, HEX) + " time=" + String(timeSinceLastDtmf) + "ms", 
                                "ToneReader");
          }
        } else if (settings_.debugTRLevel >= 1) {
          Serial.println(F("DTMF: kunde inte läsa nibble"));
          util::UIConsole::log("DTMF: kunde inte läsa nibble", "ToneReader");
        }
      }
      // Falling edge på STD – no action needed (giltig data läses vid rising edge)
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

  if (settings_.debugTRLevel >= 2) {
    Serial.print(F("DTMF: GPIOAB="));
    Serial.print(gpioAB, BIN);
    Serial.print(F(" q4..1="));
    Serial.print(q4);
    Serial.print(q3);
    Serial.print(q2);
    Serial.println(q1);
    util::UIConsole::log("DTMF: gpioAB=" + String(gpioAB, BIN) +
                             " q4=" + String(q4) + " q3=" + String(q3) +
                             " q2=" + String(q2) + " q1=" + String(q1),
                         "ToneReader");
  }
  return true;
}

char ToneReader::decodeDtmf(uint8_t nibble) {
  if (nibble < 16) return dtmf_map_[nibble];
  return '\0';
}
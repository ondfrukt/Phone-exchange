#include "services/ToneReader.h"

ToneReader::ToneReader(MCPDriver& mcpDriver, Settings& settings, LineManager& lineManager)
  : mcpDriver_(mcpDriver), settings_(settings), lineManager_(lineManager) {}

void ToneReader::update() {
  // Debug: Periodically check STD pin state
  static unsigned long lastStdCheck = 0;
  unsigned long now = millis();
  if (settings_.debugTRLevel >= 3 && (now - lastStdCheck >= 1000)) {
    bool stdLevel = false;
    if (mcpDriver_.digitalReadMCP(cfg::mcp::MCP_MAIN_ADDRESS, cfg::mcp::STD, stdLevel)) {
      Serial.print(F("DTMF DEBUG: STD pin current state: "));
      Serial.println(stdLevel ? F("HIGH") : F("LOW"));
    }
    lastStdCheck = now;
  }
  
  // Hantera MAIN-interrupts (bl.a. MT8870 STD). Töm alla väntande events.
  while (true) {
    IntResult ir = mcpDriver_.handleMainInterrupt();
    if (!ir.hasEvent) break;

    if (settings_.debugTRLevel >= 2) {
      Serial.print(F("DTMF: Received interrupt event - addr=0x"));
      Serial.print(ir.i2c_addr, HEX);
      Serial.print(F(" pin="));
      Serial.print(ir.pin);
      Serial.print(F(" level="));
      Serial.println(ir.level ? F("HIGH") : F("LOW"));
    }

    // Vi bryr oss bara om STD-pinnen från MT8870
    if (ir.i2c_addr == cfg::mcp::MCP_MAIN_ADDRESS && ir.pin == cfg::mcp::STD) {
      if (settings_.debugTRLevel >= 1) {
        Serial.print(F("DTMF: STD interrupt detected - addr=0x"));
        Serial.print(ir.i2c_addr, HEX);
        Serial.print(F(" pin="));
        Serial.print(ir.pin);
        Serial.print(F(" level="));
        Serial.println(ir.level ? F("HIGH") : F("LOW"));
        util::UIConsole::log("DTMF STD INT 0x" + String(ir.i2c_addr, HEX) +
                                 " pin=" + String(ir.pin) +
                                 " level=" + String(ir.level ? "HIGH" : "LOW"),
                             "ToneReader");
      }
      // Detect rising edge (LOW -> HIGH transition)
      bool risingEdge = ir.level && !lastStdLevel_;
      
      if (settings_.debugTRLevel >= 2) {
        Serial.print(F("DTMF: Edge detection - lastStdLevel_="));
        Serial.print(lastStdLevel_ ? F("HIGH") : F("LOW"));
        Serial.print(F(" currentLevel="));
        Serial.print(ir.level ? F("HIGH") : F("LOW"));
        Serial.print(F(" risingEdge="));
        Serial.println(risingEdge ? F("YES") : F("NO"));
      }
      
      lastStdLevel_ = ir.level;
      
      // STD blir hög när en giltig ton detekterats. Läs nibbeln på rising edge.
      if (risingEdge) {
        if (settings_.debugTRLevel >= 1) {
          Serial.println(F("DTMF: Rising edge detected - attempting to read DTMF nibble"));
          util::UIConsole::log("DTMF: Rising edge detected - attempting to read DTMF nibble", "ToneReader");
        }
        
        unsigned long now = millis();
        uint8_t nibble = 0;
        
        if (readDtmfNibble(nibble)) {
          if (settings_.debugTRLevel >= 1) {
            Serial.print(F("DTMF: Successfully read nibble=0x"));
            Serial.println(nibble, HEX);
          }
          
          // Check debouncing: ignore if same digit detected within debounce period
          // Use unsigned subtraction which handles millis() rollover correctly
          unsigned long timeSinceLastDtmf = now - lastDtmfTime_;
          bool isSameDigit = (nibble == lastDtmfNibble_);
          bool withinDebounceWindow = (timeSinceLastDtmf < DTMF_DEBOUNCE_MS);
          bool isDuplicate = isSameDigit && withinDebounceWindow;
          
          if (settings_.debugTRLevel >= 2) {
            Serial.print(F("DTMF: Debounce check - timeSince="));
            Serial.print(timeSinceLastDtmf);
            Serial.print(F("ms isSameDigit="));
            Serial.print(isSameDigit ? F("YES") : F("NO"));
            Serial.print(F(" withinWindow="));
            Serial.print(withinDebounceWindow ? F("YES") : F("NO"));
            Serial.print(F(" isDuplicate="));
            Serial.println(isDuplicate ? F("YES") : F("NO"));
          }
          
          if (!isDuplicate) {
            lastDtmfTime_ = now;
            lastDtmfNibble_ = nibble;
            
            char ch = decodeDtmf(nibble);
            if (settings_.debugTRLevel >= 1) {
              Serial.print(F("DTMF: DECODED - nibble=0x"));
              if (nibble < 16) Serial.print(nibble, HEX); else Serial.print('?');
              Serial.print(F(" => char='"));
              Serial.print(ch);
              Serial.println('\'');
              util::UIConsole::log("DTMF DECODED: nibble=0x" + String(nibble, HEX) +
                                       " => '" + String(ch) + "'",
                                   "ToneReader");
            }

            if (ch != '\0') {
              int idx = lineManager_.lastLineReady; // "senast aktiv" (Ready)
              if (idx >= 0) {
                auto& line = lineManager_.getLine(idx);
                line.dialedDigits += ch;

                if (settings_.debugTRLevel >= 1) {
                  Serial.print(F("DTMF: Added to line "));
                  Serial.print(idx);
                  Serial.print(F(" digit='"));
                  Serial.print(ch);
                  Serial.print(F("' dialedDigits=\""));
                  Serial.print(line.dialedDigits);
                  Serial.println('"');
                  util::UIConsole::log("DTMF: line " + String(idx) + " +=" + String(ch) + 
                                      " dialedDigits=\"" + line.dialedDigits + "\"", "ToneReader");
                }
              } else {
                if (settings_.debugTRLevel >= 1) {
                  Serial.println(F("DTMF: WARNING - No lastLineReady available to store digit"));
                  util::UIConsole::log("DTMF: WARNING - No lastLineReady available", "ToneReader");
                }
              }
            } else {
              if (settings_.debugTRLevel >= 1) {
                Serial.print(F("DTMF: WARNING - Decoded character is NULL (invalid nibble=0x"));
                Serial.print(nibble, HEX);
                Serial.println(F(")"));
                util::UIConsole::log("DTMF: WARNING - Decoded character is NULL (invalid nibble=0x" + 
                                    String(nibble, HEX) + ")", "ToneReader");
              }
            }
          } else if (settings_.debugTRLevel >= 1) {
            Serial.print(F("DTMF: Duplicate ignored (debouncing) nibble=0x"));
            Serial.print(nibble, HEX);
            Serial.print(F(" time="));
            Serial.print(timeSinceLastDtmf);
            Serial.println(F("ms"));
            util::UIConsole::log("DTMF: duplicate ignored (debouncing) nibble=0x" + 
                                String(nibble, HEX) + " time=" + String(timeSinceLastDtmf) + "ms", 
                                "ToneReader");
          }
        } else {
          if (settings_.debugTRLevel >= 1) {
            Serial.println(F("DTMF: ERROR - Failed to read DTMF nibble from MCP"));
            util::UIConsole::log("DTMF: ERROR - Failed to read nibble", "ToneReader");
          }
        }
      } else if (settings_.debugTRLevel >= 2) {
        // Falling edge - just for debugging
        if (!ir.level) {
          Serial.println(F("DTMF: Falling edge detected (STD went LOW)"));
        }
      }
      // Falling edge på STD – no action needed (giltig data läses vid rising edge)
    } else if (settings_.debugTRLevel >= 2) {
      // Interrupt from different pin
      Serial.print(F("DTMF: Ignoring interrupt from pin "));
      Serial.print(ir.pin);
      Serial.print(F(" (not STD="));
      Serial.print(cfg::mcp::STD);
      Serial.println(F(")"));
    }
  }
}

bool ToneReader::readDtmfNibble(uint8_t& nibble) {
  if (settings_.debugTRLevel >= 2) {
    Serial.println(F("DTMF: Reading DTMF nibble from MCP GPIO..."));
  }
  
  // Läs aktuell GPIO-status (STD är hög nu => MT8870 håller data stabil)
  uint16_t gpioAB = 0;
  if (!mcpDriver_.readGpioAB16(cfg::mcp::MCP_MAIN_ADDRESS, gpioAB)) {
    if (settings_.debugTRLevel >= 1) {
      Serial.println(F("DTMF: ERROR - Failed to read GPIO from MCP_MAIN"));
    }
    return false;
  }

  if (settings_.debugTRLevel >= 2) {
    Serial.print(F("DTMF: Raw GPIOAB=0b"));
    for (int i = 15; i >= 0; i--) {
      Serial.print((gpioAB & (1 << i)) ? '1' : '0');
    }
    Serial.print(F(" (0x"));
    Serial.print(gpioAB, HEX);
    Serial.println(F(")"));
  }

  // Q1..Q4 i cfg::mcp är pinindex 0..15 i 16-bitarsordet (A=0..7, B=8..15)
  const bool q1 = (gpioAB & (1u << cfg::mcp::Q1)) != 0; // LSB
  const bool q2 = (gpioAB & (1u << cfg::mcp::Q2)) != 0;
  const bool q3 = (gpioAB & (1u << cfg::mcp::Q3)) != 0;
  const bool q4 = (gpioAB & (1u << cfg::mcp::Q4)) != 0; // MSB

  nibble = (static_cast<uint8_t>(q4) << 3) |
           (static_cast<uint8_t>(q3) << 2) |
           (static_cast<uint8_t>(q2) << 1) |
           (static_cast<uint8_t>(q1) << 0);

  if (settings_.debugTRLevel >= 1) {
    Serial.print(F("DTMF: MT8870 pins - Q4="));
    Serial.print(q4);
    Serial.print(F(" Q3="));
    Serial.print(q3);
    Serial.print(F(" Q2="));
    Serial.print(q2);
    Serial.print(F(" Q1="));
    Serial.print(q1);
    Serial.print(F(" => nibble=0b"));
    for (int i = 3; i >= 0; i--) {
      Serial.print((nibble & (1 << i)) ? '1' : '0');
    }
    Serial.print(F(" (0x"));
    Serial.print(nibble, HEX);
    Serial.println(F(")"));
    util::UIConsole::log("DTMF: Q4=" + String(q4) + " Q3=" + String(q3) +
                             " Q2=" + String(q2) + " Q1=" + String(q1) + 
                             " nibble=0x" + String(nibble, HEX),
                         "ToneReader");
  }
  return true;
}

char ToneReader::decodeDtmf(uint8_t nibble) {
  if (nibble < 16) return dtmf_map_[nibble];
  return '\0';
}
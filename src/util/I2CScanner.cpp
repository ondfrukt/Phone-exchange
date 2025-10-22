#include "I2CScanner.h"
#include <Arduino.h>

void I2CScanner::printHex(uint8_t v) {
  if (v < 16) Serial.print('0');
  Serial.print(v, HEX);
}

size_t I2CScanner::scan(bool verbose) {
  size_t found = 0;

  if (verbose) {
    Serial.println();
    Serial.print("[I2C] Startar scanning (0x");
    printHex(_start);
    Serial.print(" - 0x");
    printHex(_end);
    Serial.println(") ...");
  }

  for (uint8_t address = _start; address <= _end; ++address) {
    _wire.beginTransmission(address);
    uint8_t error = _wire.endTransmission(true); // true = send STOP

    if (error == 0) {
      Serial.print("[I2C] Hittad enhet på 0x");
      printHex(address);
      Serial.println();
      ++found;
    } else if (error == 4) {
      Serial.print("[I2C] Okänt fel vid 0x");
      printHex(address);
      Serial.println();
    }

    if (_delayBetween) {
      delay(_delayBetween);
    }
  }

  if (verbose) {
    if (found == 0) {
      _out.println(F("[I2C] Inga enheter hittades."));
      util::UIConsole::log("I2C scan complete. No devices found.", "I2CScanner");
    } else {
      _out.print(F("[I2C] Färdig. Antal enheter: "));
      _out.println(found);
      util::UIConsole::log("I2C scan complete. Devices found: " + String(found), "I2CScanner");
    }
    _out.println();
  }

  return found;
}
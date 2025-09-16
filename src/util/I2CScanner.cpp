#include "I2CScanner.h"

void I2CScanner::printHex(uint8_t v) {
  if (v < 16) _out.print('0');
  _out.print(v, HEX);
}

size_t I2CScanner::scan(bool verbose) {
  size_t found = 0;

  if (verbose) {
    _out.println();
    _out.print(F("[I2C] Startar scanning (0x"));
    printHex(_start);
    _out.print(F(" - 0x"));
    printHex(_end);
    _out.println(F(") ..."));
  }

  for (uint8_t address = _start; address <= _end; ++address) {
    _wire.beginTransmission(address);
    uint8_t error = _wire.endTransmission(true); // true = send STOP

    if (error == 0) {
      // En enhet svarade på adressen
      _out.print(F("[I2C] Hittad enhet på 0x"));
      printHex(address);
      _out.println();
      ++found;
    } else if (error == 4) {
      // Okänt fel
      _out.print(F("[I2C] Okänt fel vid 0x"));
      printHex(address);
      _out.println();
    }

    if (_delayBetween) {
      delay(_delayBetween);
    }
  }

  if (verbose) {
    if (found == 0) {
      _out.println(F("[I2C] Inga enheter hittades."));
    } else {
      _out.print(F("[I2C] Färdig. Antal enheter: "));
      _out.println(found);
    }
    _out.println();
  }

  return found;
}
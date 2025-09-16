#pragma once

#include <Arduino.h>
#include <Wire.h>

class I2CScanner {
public:
  // wire: vilken I2C-buss som ska användas (default Wire)
  // out: var resultat ska skrivas (default Serial)
  // start/end: adressintervall (default 0x03..0x77)
  // delayBetween: ev. liten delay mellan adresser (ms)
  explicit I2CScanner(TwoWire& wire = Wire,
                      Stream& out = Serial,
                      uint8_t start = 0x03,
                      uint8_t end = 0x77,
                      uint16_t delayBetween = 0)
    : _wire(wire),
      _out(out),
      _start(start),
      _end(end),
      _delayBetween(delayBetween) {}

  // Sätter nytt intervall om du vill begränsa sökningen
  void setRange(uint8_t start, uint8_t end) {
    _start = start;
    _end = end;
  }

  // Kör en scanning och skriv resultat till _out.
  // Returnerar antal hittade enheter.
  size_t scan(bool verbose = true);

private:
  TwoWire&  _wire;
  Stream&   _out;
  uint8_t   _start;
  uint8_t   _end;
  uint16_t  _delayBetween;

  void printHex(uint8_t v);
};
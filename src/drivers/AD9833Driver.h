#pragma once

#include <Arduino.h>
#include <MD_AD9833.h>
#include <SPI.h>

class AD9833Driver {
public:
  explicit AD9833Driver(uint8_t csPin, SPIClass& spi = SPI);

  void begin();
  void setToneFrequency(float frequencyHz);
  void stopOutput();

  uint8_t csPin() const { return csPin_; }

private:
  SPIClass& spi_;
  MD_AD9833 ad9833_;
  uint8_t csPin_;
  bool started_ = false;

  static bool spiStarted_;
};


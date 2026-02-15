#include "drivers/AD9833Driver.h"

#include "config.h"

bool AD9833Driver::spiStarted_ = false;

AD9833Driver::AD9833Driver(uint8_t csPin, SPIClass& spi)
  : spi_(spi),
    ad9833_(csPin),
    csPin_(csPin) {
}

void AD9833Driver::begin() {
  if (started_) {
    return;
  }

  pinMode(csPin_, OUTPUT);
  digitalWrite(csPin_, HIGH);

  if (!spiStarted_) {
#ifdef ARDUINO_ARCH_ESP32
    spi_.begin(cfg::ESP_PINS::SCLK_PIN, -1, cfg::ESP_PINS::MOSI_PIN, -1);
#else
    spi_.begin();
#endif
    spiStarted_ = true;
  }

  ad9833_.begin();
  ad9833_.setMode(MD_AD9833::MODE_OFF);
  started_ = true;
}

void AD9833Driver::setToneFrequency(float frequencyHz) {
  if (!started_) {
    begin();
  }

  if (frequencyHz <= 0.0f) {
    ad9833_.setMode(MD_AD9833::MODE_OFF);
    return;
  }

  ad9833_.setFrequency(MD_AD9833::CHAN_0, frequencyHz);
  ad9833_.setMode(MD_AD9833::MODE_SINE);
}

void AD9833Driver::stopOutput() {
  if (!started_) {
    begin();
  }
  ad9833_.setMode(MD_AD9833::MODE_OFF);
}


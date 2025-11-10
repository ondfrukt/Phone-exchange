#pragma once

#include <Arduino.h>
#include <MD_AD9833.h>
#include <SPI.h>
#include <cstddef>
#include <cstdint>

#include "config.h"
#include "model/Types.h"

class ToneGenerator {
public:
  explicit ToneGenerator(uint8_t csPin, SPIClass& spi = SPI);
  void begin();
  void startToneSequence(model::ToneId sequence);
  void stop();
  void update();
  bool isPlaying();

private:
  struct Step {
    float     frequencyHz;
    uint32_t  durationMs;
  };

  struct StepSequence {
    const Step* steps;
    std::size_t length;
  };

  void ensureStarted_();
  void applyStep_(const Step& step);
  StepSequence getSequence_(model::ToneId sequence) const;

private:
  SPIClass&   spi_;
  MD_AD9833   ad9833_;
  uint8_t     csPin_;
  bool        started_ = false;
  bool        playing_ = false;
  StepSequence currentSequence_{nullptr, 0};
  std::size_t currentStepIndex_ = 0;
  uint32_t    stepStartTimeMs_ = 0;
};
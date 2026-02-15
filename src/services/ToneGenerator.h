#pragma once

#include <Arduino.h>
#include <cstddef>
#include <cstdint>

#include "config.h"
#include "drivers/AD9833Driver.h"
#include "model/Types.h"
#include "settings/Settings.h"

class ToneGenerator {
public:
  ToneGenerator(AD9833Driver& driver1, AD9833Driver& driver2, AD9833Driver& driver3);
  void begin();
  uint8_t startTone(model::ToneId sequence);
  void stopTone(uint8_t dac);
  void update();
  bool isPlaying() const;

private:
  struct Step {
    float     frequencyHz;
    uint32_t  durationMs;
  };

  struct StepSequence {
    const Step* steps;
    std::size_t length;
  };

  struct ChannelState {
    AD9833Driver* driver = nullptr;
    uint8_t dac = 0;
    bool playing = false;
    StepSequence currentSequence{nullptr, 0};
    std::size_t currentStepIndex = 0;
    uint32_t stepStartTimeMs = 0;
  };

  void applyStep_(ChannelState& channel, const Step& step);
  ChannelState* findChannelByDac_(uint8_t dac);
  ChannelState* findFreeChannel_();
  StepSequence getSequence_(model::ToneId sequence) const;
  void stopChannel_(ChannelState& channel);

private:
  ChannelState channels_[3];
};

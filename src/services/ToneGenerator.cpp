#include "ToneGenerator.h"
#include "util/UIConsole.h"

ToneGenerator::ToneGenerator(AD9833Driver& driver1, AD9833Driver& driver2, AD9833Driver& driver3) {
  channels_[0].driver = &driver1;
  channels_[0].dac = cfg::mt8816::DAC1;
  channels_[1].driver = &driver2;
  channels_[1].dac = cfg::mt8816::DAC2;
  channels_[2].driver = &driver3;
  channels_[2].dac = cfg::mt8816::DAC3;
}

void ToneGenerator::begin() {
  for (auto& channel : channels_) {
    if (channel.driver != nullptr) {
      channel.driver->begin();
    }
    stopChannel_(channel);
  }
}

uint8_t ToneGenerator::startTone(model::ToneId sequence) {
  if (!Settings::instance().toneGeneratorEnabled) return 0;

  ChannelState* channel = findFreeChannel_();
  if (channel == nullptr) {
    return 0;
  }

  channel->currentSequence = getSequence_(sequence);
  channel->currentStepIndex = 0;
  channel->stepStartTimeMs = millis();

  if (channel->currentSequence.steps == nullptr || channel->currentSequence.length == 0) {
    stopChannel_(*channel);
    return 0;
  }

  channel->playing = true;
  applyStep_(*channel, channel->currentSequence.steps[channel->currentStepIndex]);
  channel->stepStartTimeMs = millis();

  if (Settings::instance().debugTonGenLevel >= 1) {
    Serial.println("ToneGenerator: Started tone sequence " + String(ToneIdToString(sequence)) + " on DAC " + String(channel->dac));
    util::UIConsole::log("Started tone sequence " + String(ToneIdToString(sequence)) + " on DAC " + String(channel->dac), "ToneGenerator");
  }
  return channel->dac;
}

void ToneGenerator::stopTone(uint8_t dac) {
  ChannelState* channel = findChannelByDac_(dac);
  if (channel == nullptr) {
    return;
  }
  stopChannel_(*channel);

  if (Settings::instance().debugTonGenLevel >= 1) {
    Serial.println("ToneGenerator: Stopped tone sequence on DAC " + String(dac));
    util::UIConsole::log("Stopped tone sequence on DAC " + String(dac), "ToneGenerator");
  }
}

bool ToneGenerator::isPlaying() const {
  for (const auto& channel : channels_) {
    if (channel.playing) {
      return true;
    }
  }
  return false;
}

void ToneGenerator::update() {
  if (!Settings::instance().toneGeneratorEnabled) {
    for (auto& channel : channels_) {
      if (channel.playing) {
        stopChannel_(channel);
      }
    }
    return;
  }

  for (auto& channel : channels_) {
    if (!channel.playing || channel.currentSequence.steps == nullptr || channel.currentSequence.length == 0) {
      continue;
    }

    const Step& step = channel.currentSequence.steps[channel.currentStepIndex];
    if (step.durationMs == 0) {
      continue;
    }

    const uint32_t now = millis();
    const uint32_t elapsed = now - channel.stepStartTimeMs;
    if (elapsed < step.durationMs) {
      continue;
    }

    channel.currentStepIndex = (channel.currentStepIndex + 1) % channel.currentSequence.length;
    applyStep_(channel, channel.currentSequence.steps[channel.currentStepIndex]);
    channel.stepStartTimeMs = millis();
  }
}

void ToneGenerator::applyStep_(ChannelState& channel, const Step& step) {
  if (channel.driver == nullptr) {
    return;
  }
  channel.driver->setToneFrequency(step.frequencyHz);
}

ToneGenerator::StepSequence ToneGenerator::getSequence_(model::ToneId sequence) const {
  switch (sequence) {
    case model::ToneId::Ready: {
      static constexpr Step steps[] = {
        {425.0f, 0},
      };
      return {steps, sizeof(steps) / sizeof(steps[0])};
    }
    case model::ToneId::Ring: {
      static constexpr Step steps[] = {
        {425.0f, 1000},
        {0.0f, 5000},
      };
      return {steps, sizeof(steps) / sizeof(steps[0])};
    }
    case model::ToneId::Busy: {
      static constexpr Step steps[] = {
        {425.0f, 250},
        {0.0f, 250},
      };
      return {steps, sizeof(steps) / sizeof(steps[0])};
    }
    case model::ToneId::Fail: {
      static constexpr Step steps[] = {
        {950.0f, 330},
        {1400.0f, 330},
        {1800.0f, 330},
        {0.0f, 1000},
      };
      return {steps, sizeof(steps) / sizeof(steps[0])};
    }
    default:
      break;
  }
  return {nullptr, 0};
}

ToneGenerator::ChannelState* ToneGenerator::findChannelByDac_(uint8_t dac) {
  for (auto& channel : channels_) {
    if (channel.dac == dac) {
      return &channel;
    }
  }
  return nullptr;
}

ToneGenerator::ChannelState* ToneGenerator::findFreeChannel_() {
  for (auto& channel : channels_) {
    if (!channel.playing) {
      return &channel;
    }
  }
  return nullptr;
}

void ToneGenerator::stopChannel_(ChannelState& channel) {
  if (channel.driver != nullptr) {
    channel.driver->stopOutput();
  }
  channel.playing = false;
  channel.currentSequence = StepSequence{nullptr, 0};
  channel.currentStepIndex = 0;
  channel.stepStartTimeMs = 0;
}

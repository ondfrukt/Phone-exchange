#include "ToneGenerator.h"

ToneGenerator::ToneGenerator(uint8_t csPin, SPIClass& spi)
  : spi_(spi),
    ad9833_(csPin),
    csPin_(csPin) {
}

void ToneGenerator::begin() {
  if (started_) {
    return;
  }

  pinMode(csPin_, OUTPUT);
  digitalWrite(csPin_, HIGH);

#ifdef ARDUINO_ARCH_ESP32
  spi_.begin(cfg::ESP_PINS::SCLK_PIN, -1, cfg::ESP_PINS::MOSI_PIN, csPin_);
#else
  spi_.begin();
#endif

  ad9833_.begin();
  ad9833_.setMode(MD_AD9833::MODE_OFF);
  started_ = true;
  playing_ = false;
  currentSequence_ = StepSequence{nullptr, 0};
}

void ToneGenerator::startToneSequence(model::ToneId sequence) {

  if (!Settings::instance().toneGeneratorEnabled) return;

  ensureStarted_();

  currentSequence_ = getSequence_(sequence);
  currentStepIndex_ = 0;
  stepStartTimeMs_ = millis();

  if (currentSequence_.steps == nullptr || currentSequence_.length == 0) {
    stop();
    return;
  }

  playing_ = true;
  applyStep_(currentSequence_.steps[currentStepIndex_]);
  stepStartTimeMs_ = millis();

  if (Settings::instance().debugTonGenLevel >= 1) {
    Serial.println("ToneGenerator: Started tone sequence " + String(ToneIdToString(sequence)) + " on CS pin " + String(csPin_));
    util::UIConsole::log("Started tone sequence " + String(ToneIdToString(sequence)) + " on CS pin " + String(csPin_), "ToneGenerator");
  }

}

void ToneGenerator::stop() {
  ensureStarted_();

  playing_ = false;
  currentSequence_ = StepSequence{nullptr, 0};
  currentStepIndex_ = 0;
  stepStartTimeMs_ = 0;
  ad9833_.setMode(MD_AD9833::MODE_OFF);

  if (Settings::instance().debugTonGenLevel >= 1) {
    Serial.println("ToneGenerator: Stopped tone sequence on CS pin " + String(csPin_));
    util::UIConsole::log("Stopped tone sequence on CS pin " + String(csPin_), "ToneGenerator");
  }
}

bool ToneGenerator::isPlaying() {
  return playing_;
}

void ToneGenerator::update() {
  if (!playing_ || currentSequence_.steps == nullptr || currentSequence_.length == 0) {
    return;
  }
  const Step& step = currentSequence_.steps[currentStepIndex_];
  if (step.durationMs == 0) {
    return;
  }
  uint32_t now = millis();
  uint32_t elapsed = now - stepStartTimeMs_;
  if (elapsed < step.durationMs) {
    return;
  }
  currentStepIndex_ = (currentStepIndex_ + 1) % currentSequence_.length;
  applyStep_(currentSequence_.steps[currentStepIndex_]);
  stepStartTimeMs_ = millis();
}

void ToneGenerator::ensureStarted_() {
  if (!started_) {
    begin();
  }
}

void ToneGenerator::applyStep_(const Step& step) {
  ensureStarted_();

  if (step.frequencyHz <= 0.0f) {
    ad9833_.setMode(MD_AD9833::MODE_OFF);
    return;
  }

  ad9833_.setFrequency(MD_AD9833::CHAN_0, step.frequencyHz);
  ad9833_.setMode(MD_AD9833::MODE_SINE);
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

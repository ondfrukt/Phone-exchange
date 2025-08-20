#pragma once
#include <Arduino.h>

// Minimal fler-instans ISR-bridge för ESP32.
// - Du har extern pull-up (10k till 3V3) => pinMode(INPUT).
// - ISR sätter endast flag_ = true.
// - clearFlag() låter din app nollställa flaggan efter hantering.

class MCPInterruptService {
public:
  MCPInterruptService(int gpio_pin, int mode = FALLING)
  : pin_(gpio_pin), mode_(mode) {}

  bool begin() {
    if (pin_ < 0) return false;
    pinMode(pin_, INPUT); // extern pull-up finns
    attachInterruptArg(pin_, &MCPInterruptService::isrThunk, this, mode_);
    return true;
  }

  inline void clearFlag() { flag_ = false; }

  volatile bool flag_ = false;   // sätts TRUE av ISR

private:
  static void IRAM_ATTR isrThunk(void* arg) {
    auto* self = static_cast<MCPInterruptService*>(arg);
    self->flag_ = true;  // MINSTA möjliga i ISR
  }

  const int pin_;
  const int mode_;
};

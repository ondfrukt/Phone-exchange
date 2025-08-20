#pragma once
#include <Arduino.h>

class MCPInterruptService {
public:
  MCPInterruptService(int gpio_pin, int mode = FALLING)
  : pin_(gpio_pin), mode_(mode) {}

  bool begin() {
    if (pin_ < 0) return false;
    pinMode(pin_, INPUT); // du har alltid extern pull-up
    attachInterruptArg(pin_, &MCPInterruptService::isrThunk, this, mode_);
    return true;
  }

  void clearFlag() { flag_ = false; }

  volatile bool flag_ = false;

private:
  static void IRAM_ATTR isrThunk(void* arg) {
    auto* self = static_cast<MCPInterruptService*>(arg);
    self->flag_ = true;
  }

  const int pin_;
  const int mode_;
};

#pragma once
#include <Arduino.h>

// Minimal interrupt-service för ESP32.
// - Stödjer flera instanser via attachInterruptArg().
// - Använder alltid INPUT (du har extern pull-up).
// - ISR sätter endast flag_ = true.

class MCPInterruptService {
public:
  MCPInterruptService(int gpio_pin, int mode = FALLING)
  : pin_(gpio_pin), mode_(mode) {}

  bool begin();              // kopplar in interrupt
  void clearFlag();          // nollställer flaggan

  volatile bool flag_ = false;   // sätts TRUE av ISR

private:
  static void IRAM_ATTR isrThunk(void* arg); // ISR-trampolin

  const int pin_;
  const int mode_;
};

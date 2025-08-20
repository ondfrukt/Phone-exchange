#include "MCPInterruptService.h"

bool MCPInterruptService::begin() {
  if (pin_ < 0) return false;

  pinMode(pin_, INPUT); // extern pull-up finns alltid

  // Koppla ISR och skicka med 'this' så varje instans har sin egen flagga
  attachInterruptArg(pin_, &MCPInterruptService::isrThunk, this, mode_);
  return true;
}

void MCPInterruptService::clearFlag() {
  flag_ = false;
}

void IRAM_ATTR MCPInterruptService::isrThunk(void* arg) {
  auto* self = static_cast<MCPInterruptService*>(arg);
  self->flag_ = true;  // MINSTA möjliga i ISR
}

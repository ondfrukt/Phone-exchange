#pragma once
#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include "config.h"

// En tjänst för att läsa MCP INT, kvittera, och leverera debouncade events.
// Du initierar din Adafruit_MCP23X17 själv (adress, pinModes, etc).
// Konstruktor tar en referens till den MCP som har INT-linan kopplad till ESP.
//
// Användning (exempel):
//   MCPInterruptService mcpInt(mcpMain, /*debounceMs*/25);
//   mcpInt.configureChipInterrupts(false, true, LOW); // valfritt
//   mcpInt.attachEspInterrupt(espIntGpio, FALLING);
//   mcpInt.attachInput(0, true, CHANGE, false);
//   mcpInt.setCallback(onStable);
//   ... i loop(): mcpInt.process();
//
class MCPInterruptService {
public:
  using Callback = void(*)(uint8_t pin, uint8_t level);

  explicit MCPInterruptService(Adafruit_MCP23X17& mcp, uint32_t debounceMs = 25);

  // Ställ in debounce-tid (ms)
  void setDebounce(uint32_t ms);

  // Ställ in global INT-konfig för chipet (om du inte redan gjort det själv)
  void configureChipInterrupts(bool mirror, bool openDrain, uint8_t polarity);

  // Koppla ESP-GPIO till INT och registrera ISR (använder intern thunk)
  void attachEspInterrupt(uint8_t espGpio, int edgeMode = FALLING);

  // Bevaka specifik MCP-pin (sätter pinMode + interrupt-läge). 
  // notifyInitial=true -> callback triggas med aktuell nivå direkt vid attach.
  void attachInput(uint8_t mcpPin, bool pullup = true, uint8_t intMode = CHANGE, bool notifyInitial = false);

  // Sätt callback som får stabila (debouncade) events
  void setCallback(Callback cb);

  // Kalla denna i loop(): hanterar flagga, kvittering, debounce och callbacks
  void process();

  // Valfritt: hämta senaste stabila nivå
  uint8_t stableLevel(uint8_t pin) const;

private:
  struct Pending {
    bool active = false;
    uint8_t val = 0;
    uint32_t tExpire = 0;
  };

  static MCPInterruptService* s_instance; // för ISR-thunk (en instans)
  static void IRAM_ATTR isrThunk(void* arg);
  void IRAM_ATTR onIsr();                 // sätter endast _intFlag

  static inline bool timeReached(uint32_t now, uint32_t t) {
    return (int32_t)(now - t) >= 0;
  }

  void handleCandidateAndAck();           // läs orsak + kvittera GPIOA/B
  void schedulePending(uint8_t pin, uint8_t value, uint32_t delayMs);

  Adafruit_MCP23X17& _mcp;
  uint32_t _debounceMs;
  uint8_t  _stable[16];
  Pending  _pending[16];
  uint8_t  _pendingCount;
  volatile bool _intFlag;
  uint8_t  _espIntGpio;
  Callback _cb;
};

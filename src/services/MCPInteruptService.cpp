#include "MCPInterruptService.h"

MCPInterruptService::MCPInterruptService(Adafruit_MCP23X17& mcp, uint32_t debounceMs)
: _mcp(mcp), _debounceMs(debounceMs), _pendingCount(0), _intFlag(false), _espIntGpio(0xFF), _cb(nullptr) {
  for (uint8_t i = 0; i < 16; ++i) {
    _stable[i]  = 1;           // rimlig default vid pullup
    _pending[i] = Pending{};
  }
}

void MCPInterruptService::setDebounce(uint32_t ms) { _debounceMs = ms; }

void MCPInterruptService::configureChipInterrupts(bool mirror, bool openDrain, uint8_t polarity) {
  _mcp.setupInterrupts(mirror, openDrain, polarity);
}

void MCPInterruptService::attachEspInterrupt(uint8_t espGpio, int edgeMode) {
  _espIntGpio = espGpio;
  pinMode(_espIntGpio, INPUT_PULLUP);
  attachInterruptArg(_espIntGpio, &MCPInterruptService::isrThunk, this, edgeMode);
}

void MCPInterruptService::attachInput(uint8_t mcpPin, bool pullup, uint8_t intMode, bool notifyInitial) {
  _mcp.pinMode(mcpPin, pullup ? INPUT_PULLUP : INPUT);
  _mcp.setupInterruptPin(mcpPin, intMode);
  _stable[mcpPin] = _mcp.digitalRead(mcpPin);
  if (notifyInitial && _cb) _cb(mcpPin, _stable[mcpPin]);
}

void MCPInterruptService::setCallback(Callback cb) { _cb = cb; }

void IRAM_ATTR MCPInterruptService::isrThunk(void* arg) {
  static_cast<MCPInterruptService*>(arg)->onIsr();
}

void IRAM_ATTR MCPInterruptService::onIsr() {
  _intFlag = true;  // minimalt i ISR
}

uint8_t MCPInterruptService::stableLevel(uint8_t pin) const {
  return _stable[pin & 0x0F];
}

void MCPInterruptService::schedulePending(uint8_t pin, uint8_t value, uint32_t delayMs) {
  if (pin >= 16) return;
  uint32_t t = millis() + delayMs;
  if (!_pending[pin].active) {
    _pending[pin].active = true;
    _pendingCount++;
  }
  _pending[pin].val     = value;
  _pending[pin].tExpire = t;
}

void MCPInterruptService::handleCandidateAndAck() {
  // Hämta orsak: vilken pin och vilket värde triggade
  int8_t pin = _mcp.getLastInterruptPin();
  int8_t val = _mcp.getLastInterruptPinValue();

  // KVITTERA: läs GPIOA/B så INT-linan släpps
  (void)_mcp.readGPIO(0);
  (void)_mcp.readGPIO(1);

  if (pin >= 0 && pin < 16) {
    schedulePending((uint8_t)pin, (uint8_t)val, _debounceMs);
  }
}

void MCPInterruptService::process() {
  // 1) INT flaggad? hämta kandidat & kvittera
  if (_intFlag) {
    _intFlag = false;
    handleCandidateAndAck();
  }

  // 2) Något pending?
  if (_pendingCount == 0) return;

  // 3) Verifiera de som passerat sitt fönster
  uint32_t now = millis();
  for (uint8_t p = 0; p < 16; ++p) {
    if (_pending[p].active && timeReached(now, _pending[p].tExpire)) {
      // Läs aktuell nivå igen (I2C-läsning)
      uint8_t current = _mcp.digitalRead(p);
      if (current == _pending[p].val) {
        // Callback endast vid verklig förändring
        if (_stable[p] != current) {
          _stable[p] = current;
          if (_cb) _cb(p, current);
        }
      }
      _pending[p].active = false;
      _pendingCount--;
    }
  }
}

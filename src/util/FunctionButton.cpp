#include "util/FunctionButton.h"


void FunctionButton::update() {
    // Static variables for debouncing and tracking button state:
    // btnDown: tracks if the button is currently pressed
    // pressedAtMs: timestamp when the button was pressed
    // lastEvtMs: timestamp of the last processed event to prevent rapid repeats
    static bool     btnDown      = false;
    static uint32_t pressedAtMs  = 0;
    static uint32_t lastEvtMs    = 0;

  auto r = interruptManager_.pollEvent(cfg::mcp::MCP_MAIN_ADDRESS, cfg::mcp::FUNCTION_BUTTON);
  if (r.hasEvent) {
    uint32_t now = millis();
    if (now - lastEvtMs < 20) return;
    lastEvtMs = now;

    if (r.level == 0) {
        btnDown     = true;
        pressedAtMs = now;
    } else {
      testRing();
    }
  }
}

void FunctionButton::restartDevice(uint32_t held) {
    if (held >= 5000) {
        Serial.println("Long press (>5s): running special method");
        // myLongPressAction();
    } else {
        Serial.println("Short press: restart om 2s...");
        delay(2000);
        ESP.restart();
    }

}

void FunctionButton::testRing() {
    Serial.println("FunctionButton: testRing()");
    
    int RM = cfg::mcp::RM_PINS[0];
    int FR = cfg::mcp::FR_PINS[0];
    uint8_t addr = cfg::mcp::MCP_SLIC1_ADDRESS;
  
    mcp_ks083f.digitalWriteMCP(addr, RM, HIGH);        // enable ring mode on line 0
    for (int i = 0; i < 4; i++) {
      mcp_ks083f.digitalWriteMCP(addr, FR, HIGH);      // toggle fwd/rev pin to generate ring
      delay(25);  // 25 mS = half of 20 Hz period
      mcp_ks083f.digitalWriteMCP(addr, FR, LOW);
      delay(25);  // 25 mS = half of 20 Hz period
    }
    mcp_ks083f.digitalWriteMCP(addr, RM, LOW);         // disable ring mode
};
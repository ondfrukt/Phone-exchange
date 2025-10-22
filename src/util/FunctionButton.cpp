#include "util/TestButton.h"


void TestButton::update() {
    // Static variables for debouncing and tracking button state:
    // btnDown: tracks if the button is currently pressed
    // pressedAtMs: timestamp when the button was pressed
    // lastEvtMs: timestamp of the last processed event to prevent rapid repeats
    static bool     btnDown      = false;
    static uint32_t pressedAtMs  = 0;
    static uint32_t lastEvtMs    = 0;

    auto r = mcpDriver_.handleMainInterrupt();
    if (r.hasEvent && r.pin == cfg::mcp::TEST_BUTTON) {
        uint32_t now = millis();
        if (now - lastEvtMs < 20) return;
        lastEvtMs = now;

        if (r.level == 0) {
            btnDown     = true;
            pressedAtMs = now;
        } else {
            if (btnDown) {
                btnDown = false;
                uint32_t held = now - pressedAtMs;

                if (held >= 5000) {
                    Serial.println("Long press (>5s): running special method");
                    
                    // myLongPressAction();
                } else {
                    Serial.println("Short press: restart om 2s...");
                    delay(2000);
                    ESP.restart();
                }
            }
        }
    }
}
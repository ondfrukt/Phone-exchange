#include "util/FlashLED.h"



void FlashLED::setStatusLED(bool state) {
    if (state) {
        digitalWrite(cfg::ESP_LED_PINS::WiFiLED_PIN, HIGH);
    } else {
        digitalWrite(cfg::ESP_LED_PINS::WiFiLED_PIN, LOW);
    }
}

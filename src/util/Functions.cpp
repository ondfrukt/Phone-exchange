#include "util/Functions.h"
#include "net/WifiClient.h"
#include "net/MqttClient.h"
#include "net/Provisioning.h"

void Functions::begin() {
  initStatusLeds_();
}

void Functions::update() {
    updateStatusLeds_();

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
      // Register button press edge.
      btnDown = true;
      pressedAtMs = now;
      return;
    }

    // Ignore stray release events if we never saw a valid press edge.
    if (!btnDown) {
      return;
    }

    btnDown = false;
    const uint32_t held = now - pressedAtMs;

    // Filter bounce/noise: release shorter than 50 ms is ignored.
    if (held < 50) {
      return;
    }

    restartDevice(held);
  }
}

void Functions::initStatusLeds_() {
    pinMode(cfg::ESP_PINS::Status_LED_PIN, OUTPUT);
    pinMode(cfg::ESP_PINS::WiFi_LED_PIN, OUTPUT);
    pinMode(cfg::ESP_PINS::MQTT_LED_PIN, OUTPUT);

    digitalWrite(cfg::ESP_PINS::Status_LED_PIN, LOW);
    digitalWrite(cfg::ESP_PINS::WiFi_LED_PIN, LOW);
    digitalWrite(cfg::ESP_PINS::MQTT_LED_PIN, LOW);
}

void Functions::updateStatusLeds_() {
    const bool wifiConnected = wifiClient_.isConnected();
    const bool mqttConnected = mqttClient_.isConnected();
    const bool mqttEnabled = Settings::instance().mqttEnabled;
    const bool systemOk = wifiConnected && (!mqttEnabled || mqttConnected);

    digitalWrite(cfg::ESP_PINS::WiFi_LED_PIN, wifiConnected ? HIGH : LOW);
    digitalWrite(cfg::ESP_PINS::MQTT_LED_PIN, mqttConnected ? HIGH : LOW);
    digitalWrite(cfg::ESP_PINS::Status_LED_PIN, systemOk ? HIGH : LOW);
}

void Functions::restartDevice(uint32_t held) {
    if (held >= 5000) {
        Serial.println("Resetting settings and WiFi credentials...");
        util::UIConsole::log("Resetting settings and WiFi credentials...", "Functions");
        
        // Clear all settings stored in Preferences under "myapp" namespace
        Preferences prefs;
        prefs.begin("myapp", false);  // Settings namespace
        prefs.clear();
        prefs.end();
        delay(1000);

        // Use centralized Wi-Fi/provisioning reset path to clear all credential sources.
        Serial.println("Settings reset complete. Clearing WiFi + provisioning state...");
        util::UIConsole::log("Settings reset complete. Clearing WiFi + provisioning state...", "Functions");
        delay(200);
        net::Provisioning::factoryReset();
    } else {
        Serial.println("Restarting...");
        delay(1000);
        ESP.restart();
    }

}

void Functions::testRing() {
    Serial.println("Functions:          testRing()");
    util::UIConsole::log("testRing()", "Functions");
    
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

#pragma once
#include <Arduino.h>
#include "config.h"
#include <Wire.h>
#include "drivers/MCPDriver.h"
#include "drivers/InterruptManager.h"
#include "drivers/MT8816Driver.h"
#include "drivers/AD9833Driver.h"

#include "services/LineHandler.h"
#include "services/LineManager.h"
#include "services/SHKService.h"
#include "services/LineAction.h"
#include "services/ToneGenerator.h"
#include "services/ToneReader.h"
#include "services/RingGenerator.h"

#include "settings/settings.h"

#include "net/WifiClient.h"
#include "net/Provisioning.h"
#include "net/WebServer.h"
#include "net/MqttClient.h"

#include "esp_system.h"

#include "util/Functions.h"
#include "util/I2CScanner.h"
#include "util/UIConsole.h"

class App {
public:
    App();
    void begin();
    void loop();
    void update();

    MCPDriver mcpDriver_;
    InterruptManager interruptManager_;
    MT8816Driver mt8816Driver_;

private:
    void GPIOTest(uint8_t addr, int pin);

    // ===== Core drivers (owned by App) =====
    // Concrete instances: App creates and owns lifetime.
    ConnectionHandler connectionHandler_;
    AD9833Driver ad9833Driver1_;
    AD9833Driver ad9833Driver2_;
    AD9833Driver ad9833Driver3_;

    // Service that orchestrates the three AD9833 drivers above.
    ToneGenerator toneGenerator_;

    // ===== Telephony services (owned by App, wired with references) =====
    // Each service stores references to shared dependencies passed in constructor.
    LineManager lineManager_;
    ToneReader toneReader_;
    RingGenerator ringGenerator_;
    SHKService SHKService_;

    // ===== Network/application services =====
    net::WifiClient wifiClient_;
    net::Provisioning provisioning_;
    net::MqttClient mqttClient_;
    LineAction lineAction_;
    WebServer webServer_;

    // ===== Utility services =====
    Functions functions_;
    I2CScanner i2cScanner{Wire, Serial};
    util::UIConsole uiConsole_;

};

#pragma once
#include <Arduino.h>
#include "config.h"
#include <Wire.h>
#include "drivers/MCPDriver.h"
#include "drivers/InterruptManager.h"
#include "drivers/MT8816Driver.h"

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

#include "esp_system.h"

#include "util/Functions.h"
#include "util/I2CScanner.h"
#include "util/UIConsole.h"

class App {
public:
    App();
    void begin();
    void loop();

    MCPDriver mcpDriver_;
    InterruptManager interruptManager_;
    MT8816Driver mt8816Driver_;

private:
    void GPIOTest(uint8_t addr, int pin);

    ToneGenerator toneGenerator1_;
    ToneGenerator toneGenerator2_;
    ToneGenerator toneGenerator3_;

    LineManager lineManager_;
    ToneReader toneReader_;
    RingGenerator ringGenerator_;
    SHKService SHKService_;
    LineAction lineAction_;

    net::WifiClient wifiClient_;
    net::Provisioning provisioning_;
    WebServer webServer_;

    Functions functions_;
    I2CScanner i2cScanner{Wire, Serial};
    util::UIConsole uiConsole_;
    
};

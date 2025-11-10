#pragma once
#include "config.h"
#include <Wire.h>
#include "drivers/MCPDriver.h"
#include "drivers/MCPDriver.h"
#include "drivers/MT8816Driver.h"

#include "services/LineHandler.h"
#include "services/LineManager.h"
#include "services/SHKService.h"
#include "services/LineAction.h"
#include "services/ToneGenerator.h"

#include "settings/settings.h"

#include "net/WifiClient.h"
#include "net/Provisioning.h"
#include "net/WebServer.h"

#include "esp_system.h"

#include "util/FunctionButton.h"
#include "util/I2CScanner.h"
#include "util/UIConsole.h"


class App {
public:
    App();
    void begin();
    void loop();

    MCPDriver mcpDriver_;
    MT8816Driver mt8816Driver_;

private:
    
    LineManager lineManager_;
    SHKService SHKService_;
    LineAction lineAction_;
    ToneGenerator toneGenerator1_;
    ToneGenerator toneGenerator2_;
    ToneGenerator toneGenerator3_;

    net::WifiClient wifiClient_;
    net::Provisioning provisioning_;
    WebServer webServer_;

    FunctionButton functionButton_;
    I2CScanner i2cScanner{Wire, Serial};
    util::UIConsole uiConsole_;
    
};
#pragma once
#include "config.h"
#include <Wire.h>
#include "drivers/MCPDriver.h"
#include "services/LineHandler.h"
#include "services/LineManager.h"
#include "services/SHKService.h"
#include "services/LineAction.h"
#include "drivers/MCPDriver.h"
#include "settings/settings.h"
#include "net/WifiClient.h"
#include "net/Provisioning.h"
#include "net/WebServer.h"

// #include "drivers/ToneOut.h"
// #include "net/WebServer.h"
// #include "net/WifiClient.h"

class App {
public:
    App();
    void begin();
    void loop();

private:
    MCPDriver mcpDriver_;
    LineManager lineManager_;
    SHKService SHKService_;
    LineAction lineAction_;
    net::WifiClient wifiClient_;
    net::Provisioning provisioning_;
    WebServer webServer_;
};
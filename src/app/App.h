#pragma once
#include "config.h"
#include <Wire.h>
// #include "services/LineManager.h"
#include "MCPDriver.h"
// #include "drivers/ToneOut.h"
// #include "net/WebServer.h"
// #include "net/WifiClient.h"

class App {
public:
    void begin();
    void loop();

private:
    MCPDriver mcpDriver_;
    
// Här skapar jag olika objekt av mina klasser som jag behöver i appen
// LineManager lineManager_;
// MT8816Driver mt8816Driver_;
// WifiClient wifiClient_;
// WebServer  webServer_;
// Osv.....
};
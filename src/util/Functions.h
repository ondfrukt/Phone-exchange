#pragma once
#include <Preferences.h>
#include "drivers/InterruptManager.h"
#include "net/WifiClient.h"
#include "drivers/MCPDriver.h"
#include "settings/settings.h"
#include "config.h"

class Functions {

public:
    Functions(InterruptManager& interruptManager, MCPDriver& mcp_ks083f) : interruptManager_(interruptManager), mcp_ks083f(mcp_ks083f){};
    void update();
private:
    InterruptManager& interruptManager_;
    MCPDriver& mcp_ks083f;
    void testRing();
    void restartDevice(uint32_t held);

};
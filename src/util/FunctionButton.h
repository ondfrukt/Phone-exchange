#pragma once
#include "drivers/InterruptManager.h"
#include "drivers/MCPDriver.h"
#include "config.h"

class FunctionButton {

public:
    FunctionButton(InterruptManager& interruptManager, MCPDriver& mcp_ks083f) : interruptManager_(interruptManager), mcp_ks083f(mcp_ks083f) {};
    void update();
private:
    InterruptManager& interruptManager_;
    MCPDriver& mcp_ks083f;
    void testRing();
    void restartDevice(uint32_t held);

};
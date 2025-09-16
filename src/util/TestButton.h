#pragma once
#include "drivers/MCPDriver.h"
#include "config.h"

class TestButton {

public:
    TestButton(MCPDriver& mcpDriver) : mcpDriver_(mcpDriver) {};
    void update();
private:
    MCPDriver& mcpDriver_;
};
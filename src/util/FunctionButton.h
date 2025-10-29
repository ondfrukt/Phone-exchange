#pragma once
#include "drivers/MCPDriver.h"
#include "config.h"

class FunctionButton {

public:
    FunctionButton(MCPDriver& mcpDriver) : mcpDriver_(mcpDriver) {};
    void update();
private:
    MCPDriver& mcpDriver_;
};
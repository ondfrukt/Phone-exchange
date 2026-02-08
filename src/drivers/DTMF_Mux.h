#pragma once
#include <Arduino.h>
#include "config.h"
#include "settings/settings.h"
#include "drivers/MCPDriver.h"
#include "util/UIConsole.h"



class DTMF_Mux {
  public:
    DTMF_Mux(MCPDriver& mcpDriver, Settings& settings);
    void begin();
    private:

    MCPDriver& mcpDriver_;
    Settings& settings_;
};


#pragma once
#include <Arduino.h>
#include "config.h"
#include "settings/settings.h"
#include "MCPDriver.h"


class MT8816Driver {
  public:
    MT8816Driver(MCPDriver& mcpDriver, Settings& settings);
    void begin();

    void SetAudioConnection(uint8_t line, uint8_t audio, bool state);
    void SetLineConnection(uint8_t lineA, uint8_t lineB, bool state);

    void SetConnection(uint8_t x, uint8_t y, bool state);
    void setAddress(uint8_t x, uint8_t y);

    bool getConnection(int x, int y);
    void printConnections();
    bool connections[16][8];

  private:

    void strobe();
    void reset();

    MCPDriver& mcpDriver_;
    Settings& settings_;
};
#pragma once
#include <Arduino.h>
#include "config.h"
#include "drivers/MCPDriver.h"
#include "services/LineManager.h"
#include "settings.h"

class ToneReader {
  public:
    ToneReader(MCPDriver& mcpDriver, Settings& settings, LineManager& lineManager);
    void update();

  private:
    MCPDriver& mcpDriver_;
    Settings& settings_;
    LineManager& lineManager_;

    // Debouncing state
    static constexpr uint8_t INVALID_DTMF_NIBBLE = 0xFF;
    static constexpr unsigned long DTMF_DEBOUNCE_MS = 150; // Minimum time between DTMF detections
    
    unsigned long lastDtmfTime_ = 0;
    uint8_t lastDtmfNibble_ = INVALID_DTMF_NIBBLE;
    bool lastStdLevel_ = false;

    // MT8870 utgångar: Q1 är LSB, Q4 är MSB
    bool readDtmfNibble(uint8_t& nibble);
    char decodeDtmf(uint8_t nibble);

    static constexpr char dtmf_map_[16] = {
      'D', // 0x0
      '1', // 0x1
      '2', // 0x2
      '3', // 0x3
      '4', // 0x4
      '5', // 0x5
      '6', // 0x6
      '7', // 0x7
      '8', // 0x8
      '9', // 0x9
      '0', // 0xA
      '*', // 0xB
      '#', // 0xC
      'A', // 0xD
      'B', // 0xE
      'C'  // 0xF
    };
};
#pragma once
#include <Arduino.h>
#include "config.h"
#include "drivers/InterruptManager.h"
#include "drivers/MCPDriver.h"
#include "settings.h"

class LineManager;

class ToneReader {
  public:
    ToneReader(InterruptManager& interruptManager, MCPDriver& mcpDriver, Settings& settings, LineManager& lineManager);
    void update();
    void activate();
    void deactivate();
    bool isActive;

  private:
    InterruptManager& interruptManager_;
    MCPDriver& mcpDriver_;
    Settings& settings_;
    LineManager& lineManager_;

    // Debouncing state
    static constexpr uint8_t INVALID_DTMF_NIBBLE = 0xFF;
    
    unsigned long lastDtmfTime_ = 0;
    uint8_t lastDtmfNibble_ = INVALID_DTMF_NIBBLE;
    bool lastStdLevel_ = false;
    
    // STD signal stability tracking
    unsigned long stdRisingEdgeTime_ = 0;  // When STD went high
    bool stdRisingEdgePending_ = false;    // Whether we're waiting to process a rising edge

    // MT8870 utgångar: Q1 är LSB, Q4 är MSB
    bool readDtmfNibble(uint8_t& nibble);
    char decodeDtmf(uint8_t nibble);

    static constexpr char dtmf_map_[16] = {
      // A, B, C, D are not used here
      '\0', // 0x0 ('D')
      '1',  '2',  '3',
      '4',  '5',  '6',
      '7',  '8',  '9',
      '0',  '*',  '#',
      '\0', // 0xD (A)
      '\0', // 0xE (B)
      '\0'  // 0xF (C)
    };
};
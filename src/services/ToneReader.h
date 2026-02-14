#pragma once
#include <Arduino.h>
#include "config.h"
#include "drivers/InterruptManager.h"
#include "drivers/MCPDriver.h"
#include "settings/Settings.h"

class LineManager;

class ToneReader {
  public:
    ToneReader(InterruptManager& interruptManager, MCPDriver& mcpDriver, Settings& settings, LineManager& lineManager);
    void update();
    void activate();
    void deactivate();
    void toneScan();
    bool isActive = false;

  private:
    // Ignore/avoid locking STD immediately after TMUX switch to reduce wrong-line locks.
    static constexpr unsigned long TMUX_POST_SWITCH_GUARD_MS = 8;

    InterruptManager& interruptManager_;
    MCPDriver& mcpDriver_;
    Settings& settings_;
    LineManager& lineManager_;

    // Debouncing state
    static constexpr uint8_t INVALID_DTMF_NIBBLE = 0xFF;
    
    unsigned long lastDtmfTimeByLine_[8] = {0};
    uint8_t lastDtmfNibbleByLine_[8] = {
      INVALID_DTMF_NIBBLE, INVALID_DTMF_NIBBLE, INVALID_DTMF_NIBBLE, INVALID_DTMF_NIBBLE,
      INVALID_DTMF_NIBBLE, INVALID_DTMF_NIBBLE, INVALID_DTMF_NIBBLE, INVALID_DTMF_NIBBLE
    };
    bool lastStdLevel_ = false;
    int scanCursor_ = -1;
    int currentScanLine_ = -1;
    int stdLineIndex_ = -1;
    unsigned long lastTmuxSwitchAtMs_ = 0;
    bool scanPauseLogged_ = false;
    uint8_t lastLoggedScanMask_ = 0xFF;
    
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

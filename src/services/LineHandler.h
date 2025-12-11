#pragma once
#include <Arduino.h>
#include <string.h>
#include "model/Types.h"
using namespace model;

// Maximum number of lines

class LineHandler {
public:

    // Line variables
    int lineNumber;                     // Identifier for the line (0-7)
    bool lineActive;                    // Is the line active or not
    String phoneNumber;                 // Phone number for the line    
    LineStatus currentLineStatus;       // Current status of the line
    LineStatus previousLineStatus;      // Previous status for the line

    int incomingFrom;                   // Incoming call from
    int outgoingTo;                     // Outgoing call to

    uint8_t toneGenUsed;                // Current tone generator used for the line

    // SHK variables
    HookStatus currentHookStatus;       // Status of the hook (hook on/off)
    HookStatus previousHookStatus;      // Previous status of the hook
    bool SHK;                           // Current state of the SHK pin (0 = hook on, 1 = hook off)

    // Pulsing variables
    unsigned gap;                       // Time from last edge
    unsigned long edge;                 // Timestamp for the last edge
    String dialedDigits;                // char to store the dialed digits

    // Timer variables
    unsigned long lineTimerEnd;          // End time for the line timer

    // Ring timing variables (used by RingGenerator)
    uint32_t ringCurrentIteration;       // Current ring iteration count
    unsigned long ringStateStartTime;    // When the current ring state started
    unsigned long ringLastFRToggleTime;  // Last time FR pin was toggled
    bool ringFRPinState;                 // Current state of FR pin during ringing

    LineHandler(int line);
    void lineIdle();
    
private:
};
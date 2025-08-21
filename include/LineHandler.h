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
    String phoneNumber;
    LineStatus currentLineStatus;     // Current status of the line
    LineStatus previousLineStatus;    // Previous status for the line

    int incomingFrom;                   // Incoming call from
    int outgoingTo;                     // Outgoing call to

    // SHK variables
    HookStatus hookStatus;            // Status of the hook (hook on/off)
    bool SHK;                           // Current state of the SHK pin (0 = hook on, 1 = hook off)
    unsigned long lastDebounceTime;     // Last time the SHK pin changed state
    bool lastDebounceValue;             // Last value of the SHK pin

    // Pulsing variables
    unsigned gap;                       // Time from last edge
    unsigned long edge;                 // Timestamp for the last edge
    bool pulsing;                       // Flag to indicate the low part of the pulse
    int pulsCount;                      // Count number of pulses
    bool pulsingFlag;                   // Flag to indicate if pulsing are active
    String dialedDigits;                // char to store the dialed digits

    // Timer variables
    unsigned int lineTimerLimit;        // Current limit for the line timer
    unsigned long lineTimerStart;       // Start time for the line timer
    bool lineTimerActive;               // Flag to indicate if the line timer is active

    LineHandler(int line);

    void setLineStatus(LineStatus newStatus);

    void newDigitReceived(char digit);
    void resetDialedDigits();
    void lineIdle();

    void startLineTimer(unsigned int limit);
    void stopLineTimer();
    
private:
    bool firstDigitFlag = true;
};
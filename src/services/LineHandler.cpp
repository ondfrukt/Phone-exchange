#include "LineHandler.h"

// Constructor
LineHandler::LineHandler(int line) {

    lineNumber = line;
    phoneNumber = String(line);
    lineActive = false;   
    currentLineStatus = LineStatus::Idle;
    previousLineStatus = LineStatus::Idle;

    incomingFrom = -1;           
    outgoingTo = -1;                      
    
    currentHookStatus = HookStatus::On;
    previousHookStatus = HookStatus::On;
    SHK = 0;
    dialedDigits = "";
    
    lineTimerLimit = 0;
    lineTimerStart = 0;
    lineTimerActive = false;
}


// Start line timer
void LineHandler::startLineTimer(unsigned int limit) {
  lineTimerStart = millis();
  lineTimerLimit = limit;
  lineTimerActive = true;
}

// Stops and clears line timer
void LineHandler::stopLineTimer() {
  lineTimerActive = false;
  lineTimerStart = 0;
  lineTimerLimit = 0;
}

// Reset variables when idel is set as new status
void LineHandler::lineIdle() {
  dialedDigits = "";
  lineTimerLimit = 0;
  lineTimerStart = 0;
  lineTimerActive = false;
  incomingFrom = -1;
  outgoingTo = -1;
}
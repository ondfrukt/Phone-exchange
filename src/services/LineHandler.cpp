#include "LineHandler.h"

// Constructor
LineHandler::LineHandler(int line) {

    lineNumber = line;
    phoneNumber = "";
    lineActive = false;   
    currentLineStatus = LineStatus::Idle;
    previousLineStatus = LineStatus::Idle;

    incomingFrom = -1;           
    outgoingTo = -1;                      
    
    currentHookStatus = HookStatus::On;
    previousHookStatus = HookStatus::On;
    SHK = 0;
    dialedDigits = "";
    lineTimerEnd = -1;
    
    ringCurrentIteration = 0;
    ringStateStartTime = 0;
    ringLastFRToggleTime = 0;
    ringFRPinState = false;
}


// Reset variables when idel is set as new status
void LineHandler::lineIdle() {
  dialedDigits    = "";
  lineTimerEnd  = -1;
  incomingFrom    = -1;
  outgoingTo      = -1;
  ringCurrentIteration = 0;
  ringStateStartTime = 0;
  ringLastFRToggleTime = 0;
  ringFRPinState = false;
}
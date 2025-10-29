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
    lineTimerEnd = -1;
}


// Reset variables when idel is set as new status
void LineHandler::lineIdle() {
  dialedDigits    = "";
  lineTimerEnd  = -1;
  incomingFrom    = -1;
  outgoingTo      = -1;
}
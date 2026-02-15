#include "LineHandler.h"
#include "config.h"

// Constructor
LineHandler::LineHandler(int line) {

    lineNumber = line;

    // Set TMUX address based on line number
     if (line >= 0 && line < 8) {
      const uint8_t* addr = cfg::TMUX4051::TMUXAddresses[line];
      tmuxAddress[0] = addr[0];
      tmuxAddress[1] = addr[1];
      tmuxAddress[2] = addr[2];
    } else {
      tmuxAddress[0] = 0;
      tmuxAddress[1] = 0;
      tmuxAddress[2] = 0;
    }

    phoneNumber = "";
    lineName = "";
    lineActive = false;   
    currentLineStatus = LineStatus::Idle;
    previousLineStatus = LineStatus::Idle;

    incomingFrom = -1;           
    outgoingTo = -1;                      
    toneGenUsed = 0;
    
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

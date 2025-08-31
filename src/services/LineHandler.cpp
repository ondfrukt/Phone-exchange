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
    lastDebounceTime = millis();
    lastDebounceValue = true;
    gap = 0;
    pulsCount = 0;
    dialedDigits = "";
    edge = millis();
    
    lineTimerLimit = 0;
    lineTimerStart = 0;
    lineTimerActive = false;
}

// Change line status
void LineHandler::setLineStatus(LineStatus newStatus) {
  stopLineTimer();
  previousLineStatus = currentLineStatus;
  currentLineStatus = newStatus;
  if (newStatus == LineStatus::Idle) {
    lineIdle();
  }
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

// Process a new digit for a specific line
void LineHandler::newDigitReceived(char digit) {
  dialedDigits += digit;
  if (firstDigitFlag == true) {
    firstDigitFlag = false;
    Serial.print("Digits received: ");
  }
  Serial.print(String(digit));
  lineTimerStart = millis();
}

// Reset dialed digits
void LineHandler::resetDialedDigits() {
  dialedDigits = "";
  firstDigitFlag = true;
} 

// Reset variables when idel is set as new status
void LineHandler::lineIdle() {
  resetDialedDigits();
  pulsCount = 0;
  dialedDigits = "";
  edge = 0; 
  lineTimerLimit = 0;
  lineTimerStart = 0;
  lineTimerActive = false;

  incomingFrom = -1;
  outgoingTo = -1;
}
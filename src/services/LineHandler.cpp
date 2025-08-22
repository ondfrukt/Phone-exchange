#include "LineHandler.h"

// Constructor
LineHandler::LineHandler(int line) {

    lineNumber = line;                    // Identifier for the line (0-7)
    phoneNumber = String(line);           // Phone number for the line
    currentLineStatus = LineStatus::Idle; // Current status of the line
    previousLineStatus = LineStatus::Idle;// Previous status for the line

    incomingFrom = -1;             // Incoming call from
    outgoingTo = -1;               // Outgoing call to
    
    hookStatus = HookStatus::On;   // Status of the hook (hook on/off)
    SHK = 0;                       // Current state of the SHK pin (0 = hook on, 1 = hook off)
    lastDebounceTime = millis();   // Last time the SHK pin changed state
    lastDebounceValue = true;      // Last value of the SHK pin
    gap = 0;                       // Time from last edge
    pulsing = false;               // Flag to indicate if the line is currently pulsing
    pulsCount = 0;                 // Count number of pulses
    dialedDigits = "";             // String to store the dialed digits
    edge = millis();               // Timestamp for the last edge
    
    lineTimerLimit = 0;            // Current limit for the line timer
    lineTimerStart = 0;            // Start time for the line timer
    lineTimerActive = false;       // Flag to indicate if the line timer is active
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
  pulsing = false;
  pulsCount = 0;
  dialedDigits = "";
  edge = 0; 
  lineTimerLimit = 0;
  lineTimerStart = 0;
  lineTimerActive = false;

  incomingFrom = -1;
  outgoingTo = -1;
}
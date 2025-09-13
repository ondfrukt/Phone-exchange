#include "LineAction.h"


LineAction::LineAction(LineManager& lineManager, Settings& settings)
: lineManager_(lineManager), settings_(settings) {
  // ev. startlogik
};

void LineAction::update() {


  // Check if any line has changed status
  if(lineManager_.lineChangeFlag != 0){
    uint8_t changes = lineManager_.lineChangeFlag & settings_.activeLinesMask;
    for (int index = 0; index < 8; ++index)
        if (changes & (1 << index)) {
          lineManager_.clearChangeFlag(index); // Clear the change flag
          action(index);
      } 
  }
  // Check if any line timers have expired
  if(lineManager_.activeTimersMask != 0){
    unsigned long currentTime = millis();
    uint8_t timers = lineManager_.activeTimersMask & settings_.activeLinesMask;
    

    for (int index = 0; index < 8; ++index)
        if (timers & (1 << index)) {
          LineHandler& line = lineManager_.getLine(index);
          if (line.lineTimerEnd != -1 && currentTime >= line.lineTimerEnd) {
            // Timer has expired
            lineManager_.activeTimersMask &= ~(1 << index); // Clear the timer active flag
            timerExpired(line);
          }
      } 
  }
}

void LineAction::action(int index) {
  using namespace model;
  LineHandler& line = lineManager_.getLine(index);
  LineStatus newStatus = line.currentLineStatus;
  LineStatus previousStatus = line.previousLineStatus;

  switch (newStatus) {
    
    case LineStatus::Idle:
      // mqttHandler.publishMQTT(line, line_idle);
      // toneGen1.setMode(ToneGenerator::TONE_OFF);
      break;

    case LineStatus::Ready:
      // mqttHandler.publishMQTT(line, line_ready);
      // mt8816.connect(DTMF_line, line);
      // lastLineReady = line;
      // Line[line].startLineTimer(statusTimer_Ready);
      // toneGen1.setMode(ToneGenerator::TONE_READY);
      break;
    
    case LineStatus::PulseDialing:
    // Timers for pulse dialing is handled when a digit is received

    
    //   mqttHandler.publishMQTT(line, line_pulse_dialing);
    //   Line[line].startLineTimer(statusTimer_pulsDialing);
    //   toneGen1.setMode(ToneGenerator::TONE_OFF);
      break;
    
    case LineStatus::ToneDialing:
    // Timer for tone dialing is handled when a digit is received


    //   mqttHandler.publishMQTT(line, line_tone_dialing);
    //   mt8816.connect(DTMF_line, line);
    //   Line[line].startLineTimer(statusTimer_toneDialing);
    //   toneGen1.setMode(ToneGenerator::TONE_OFF);
      break;
    
    case LineStatus::Busy:
      lineManager_.setLineTimer(index, settings_.timer_busy); // Set timer for Busy state
    //   mqttHandler.publishMQTT(line, line_busy);
    //   Line[line].startLineTimer(statusTimer_busy);
    //   mt8816.connect(DTMF_line, line);
    //   toneGen1.setMode(ToneGenerator::TONE_BUSY);
      break;

    case LineStatus::Fail:
      lineManager_.setLineTimer(index, settings_.timer_fail); // Set timer for Fail state
    //   mqttHandler.publishMQTT(line, line_fail);
    //   Line[line].resetDialedDigits();
    //   Line[line].startLineTimer(statusTimer_fail);
    //   mt8816.connect(DTMF_line, line);
    //   toneGen1.setMode(ToneGenerator::TONE_FAIL);
      break;
    
    case LineStatus::Ringing:
      lineManager_.setLineTimer(index, settings_.timer_Ringing); // Set timer for Ringing state
    // Serial.println("Line " + String(line) + " dialed digits: " + Line[line].dialedDigits);
    // for (int i = 0; i < activeLines; i++) {

    //   // Check if the diled digits match a number in the phonebook ant its not the same line as ringing
    //   if (Line[line].dialedDigits == Line[i].phoneNumber && Line[i].lineNumber != line) {
        
    //     // Checking if the line is idle
    //     if (Line[i].currentLineStatus != line_idle){
    //       lineAction(line, line_busy);
    //       return;
    //     } 
    //     mqttHandler.publishMQTT(line, line_ringing);

    //     Line[line].outgoingTo = i;
        

    //     Line[i].setLineStatus(line_incoming);
    //     mqttHandler.publishMQTT(i, line_incoming);
    //     Line[i].incomingFrom = line;

    //     Line[line].resetDialedDigits();

    //     ringHandler.generateRingSignal(i);
    //     Line[line].startLineTimer(statusTimer_Ringing);
    //     return;
    //   }
    // }
    // // If no match is found in the loop, the line status is set to fail
    // Serial.println("Line " + String(line) + " failed to connect. Wrong number?");
    // Line[line].resetDialedDigits();
    // lineAction(line, line_fail);
      break;
    
    case LineStatus::Connected:
    //   mqttHandler.publishMQTT(line, line_connected);


    //   // Setting for the calling line
    //   Line[Line[line].incomingFrom].setLineStatus(line_connected);
    //   Line[Line[line].incomingFrom].outgoingTo = line;

    //   // Setting for the called line
    //   Line[line].outgoingTo = Line[line].incomingFrom;

    //   // Publinshing the status for the calling line
    //   mqttHandler.publishMQTT(Line[line].incomingFrom, line_connected);

    //   //Connectiong 
    //   mt8816.connect(line, Line[line].incomingFrom );
    //   mt8816.connect(Line[line].incomingFrom, line);
      break;

    case LineStatus::Disconnected:
      lineManager_.setLineTimer(index, settings_.timer_disconnected); // Set timer for Disconnected state
    //   mqttHandler.publishMQTT(line, line_disconnected);
    //   Line[line].startLineTimer(statusTimer_disconnected);
      
    //   // Oklart om detta fungerar?
    //   for (int i = 0; i < activeLines; i++){
    //     mt8816.disconnect(line, i);
    //     mt8816.disconnect(i, line);
    //   }
      break;
    
    case LineStatus::Timeout:
      lineManager_.setLineTimer(index, settings_.timer_timeout); // Set timer for Timeout state
    //   mqttHandler.publishMQTT(line, line_timeout);
    //   Line[line].startLineTimer(statusTimer_timeout);
    //   toneGen1.setMode(ToneGenerator::TONE_OFF);
    //   break;
    case LineStatus::Abandoned:
      lineManager_.setLineTimer(index, settings_.timer_timeout); // Set timer for Abandoned state
    //   Line[line].resetDialedDigits();
    // mqttHandler.publishMQTT(line, line_abandoned);
    // break;
    case LineStatus::Incoming:
    //   mqttHandler.publishMQTT(line, line_incoming);
    //   break;
    case LineStatus::Operator:
    //   mqttHandler.publishMQTT(line, line_operator);
    //   // Insert Action!
    //   break;
    default:
    //   // Handle unexpected status
      break;
  }
}

void LineAction::timerExpired(LineHandler& line) {
  using namespace model;
  int index = line.lineNumber;
  LineStatus currentStatus = line.currentLineStatus;

  if (settings_.debugLALevel >= 1) {
    Serial.println("LineAction: Timer expired for line " + String(index) + " in state " + model::toString(currentStatus));
  }

  switch (currentStatus) {
    case LineStatus::Ready:

      break;

    case LineStatus::PulseDialing:
    case LineStatus::ToneDialing:

      break;

    case LineStatus::Ringing:

      break;

    case LineStatus::Busy:

      break;

    case LineStatus::Fail:

      break;

    case LineStatus::Disconnected:

      break;

    case LineStatus::Timeout:

      break;

    default:

      break;
  }
}
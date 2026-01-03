#include "LineAction.h"


LineAction::LineAction(LineManager& lineManager, Settings& settings, MT8816Driver& mt8816Driver, RingGenerator& ringGenerator, ToneReader& toneReader,
            ToneGenerator& toneGen1, ToneGenerator& toneGen2, ToneGenerator& toneGen3)
          : lineManager_(lineManager), settings_(settings), mt8816Driver_(mt8816Driver), ringGenerator_(ringGenerator), toneReader_(toneReader),
            toneGen1_(toneGen1), toneGen2_(toneGen2), toneGen3_(toneGen3) {
};

void LineAction::begin() {
  toneGenerators[0] = &toneGen1_;
  toneGenerators[1] = &toneGen2_;
  toneGenerators[2] = &toneGen3_;
}

// Main update loop to check for line status changes and timer expirations
void LineAction::update() {

  // Check if any hook status has changed
  if(lineManager_.lineHookChangeFlag != 0){
    uint8_t hookChanges = lineManager_.lineHookChangeFlag & settings_.activeLinesMask;
    for (int index = 0; index < 8; ++index)
        if (hookChanges & (1 << index)) {
          lineManager_.lineHookChangeFlag &= ~(1 << index); // Clear the hook change flag
          LineHandler& line = lineManager_.getLine(index);

          // Update line status based on hook state

          // ON -> OFF & Line status = Idle
          if (line.previousHookStatus == model::HookStatus::On 
            && line.currentHookStatus == model::HookStatus::Off 
            && line.currentLineStatus == model::LineStatus::Idle) {

            lineManager_.setStatus(index, model::LineStatus::Ready);
          
          // ON -> OFF & Line status = Incoming
          } else if (line.previousHookStatus == model::HookStatus::On 
            && line.currentHookStatus == model::HookStatus::Off
            && line.currentLineStatus == model::LineStatus::Incoming) {
            
            int incomingFrom = lineManager_.connectionMatrix.getConnectedLine(index);

            // Establish connection
            lineManager_.connectionMatrix.setConnection(incomingFrom, index, ConnectionMatrix::State:: Established);
            lineManager_.setStatus(incomingFrom, LineStatus::Connected);
            lineManager_.setStatus(index, LineStatus::Connected);
            mt8816Driver_.SetAudioConnection(incomingFrom, index, true); // Connect audio between the two lines

          // OFF -> ON & Line status = Connected
          } else if (line.previousHookStatus == model::HookStatus::Off 
            && line.currentHookStatus == model::HookStatus::On 
            && line.currentLineStatus == model::LineStatus::Connected) {
            
            int incomingFrom = lineManager_.connectionMatrix.getConnectedLine(index);

            Serial.print("Hanging up line ");
            Serial.print(index);
            Serial.print(" connected to line ");
            Serial.println(incomingFrom);
            
            // Disconnect connection
            lineManager_.connectionMatrix.setConnection(incomingFrom, index, ConnectionMatrix::State:: None);
            lineManager_.setStatus(incomingFrom, LineStatus::Disconnected);
            lineManager_.setStatus(index, LineStatus::Idle);
            mt8816Driver_.SetAudioConnection(incomingFrom, index, false); // Connect audio between the two lines

          // OFF -> ON & Line status = Disconnected (other party already hung up)
          } else if (line.previousHookStatus == model::HookStatus::Off 
            && line.currentHookStatus == model::HookStatus::On 
            && line.currentLineStatus == model::LineStatus::Disconnected) {
            
            // User hung up while hearing disconnected tone - go directly to Idle
            Serial.print("Line ");
            Serial.print(index);
            Serial.println(" hung up while in Disconnected state - going to Idle");
            lineManager_.setStatus(index, LineStatus::Idle);

          // OFF -> ON & Line status = Ringing (caller hangs up before answer)
          } else if (line.previousHookStatus == model::HookStatus::Off 
            && line.currentHookStatus == model::HookStatus::On 
            && line.currentLineStatus == model::LineStatus::Ringing) {
            
            int calledLine = lineManager_.connectionMatrix.getConnectedLine(index);
            
            // Cancel the call
            lineManager_.connectionMatrix.setConnection(calledLine, index, ConnectionMatrix::State::None);
            lineManager_.setStatus(calledLine, LineStatus::Idle);
            lineManager_.setStatus(index, LineStatus::Idle);
            ringGenerator_.stopRingingLine(calledLine); // Stop ringing

          // OFF -> ON
          } else {
            lineManager_.setStatus(index, model::LineStatus::Idle);
          }

          // Update previous hook status after processing the change
          line.previousHookStatus = line.currentHookStatus;
      } 
  }


  // Check if any line has changed status
  if(lineManager_.lineStatusChangeFlag != 0){
    uint8_t changes = lineManager_.lineStatusChangeFlag & settings_.activeLinesMask;
    for (int index = 0; index < 8; ++index)
        if (changes & (1 << index)) {
          lineManager_.clearChangeFlag(index); // Clear the change flag

          // Handle the action for the line
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

// Handles actions based on new line status
void LineAction::action(int index) {
  using namespace model;
  LineHandler& line = lineManager_.getLine(index);
  LineStatus newStatus = line.currentLineStatus;
  LineStatus previousStatus = line.previousLineStatus;

  

  switch (newStatus) {
    
    case LineStatus::Idle:
      turnOffToneGenIfUsed(line);
      ringGenerator_.stopRingingLine(index); // Stop ringing if active

      //mt8816Driver_.SetAudioConnection(index, cfg::mt8816::DTMF, false); // Close listening port for DTMF
      //mt8816Driver_.SetAudioConnection(index, cfg::mt8816::DAC1, false); // Disconnect any audio connections
      //mt8816Driver_.SetAudioConnection(index, cfg::mt8816::DAC2, false); // Disconnect any audio connections
      //mt8816Driver_.SetAudioConnection(index, cfg::mt8816::DAC3, false); // Disconnect any audio connections
      // mqttHandler.publishMQTT(line, line_idle);
      break;

    case LineStatus::Ready:
      
      // mqttHandler.publishMQTT(line, line_ready);
      startToneGenForStatus(line, model::ToneId::Ready);
      //mt8816Driver_.SetAudioConnection(index, cfg::mt8816::DTMF, true); // Open listening port for DTMF
      // lastLineReady = line;
      break;
    
    case LineStatus::PulseDialing:
      turnOffToneGenIfUsed(line);
    //mt8816Driver_.SetAudioConnection(index, cfg::mt8816::DTMF, false); // Close listening port for DTMF
    // Timers for pulse dialing is handled when a digit is received
    //   mqttHandler.publishMQTT(line, line_pulse_dialing);8
    //   Line[line].startLineTimer(statusTimer_pulsDialing);
    //   toneGen1.setMode(ToneGenerator::TONE_OFF);
      break;
    
    case LineStatus::ToneDialing:
      turnOffToneGenIfUsed(line);
    // Timer for tone dialing is handled when a digit is received

    //   mqttHandler.publishMQTT(line, line_tone_dialing);
    //   mt8816.connect(DTMF_line, line);
    //   Line[line].startLineTimer(statusTimer_toneDialing);
    //   toneGen1.setMode(ToneGenerator::TONE_OFF);
      break;
    
    case LineStatus::Busy:
      turnOffToneGenIfUsed(line);
      startToneGenForStatus(line, model::ToneId::Busy);
      lineManager_.setLineTimer(index, settings_.timer_busy); // Set timer for Busy state
    //   mqttHandler.publishMQTT(line, line_busy);
    //   Line[line].startLineTimer(statusTimer_busy);
    //   mt8816.connect(DTMF_line, line);
    //   toneGen1.setMode(ToneGenerator::TONE_BUSY);
      break;

    case LineStatus::Fail:
      turnOffToneGenIfUsed(line);
      startToneGenForStatus(line, model::ToneId::Fail);
      lineManager_.setLineTimer(index, settings_.timer_fail); // Set timer for Fail state
    //   mqttHandler.publishMQTT(line, line_fail);
    //   Line[line].resetDialedDigits();
    //   Line[line].startLineTimer(statusTimer_fail);
    //   mt8816.connect(DTMF_line, line);
    //   toneGen1.setMode(ToneGenerator::TONE_FAIL);
      break;
    
    case LineStatus::Ringing:
      turnOffToneGenIfUsed(line);
      startToneGenForStatus(line, model::ToneId::Ring);
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
      turnOffToneGenIfUsed(line);
      ringGenerator_.stopRingingLine(index); // Stop ringing if active

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
      turnOffToneGenIfUsed(line);
      lineManager_.setLineTimer(index, settings_.timer_disconnected);

    //   mqttHandler.publishMQTT(line, line_disconnected);

      break;
    
    case LineStatus::Timeout:
      turnOffToneGenIfUsed(line);
      lineManager_.setLineTimer(index, settings_.timer_timeout); // Set timer for Timeout state
    //   mqttHandler.publishMQTT(line, line_timeout);
    //   Line[line].startLineTimer(statusTimer_timeout);
    //   toneGen1.setMode(ToneGenerator::TONE_OFF);
      break;
    
    case LineStatus::Abandoned:
      turnOffToneGenIfUsed(line);
      lineManager_.setLineTimer(index, settings_.timer_timeout); // Set timer for Abandoned state
    //   Line[line].resetDialedDigits();
    // mqttHandler.publishMQTT(line, line_abandoned);
      break;
    
    case LineStatus::Incoming:
      ringGenerator_.generateRingSignal(index);
      lineManager_.setLineTimer(index, settings_.timer_incomming); // Set timer for Incoming state

    //   mqttHandler.publishMQTT(line, line_incoming);
      break;
    
    case LineStatus::Operator:
    //   mqttHandler.publishMQTT(line, line_operator);
    //   // Insert Action!
    //   break;
    default:
    //   // Handle unexpected status
      break;
  }
}

// Handles a line when its timer has expired
void LineAction::timerExpired(LineHandler& line) {
  using namespace model;
  int index = line.lineNumber;
  LineStatus currentStatus = line.currentLineStatus;

  if (settings_.debugLALevel >= 1) {
    Serial.println("LineAction: Timer expired for line " + String(index) + " in state " + model::toString(currentStatus));
    util::UIConsole::log("LineAction: Timer expired for line " + String(index) + " in state " + model::toString(currentStatus), "LineAction");
  }

  switch (currentStatus) {
    case LineStatus::Ready:
      lineManager_.setStatus(index, LineStatus::Timeout);
      break;

    case LineStatus::ToneDialing:
    case LineStatus::PulseDialing: 
      {
          int lineCalled = lineManager_.searchPhoneNumber(line.dialedDigits);
          Serial.print("lineCalled received: " + String(lineCalled) + "\n");

      if (settings_.debugLALevel >= 1) {
        Serial.println("LineAction: ToneDialing line " + String(index) + " dialed digits: " + line.dialedDigits + ", found lineCalled: " + String(lineCalled));
        util::UIConsole::log("ToneDialing line " + String(index) + " dialed digits: " + line.dialedDigits + ", found lineCalled: " + String(lineCalled), "LineAction");
      }
      
      // Check for special case of dialing own number
      if (lineCalled == index){
        Serial.println(RED "LineAction: Line " + String(index) + " dialed its own number. Setting to Fail." + COLOR_RESET);
        lineManager_.setStatus(index, LineStatus::Fail);
      }
      // Check if a matching phone number was found
      else if (lineCalled != -1){
        
        // Check if the called line is idle
        if (lineManager_.getLine(lineCalled).currentLineStatus != LineStatus::Idle){
          Serial.println(RED "LineAction: Line " + String(lineCalled) + " is not idle. Setting line " + String(index) + " to Busy." + COLOR_RESET);
          lineManager_.setStatus(index, LineStatus::Busy);
          return;
        }

        // Set up the connection and change statuses
        lineManager_.connectionMatrix.setConnection(index, lineCalled, ConnectionMatrix::State:: Ringing);
        lineManager_.setStatus(index, LineStatus::Ringing);
        lineManager_.setStatus(lineCalled, LineStatus::Incoming);

      }
      // No matching phone number found
      else {
        Serial.println(RED "LineAction: No matching phone number found for line " + String(index) + " dialed digits: " + line.dialedDigits + COLOR_RESET);
        lineManager_.setStatus(index, LineStatus::Fail);
      }
          
      break;
    }

    case LineStatus::Ringing:
      lineManager_.setStatus(index, LineStatus::Disconnected);
      break;

    case LineStatus::Incoming:
      lineManager_.setStatus(index, LineStatus::Idle);
      break;

    case LineStatus::Busy:
      lineManager_.setStatus(index, LineStatus::Timeout);
      break;

    case LineStatus::Fail:
      lineManager_.setStatus(index, LineStatus::Timeout);
      break;

    case LineStatus::Disconnected:
      lineManager_.setStatus(index, LineStatus::Timeout);
      break;

    case LineStatus::Timeout:
      lineManager_.setStatus(index, LineStatus::Abandoned);

      break;

    default:

      break;
  }
}

// Start tone generator for specific line status
void LineAction::startToneGenForStatus(LineHandler& line, model::ToneId status) {
  if (!toneGen1_.isPlaying()){
    toneGen1_.startToneSequence(status);
    line.toneGenUsed = 1;
  }
  else if (!toneGen2_.isPlaying()){
    toneGen2_.startToneSequence(status);
    line.toneGenUsed = 2;
  }
  else if (!toneGen3_.isPlaying()){
    toneGen3_.startToneSequence(status);
    line.toneGenUsed = 3;
  }
}

// Turn off tone generator if it is being used by the line
void LineAction::turnOffToneGenIfUsed(LineHandler& line) {
  if (line.toneGenUsed != 0){
    toneGenerators[line.toneGenUsed -1]->stop();
    line.toneGenUsed = 0;
  }
}

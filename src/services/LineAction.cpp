#include "LineAction.h"


LineAction::LineAction(LineManager& lineManager, Settings& settings, MT8816Driver& mt8816Driver, RingGenerator& ringGenerator, ToneReader& toneReader,
            ToneGenerator& toneGen1, ToneGenerator& toneGen2, ToneGenerator& toneGen3, ConnectionHandler& connectionHandler)
          : lineManager_(lineManager), settings_(settings), mt8816Driver_(mt8816Driver), ringGenerator_(ringGenerator), toneReader_(toneReader),
            toneGen1_(toneGen1), toneGen2_(toneGen2), toneGen3_(toneGen3), connectionHandler_(connectionHandler) {
};

void LineAction::begin() {
  toneGenerators[0] = &toneGen1_;
  toneGenerators[1] = &toneGen2_;
  toneGenerators[2] = &toneGen3_;
}

// Main update loop to check for line status changes and timer expirations
void LineAction::update() {

  // First check for hook status changes and update line statuses accordingly
  hookStatusCangeCheck();
  // Then check for line status changes and handle actions
  statusChangeCheck();
  // Finally check for timer expirations and handle them
  timerExpiredCheck();
}

// Check for hook status changes and update line status accordingly
void LineAction::hookStatusCangeCheck() {

  if(lineManager_.lineHookChangeFlag != 0){
    uint8_t hookChanges = lineManager_.lineHookChangeFlag & settings_.activeLinesMask;

    // loop through lines with hook status changes and update line status accordingly
    for (int index = 0; index < 8; ++index)
      if (hookChanges & (1 << index)) {
        lineManager_.lineHookChangeFlag &= ~(1 << index); // Clear the hook change flag
        LineHandler& line = lineManager_.getLine(index);
        uint8_t incomingFrom = line.incomingFrom; // Store incomingFrom before any potential status changes

      // Update line status based on hook state

      // Hook ON -> Hook OFF & Line status = Idle
      if (line.previousHookStatus == model::HookStatus::On 
        && line.currentHookStatus == model::HookStatus::Off 
        && line.currentLineStatus == model::LineStatus::Idle) {

        // New call - set status to Ready
        lineManager_.setStatus(index, model::LineStatus::Ready);
        
      // Hook ON -> Hook OFF & Line status = Incoming
      } else if (line.previousHookStatus == model::HookStatus::On
        && line.currentHookStatus == model::HookStatus::Off
        && line.currentLineStatus == model::LineStatus::Incoming) {

        // Answer the call - set status to Connected and update connection info
        lineManager_.setStatus(incomingFrom, model::LineStatus::Connected);
        line.outgoingTo = incomingFrom;
        lineManager_.getLine(incomingFrom).incomingFrom = index;
        lineManager_.setStatus(index, model::LineStatus::Connected);
          
      // Hook OFF -> Hook ON & Line status = Connected
      } else if (line.previousHookStatus == model::HookStatus::Off 
        && line.currentHookStatus == model::HookStatus::On 
        && line.currentLineStatus == model::LineStatus::Connected) {
        
        lineManager_.setStatus(incomingFrom, model::LineStatus::Disconnected);
        lineManager_.setStatus(index, model::LineStatus::Idle);

      // Hook OFF -> Hook ON & Line status = Disconnected (other party already hung up)
      } else if (line.previousHookStatus == model::HookStatus::Off 
        && line.currentHookStatus == model::HookStatus::On 
        && line.currentLineStatus == model::LineStatus::Disconnected) {
        
        lineManager_.setStatus(index, model::LineStatus::Idle);
      
      // Hook OFF -> Hook ON & Line status = Ringing (caller hangs up while phone is ringing)
      } else if (line.previousHookStatus == model::HookStatus::Off 
        && line.currentHookStatus == model::HookStatus::On
        && line.currentLineStatus == model::LineStatus::Ringing) {
        
        lineManager_.setStatus(line.outgoingTo, model::LineStatus::Idle);
        lineManager_.setStatus(index, model::LineStatus::Idle);
        
      // Hook OFF -> Hook ON: For any other status, if the hook is put back on, we set the line to Idle
      } else {
        lineManager_.setStatus(index, model::LineStatus::Idle);
      }

      // Update previous hook status after processing the change
      line.previousHookStatus = line.currentHookStatus;
    } 
  }
}

// Check for line status changes and handle actions
void LineAction::statusChangeCheck() {
  if(lineManager_.lineStatusChangeFlag != 0){
    uint8_t changes = lineManager_.lineStatusChangeFlag & settings_.activeLinesMask;
    for (int index = 0; index < 8; ++index)
      if (changes & (1 << index)) {
        lineManager_.clearChangeFlag(index); // Clear the change flag
        // Handle the action for the line
        action(index);
      } 
  }
}

// Check for timer expirations and handle them
void LineAction::timerExpiredCheck() {
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
      ringGenerator_.stopRingingLine(index);
      break;

    case LineStatus::Ready:
      startToneGenForStatus(line, model::ToneId::Ready);
      break;
    
    case LineStatus::PulseDialing:
      turnOffToneGenIfUsed(line);
      break;
    
    case LineStatus::ToneDialing:
      turnOffToneGenIfUsed(line);
      break;
    
    case LineStatus::Ringing:
      turnOffToneGenIfUsed(line);
      startToneGenForStatus(line, model::ToneId::Ring);
      lineManager_.setLineTimer(index, settings_.timer_Ringing);
      break;
    
    case LineStatus::Incoming:
      //ringGenerator_.generateRingSignal(index);
      lineManager_.setLineTimer(index, settings_.timer_incomming);
      break;
    
    case LineStatus::Connected:
      turnOffToneGenIfUsed(line);
      ringGenerator_.stopRingingLine(index);
      connectionHandler_.connectLines(index, line.incomingFrom);
      break;
    
    case LineStatus::Busy:
      turnOffToneGenIfUsed(line);
      startToneGenForStatus(line, model::ToneId::Busy);
      lineManager_.setLineTimer(index, settings_.timer_busy);
      break;

    case LineStatus::Fail:
      turnOffToneGenIfUsed(line);
      startToneGenForStatus(line, model::ToneId::Fail);
      lineManager_.setLineTimer(index, settings_.timer_fail);
      break;

    case LineStatus::Disconnected:
      turnOffToneGenIfUsed(line);
      lineManager_.setLineTimer(index, settings_.timer_disconnected);
      connectionHandler_.disconnectLines(index, line.incomingFrom);
      break;
    
    case LineStatus::Timeout:
      turnOffToneGenIfUsed(line);
      lineManager_.setLineTimer(index, settings_.timer_timeout);
      break;
    
    case LineStatus::Abandoned:
      turnOffToneGenIfUsed(line);
      lineManager_.setLineTimer(index, settings_.timer_timeout);
      break;
    
    case LineStatus::Operator:
      break;

    default:
      break;
  }
}

// Handles a line when its timer has expired
void LineAction::timerExpired(LineHandler& line) {
  using namespace model;
  int index = line.lineNumber;
  LineStatus currentStatus = line.currentLineStatus;

  if (settings_.debugLALevel >= 1) {
    Serial.println("LineAction: Timer expired for line " + String(index) + " in state " + model::LineStatusToString(currentStatus));
    util::UIConsole::log("LineAction: Timer expired for line " + String(index) + " in state " + model::LineStatusToString(currentStatus), "LineAction");
  }

  switch (currentStatus) {
    case LineStatus::Ready:
      lineManager_.setStatus(index, LineStatus::Timeout);
      break;

    case LineStatus::ToneDialing:
    case LineStatus::PulseDialing: {
      
      int lineCalled = lineManager_.searchPhoneNumber(line.dialedDigits);
      if (settings_.debugLALevel >= 1) {
        Serial.println("LineAction: ToneDialing line " + String(index) + " dialed digits: " + line.dialedDigits + ", found lineCalled: " + String(lineCalled));
        util::UIConsole::log("ToneDialing line " + String(index) + " dialed digits: " + line.dialedDigits + ", found lineCalled: " + String(lineCalled), "LineAction");
      }
      
      if (lineCalled == index){
        Serial.println(RED "LineAction: Line " + String(index) + " dialed its own number. Setting to Fail." + COLOR_RESET);
        lineManager_.setStatus(index, LineStatus::Fail);
        break;
      }

      // Check if a matching phone number was found
      if (lineCalled != -1){

        // Check if the line that is being called is active
        if (!lineManager_.getLine(lineCalled).lineActive){
          Serial.println(RED "LineAction: Line " + String(lineCalled) + " is not active. Setting to Fail." + COLOR_RESET);
          lineManager_.setStatus(index, LineStatus::Fail);
          return;
        }
        
        // Check if the called line is idle
        if (lineManager_.getLine(lineCalled).currentLineStatus != LineStatus::Idle){
          Serial.println(RED "LineAction: Line " + String(lineCalled) + " is not idle. Setting line " + String(index) + " to Busy." + COLOR_RESET);
          lineManager_.setStatus(index, LineStatus::Busy);
          return;
        }

        // Changing status for the calling line and the called line to start the ringing process
        line.outgoingTo = lineCalled;
        lineManager_.getLine(lineCalled).incomingFrom = index;
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
  if (settings_.debugLALevel >= 2) {
    Serial.println("LineAction: Starting tone generator for line " + String(line.lineNumber) + " with toneId " + ToneIdToString(status));
    util::UIConsole::log("Starting tone generator for line " + String(line.lineNumber) + " with status " + ToneIdToString(status), "LineAction");
  }

  if (!toneGen1_.isPlaying()){
    toneGen1_.startToneSequence(status);
    line.toneGenUsed = cfg::mt8816::DAC1;
    connectionHandler_.connectAudioToLine(line.lineNumber, cfg::mt8816::DAC1); // Connect line to tone generator 1
    if (settings_.debugLALevel >= 2) {
      Serial.println("LineAction: Tone generator 1 is free, using it for line " + String(line.lineNumber));
      util::UIConsole::log("Tone generator 1 is free, using it for line " + String(line.lineNumber), "LineAction");
    }
  }
  else if (!toneGen2_.isPlaying()){
    toneGen2_.startToneSequence(status);
    line.toneGenUsed = cfg::mt8816::DAC2;
    connectionHandler_.connectAudioToLine(line.lineNumber, cfg::mt8816::DAC2); // Connect line to tone generator 2
    if (settings_.debugLALevel >= 2) {
      Serial.println("LineAction: Tone generator 1 is busy, using tone generator 2 for line " + String(line.lineNumber));
      util::UIConsole::log("Tone generator 1 is busy, using tone generator 2 for line " + String(line.lineNumber), "LineAction");
    }
  }
  else if (!toneGen3_.isPlaying()){
    toneGen3_.startToneSequence(status);
    line.toneGenUsed = cfg::mt8816::DAC3;
    connectionHandler_.connectAudioToLine(line.lineNumber, cfg::mt8816::DAC3); // Connect line to tone generator 3
    if (settings_.debugLALevel >= 2) {
      Serial.println("LineAction: Tone generator 1 and 2 are busy, using tone generator 3 for line " + String(line.lineNumber));
      util::UIConsole::log("Tone generator 1 and 2 are busy, using tone generator 3 for line " + String(line.lineNumber), "LineAction");
    }
  }
  else {
    Serial.println(RED "LineAction: All tone generators are busy! Cannot play tone for line " + String(line.lineNumber) + COLOR_RESET);
    util::UIConsole::log("All tone generators are busy! Cannot play tone for line " + String(line.lineNumber), "LineAction");
  }
}

// Turn off tone generator if it is being used by the line
void LineAction::turnOffToneGenIfUsed(LineHandler& line) {
  if (line.toneGenUsed == 0) {
    return;
  }

  switch (line.toneGenUsed) {
    case cfg::mt8816::DAC1:
      toneGen1_.stop();
      break;
    case cfg::mt8816::DAC2:
      toneGen2_.stop();
      break;
    case cfg::mt8816::DAC3:
      toneGen3_.stop();
      break;
    default:
      // Invalid mapping, clear state to avoid repeated faults.
      line.toneGenUsed = 0;
      return;
  }

  connectionHandler_.disconnectAudioToLine(line.lineNumber, line.toneGenUsed); // Disconnect line from tone generator
  line.toneGenUsed = 0;
}

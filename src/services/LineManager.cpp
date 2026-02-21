#include "LineManager.h"
#include <Arduino.h>
#include "services/ToneReader.h"
#include <vector>

LineManager::LineManager(Settings& settings)
:settings_(settings){
  lines.reserve(8);

  auto& s = Settings::instance();   // singleton
  for (int i = 0; i < 8; ++i) {
    lines.emplace_back(i);
    lines.back().phoneNumber = s.linePhoneNumbers[i];
    lines.back().lineName = s.lineNames[i];
    bool isActive = ((s.activeLinesMask >> i) & 0x01) != 0;
    lines.back().lineActive = isActive;
  }
  lineStatusChangeFlag = 0;     // Intiate to zero (no changes)
  lineHookChangeFlag = 0;       // Intiate to zero (no hook changes)
  activeTimersMask = 0;         // Intiate to zero (no active timers)
  linesNotIdle = 0;             // Intiate to zero (all lines idle)
  lastLineReady = -1;           // No line is ready at start
  toneScanMask = 0;             // Intiate to zero (no lines to scan for tones)
}

void LineManager::begin() {
  for (auto& line : lines) {
    line.lineIdle();
  }
}

void LineManager::syncLineActive(size_t i) {
  auto& settings_ = Settings::instance();
  bool isActive = ((settings_.activeLinesMask >> i) & 0x01) != 0;
  lines[i].lineActive = isActive;
}

// Returns a reference to the LineHandler object for the specified line index
LineHandler& LineManager::getLine(int index) {
  if (index < 0 || index >= static_cast<int>(lines.size())) {
    Serial.print("LineManager::getLine - ogiltigt index: ");
    Serial.println(index);
    util::UIConsole::log("LineManager::getLine - ogiltigt index: " + String(index), "LineManager");
    // Undvik exceptions i Arduino-miljö; returnera första som “safe fallback”
    return lines[0];
  }
  return lines[static_cast<size_t>(index)];
}

// Set the status for the specified line
void LineManager::setStatus(int index, LineStatus newStatus) {
  // Validate index
  if (index < 0 || index >= static_cast<int>(lines.size())) {
    Serial.print("LineManager::setStatus - ogiltigt index: ");
    Serial.println(index);
    util::UIConsole::log("LineManager::setStatus - ogiltigt index: " + String(index), "LineManager");
    return;
  }

  // Uppdating the status and changing previous status
  lines[index].previousLineStatus = lines[index].currentLineStatus;
  lines[index].currentLineStatus = newStatus;
  resetLineTimer(index); // Reset any existing timer for this line


  // Setting up bitmasks and handling tone reader activation based on the new status
  switch (newStatus) {
    case LineStatus::Idle:
      lines[index].lineIdle();          // Reset line variables when setting to Idle
      linesNotIdle &= ~(1 << index);    // Clear the bit for this line
      toneScanMask &= ~(1 << index);    // Clear the bit to stop scanning this line for tones
      if (lastLineReady == index) {     // Reset lastLineReady if this was the line that was last ready
        lastLineReady = -1;
      }

      // Deactivate MT8870 if no lines need tone scanning
      if (toneScanMask == 0 && toneReader_ != nullptr) {  
        toneReader_->deactivate();
      }
      break;
    
    case LineStatus::Ready:
    case LineStatus::ToneDialing:
    case LineStatus::PulseDialing:
      lastLineReady = index;             // Update the most recent line used for tone scanning
      linesNotIdle |= (1 << index);      // Set the bit for this line
      toneScanMask |= (1 << index);      // Set the bit to scan this line for tones

      // Activate MT8870 if not already active
      if (toneReader_ != nullptr && !toneReader_->isActive) {
        toneReader_->activate();
      }
      break;
  
    default:
      linesNotIdle &= ~(1 << index);     // Clear the bit for this line since we do not need to scan for 
                                         // tones in lines that are not Ready or PulseDialing
      toneScanMask &= ~(1 << index);
      if (toneScanMask == 0 && toneReader_ != nullptr) {
        toneReader_->deactivate();
      }
      break;
  }

  // No matther what the new status is, we want to set the lineStatusChangeFlag so that LineAction 
  // can handle any necessary actions based on the new status
  lineStatusChangeFlag |= (1 << index);           
  for (auto& cb : statusChangedCallbacks_) {
    if (cb) cb(index, newStatus);
  }

  if (settings_.debugLmLevel >= 0) {
    Serial.print(BLUE "LineManager:        Line ");
    Serial.print(index);
    Serial.print(" status changed to ");
    Serial.print(model::LineStatusToString(newStatus));
    Serial.println(COLOR_RESET);
    util::UIConsole::log("Line " + String(index) + " status changed to " + model::LineStatusToString(newStatus), "LineManager");
  }

  
}

// Clear the status change flag for the specified line
void LineManager::clearChangeFlag(int index) {
  if (index < 0 || index >= static_cast<int>(lines.size())) {
    Serial.print("LineManager::clearChangeFlag - ogiltigt index: ");
    Serial.println(index);
    util::UIConsole::log("LineManager::clearChangeFlag - ogiltigt index: " + String(index), "LineManager");
    return;
  }
  // Clear the change flag for the specified line
  lineStatusChangeFlag &= ~(1 << index); 
}

// 
void LineManager::setStatusChangedCallback(StatusChangedCallback cb) {
  statusChangedCallbacks_.clear();
  if (cb) statusChangedCallbacks_.push_back(std::move(cb));
}

void LineManager::addStatusChangedCallback(StatusChangedCallback cb) {
  if (cb) statusChangedCallbacks_.push_back(std::move(cb));
}

// Set a timer for the specified line
void LineManager::setLineTimer(int index, unsigned int limit) {
  if (index < 0 || index >= static_cast<int>(lines.size())) {
    Serial.print("LineManager::setLineTimer - ogiltigt index: ");
    Serial.println(index);
    util::UIConsole::log("LineManager::setLineTimer - ogiltigt index: " + String(index), "LineManager");
    return;

  } else {
    // Set timer end time
    if (settings_.debugLmLevel >= 1){
      Serial.print("LineManager:        Setting timer for line ");
      Serial.print(index);
      Serial.print(" with limit ");
      Serial.print(limit);
      Serial.println(" ms");
      util::UIConsole::log("LineManager:        Setting timer for line " + String(index) + 
                          " with limit " + String(limit) + " ms", "LineManager");
    }
    lines[index].lineTimerEnd = millis() + limit;
    activeTimersMask |= (1 << index);  // Set the timer active flag
  }
}

// Reset (disable) the timer for the specified line
void LineManager::resetLineTimer(int index) {
  if (index < 0 || index >= static_cast<int>(lines.size())) {
    if (settings_.debugLmLevel >= 1){
      Serial.println("LineManager:        resetLineTimer - invalid index");
      util::UIConsole::log("LineManager:        resetLineTimer - invalid index", "LineManager");
    }
    return;
  }

  if (settings_.debugLmLevel >= 1){
    Serial.print("LineManager:        Resetting timer for line ");
    Serial.println(index);
    util::UIConsole::log("LineManager:        Resetting timer for line " + String(index), "LineManager");
  }
  lines[index].lineTimerEnd = -1;
  activeTimersMask &= ~(1 << index); // Clear the timer active flag
}

// Set the phone number for the specified line
void LineManager::setPhoneNumber(int index, const String& value) {
  if (index < 0 || index >= static_cast<int>(lines.size())) {
    Serial.print("LineManager::setPhoneNumber - ogiltigt index: ");
    Serial.println(index);
    util::UIConsole::log("LineManager::setPhoneNumber - ogiltigt index: " + String(index), "LineManager");
    return;
  }

  String sanitized = value;
  sanitized.trim();

  lines[index].phoneNumber = sanitized;
  settings_.linePhoneNumbers[index] = sanitized;
}

void LineManager::setLineName(int index, const String& value) {
  if (index < 0 || index >= static_cast<int>(lines.size())) {
    Serial.print("LineManager::setLineName - ogiltigt index: ");
    Serial.println(index);
    util::UIConsole::log("LineManager::setLineName - ogiltigt index: " + String(index), "LineManager");
    return;
  }

  String sanitized = value;
  sanitized.trim();

  lines[index].lineName = sanitized;
  settings_.lineNames[index] = sanitized;
}

// Search for a line index based on the provided phone number. Returns -1 if not found.
int LineManager::searchPhoneNumber(const String& phoneNumber) {
  // Trimma och förbered söksträngen

  // Degbug output of current phone numbers
  if (settings_.debugLmLevel >= 2){
    Serial.print("Numbers: ");
    for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
      Serial.print(String(lines[i].phoneNumber));
      Serial.print(", ");
    }
    Serial.println();
  }
  
  // Debug output for search
  if (settings_.debugLmLevel >= 1){
    Serial.print("LineManager:        Searching for phone number '");
    Serial.print(phoneNumber);
    Serial.println("'");
    util::UIConsole::log("LineManager:        Searching for phone number '" + phoneNumber + "'", "LineManager");
  }

  // Trim the input phone number for searching
  String searchNumber = phoneNumber;
  searchNumber.trim();

  // Loop through lines to find a matching phone number (skipping inactive lines)
  for (size_t i = 0; i < lines.size(); ++i) {
    if (!lines[i].lineActive) {
      continue;
    }
    
    // compare the line's phone number with the search number
    if (lines[i].phoneNumber == searchNumber) {
      
      if (settings_.debugLmLevel >= 1){
        Serial.print("LineManager:        Found matching phone number on line ");
        Serial.print(i);
        Serial.println();
        util::UIConsole::log("LineManager:        Found matching phone number on line " + String(i), "LineManager");
      }
      return static_cast<int>(i);  // Return the line index
    }
  }
  
  return -1;  // No match found
}

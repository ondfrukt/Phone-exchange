#include "LineManager.h"
#include <Arduino.h>
#include <vector>

LineManager::LineManager(Settings& settings)
:settings_(settings){
  lines.reserve(8);

  auto& s = Settings::instance();   // singleton
  for (int i = 0; i < 8; ++i) {
    lines.emplace_back(i);

    lines.back().phoneNumber = s.linePhoneNumbers[i];
    bool isActive = ((s.activeLinesMask >> i) & 0x01) != 0;
    lines.back().lineActive = isActive;
  }
  lineChangeFlag = 0;     // Intiate to zero (no changes)
  activeTimersMask = 0;   // Intiate to zero (no active timers)
  linesNotIdle = 0;       // Intiate to zero (all lines idle)
  linesRinging = 0;       // Intiate to zero (no lines ringing)
  lastLineReady = -1;     // No line is ready at start
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

void LineManager::setStatus(int index, LineStatus newStatus) {
  if (index < 0 || index >= static_cast<int>(lines.size())) {
    Serial.print("LineManager::setStatus - ogiltigt index: ");
    Serial.println(index);
    util::UIConsole::log("LineManager::setStatus - ogiltigt index: " + String(index), "LineManager");
    return;
  }

  // Uppdating the status and changing previous status
  lines[index].previousLineStatus = lines[index].currentLineStatus;
  lines[index].currentLineStatus = newStatus;


  if (newStatus == LineStatus::Idle) {
    lines[index].lineIdle();
    linesNotIdle &= ~(1 << index);          // Clear the bit for this line
    // Reset lastLineReady if this was the line that was last ready
    if (lastLineReady == index) {
      lastLineReady = -1;
    }
  }
  else if (newStatus == LineStatus::Ready) {
    lastLineReady = index;                   // Update the most recent Ready line
    linesNotIdle |= (1 << index);            // Set the bit for this line
  }
  else {
    linesNotIdle |= (1 << index);           // Set the bit for this line
  } 

  lineChangeFlag |= (1 << index);           // Set the change flag for the specified line
  
  if (settings_.debugLmLevel >= 1){
    Serial.println("LineManager: Callback function called");
    util::UIConsole::log("LineManager: Callback function called", "LineManager");
  }

  if (pushStatusChanged_) pushStatusChanged_(index, newStatus);  // Call the callback if set

  if (settings_.debugLmLevel >= 0) {
    Serial.print("LineManager: Line ");
    Serial.print(index);
    Serial.print(" status changed to ");
    Serial.println(model::toString(newStatus));
    util::UIConsole::log("Line " + String(index) + " status changed to " + model::toString(newStatus), "LineManager");
  }
}

void LineManager::clearChangeFlag(int index) {
  if (index < 0 || index >= static_cast<int>(lines.size())) {
    Serial.print("LineManager::clearChangeFlag - ogiltigt index: ");
    Serial.println(index);
    util::UIConsole::log("LineManager::clearChangeFlag - ogiltigt index: " + String(index), "LineManager");
    return;
  }
  // Clear the change flag for the specified line
  lineChangeFlag &= ~(1 << index); 
}

void LineManager::setStatusChangedCallback(StatusChangedCallback cb) {
  pushStatusChanged_ = std::move(cb);
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
      Serial.print("LineManager: Setting timer for line ");
      Serial.print(index);
      Serial.print(" with limit ");
      Serial.print(limit);
      Serial.println(" ms");
      util::UIConsole::log("LineManager: Setting timer for line " + String(index) + 
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
      Serial.println("LineManager: resetLineTimer - invalid index");
      util::UIConsole::log("LineManager: resetLineTimer - invalid index", "LineManager");
    }
    return;
  }

  if (settings_.debugLmLevel >= 1){
    Serial.print("LineManager: Resetting timer for line ");
    Serial.println(index);
    util::UIConsole::log("LineManager: Resetting timer for line " + String(index), "LineManager");
  }
  lines[index].lineTimerEnd = -1;
  activeTimersMask &= ~(1 << index); // Clear the timer active flag
}

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

#include "LineManager.h"
#include <Arduino.h>
#include <vector>

LineManager::LineManager(Settings& settings)
:settings_(settings){
  lines.reserve(8);

  auto& s = Settings::instance();   // singleton
  for (int i = 0; i < 8; ++i) {
    lines.emplace_back(i);

    bool isActive = ((s.activeLinesMask >> i) & 0x01) != 0;
    lines.back().lineActive = isActive;
  }
  lineChangeFlag = 0;     // Intiate to zero (no changes)
  activeTimersMask = 0;   // Intiate to zero (no active timers)
  linesNotIdle = 0;       // Intiate to zero (all lines idle)
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
    // Undvik exceptions i Arduino-miljö; returnera första som “safe fallback”
    return lines[0];
  }
  return lines[static_cast<size_t>(index)];
}

void LineManager::setStatus(int index, LineStatus newStatus) {
  if (index < 0 || index >= static_cast<int>(lines.size())) {
    Serial.print("LineManager::setStatus - ogiltigt index: ");
    return;
  }

  // Uppdating the status and changing previous status
  lines[index].previousLineStatus = lines[index].currentLineStatus;
  lines[index].currentLineStatus = newStatus;


  if (newStatus == LineStatus::Idle) {
    lines[index].lineIdle();
    linesNotIdle &= ~(1 << index);          // Clear the bit for this line
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
  }

  if (pushStatusChanged_) pushStatusChanged_(index, newStatus);  // Call the callback if set

  if (settings_.debugLmLevel >= 0) {
    Serial.print("LineManager: Line ");
    Serial.print(index);
    Serial.print(" status changed to ");
    Serial.println(model::toString(newStatus));
  }
}

void LineManager::clearChangeFlag(int index) {
  if (index < 0 || index >= static_cast<int>(lines.size())) {
    Serial.print("LineManager::clearChangeFlag - ogiltigt index: ");
    Serial.println(index);
    return;
  }
  // Clear the change flag for the specified line
  lineChangeFlag &= ~(1 << index); 
}

void LineManager::setStatusChangedCallback(StatusChangedCallback cb) {
  pushStatusChanged_ = std::move(cb);
}
#include "LineManager.h"
#include <Arduino.h>
#include <vector>

LineManager::LineManager(Settings& settings)
:settings_(settings){
  lines.reserve(8);

  auto& s = Settings::instance();   // singleton
  for (int i = 0; i < 8; ++i) {
    lines.emplace_back(i);

    // Om du fortfarande använder 'activeLines' (bitmask 0..7), behåll raden nedan.
    // Byter du till 'activeLinesMask' – ändra namnet här.
    bool isActive = ((s.activeLinesMask >> i) & 0x01) != 0;
    lines.back().lineActive = isActive;
  }

  lineChangeFlag = 0; // Initiera flaggan

}

void LineManager::begin() {
  for (auto& line : lines) {
    line.lineIdle();
  }
}

void LineManager::update() {
  // Hämta ev. ändrad mask under körning och spegla in i lineActive
  auto& s = Settings::instance();
  for (size_t i = 0; i < lines.size(); ++i) {
    bool isActive = ((s.activeLinesMask >> i) & 0x01) != 0;  // eller s.activeLinesMask
    lines[i].lineActive = isActive;
  }
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

  lines[index].previousLineStatus = lines[index].currentLineStatus;
  lines[index].currentLineStatus = newStatus;
  lineChangeFlag |= (1 << index); // Sätt motsvarande bit i flaggan

  if (settings_.debugLmLevel >= 1) {
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
  lineChangeFlag &= ~(1 << index); // Rensa motsvarande bit i flaggan
}

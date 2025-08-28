#include "LineManager.h"
#include <Arduino.h>
#include "settings/settings.h"   // <-- byt från settings_global.h

LineManager::LineManager() {
  lines.reserve(8);

  auto& s = Settings::instance();   // singleton
  for (int i = 0; i < 8; ++i) {
    lines.emplace_back(i);

    // Om du fortfarande använder 'activeLines' (bitmask 0..7), behåll raden nedan.
    // Byter du till 'activeLinesMask' – ändra namnet här.
    bool isActive = ((s.activeLinesMask >> i) & 0x01) != 0;
    lines.back().lineActive = isActive;
  }
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

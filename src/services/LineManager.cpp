#include "LineManager.h"

// Konstruktor. Skapar en vektor med LineHandler-objekt
LineManager::LineManager(int activeLines) {
  lines.reserve(activeLines);
  for (int i = 0; i < activeLines; ++i) {
    lines.emplace_back(i);  // Skapar en ny LineHandler i slutet av vektorn
  }
}

// Går igenom alla linjer och nollställer deras status
void LineManager::begin() {
  for (auto& line : lines) {
    line.lineIdle();  // nollställer interna variabler
  }
}

LineHandler& LineManager::getLine(int index) {
  // Säker indexhantering (clamp)
  if (lines.empty()) {
    // Skapa en default-linje om inget finns (bälte & hängslen)
    lines.emplace_back(0);
    lines.back().lineIdle();
  }
  if (index < 0) index = 0;
  if (index >= static_cast<int>(lines.size())) index = static_cast<int>(lines.size()) - 1;
  return lines[static_cast<size_t>(index)];
}

void LineManager::update() {
  // Lägg ev. periodiskt underhåll här:
  // - timers (om du senare gör LineHandler::update(nowMs))
  // - övervakning av tillstånd mellan linjer
  // just nu: inget att göra
}
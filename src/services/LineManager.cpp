#include "LineManager.h"
#include "settings_global.h"


// Konstruktor. Skapar en vektor med LineHandler-objekt
LineManager::LineManager() {
  lines.reserve(8);
  for (int i = 0; i < 8; ++i) {
    lines.emplace_back(i);  // Skapar en ny LineHandler i slutet av vektorn
    bool isActive = (settings.activeLines >> i) & 0x01; // Kontrollera om linjen är aktiv

    // Markerar linjen som aktiv om den är det i inställningarna
    if (isActive) {
      lines.back().lineActive = true;
    }
  }
}

// Går igenom alla linjer och nollställer deras status
void LineManager::begin() {
  for (auto& line : lines) {
    line.lineIdle();  // nollställer interna variabler
  }
}

LineHandler& LineManager::getLine(int index) {
  if (index >= static_cast<int>(lines.size())) {
      Serial.print("Index not valid: ");
      Serial.println(index);
      throw std::out_of_range("Invalid index in getLine");
  }
  return lines[static_cast<size_t>(index)];
}

void LineManager::update() {
  // Lägg ev. periodiskt underhåll här:
  // - timers (om du senare gör LineHandler::update(nowMs))
  // - övervakning av tillstånd mellan linjer
  // just nu: inget att göra
}
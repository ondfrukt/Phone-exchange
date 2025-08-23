#pragma once
#include <vector>
#include "LineHandler.h"
#include "settings_global.h"

class LineManager {
  public:
    LineManager();
    void begin();
    void update();
    LineHandler& getLine(int index); // Hämtar en specifik linje
  
  private:
    std::vector<LineHandler> lines; // Vektor för att lagra linjehanterare
};

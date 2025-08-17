#pragma once
#include <vector>
#include "LineHandler.h"

class LineManager {
  public:
    LineManager(int acriveLines);
    void begin();
    void update();
    LineHandler& getLine(int index); // Hämtar en specifik linje
  
  private:
    std::vector<LineHandler> lines; // Vektor för att lagra linjehanterare
};

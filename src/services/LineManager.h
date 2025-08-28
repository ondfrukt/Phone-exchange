#pragma once
#include <vector>
#include "LineHandler.h"
#include "settings/settings.h"     // <-- byt från settings_global.h

class LineManager {
public:
  LineManager();
  void begin();
  void update();
  LineHandler& getLine(int index);

private:
  std::vector<LineHandler> lines;
};

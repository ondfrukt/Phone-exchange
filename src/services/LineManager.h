#pragma once
#include "settings/settings.h"
#include "model/Types.h"
#include "LineHandler.h"

class LineManager {
public:
  LineManager(Settings& settings);
  void begin();
  void update();
  void setStatus(int index, LineStatus newStatus);
  void clearChangeFlag(int index);
  void setLineTimer(int index, unsigned int limit);

  LineHandler& getLine(int index);

  uint8_t lineChangeFlag;
  uint8_t activeLineTimers; // Bitmask för aktiva linjetimers

private:
  std::vector<LineHandler> lines;
  Settings& settings_;
};

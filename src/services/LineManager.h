#pragma once
#include "settings/settings.h"
#include "model/Types.h"
#include "LineHandler.h"

class LineManager {
public:
  LineManager(Settings& settings);
  void begin();
  void syncLineActive(size_t i);
  void setStatus(int index, LineStatus newStatus);
  void clearChangeFlag(int index);
  void setLineTimer(int index, unsigned int limit);

  using StatusChangedCallback = std::function<void(int /*lineIndex*/, model::LineStatus)>;
  using ActiveLinesChangedCallback = std::function<void(uint8_t)>;

  // Callback functions for webserver
  void setStatusChangedCallback(StatusChangedCallback cb);
  void setActiveLinesChangedCallback(ActiveLinesChangedCallback cb);

  LineHandler& getLine(int index);

  uint8_t lineChangeFlag;
  uint8_t activeTimersMask; // Bitmask for active line timers
  uint8_t linesNotIdle;     // Bitmask for lines that are not Idle
  int lastLineReady;   // Most recent line that became Ready (for toneReader)

private:
  // The vector that holds all LineHandler instances
  std::vector<LineHandler> lines;

  Settings& settings_;
  StatusChangedCallback pushStatusChanged_{nullptr};

};

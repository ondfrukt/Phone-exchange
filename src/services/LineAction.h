#pragma once
#include <Arduino.h>
#include "model/Types.h"
#include "settings/Settings.h"   // dubbelkolla filnamn/versaler
#include "LineManager.h"
#include "drivers/MT8816Driver.h"

class LineAction {
public:
  LineAction(LineManager& lineManager, Settings& settings, MT8816Driver& mt8816Driver);
  void update();
  void action(int index);

private:
  LineManager& lineManager_;
  Settings&    settings_;
  MT8816Driver& mt8816Driver_;
  void timerExpired(LineHandler& line);
};

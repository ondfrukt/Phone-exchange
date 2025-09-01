#pragma once
#include <Arduino.h>
#include "model/Types.h"
#include "settings/Settings.h"   // dubbelkolla filnamn/versaler
#include "LineManager.h"

class LineAction {
public:
  LineAction(LineManager& lineManager, Settings& settings);
  void update();
  void action(int index);

private:
  LineManager& lineManager_;
  Settings&    settings_;
};

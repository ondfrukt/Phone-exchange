#pragma once
#include <Arduino.h>
#include "model/Types.h"
#include "settings/Settings.h"   // dubbelkolla filnamn/versaler
#include "LineManager.h"
#include "drivers/MT8816Driver.h"
#include "services/ToneGenerator.h"

class LineAction {
public:
  LineAction(LineManager& lineManager, Settings& settings, MT8816Driver& mt8816Driver,
             ToneGenerator& toneGen0, ToneGenerator& toneGen1, ToneGenerator& toneGen2);
  
  void begin();
  void update();
  void action(int index);

private:
  LineManager& lineManager_;
  Settings&    settings_;
  MT8816Driver& mt8816Driver_;
  ToneGenerator& toneGen0_;
  ToneGenerator& toneGen1_;
  ToneGenerator& toneGen2_;
  ToneGenerator* toneGenerators[3];

  void turnOffToneGenIfUsed(LineHandler& line);
  void startToneGenForStatus(LineHandler& line, model::ToneId status);

  void timerExpired(LineHandler& line);
};

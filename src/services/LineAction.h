#pragma once
#include <Arduino.h>
#include "drivers/MT8816Driver.h"
#include "model/Types.h"
#include "settings/Settings.h"   // dubbelkolla filnamn/versaler
#include "LineManager.h"
#include "services/RingGenerator.h"
#include "services/ToneReader.h"
#include "services/ConnectionHandler.h"
#include "services/ToneGenerator.h"

class LineAction {
public:
  LineAction(LineManager& lineManager, Settings& settings, MT8816Driver& mt8816Driver, RingGenerator& ringGenerator,
             ToneReader& toneReader,
             ToneGenerator& toneGen1, ToneGenerator& toneGen2, ToneGenerator& toneGen3, ConnectionHandler& connectionHandler);
  
  void begin();
  void update();
  void action(int index);

private:
  LineManager& lineManager_;
  Settings&    settings_;
  MT8816Driver& mt8816Driver_;
  RingGenerator& ringGenerator_;
  ToneReader& toneReader_;
  ToneGenerator& toneGen1_;
  ToneGenerator& toneGen2_;
  ToneGenerator& toneGen3_;
  ToneGenerator* toneGenerators[3];
  ConnectionHandler& connectionHandler_;

  void turnOffToneGenIfUsed(LineHandler& line);
  void startToneGenForStatus(LineHandler& line, model::ToneId status);

  void timerExpired(LineHandler& line);
};

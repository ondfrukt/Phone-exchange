#pragma once
#include <Arduino.h>
#include "drivers/MT8816Driver.h"
#include "model/Types.h"
#include "settings/Settings.h"
#include "LineManager.h"
#include "services/RingGenerator.h"
#include "services/ToneReader.h"
#include "services/ConnectionHandler.h"
#include "services/ToneGenerator.h"

namespace net { class MqttClient; }

class LineAction {
public:
  LineAction(LineManager& lineManager, Settings& settings, MT8816Driver& mt8816Driver, RingGenerator& ringGenerator,
             ToneReader& toneReader,
             ToneGenerator& toneGenerator,
             ConnectionHandler& connectionHandler, net::MqttClient& mqttClient);
  
  void begin();
  void update();
  void action(int index);

private:
  LineManager& lineManager_;
  Settings&    settings_;
  MT8816Driver& mt8816Driver_;
  RingGenerator& ringGenerator_;
  ToneReader& toneReader_;
  ToneGenerator& toneGenerator_;
  ConnectionHandler& connectionHandler_;
  net::MqttClient& mqttClient_;

  void hookStatusCangeCheck();
  void statusChangeCheck();
  void timerExpiredCheck();

  void turnOffToneGenIfUsed(LineHandler& line);
  void startToneGenForStatus(LineHandler& line, model::ToneId status);
  void timerExpired(LineHandler& line);
};

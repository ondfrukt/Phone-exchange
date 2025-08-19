#pragma once
#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include "config.h"

class MCPInterruptService {
public:
  MCPInterruptService();

  // Initiera service: koppla ISR till ESP32-pinnen för SLIC1
  void begin(Adafruit_MCP23X17& slic1);

  // Körs i loop() för att processa interrupt
  void poll();

private:
  Adafruit_MCP23X17* slic1_ = nullptr;
  volatile bool slic1Flag_ = false;

  // statisk ISR-trampolin
  static void IRAM_ATTR isrSlic1_();

  // global pekare till instansen
  static MCPInterruptService* instance_;
};
#pragma once
#include <Arduino.h>
#include <vector>
#include <stdint.h>
#include "settings/Settings.h"
#include "drivers/MT8816Driver.h"
#include "util/UIConsole.h"


class LineAudioConnections {
public:
  LineAudioConnections(MT8816Driver& mt8816Driver, Settings& settings) : mt8816Driver_(mt8816Driver), settings(settings) {};

  void connectLines(uint8_t lineA, uint8_t lineB);
  void disconnectLines(uint8_t lineA, uint8_t lineB);

  void connectAudioToLine(uint8_t line, uint8_t audioSource);
  void disconnectAudioToLine(uint8_t line, uint8_t audioSource);

  void printConnections() const;
  bool isConnected(uint8_t lineA, uint8_t lineB) const;

private:

  std::vector<std::pair<uint8_t, uint8_t>> activeConnections;
  void addConnection(uint8_t lineA, uint8_t lineB);
  void removeConnection(uint8_t lineA, uint8_t lineB);

  MT8816Driver& mt8816Driver_;
  Settings& settings;

};
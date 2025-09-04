// Genererar JSON som { "lines":[ {"id":0,"status":"Idle"}, ... ] }
#pragma once
#include <Arduino.h>
class LineManager;

namespace net {
  // Bygger JSON för alla linjer. Exempel:
  // {"lines":[{"id":0,"status":"Idle"}, ...]}
  String buildLinesStatusJson(const LineManager& lm);
}

#include "StatusSerializer.h"
#include "services/LineManager.h"
#include "services/LineHandler.h"
#include "model/Types.h"

namespace net {

String buildLinesStatusJson(const LineManager& lm) {
  // Bygg manuellt för att slippa externa libbar. Lätt att utöka fält senare.
  String out = "{\"lines\":[";
  for (int i = 0; i < 8; ++i) {
    const auto& line = const_cast<LineManager&>(lm).getLine(i); // getLine saknar const-variant
    out += "{\"id\":" + String(i);
    out += ",\"status\":\""; out += model::toString(line.currentLineStatus); out += "\"";
    // Lägg till fler fält här när du vill skala upp:
    // out += ",\"active\":"; out += (line.lineActive ? "true" : "false");
    // out += ",\"hook\":\"";  out += (line.SHK ? "Off" : "On"); out += "\"";
    out += "}";
    if (i < 7) out += ",";
  }
  out += "]}";
  return out;
}

} // namespace net

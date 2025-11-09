#include "StatusSerializer.h"
#include "services/LineManager.h"
#include "services/LineHandler.h"
#include "model/Types.h"

namespace {

String escapeJson(const String& in) {
  String out;
  out.reserve(in.length());
  for (size_t i = 0; i < in.length(); ++i) {
    char c = in.charAt(static_cast<unsigned int>(i));
    if (c == '\\' || c == '\"') out += '\\';
    out += c;
  }
  return out;
}

} // namespace

namespace net {

String buildLinesStatusJson(const LineManager& lm) {
  // Bygg manuellt för att slippa externa libbar. Lätt att utöka fält senare.
  String out = "{\"lines\":[";
  for (int i = 0; i < 8; ++i) {
    const auto& line = const_cast<LineManager&>(lm).getLine(i); // getLine saknar const-variant
    out += "{\"id\":" + String(i);
    out += ",\"status\":\""; out += model::toString(line.currentLineStatus); out += "\"";
    out += ",\"phone\":\""; out += escapeJson(line.phoneNumber); out += "\"";
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

#include "StatusSerializer.h"
#include "services/LineManager.h"
#include "services/LineHandler.h"
#include "model/Types.h"
#include <cstdio>

namespace net {

namespace {
String escapeJsonString(const String& s) {
  String out;
  out.reserve(s.length());
  for (size_t i = 0; i < s.length(); ++i) {
    char c = s[i];
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '\"': out += "\\\""; break;
      case '\b': out += "\\b";  break;
      case '\f': out += "\\f";  break;
      case '\n': out += "\\n";  break;
      case '\r': out += "\\r";  break;
      case '\t': out += "\\t";  break;
      default:
        if ((uint8_t)c < 0x20) {
          char buf[8];
          snprintf(buf, sizeof(buf), "\\u%04x", (uint8_t)c);
          out += buf;
        } else {
          out += c;
        }
    }
  }
  return out;
}
} // namespace

String buildLinesStatusJson(const LineManager& lm) {
  // Bygg manuellt för att slippa externa libbar. Lätt att utöka fält senare.
  String out = "{\"lines\":[";
  for (int i = 0; i < 8; ++i) {
    const auto& line = const_cast<LineManager&>(lm).getLine(i); // getLine saknar const-variant
    out += "{\"id\":" + String(i);
    out += ",\"status\":\""; out += model::toString(line.currentLineStatus); out += "\"";
    out += ",\"phone\":\""; out += escapeJsonString(line.phoneNumber); out += "\"";
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

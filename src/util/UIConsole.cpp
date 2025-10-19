#include "util/UIConsole.h"
#include "freertos/semphr.h"

namespace util {

// --- interna data ---
static std::vector<String> g_buffer;
static size_t g_maxLines = 100;
static ConsoleSink g_sink = nullptr;
static SemaphoreHandle_t g_mutex = nullptr;

// Enkel JSON-escape (lokal helper)
static String escapeJsonString(const String& s) {
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

void UIConsole::init(size_t maxLines) {
  g_maxLines = maxLines ? maxLines : 100;
  if (!g_mutex) {
    g_mutex = xSemaphoreCreateMutex();
  }
  // reservera lite plats för buffer (valfritt)
  g_buffer.reserve(min<size_t>(g_maxLines, 64));
}

static void push_buffer_locked(const String& json) {
  g_buffer.push_back(json);
  if (g_buffer.size() > g_maxLines) {
    // ta bort äldsta
    g_buffer.erase(g_buffer.begin());
  }
}

// Bygg JSON och leverera antingen till sink eller buffra
void UIConsole::log(const String& text, const char* source) {
  // skapa JSON
  const String escText = escapeJsonString(text);
  String json = "{\"text\":\"" + escText + "\"";
  if (source && strlen(source) > 0) {
    const String escSrc = escapeJsonString(String(source));
    json += ",\"source\":\"" + escSrc + "\"";
  }
  json += ",\"ts\":" + String(millis());
  json += "}";

  // kort och skyddat
  if (g_mutex) xSemaphoreTake(g_mutex, portMAX_DELAY);
  if (g_sink) {
    // om sink är satt, leverera direkt (utan buffring)
    // leverera utanför mutex? Vi kan leverera innanför för enkelhet (sink bör vara snabb)
    g_sink(json);
  } else {
    // buffra
    push_buffer_locked(json);
  }
  if (g_mutex) xSemaphoreGive(g_mutex);
}

// Sätt sink; flusha buffer när sink sätts
void UIConsole::setSink(ConsoleSink sink) {
  if (g_mutex) xSemaphoreTake(g_mutex, portMAX_DELAY);
  g_sink = sink;
  // flusha buffer
  if (g_sink && !g_buffer.empty()) {
    // skicka i ordning
    for (const auto &j : g_buffer) {
      g_sink(j);
    }
    g_buffer.clear();
  }
  if (g_mutex) xSemaphoreGive(g_mutex);
}

void UIConsole::clearSink() {
  if (g_mutex) xSemaphoreTake(g_mutex, portMAX_DELAY);
  g_sink = nullptr;
  if (g_mutex) xSemaphoreGive(g_mutex);
}

} // namespace util
#include "util/UIConsole.h"
#include "freertos/semphr.h"
#include <time.h>

namespace util {

static std::vector<String> g_buffer;
static size_t g_maxLines = 100;
static ConsoleSink g_sink = nullptr;
static SemaphoreHandle_t g_mutex = nullptr;

static String escapeJsonString(const String& s) {
  String out;
  out.reserve(s.length());
  for (size_t i = 0; i < s.length(); ++i) {
    char c = s[i];
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
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

static String stripInlineSourcePrefix(const String& text, const char* source) {
  if (!source || strlen(source) == 0) return text;
  const String prefix = String(source) + ":";
  if (!text.startsWith(prefix)) return text;

  size_t i = prefix.length();
  while (i < text.length() && text[i] == ' ') {
    ++i;
  }
  return text.substring(i);
}

static unsigned long timestampForLog() {
  const time_t now = time(nullptr);
  // 1700000000 ~= Nov 2023. Before that, RTC is likely not synced yet.
  if (now > 1700000000) {
    return static_cast<unsigned long>(now);
  }
  return millis();
}

void UIConsole::init(size_t maxLines) {
  g_maxLines = maxLines ? maxLines : 100;
  if (!g_mutex) {
    g_mutex = xSemaphoreCreateMutex();
  }
  g_buffer.reserve(min<size_t>(g_maxLines, 64));
}

static void push_buffer_locked(const String& json) {
  g_buffer.push_back(json);
  if (g_buffer.size() > g_maxLines) {
    g_buffer.erase(g_buffer.begin());
  }
}

void UIConsole::log(const String& text, const char* source) {
  const String displayText = stripInlineSourcePrefix(text, source);
  const String escText = escapeJsonString(displayText);

  String json = "{\"text\":\"" + escText + "\"";
  if (source && strlen(source) > 0) {
    const String escSrc = escapeJsonString(String(source));
    json += ",\"source\":\"" + escSrc + "\"";
  }
  json += ",\"ts\":" + String(timestampForLog()) + "}";

  if (g_mutex) xSemaphoreTake(g_mutex, portMAX_DELAY);
  push_buffer_locked(json);
  ConsoleSink sink = g_sink;
  if (g_mutex) xSemaphoreGive(g_mutex);

  if (sink) {
    sink(json);
  }
}

void UIConsole::setSink(ConsoleSink sink) {
  if (g_mutex) xSemaphoreTake(g_mutex, portMAX_DELAY);
  g_sink = sink;
  if (g_mutex) xSemaphoreGive(g_mutex);
}

void UIConsole::clearSink() {
  if (g_mutex) xSemaphoreTake(g_mutex, portMAX_DELAY);
  g_sink = nullptr;
  if (g_mutex) xSemaphoreGive(g_mutex);
}

void UIConsole::forEachBuffered(const std::function<void(const String&)>& fn) {
  if (!fn) return;

  std::vector<String> copy;
  if (g_mutex) xSemaphoreTake(g_mutex, portMAX_DELAY);
  copy = g_buffer;
  if (g_mutex) xSemaphoreGive(g_mutex);

  for (const auto& json : copy) {
    fn(json);
  }
}

} // namespace util


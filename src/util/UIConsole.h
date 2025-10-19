#pragma once
#include <Arduino.h>
#include <vector>
#include <functional>

namespace util {

// Sink tar emot en färdig JSON-sträng (t.ex. {"text":"...","source":"...","ts":123})
using ConsoleSink = std::function<void(const String& json)>;

// Enkel global console/logger för web-UI (buffer + sink)
class UIConsole {
public:
  // Initiera (kan kallas en gång tidigt). maxLines default 200.
  static void init(size_t maxLines = 200);

  // Logga ett meddelande (kan kallas från var som helst i firmware)
  // source kan vara nullptr eller t.ex. "MT8816"
  static void log(const String& text, const char* source = nullptr);

  // Registrera en sink (t.ex. WebServer) som hanterar JSON-strängar.
  // När en sink registreras flushas buffern (om någon finns).
  static void setSink(ConsoleSink sink);

  // Ta bort sink (dvs. återgå till buffring)
  static void clearSink();
};

} // namespace util
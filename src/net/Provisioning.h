#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiProv.h>
#include "net/WifiClient.h"
#include "net/WebServer.h"

namespace net {

class Provisioning {
public:
  // OBS: passera in en referens till din WifiClient
  void begin(WifiClient& wifiClient, const char* hostname = "phoneexchange");

  // Rensa Wi-Fi-uppgifter i NVS och boota om (för att starta omprovisionering)
  static void factoryReset();

private:

  static void onSysEvent_(arduino_event_t *sys_event);

  static inline WifiClient* wifi_ = nullptr;  // pekare till din WifiClient
  static inline String host_;
  static inline bool   startedProvisioning_ = false;
};

} // namespace net

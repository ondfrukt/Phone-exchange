#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiProv.h>
#include <Preferences.h>
#include "net/WifiClient.h"
#include "net/WebServer.h"
#include "util/UIConsole.h"

namespace net {

class Provisioning {
public:
  // OBS: passera in en referens till din WifiClient
  void begin(WifiClient& wifiClient, const char* hostname = "phoneexchange");
  void loop();

  // Rensa Wi-Fi-uppgifter i NVS och boota om (för att starta omprovisionering)
  static void factoryReset();

private:
  static void stopProvisioning_();

  static void onSysEvent_(arduino_event_t *sys_event);

  static inline WifiClient* wifi_ = nullptr;  // pekare till din WifiClient
  static inline String host_;
  static inline bool   startedProvisioning_ = false;
  static inline unsigned long provisioningStartedAtMs_ = 0;
  static inline unsigned long credFailAtMs_ = 0;
  static inline String pendingSsid_;
  static inline String pendingPassword_;
  static inline bool credentialsCommitted_ = false;
  static inline bool resetStatePending_ = false;
  static inline bool provisioningTriedThisBoot_ = false;
  static constexpr unsigned long kResetStateDelayMs = 300;
  static constexpr unsigned long kProvisioningWindowMs = 5UL * 60UL * 1000UL;
};

} // namespace net

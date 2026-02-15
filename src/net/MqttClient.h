#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "settings/settings.h"
#include "net/WifiClient.h"
#include "model/Types.h"

class LineManager;

namespace net {

class MqttClient {
public:
  MqttClient(Settings& settings, net::WifiClient& wifi, LineManager& lineManager);

  void begin();
  void loop();
  void reconfigureFromSettings();
  bool isConnected() { return mqtt_.connected(); }

  void publishLineStatus(int lineIndex);
  void publishFullSnapshot();

private:
  Settings& settings_;
  net::WifiClient& wifi_;
  LineManager& lineManager_;

  ::WiFiClient tcpClient_;
  PubSubClient mqtt_;

  String host_;
  uint16_t port_ = 1883;
  String baseTopic_;
  String clientId_;
  String user_;
  String pass_;

  unsigned long reconnectDelayMs_ = 0;
  unsigned long lastConnectAttemptMs_ = 0;
  bool registeredCallbacks_ = false;
  int lastBlockedReason_ = -1;
  bool wasConnected_ = false;
  uint8_t failedConnectAttempts_ = 0;

  static constexpr unsigned long kInitialReconnectDelayMs = 3000;
  static constexpr unsigned long kMaxReconnectDelayMs = 60000;
  static constexpr unsigned long kLongRetryDelayMs = 10UL * 60UL * 1000UL; // 10 minutes
  static constexpr uint8_t kQuickRetryAttempts = 3;

  void loadConfig_();
  bool shouldRun_() const;
  int blockedReason_() const;
  bool connect_();
  void disconnect_();
  String makeTopic_(const char* suffix) const;
  const char* stateToText_(int state) const;
};

} // namespace net

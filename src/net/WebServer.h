#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "net/WifiClient.h"
#include "settings/settings.h"

namespace net { class WifiClient; } 

class LineManager;

class WebServer {
public:
  WebServer(Settings& settings, LineManager& lineManager, net::WifiClient& wifi, uint16_t port = 80);
  bool begin();
  void listFS();

  bool isReady() const { return serverStarted_ && fsMounted_; }

  // Publika hjälpmetoder om du vill kunna pusha manuellt
  void sendFullStatusSse();
  void sendActiveMaskSse();
  void sendDebugSse();

private:
  Settings& settings_;
  LineManager& lm_;
  AsyncWebServer server_;
  AsyncEventSource events_{"/events"};
  net::WifiClient& wifi_;

  bool serverStarted_ = false;
  bool fsMounted_ = false;

  TaskHandle_t pingTask_ = nullptr;
  
  void setupFilesystem_();
  void initSse_();
  void setupCallbacks_();
  void setupLineManagerCallback_();
  void setupApiRoutes_();
  void setLineActiveBit_(int line, bool makeActive);
  void restartDevice_();
  void toggleLineActiveBit_(int line);

  void pushInitialSnapshot_();

  void startPingLoop_();

  // Help functions
  String buildStatusJson_() const;
  String buildActiveJson_(uint8_t mask);
  String buildDebugJson_() const;
};

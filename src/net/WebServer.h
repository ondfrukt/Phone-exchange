#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "settings/settings.h"

class LineManager;

class WebServer {
public:
  WebServer(Settings& settings, LineManager& lineManager, uint16_t port = 80);
  
  bool begin();
  void listFS();

  bool isReady() const { return serverStarted_ && fsMounted_; }

  // Publika hjälpmetoder om du vill kunna pusha manuellt
  void sendFullStatusSse();
  void sendActiveMaskSse();

private:
  LineManager& lm_;
  Settings& settings_;
  AsyncWebServer server_;
  AsyncEventSource events_{"/events"};

  bool serverStarted_ = false;
  bool fsMounted_ = false;
  
  void setupFilesystem_();
  void initSse_();
  void setupCallbacks_();
  void setupLineManagerCallback_();
  void setupApiRoutes_();
  void setLineActiveBit_(int line, bool makeActive);

  void toggleLineActiveBit_(int line);

  void pushInitialSnapshot_();

  // Help functions
  String buildStatusJson_() const;
  String buildActiveJson_(uint8_t mask);
};

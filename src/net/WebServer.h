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
  bool isReady() const { return serverStarted_ && fsMounted_; }

  // Anropas vid behov (vi kallar den från callbacken i LineManager)
  void sendFullStatusSse();
  void listFS();

  void attachToLineManager();

private:
  LineManager& lm_;
  Settings& settings_;
  AsyncWebServer server_;
  AsyncEventSource events_{"/events"};
  bool fsMounted_ = false;
  bool serverStarted_ = false;

  void setupFilesystem_();
  void setupRoutes_();
  void attach(); // koppla till LineManager
  String buildStatusJson_() const; // använder StatusSerializer
};

#include "WebServer.h"
#include "util/StatusSerializer.h"
#include "services/LineManager.h"

WebServer::WebServer(Settings& settings, LineManager& lineManager, uint16_t port)
: settings_ (settings), lm_(lineManager), server_(port) {}

bool WebServer::begin() {
  setupFilesystem_();

  events_.onConnect([](AsyncEventSourceClient*){
    // valfri initial push
  });
  server_.addHandler(&events_);

  attach();
  setupRoutes_();

  server_.begin();
  serverStarted_ = true;
  return serverStarted_ && fsMounted_;
}

void WebServer::setupFilesystem_() {
  fsMounted_ = LittleFS.begin(true);
  if (!fsMounted_) Serial.println("[WebServer] LittleFS mount misslyckades.");
}

void WebServer::setupRoutes_() {
  if (fsMounted_) {
    server_.serveStatic("/", LittleFS, "/")
          .setDefaultFile("index.html")
          .setCacheControl("max-age=60");
  } else {
    server_.on("/", HTTP_GET, [](AsyncWebServerRequest *req){
      req->send(500, "text/plain; charset=utf-8", "LittleFS ej monterat. Kör 'pio run -t uploadfs'.");
    });
  }

  server_.on("/health", HTTP_GET, [](AsyncWebServerRequest *req){
    req->send(200, "text/plain", "ok");
  });

  server_.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *req){
    req->send(200, "application/json", buildStatusJson_());
  });

  server_.onNotFound([](AsyncWebServerRequest *req){
    req->send(404, "text/plain; charset=utf-8", "404 Not Found");
  });
}

String WebServer::buildStatusJson_() const {
  return net::buildLinesStatusJson(lm_);
}

void WebServer::sendFullStatusSse() {
  const String json = buildStatusJson_();
  events_.send(json.c_str(), nullptr, millis(), 3000);
}

void WebServer::listFS() {
  File root = LittleFS.open("/");
  for (File f = root.openNextFile(); f; f = root.openNextFile()) {
    Serial.printf("WebbServer: File: %s (%u bytes) found\n", f.name(), (unsigned)f.size());
  }
}

void WebServer::attachToLineManager() {
  lm_.setStatusChangedCallback([this](int index, LineStatus s){
    // Bygg en JSON för den linje som ändrades
    String json = "{\"line\":" + String(index) +
                  ",\"status\":\"" + model::toString(s) + "\"}";
    events_.send(json.c_str(), "lineStatus", millis());
  });
}

void WebServer::attach() {
  lm_.setStatusChangedCallback([this](int index, LineStatus s){
    String json = "{\"line\":" + String(index) +
                  ",\"status\":\"" + model::toString(s) + "\"}";
    events_.send(json.c_str(), "lineStatus", millis());
  });
}
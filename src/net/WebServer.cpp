#include "WebServer.h"
#include "util/StatusSerializer.h"
#include "services/LineManager.h"

WebServer::WebServer(Settings& settings, LineManager& lineManager, uint16_t port)
: settings_ (settings), lm_(lineManager), server_(port) {}

bool WebServer::begin() {

  setupFilesystem_();
  initSse_();
  setupCallbacks_();
  setupApiRoutes_();
  pushInitialSnapshot_();


  server_.begin();
  serverStarted_ = true;
  return serverStarted_ && fsMounted_;
}

void WebServer::listFS() {
  File root = LittleFS.open("/");
  for (File f = root.openNextFile(); f; f = root.openNextFile()) {
    Serial.printf("WebServer: File: %s (%u bytes)\n", f.name(), (unsigned)f.size());
  }
}


// --- Private ---

void WebServer::setupFilesystem_() {
  fsMounted_ = LittleFS.begin(true);
  if (!fsMounted_) Serial.println("[WebServer] LittleFS mount misslyckades.");
}

void WebServer::initSse_() {
  events_.onConnect([this](AsyncEventSourceClient* client){
    Serial.println("WebServer: SSE-klient ansluten 📡");
    // Skicka initial data till den nya klienten
    client->send(buildStatusJson_().c_str(), "lineStatus", millis());
    client->send(buildActiveJson_(settings_.activeLinesMask).c_str(), "activeMask", millis());
  });
  server_.addHandler(&events_);
}

void WebServer::setupCallbacks_() {
  setupLineManagerCallback_();
  // här kan du lägga till fler i framtiden
}

void WebServer::setupLineManagerCallback_() {
  lm_.setStatusChangedCallback([this](int index, LineStatus status){
    String json = "{\"line\":" + String(index) +
                  ",\"status\":\"" + model::toString(status) + "\"}";
    events_.send(json.c_str(), "lineStatus", millis());
  });
}

void WebServer::setupApiRoutes_() {
  // Health check
  server_.on("/health", HTTP_GET, [](AsyncWebServerRequest *req){
    req->send(200, "text/plain", "ok");
  });

  // Full status som JSON
  server_.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *req){
    req->send(200, "application/json", buildStatusJson_());
  });

  // Aktiva linjer som JSON
  server_.on("/api/active", HTTP_GET, [this](AsyncWebServerRequest *req){
    req->send(200, "application/json", buildActiveJson_(settings_.activeLinesMask));
  });


  // Toggle: POST /api/active/toggle  (body: line=3)
  server_.on("/api/active/toggle", HTTP_POST, [this](AsyncWebServerRequest* req){
    int line = -1;

    // 1) försök läsa från URL (?line=3)
    if (req->hasParam("line")) {
      line = req->getParam("line")->value().toInt();
    }
    // 2) annars försök läsa från POST-body (Content-Type: application/x-www-form-urlencoded)
    else if (req->hasParam("line", /*post=*/true)) {
      line = req->getParam("line", /*post=*/true)->value().toInt();
    }

    if (line < 0 || line > 7) {
      req->send(400, "application/json", "{\"error\":\"missing/invalid line\"}");
      return;
    }

    Serial.printf("[API] toggle line=%d\n", line);
    toggleLineActiveBit_(line);
    req->send(200, "application/json", buildActiveJson_(settings_.activeLinesMask));
  });

  // Set: POST /api/active/set  (body: line=3&active=1)
  server_.on("/api/active/set", HTTP_POST, [this](AsyncWebServerRequest* req){
    int line = -1, active = -1;

    if (req->hasParam("line")) {
      line = req->getParam("line")->value().toInt();
    } else if (req->hasParam("line", true)) {
      line = req->getParam("line", true)->value().toInt();
    }

    if (req->hasParam("active")) {
      active = req->getParam("active")->value().toInt();
    } else if (req->hasParam("active", true)) {
      active = req->getParam("active", true)->value().toInt();
    }

    if (line < 0 || line > 7 || (active != 0 && active != 1)) {
      req->send(400, "application/json", "{\"error\":\"missing/invalid line/active\"}");
      return;
    }

    Serial.printf("[API] set line=%d active=%d\n", line, active);
    setLineActiveBit_(line, active != 0);
    req->send(200, "application/json", buildActiveJson_(settings_.activeLinesMask));
  });


  // Statisk filserver
  if (fsMounted_) {
    server_.serveStatic("/", LittleFS, "/")
           .setDefaultFile("index.html")
           .setCacheControl("max-age=60");
  } else {
    server_.on("/", HTTP_GET, [](AsyncWebServerRequest *req){
      req->send(500, "text/plain; charset=utf-8", "LittleFS ej monterat. Kör 'pio run -t uploadfs'.");
    });
  }

  // Fallback 404
  server_.onNotFound([](AsyncWebServerRequest *req){
    req->send(404, "text/plain; charset=utf-8", "404 Not Found");
  });
}

void WebServer::setLineActiveBit_(int line, bool makeActive) {
  if (line < 0 || line > 7) return;

  uint8_t before = settings_.activeLinesMask;
  if (makeActive) settings_.activeLinesMask |=  (1u << line);
  else            settings_.activeLinesMask &= ~(1u << line);

  // Respektera tillåtna linjer (SLIC1/SLIC2 närvaro)
  settings_.adjustActiveLines();

  // Spara till NVS
  //settings_.save();

  // Spegla in masken i LineManager direkt (så UI och logik följer med)
  lm_.update();

  // Skicka ut ny mask till alla via SSE
  sendActiveMaskSse();

  if (settings_.debugWSLevel <= 1) {
    Serial.printf("WebServer: ActiveMask: 0x%02X -> 0x%02X\n", before, settings_.activeLinesMask);
  }
}

void WebServer::toggleLineActiveBit_(int line) {
  if (line < 0 || line > 7) return;
  bool wasActive = (settings_.activeLinesMask >> line) & 0x01;
  setLineActiveBit_(line, !wasActive);
}

void WebServer::pushInitialSnapshot_() {
  sendFullStatusSse();
  sendActiveMaskSse();
}


// --- Help functions ---

String WebServer::buildStatusJson_() const {
  return net::buildLinesStatusJson(lm_);
}

String WebServer::buildActiveJson_(uint8_t mask) {
  String json = "{\"mask\":" + String(mask) + ",\"active\":[";
  bool first = true;
  for (int i = 0; i < 8; ++i) {
    if (mask & (1u << i)) {
      if (!first) json += ",";
      json += String(i);
      first = false;
    }
  }
  json += "]}";
  return json;
}

void WebServer::sendFullStatusSse() {
  const String json = buildStatusJson_();
  events_.send(json.c_str(), "lineStatus", millis());

  Serial.println(settings_.debugWSLevel);
  if (settings_.debugWSLevel >= 1) {
    Serial.println("[WebServer] Full status skickad via SSE");
  }
}

void WebServer::sendActiveMaskSse() {
  const String json = buildActiveJson_(settings_.activeLinesMask);
  events_.send(json.c_str(), "activeMask", millis());
  if (settings_.debugWSLevel >= 1) {
    Serial.println("[WebServer] Active mask skickad via SSE");
  }
}
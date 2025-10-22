#include "WebServer.h"
#include "util/StatusSerializer.h"
#include "services/LineManager.h"
#include "util/UIConsole.h"

WebServer::WebServer(Settings& settings, LineManager& lineManager, net::WifiClient& wifi ,uint16_t port)
: settings_ (settings), lm_(lineManager), wifi_(wifi), server_(port) {}

bool WebServer::begin() {

  setupFilesystem_();
  // Initialize Console buffering (safe to call even if already init'd)
  util::UIConsole::init(200);

  initSse_();
  setupCallbacks_();
  setupApiRoutes_();
  pushInitialSnapshot_();

  if (settings_.debugWSLevel >= 1) {
    listFS();
  }

  server_.begin();
  serverStarted_ = true;

  // Bind console sink after server is started so messages are forwarded to SSE
  bindConsoleSink_();

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
  if (!fsMounted_) Serial.println("WebServer: LittleFS mount misslyckades.");
}

void WebServer::initSse_() {
  events_.onConnect([this](AsyncEventSourceClient* client){
    Serial.println("WebServer: SSE-klient ansluten 📡");
    // Skicka initial data till den nya klienten
    client->send(buildStatusJson_().c_str(), nullptr, millis());
    client->send(buildActiveJson_(settings_.activeLinesMask).c_str(), "activeMask", millis());
    client->send(buildDebugJson_().c_str(), "debug", millis());
    util::UIConsole::forEachBuffered([client](const String& json) {
      client->send(json.c_str(), "console", millis());
    });
  });
  server_.addHandler(&events_);
}

void WebServer::bindConsoleSink_() {
  // Register the sink that forwards Console JSON strings to SSE "console"
  util::UIConsole::setSink([this](const String& json) {
    // Forward the ready-made JSON to SSE clients
    events_.send(json.c_str(), "console", millis());
    if (settings_.debugWSLevel >= 2) {
      Serial.println("WebServer: forwarded console message to SSE");
    }
  });
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

    if (req->hasParam("line")) {
      line = req->getParam("line")->value().toInt();
    } else if (req->hasParam("line", /*post=*/true)) {
      line = req->getParam("line", /*post=*/true)->value().toInt();
    }

    if (line < 0 || line > 7) {
      req->send(400, "application/json", "{\"error\":\"missing/invalid line\"}");
      return;
    }
    if (settings_.debugWSLevel >= 1) {
      Serial.printf("WebServer: API toggle line=%d\n", line);
    }
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

    Serial.printf("API: set line=%d active=%d\n", line, active);
    setLineActiveBit_(line, active != 0);
    req->send(200, "application/json", buildActiveJson_(settings_.activeLinesMask));
  });

  server_.on("/api/debug", HTTP_GET, [this](AsyncWebServerRequest *req){
    req->send(200, "application/json", buildDebugJson_());
  });

  server_.on("/api/debug/set", HTTP_POST, [this](AsyncWebServerRequest* req){
    auto getOptUChar = [req](const char* k, int& out) {
      out = -1;
      if (req->hasParam(k)) out = req->getParam(k)->value().toInt();
      else if (req->hasParam(k, true)) out = req->getParam(k, true)->value().toInt();
      return (out >= 0);
    };

    int shk=-1, lm=-1, ws=-1, la=-1, mt=-1;
    bool hasShk = getOptUChar("shk", shk);
    bool hasLm  = getOptUChar("lm",  lm);
    bool hasWs  = getOptUChar("ws",  ws);
    bool hasLa  = getOptUChar("la",  la);
    bool hasMt  = getOptUChar("mt",  mt);

    if (!hasShk && !hasLm && !hasWs && !hasLa && !hasMt) {
      req->send(400, "application/json", "{\"error\":\"provide at least one of shk|lm|ws|la|mt\"}");
      return;
    }

    auto inRange = [](int v){ return v>=0 && v<=2; };
    if ((hasShk && !inRange(shk)) || (hasLm && !inRange(lm)) || (hasWs && !inRange(ws)) || (hasLa && !inRange(la)) || (hasMt && !inRange(mt))) {
      req->send(400, "application/json", "{\"error\":\"values must be 0..2\"}");
      return;
    }

    // Uppdatera värden
    if (hasShk) settings_.debugSHKLevel = (uint8_t)shk;
    if (hasLm)  settings_.debugLmLevel  = (uint8_t)lm;
    if (hasWs)  settings_.debugWSLevel  = (uint8_t)ws;
    if (hasLa)  settings_.debugLALevel  = (uint8_t)la;
    if (hasMt)  settings_.debugMTLevel  = (uint8_t)mt;

    // Spara till NVS
    //settings_.save();

    // Skicka live-uppdatering till andra klienter
    sendDebugSse();

    // Svara med aktuella nivåer
    req->send(200, "application/json", buildDebugJson_());
  });

  server_.on("/api/info", HTTP_GET, [this](AsyncWebServerRequest* req){
    String hn = wifi_.getHostname();
    if (hn.isEmpty()) hn = "phoneexchange";

    String mac = wifi_.getMac();   // "AA:BB:CC:DD:EE:FF"
    String ip  = wifi_.getIp();

    String json = "{";
    json += "\"hostname\":\"" + hn + "\",";
    json += "\"ip\":\"" + ip + "\",";
    json += "\"mac\":\"" + mac + "\"";
    json += "}";

    req->send(200, "application/json", json);
  });

  // Restart: POST /api/restart
  server_.on("/api/restart", HTTP_POST, [this](AsyncWebServerRequest* req){
    // Svara först, sedan schemalägg omstart så HTTP-svaret hinner ut
    req->send(200, "application/json", "{\"ok\":true}");

    // Schemalägg omstart i separat FreeRTOS-task (icke-blockerande)
    xTaskCreate([](void* arg){
      auto self = static_cast<WebServer*>(arg);
      vTaskDelay(pdMS_TO_TICKS(1000)); // 1s grace
      self->restartDevice_();
      vTaskDelete(nullptr);
    }, "restartTask", 2048, this, 1, nullptr);
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
  lm_.syncLineActive(line);

  // Skicka ut ny mask till alla via SSE
  sendActiveMaskSse();

  if (settings_.debugWSLevel >= 1) {
    Serial.printf("WebServer: ActiveMask: 0x%02X -> 0x%02X\n", before, settings_.activeLinesMask);
  }
}

void WebServer::toggleLineActiveBit_(int line) {
  if (line < 0 || line > 7) return;
  bool wasActive = (settings_.activeLinesMask >> line) & 0x01;
  setLineActiveBit_(line, !wasActive);

  if (settings_.debugWSLevel >= 1) {
    Serial.printf("WebServer: Toggle line %d: %d -> %d\n", line, wasActive ? 1 : 0, wasActive ? 0 : 1);
  }

}

void WebServer::pushInitialSnapshot_() {
  sendFullStatusSse();
  sendActiveMaskSse();

  if (settings_.debugWSLevel >= 1) {
    Serial.println("WebServer: Initial snapshot skickad via SSE");
  }
}

void WebServer::restartDevice_() {
  Serial.println("WebServer: Startar om enheten");
  delay(3000);
  ESP.restart();
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

String WebServer::buildDebugJson_() const {
  String json = "{";
  json += "\"shk\":" + String(settings_.debugSHKLevel);
  json += ",\"lm\":" + String(settings_.debugLmLevel);
  json += ",\"ws\":" + String(settings_.debugWSLevel);
  json += ",\"la\":" + String(settings_.debugLALevel);
  json += ",\"mt\":" + String(settings_.debugMTLevel);
  json += "}";
  return json;
}

void WebServer::sendDebugSse() {
  const String json = buildDebugJson_();
  events_.send(json.c_str(), "debug", millis());
  if (settings_.debugWSLevel >= 1) {
    Serial.println("WebServer: Debug levels skickade via SSE");
  }
}

void WebServer::sendFullStatusSse() {
  const String json = buildStatusJson_();
  events_.send(json.c_str(), nullptr, millis());

  Serial.println(settings_.debugWSLevel);
  if (settings_.debugWSLevel >= 1) {
    Serial.println("WebServer: Full status skickad via SSE");
  }
}

void WebServer::sendActiveMaskSse() {
  const String json = buildActiveJson_(settings_.activeLinesMask);
  events_.send(json.c_str(), "activeMask", millis());
  if (settings_.debugWSLevel >= 1) {
    Serial.println("WebServer: Active mask skickad via SSE");
  }
}

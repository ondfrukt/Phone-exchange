#include "WebServer.h"
#include "util/StatusSerializer.h"
#include "services/LineManager.h"
#include "services/RingGenerator.h"
#include "util/UIConsole.h"

namespace {
String escapeJson(const String& in) {
  String out;
  out.reserve(in.length());
  for (size_t i = 0; i < in.length(); ++i) {
    const char c = in.charAt(static_cast<unsigned int>(i));
    if (c == '\\' || c == '"') out += '\\';
    out += c;
  }
  return out;
}
} // namespace

WebServer::WebServer(Settings& settings, LineManager& lineManager, net::WifiClient& wifi, RingGenerator& ringGenerator, LineAction& lineAction, uint16_t port)
: settings_ (settings), lineManager_(lineManager), ringGenerator_(ringGenerator), lineAction_(lineAction), wifi_(wifi), server_(port) {}

bool WebServer::begin() {

  setupFilesystem_();
  Serial.printf("WebServer: LittleFS mount %s\n", fsMounted_ ? "OK" : "FAILED");
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
  Serial.printf("WebServer: HTTP server started on port %u\n", 80u);
  Serial.printf("WebServer: Current IP is %s\n", wifi_.getIp().c_str());

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
  // AsyncEventSource handles client management internally
  
  events_.onConnect([this](AsyncEventSourceClient* client){
    if (settings_.debugWSLevel >= 2) {
      Serial.printf("WebServer: SSE connected (open streams=%u)\n", (unsigned)events_.count());
      util::UIConsole::log("SSE connected (likely page load or page switch).", "WebServer");
    }
    // Skicka initial data till den nya klienten
    client->send(buildStatusJson_().c_str(), nullptr, millis());
    client->send(buildActiveJson_(settings_.activeLinesMask).c_str(), "activeMask", millis());
    client->send(buildDebugJson_().c_str(), "debug", millis());
    client->send(buildToneGeneratorJson_().c_str(), "toneGen", millis());
    util::UIConsole::forEachBuffered([client](const String& json) {
      client->send(json.c_str(), "console", millis());
    });
  });
  events_.onDisconnect([this](AsyncEventSourceClient*){
    if (settings_.debugWSLevel >= 2) {
      Serial.printf("WebServer: SSE disconnected (open streams=%u)\n", (unsigned)events_.count());
    }
  });
  server_.addHandler(&events_);
}

void WebServer::bindConsoleSink_() {
  // Register the sink that forwards Console JSON strings to SSE "console"
  util::UIConsole::setSink([this](const String& json) {
    // Forward the ready-made JSON to SSE clients only if there are connected clients
    if (events_.count() > 0) {
      events_.send(json.c_str(), "console", millis());
    }
    if (settings_.debugWSLevel >= 2) {
      Serial.println("WebServer: forwarded console message to SSE");
      // NOTE: Do NOT call util::UIConsole::log() here as it would cause infinite recursion
    }
  });
}

void WebServer::setupCallbacks_() {
  setupLineManagerCallback_();
  // här kan du lägga till fler i framtiden
}

void WebServer::setupLineManagerCallback_() {
  lineManager_.addStatusChangedCallback([this](int index, LineStatus status){
    // Only send SSE if there are connected clients
    if (events_.count() > 0) {
      String json = "{\"line\":" + String(index) +
                    ",\"status\":\"" + model::LineStatusToString(status) + "\"}";
      events_.send(json.c_str(), "lineStatus", millis());
    }
  });
}


// API routes. Routes which provide JSON data or accept commands.
void WebServer::setupApiRoutes_() {

  // Simple health check endpoint
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
  // Set phone number: POST /api/line/phone  (body: line=3&phone=123456789)
  server_.on("/api/line/phone", HTTP_POST, [this](AsyncWebServerRequest* req){
    const AsyncWebParameter* lineParam = nullptr;
    const AsyncWebParameter* phoneParam = nullptr;

    if (req->hasParam("line", true)) {
      lineParam = req->getParam("line", true);
    } else if (req->hasParam("line")) {
      lineParam = req->getParam("line");
    }

    if (req->hasParam("phone", true)) {
      phoneParam = req->getParam("phone", true);
    } else if (req->hasParam("phone")) {
      phoneParam = req->getParam("phone");
    }

    if (!lineParam || !phoneParam) {
      req->send(400, "application/json", "{\"error\":\"missing line/phone\"}");
      return;
    }

    int line = lineParam->value().toInt();
    if (line < 0 || line > 7) {
      req->send(400, "application/json", "{\"error\":\"invalid line\"}");
      return;
    }

    String value = phoneParam->value();
    value.trim();
    if (value.length() > 32) {
      req->send(400, "application/json", "{\"error\":\"phone too long\"}");
      return;
    }

    bool hasControlChars = false;
    for (size_t i = 0; i < static_cast<size_t>(value.length()); ++i) {
      char c = value.charAt(static_cast<unsigned int>(i));
      if (static_cast<unsigned char>(c) < 32u) {
        hasControlChars = true;
        break;
      }
    }

    if (hasControlChars) {
      req->send(400, "application/json", "{\"error\":\"invalid characters\"}");
      return;
    }

    if (value.length() > 0) {
      for (int i = 0; i < 8; ++i) {
        if (i == line) continue;
        String existing = settings_.linePhoneNumbers[i];
        existing.trim();
        if (existing.isEmpty()) continue;
        if (existing == value) {
          req->send(409, "application/json", "{\"error\":\"phone already in use\"}");
          return;
        }
      }
    }

    lineManager_.setPhoneNumber(line, value);
    settings_.save();

    sendFullStatusSse();

    if (settings_.debugWSLevel >= 1) {
      Serial.printf("WebServer: Phone number line %d set to %s\n", line, value.c_str());
      util::UIConsole::log("Phone number for line " + String(line) + " updated", "WebServer");
    }

    req->send(200, "application/json", "{\"ok\":true}");
  });
  // Set line name: POST /api/line/name (body: line=3&name=Kitchen)
  server_.on("/api/line/name", HTTP_POST, [this](AsyncWebServerRequest* req){
    const AsyncWebParameter* lineParam = nullptr;
    const AsyncWebParameter* nameParam = nullptr;

    if (req->hasParam("line", true)) {
      lineParam = req->getParam("line", true);
    } else if (req->hasParam("line")) {
      lineParam = req->getParam("line");
    }

    if (req->hasParam("name", true)) {
      nameParam = req->getParam("name", true);
    } else if (req->hasParam("name")) {
      nameParam = req->getParam("name");
    }

    if (!lineParam || !nameParam) {
      req->send(400, "application/json", "{\"error\":\"missing line/name\"}");
      return;
    }

    int line = lineParam->value().toInt();
    if (line < 0 || line > 7) {
      req->send(400, "application/json", "{\"error\":\"invalid line\"}");
      return;
    }

    String value = nameParam->value();
    value.trim();
    if (value.length() > 32) {
      req->send(400, "application/json", "{\"error\":\"name too long\"}");
      return;
    }

    bool hasControlChars = false;
    for (size_t i = 0; i < static_cast<size_t>(value.length()); ++i) {
      char c = value.charAt(static_cast<unsigned int>(i));
      if (static_cast<unsigned char>(c) < 32u) {
        hasControlChars = true;
        break;
      }
    }

    if (hasControlChars) {
      req->send(400, "application/json", "{\"error\":\"invalid characters\"}");
      return;
    }

    lineManager_.setLineName(line, value);
    settings_.save();

    sendFullStatusSse();

    if (settings_.debugWSLevel >= 1) {
      Serial.printf("WebServer: Line %d name set to %s\n", line, value.c_str());
      util::UIConsole::log("Name for line " + String(line) + " updated", "WebServer");
    }

    req->send(200, "application/json", "{\"ok\":true}");
  });
  // Debug nivåer: GET /api/debug , POST /api/debug/set
  server_.on("/api/debug", HTTP_GET, [this](AsyncWebServerRequest *req){
    req->send(200, "application/json", buildDebugJson_());
  });
  // Set debug nivåer: POST /api/debug/set  (body: shk=1&lm=2&ws=0&la=1&mt=2&tr=0&tg=1&rg=2&mcp=1&i2c=0&im=1)
  server_.on("/api/debug/set", HTTP_POST, [this](AsyncWebServerRequest* req){
    auto getOptUChar = [req](const char* k, int& out) {
      out = -1;
      if (req->hasParam(k)) out = req->getParam(k)->value().toInt();
      else if (req->hasParam(k, true)) out = req->getParam(k, true)->value().toInt();
      return (out >= 0);
    };

    int shk=-1, lm=-1, ws=-1, la=-1, mt=-1, tr=-1, tg=-1, rg=-1, mcp=-1, i2c=-1, im=-1;
    bool hasShk = getOptUChar("shk", shk);
    bool hasLm  = getOptUChar("lm",  lm);
    bool hasWs  = getOptUChar("ws",  ws);
    bool hasLa  = getOptUChar("la",  la);
    bool hasMt  = getOptUChar("mt",  mt);
    bool hasTr  = getOptUChar("tr",  tr);
    bool hasTg  = getOptUChar("tg",  tg);
    bool hasRg  = getOptUChar("rg",  rg);
    bool hasMcp = getOptUChar("mcp", mcp);
    bool hasI2c = getOptUChar("i2c", i2c);
    bool hasIm  = getOptUChar("im",  im);

    if (!hasShk && !hasLm && !hasWs && !hasLa && !hasMt && !hasTr && !hasTg && !hasRg && !hasMcp && !hasI2c && !hasIm) {
      req->send(400, "application/json", "{\"error\":\"provide at least one of shk|lm|ws|la|mt|tr|tg|rg|mcp|i2c|im\"}");
      return;
    }

    auto inRange = [](int v){ return v>=0 && v<=2; };
    if ((hasShk && !inRange(shk)) || (hasLm && !inRange(lm)) || (hasWs && !inRange(ws)) || (hasLa && !inRange(la)) || (hasMt && !inRange(mt)) || (hasTr && !inRange(tr)) || (hasTg && !inRange(tg)) || (hasRg && !inRange(rg)) || (hasMcp && !inRange(mcp)) || (hasI2c && !inRange(i2c)) || (hasIm && !inRange(im))) {
      req->send(400, "application/json", "{\"error\":\"values must be 0..2\"}");
      return;
    }

    // Uppdatera värden
    if (hasShk) settings_.debugSHKLevel = (uint8_t)shk;
    if (hasLm)  settings_.debugLmLevel  = (uint8_t)lm;
    if (hasWs)  settings_.debugWSLevel  = (uint8_t)ws;
    if (hasLa)  settings_.debugLALevel  = (uint8_t)la;
    if (hasMt)  settings_.debugMTLevel  = (uint8_t)mt;
    if (hasTr)  settings_.debugTRLevel  = (uint8_t)tr;
    if (hasTg)  settings_.debugTonGenLevel  = (uint8_t)tg;
    if (hasRg)  settings_.debugRGLevel  = (uint8_t)rg;
    if (hasMcp) settings_.debugMCPLevel = (uint8_t)mcp;
    if (hasI2c) settings_.debugI2CLevel = (uint8_t)i2c;
    if (hasIm)  settings_.debugIMLevel  = (uint8_t)im;

    // Spara till NVS
    settings_.save();

    // Skicka live-uppdatering till andra klienter
    sendDebugSse();

    // Svara med aktuella nivåer
    req->send(200, "application/json", buildDebugJson_());
  });
  // Tone generator: GET /api/tone-generator , POST /api/tone-generator/set
  server_.on("/api/tone-generator", HTTP_GET, [this](AsyncWebServerRequest *req){
    req->send(200, "application/json", buildToneGeneratorJson_());
  });
  // Set tone generator: POST /api/tone-generator/set  (body: enabled=1)
  server_.on("/api/tone-generator/set", HTTP_POST, [this](AsyncWebServerRequest* req){
    int enabled = -1;

    if (req->hasParam("enabled")) enabled = req->getParam("enabled")->value().toInt();
    else if (req->hasParam("enabled", true)) enabled = req->getParam("enabled", true)->value().toInt();

    if (enabled != 0 && enabled != 1) {
      req->send(400, "application/json", "{\"error\":\"enabled must be 0 or 1\"}");
      return;
    }

    settings_.toneGeneratorEnabled = (enabled == 1);
    settings_.save();

    sendToneGeneratorSse();
    req->send(200, "application/json", buildToneGeneratorJson_());
  });
  // Enhetsinfo: GET /api/info
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
  // WiFi Reset: POST /api/wifi/reset
  server_.on("/api/wifi/reset", HTTP_POST, [this](AsyncWebServerRequest* req){
    req->send(200, "application/json", "{\"ok\":true,\"message\":\"WiFi settings will be erased and device will restart\"}");
    
    if (settings_.debugWSLevel >= 1) {
      Serial.println("WebServer: WiFi factory reset requested");
      util::UIConsole::log("WiFi factory reset requested - erasing credentials", "WebServer");
    }

    // Schemalägg WiFi-reset i separat task så HTTP-svaret hinner ut
    xTaskCreate([](void* arg){
      vTaskDelay(pdMS_TO_TICKS(1000)); // 1s grace för att låta svaret skickas
      net::Provisioning::factoryReset(); // Raderar WiFi-creds och startar om
      vTaskDelete(nullptr);
    }, "wifiResetTask", 2048, nullptr, 1, nullptr);
  });
  // Ring test: POST /api/ring/test (body: line=3)
  server_.on("/api/ring/test", HTTP_POST, [this](AsyncWebServerRequest* req){

    int line = -1;
    if (req->hasParam("line")) {
      line = req->getParam("line")->value().toInt();
    } else if (req->hasParam("line", true)) {
      line = req->getParam("line", true)->value().toInt();
    }

    if (line < 0 || line > 7) {
      req->send(400, "application/json", "{\"error\":\"invalid line\"}");
      return;
    }

    if (!settings_.isLineActive(line)) {
      req->send(400, "application/json", "{\"error\":\"line not active\"}");
      return;
    }

    lineManager_.setStatus(line, LineStatus::Incoming);
    req->send(200, "application/json", "{\"ok\":true}");

    if (settings_.debugWSLevel >= 1) {
      Serial.printf("WebServer: Ring test started on line %d\n", line);
      util::UIConsole::log("Ring test started on line " + String(line), "WebServer");
    }
  });
  // Stop ring: POST /api/ring/stop
  server_.on("/api/ring/stop", HTTP_POST, [this](AsyncWebServerRequest* req){

    int line = -1;
    if (req->hasParam("line")) {
      line = req->getParam("line")->value().toInt();
    } else if (req->hasParam("line", true)) {
      line = req->getParam("line", true)->value().toInt();
    }

    if (line < 0 || line > 7) {
      req->send(400, "application/json", "{\"error\":\"invalid line\"}");
      return;
    }

    if (!settings_.isLineActive(line)) {
      req->send(400, "application/json", "{\"error\":\"line not active\"}");
      return;
    }

    lineManager_.setStatus(line, LineStatus::Idle);
    req->send(200, "application/json", "{\"ok\":true}");

    if (settings_.debugWSLevel >= 1) {
      Serial.println("WebServer: Ring stopped");
      util::UIConsole::log("Ring stopped", "WebServer");
    }
  });
  // Get ring settings: GET /api/settings/ring
  server_.on("/api/settings/ring", HTTP_GET, [this](AsyncWebServerRequest* req){
    String json = "{";
    json += "\"ringLengthMs\":" + String(settings_.ringLengthMs) + ",";
    json += "\"ringPauseMs\":" + String(settings_.ringPauseMs) + ",";
    json += "\"ringIterations\":" + String(settings_.ringIterations);
    json += "}";
    req->send(200, "application/json", json);
  });
  // Set ring settings: POST /api/settings/ring
  server_.on("/api/settings/ring", HTTP_POST, [this](AsyncWebServerRequest* req){
    auto getParam = [req](const char* key) -> int {
      if (req->hasParam(key)) return req->getParam(key)->value().toInt();
      if (req->hasParam(key, true)) return req->getParam(key, true)->value().toInt();
      return -1;
    };

    int ringLength = getParam("ringLengthMs");
    int ringPause = getParam("ringPauseMs");
    int ringIter = getParam("ringIterations");

    bool updated = false;
    if (ringLength >= 100 && ringLength <= 10000) {
      settings_.ringLengthMs = ringLength;
      updated = true;
    }
    if (ringPause >= 100 && ringPause <= 10000) {
      settings_.ringPauseMs = ringPause;
      updated = true;
    }
    if (ringIter >= 1 && ringIter <= 10) {
      settings_.ringIterations = ringIter;
      updated = true;
    }

    if (updated) {
      settings_.save();
      req->send(200, "application/json", "{\"ok\":true}");
      if (settings_.debugWSLevel >= 1) {
        Serial.println("WebServer: Ring settings updated");
        util::UIConsole::log("Ring settings updated", "WebServer");
      }
    } else {
      req->send(400, "application/json", "{\"error\":\"invalid parameters\"}");
    }
  });
  // Get SHK/ring-detection settings: GET /api/settings/shk
  server_.on("/api/settings/shk", HTTP_GET, [this](AsyncWebServerRequest* req){
    String json = "{";
    json += "\"burstTickMs\":" + String(settings_.burstTickMs) + ",";
    json += "\"hookStableMs\":" + String(settings_.hookStableMs) + ",";
    json += "\"hookStableConsec\":" + String(settings_.hookStableConsec);
    json += "}";
    req->send(200, "application/json", json);
  });
  // Set SHK/ring-detection settings: POST /api/settings/shk
  server_.on("/api/settings/shk", HTTP_POST, [this](AsyncWebServerRequest* req){
    auto getParam = [req](const char* key) -> int {
      if (req->hasParam(key)) return req->getParam(key)->value().toInt();
      if (req->hasParam(key, true)) return req->getParam(key, true)->value().toInt();
      return -1;
    };

    bool updated = false;
    int val;

    val = getParam("burstTickMs");
    if (val >= 1 && val <= 100) { settings_.burstTickMs = (uint32_t)val; updated = true; }

    val = getParam("hookStableMs");
    if (val >= 10 && val <= 2000) { settings_.hookStableMs = (uint32_t)val; updated = true; }

    val = getParam("hookStableConsec");
    if (val >= 0 && val <= 100) { settings_.hookStableConsec = (uint8_t)val; updated = true; }

    if (updated) {
      settings_.save();
      req->send(200, "application/json", "{\"ok\":true}");
      if (settings_.debugWSLevel >= 1) {
        Serial.println("WebServer: SHK settings updated");
        util::UIConsole::log("SHK settings updated", "WebServer");
      }
    } else {
      req->send(400, "application/json", "{\"error\":\"invalid parameters\"}");
    }
  });
  // Get ToneReader settings: GET /api/settings/tone-reader
  server_.on("/api/settings/tone-reader", HTTP_GET, [this](AsyncWebServerRequest* req){
    String json = "{";
    json += "\"dtmfDebounceMs\":" + String(settings_.dtmfDebounceMs) + ",";
    json += "\"dtmfMinToneDurationMs\":" + String(settings_.dtmfMinToneDurationMs) + ",";
    json += "\"dtmfStdStableMs\":" + String(settings_.dtmfStdStableMs) + ",";
    json += "\"tmuxScanDwellMinMs\":" + String(settings_.tmuxScanDwellMinMs);
    json += "}";
    req->send(200, "application/json", json);
  });
  // Set ToneReader settings: POST /api/settings/tone-reader
  server_.on("/api/settings/tone-reader", HTTP_POST, [this](AsyncWebServerRequest* req){
    auto getParam = [req](const char* key) -> int {
      if (req->hasParam(key)) return req->getParam(key)->value().toInt();
      if (req->hasParam(key, true)) return req->getParam(key, true)->value().toInt();
      return -1;
    };

    bool updated = false;
    int val = -1;

    val = getParam("dtmfDebounceMs");
    if (val >= 20 && val <= 1000) { settings_.dtmfDebounceMs = (uint32_t)val; updated = true; }

    val = getParam("dtmfMinToneDurationMs");
    if (val >= 10 && val <= 200) { settings_.dtmfMinToneDurationMs = (uint32_t)val; updated = true; }

    val = getParam("dtmfStdStableMs");
    if (val >= 1 && val <= 100) { settings_.dtmfStdStableMs = (uint32_t)val; updated = true; }

    val = getParam("tmuxScanDwellMinMs");
    if (val >= 1 && val <= 500) { settings_.tmuxScanDwellMinMs = (uint32_t)val; updated = true; }

    if (updated) {
      settings_.save();
      req->send(200, "application/json", "{\"ok\":true}");
      if (settings_.debugWSLevel >= 1) {
        Serial.println("WebServer: ToneReader settings updated");
        util::UIConsole::log("ToneReader settings updated", "WebServer");
      }
    } else {
      req->send(400, "application/json", "{\"error\":\"invalid parameters\"}");
    }
  });
  // Get timer settings: GET /api/settings/timers
  server_.on("/api/settings/timers", HTTP_GET, [this](AsyncWebServerRequest* req){
    String json = "{";
    json += "\"timer_Ready\":" + String(settings_.timer_Ready) + ",";
    json += "\"timer_Dialing\":" + String(settings_.timer_Dialing) + ",";
    json += "\"timer_Ringing\":" + String(settings_.timer_Ringing) + ",";
    json += "\"timer_pulsDialing\":" + String(settings_.timer_pulsDialing) + ",";
    json += "\"timer_toneDialing\":" + String(settings_.timer_toneDialing) + ",";
    json += "\"timer_fail\":" + String(settings_.timer_fail) + ",";
    json += "\"timer_disconnected\":" + String(settings_.timer_disconnected) + ",";
    json += "\"timer_timeout\":" + String(settings_.timer_timeout) + ",";
    json += "\"timer_busy\":" + String(settings_.timer_busy);
    json += "}";
    req->send(200, "application/json", json);
  });
  // Set timer settings: POST /api/settings/timers
  server_.on("/api/settings/timers", HTTP_POST, [this](AsyncWebServerRequest* req){
    auto getParam = [req](const char* key) -> int {
      if (req->hasParam(key)) return req->getParam(key)->value().toInt();
      if (req->hasParam(key, true)) return req->getParam(key, true)->value().toInt();
      return -1;
    };

    bool updated = false;
    int val;

    val = getParam("timer_Ready");
    if (val >= 1000 && val <= 600000) { settings_.timer_Ready = val; updated = true; }

    val = getParam("timer_Dialing");
    if (val >= 1000 && val <= 60000) { settings_.timer_Dialing = val; updated = true; }

    val = getParam("timer_Ringing");
    if (val >= 1000 && val <= 60000) { settings_.timer_Ringing = val; updated = true; }

    val = getParam("timer_pulsDialing");
    if (val >= 1000 && val <= 60000) { settings_.timer_pulsDialing = val; updated = true; }

    val = getParam("timer_toneDialing");
    if (val >= 1000 && val <= 60000) { settings_.timer_toneDialing = val; updated = true; }

    val = getParam("timer_fail");
    if (val >= 1000 && val <= 120000) { settings_.timer_fail = val; updated = true; }

    val = getParam("timer_disconnected");
    if (val >= 1000 && val <= 120000) { settings_.timer_disconnected = val; updated = true; }

    val = getParam("timer_timeout");
    if (val >= 1000 && val <= 120000) { settings_.timer_timeout = val; updated = true; }

    val = getParam("timer_busy");
    if (val >= 1000 && val <= 120000) { settings_.timer_busy = val; updated = true; }

    if (updated) {
      settings_.save();
      req->send(200, "application/json", "{\"ok\":true}");
      if (settings_.debugWSLevel >= 1) {
        Serial.println("WebServer: Timer settings updated");
        util::UIConsole::log("Timer settings updated", "WebServer");
      }
    } else {
      req->send(400, "application/json", "{\"error\":\"invalid parameters\"}");
    }
  });

  // Get MQTT settings: GET /api/settings/mqtt
  server_.on("/api/settings/mqtt", HTTP_GET, [this](AsyncWebServerRequest* req){
    req->send(200, "application/json", buildMqttJson_());
  });
  // Set MQTT settings: POST /api/settings/mqtt
  server_.on("/api/settings/mqtt", HTTP_POST, [this](AsyncWebServerRequest* req){
    auto getStr = [req](const char* key, String& out) -> bool {
      if (req->hasParam(key)) {
        out = req->getParam(key)->value();
        return true;
      }
      if (req->hasParam(key, true)) {
        out = req->getParam(key, true)->value();
        return true;
      }
      return false;
    };

    auto getInt = [req](const char* key, int& out) -> bool {
      if (req->hasParam(key)) {
        out = req->getParam(key)->value().toInt();
        return true;
      }
      if (req->hasParam(key, true)) {
        out = req->getParam(key, true)->value().toInt();
        return true;
      }
      return false;
    };

    String host;
    String user;
    String pass;
    String clientId;
    String baseTopic;
    int enabled = settings_.mqttEnabled ? 1 : 0;
    int retain = settings_.mqttRetain ? 1 : 0;
    int port = settings_.mqttPort;
    int qos = settings_.mqttQos;

    getInt("enabled", enabled);
    getInt("retain", retain);
    getInt("port", port);
    getInt("qos", qos);
    getStr("host", host);
    getStr("username", user);
    getStr("password", pass);
    getStr("clientId", clientId);
    getStr("baseTopic", baseTopic);

    host.trim();
    user.trim();
    pass.trim();
    clientId.trim();
    baseTopic.trim();

    if (enabled != 0 && enabled != 1) {
      req->send(400, "application/json", "{\"error\":\"enabled must be 0 or 1\"}");
      return;
    }
    if (retain != 0 && retain != 1) {
      req->send(400, "application/json", "{\"error\":\"retain must be 0 or 1\"}");
      return;
    }
    if (port < 1 || port > 65535) {
      req->send(400, "application/json", "{\"error\":\"port must be 1..65535\"}");
      return;
    }
    if (qos < 0 || qos > 2) {
      req->send(400, "application/json", "{\"error\":\"qos must be 0..2\"}");
      return;
    }
    if (enabled == 1 && host.isEmpty()) {
      req->send(400, "application/json", "{\"error\":\"host is required when enabled\"}");
      return;
    }
    if (clientId.length() > 64 || baseTopic.length() > 128 || host.length() > 128 ||
        user.length() > 64 || pass.length() > 64) {
      req->send(400, "application/json", "{\"error\":\"one or more fields are too long\"}");
      return;
    }
    auto hasControl = [](const String& value) {
      for (size_t i = 0; i < static_cast<size_t>(value.length()); ++i) {
        char c = value.charAt(static_cast<unsigned int>(i));
        if (static_cast<unsigned char>(c) < 32u) return true;
      }
      return false;
    };
    if (hasControl(host) || hasControl(user) || hasControl(pass) || hasControl(clientId) || hasControl(baseTopic)) {
      req->send(400, "application/json", "{\"error\":\"invalid characters\"}");
      return;
    }

    settings_.mqttEnabled = (enabled == 1);
    settings_.mqttHost = host;
    settings_.mqttPort = static_cast<uint16_t>(port);
    settings_.mqttUsername = user;
    settings_.mqttPassword = pass;
    settings_.mqttClientId = clientId.length() ? clientId : "phoneexchange";
    settings_.mqttBaseTopic = baseTopic.length() ? baseTopic : "phoneexchange";
    settings_.mqttRetain = (retain == 1);
    settings_.mqttQos = static_cast<uint8_t>(qos);
    settings_.mqttConfigDirty = true;
    settings_.save();
    util::UIConsole::log("MQTT settings updated (reconfigure scheduled).", "WebServer");

    req->send(200, "application/json", buildMqttJson_());
  });

  // Statisk filserver
  if (fsMounted_) {
    server_.serveStatic("/", LittleFS, "/")
           .setDefaultFile("index.html")
           .setTryGzipFirst(false)
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
  settings_.save();

  // Spegla in masken i LineManager direkt (så UI och logik följer med)
  lineManager_.syncLineActive(line);

  // Skicka ut ny mask till alla via SSE
  sendActiveMaskSse();

  if (settings_.debugWSLevel >= 1) {
    Serial.printf("WebServer: ActiveMask: 0x%02X -> 0x%02X\n", before, settings_.activeLinesMask);
    util::UIConsole::log("ActiveMask: 0x" + String(before, HEX) + " -> 0x" + String(settings_.activeLinesMask, HEX), "WebServer");
  }
}

void WebServer::toggleLineActiveBit_(int line) {
  if (line < 0 || line > 7) return;
  bool wasActive = (settings_.activeLinesMask >> line) & 0x01;
  setLineActiveBit_(line, !wasActive);

  if (settings_.debugWSLevel >= 1) {
    Serial.printf("WebServer: Toggle line %d: %d -> %d\n", line, wasActive ? 1 : 0, wasActive ? 0 : 1);
    util::UIConsole::log("Toggle line " + String(line) + ": " + String(wasActive ? 1 : 0) + " -> " + String(wasActive ? 0 : 1), "WebServer");
  }

}

void WebServer::pushInitialSnapshot_() {
  sendFullStatusSse();
  sendActiveMaskSse();

  if (settings_.debugWSLevel >= 1) {
    Serial.println("WebServer: Initial snapshot sent via SSE");
    util::UIConsole::log("Initial snapshot sent via SSE", "WebServer");
  }
}

void WebServer::restartDevice_() {
  Serial.println("WebServer: Restarting device...");
  delay(50);
  util::UIConsole::log("Restarting device...", "WebServer");
  delay(1500);
  ESP.restart();
}

// --- Help functions ---

String WebServer::buildStatusJson_() const {
  return net::buildLinesStatusJson(lineManager_);
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
  json += ",\"tr\":" + String(settings_.debugTRLevel);
  json += ",\"tg\":" + String(settings_.debugTonGenLevel);
  json += ",\"rg\":" + String(settings_.debugRGLevel);
  json += ",\"mcp\":" + String(settings_.debugMCPLevel);
  json += ",\"i2c\":" + String(settings_.debugI2CLevel);
  json += ",\"im\":" + String(settings_.debugIMLevel);
  json += "}";
  return json;
}

void WebServer::sendDebugSse() {
  // Only send SSE if there are connected clients
  if (events_.count() > 0) {
    const String json = buildDebugJson_();
    events_.send(json.c_str(), "debug", millis());
    if (settings_.debugWSLevel >= 1) {
      Serial.println("WebServer: Debug levels skickade via SSE");
      util::UIConsole::log("Debug levels sent via SSE", "WebServer");
    }
  }
}

String WebServer::buildToneGeneratorJson_() const {
  String json = "{";
  json += "\"enabled\":";
  json += settings_.toneGeneratorEnabled ? "true" : "false";
  json += "}";
  return json;
}

String WebServer::buildMqttJson_() const {
  String json = "{";
  json += "\"enabled\":"; json += settings_.mqttEnabled ? "true" : "false";
  json += ",\"host\":\"" + escapeJson(settings_.mqttHost) + "\"";
  json += ",\"port\":" + String(settings_.mqttPort);
  json += ",\"username\":\"" + escapeJson(settings_.mqttUsername) + "\"";
  json += ",\"password\":\"" + escapeJson(settings_.mqttPassword) + "\"";
  json += ",\"clientId\":\"" + escapeJson(settings_.mqttClientId) + "\"";
  json += ",\"baseTopic\":\"" + escapeJson(settings_.mqttBaseTopic) + "\"";
  json += ",\"retain\":"; json += settings_.mqttRetain ? "true" : "false";
  json += ",\"qos\":" + String(settings_.mqttQos);
  json += "}";
  return json;
}

void WebServer::sendToneGeneratorSse() {
  // Only send SSE if there are connected clients
  if (events_.count() > 0) {
    const String json = buildToneGeneratorJson_();
    events_.send(json.c_str(), "toneGen", millis());
    if (settings_.debugWSLevel >= 1) {
      Serial.println("WebServer: Tone generator state skickad via SSE");
      util::UIConsole::log("Tone generator state sent via SSE", "WebServer");
    }
  }
}

void WebServer::sendFullStatusSse() {
  // Only send SSE if there are connected clients
  if (events_.count() > 0) {
    const String json = buildStatusJson_();
    events_.send(json.c_str(), nullptr, millis());

    if (settings_.debugWSLevel >= 1) {
      Serial.println("WebServer: Full status skickad via SSE");
      util::UIConsole::log("Full status sent via SSE", "WebServer");
    }
  }
}

void WebServer::sendActiveMaskSse() {
  // Only send SSE if there are connected clients
  if (events_.count() > 0) {
    const String json = buildActiveJson_(settings_.activeLinesMask);
    events_.send(json.c_str(), "activeMask", millis());
    if (settings_.debugWSLevel >= 1) {
      Serial.println("WebServer: Active mask skickad via SSE");
      util::UIConsole::log("Active mask sent via SSE", "WebServer");
    }
  }
}

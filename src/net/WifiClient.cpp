#include "WifiClient.h"
#include "settings/settings.h"
#include <time.h>
using namespace net;

WifiClient::WifiClient() : connecting_(false) {}

void WifiClient::begin(const char* hostname) {
  prefs_.begin("wifi", false);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);

  // 1) Choose hostname (either given, or auto based on MAC)
  if (hostname && strlen(hostname) > 0) {
    hostname_ = hostname;
  } else {
    hostname_ = makeDefaultHostname_(); // ex: phoneexchange-7A3F
  }

  // 2) Set hostname BEFORE WiFi.begin()
  if (!WiFi.setHostname(hostname_.c_str())) {
    if (Settings::instance().debugWSLevel >= 1) {
      Serial.println("WifiClient:         Failed to set hostname.");
      util::UIConsole::log("Failed to set hostname.", "WifiClient");
    }
  }

  // 3) Register event listener (before connect)
  WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info){ onWiFiEvent_(event, info); });

  // 4) Try to connect with saved credentials
  String savedSsid, savedPass;
  if (loadCredentials(savedSsid, savedPass)) {
    ssid_ = savedSsid;
    password_ = savedPass;
    connect_();
  } else {
    Serial.println("WifiClient:         No saved credentials, waiting for provisioning...");
    util::UIConsole::log("No saved credentials, waiting for provisioning...", "WifiClient");
  }
}

void WifiClient::loop() {
  const unsigned long now = millis();

  // If we are "connecting" for too long without a valid IP, force a reconnect.
  if (connecting_ && ((long)(now - lastConnectAttemptMs_) >= (long)kConnectAttemptTimeoutMs)) {
    if (Settings::instance().debugWSLevel >= 1) {
      Serial.println("WifiClient:         Connect timeout, forcing reconnect.");
      util::UIConsole::log("Connect timeout, forcing reconnect.", "WifiClient");
    }
    forceReconnect_();
  }

  // Automatic reconnect with backoff to avoid tight loop with wrong credentials.
  const bool timeToRetry =
      (reconnectDelayMs_ == 0) ||
      ((long)(now - lastConnectAttemptMs_) >= (long)reconnectDelayMs_);
  if (!isConnected() && !connecting_ && ssid_.length() > 0 && timeToRetry) {
    connect_();
  }
}

bool WifiClient::isConnected() const {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  const IPAddress ip = WiFi.localIP();
  return ip != IPAddress((uint32_t)0);
}

String WifiClient::getIp() const {
  if (isConnected()) return WiFi.localIP().toString();
  return String("0.0.0.0");
}

String WifiClient::getMac() const {
  return WiFi.macAddress();
}

void WifiClient::saveCredentials(const char* ssid, const char* password) {
  prefs_.putString("ssid", ssid);
  prefs_.putString("pass", password);
  ssid_ = ssid;
  password_ = password;
  Serial.println("WifiClient:         Credentials saved");
  util::UIConsole::log("Credentials saved", "WifiClient");
}

bool WifiClient::loadCredentials(String& ssid, String& password) {
  const bool hasSsid = prefs_.isKey("ssid");
  if (!hasSsid) {
    ssid = "";
    password = "";
    return false;
  }
  ssid = prefs_.getString("ssid", "");
  // Password is optional: if not stored, default to empty string (open networks).
  password = prefs_.getString("pass", "");
  return ssid.length() > 0;
}

void WifiClient::connect_() {
  if (connecting_ || WiFi.status() == WL_CONNECTED) {
    return;
  }
  Serial.printf("WifiClient:         Connecting to %s... (host=%s)\n", ssid_.c_str(), hostname_.c_str());
  util::UIConsole::log("Connecting to " + ssid_ + "...", "WifiClient");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_.c_str(), password_.c_str());
  lastConnectAttemptMs_ = millis();
  if (reconnectDelayMs_ == 0) {
    reconnectDelayMs_ = kInitialReconnectDelayMs;
  }
  connecting_ = true;
}

void WifiClient::forceReconnect_() {
  connecting_ = false;
  WiFi.disconnect(false, false);
  if (reconnectDelayMs_ == 0) {
    reconnectDelayMs_ = kInitialReconnectDelayMs;
  } else {
    reconnectDelayMs_ = min(reconnectDelayMs_ * 2, kMaxReconnectDelayMs);
  }
  lastConnectAttemptMs_ = millis();
}

void WifiClient::onWiFiEvent_(WiFiEvent_t event, const WiFiEventInfo_t& info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("WifiClient:         Connected to AP");
      util::UIConsole::log("Connected to AP", "WifiClient");
      break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
      const IPAddress ip = WiFi.localIP();
      Serial.print("WifiClient:         Got IP: ");
      util::UIConsole::log("Got IP: " + ip.toString(), "WifiClient");
      Serial.println(ip);
      connecting_ = false;
      reconnectDelayMs_ = 0;

      if (ip == IPAddress((uint32_t)0)) {
        if (Settings::instance().debugWSLevel >= 1) {
          Serial.println("WifiClient:         Got IP event but local IP is still 0.0.0.0, waiting...");
          util::UIConsole::log("Got IP event but local IP is 0.0.0.0, waiting...", "WifiClient");
        }
        break;
      }

      // Synchronize clock with NTP
      syncTime_();

      // Start mDNS when IP is ready
      if (!mdnsStarted_) {
        if (MDNS.begin(hostname_.c_str())) {
          mdnsStarted_ = true;
          MDNS.addService("http", "tcp", 80);
          if (Settings::instance().debugWSLevel >= 1) {
            Serial.printf("WifiClient:         mDNS running -> http://%s.local/\n", hostname_.c_str());
            util::UIConsole::log("mDNS running -> http://" + hostname_ + ".local/", "WifiClient");
          }
        } else {
          if (Settings::instance().debugWSLevel >= 1) {
            Serial.println("WifiClient:         MDNS.begin failed");
            util::UIConsole::log("MDNS.begin failed", "WifiClient");
          }
        }
      }
      break;
    }

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      connecting_ = false;
      if (reconnectDelayMs_ == 0) {
        reconnectDelayMs_ = kInitialReconnectDelayMs;
      } else {
        reconnectDelayMs_ = min(reconnectDelayMs_ * 2, kMaxReconnectDelayMs);
      }
      Serial.printf("WifiClient:         Disconnected (reason=%d)\n", info.wifi_sta_disconnected.reason);
      util::UIConsole::log("Disconnected, retrying connection...", "WifiClient");
      if (Settings::instance().debugWSLevel >= 1) {
        Serial.printf("WifiClient:         Retry in %lu ms.\n", reconnectDelayMs_);
        util::UIConsole::log("Retry in " + String(reconnectDelayMs_) + " ms", "WifiClient");
      }
      // Stop mDNS until we have IP again
      if (mdnsStarted_) {
        MDNS.end();
        mdnsStarted_ = false;
      }
      break;

    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      Serial.println("WifiClient:         Lost IP, forcing reconnect.");
      util::UIConsole::log("Lost IP, forcing reconnect.", "WifiClient");
      forceReconnect_();
      break;

    default:
      break;
  }
}

String WifiClient::makeDefaultHostname_() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char buf[32];
  snprintf(buf, sizeof(buf), "phoneexchange-%02X%02X", mac[4], mac[5]);
  return String(buf);
}

void WifiClient::syncTime_() {
  // Configure timezone for Swedish time (CET/CEST with automatic daylight saving)
  // CET-1CEST,M3.5.0,M10.5.0/3 = CET is UTC+1, CEST starts last Sunday in March, ends last Sunday in October
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  
  if (Settings::instance().debugWSLevel >= 1) {
    Serial.println("WifiClient:         Syncing time with NTP...");
    util::UIConsole::log("Syncing time with NTP...", "WifiClient");
  }
  
  // Wait a bit for NTP to sync
  struct tm timeinfo;
  int retries = 0;
  while (!getLocalTime(&timeinfo) && retries < 10) {
    delay(500);
    retries++;
  }
  
  if (retries < 10) {
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
    if (Settings::instance().debugWSLevel >= 1) {
      Serial.printf("WifiClient:         Time synced: %s\n", timeStr);
      util::UIConsole::log("Time synced: " + String(timeStr), "WifiClient");
    }
  } else {
    if (Settings::instance().debugWSLevel >= 1) {
      Serial.println("WifiClient:         Failed to sync time");
      util::UIConsole::log("Failed to sync time", "WifiClient");
    }
  }
}

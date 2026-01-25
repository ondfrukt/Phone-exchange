#include "WifiClient.h"
#include <time.h>
using namespace net;

WifiClient::WifiClient() : connecting_(false) {}

void WifiClient::begin(const char* hostname) {
  prefs_.begin("wifi", false);

  // 1) Välj hostname (antingen given, eller auto baserat på MAC)
  if (hostname && strlen(hostname) > 0) {
    hostname_ = hostname;
  } else {
    hostname_ = makeDefaultHostname_(); // ex: phoneexchange-7A3F
  }

  // 2) Sätt hostname FÖRE WiFi.begin()
  if (!WiFi.setHostname(hostname_.c_str())) {
    Serial.println("WifiClient: Failed to set hostname.");
    util::UIConsole::log("Failed to set hostname.", "WifiClient");
  }

  // 3) Registrera eventlyssnare (innan connect)
  WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t){ onWiFiEvent_(event); });

  // 4) Försök ansluta med sparade credentials
  String savedSsid, savedPass;
  if (loadCredentials(savedSsid, savedPass)) {
    ssid_ = savedSsid;
    password_ = savedPass;
    connect_();
  } else {
    Serial.println("WifiClient: No saved credentials, waiting for provisioning...");
    util::UIConsole::log("No saved credentials, waiting for provisioning...", "WifiClient");
  }
}

void WifiClient::loop() {
  // Automatisk reconnect
  if (!isConnected() && !connecting_ && ssid_.length() > 0) {
    connect_();
  }
}

bool WifiClient::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
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
  Serial.println("WifiClient: Credentials saved");
  util::UIConsole::log("Credentials saved", "WifiClient");
}

bool WifiClient::loadCredentials(String& ssid, String& password) {
  ssid = prefs_.getString("ssid", "");
  password = prefs_.getString("pass", "");
  return ssid.length() > 0;
}

void WifiClient::connect_() {
  Serial.printf("WifiClient: Connecting to %s… (host=%s)\n", ssid_.c_str(), hostname_.c_str());
  util::UIConsole::log("Connecting to " + ssid_ + "...", "WifiClient");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_.c_str(), password_.c_str());
  connecting_ = true;
}

void WifiClient::onWiFiEvent_(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("WifiClient: Connected to AP");
      util::UIConsole::log("Connected to AP", "WifiClient");
      break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
      Serial.print("WifiClient: Got IP: ");
      util::UIConsole::log("Got IP: " + WiFi.localIP().toString(), "WifiClient");
      Serial.println(WiFi.localIP());
      connecting_ = false;

      // Synkronisera klockan med NTP
      syncTime_();

      // Starta mDNS när IP är klart
      if (!mdnsStarted_) {
        if (MDNS.begin(hostname_.c_str())) {
          mdnsStarted_ = true;
          MDNS.addService("http", "tcp", 80);
          Serial.printf("WifiClient: mDNS igång → http://%s.local/\n", hostname_.c_str());
          util::UIConsole::log("mDNS running → http://" + hostname_ + ".local/", "WifiClient");
        } else {
          Serial.println("WifiClient: ⚠️ MDNS.begin misslyckades");
          util::UIConsole::log("⚠️ MDNS.begin failed", "WifiClient");
        }
      }
      break;
    }

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WifiClient: Disconnected, retrying…");
      util::UIConsole::log("Disconnected, retrying...", "WifiClient");
      connecting_ = false;
      // Stäng mDNS tills vi åter har IP
      if (mdnsStarted_) {
        MDNS.end();
        mdnsStarted_ = false;
      }
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
  // Konfigurera tidszon för svensk tid (CET/CEST med automatisk sommartid)
  // CET-1CEST,M3.5.0,M10.5.0/3 = CET är UTC+1, CEST börjar sista söndagen i mars, slutar sista söndagen i oktober
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  
  Serial.println("WifiClient: Syncing time with NTP...");
  util::UIConsole::log("Syncing time with NTP...", "WifiClient");
  
  // Vänta lite för att NTP ska hinna synka
  struct tm timeinfo;
  int retries = 0;
  while (!getLocalTime(&timeinfo) && retries < 10) {
    delay(500);
    retries++;
  }
  
  if (retries < 10) {
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
    Serial.printf("WifiClient: Time synced: %s\n", timeStr);
    util::UIConsole::log("Time synced: " + String(timeStr), "WifiClient");
  } else {
    Serial.println("WifiClient: Failed to sync time");
    util::UIConsole::log("Failed to sync time", "WifiClient");
  }
}

#include "WifiClient.h"
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
    Serial.println("WifiClient: ⚠️ setHostname misslyckades (fortsätter ändå)");
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
}

bool WifiClient::loadCredentials(String& ssid, String& password) {
  ssid = prefs_.getString("ssid", "");
  password = prefs_.getString("pass", "");
  return ssid.length() > 0;
}

void WifiClient::connect_() {
  Serial.printf("WifiClient: Connecting to %s… (host=%s)\n", ssid_.c_str(), hostname_.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_.c_str(), password_.c_str());
  connecting_ = true;
}

void WifiClient::onWiFiEvent_(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("WifiClient: Connected to AP");
      break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
      Serial.print("WifiClient: Got IP: ");
      Serial.println(WiFi.localIP());
      connecting_ = false;

      // Starta mDNS när IP är klart
      if (!mdnsStarted_) {
        if (MDNS.begin(hostname_.c_str())) {
          mdnsStarted_ = true;
          MDNS.addService("http", "tcp", 80);
          Serial.printf("WifiClient: mDNS igång → http://%s.local/\n", hostname_.c_str());
        } else {
          Serial.println("WifiClient: ⚠️ MDNS.begin misslyckades");
        }
      }
      break;
    }

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WifiClient: Disconnected, retrying…");
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

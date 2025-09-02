#include "WifiClient.h"

using namespace net;

WifiClient::WifiClient() : connecting_(false) {}

void WifiClient::begin(const char* hostname) {
    hostname_ = hostname;
    prefs_.begin("wifi", false);

    String savedSsid, savedPass;
    if (loadCredentials(savedSsid, savedPass)) {
        ssid_ = savedSsid;
        password_ = savedPass;
        connect_();
    } else {
        Serial.println("WifiClient: No saved credentials, waiting for provisioning...");
    }

    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
        onWiFiEvent_(event);
    });
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
    Serial.printf("WifiClient: Connecting to %s...\n", ssid_.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid_.c_str(), password_.c_str());
    connecting_ = true;
}

void WifiClient::onWiFiEvent_(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("WifiClient: Connected to AP");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.print("WifiClient: Got IP: ");
            Serial.println(WiFi.localIP());
            connecting_ = false;
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("WifiClient: Disconnected, retrying...");
            connecting_ = false;
            break;
        default:
            break;
    }
}

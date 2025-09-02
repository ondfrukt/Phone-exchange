#pragma once
#include <WiFi.h>
#include <Preferences.h>

namespace net {

class WifiClient {
public:
    WifiClient();

    // Initiera med app-namn, används för mDNS/OTA
    void begin(const char* hostname = "phoneexchange");

    // Körs i din App::loop()
    void loop();

    // Status
    bool isConnected() const;
    String getIp() const;

    // Hantering av credentials (kan sättas av provisioning)
    void saveCredentials(const char* ssid, const char* password);
    bool loadCredentials(String& ssid, String& password);

    bool hasCredentials() const { return ssid_.length() > 0; }
    void connectNow() { if (ssid_.length()) connect_(); }   // valfritt

private:
    void connect_();
    void onWiFiEvent_(WiFiEvent_t event);

    Preferences prefs_;
    String ssid_;
    String password_;
    String hostname_;
    bool connecting_;
};

} // namespace net

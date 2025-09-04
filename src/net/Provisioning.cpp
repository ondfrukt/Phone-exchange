#include "Provisioning.h"
using namespace net;

// Justera vid behov
static const char* kPop         = "abcd1234";         // Proof of Possession (PIN)
static const char* kServiceName = "PHONE_EXCHANGE";   // Namn som syns i Espressifs app
static const char* kServiceKey  = nullptr;            // Används bara för SoftAP
static const bool  kResetProv   = false;              // Rensa ev. gammal prov-data?

static uint8_t kUuid[16] = {
  0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
  0xea, 0x41, 0xa9, 0xea, 0x12, 0x21, 0x2f, 0x88
};

void Provisioning::begin(WifiClient& wifiClient, const char* hostname) {
  host_ = hostname;
  wifi_ = &wifiClient;

  // Lyssna på systemhändelser (Wi-Fi + provisioning)
  WiFi.onEvent(&Provisioning::onSysEvent_);

  // Om WifiClient redan har creds -> starta INTE provisioning
  if (wifi_->hasCredentials()) {
    Serial.printf("Provisioning: Saved SSID in NVS (via WifiClient). Skipping BLE");
    // WifiClient kopplar upp i sin begin()/loop() – vi gör inget här.
    return;
  }

  // Annars startar vi BLE-provisionering så användaren kan mata in creds
  startedProvisioning_ = true;

  Serial.println("Provisioning: Open Espressif BLE Provisioning app to configure Wi-Fi.");
  WiFiProv.beginProvision(
    WIFI_PROV_SCHEME_BLE,                 // Transport: BLE
    WIFI_PROV_SCHEME_HANDLER_FREE_BTDM,   // Frigör BT-minne efter provisioning
    WIFI_PROV_SECURITY_1,                 // PoP/PIN-säkerhet
    kPop,                                 // PoP
    kServiceName,                         // "PROV_..." gör det lätt för appen
    kServiceKey,                          // ej använd för BLE
    kUuid,                                // 16-byte UUID
    kResetProv                            // false = behåll ev. tidigare prov-data
  );
}

void Provisioning::factoryReset() {
  Serial.println("Provisioning: Factory reset: erasing Wi-Fi creds (NVS) and restarting...");
  WiFi.disconnect(true /*wifioff*/, true /*erase NVS*/);
  delay(250);
  ESP.restart();
}

void Provisioning::onSysEvent_(arduino_event_t *sys_event) {
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_START:
      Serial.println("Provisioning: Provisioning started. Open Espressif BLE Provisioning app.");
      break;

    case ARDUINO_EVENT_PROV_CRED_RECV: {
      const char* ssid = (const char*)sys_event->event_info.prov_cred_recv.ssid;
      const char* pwd  = (const char*)sys_event->event_info.prov_cred_recv.password;
      Serial.printf("Provisioning: Received Wi-Fi creds. SSID=\"%s\"\n", ssid);

      // Spara via WifiClient (i NVS via Preferences)
      if (wifi_) {
        wifi_->saveCredentials(ssid, pwd);
      }
      break;
    }

    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      Serial.println("Provisioning: Credentials accepted.");
      break;

    case ARDUINO_EVENT_PROV_CRED_FAIL:
      Serial.println("Provisioning: Credentials failed (wrong password or AP not found).");
      break;

    case ARDUINO_EVENT_PROV_END:
      Serial.println("Provisioning: Provisioning finished.");
      startedProvisioning_ = false;

      // INTE WiFi.begin() här. WifiClient tar det.
      if (wifi_) {
        // Om du vill forcera direktförsök: wifi_->connectNow();
      }
      break;

    // Rena Wi-Fi-händelser (för logg)
    //case ARDUINO_EVENT_WIFI_STA_CONNECTED:
    //  Serial.println("[WIFI] Connected to AP.");
    //  break;

    //case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    //  Serial.printf("[WIFI] Got IP: %s\n", WiFi.localIP().toString().c_str());
    //  break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("[WIFI] Disconnected.");
      break;

    default:
      break;
  }
}

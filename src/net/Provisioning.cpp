#include "Provisioning.h"
#include <esp_err.h>
using namespace net;

// Justera vid behov
static const char* kPop         = "abcd1234";         // Proof of Possession (PIN)
static const char* kServiceName = "PHONE_EXCHANGE";   // Namn som syns i Espressifs app
static const char* kServiceKey  = nullptr;            // Används bara för SoftAP
static const bool  kResetProv   = true;               // Starta med ren provisioning-state när inga app-creds finns

static uint8_t kUuid[16] = {
  0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
  0xea, 0x41, 0xa9, 0xea, 0x12, 0x21, 0x2f, 0x88
};

void Provisioning::begin(WifiClient& wifiClient, const char* hostname) {
  host_ = hostname;
  wifi_ = &wifiClient;
  pendingSsid_.clear();
  pendingPassword_.clear();
  credentialsCommitted_ = false;
  resetStatePending_ = false;
  credFailAtMs_ = 0;
  provisioningStartedAtMs_ = 0;
  startedProvisioning_ = false;

  // Lyssna på systemhändelser (Wi-Fi + provisioning)
  WiFi.onEvent(&Provisioning::onSysEvent_);

  // Om WifiClient redan har creds -> starta inte provisioning.
  if (wifi_->hasCredentials()) {
    Serial.println("Provisioning: Saved SSID found. Skipping provisioning.");
    util::UIConsole::log("Saved SSID found. Skipping provisioning.", "Provisioning");
    return;
  }

  // Annars startar vi BLE-provisionering så användaren kan mata in creds
  startedProvisioning_ = true;
  provisioningStartedAtMs_ = millis();

  Serial.println("Provisioning: Open Espressif BLE Provisioning app to configure Wi-Fi.");
  util::UIConsole::log("Open Espressif BLE Provisioning app to configure Wi-Fi.", "Provisioning");
  WiFiProv.beginProvision(
    WIFI_PROV_SCHEME_BLE,                 // Transport: BLE
    WIFI_PROV_SCHEME_HANDLER_FREE_BTDM,   // Frigör BT-minne efter provisioning
    WIFI_PROV_SECURITY_1,                 // PoP/PIN-säkerhet
    kPop,                                 // PoP
    kServiceName,                         // "PROV_..." gör det lätt för appen
    kServiceKey,                          // ej använd för BLE
    kUuid,                                // 16-byte UUID
    kResetProv                            // true = rensa tidigare provisioning-state för stabil ny scan-session
  );
}

void Provisioning::loop() {
  if (!startedProvisioning_) {
    return;
  }

  if (resetStatePending_ && (millis() - credFailAtMs_ >= kResetStateDelayMs)) {
    const esp_err_t err = wifi_prov_mgr_reset_sm_state_on_failure();
    if (err == ESP_OK) {
      Serial.println("Provisioning: Reset to waiting state after credential failure.");
      util::UIConsole::log("Reset to waiting state after credential failure.", "Provisioning");
      resetStatePending_ = false;
    } else {
      Serial.printf("Provisioning: reset_sm_state_on_failure failed (%d), retrying...\n", static_cast<int>(err));
      util::UIConsole::log("reset_sm_state_on_failure failed, retrying...", "Provisioning");
      credFailAtMs_ = millis();
    }
  }

  const unsigned long elapsed = millis() - provisioningStartedAtMs_;
  if (elapsed >= kProvisioningWindowMs) {
    Serial.println("Provisioning: 5-minute provisioning window expired, stopping provisioning.");
    util::UIConsole::log("5-minute provisioning window expired, stopping provisioning.", "Provisioning");
    stopProvisioning_();
  }
}

void Provisioning::stopProvisioning_() {
  if (!startedProvisioning_) {
    return;
  }

  wifi_prov_mgr_stop_provisioning();
  startedProvisioning_ = false;
  pendingSsid_.clear();
  pendingPassword_.clear();
  credentialsCommitted_ = false;
  resetStatePending_ = false;
}

void Provisioning::factoryReset() {
  Serial.println("Provisioning: Factory reset: erasing Wi-Fi creds (NVS) and restarting...");
  util::UIConsole::log("Factory reset: erasing Wi-Fi creds (NVS) and restarting...", "Provisioning");
  
  // Rensa WifiClient credentials (Preferences)
  Preferences prefs;
  prefs.begin("wifi", false);
  prefs.clear();  // Radera alla nycklar i "wifi" namnutrymmet
  prefs.end();
  
  // Rensa WiFi-credentials från NVS
  WiFi.disconnect(true /*wifioff*/, true /*erase NVS*/);
  
  delay(250);
  ESP.restart();
}

void Provisioning::onSysEvent_(arduino_event_t *sys_event) {
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_START:
      Serial.println("Provisioning: Provisioning started. Open Espressif BLE Provisioning app.");
      util::UIConsole::log("Provisioning started. Open Espressif BLE Provisioning app.", "Provisioning");
      break;

    case ARDUINO_EVENT_PROV_CRED_RECV: {
      const char* ssid = (const char*)sys_event->event_info.prov_cred_recv.ssid;
      const char* pwd  = (const char*)sys_event->event_info.prov_cred_recv.password;
      Serial.printf("Provisioning: Received Wi-Fi creds. SSID=\"%s\"\n", ssid);
      util::UIConsole::log("Received Wi-Fi creds. SSID=\"" + String(ssid) + "\"", "Provisioning");
      pendingSsid_ = ssid ? ssid : "";
      pendingPassword_ = pwd ? pwd : "";
      credentialsCommitted_ = false;
      break;
    }

    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      Serial.println("Provisioning: Credentials accepted.");
      util::UIConsole::log("Credentials accepted.", "Provisioning");
      if (wifi_ && pendingSsid_.length() > 0 && !credentialsCommitted_) {
        wifi_->saveCredentials(pendingSsid_.c_str(), pendingPassword_.c_str());
        credentialsCommitted_ = true;
      }
      break;

    case ARDUINO_EVENT_PROV_CRED_FAIL:
      Serial.println("Provisioning: Credentials failed (wrong password or AP not found).");
      util::UIConsole::log("Credentials failed (wrong password or AP not found).", "Provisioning");
      pendingSsid_.clear();
      pendingPassword_.clear();
      credentialsCommitted_ = false;
      resetStatePending_ = true;
      credFailAtMs_ = millis();
      break;

    case ARDUINO_EVENT_PROV_END:
      Serial.println("Provisioning: Provisioning finished.");
      util::UIConsole::log("Provisioning finished.", "Provisioning");
      startedProvisioning_ = false;
      pendingSsid_.clear();
      pendingPassword_.clear();
      credentialsCommitted_ = false;
      resetStatePending_ = false;

      if (wifi_) {
        // Kicka en explicit connect efter provisioning så vi inte fastnar i
        // "connected without valid IP"-läge tills manuell reboot.
        wifi_->connectNow();
      }
      break;

    // Rena Wi-Fi-händelser (för logg)
    //case ARDUINO_EVENT_WIFI_STA_CONNECTED:
    //  Serial.println("[WIFI] Connected to AP.");
    //  break;

    //case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    //  Serial.printf("[WIFI] Got IP: %s\n", WiFi.localIP().toString().c_str());
    //  break;

    // case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    //   Serial.println("[WIFI] Disconnected.");
    //   break;

    default:
      break;
  }
}

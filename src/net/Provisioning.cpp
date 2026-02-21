#include "Provisioning.h"
#include <esp_err.h>
#include <esp_wifi.h>

using namespace net;

// Provisioning defaults
static const char* kPop = "abcd1234";              // Proof of Possession (PIN)
static const char* kServiceName = "PHONE_EXCHANGE";
static const char* kServiceKey = nullptr;            // Used by SoftAP, ignored for BLE
static const bool kResetProv = false;                 // Keep provisioning-state unless explicit factory reset
static const bool kUseBleProvisioning = true;         // Set false to force SoftAP provisioning

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

  // Listen to system events (Wi-Fi + provisioning)
  WiFi.onEvent(&Provisioning::onSysEvent_);

  // If credentials are already present, skip provisioning.
  if (wifi_->hasCredentials()) {
    Serial.println("Provisioning:       Saved SSID found. Skipping provisioning.");
    util::UIConsole::log("Saved SSID found. Skipping provisioning.", "Provisioning");
    return;
  }

  startedProvisioning_ = true;
  provisioningStartedAtMs_ = millis();

  if (kUseBleProvisioning) {
    // Ensure STA stack is up before enabling BLE provisioning/coexistence.
    WiFi.mode(WIFI_MODE_STA);
    delay(50);
    Serial.println("Provisioning:       Starting Wi-Fi provisioning (BLE).");
    Serial.println("Provisioning:       Open Espressif BLE Provisioning app and use PoP abcd1234.");
    util::UIConsole::log("Starting Wi-Fi provisioning (BLE).", "Provisioning");
    util::UIConsole::log("Open Espressif BLE Provisioning app and use PoP abcd1234.", "Provisioning");
  } else {
    Serial.println("Provisioning:       Starting Wi-Fi provisioning (SoftAP).");
    Serial.println("Provisioning:       Connect to AP PHONE_EXCHANGE and use PoP abcd1234.");
    util::UIConsole::log("Starting Wi-Fi provisioning (SoftAP).", "Provisioning");
    util::UIConsole::log("Connect to AP PHONE_EXCHANGE and use PoP abcd1234.", "Provisioning");
  }

  WiFiProv.beginProvision(
    kUseBleProvisioning ? WIFI_PROV_SCHEME_BLE : WIFI_PROV_SCHEME_SOFTAP,
    kUseBleProvisioning ? WIFI_PROV_SCHEME_HANDLER_FREE_BTDM : WIFI_PROV_SCHEME_HANDLER_NONE,
    WIFI_PROV_SECURITY_1,
    kPop,
    kServiceName,
    kServiceKey,
    kUuid,
    kResetProv
  );
}

void Provisioning::loop() {
  if (!startedProvisioning_) {
    return;
  }

  if (resetStatePending_ && (millis() - credFailAtMs_ >= kResetStateDelayMs)) {
    const esp_err_t err = wifi_prov_mgr_reset_sm_state_on_failure();
    if (err == ESP_OK) {
      Serial.println("Provisioning:       Reset to waiting state after credential failure.");
      util::UIConsole::log("Reset to waiting state after credential failure.", "Provisioning");
      resetStatePending_ = false;
    } else {
      Serial.printf("Provisioning:       reset_sm_state_on_failure failed (%d), retrying...\n", static_cast<int>(err));
      util::UIConsole::log("reset_sm_state_on_failure failed, retrying...", "Provisioning");
      credFailAtMs_ = millis();
    }
  }

  const unsigned long elapsed = millis() - provisioningStartedAtMs_;
  if (elapsed >= kProvisioningWindowMs) {
    Serial.println("Provisioning:       5-minute provisioning window expired, stopping provisioning.");
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
  Serial.println("Provisioning:       Factory reset: erasing Wi-Fi creds (NVS) and restarting...");
  util::UIConsole::log("Factory reset: erasing Wi-Fi creds (NVS) and restarting...", "Provisioning");

  Preferences prefs;
  prefs.begin("wifi", false);
  prefs.clear();
  prefs.end();

  WiFi.disconnect(true /*wifioff*/, true /*erase NVS*/);

  delay(250);
  ESP.restart();
}

void Provisioning::onSysEvent_(arduino_event_t* sys_event) {
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_START:
      if (kUseBleProvisioning) {
        Serial.println("Provisioning:       Provisioning started (BLE).");
        util::UIConsole::log("Provisioning started (BLE).", "Provisioning");
      } else {
        Serial.println("Provisioning:       Provisioning started (SoftAP).");
        util::UIConsole::log("Provisioning started (SoftAP).", "Provisioning");
      }
      break;

    case ARDUINO_EVENT_PROV_CRED_RECV: {
      const char* ssid = (const char*)sys_event->event_info.prov_cred_recv.ssid;
      const char* pwd = (const char*)sys_event->event_info.prov_cred_recv.password;
      Serial.printf("Provisioning:       Received Wi-Fi creds. SSID=\"%s\"\n", ssid);
      util::UIConsole::log("Received Wi-Fi creds. SSID=\"" + String(ssid) + "\"", "Provisioning");
      pendingSsid_ = ssid ? ssid : "";
      pendingPassword_ = pwd ? pwd : "";
      credentialsCommitted_ = false;
      break;
    }

    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      Serial.println("Provisioning:       Credentials accepted.");
      util::UIConsole::log("Credentials accepted.", "Provisioning");
      if (wifi_ && pendingSsid_.length() > 0 && !credentialsCommitted_) {
        wifi_->saveCredentials(pendingSsid_.c_str(), pendingPassword_.c_str());
        credentialsCommitted_ = true;
      }
      break;

    case ARDUINO_EVENT_PROV_CRED_FAIL:
      Serial.println("Provisioning:       Credentials failed (wrong password or AP not found).");
      util::UIConsole::log("Credentials failed (wrong password or AP not found).", "Provisioning");
      pendingSsid_.clear();
      pendingPassword_.clear();
      credentialsCommitted_ = false;
      resetStatePending_ = true;
      credFailAtMs_ = millis();
      break;

    case ARDUINO_EVENT_PROV_END:
      Serial.println("Provisioning:       Provisioning finished.");
      util::UIConsole::log("Provisioning finished.", "Provisioning");
      startedProvisioning_ = false;
      pendingSsid_.clear();
      pendingPassword_.clear();
      credentialsCommitted_ = false;
      resetStatePending_ = false;

      if (wifi_) {
        // Explicit reconnect after provisioning to avoid stale no-IP state.
        wifi_->connectNow();
      }
      break;

    default:
      break;
  }
}


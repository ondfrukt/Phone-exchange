#include "Provisioning.h"
#include <esp_err.h>
#include <esp_wifi.h>

using namespace net;

// Provisioning defaults
static const char* kPop = "abcd1234";              // Proof of Possession (PIN)
static const char* kServiceName = "PHONE_EXCHANGE";  // SoftAP SSID
static const char* kServiceKey = nullptr;            // Optional SoftAP password (nullptr => open AP)
static const bool kResetProv = false;                 // Keep provisioning-state unless explicit factory reset

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

  if (provisioningTriedThisBoot_) {
    Serial.println("Provisioning:       Already attempted this boot. Waiting for restart.");
    util::UIConsole::log("Already attempted this boot. Waiting for restart.", "Provisioning");
    return;
  }

  provisioningTriedThisBoot_ = true;
  startedProvisioning_ = true;
  provisioningStartedAtMs_ = millis();

  Serial.println("Provisioning:       Starting Wi-Fi provisioning (SoftAP).");
  Serial.println("Provisioning:       Connect to AP PHONE_EXCHANGE and use PoP abcd1234.");
  util::UIConsole::log("Starting Wi-Fi provisioning (SoftAP).", "Provisioning");
  util::UIConsole::log("Connect to AP PHONE_EXCHANGE and use PoP abcd1234.", "Provisioning");

  WiFiProv.beginProvision(
    WIFI_PROV_SCHEME_SOFTAP,
    WIFI_PROV_SCHEME_HANDLER_NONE,
    WIFI_PROV_SECURITY_1,
    kPop,
    kServiceName,
    kServiceKey,
    nullptr,
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

  const esp_err_t provResetErr = wifi_prov_mgr_reset_provisioning();
  if (provResetErr != ESP_OK) {
    Serial.printf("Provisioning:       wifi_prov_mgr_reset_provisioning returned %d\n", static_cast<int>(provResetErr));
  }

  Preferences prefs;
  prefs.begin("wifi", false);
  prefs.clear();
  prefs.end();

  WiFi.disconnect(true /*wifioff*/, true /*erase NVS*/);
  esp_wifi_restore();

  delay(250);
  ESP.restart();
}

void Provisioning::onSysEvent_(arduino_event_t* sys_event) {
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_START:
      Serial.println("Provisioning:       Provisioning started (SoftAP).");
      util::UIConsole::log("Provisioning started (SoftAP).", "Provisioning");
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
      // Do not save/connect here. Let provisioning manager finish first (PROV_END)
      // to avoid parallel WiFi.begin() attempts from WifiClient during provisioning.
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

      if (wifi_ && pendingSsid_.length() > 0 && !credentialsCommitted_) {
        wifi_->saveCredentials(pendingSsid_.c_str(), pendingPassword_.c_str());
        credentialsCommitted_ = true;
      }

      startedProvisioning_ = false;
      pendingSsid_.clear();
      pendingPassword_.clear();
      resetStatePending_ = false;

      credentialsCommitted_ = false;
      break;

    default:
      break;
  }
}


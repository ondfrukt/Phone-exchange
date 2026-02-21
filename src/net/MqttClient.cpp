#include "net/MqttClient.h"
#include "services/LineManager.h"
#include "util/StatusSerializer.h"
#include "util/UIConsole.h"

namespace net {

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

MqttClient::MqttClient(Settings& settings, net::WifiClient& wifi, LineManager& lineManager)
  : settings_(settings), wifi_(wifi), lineManager_(lineManager), mqtt_(tcpClient_) {}

void MqttClient::begin() {
  const bool debug = settings_.debugMQTTLevel >= 1;
  loadConfig_();
  mqtt_.setKeepAlive(30);
  mqtt_.setSocketTimeout(5);
  mqtt_.setServer(host_.c_str(), port_);
  const bool hasUser = user_.length() > 0;
  if (debug) {
    util::UIConsole::log(
      "Startup config: enabled=" + String(settings_.mqttEnabled ? "true" : "false") +
      ", host=\"" + host_ + "\", port=" + String(port_) +
      ", clientId=\"" + clientId_ + "\", user=" + String(hasUser ? "set" : "empty"),
      "MqttClient");
    if (settings_.mqttEnabled && host_.length() > 0) {
      util::UIConsole::log("MQTT settings found in NVS and ready for connect.", "MqttClient");
    } else {
      util::UIConsole::log("No usable MQTT settings yet (disabled or host missing).", "MqttClient");
    }
  }

  if (!registeredCallbacks_) {
    lineManager_.addStatusChangedCallback([this](int lineIndex, model::LineStatus) {
      publishLineStatus(lineIndex);
    });
    registeredCallbacks_ = true;
  }
}

void MqttClient::reconfigureFromSettings() {
  if (settings_.debugMQTTLevel >= 1) {
    util::UIConsole::log("Reconfigure requested from updated settings.", "MqttClient");
  }
  disconnect_();
  loadConfig_();
  mqtt_.setServer(host_.c_str(), port_);
  reconnectDelayMs_ = 0;
  lastConnectAttemptMs_ = 0;
  failedConnectAttempts_ = 0;
  wasConnected_ = false;
  lastBlockedReason_ = -1;
}

void MqttClient::loop() {
  const bool debug = settings_.debugMQTTLevel >= 1;
  if (settings_.mqttConfigDirty) {
    reconfigureFromSettings();
    settings_.mqttConfigDirty = false;
  }

  const bool wasConnectedBefore = wasConnected_;
  const bool nowConnected = mqtt_.connected();
  if (wasConnectedBefore && !nowConnected) {
    const int state = mqtt_.state();
    String reason = "Connection dropped";
    reason += " (state=" + String(state) + " " + String(stateToText_(state)) + ")";
    if (!wifi_.isConnected()) {
      reason += ", WiFi not connected";
    }
    if (debug) {
      util::UIConsole::log(reason, "MqttClient");
    }
  }
  wasConnected_ = nowConnected;

  if (!shouldRun_()) {
    const int reason = blockedReason_();
    if (reason != lastBlockedReason_) {
      if (debug) {
        if (reason == 1) util::UIConsole::log("MQTT is disabled in settings.", "MqttClient");
        else if (reason == 2) util::UIConsole::log("MQTT host is empty; waiting for settings.", "MqttClient");
        else if (reason == 3) util::UIConsole::log("Waiting for WiFi before MQTT connect.", "MqttClient");
      }
      lastBlockedReason_ = reason;
    }
    if (mqtt_.connected()) disconnect_();
    return;
  }
  if (lastBlockedReason_ == 3) {
    if (debug) {
      util::UIConsole::log("WiFi is back; resuming MQTT connect attempts.", "MqttClient");
    }
  }
  lastBlockedReason_ = 0;

  if (nowConnected) {
    mqtt_.loop();
    return;
  }

  const unsigned long now = millis();
  const bool retryNow = (reconnectDelayMs_ == 0) ||
                        ((long)(now - lastConnectAttemptMs_) >= (long)reconnectDelayMs_);
  if (!retryNow) return;

  if (debug) {
    util::UIConsole::log(
      "Trying to connect to " + host_ + ":" + String(port_) +
      " (clientId=" + clientId_ + ")",
      "MqttClient");
  }
  if (connect_()) {
    reconnectDelayMs_ = 0;
    failedConnectAttempts_ = 0;
    if (debug && !wasConnectedBefore) util::UIConsole::log("Connection established.", "MqttClient");
    publishFullSnapshot();
  } else {
    failedConnectAttempts_++;
    if (failedConnectAttempts_ <= kQuickRetryAttempts) {
      if (reconnectDelayMs_ == 0) reconnectDelayMs_ = kInitialReconnectDelayMs;
      else reconnectDelayMs_ = min(reconnectDelayMs_ * 2, kMaxReconnectDelayMs);
    } else {
      reconnectDelayMs_ = kLongRetryDelayMs;
    }
    lastConnectAttemptMs_ = now;
    if (debug) {
      util::UIConsole::log(
        "Connect failed; next retry in " + String(reconnectDelayMs_) + " ms (attempt " +
        String(failedConnectAttempts_) + ").",
        "MqttClient");
    }
  }
}

void MqttClient::publishLineStatus(int lineIndex) {
  if (!mqtt_.connected()) return;
  if (lineIndex < 0 || lineIndex > 7) return;
  const auto& line = lineManager_.getLine(lineIndex);
  const auto status = line.currentLineStatus;
  String lineName = line.lineName;
  lineName.trim();

  const String topic = makeTopic_("line/status");
  String payload = "{";
  payload += "\"line\":" + String(lineIndex) + ",";
  payload += "\"status\":\"" + String(model::LineStatusToString(status)) + "\"";
  if (lineName.length() > 0) {
    payload += ",\"name\":\"" + escapeJson(lineName) + "\"";
  }
  payload += "}";

  mqtt_.publish(topic.c_str(), payload.c_str(), settings_.mqttRetain);
}

void MqttClient::publishFullSnapshot() {
  if (!mqtt_.connected()) return;

  const String linesTopic = makeTopic_("lines");
  const String linesJson = net::buildLinesStatusJson(lineManager_);
  mqtt_.publish(linesTopic.c_str(), linesJson.c_str(), settings_.mqttRetain);

  const String activeTopic = makeTopic_("active");
  String activeJson = "{\"mask\":" + String(settings_.activeLinesMask) + "}";
  mqtt_.publish(activeTopic.c_str(), activeJson.c_str(), settings_.mqttRetain);
}

void MqttClient::loadConfig_() {
  host_ = settings_.mqttHost;
  port_ = settings_.mqttPort == 0 ? 1883 : settings_.mqttPort;
  baseTopic_ = settings_.mqttBaseTopic.length() ? settings_.mqttBaseTopic : "phoneexchange";
  clientId_ = settings_.mqttClientId.length() ? settings_.mqttClientId : "phoneexchange";
  user_ = settings_.mqttUsername;
  pass_ = settings_.mqttPassword;
}

bool MqttClient::shouldRun_() const {
  return settings_.mqttEnabled && wifi_.isConnected() && host_.length() > 0;
}

int MqttClient::blockedReason_() const {
  if (!settings_.mqttEnabled) return 1;
  if (host_.length() == 0) return 2;
  if (!wifi_.isConnected()) return 3;
  return 0;
}

bool MqttClient::connect_() {
  lastConnectAttemptMs_ = millis();
  if (!shouldRun_()) return false;

  const String lwtTopic = makeTopic_("status");
  const bool ok = user_.length()
    ? mqtt_.connect(clientId_.c_str(), user_.c_str(), pass_.c_str(),
                    lwtTopic.c_str(), settings_.mqttQos, settings_.mqttRetain, "offline")
    : mqtt_.connect(clientId_.c_str(), lwtTopic.c_str(), settings_.mqttQos, settings_.mqttRetain, "offline");

  if (ok) {
    mqtt_.publish(lwtTopic.c_str(), "online", settings_.mqttRetain);
    if (settings_.debugMQTTLevel >= 1) {
      util::UIConsole::log("Connected to broker " + host_ + ":" + String(port_), "MqttClient");
    }
    wasConnected_ = true;
  } else {
    const int state = mqtt_.state();
    if (settings_.debugMQTTLevel >= 1) {
      util::UIConsole::log(
        "Connect failed, state=" + String(state) + " (" + String(stateToText_(state)) + ")",
        "MqttClient");
    }
  }
  return ok;
}

void MqttClient::disconnect_() {
  if (!mqtt_.connected()) return;
  const String topic = makeTopic_("status");
  mqtt_.publish(topic.c_str(), "offline", settings_.mqttRetain);
  mqtt_.disconnect();
  if (settings_.debugMQTTLevel >= 1) {
    util::UIConsole::log("Disconnected from broker.", "MqttClient");
  }
  wasConnected_ = false;
}

String MqttClient::makeTopic_(const char* suffix) const {
  String topic = baseTopic_;
  if (!topic.endsWith("/")) topic += "/";
  topic += suffix;
  return topic;
}

const char* MqttClient::stateToText_(int state) const {
  switch (state) {
    case -4: return "MQTT_CONNECTION_TIMEOUT";
    case -3: return "MQTT_CONNECTION_LOST";
    case -2: return "MQTT_CONNECT_FAILED";
    case -1: return "MQTT_DISCONNECTED";
    case 0:  return "MQTT_CONNECTED";
    case 1:  return "MQTT_CONNECT_BAD_PROTOCOL";
    case 2:  return "MQTT_CONNECT_BAD_CLIENT_ID";
    case 3:  return "MQTT_CONNECT_UNAVAILABLE";
    case 4:  return "MQTT_CONNECT_BAD_CREDENTIALS";
    case 5:  return "MQTT_CONNECT_UNAUTHORIZED";
    default: return "MQTT_STATE_UNKNOWN";
  }
}

} // namespace net

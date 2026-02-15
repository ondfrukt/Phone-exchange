#pragma once
#include <Preferences.h>
#include "drivers/InterruptManager.h"
#include "drivers/MCPDriver.h"
#include "settings/settings.h"
#include "config.h"

namespace net {
class WifiClient;
class MqttClient;
}

class Functions {

public:
    Functions(InterruptManager& interruptManager, MCPDriver& mcp_ks083f, net::WifiClient& wifiClient, net::MqttClient& mqttClient)
      : interruptManager_(interruptManager), mcp_ks083f(mcp_ks083f), wifiClient_(wifiClient), mqttClient_(mqttClient) {}
    void begin();
    void update();

private:
    InterruptManager& interruptManager_;
    MCPDriver& mcp_ks083f;
    net::WifiClient& wifiClient_;
    net::MqttClient& mqttClient_;
    void testRing();
    void restartDevice(uint32_t held);
    void initStatusLeds_();
    void updateStatusLeds_();

};

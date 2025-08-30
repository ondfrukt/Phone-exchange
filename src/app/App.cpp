#include "app/App.h"
#include "settings/settings.h"
#include "util/i2CScanner.h"
using namespace cfg;

App::App()
    : mcpDriver_(),
      lineManager_()
{
}

void App::begin() {
    auto& settings = Settings::instance();
    settings.load();  // ladda sparade inställningar (om några)

    Serial.begin(115200);
    Serial.println("App starting...");
    i2cScan(Wire, i2c::SDA_PIN, i2c::SCL_PIN, 100000);
    Wire.begin(i2c::SDA_PIN, i2c::SCL_PIN);
    mcpDriver_.begin();
    lineManager_.begin();
}

void App::loop() {
  // Dränera max N events per varv för rättvisa
  for (int i = 0; i < 16; ++i) {
    IntResult r = mcpDriver_.handleSlic1Interrupt();
    if (!r.hasEvent) break;
    // ...hantera event...
    yield();          // eller delay(0); ger tid till andra tasks
  }

  // Hantera även andra källor
  for (int i = 0; i < 16; ++i) {
    auto r2 = mcpDriver_.handleSlic2Interrupt();
    if (!r2.hasEvent) break;
    yield();
  }
}



// struct IntResult {
//   bool    hasEvent = false;  // om något fanns att hämta
//   uint8_t line     = 255;    // 0..7, 255 = okänd linje
//   uint8_t pin      = 255;    // 0..15, 255 = ogiltig
//   bool    level    = false;  // nivå enligt INTCAP/getLastInterruptValue()
//   uint8_t i2c_addr = 0x00;   // vilken MCP
// };
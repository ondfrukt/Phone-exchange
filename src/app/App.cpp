#include "app/App.h"
#include "util/i2CScanner.h"
using namespace cfg;



App::App()
    : mcpDriver_(),
      lineManager_()
{
}

void App::begin() {
    Serial.begin(115200);
    Serial.println("App starting...");
    i2cScan(Wire, 9, 10, 100000);
    Wire.begin(i2c::SDA_PIN, i2c::SCL_PIN);
    mcpDriver_.begin();
    lineManager_.begin();
}

void App::loop() {
    while (true) {
    IntResult r = mcpDriver_.handleSlic1Interrupt(); // läser INTCAP -> kvitterar
    if (!r.hasEvent) break;
    }
}



// struct IntResult {
//   bool    hasEvent = false;  // om något fanns att hämta
//   uint8_t line     = 255;    // 0..7, 255 = okänd linje
//   uint8_t pin      = 255;    // 0..15, 255 = ogiltig
//   bool    level    = false;  // nivå enligt INTCAP/getLastInterruptValue()
//   uint8_t i2c_addr = 0x00;   // vilken MCP
// };
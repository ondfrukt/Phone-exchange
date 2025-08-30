#include "app/App.h"
#include "util/i2CScanner.h"
using namespace cfg;

App::App()
    : mcpDriver_(),
      lineManager_(),
      SHKService_(lineManager_, mcpDriver_, Settings::instance())
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
    settings.adjustActiveLines();
    lineManager_.begin();
}

void App::loop() {
    // Hantera SLIC‑1 avbrott

    for (int i=0; i<16; ++i) {
        IntResult r = mcpDriver_.handleSlic1Interrupt();
        if (r.hasEvent && r.line < 8) {
        uint32_t mask = (1u << r.line);
        SHKService_.notifyLinesPossiblyChanged(mask, millis());
        yield();
        }
    }

        for (int i=0; i<16; ++i) {
        IntResult r = mcpDriver_.handleSlic2Interrupt();
        if (r.hasEvent && r.line < 8) {
        uint32_t mask = (1u << r.line);
        SHKService_.notifyLinesPossiblyChanged(mask, millis());
        yield();
        }
    }

    uint32_t nowMs = millis();
    if (SHKService_.needsTick(nowMs)) {
        SHKService_.tick(nowMs); // uppdaterar hook‑status och pulser
    }
}
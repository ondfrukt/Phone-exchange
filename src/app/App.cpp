#include "app/App.h"
#include "util/i2CScanner.h"
using namespace cfg;

App::App()
  : mcpDriver_(),
    lineManager_(Settings::instance()),
    SHKService_(lineManager_, mcpDriver_, Settings::instance()),
    lineAction_(lineManager_, Settings::instance()),
    webServer_(Settings::instance(), lineManager_, wifiClient_, 80){
}

void App::begin() {
    Serial.begin(115200);
    Serial.println("App starting...");

    auto& settings = Settings::instance();

    settings.resetDefaults(); // sätt standardvärden. Används nu under utveckling
    //settings.load();  // ladda sparade inställningar (om några) (ska användas i färdig produkt)
    

    i2cScan(Wire, i2c::SDA_PIN, i2c::SCL_PIN, 100000);
    Wire.begin(i2c::SDA_PIN, i2c::SCL_PIN);

    mcpDriver_.begin();
    settings.adjustActiveLines(); // säkerställ att minst en linje är aktiv

    lineManager_.begin();
    wifiClient_.begin("phoneexchange");
    provisioning_.begin(wifiClient_, "phoneexchange");
    webServer_.begin();
    webServer_.listFS();

    Serial.println("App setup complete");
}

void App::loop() {
    // Hantera SLIC‑1 avbrott

    wifiClient_.loop();


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
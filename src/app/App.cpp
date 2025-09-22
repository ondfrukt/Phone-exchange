#include "app/App.h"
using namespace cfg;

App::App()
  : mcpDriver_(),
    mt8816Driver_(mcpDriver_, Settings::instance()),
    lineManager_(Settings::instance()),
    SHKService_(lineManager_, mcpDriver_, Settings::instance()),
    lineAction_(lineManager_, Settings::instance(), mt8816Driver_),
    webServer_(Settings::instance(), lineManager_, wifiClient_, 80),
    testButton_(mcpDriver_) {}

void App::begin() {
    Serial.begin(115200);
    Serial.println("----- App starting -----");

		// ----  Settings ----
    auto& settings = Settings::instance();
    settings.resetDefaults(); // sätt standardvärden. Används nu under utveckling
    //settings.load();  // ladda sparade inställningar (om några) (ska användas i färdig produkt)

		// ---- I2C och I2C-scanner ----
		Wire.begin(i2c::SDA_PIN, i2c::SCL_PIN);
		i2cScanner.scan();

		// ---- Drivrutiner och tjänster ----
    mcpDriver_.begin();
		mt8816Driver_.begin();

    lineManager_.begin();
    settings.adjustActiveLines(); // säkerställ att minst en linje är aktiv

		// --- Net and webserver ---
    wifiClient_.begin("phoneexchange");
    provisioning_.begin(wifiClient_, "phoneexchange");
    webServer_.begin();
    Serial.println("----- App setup complete -----");
}


void App::loop() {
	wifiClient_.loop();
	lineAction_.update(); // Check for line status changes and timers
	SHKService_.update(); // Check for SHK changes and process pulses
  testButton_.update();
}
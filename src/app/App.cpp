#include "app/App.h"
using namespace cfg;

App::App()
  : mcpDriver_(),
    mt8816Driver_(mcpDriver_, Settings::instance()),
    lineManager_(Settings::instance()),
    SHKService_(lineManager_, mcpDriver_, Settings::instance()),
    lineAction_(lineManager_, Settings::instance(), mt8816Driver_),
    webServer_(Settings::instance(), lineManager_, wifiClient_, 80),
    functionButton_(mcpDriver_) {}

void App::begin() {
    Serial.begin(115200);
    util::UIConsole::init(200); 
    Serial.println();
    Serial.println("----- App starting -----");
    util::UIConsole::log("", "App");
    util::UIConsole::log("----- App starting -----", "App");

		// ----  Settings ----
    auto& settings = Settings::instance();
    settings.resetDefaults(); // sätt standardvärden. Används nu under utveckling
    //settings.load();  // ladda sparade inställningar (om några) (ska användas i färdig produkt)

		// ---- I2C och I2C-scanner ----
		Wire.begin(i2c::SDA_PIN, i2c::SCL_PIN);

    // I2C-scanner if debug is enabled
    if (settings.debugI2CLevel >= 1) i2cScanner.scan();
		

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
    util::UIConsole::log("----- App setup complete -----", "App");
}

void App::loop() {
	wifiClient_.loop();
	lineAction_.update(); // Check for line status changes and timers
	SHKService_.update(); // Check for SHK changes and process pulses
  functionButton_.update();
}
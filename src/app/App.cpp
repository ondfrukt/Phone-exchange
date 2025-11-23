#include "app/App.h"
using namespace cfg;

App::App()
  : mcpDriver_(),
    mt8816Driver_(mcpDriver_, Settings::instance()),
    lineManager_(Settings::instance()),

    toneGenerator1_(cfg::ad9833::CS1_PIN),
    toneGenerator2_(cfg::ad9833::CS2_PIN),  
    toneGenerator3_(cfg::ad9833::CS3_PIN),
    toneReader_(mcpDriver_, Settings::instance(), lineManager_),

    SHKService_(lineManager_, mcpDriver_, Settings::instance()),
    lineAction_(lineManager_, Settings::instance(), mt8816Driver_,
                toneGenerator1_, toneGenerator2_, toneGenerator3_),
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
    settings.resetDefaults(); 
    //settings.load();

		// ---- I2C och I2C-scanner ----
		Wire.begin(i2c::SDA_PIN, i2c::SCL_PIN);

    // I2C-scanner if debug is enabled
    if (settings.debugI2CLevel >= 1) i2cScanner.scan();
		

		// ---- Drivrutiner och tjänster ----
    mcpDriver_.begin();
		mt8816Driver_.begin();
    lineAction_.begin();
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
  toneReader_.update(); // Check for DTMF tones

  if (toneGenerator1_.isPlaying()) toneGenerator1_.update();
  if (toneGenerator2_.isPlaying()) toneGenerator2_.update();
  if (toneGenerator3_.isPlaying()) toneGenerator3_.update();

  // Läs endast STD och skriv ut "1" vid hög nivå
  {
    bool level = false;
    mcpDriver_.digitalReadMCP(cfg::mcp::MCP_MAIN_ADDRESS, cfg::mcp::STD, level);
    if (level) {
      Serial.println("1");
    }
  }

  functionButton_.update();
}
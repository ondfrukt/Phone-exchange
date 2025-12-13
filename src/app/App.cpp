#include "app/App.h"
using namespace cfg;

App::App()
  : mcpDriver_(),
    interruptManager_(mcpDriver_, Settings::instance()),
    mt8816Driver_(mcpDriver_, Settings::instance()),
    lineManager_(Settings::instance()),

    toneGenerator1_(cfg::ad9833::CS1_PIN),
    toneGenerator2_(cfg::ad9833::CS2_PIN),  
    toneGenerator3_(cfg::ad9833::CS3_PIN),
    toneReader_(interruptManager_, mcpDriver_, Settings::instance(), lineManager_),

    SHKService_(lineManager_, interruptManager_, mcpDriver_, Settings::instance()),
    lineAction_(lineManager_, Settings::instance(), mt8816Driver_,
                toneGenerator1_, toneGenerator2_, toneGenerator3_),
    ringGenerator_(mcpDriver_, Settings::instance(), lineManager_),
    webServer_(Settings::instance(), lineManager_, wifiClient_, &ringGenerator_, 80),
    functionButton_(interruptManager_) {}

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
    
    // Set RingGenerator reference in SHKService to filter out ringing lines
    SHKService_.setRingGenerator(&ringGenerator_);
    
    toneGenerator1_.begin();
    toneGenerator2_.begin();
    toneGenerator3_.begin();

		// --- Net and webserver ---
    wifiClient_.begin("phoneexchange");
    provisioning_.begin(wifiClient_, "phoneexchange");
    webServer_.begin();

    Serial.println("----- App setup complete -----");
    util::UIConsole::log("----- App setup complete -----", "App");
}

void App::loop() {
  auto& settings = Settings::instance();
  
  // Debug: Show that main loop is running (very low frequency)
  static unsigned long lastLoopDebug = 0;
  unsigned long now = millis();
  
  // Collect all interrupts from MCP devices into InterruptManager queue
  interruptManager_.collectInterrupts();
  
  wifiClient_.loop();   // Handle WiFi events and connection
  lineAction_.update(); // Check for line status changes and timers
  SHKService_.update(); // Check for SHK changes and process pulses
  toneReader_.update(); // Check for DTMF tones
  ringGenerator_.update(); // Update ring signal generation

  if (toneGenerator1_.isPlaying() && Settings::instance().toneGeneratorEnabled) toneGenerator1_.update();
  if (toneGenerator2_.isPlaying() && Settings::instance().toneGeneratorEnabled) toneGenerator2_.update();
  if (toneGenerator3_.isPlaying() && Settings::instance().toneGeneratorEnabled) toneGenerator3_.update();

  functionButton_.update();
}
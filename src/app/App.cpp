#include "app/App.h"
using namespace cfg;

// Note: C++ initializes members in the order they are declared in App.h,
// not the visual order in this initializer list.
App::App()
  // ===== Low-level drivers =====
  : mcpDriver_(),
    interruptManager_(mcpDriver_, Settings::instance()),
    mt8816Driver_(mcpDriver_, Settings::instance()),
    connectionHandler_(mt8816Driver_, Settings::instance()),

    // ===== Tone generation stack =====
    // Three physical AD9833 chips + one coordinating service.
    ad9833Driver1_(cfg::ESP_PINS::CS1_PIN),
    ad9833Driver2_(cfg::ESP_PINS::CS2_PIN),
    ad9833Driver3_(cfg::ESP_PINS::CS3_PIN),
    toneGenerator_(ad9833Driver1_, ad9833Driver2_, ad9833Driver3_),

    // ===== Telephony services =====
    lineManager_(Settings::instance()),
    toneReader_(interruptManager_, mcpDriver_, Settings::instance(), lineManager_),
    ringGenerator_(mcpDriver_, Settings::instance(), lineManager_),
    SHKService_(lineManager_, interruptManager_, mcpDriver_, Settings::instance(), ringGenerator_),

    // ===== Networking services =====
    wifiClient_(),
    provisioning_(),
    mqttClient_(Settings::instance(), wifiClient_, lineManager_),
    lineAction_(lineManager_, Settings::instance(), mt8816Driver_, ringGenerator_, toneReader_,
                toneGenerator_, connectionHandler_, mqttClient_),

    // WebServer depends on line/ring/action + wifi.
    webServer_(Settings::instance(), lineManager_, wifiClient_, ringGenerator_, lineAction_, 80),
    functions_(interruptManager_, mcpDriver_, wifiClient_, mqttClient_) {
    // Late wiring: optional callback dependency that cannot be injected in ctor
    // without circular include pressure.
    lineManager_.setToneReader(&toneReader_);
}


void App::begin() {
    Serial.begin(115200);
    functions_.begin();
    util::UIConsole::init(200); 
    Serial.println();
    Serial.println("----- App starting -----");
    util::UIConsole::log("", "App");
    util::UIConsole::log("----- App starting -----", "App");

		// ----  Settings ----
    auto& settings = Settings::instance();
    const bool settingsLoaded = settings.load();
    if (settingsLoaded) {
      Serial.println("App: Settings loaded from NVS.");
      util::UIConsole::log("Settings loaded from NVS.", "App");
    } else {
      Serial.println("App: Using default settings (stored to NVS if needed).");
      util::UIConsole::log("Using default settings (stored to NVS if needed).", "App");
    }

		// ---- I2C ----
		Wire.begin(ESP_PINS::SDA_PIN, ESP_PINS::SCL_PIN);

    // I2C-scanner if debug is enabled
    if (settings.debugI2CLevel >= 1) i2cScanner.scan();
		
		// ---- Drivers setup ----
    mcpDriver_.begin();
		mt8816Driver_.begin();
    toneGenerator_.begin();
    Serial.println("----- Drivers initialized -----");
    
    //----- Service setup -----
    lineAction_.begin();
    lineManager_.begin();
    settings.adjustActiveLines(); // säkerställ att minst en linje är aktiv
    Serial.println("----- Services initialized -----");

    // ----- Net applications -----
    wifiClient_.begin("phoneexchange");
    provisioning_.begin(wifiClient_, "phoneexchange");
    mqttClient_.begin();
    Serial.println("App: Deferring WebServer start until WiFi has IP");

    Serial.println("----- App setup complete -----");
    Serial.println();
    util::UIConsole::log("----- App setup complete -----", "App");
    util::UIConsole::log("", "App");
}

void App::loop() {
  update();
}

void App::update() {
  
  // ---- Interrupt handling ----
  interruptManager_.collectInterrupts();  // Collect all interrupts from MCP devices into InterruptManager queue
  
  // ---- Network and services updates ----
  wifiClient_.loop();       // Handle WiFi events and connection
  provisioning_.loop();     // Auto-close provisioning window after timeout
  webServer_.update();      // Handle web server events and client interactions
  mqttClient_.loop();       // Handle MQTT connection and messaging

  // ---- Service updates (order can matter) ----
  lineAction_.update();     // Check for line status changes and timers
  SHKService_.update();     // Check for SHK changes and process pulses
  toneReader_.update();     // Check for DTMF tones
  ringGenerator_.update();  // Update ring signal steps and timing
  toneGenerator_.update();  // Update tone generation steps and timing
  functions_.update();      // Run any utility functions
}

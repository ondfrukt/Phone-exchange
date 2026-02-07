#include "app/App.h"
using namespace cfg;

App::App()
  : mcpDriver_(),
    interruptManager_(mcpDriver_, Settings::instance()),
    mt8816Driver_(mcpDriver_, Settings::instance()),

    toneGenerator1_(cfg::ESP_PINS::CS1_PIN),
    toneGenerator2_(cfg::ESP_PINS::CS2_PIN),
    toneGenerator3_(cfg::ESP_PINS::CS3_PIN),

    lineManager_(Settings::instance()),
    toneReader_(interruptManager_, mcpDriver_, Settings::instance(), lineManager_),
    ringGenerator_(mcpDriver_, Settings::instance(), lineManager_),
    SHKService_(lineManager_, interruptManager_, mcpDriver_, Settings::instance(), ringGenerator_),
    lineAction_(lineManager_, Settings::instance(), mt8816Driver_, ringGenerator_, toneReader_,
                toneGenerator1_, toneGenerator2_, toneGenerator3_),

    webServer_(Settings::instance(), lineManager_, wifiClient_, ringGenerator_, lineAction_, 80),
    functions_(interruptManager_, mcpDriver_) {
    lineManager_.setToneReader(&toneReader_);
}


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

		// ---- I2C ----
		Wire.begin(ESP_PINS::SDA_PIN, ESP_PINS::SCL_PIN);

    // I2C-scanner if debug is enabled
    if (settings.debugI2CLevel >= 1) i2cScanner.scan();
		

		// ---- Drivrutiner och tjänster ----
    i2cScanner.scan();
    mcpDriver_.begin();
		mt8816Driver_.begin();
    lineAction_.begin();
    lineManager_.begin();
    settings.adjustActiveLines(); // säkerställ att minst en linje är aktiv
    
    toneGenerator1_.begin();
    toneGenerator2_.begin();
    toneGenerator3_.begin();
    Serial.println("----- App setup complete -----");
    Serial.println();
    util::UIConsole::log("----- App setup complete -----", "App");
    util::UIConsole::log("", "App");

    // --- Net and webserver ---
    wifiClient_.begin("phoneexchange");
    provisioning_.begin(wifiClient_, "phoneexchange");
    webServer_.begin();
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

  if (settings.debugMCPLevel >= 1) {
    GPIOTest(cfg::mcp::MCP_MAIN_ADDRESS, cfg::mcp::FUNCTION_BUTTON);
  }
}

void App::GPIOTest(uint8_t addr, int pin) {
  struct PinWatch {
    bool used = false;
    uint8_t addr = 0;
    int pin = -1;
    bool value = false;
    bool initialized = false;
    uint32_t lastSampleMs = 0;
  };

  static PinWatch watched[8];

  PinWatch* slot = nullptr;
  for (auto& entry : watched) {
    if (entry.used && entry.addr == addr && entry.pin == pin) {
      slot = &entry;
      break;
    }
  }

  if (!slot) {
    for (auto& entry : watched) {
      if (!entry.used) {
        entry.used = true;
        entry.addr = addr;
        entry.pin = pin;
        slot = &entry;
        break;
      }
    }
  }

  if (!slot) return;

  // Important for MCP interrupt debugging:
  // reading GPIO acknowledges/clears pending interrupts on MCP23x17.
  // Keep sampling rate low so INT remains asserted long enough to observe.
  constexpr uint32_t kSampleIntervalMs = 150;
  uint32_t nowMs = millis();
  if ((nowMs - slot->lastSampleMs) < kSampleIntervalMs) {
    return;
  }
  slot->lastSampleMs = nowMs;

  bool currentValue = false;
  if (!mcpDriver_.digitalReadMCP(addr, static_cast<uint8_t>(pin), currentValue)) {
    return;
  }

  if (!slot->initialized) {
    slot->value = currentValue;
    slot->initialized = true;
    return;
  }

  if (slot->value != currentValue) {
    slot->value = currentValue;
    Serial.print(F("GPIOTest addr=0x"));
    Serial.print(addr, HEX);
    Serial.print(F(" pin="));
    Serial.print(pin);
    Serial.print(F(" state="));
    Serial.println(currentValue ? F("HIGH") : F("LOW"));
  }
}

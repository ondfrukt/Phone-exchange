#include "drivers/MCPDriver.h"
using namespace cfg;

// Register addresses for MCP23X17
static constexpr uint8_t REG_IOCON     = 0x0A;
static constexpr uint8_t REG_INTFA     = 0x0E;
static constexpr uint8_t REG_INTFB     = 0x0F;
static constexpr uint8_t REG_INTCAPA   = 0x10;
static constexpr uint8_t REG_INTCAPB   = 0x11;
static constexpr uint8_t REG_GPIOA     = 0x12;
static constexpr uint8_t REG_GPIOB     = 0x13;
static constexpr uint8_t REG_GPINTENA  = 0x04;
static constexpr uint8_t REG_GPINTENB  = 0x05;
static constexpr uint8_t REG_INTCONA   = 0x08;
static constexpr uint8_t REG_INTCONB   = 0x09;
static constexpr uint8_t REG_DEFVALA   = 0x06;
static constexpr uint8_t REG_DEFVALB   = 0x07;
static constexpr uint8_t REG_GPPUA     = 0x0C;
static constexpr uint8_t REG_GPPUB     = 0x0D;

// Split a PinModeEntry table into separate mode and initial-value arrays
static void splitPinTable(const cfg::mcp::PinModeEntry (&tbl)[16], uint8_t (&modes)[16], bool (&initial)[16]) {
  for (int i=0;i<16;i++){ modes[i]=tbl[i].mode; initial[i]=tbl[i].initial; }
}

// Write an 8-bit value to a register over I2C
bool MCPDriver::writeReg8_(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg); Wire.write(val);
  return Wire.endTransmission() == 0;
}

// Read an 8-bit register over I2C with success status
bool MCPDriver::readReg8_OK_(uint8_t addr, uint8_t reg, uint8_t& out) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)addr, 1) != 1) return false;
  out = Wire.read();
  return true;
}

// Read two consecutive registers as a 16-bit little-endian value with success status
bool MCPDriver::readRegPair16_OK_(uint8_t addr, uint8_t regA, uint16_t& out16) {
  Wire.beginTransmission(addr);
  Wire.write(regA);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)addr, 2) != 2) return false;
  uint8_t a = Wire.read();
  uint8_t b = Wire.read();
  out16 = (uint16_t)a | ((uint16_t)b<<8);
  return true;
}

// Read an 8-bit register over I2C without error checking
uint8_t MCPDriver::readReg8_(uint8_t addr, uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom((int)addr, 1);
  return Wire.available() ? Wire.read() : 0;
}

// Read two consecutive registers as a 16-bit little-endian value without error checking
void MCPDriver::readRegPair16_(uint8_t addr, uint8_t regA, uint16_t& out16) {
  Wire.beginTransmission(addr);
  Wire.write(regA);
  Wire.endTransmission(false);
  Wire.requestFrom((int)addr, 2);
  uint8_t a = Wire.available() ? Wire.read() : 0;
  uint8_t b = Wire.available() ? Wire.read() : 0;
  out16 = (uint16_t)a | ((uint16_t)b<<8);
}

// Initialize all MCP devices, configure pins and interrupts
bool MCPDriver::begin() {
  Wire.setTimeOut(50);
  auto& settings = Settings::instance();
  haveMain_ = haveSlic1_ = haveSlic2_ = haveMT8816_ = false;

  // Probe each MCP device
  haveSlic1_  = probeMcp_(mcpSlic1_,  cfg::mcp::MCP_SLIC1_ADDRESS);
  haveSlic2_  = probeMcp_(mcpSlic2_,  cfg::mcp::MCP_SLIC2_ADDRESS);
  haveMain_   = probeMcp_(mcpMain_,   cfg::mcp::MCP_MAIN_ADDRESS);
  haveMT8816_ = probeMcp_(mcpMT8816_, cfg::mcp::MCP_MT8816_ADDRESS);

  // Store presence info in settings
  settings.mcpSlic1Present  = haveSlic1_;
  settings.mcpSlic2Present = haveSlic2_;
  settings.mcpMainPresent  = haveMain_;
  settings.mcpMt8816Present = haveMT8816_;

  // Print MCP detection status
  if (settings.mcpSlic1Present) {
  Serial.println(F("MCP init:"));
    util::UIConsole::log("MCP init:", "MCPDriver");
    if (haveMain_) {
      Serial.println(F(" - MCP_MAIN   hittad")); util::UIConsole::log(" - MCP_MAIN   hittad", "MCPDriver");
    } else {
      Serial.println(F(" - MCP_MAIN   saknas")); util::UIConsole::log(" - MCP_MAIN   saknas", "MCPDriver");
    }
    if (haveSlic1_) {
      Serial.println(F(" - MCP_SLIC1  hittad")); util::UIConsole::log(" - MCP_SLIC1  hittad", "MCPDriver");
    } else {
      Serial.println(F(" - MCP_SLIC1  saknas")); util::UIConsole::log(" - MCP_SLIC1  saknas", "MCPDriver");
    }
    if (haveSlic2_) {
      Serial.println(F(" - MCP_SLIC2  hittad")); util::UIConsole::log(" - MCP_SLIC2  hittad", "MCPDriver");
    } else {
      Serial.println(F(" - MCP_SLIC2  saknas")); util::UIConsole::log(" - MCP_SLIC2  saknas", "MCPDriver");
    }
    if (haveMT8816_) {
      Serial.println(F(" - MCP_MT8816 hittad")); util::UIConsole::log(" - MCP_MT8816 hittad", "MCPDriver");
    } else {
      Serial.println(F(" - MCP_MT8816 saknas")); util::UIConsole::log(" - MCP_MT8816 saknas", "MCPDriver");
    }
  }

  // Abort if no MCP found
  if (!(haveMain_ || haveSlic1_ || haveSlic2_ || haveMT8816_ )) {
    Serial.println(F("Ingen MCP hittades, avbryter."));
    util::UIConsole::log("Ingen MCP hittades, avbryter.", "MCPDriver");
    return false;
  }

  // Lambda to program IOCON on both banks
  auto programIOCON = [&](uint8_t addr, uint8_t iocon){
    return writeReg8_(addr, REG_IOCON, iocon) && writeReg8_(addr, REG_IOCON+1, iocon);
  };

  // Configure MCP_MAIN pins
  if (haveMain_) {
    uint8_t modes[16]; bool initial[16];
    splitPinTable(mcp::MCP_MAIN, modes, initial);
    if (!applyPinModes_(mcpMain_, modes, initial)) return false;
    programIOCON(cfg::mcp::MCP_MAIN_ADDRESS, 0x44);   // MAIN: active-low, open-drain, mirrored interrupts
    enableMainInterrupts_(cfg::mcp::MCP_MAIN_ADDRESS, mcpMain_);
  }
  // Configure MCP_SLIC1 pins
  if (haveSlic1_) {
    uint8_t modes[16]; bool initial[16];
    splitPinTable(mcp::MCP_SLIC, modes, initial);
    if (!applyPinModes_(mcpSlic1_, modes, initial)) return false;
  }
  // Configure MCP_SLIC2 pins
  if (haveSlic2_) {
    uint8_t modes[16]; bool initial[16];
    splitPinTable(mcp::MCP_SLIC, modes, initial);
    if (!applyPinModes_(mcpSlic2_, modes, initial)) return false;
  }
  // Configure MCP_MT8816 pins
  if (haveMT8816_) {
    uint8_t modes[16]; bool initial[16];
    splitPinTable(mcp::MCP_MT8816, modes, initial);
    if (!applyPinModes_(mcpMT8816_, modes, initial)) return false;
    programIOCON(cfg::mcp::MCP_MT8816_ADDRESS, 0x4A);
  }

  // Configure interrupts for SLIC1
  if (haveSlic1_) {
    writeReg8_(mcp::MCP_SLIC1_ADDRESS, REG_IOCON,     0x44);
    writeReg8_(mcp::MCP_SLIC1_ADDRESS, REG_IOCON + 1, 0x44);

    enableSlicShkInterrupts_(cfg::mcp::MCP_SLIC1_ADDRESS, mcpSlic1_);
    
    uint16_t dummy;
    (void)readRegPair16_OK_(mcp::MCP_SLIC1_ADDRESS, REG_INTCAPA, dummy);
    (void)readRegPair16_OK_(mcp::MCP_SLIC1_ADDRESS, REG_GPIOA,   dummy);
  }

  // Configure interrupts for SLIC2
  if (haveSlic2_) {
    writeReg8_(mcp::MCP_SLIC2_ADDRESS, REG_IOCON,     0x44);
    writeReg8_(mcp::MCP_SLIC2_ADDRESS, REG_IOCON + 1, 0x44);

    enableSlicShkInterrupts_(cfg::mcp::MCP_SLIC2_ADDRESS, mcpSlic2_);

    uint16_t dummy;
    (void)readRegPair16_OK_(mcp::MCP_SLIC2_ADDRESS, REG_INTCAPA, dummy);
    (void)readRegPair16_OK_(mcp::MCP_SLIC2_ADDRESS, REG_GPIOA,   dummy);
  }
  // Attach interrupts for MCP_MAIN
  if (haveMain_) {
    pinMode(mcp::MCP_MAIN_INT_PIN, INPUT_PULLUP);
    attachInterruptArg(digitalPinToInterrupt(mcp::MCP_MAIN_INT_PIN),
                       &MCPDriver::isrMainThunk, this, FALLING);
    Serial.print(F("MCP INT: MCP_MAIN interrupt attached to ESP32 pin "));
    Serial.println(mcp::MCP_MAIN_INT_PIN);
    util::UIConsole::log("MCP INT: MCP_MAIN interrupt attached to ESP32 pin " + 
                         String(mcp::MCP_MAIN_INT_PIN), "MCPDriver");
  }
  // Attach interrupts for MCP_SLIC1
  if (haveSlic1_) {
    pinMode(mcp::MCP_SLIC_INT_1_PIN, INPUT_PULLUP);
    attachInterruptArg(digitalPinToInterrupt(mcp::MCP_SLIC_INT_1_PIN),
                       &MCPDriver::isrSlic1Thunk, this, FALLING);
    Serial.print(F("MCP INT: MCP_SLIC1 interrupt attached to ESP32 pin "));
    Serial.println(mcp::MCP_SLIC_INT_1_PIN);
    util::UIConsole::log("MCP INT: MCP_SLIC1 interrupt attached to ESP32 pin " + 
                         String(mcp::MCP_SLIC_INT_1_PIN), "MCPDriver");
  }
  // Attach interrupts for MCP_SLIC2
  if (haveSlic2_) {
    pinMode(mcp::MCP_SLIC_INT_2_PIN, INPUT_PULLUP);
    attachInterruptArg(digitalPinToInterrupt(mcp::MCP_SLIC_INT_2_PIN),
                       &MCPDriver::isrSlic2Thunk, this, FALLING);
    Serial.print(F("MCP INT: MCP_SLIC2 interrupt attached to ESP32 pin "));
    Serial.println(mcp::MCP_SLIC_INT_2_PIN);
    util::UIConsole::log("MCP INT: MCP_SLIC2 interrupt attached to ESP32 pin " + 
                         String(mcp::MCP_SLIC_INT_2_PIN), "MCPDriver");
  }
  
  Serial.println(F("MCP init: All interrupts configured successfully"));
  util::UIConsole::log("MCP init: All interrupts configured successfully", "MCPDriver");
  
  return true;
}

// Write a digital value to a pin on the specified MCP device
bool MCPDriver::digitalWriteMCP(uint8_t addr, uint8_t pin, bool value) {
  if (addr==mcp::MCP_MAIN_ADDRESS   && !haveMain_)   return false;
  if (addr==mcp::MCP_SLIC1_ADDRESS  && !haveSlic1_)  return false;
  if (addr == mcp::MCP_SLIC2_ADDRESS && !haveSlic2_) return false;
  if (addr==mcp::MCP_MT8816_ADDRESS && !haveMT8816_) return false;

  Adafruit_MCP23X17* m = nullptr;
  if (addr==mcp::MCP_MAIN_ADDRESS)        m=&mcpMain_;
  else if (addr==mcp::MCP_SLIC1_ADDRESS)  m=&mcpSlic1_;
  else if (addr == mcp::MCP_SLIC2_ADDRESS) m = &mcpSlic2_;
  else if (addr==mcp::MCP_MT8816_ADDRESS) m=&mcpMT8816_;
  else return false;

  m->digitalWrite(pin, value);
  return true;
}

// Read a digital value from a pin on the specified MCP device
bool MCPDriver::digitalReadMCP(uint8_t addr, uint8_t pin, bool& out) {
  if (addr==mcp::MCP_MAIN_ADDRESS   && !haveMain_)   return false;
  if (addr==mcp::MCP_SLIC1_ADDRESS  && !haveSlic1_)  return false;
  if (addr==mcp::MCP_SLIC2_ADDRESS  && !haveSlic2_)  return false;
  if (addr==mcp::MCP_MT8816_ADDRESS && !haveMT8816_) return false;

  Adafruit_MCP23X17* m = nullptr;
  if (addr==mcp::MCP_MAIN_ADDRESS)        m=&mcpMain_;
  else if (addr==mcp::MCP_SLIC1_ADDRESS)  m=&mcpSlic1_;
  else if (addr==mcp::MCP_SLIC2_ADDRESS)  m=&mcpSlic2_;
  else if (addr==mcp::MCP_MT8816_ADDRESS) m=&mcpMT8816_;
  else return false;

  out = m->digitalRead(pin);
  return true;
}

// Interrupt service routines for each MCP device (thunks set flags)
void IRAM_ATTR MCPDriver::isrMainThunk(void* arg)   { 
  auto* driver = reinterpret_cast<MCPDriver*>(arg);
  driver->mainIntFlag_ = true; 
  driver->mainIntCounter_++;
}
void IRAM_ATTR MCPDriver::isrSlic1Thunk(void* arg)  { 
  auto* driver = reinterpret_cast<MCPDriver*>(arg);
  driver->slic1IntFlag_ = true; 
  driver->slic1IntCounter_++;
}
void IRAM_ATTR MCPDriver::isrSlic2Thunk(void* arg)  { 
  auto* driver = reinterpret_cast<MCPDriver*>(arg);
  driver->slic2IntFlag_ = true; 
  driver->slic2IntCounter_++;
}
void IRAM_ATTR MCPDriver::isrMT8816Thunk(void* arg) { 
  auto* driver = reinterpret_cast<MCPDriver*>(arg);
  driver->mt8816IntFlag_ = true; 
  driver->mt8816IntCounter_++;
}

// Handle interrupts for MCP_MAIN
IntResult MCPDriver::handleMainInterrupt()   {
  if (!haveMain_) return {};
  auto& settings = Settings::instance();
  
  // Debug: Report interrupt counter periodically
  static unsigned long lastDebugTime = 0;
  unsigned long now = millis();
  if (settings.debugMCPLevel >= 1 && (now - lastDebugTime >= 5000)) {
    Serial.print(F("MCP_MAIN: Total interrupts fired: "));
    Serial.println(mainIntCounter_);
    util::UIConsole::log("MCP_MAIN: Total interrupts fired: " + String(mainIntCounter_), "MCPDriver");
    lastDebugTime = now;
  }
  
  return handleInterrupt_(mainIntFlag_,   mcpMain_,   mcp::MCP_MAIN_ADDRESS);
}
// Handle interrupts for MCP_SLIC1
IntResult MCPDriver::handleSlic1Interrupt()  {
  if (!haveSlic1_) return {};
  IntResult r = handleInterrupt_(slic1IntFlag_,  mcpSlic1_,  mcp::MCP_SLIC1_ADDRESS);
  return r;
}

// Handle interrupts for MCP_SLIC2
IntResult MCPDriver::handleSlic2Interrupt()  {
  if (!haveSlic2_) return {};
  IntResult r = handleInterrupt_(slic2IntFlag_,  mcpSlic2_,  mcp::MCP_SLIC2_ADDRESS);
  return r;
}
// Handle interrupts for MCP_MT8816

IntResult MCPDriver::handleMT8816Interrupt() {
  if (!haveMT8816_) return {};
  return handleInterrupt_(mt8816IntFlag_, mcpMT8816_, mcp::MCP_MT8816_ADDRESS);
}

// Handle an interrupt for a specific MCP device and return the result
IntResult MCPDriver::handleInterrupt_(volatile bool& flag, Adafruit_MCP23X17& mcp, uint8_t addr) {
  bool fired=false;
  noInterrupts();
  fired = flag;
  flag  = false;
  interrupts();

  IntResult r;
  r.i2c_addr = addr;
  if (!fired) return r;

  auto& settings = Settings::instance();
  const bool verbose = settings.debugMCPLevel >= 2;
  const bool basicDebug = settings.debugMCPLevel >= 1;

  // Debug: Confirm interrupt flag was set
  if (basicDebug) {
    Serial.print(F("MCP INT: Processing interrupt for addr=0x"));
    Serial.println(addr, HEX);
    util::UIConsole::log("MCP INT: Processing interrupt for addr=0x" + String(addr, HEX),
                         "MCPDriver");
  }

  uint16_t intf = 0;
  if (!readRegPair16_OK_(addr, REG_INTFA, intf)) {
    if (basicDebug) {
      Serial.println(F("MCP INT: Failed to read INTF register"));
      util::UIConsole::log("MCP INT: Failed to read INTF register", "MCPDriver");
    }
    return r;
  }
  
  // Debug: Show INTF register value
  if (verbose) {
    Serial.print(F("MCP INT: INTF=0b"));
    Serial.println(intf, BIN);
    util::UIConsole::log("MCP INT: INTF=0b" + String(intf, BIN), "MCPDriver");
  }
  
  if (intf == 0) {
    // No pin reported despite the interrupt flag being set; make sure the
    // event is acknowledged to avoid getting stuck in a busy loop.
    uint16_t dummy = 0;
    (void)readRegPair16_OK_(addr, REG_GPIOA, dummy);
    if (verbose) {
      Serial.print(F("MCP INT: flag set men INTF=0 på 0x"));
      Serial.println(addr, HEX);
      util::UIConsole::log("MCP INT: flag set men INTF=0 på 0x" + String(addr, HEX),
                           "MCPDriver");
    }
    return r;
  }

  uint16_t intcap = 0;
  if (!readRegPair16_OK_(addr, REG_INTCAPA, intcap)) {
    if (basicDebug) {
      Serial.println(F("MCP INT: Failed to read INTCAP register"));
      util::UIConsole::log("MCP INT: Failed to read INTCAP register", "MCPDriver");
    }
    return r;
  }

  // Debug: Show INTCAP register value (after reading it)
  if (verbose) {
    Serial.print(F("MCP INT: INTCAP=0b"));
    Serial.println(intcap, BIN);
    util::UIConsole::log("MCP INT: INTCAP=0b" + String(intcap, BIN), "MCPDriver");
  }

  for (uint8_t p = 0; p < 16; ++p) {
    if (intf & (1u << p)) {
      r.pin   = p;
      r.level = (intcap & (1u << p)) != 0;
      r.hasEvent = true;
      break;
    }
  }

  // Extra clear after INTCAP (harmless but makes acknowledgment more robust)
  uint16_t dummy = 0;
  (void)readRegPair16_OK_(addr, REG_GPIOA, dummy);

  if (!r.hasEvent) return r;

  if (verbose) {
    Serial.print(F("MCP INT: addr=0x"));
    Serial.print(addr, HEX);
    Serial.print(F(" pin="));
    Serial.print(r.pin);
    Serial.print(F(" level="));
    Serial.println(r.level ? F("HIGH") : F("LOW"));
    util::UIConsole::log("MCP INT 0x" + String(addr, HEX) + " pin=" + String(r.pin) +
                             " level=" + String(r.level ? "HIGH" : "LOW"),
                         "MCPDriver");
  }

  // SLIC: map to line (do not return here)
  if (addr == cfg::mcp::MCP_SLIC1_ADDRESS || addr == cfg::mcp::MCP_SLIC2_ADDRESS) {
    int8_t line = mapSlicPinToLine_(addr, r.pin);
    r.line = (line >= 0) ? static_cast<uint8_t>(line) : 255;
  }

  // Sync DEFVAL to the captured level (MAIN only; requires INTCON=1 for the pin)
  if (addr == cfg::mcp::MCP_MAIN_ADDRESS) {
    if (r.pin < 8) {
      uint8_t defvala = 0;
      if (readReg8_OK_(addr, REG_DEFVALA, defvala)) {
        if (r.level) defvala |=  (1u << r.pin);
        else         defvala &= ~(1u << r.pin);
        (void)writeReg8_(addr, REG_DEFVALA, defvala);
      }
    } else {
      uint8_t bit = r.pin - 8; // 0..7
      uint8_t defvalb = 0;
      if (readReg8_OK_(addr, REG_DEFVALB, defvalb)) {
        if (r.level) defvalb |=  (1u << bit);
        else         defvalb &= ~(1u << bit);
        (void)writeReg8_(addr, REG_DEFVALB, defvalb);
      }
    }
  }

  return r;
}

// Map a SLIC pin to its corresponding line index
int8_t MCPDriver::mapSlicPinToLine_(uint8_t addr, uint8_t pin) const {
  for (uint8_t line = 0; line < cfg::mcp::SHK_LINE_COUNT; ++line) {
    if (cfg::mcp::SHK_LINE_ADDR[line] == addr &&
        cfg::mcp::SHK_PINS[line]      == pin) {
      return static_cast<int8_t>(line);
    }
  }
  return -1;
}


void MCPDriver::mt8816PowerControl(bool set) {
  mt8816Powered_ = set;
  digitalWriteMCP(cfg::mcp::MCP_MT8816_ADDRESS, cfg::mcp::PWDN_MT8870, set);
}

// Apply pin modes and initial states to an MCP device
bool MCPDriver::applyPinModes_(Adafruit_MCP23X17& mcp, const uint8_t (&modes)[16], const bool (&initial)[16]) {
  for (uint8_t p=0;p<16;p++) {
    switch (modes[p]) {
      case INPUT:
        mcp.pinMode(p, INPUT);
        mcp.digitalWrite(p, LOW);
        break;
      case INPUT_PULLUP:
        mcp.pinMode(p, INPUT);
        mcp.digitalWrite(p, HIGH);
        break;
      case OUTPUT:
        mcp.pinMode(p, OUTPUT);
        mcp.digitalWrite(p, initial[p] ? HIGH : LOW);
        break;
      default:
        return false;
    }
  }
  return true;
}

// Enable interrupts for SLIC shake pins and configure pull-ups
void MCPDriver::enableSlicShkInterrupts_(uint8_t i2cAddr, Adafruit_MCP23X17& mcp) {
  uint8_t gpintena=0, gpintenb=0;
  uint8_t intcona=0, intconb=0;

  uint8_t gppua=0, gppub=0;
  (void)readReg8_OK_(i2cAddr, REG_GPPUA, gppua);
  (void)readReg8_OK_(i2cAddr, REG_GPPUB, gppub);

  // Set interrupt and pull-up bits for shake pins
  for (uint8_t i=0; i<sizeof(mcp::SHK_PINS)/sizeof(mcp::SHK_PINS[0]); ++i) {
    uint8_t p = mcp::SHK_PINS[i];
    if (p<8) gpintena |= (1u<<p);
    else     gpintenb |= (1u<<(p-8));
    if (p<8) gppua |= (1u<<p); else gppub |= (1u<<(p-8));
  }

  writeReg8_(i2cAddr, REG_INTCONA, intcona);
  writeReg8_(i2cAddr, REG_INTCONB, intconb);

  writeReg8_(i2cAddr, REG_DEFVALA, 0x00);
  writeReg8_(i2cAddr, REG_DEFVALB, 0x00);

  writeReg8_(i2cAddr, REG_GPPUA, gppua);
  writeReg8_(i2cAddr, REG_GPPUB, gppub);

  writeReg8_(i2cAddr, REG_GPINTENA, gpintena);
  writeReg8_(i2cAddr, REG_GPINTENB, gpintenb);

  (void)mcp.readGPIOAB();
}

// Enable interrupts for MCP_MAIN (e.g. function button and MT8870 STD)
void MCPDriver::enableMainInterrupts_(uint8_t i2cAddr, Adafruit_MCP23X17& mcp) {
  auto& settings = Settings::instance();
  
  Serial.println(F("MCP INT: Configuring MCP_MAIN interrupts..."));
  util::UIConsole::log("MCP INT: Configuring MCP_MAIN interrupts...", "MCPDriver");
  
  uint8_t gpintena=0, gpintenb=0;
  uint8_t intcona=0, intconb=0;
  uint8_t gppua=0, gppub=0;
  uint8_t defvala=0, defvalb=0;

  // Read current settings
  (void)readReg8_OK_(i2cAddr, REG_GPINTENA, gpintena);
  (void)readReg8_OK_(i2cAddr, REG_GPINTENB, gpintenb);
  (void)readReg8_OK_(i2cAddr, REG_INTCONA,  intcona);
  (void)readReg8_OK_(i2cAddr, REG_INTCONB,  intconb);
  (void)readReg8_OK_(i2cAddr, REG_GPPUA,    gppua);
  (void)readReg8_OK_(i2cAddr, REG_GPPUB,    gppub);
  (void)readReg8_OK_(i2cAddr, REG_DEFVALA,  defvala);
  (void)readReg8_OK_(i2cAddr, REG_DEFVALB,  defvalb);

  // ---- FUNCTION_BUTTON: CHANGE mode with pull-up ----
  {
    uint8_t p = cfg::mcp::FUNCTION_BUTTON;
    Serial.print(F("MCP INT: Configuring FUNCTION_BUTTON on pin "));
    Serial.println(p);
    if (p < 8) {
      gpintena |= (1u << p);   // enable INT
      gppua    |= (1u << p);   // pull-up
      intcona  &= ~(1u << p);  // CHANGE mode (ignore DEFVAL)
    } else {
      uint8_t bit = p - 8;
      gpintenb |= (1u << bit); // enable INT
      gppub    |= (1u << bit); // pull-up
      intconb  &= ~(1u << bit); // CHANGE mode
    }
  }

  // ---- MT8870 STD: compare-to-DEFVAL (INTCON=1). DEFVAL synced in handleInterrupt_ ----
  {
    uint8_t p = cfg::mcp::STD; // GPB3 according to config
    Serial.print(F("MCP INT: Configuring MT8870 STD on pin "));
    Serial.print(p);
    Serial.println(F(" (compare-to-DEFVAL mode)"));
    util::UIConsole::log("MCP INT: Configuring MT8870 STD on pin " + String(p) + 
                         " (compare-to-DEFVAL mode)", "MCPDriver");
    if (p < 8) {
      gpintena |= (1u << p);
      gppua    |= (1u << p);  // håll linjen stabilt hög när MT8870 släpper STD
      intcona  |= (1u << p);  // enable compare-to-DEFVAL
      // DEFVALA updated dynamically by handleInterrupt_
    } else {
      uint8_t bit = p - 8;
      gpintenb |= (1u << bit);
      gppub    |= (1u << bit); // håll linjen stabilt hög när MT8870 släpper STD
      intconb  |= (1u << bit); // enable compare-to-DEFVAL
      // DEFVALB updated dynamically by handleInterrupt_
    }
  }

  // Write back configuration
  writeReg8_(i2cAddr, REG_GPPUA, gppua);
  writeReg8_(i2cAddr, REG_GPPUB, gppub);

  writeReg8_(i2cAddr, REG_INTCONA, intcona);
  writeReg8_(i2cAddr, REG_INTCONB, intconb);

  writeReg8_(i2cAddr, REG_DEFVALA, defvala);
  writeReg8_(i2cAddr, REG_DEFVALB, defvalb);

  writeReg8_(i2cAddr, REG_GPINTENA, gpintena);
  writeReg8_(i2cAddr, REG_GPINTENB, gpintenb);
  
  Serial.print(F("MCP INT: GPINTENA=0b"));
  Serial.print(gpintena, BIN);
  Serial.print(F(" GPINTENB=0b"));
  Serial.println(gpintenb, BIN);
  Serial.print(F("MCP INT: INTCONA=0b"));
  Serial.print(intcona, BIN);
  Serial.print(F(" INTCONB=0b"));
  Serial.println(intconb, BIN);
  util::UIConsole::log("MCP INT: GPINTENA=0b" + String(gpintena, BIN) + 
                       " GPINTENB=0b" + String(gpintenb, BIN), "MCPDriver");

  // Acknowledge any pending flags (read INTCAP followed by GPIO)
  uint16_t dummy=0;
  (void)readRegPair16_OK_(i2cAddr, REG_INTCAPA, dummy);
  (void)readRegPair16_OK_(i2cAddr, REG_GPIOA,   dummy);
}

// Read 16-bit GPIO value (GPIOA low byte, GPIOB high byte)
bool MCPDriver::readGpioAB16(uint8_t addr, uint16_t& out16) {
  uint8_t a = readReg8_(addr, 0x12);
  uint8_t b = readReg8_(addr, 0x13);
  out16 = (uint16_t(b) << 8) | uint16_t(a);
  return true;
}

// Probe an MCP device at the given I2C address; return true if present
bool MCPDriver::probeMcp_(Adafruit_MCP23X17& mcp, uint8_t addr) {
  if (!mcp.begin_I2C(addr)) return false;
  return true;
}
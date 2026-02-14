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
static constexpr uint8_t REG_OLATA     = 0x14;

// Split a PinModeEntry table into separate mode and initial-value arrays
static void splitPinTable(const cfg::mcp::PinModeEntry (&tbl)[16], uint8_t (&modes)[16], bool (&initial)[16]) {
  for (int i = 0; i < 16; i++) { modes[i] = tbl[i].mode; initial[i] = tbl[i].initial; }
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
  out16 = (uint16_t)a | ((uint16_t)b << 8);
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
  out16 = (uint16_t)a | ((uint16_t)b << 8);
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
  settings.mcpSlic1Present   = haveSlic1_;
  settings.mcpSlic2Present   = haveSlic2_;
  settings.mcpMainPresent    = haveMain_;
  settings.mcpMt8816Present  = haveMT8816_;

  // Print MCP detection status
  if (settings.debugMCPLevel >= 1) {
    Serial.println(F("MCP init"));
  }

  // Abort if no MCP found
  if (!(haveMain_ || haveSlic1_ || haveSlic2_ || haveMT8816_ )) {
    Serial.println(F("Ingen MCP hittades, avbryter."));
    return false;
  }

  // ------------------------------------------------------------
  // Helpers
  // ------------------------------------------------------------
  // Program IOCON safely: write to both mirror addresses (0x0A/0x0B) to avoid BANK confusion
  auto programIOCON = [&](uint8_t addr, uint8_t iocon) {
    // IOCON in BANK=0 is accessible as 0x0A and 0x0B (mirror). Writing both is harmless and robust.
    const bool okA = writeReg8_(addr, 0x0A, iocon);
    const bool okB = writeReg8_(addr, 0x0B, iocon);
    return okA && okB;
  };

  // Dump key interrupt-related registers (for debugging)
  auto dumpIntRegs = [&](const char* tag, uint8_t addr) {
    uint8_t ioconA=0, ioconB=0, iodira=0, iodirb=0, gpintena=0, gpintenb=0, gppua=0, gppub=0, intcona=0, intconb=0;
    (void)readReg8_OK_(addr, 0x0A, ioconA);
    (void)readReg8_OK_(addr, 0x0B, ioconB);
    (void)readReg8_OK_(addr, 0x00, iodira);
    (void)readReg8_OK_(addr, 0x01, iodirb);
    (void)readReg8_OK_(addr, 0x04, gpintena);
    (void)readReg8_OK_(addr, 0x05, gpintenb);
    (void)readReg8_OK_(addr, 0x0C, gppua);
    (void)readReg8_OK_(addr, 0x0D, gppub);
    (void)readReg8_OK_(addr, 0x08, intcona);
    (void)readReg8_OK_(addr, 0x09, intconb);

    Serial.printf("%s addr=0x%02X IOCON(0x0A/0x0B)=0x%02X/0x%02X IODIR(A/B)=0x%02X/0x%02X "
                  "GPINTEN(A/B)=0x%02X/0x%02X GPPU(A/B)=0x%02X/0x%02X INTCON(A/B)=0x%02X/0x%02X\n",
                  tag, addr, ioconA, ioconB, iodira, iodirb, gpintena, gpintenb, gppua, gppub, intcona, intconb);
  };

  // ------------------------------------------------------------
  // Configure pins (direction / initial)
  // ------------------------------------------------------------

  // Configure MCP_MAIN pins
  if (haveMain_) {
    uint8_t modes[16]; bool initial[16];
    splitPinTable(mcp::MCP_MAIN, modes, initial);
    if (!applyPinModes_(mcpMain_, modes, initial)){
      Serial.println(F("Fel vid konfiguration av MAIN pinlägen"));
      return false;
    }
    // MAIN: active-low, open-drain, mirrored interrupts (INTA/INTB become one)
    // IOCON bits: MIRROR=1 (0x40), ODR=1 (0x04), INTPOL=0 (active low)
    programIOCON(cfg::mcp::MCP_MAIN_ADDRESS, 0x44);

    enableMainInterrupts_(cfg::mcp::MCP_MAIN_ADDRESS, mcpMain_);
    if (settings.debugMCPLevel >= 3) dumpIntRegs("MAIN AFTER", cfg::mcp::MCP_MAIN_ADDRESS);
  }
  // ...existing code...
  // Configure MCP_SLIC1 pins
  if (haveSlic1_) {
    // ...existing code...
    uint8_t modes[16]; bool initial[16];
    splitPinTable(mcp::MCP_SLIC, modes, initial);
    if (!applyPinModes_(mcpSlic1_, modes, initial)) {
      Serial.println(F("Fel vid konfiguration av SLIC1 pinlägen"));
      return false;
    }
  }

  // Configure MCP_SLIC2 pins
  // ...existing code...
  if (haveSlic2_) {
    // ...existing code...
    uint8_t modes[16]; bool initial[16];
    splitPinTable(mcp::MCP_SLIC, modes, initial);
    if (!applyPinModes_(mcpSlic2_, modes, initial)) {
      Serial.println(F("Fel vid konfiguration av SLIC2 pinlägen"));
      return false;
    }
  }

  // Configure MCP_MT8816 pins
  // ...existing code...
  if (haveMT8816_) {
    // ...existing code...
    uint8_t modes[16]; bool initial[16];
    splitPinTable(mcp::MCP_MT8816, modes, initial);
    if (!applyPinModes_(mcpMT8816_, modes, initial)) {
      Serial.println(F("Fel vid konfiguration av MT8816 pinlägen"));
      return false;
    }
    programIOCON(cfg::mcp::MCP_MT8816_ADDRESS, 0x4A);
    // ...existing code...
  }

  // ------------------------------------------------------------
  // Configure interrupts
  // ------------------------------------------------------------

  if (haveSlic1_) {
    // SLIC: active-low, open-drain, mirrored interrupts
    programIOCON(cfg::mcp::MCP_SLIC1_ADDRESS, 0x44);
    enableSlicShkInterrupts_(cfg::mcp::MCP_SLIC1_ADDRESS, mcpSlic1_);
    // Clear any pending interrupt latches (important: reading INTCAP or GPIO clears INT)
    uint16_t dummy = 0;
    (void)readRegPair16_OK_(cfg::mcp::MCP_SLIC1_ADDRESS, REG_INTCAPA, dummy);
    (void)readRegPair16_OK_(cfg::mcp::MCP_SLIC1_ADDRESS, REG_GPIOA,   dummy);
  }

  if (haveSlic2_) {
    programIOCON(cfg::mcp::MCP_SLIC2_ADDRESS, 0x44);
    enableSlicShkInterrupts_(cfg::mcp::MCP_SLIC2_ADDRESS, mcpSlic2_);
    uint16_t dummy = 0;
    (void)readRegPair16_OK_(cfg::mcp::MCP_SLIC2_ADDRESS, REG_INTCAPA, dummy);
    (void)readRegPair16_OK_(cfg::mcp::MCP_SLIC2_ADDRESS, REG_GPIOA,   dummy);
  }
  
  // ------------------------------------------------------------
  // Attach ESP32 interrupts (only if INT lines are wired)
  // ------------------------------------------------------------
  if (haveMain_) {
    pinMode(mcp::MCP_MAIN_INT_PIN, INPUT_PULLUP);
    attachInterruptArg(digitalPinToInterrupt(mcp::MCP_MAIN_INT_PIN),
                       &MCPDriver::isrMainThunk, this, FALLING);
  }

  if (haveSlic1_) {
    pinMode(mcp::MCP_SLIC_INT_1_PIN, INPUT_PULLUP);
    attachInterruptArg(digitalPinToInterrupt(mcp::MCP_SLIC_INT_1_PIN),
                       &MCPDriver::isrSlic1Thunk, this, FALLING);
  }
  
  if (haveSlic2_) {
    pinMode(mcp::MCP_SLIC_INT_2_PIN, INPUT_PULLUP);
    attachInterruptArg(digitalPinToInterrupt(mcp::MCP_SLIC_INT_2_PIN),
                       &MCPDriver::isrSlic2Thunk, this, FALLING);
  }

  // ...existing code...
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

// Update TMUX address lines on MCP_MAIN GPA0..GPA2 in one register write.
// Keeps GPA3..GPA7 unchanged.
bool MCPDriver::writeMainTmuxAddress(uint8_t sel) {
  if (!haveMain_) return false;

  uint8_t oldA = 0;
  if (!readReg8_OK_(cfg::mcp::MCP_MAIN_ADDRESS, REG_OLATA, oldA)) {
    if (!readReg8_OK_(cfg::mcp::MCP_MAIN_ADDRESS, REG_GPIOA, oldA)) {
      return false;
    }
  }

  const uint8_t newA = static_cast<uint8_t>((oldA & static_cast<uint8_t>(~0x07u)) |
                                            (sel & 0x07u));
  if (newA == oldA) {
    return true;
  }

  return writeReg8_(cfg::mcp::MCP_MAIN_ADDRESS, REG_OLATA, newA);
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
    Serial.print(F("MCPDriver: Processing interrupt for addr=0x"));
    Serial.println(addr, HEX);
    util::UIConsole::log("Processing interrupt for addr=0x" + String(addr, HEX),
                         "MCPDriver");
  }

  uint16_t intf = 0;
  if (!readRegPair16_OK_(addr, REG_INTFA, intf)) {
    if (basicDebug) {
      Serial.println(F("MCPDriver: Failed to read INTF register"));
      util::UIConsole::log("Failed to read INTF register", "MCPDriver");
    }
    return r;
  }

  // Debug: Show INTF register value
  if (verbose) {
    Serial.print(F("MCPDriver: INTF=0b"));
    Serial.println(intf, BIN);
    util::UIConsole::log("INTF=0b" + String(intf, BIN), "MCPDriver");
  }

  if (intf == 0) {
    // No pin reported despite the interrupt flag being set; make sure the
    // event is acknowledged to avoid getting stuck in a busy loop.
    uint16_t dummy = 0;
    (void)readRegPair16_OK_(addr, REG_GPIOA, dummy);
    if (verbose) {
      Serial.print(F("MCPDriver: flag set men INTF=0 på 0x"));
      Serial.println(addr, HEX);
      util::UIConsole::log("flag set men INTF=0 på 0x" + String(addr, HEX),
                           "MCPDriver");
    }
    return r;
  }

  uint16_t intcap = 0;
  if (!readRegPair16_OK_(addr, REG_INTCAPA, intcap)) {
    if (basicDebug) {
      Serial.println(F("MCPDriver: Failed to read INTCAP register"));
      util::UIConsole::log("Failed to read INTCAP register", "MCPDriver");
    }
    return r;
  }

  // Debug: Show INTCAP register value (after reading it)
  if (verbose) {
    Serial.print(F("MCPDriver: INTCAP=0b"));
    Serial.println(intcap, BIN);
    util::UIConsole::log("INTCAP=0b" + String(intcap, BIN), "MCPDriver");
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
    Serial.print(F("MCPDriver: INT addr=0x"));
    Serial.print(addr, HEX);
    Serial.print(F(" pin="));
    Serial.print(r.pin);
    Serial.print(F(" level="));
    Serial.println(r.level ? F("HIGH") : F("LOW"));
    util::UIConsole::log("INT 0x" + String(addr, HEX) + " pin=" + String(r.pin) +
                             " level=" + String(r.level ? "HIGH" : "LOW"),
                         "MCPDriver");
  }

  // SLIC: map to line (do not return here)
  if (addr == cfg::mcp::MCP_SLIC1_ADDRESS || addr == cfg::mcp::MCP_SLIC2_ADDRESS) {
    int8_t line = mapSlicPinToLine_(addr, r.pin);
    r.line = (line >= 0) ? static_cast<uint8_t>(line) : 255;
  }

  // Sync DEFVAL to the captured level (MAIN only; requires INTCON=1 for the pin)
  // BUT NOT for STD pin which uses pure CHANGE mode
  if (addr == cfg::mcp::MCP_MAIN_ADDRESS && r.pin != cfg::mcp::STD) {
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

// Control power to the MT8816 device via MCP_MAIN
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
  auto& settings = Settings::instance();

  // Build SHK bitmasks for this specific SLIC expander.
  // Desired behaviour: any SHK edge should assert INT (CHANGE mode).
  uint8_t shkMaskA = 0;
  uint8_t shkMaskB = 0;
  for (uint8_t i=0; i<sizeof(cfg::mcp::SHK_PINS)/sizeof(cfg::mcp::SHK_PINS[0]); ++i) {
    if (cfg::mcp::SHK_LINE_ADDR[i] != i2cAddr) continue;

    const uint8_t p = cfg::mcp::SHK_PINS[i];
    if (p < 8) shkMaskA |= (1u << p);
    else       shkMaskB |= (1u << (p - 8));
  }

  uint8_t gpintena = 0, gpintenb = 0;
  uint8_t intcona  = 0, intconb  = 0;
  uint8_t gppua    = 0, gppub    = 0;
  (void)readReg8_OK_(i2cAddr, REG_GPINTENA, gpintena);
  (void)readReg8_OK_(i2cAddr, REG_GPINTENB, gpintenb);
  (void)readReg8_OK_(i2cAddr, REG_INTCONA,  intcona);
  (void)readReg8_OK_(i2cAddr, REG_INTCONB,  intconb);
  (void)readReg8_OK_(i2cAddr, REG_GPPUA,    gppua);
  (void)readReg8_OK_(i2cAddr, REG_GPPUB,    gppub);

  // SHK pins in CHANGE mode: INTCON bit = 0
  intcona &= static_cast<uint8_t>(~shkMaskA);
  intconb &= static_cast<uint8_t>(~shkMaskB);

  // Enable pull-ups + interrupt-on-change on SHK pins.
  gppua    |= shkMaskA;
  gppub    |= shkMaskB;
  gpintena |= shkMaskA;
  gpintenb |= shkMaskB;

  (void)writeReg8_(i2cAddr, REG_INTCONA, intcona);
  (void)writeReg8_(i2cAddr, REG_INTCONB, intconb);

  // DEFVAL is not used in CHANGE mode (INTCON=0), but keep deterministic values.
  (void)writeReg8_(i2cAddr, REG_DEFVALA, 0x00);
  (void)writeReg8_(i2cAddr, REG_DEFVALB, 0x00);

  (void)writeReg8_(i2cAddr, REG_GPPUA, gppua);
  (void)writeReg8_(i2cAddr, REG_GPPUB, gppub);

  (void)writeReg8_(i2cAddr, REG_GPINTENA, gpintena);
  (void)writeReg8_(i2cAddr, REG_GPINTENB, gpintenb);

  // Clear any pending capture before runtime.
  (void)mcp.readGPIOAB();

  if (settings.debugMCPLevel >= 1) {
    Serial.printf("MCPDriver: SLIC 0x%02X SHK INT cfg GPINTEN(A/B)=0x%02X/0x%02X, GPPU(A/B)=0x%02X/0x%02X\n",
                  i2cAddr, gpintena, gpintenb, gppua, gppub);
    util::UIConsole::log("SLIC 0x" + String(i2cAddr, HEX) +
                         " SHK INT cfg GPINTEN(A/B)=0x" + String(gpintena, HEX) + "/0x" + String(gpintenb, HEX) +
                         ", GPPU(A/B)=0x" + String(gppua, HEX) + "/0x" + String(gppub, HEX),
                         "MCPDriver");
  }
}

// Enable interrupts for MCP_MAIN (e.g. function button and MT8870 STD)
void MCPDriver::enableMainInterrupts_(uint8_t i2cAddr, Adafruit_MCP23X17& mcp) {
  auto& settings = Settings::instance();

  if (settings.debugMCPLevel >= 1){
    Serial.println(F("MCPDriver: Configuring MCP_MAIN interrupts..."));
    util::UIConsole::log("Configuring MCP_MAIN interrupts...", "MCPDriver");
  }

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

  // ---- MT8870 STD: CHANGE mode (INTCON=0). Edge detection done in ToneReader ----
  {
    uint8_t p = cfg::mcp::STD; // GPB3 according to config
    if (settings.debugMCPLevel >= 1) {
      Serial.print(F("Configuring MT8870 STD on pin "));
      Serial.print(p);
      Serial.println(F(" (CHANGE mode)"));
      util::UIConsole::log("Configuring MT8870 STD on pin " + String(p) +
                          " (CHANGE mode)", "MCPDriver");
    }
    if (p < 8) {
      gpintena |= (1u << p);
      gppua    |= (1u << p);  // håll linjen stabilt hög när MT8870 släpper STD
      intcona  &= ~(1u << p);  // CHANGE mode (disable compare-to-DEFVAL)
    } else {
      uint8_t bit = p - 8;
      gpintenb |= (1u << bit);
      gppub    |= (1u << bit); // håll linjen stabilt hög när MT8870 släpper STD
      intconb  &= ~(1u << bit); // CHANGE mode (disable compare-to-DEFVAL)
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

  if (settings.debugMCPLevel >= 2) {
    Serial.println(F("MCPDriver: MCP_MAIN interrupt configuration:"));
    util::UIConsole::log("MCP_MAIN interrupt configuration:", "MCPDriver");
    Serial.print(F("MCPDriver: GPINTENA=0b"));
    Serial.print(gpintena, BIN);
    Serial.print(F(" GPINTENB=0b"));
    Serial.println(gpintenb, BIN);
    Serial.print(F("MCPDriver: INTCONA=0b"));
    Serial.print(intcona, BIN);
    Serial.print(F(" INTCONB=0b"));
    Serial.println(intconb, BIN);
    util::UIConsole::log("INTCONA=0b" + String(intcona, BIN) +
                        " INTCONB=0b" + String(intconb, BIN), "MCPDriver");
  }

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

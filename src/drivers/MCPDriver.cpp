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

// Splits a table of PinModeEntry into separate mode and initial value arrays
static void splitPinTable(const cfg::mcp::PinModeEntry (&tbl)[16], uint8_t (&modes)[16], bool (&initial)[16]) {
  for (int i=0;i<16;i++){ modes[i]=tbl[i].mode; initial[i]=tbl[i].initial; }
}

// Writes an 8-bit value to a register via I2C
bool MCPDriver::writeReg8_(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg); Wire.write(val);
  return Wire.endTransmission() == 0;
}

// Reads an 8-bit value from a register via I2C, returns success status
bool MCPDriver::readReg8_OK_(uint8_t addr, uint8_t reg, uint8_t& out) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)addr, 1) != 1) return false;
  out = Wire.read();
  return true;
}

// Reads a 16-bit value from two consecutive registers via I2C, returns success status
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

// Reads an 8-bit value from a register via I2C (no error checking)
uint8_t MCPDriver::readReg8_(uint8_t addr, uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom((int)addr, 1);
  return Wire.available() ? Wire.read() : 0;
}

// Reads a 16-bit value from two consecutive registers via I2C (no error checking)
void MCPDriver::readRegPair16_(uint8_t addr, uint8_t regA, uint16_t& out16) {
  Wire.beginTransmission(addr);
  Wire.write(regA);
  Wire.endTransmission(false);
  Wire.requestFrom((int)addr, 2);
  uint8_t a = Wire.available() ? Wire.read() : 0;
  uint8_t b = Wire.available() ? Wire.read() : 0;
  out16 = (uint16_t)a | ((uint16_t)b<<8);
}

// Initializes all MCP chips and configures their pins and interrupts
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

  // Lambda to program IOCON register
  auto programIOCON = [&](uint8_t addr, uint8_t iocon){
    return writeReg8_(addr, REG_IOCON, iocon) && writeReg8_(addr, REG_IOCON+1, iocon);
  };

  // Configure MCP_MAIN pins
  if (haveMain_) {
    uint8_t modes[16]; bool initial[16];
    splitPinTable(mcp::MCP_MAIN, modes, initial);
    if (!applyPinModes_(mcpMain_, modes, initial)) return false;
    programIOCON(cfg::mcp::MCP_MAIN_ADDRESS, 0x44);   // MAIN: aktiv låg, open-drain, mirror
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
  }
  // Attach interrupts for MCP_SLIC1
  if (haveSlic1_) {
    pinMode(mcp::MCP_SLIC_INT_1_PIN, INPUT_PULLUP);
    attachInterruptArg(digitalPinToInterrupt(mcp::MCP_SLIC_INT_1_PIN),
                       &MCPDriver::isrSlic1Thunk, this, FALLING);
  }
  // Attach interrupts for MCP_SLIC2
  if (haveSlic2_) {
    pinMode(mcp::MCP_SLIC_INT_2_PIN, INPUT_PULLUP);
    attachInterruptArg(digitalPinToInterrupt(mcp::MCP_SLIC_INT_2_PIN),
                       &MCPDriver::isrSlic2Thunk, this, FALLING);
  }
  return true;
}

// Writes a digital value to a pin on the specified MCP device
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

// Reads a digital value from a pin on the specified MCP device
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

// Interrupt service routines for each MCP device
void IRAM_ATTR MCPDriver::isrMainThunk(void* arg)   { reinterpret_cast<MCPDriver*>(arg)->mainIntFlag_   = true; }
void IRAM_ATTR MCPDriver::isrSlic1Thunk(void* arg)  { reinterpret_cast<MCPDriver*>(arg)->slic1IntFlag_  = true; }
void IRAM_ATTR MCPDriver::isrSlic2Thunk(void* arg)  { reinterpret_cast<MCPDriver*>(arg)->slic2IntFlag_  = true; }
void IRAM_ATTR MCPDriver::isrMT8816Thunk(void* arg) { reinterpret_cast<MCPDriver*>(arg)->mt8816IntFlag_ = true; }

// Handles interrupts for MCP_MAIN
IntResult MCPDriver::handleMainInterrupt()   {
  if (!haveMain_) return {};
  return handleInterrupt_(mainIntFlag_,   mcpMain_,   mcp::MCP_MAIN_ADDRESS);
}
// Handles interrupts for MCP_SLIC1
IntResult MCPDriver::handleSlic1Interrupt()  {
  if (!haveSlic1_) return {};
  IntResult r = handleInterrupt_(slic1IntFlag_,  mcpSlic1_,  mcp::MCP_SLIC1_ADDRESS);
  return r;
}

// Handles interrupts for MCP_SLIC2
IntResult MCPDriver::handleSlic2Interrupt()  {
  if (!haveSlic2_) return {};
  IntResult r = handleInterrupt_(slic2IntFlag_,  mcpSlic2_,  mcp::MCP_SLIC2_ADDRESS);
  return r;
}
// Handles interrupts for MCP_MT8816

IntResult MCPDriver::handleMT8816Interrupt() {
  if (!haveMT8816_) return {};
  return handleInterrupt_(mt8816IntFlag_, mcpMT8816_, mcp::MCP_MT8816_ADDRESS);
}

// Handles an interrupt for a specific MCP device, returns interrupt result
IntResult MCPDriver::handleInterrupt_(volatile bool& flag, Adafruit_MCP23X17& mcp, uint8_t addr) {
  bool fired=false;
  noInterrupts();
  fired = flag;
  flag  = false;
  interrupts();

  IntResult r;
  r.i2c_addr = addr;
  if (!fired) return r;

  int8_t pin = mcp.getLastInterruptPin();
  if (pin >= 0 && pin <= 15) {
    r.pin = static_cast<uint8_t>(pin);

    uint16_t intcap = 0;
    if (!readRegPair16_OK_(addr, REG_INTCAPA, intcap)) return r;

    r.level    = (intcap & (1u << r.pin)) != 0;
    r.hasEvent = true;

    // SLIC: mappa till line (ingen return här)
    if (addr == cfg::mcp::MCP_SLIC1_ADDRESS || addr == cfg::mcp::MCP_SLIC2_ADDRESS) {
      int8_t line = mapSlicPinToLine_(addr, r.pin);
      r.line = (line >= 0) ? static_cast<uint8_t>(line) : 255;
    }

    // Extra clear efter INTCAP (oskyldigt, men gör kvittering superstabil)
    uint16_t dummy = 0;
    (void)readRegPair16_OK_(addr, REG_GPIOA, dummy);

    // Synka DEFVAL till fångad nivå (endast MAIN; kräver INTCON=1 på pinnen)
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

  // Fallback om "last pin" inte fås från drivrutinen
  r = readIntfIntcapFallback_(addr);
  return r;
}

// Fallback interrupt handler: scans interrupt flags and captures pin info
IntResult MCPDriver::readIntfIntcapFallback_(uint8_t addr) {
  IntResult r; r.i2c_addr = addr;
  uint16_t intf=0, intcap=0;

  if (!readRegPair16_OK_(addr, REG_INTFA,   intf))   return r;
  if (!readRegPair16_OK_(addr, REG_INTCAPA, intcap)) return r;
  if (intf==0) return r;

  for (uint8_t p=0; p<16; ++p) {
    if (intf & (1u<<p)) {
      r.pin      = p;
      r.level    = (intcap & (1u<<p)) != 0;
      r.hasEvent = true;
      break;
    }
  }

  (void)readRegPair16_OK_(addr, REG_GPIOA, intcap);
  return r;
}

// Maps a SLIC pin to its corresponding line number
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

// Applies pin modes and initial values to an MCP device
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

// Enables interrupts for SLIC shake pins and configures pull-ups
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

// Aktiverar interrupts för MCP_MAIN, ex. knapp och MT8870 STD
void MCPDriver::enableMainInterrupts_(uint8_t i2cAddr, Adafruit_MCP23X17& mcp) {
  uint8_t gpintena=0, gpintenb=0;
  uint8_t intcona=0, intconb=0;
  uint8_t gppua=0, gppub=0;
  uint8_t defvala=0, defvalb=0;

  // Läs befintliga värden
  (void)readReg8_OK_(i2cAddr, REG_GPINTENA, gpintena);
  (void)readReg8_OK_(i2cAddr, REG_GPINTENB, gpintenb);
  (void)readReg8_OK_(i2cAddr, REG_INTCONA,  intcona);
  (void)readReg8_OK_(i2cAddr, REG_INTCONB,  intconb);
  (void)readReg8_OK_(i2cAddr, REG_GPPUA,    gppua);
  (void)readReg8_OK_(i2cAddr, REG_GPPUB,    gppub);
  (void)readReg8_OK_(i2cAddr, REG_DEFVALA,  defvala);
  (void)readReg8_OK_(i2cAddr, REG_DEFVALB,  defvalb);

  // ---- TEST_BUTTON: CHANGE-läge med pull-up ----
  {
    uint8_t p = cfg::mcp::TEST_BUTTON;
    if (p < 8) {
      gpintena |= (1u << p);   // enable INT
      gppua    |= (1u << p);   // pull-up
      intcona  &= ~(1u << p);  // CHANGE mode (ignorera DEFVAL)
    } else {
      uint8_t bit = p - 8;
      gpintenb |= (1u << bit); // enable INT
      gppub    |= (1u << bit); // pull-up
      intconb  &= ~(1u << bit); // CHANGE mode
    }
  }

  // ---- MT8870 STD: compare-to-DEFVAL (INTCON=1). DEFVAL synkas i handleInterrupt_ ----
  {
    uint8_t p = cfg::mcp::STD; // GPB3 enligt konfig
    if (p < 8) {
      gpintena |= (1u << p);
      // STD drivs av MT8870 => pull-up ofta onödig, låt gppua vara oförändrad
      intcona  |= (1u << p);  // enable compare-to-DEFVAL
      // DEFVALA lämnas orörd här; uppdateras av handleInterrupt_ efter varje fångad nivå
    } else {
      uint8_t bit = p - 8;
      gpintenb |= (1u << bit);
      // STD drivs av MT8870 => lämna gppub som är
      intconb  |= (1u << bit); // enable compare-to-DEFVAL
      // DEFVALB uppdateras dynamiskt av handleInterrupt_
    }
  }

  // Skriv tillbaka
  writeReg8_(i2cAddr, REG_GPPUA, gppua);
  writeReg8_(i2cAddr, REG_GPPUB, gppub);

  writeReg8_(i2cAddr, REG_INTCONA, intcona);
  writeReg8_(i2cAddr, REG_INTCONB, intconb);

  writeReg8_(i2cAddr, REG_DEFVALA, defvala);
  writeReg8_(i2cAddr, REG_DEFVALB, defvalb);

  writeReg8_(i2cAddr, REG_GPINTENA, gpintena);
  writeReg8_(i2cAddr, REG_GPINTENB, gpintenb);

  // Kvittera eventuella hängande flaggor (läs INTCAP följt av GPIO)
  uint16_t dummy=0;
  (void)readRegPair16_OK_(i2cAddr, REG_INTCAPA, dummy);
  (void)readRegPair16_OK_(i2cAddr, REG_GPIOA,   dummy);
}

// Reads the 16-bit GPIO value from both ports of an MCP device
bool MCPDriver::readGpioAB16(uint8_t addr, uint16_t& out16) {
  uint8_t a = readReg8_(addr, 0x12);
  uint8_t b = readReg8_(addr, 0x13);
  out16 = (uint16_t(b) << 8) | uint16_t(a);
  return true;
}

// Probes an MCP device at the given address, returns true if found
bool MCPDriver::probeMcp_(Adafruit_MCP23X17& mcp, uint8_t addr) {
  if (!mcp.begin_I2C(addr)) return false;
  return true;
}
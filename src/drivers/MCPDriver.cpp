
#include "drivers/MCPDriver.h"
using namespace cfg;

// ===== Register-adresser för MCP23X17 (BANK=0) =====
static constexpr uint8_t REG_IOCON     = 0x0A; // även 0x0B
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

// Helpers för att plocka ut mode/initial från dina arrayer
static void splitPinTable(const cfg::mcp::PinModeEntry (&tbl)[16], uint8_t (&modes)[16], bool (&initial)[16]) {
  for (int i=0;i<16;i++){ modes[i]=tbl[i].mode; initial[i]=tbl[i].initial; }
}


// ===== [NYTT] Säkra I2C-hjälpare som returnerar bool =====
bool MCPDriver::writeReg8_(uint8_t addr, uint8_t reg, uint8_t val) {

  Wire.beginTransmission(addr);
  Wire.write(reg); Wire.write(val);
  return Wire.endTransmission() == 0; // 0 = success
}
bool MCPDriver::readReg8_OK_(uint8_t addr, uint8_t reg, uint8_t& out) {

  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)addr, 1) != 1) return false;
  out = Wire.read();
  return true;
}
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

// ===== [KVAR] Original enkla helpers (behålls för kompatibilitet internt) =====
uint8_t MCPDriver::readReg8_(uint8_t addr, uint8_t reg) {

  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom((int)addr, 1);
  return Wire.available() ? Wire.read() : 0;
}
void MCPDriver::readRegPair16_(uint8_t addr, uint8_t regA, uint16_t& out16) {

  Wire.beginTransmission(addr);
  Wire.write(regA);
  Wire.endTransmission(false);
  Wire.requestFrom((int)addr, 2);
  uint8_t a = Wire.available() ? Wire.read() : 0;
  uint8_t b = Wire.available() ? Wire.read() : 0;
  out16 = (uint16_t)a | ((uint16_t)b<<8);
}

bool MCPDriver::begin() {
  // Kort I2C-timeout så vi inte låser oss
  Wire.setTimeOut(50);
  auto& settings = Settings::instance();
  haveMain_ = haveSlic1_ = haveSlic2_ = haveMT8816_ = false;

  // 1) Försök hitta alla MCP:er
  haveSlic1_  = probeMcp_(mcpSlic1_,  cfg::mcp::MCP_SLIC1_ADDRESS);
  haveSlic2_  = probeMcp_(mcpSlic2_,  cfg::mcp::MCP_SLIC2_ADDRESS);
  haveMain_   = probeMcp_(mcpMain_,   cfg::mcp::MCP_MAIN_ADDRESS);
  haveMT8816_ = probeMcp_(mcpMT8816_, cfg::mcp::MCP_MT8816_ADDRESS);

  // Spara utfallet i Settings så alla andra delar kan läsa det
  settings.mcpSlic1Present  = haveSlic1_;
  settings.mcpSlic2Present = haveSlic2_;
  settings.mcpMainPresent  = haveMain_;
  settings.mcpMt8816Present = haveMT8816_;

  Serial.println(F("MCP init:"));
  if (haveMain_)   Serial.println(F(" - MCP_MAIN   hittad")); else Serial.println(F(" - MCP_MAIN   saknas"));
  if (haveSlic1_)  Serial.println(F(" - MCP_SLIC1  hittad")); else Serial.println(F(" - MCP_SLIC1  saknas"));
  if (haveSlic2_)  Serial.println(F(" - MCP_SLIC2  hittad")); else Serial.println(F(" - MCP_SLIC2  saknas"));
  if (haveMT8816_) Serial.println(F(" - MCP_MT8816 hittad")); else Serial.println(F(" - MCP_MT8816 saknas"));

  if (!(haveMain_ || haveSlic1_ || haveSlic2_ || haveMT8816_ )) {
    Serial.println(F("Ingen MCP hittades, avbryter."));
    return false;
  }

  // 2) Sätt pin-lägen utifrån dina tabeller + grundläggande IOCON
  auto programIOCON = [&](uint8_t addr){
    // Rimlig default: MIRROR=1, ODR=1, (SEQOP=1 ok). Skriv båda IOCON-reg.
    const uint8_t iocon = 0x4A; // 0b01001010
    return writeReg8_(addr, REG_IOCON, iocon) && writeReg8_(addr, REG_IOCON+1, iocon);
  };

  if (haveMain_) {
    uint8_t modes[16]; bool initial[16];
    splitPinTable(mcp::MCP_MAIN, modes, initial);
    if (!applyPinModes_(mcpMain_, modes, initial)) return false;
    (void)programIOCON(mcp::MCP_MAIN_ADDRESS);
  }
  if (haveSlic1_) {
    uint8_t modes[16]; bool initial[16];
    splitPinTable(mcp::MCP_SLIC, modes, initial);
    if (!applyPinModes_(mcpSlic1_, modes, initial)) return false;
    // Vi kommer strax överskriva IOCON för SLIC1 mer strikt (0x44).
  }
  if (haveSlic2_) {
    uint8_t modes[16]; bool initial[16];
    splitPinTable(mcp::MCP_SLIC, modes, initial);
    if (!applyPinModes_(mcpSlic2_, modes, initial)) return false;
  }

  if (haveMT8816_) {
    uint8_t modes[16]; bool initial[16];
    splitPinTable(mcp::MCP_MT8816, modes, initial);
    if (!applyPinModes_(mcpMT8816_, modes, initial)) return false;
    (void)programIOCON(mcp::MCP_MT8816_ADDRESS);
  }

  // 3) SLIC1: Tvångssätt IOCON (BANK-säkert) + enable SHK-INT + rensa latch
  if (haveSlic1_) {
    // a) Sätt IOCON exakt = 0x44 (MIRROR=1, ODR=1). Skriv båda adresserna!
    writeReg8_(mcp::MCP_SLIC1_ADDRESS, REG_IOCON,     0x44);
    writeReg8_(mcp::MCP_SLIC1_ADDRESS, REG_IOCON + 1, 0x44);

    // b) Enable:a alla dina SHK-pinnar (CHANGE + pull-ups) via din helper
    enableSlicShkInterrupts_(cfg::mcp::MCP_SLIC1_ADDRESS, mcpSlic1_);
    
    // c) Rensa ev. latched status så INT re-armas från ren start
    uint16_t dummy;
    (void)readRegPair16_OK_(mcp::MCP_SLIC1_ADDRESS, REG_INTCAPA, dummy); // läs INTCAP A/B
    (void)readRegPair16_OK_(mcp::MCP_SLIC1_ADDRESS, REG_GPIOA,   dummy); // extra kvittens
  }

  if (haveSlic2_) {
    // Sätt IOCON till 0x44 (MIRROR=1, ODR=1) för båda bankerna
    writeReg8_(mcp::MCP_SLIC2_ADDRESS, REG_IOCON,     0x44);
    writeReg8_(mcp::MCP_SLIC2_ADDRESS, REG_IOCON + 1, 0x44);

    // Aktivera SLIC‑SHK‑avbrott och rensa latch
    enableSlicShkInterrupts_(cfg::mcp::MCP_SLIC2_ADDRESS, mcpSlic2_);

    uint16_t dummy;
    (void)readRegPair16_OK_(mcp::MCP_SLIC2_ADDRESS, REG_INTCAPA, dummy);
    (void)readRegPair16_OK_(mcp::MCP_SLIC2_ADDRESS, REG_GPIOA,   dummy);
  }
  // 4) Koppla ESP32-interrupts (fallande flank) endast för kretsar som finns
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
  return true;
}

// ===== Basala GPIO-funktioner =====
bool MCPDriver::digitalWriteMCP(uint8_t addr, uint8_t pin, bool value) {
  // [NYTT] Respektera närvaro — undvik I2C-timeouts mot saknade chips
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

bool MCPDriver::digitalReadMCP(uint8_t addr, uint8_t pin, bool& out) {
  // [NYTT] Respektera närvaro
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

// ===== ISR-thunks =====
void IRAM_ATTR MCPDriver::isrMainThunk(void* arg)   { reinterpret_cast<MCPDriver*>(arg)->mainIntFlag_   = true; }
void IRAM_ATTR MCPDriver::isrSlic1Thunk(void* arg)  { reinterpret_cast<MCPDriver*>(arg)->slic1IntFlag_  = true; }
void IRAM_ATTR MCPDriver::isrSlic2Thunk(void* arg)  { reinterpret_cast<MCPDriver*>(arg)->slic2IntFlag_  = true; }
void IRAM_ATTR MCPDriver::isrMT8816Thunk(void* arg) { reinterpret_cast<MCPDriver*>(arg)->mt8816IntFlag_ = true; }

// ===== Loop-hanterare =====
IntResult MCPDriver::handleMainInterrupt()   {
  if (!haveMain_) return {}; // [NYTT]
  return handleInterrupt_(mainIntFlag_,   mcpMain_,   mcp::MCP_MAIN_ADDRESS);
}
IntResult MCPDriver::handleSlic1Interrupt()  {
  if (!haveSlic1_) return {};
  IntResult r = handleInterrupt_(slic1IntFlag_,  mcpSlic1_,  mcp::MCP_SLIC1_ADDRESS);
  return r;
}

IntResult MCPDriver::handleSlic2Interrupt()  {
  if (!haveSlic2_) return {};
  IntResult r = handleInterrupt_(slic2IntFlag_,  mcpSlic2_,  mcp::MCP_SLIC2_ADDRESS);
  return r;
}
IntResult MCPDriver::handleMT8816Interrupt() {
  if (!haveMT8816_) return {}; // [NYTT]
  return handleInterrupt_(mt8816IntFlag_, mcpMT8816_, mcp::MCP_MT8816_ADDRESS);
}

IntResult MCPDriver::handleInterrupt_(volatile bool& flag, Adafruit_MCP23X17& mcp, uint8_t addr) {
  // Ta en atomisk snapshot av flaggan
  bool fired=false;
  noInterrupts();
  fired = flag; flag = false;
  interrupts();
  IntResult r; r.i2c_addr = addr;
  if (!fired) return r;

  int8_t pin = mcp.getLastInterruptPin();    // -1 om okänt
  if (pin >= 0 && pin <= 15) {
    r.pin      = static_cast<uint8_t>(pin);

    // Läs INTCAP för att få värdet och samtidigt kvittera
    uint16_t intcap=0;
    // [NYTT] robust läsning; om I2C fel -> avbryt snyggt
    if (!readRegPair16_OK_(addr, REG_INTCAPA, intcap)) return r;
    r.level    = (intcap & (1u<<pin)) != 0;
    r.hasEvent = true;

    if (addr == cfg::mcp::MCP_SLIC1_ADDRESS || addr == cfg::mcp::MCP_SLIC2_ADDRESS) {
      int8_t line = mapSlicPinToLine_(addr, r.pin);
      r.line = (line >= 0) ? static_cast<uint8_t>(line) : 255;
    }
    return r;
  }

  // Fallback: läs INTF/INTCAP manuellt
  r = readIntfIntcapFallback_(addr); // kvitterar också
  return r;
}

IntResult MCPDriver::readIntfIntcapFallback_(uint8_t addr) {
  IntResult r; r.i2c_addr = addr;
  uint16_t intf=0, intcap=0;

  // [NYTT] robust läsning
  if (!readRegPair16_OK_(addr, REG_INTFA,   intf))   return r; // INTFA/INTFB
  if (!readRegPair16_OK_(addr, REG_INTCAPA, intcap)) return r; // INTCAPA/INTCAPB (läser och kvitterar)
  if (intf==0) return r;

  // Ta första satta biten (om flera samtidigt – förenklad hantering)
  for (uint8_t p=0; p<16; ++p) {
    if (intf & (1u<<p)) {
      r.pin      = p;
      r.level    = (intcap & (1u<<p)) != 0;
      r.hasEvent = true;
      break;
    }
  }
  // Extra: läs GPIO för säker kvittens (best-effort)
  (void)readRegPair16_OK_(addr, REG_GPIOA, intcap);
  return r;
}

int8_t MCPDriver::mapSlicPinToLine_(uint8_t addr, uint8_t pin) const {
  for (uint8_t line = 0; line < cfg::mcp::SHK_LINE_COUNT; ++line) {
    if (cfg::mcp::SHK_LINE_ADDR[line] == addr &&
        cfg::mcp::SHK_PINS[line]      == pin) {
      return static_cast<int8_t>(line);
    }
  }
  return -1;
}


bool MCPDriver::applyPinModes_(Adafruit_MCP23X17& mcp, const uint8_t (&modes)[16], const bool (&initial)[16]) {
  // Sätt lägen
  for (uint8_t p=0;p<16;p++) {
    switch (modes[p]) {
      case INPUT:
        mcp.pinMode(p, INPUT);
        mcp.digitalWrite(p, LOW);   // pull-up OFF
        break;
      case INPUT_PULLUP:
        mcp.pinMode(p, INPUT);
        mcp.digitalWrite(p, HIGH);  // aktivera pull-up i äldre bibliotek
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

void MCPDriver::enableSlicShkInterrupts_(uint8_t i2cAddr, Adafruit_MCP23X17& mcp) {
  // Aktivera INT på alla SHK-pinnar och sätt CHANGE (INTCON=0)
  // – dvs jämförelse mot föregående värde, inte mot DEFVAL.
  // Samtidigt: om någon SHK är INPUT utan pullup, på med pullup för stabilitet.
  uint8_t gpintena=0, gpintenb=0;
  uint8_t intcona=0, intconb=0; // 0 = jämför med föregående (CHANGE)

  // [NYTT] Läs befintliga pullups robust
  uint8_t gppua=0, gppub=0;
  (void)readReg8_OK_(i2cAddr, REG_GPPUA, gppua);
  (void)readReg8_OK_(i2cAddr, REG_GPPUB, gppub);

  for (uint8_t i=0; i<sizeof(mcp::SHK_PINS)/sizeof(mcp::SHK_PINS[0]); ++i) {
    uint8_t p = mcp::SHK_PINS[i]; // 0..15
    if (p<8) gpintena |= (1u<<p);
    else     gpintenb |= (1u<<(p-8));
    // Sätt pull-up (aktiv låg SHK enligt din kommentar)
    if (p<8) gppua |= (1u<<p); else gppub |= (1u<<(p-8));
  }

  // Skriv INTCON=0 för båda portar (CHANGE)
  writeReg8_(i2cAddr, REG_INTCONA, intcona);
  writeReg8_(i2cAddr, REG_INTCONB, intconb);

  // DEFVAL ointressant för CHANGE, men nollställ för tydlighet
  writeReg8_(i2cAddr, REG_DEFVALA, 0x00);
  writeReg8_(i2cAddr, REG_DEFVALB, 0x00);

  // Pullups
  writeReg8_(i2cAddr, REG_GPPUA, gppua);
  writeReg8_(i2cAddr, REG_GPPUB, gppub);

  // Slå på GPINTEN
  writeReg8_(i2cAddr, REG_GPINTENA, gpintena);
  writeReg8_(i2cAddr, REG_GPINTENB, gpintenb);

  // Läs INTCAP för att rensa ev. gamla tillstånd
  (void)mcp.readGPIOAB();
}

bool MCPDriver::readGpioAB16(uint8_t addr, uint16_t& out16) {
  uint8_t a = readReg8_(addr, 0x12);
  uint8_t b = readReg8_(addr, 0x13);
  out16 = (uint16_t(b) << 8) | uint16_t(a);
  return true; // lägg gärna verklig felhantering
}

bool MCPDriver::probeMcp_(Adafruit_MCP23X17& mcp, uint8_t addr) {
  // Adafruit_MCP23X17::begin_I2C returnerar bool; hantera fel säkert
  if (!mcp.begin_I2C(addr)) return false;
  return true;
}

#include "MCPDriver.h"
#include "config.h"   // använder dina konstanter & pin-tabeller
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

bool MCPDriver::begin() {
  // 1) Hitta/enabla alla MCP:er via begin_I2C
  bool ok = true;
  ok &= mcpMain_.begin_I2C (mcp::MCP_MAIN_ADDRESS);
  ok &= mcpSlic1_.begin_I2C(mcp::MCP_SLIC1_ADDRESS);
  ok &= mcpMT8816_.begin_I2C(mcp::MCP_MT8816_ADDRESS);
  if (!ok) return false;  // ”Hitta och returnera true om MCP hittades” uppfylls här. :contentReference[oaicite:1]{index=1}

  // 2) Ställ in GPIO-egenskaper för alla enligt config.h-tabeller
  {
    uint8_t modes[16]; bool initial[16];
    splitPinTable(mcp::MCP_MAIN, modes, initial);
    if (!applyPinModes_(mcpMain_, modes, initial)) return false;  // MAIN  :contentReference[oaicite:2]{index=2}
    splitPinTable(mcp::MCP_SLIC, modes, initial);
    if (!applyPinModes_(mcpSlic1_, modes, initial)) return false; // SLIC1 :contentReference[oaicite:3]{index=3}
    splitPinTable(mcp::MCP_MT8816, modes, initial);
    if (!applyPinModes_(mcpMT8816_, modes, initial)) return false;// MT8816 :contentReference[oaicite:4]{index=4}
  }

  // 3) INT-egenskaper: INTA/INTB ihop (MIRROR) + open drain (ODR)
  // Adafruit-biblioteket har setMirror/setOpenDrain/setIntPolarity etc på nyare versioner,
  // men för portabelhet sätter vi IOCON direkt: ODR=1, MIRROR=1, INT active low.
  auto programIOCON = [&](uint8_t addr){
    // Bit: BANK=0/MIRROR=1/SEQOP=1(behåll)/DISSLW=0/HAEN=0/ODR=1/INTPOL=0
    uint8_t iocon = 0b01001010; // MIRROR=1, ODR=1, övriga rimliga default
    Wire.beginTransmission(addr);
    Wire.write(REG_IOCON); Wire.write(iocon); Wire.endTransmission();
    Wire.beginTransmission(addr);
    Wire.write(REG_IOCON+1); Wire.write(iocon); Wire.endTransmission(); // dublett
  };
  programIOCON(mcp::MCP_MAIN_ADDRESS);
  programIOCON(mcp::MCP_SLIC1_ADDRESS);
  programIOCON(mcp::MCP_MT8816_ADDRESS);

  // 4) Aktivera INT på valda GPIO (SLIC SHK-pinnarna) och CHANGE-trigger
  enableSlicShkInterrupts_(mcpSlic1_); // använder cfg::mcp::SHK_PINS[] :contentReference[oaicite:5]{index=5}

  // 5) ESP32-GPIO för INT-ingångar + attachInterrupt (fallande flank)
  pinMode(mcp::MCP_MAIN_INT_PIN,   INPUT_PULLUP);
  pinMode(mcp::MCP_SLIC_INT_1_PIN, INPUT_PULLUP);
  pinMode(mcp::MCP_SLIC_INT_2_PIN, INPUT_PULLUP); // om du använder den  :contentReference[oaicite:6]{index=6}

  attachInterruptArg(digitalPinToInterrupt(mcp::MCP_MAIN_INT_PIN),
                     &MCPDriver::isrMainThunk,  this, FALLING);
  attachInterruptArg(digitalPinToInterrupt(mcp::MCP_SLIC_INT_1_PIN),
                     &MCPDriver::isrSlic1Thunk, this, FALLING);
  attachInterruptArg(digitalPinToInterrupt(mcp::MCP_SLIC_INT_2_PIN),
                     &MCPDriver::isrMT8816Thunk, this, FALLING);

  return true;
}

// ===== Basala GPIO-funktioner =====
bool MCPDriver::digitalWriteMCP(uint8_t addr, uint8_t pin, bool value) {
  Adafruit_MCP23X17* m = nullptr;
  if (addr==mcp::MCP_MAIN_ADDRESS)   m=&mcpMain_;
  else if (addr==mcp::MCP_SLIC1_ADDRESS)  m=&mcpSlic1_;
  else if (addr==mcp::MCP_MT8816_ADDRESS) m=&mcpMT8816_;
  else return false;
  m->digitalWrite(pin, value);
  return true;
}
bool MCPDriver::digitalReadMCP(uint8_t addr, uint8_t pin, bool& out) {
  Adafruit_MCP23X17* m = nullptr;
  if (addr==mcp::MCP_MAIN_ADDRESS)   m=&mcpMain_;
  else if (addr==mcp::MCP_SLIC1_ADDRESS)  m=&mcpSlic1_;
  else if (addr==mcp::MCP_MT8816_ADDRESS) m=&mcpMT8816_;
  else return false;
  out = m->digitalRead(pin);
  return true;
}

// ===== ISR-thunks =====
void IRAM_ATTR MCPDriver::isrMainThunk(void* arg)  { reinterpret_cast<MCPDriver*>(arg)->mainIntFlag_  = true; }
void IRAM_ATTR MCPDriver::isrSlic1Thunk(void* arg){ reinterpret_cast<MCPDriver*>(arg)->slic1IntFlag_ = true; }
void IRAM_ATTR MCPDriver::isrMT8816Thunk(void* arg){reinterpret_cast<MCPDriver*>(arg)->mt8816IntFlag_= true; }

// ===== Loop-hanterare =====
IntResult MCPDriver::handleMainInterrupt()   { return handleInterrupt_(mainIntFlag_,   mcpMain_,   mcp::MCP_MAIN_ADDRESS); }
IntResult MCPDriver::handleSlic1Interrupt()  { return handleInterrupt_(slic1IntFlag_,  mcpSlic1_,  mcp::MCP_SLIC1_ADDRESS); }
IntResult MCPDriver::handleMT8816Interrupt() { return handleInterrupt_(mt8816IntFlag_, mcpMT8816_, mcp::MCP_MT8816_ADDRESS); }

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
    readRegPair16_(addr, REG_INTCAPA, intcap);
    r.level    = (intcap & (1u<<pin)) != 0;
    r.hasEvent = true;

    return r;
  }

  // Fallback: läs INTF/INTCAP manuellt
  r = readIntfIntcapFallback_(addr); // kvitterar också
  return r;
}

IntResult MCPDriver::readIntfIntcapFallback_(uint8_t addr) {
  IntResult r; r.i2c_addr = addr;
  uint16_t intf=0, intcap=0;
  readRegPair16_(addr, REG_INTFA, intf);     // INTFA/INTFB
  readRegPair16_(addr, REG_INTCAPA, intcap); // INTCAPA/INTCAPB (läser och kvitterar)
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
  // Extra: läs GPIO för säker kvittens
  (void)readRegPair16_(addr, REG_GPIOA, intcap);
  return r;
}

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

void MCPDriver::enableSlicShkInterrupts_(Adafruit_MCP23X17& mcp) {
  // Aktivera INT på alla SHK-pinnar och sätt CHANGE (INTCON=0)
  // – dvs jämförelse mot föregående värde, inte mot DEFVAL.
  // Samtidigt: om någon SHK är INPUT utan pullup, på med pullup för stabilitet.
  uint8_t gpintena=0, gpintenb=0;
  uint8_t intcona=0, intconb=0; // 0 = jämför med föregående (CHANGE)
  uint8_t gppua= readReg8_(mcp::MCP_SLIC1_ADDRESS, REG_GPPUA);
  uint8_t gppub= readReg8_(mcp::MCP_SLIC1_ADDRESS, REG_GPPUB);

  for (uint8_t i=0; i<sizeof(mcp::SHK_PINS)/sizeof(mcp::SHK_PINS[0]); ++i) {
    uint8_t p = mcp::SHK_PINS[i]; // 0..15  :contentReference[oaicite:7]{index=7}
    if (p<8) gpintena |= (1u<<p);
    else     gpintenb |= (1u<<(p-8));
    // Sätt pull-up (aktiv låg SHK enligt din kommentar)
    if (p<8) gppua |= (1u<<p); else gppub |= (1u<<(p-8));
  }

  // Skriv INTCON=0 för båda portar (CHANGE)
  Wire.beginTransmission(mcp::MCP_SLIC1_ADDRESS);
  Wire.write(REG_INTCONA); Wire.write(intcona);
  Wire.endTransmission();
  Wire.beginTransmission(mcp::MCP_SLIC1_ADDRESS);
  Wire.write(REG_INTCONB); Wire.write(intconb);
  Wire.endTransmission();

  // DEFVAL ointressant för CHANGE, men nollställ för tydlighet
  Wire.beginTransmission(mcp::MCP_SLIC1_ADDRESS);
  Wire.write(REG_DEFVALA); Wire.write(0x00);
  Wire.endTransmission();
  Wire.beginTransmission(mcp::MCP_SLIC1_ADDRESS);
  Wire.write(REG_DEFVALB); Wire.write(0x00);
  Wire.endTransmission();

  // Pullups
  Wire.beginTransmission(mcp::MCP_SLIC1_ADDRESS);
  Wire.write(REG_GPPUA); Wire.write(gppua);
  Wire.endTransmission();
  Wire.beginTransmission(mcp::MCP_SLIC1_ADDRESS);
  Wire.write(REG_GPPUB); Wire.write(gppub);
  Wire.endTransmission();

  // Slå på GPINTEN
  Wire.beginTransmission(mcp::MCP_SLIC1_ADDRESS);
  Wire.write(REG_GPINTENA); Wire.write(gpintena);
  Wire.endTransmission();
  Wire.beginTransmission(mcp::MCP_SLIC1_ADDRESS);
  Wire.write(REG_GPINTENB); Wire.write(gpintenb);
  Wire.endTransmission();

  // Läs INTCAP för att rensa ev. gamla tillstånd
  (void)mcp.readGPIOAB();
}

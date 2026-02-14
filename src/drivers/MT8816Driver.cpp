#include "MT8816Driver.h"

using namespace cfg;

MT8816Driver::MT8816Driver(MCPDriver& mcpDriver, Settings& settings) : mcpDriver_(mcpDriver), settings_(settings) {
}

void MT8816Driver::begin(){
	// Reset MCP
	reset();
	Serial.println("MT8816: Initialized successfully.");
    util::UIConsole::log("Initialized successfully.", "MT8816Driver");
}

void MT8816Driver::setConnection(uint8_t x, uint8_t y, bool state) {

    mcpDriver_.digitalWriteMCP(mcp::MCP_MT8816_ADDRESS, mcp::STROBE, LOW);
    mcpDriver_.digitalWriteMCP(mcp::MCP_MT8816_ADDRESS, mcp::CS, LOW);
    setAddress(x, y);
    delayMicroseconds(10);
    mcpDriver_.digitalWriteMCP(mcp::MCP_MT8816_ADDRESS, mcp::DATA, state ? HIGH : LOW);
    delayMicroseconds(5);
    mcpDriver_.digitalWriteMCP(mcp::MCP_MT8816_ADDRESS, mcp::CS, HIGH);
    delayMicroseconds(5);
    mcpDriver_.digitalWriteMCP(mcp::MCP_MT8816_ADDRESS, mcp::STROBE, HIGH);
    delayMicroseconds(10);
    mcpDriver_.digitalWriteMCP(mcp::MCP_MT8816_ADDRESS, mcp::STROBE, LOW);
    mcpDriver_.digitalWriteMCP(mcp::MCP_MT8816_ADDRESS, mcp::CS, LOW);

    if (settings_.debugMTLevel >= 2) {
      Serial.print("MT8816: Set connection x=");
      Serial.print(x);
      Serial.print(" y=");
      Serial.print(y);
      Serial.print(" state=");
      Serial.println(state ? "HIGH" : "LOW");
      util::UIConsole::log("Set connection x=" + String(x) + " y=" + String(y) + " state=" + String(state ? "HIGH" : "LOW"), "MT8816Driver");
    }
}

void MT8816Driver::setAddress(uint8_t x, uint8_t y)
{

  for (int i = 0; i < 4; ++i) {
    bool bit = (x >> i) & 0x01;
    mcpDriver_.digitalWriteMCP(mcp::MCP_MT8816_ADDRESS, cfg::mt8816::ax_pins[i], bit);
  }

  for (int i = 0; i < 3; ++i) {
    bool bit = (y >> i) & 0x01;
    mcpDriver_.digitalWriteMCP(mcp::MCP_MT8816_ADDRESS, cfg::mt8816::ay_pins[i], bit);
  }
}

void MT8816Driver::reset()
{   
  // Pulse the reset pin to reset the IC
  mcpDriver_.digitalWriteMCP(mcp::MCP_MT8816_ADDRESS, mcp::RESET, LOW);
  delayMicroseconds(10);
  mcpDriver_.digitalWriteMCP(mcp::MCP_MT8816_ADDRESS, mcp::RESET, HIGH);
  delay(100);  
  mcpDriver_.digitalWriteMCP(mcp::MCP_MT8816_ADDRESS, mcp::RESET, LOW);

  mcpDriver_.digitalWriteMCP(mcp::MCP_MT8816_ADDRESS, mcp::STROBE, LOW);
  mcpDriver_.digitalWriteMCP(mcp::MCP_MT8816_ADDRESS, mcp::CS, LOW);

  if (settings_.debugMTLevel >= 1) {
    Serial.println("MT8816: reset performed.");
    util::UIConsole::log("Reset performed.", "MT8816Driver");
  }
}

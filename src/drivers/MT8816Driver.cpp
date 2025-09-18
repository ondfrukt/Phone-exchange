#include "MT8816Driver.h"

using namespace cfg;

MT8816Driver::MT8816Driver(MCPDriver& mcpDriver, Settings& settings) : mcpDriver_(mcpDriver), settings_(settings) {
    // Initiera anslutningsmatris
    memset(connections, 0, sizeof(connections));
}

void MT8816Driver::begin()
{
	// Reset MCP
	reset();
	Serial.println("MT8816 initialized successfully.");
}

void MT8816Driver::SetAudioConnection(uint8_t line, uint8_t audio, bool state) {
	SetConnection(line, audio, state);

	if (settings_.debugMTLevel >= 1) {
		Serial.print("MT8816Driver: Audio connection for line ");
		Serial.print(line);
		Serial.print(" and audio ");
		Serial.print(audio);
		Serial.print(" set to ");
		Serial.println(state ? "CONNECTED" : "DISCONNECTED");
	}
}
void MT8816Driver::SetLineConnection(uint8_t lineA, uint8_t lineB, bool state) {
    SetConnection(lineA, lineB, state);
    SetConnection(lineB, lineA, state);

	if (settings_.debugMTLevel >= 1) {
		Serial.print("MT8816Driver: Line connection between ");
		Serial.print(lineA);
		Serial.print(" and ");
		Serial.print(lineB);
		Serial.print(" set to ");
		Serial.println(state ? "CONNECTED" : "DISCONNECTED");
	}
}

void MT8816Driver::SetConnection(uint8_t x, uint8_t y, bool state) {
    setAddress(x, y);
    mcpDriver_.digitalWriteMCP(mcp::MCP_MT8816_ADDRESS, mcp::DATA, state ? HIGH : LOW);
    delay(10);  // Short delay to ensure data pin is stable
    strobe();
    connections[x][y] = state;
}

bool MT8816Driver::getConnection(int x, int y) {
    // Check if the coordinates are within the valid range
    if (x >= 0 && x < 8 && y >= 0 && y < 8) {
        return connections[x][y];
    } else {
        // Handle invalid coordinates, e.g., by returning a default value or throwing an exception
        Serial.print("Error: Invalid coordinates (");
        Serial.print(x);
        Serial.print(", ");
        Serial.print(y);
        Serial.println(").");
        return false;
    }
}

void MT8816Driver::printConnections() {
  // Skriv ut kolumnrubrikerna
  Serial.print("     ");  // Något extra utrymme för radnummerkolumnen
  for (int j = 0; j < 8; j++) {
    Serial.print(j);
    Serial.print(" ");
  }
  Serial.println(); // Avsluta kolumnrubrikraden
  // Skriv ut varje rad med radnummer först
  for (int i = 0; i < 16; i++) {
    // Skriv ut radnumret
    if (i < 10) {
      // Lägg till ett extra mellanslag för enhetliga avstånd, om du har en- och tvåsiffriga tal
      Serial.print(" ");
    }
    Serial.print(i);
    Serial.print(" | ");  // Avgränsare mellan radnumret och själva matrisdata
    
    // Skriv ut värdena i raden
    for (int j = 0; j < 8; j++) {
      Serial.print((int)connections[i][j]);
      Serial.print(" ");
    }
    Serial.println(); // Ny rad efter varje rad i matrisen
  }
  Serial.println(); // Extra radbrytning för att skilja från annan output
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
    Serial.println("MT8816 reset performed.");
}

void MT8816Driver::strobe()
{
    mcpDriver_.digitalWriteMCP(mcp::MCP_MT8816_ADDRESS, mcp::STROBE, HIGH);
    delayMicroseconds(5);
    mcpDriver_.digitalWriteMCP(mcp::MCP_MT8816_ADDRESS, mcp::STROBE, LOW);
    delayMicroseconds(5);
}

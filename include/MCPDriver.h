#pragma once
#include "config.h"
#include <Adafruit_MCP23X17.h>


class MCPDriver {
public:
  // Publikt API: bara en funktion
  bool begin();
  void ackMainInt();     // kvittera INT från mcpMain
  void ackMT8816Int();   // kvittera INT från mcpMT8816 (om du använder dess INT)
  void ackSlic1Int();    // kvittera INT från mcpSlic1
  void ackSlic2Int();    // kvittera INT från mcpSlic1

private:
  // Instanser för alla MCP-enheter
  Adafruit_MCP23X17 mcpMain;
  Adafruit_MCP23X17 mcpMT8816;
  Adafruit_MCP23X17 mcpSlic1;
  Adafruit_MCP23X17 mcpSlic2;

  // Hjälpfunktioner som bara används internt
  void SetMCPPinModes(Adafruit_MCP23X17& mcp,
                      const cfg::mcp::PinModeEntry (&list)[16]);
  bool setupMain();
  bool setupMT8816();
  bool setupSlics();

  void enableSlicShkInterrupts_(Adafruit_MCP23X17& mcp);   // per-pin ints (CHANGE)
  int8_t mapShkIndex_(uint8_t mcpPin) const;              // MCP-pin -> SHK-index 0..3
  void handleSlicInt_();
};

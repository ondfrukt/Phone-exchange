#include "DTMF_Mux.h"
using namespace cfg;

DTMF_Mux::DTMF_Mux(MCPDriver& mcpDriver, Settings& settings)
  : mcpDriver_(mcpDriver), settings_(settings) {
}

void DTMF_Mux::begin() {
    mcpDriver_.digitalWriteMCP(mcp::MCP_MAIN_ADDRESS, mcp::TM_A0, HIGH);
    mcpDriver_.digitalWriteMCP(mcp::MCP_MAIN_ADDRESS, mcp::TM_A1, HIGH);
    mcpDriver_.digitalWriteMCP(mcp::MCP_MAIN_ADDRESS, mcp::TM_A2, HIGH);
}
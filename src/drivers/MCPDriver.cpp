#include "MCPDriver.h"
#include "config.h"
using cfg::mcp;

void MCPDriver::begin() {
  mcpMain.begin_I2C(cfg::mcp::MCP_ADDRESS);
  mcpMT8816.begin_I2C(cfg::mcp::MCP_MT8816_ADDRESS);
  mcpSlic1.begin_I2C(cfg::mcp::MCP_SLIC1_ADDRESS);
  mcpSlic2.begin_I2C(cfg::mcp::MCP_SLIC2_ADDRESS);
};

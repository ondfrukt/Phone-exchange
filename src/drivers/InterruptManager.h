#pragma once
#include <Arduino.h>
#include <queue>
#include "drivers/MCPDriver.h"
#include "settings/settings.h"
#include "services/RingGenerator.h"
#include "util/UIConsole.h"

// InterruptManager: Centralt samlingspunkt för alla MCP-interrupts
// Hardware → MCPDriver → InterruptManager (queue) → Components (poll)
class InterruptManager {
public:
  InterruptManager(MCPDriver& mcpDriver, Settings& settings);

  // Samla in interrupts från MCPDriver och lägg i kön
  void collectInterrupts();

  // Polla nästa händelse för en specifik MCP-adress och pin (returnerar tom IntResult om ingen finns)
  IntResult pollEvent(uint8_t i2c_addr, uint8_t pin);

  // Polla nästa händelse för en specifik MCP-adress (returnerar tom IntResult om ingen finns)
  IntResult pollEventByAddress(uint8_t i2c_addr);

  // Polla alla händelser (används när komponenter vill hantera alla typer av events)
  IntResult pollAnyEvent();

  // Töm alla händelser i kön
  void clearQueue();

  // Returnera antalet väntande händelser
  size_t queueSize() const { return eventQueue_.size(); }

private:
  MCPDriver& mcpDriver_;
  Settings& settings_;
  std::queue<IntResult> eventQueue_;
  
  static constexpr size_t MAX_QUEUE_SIZE = 100;
};

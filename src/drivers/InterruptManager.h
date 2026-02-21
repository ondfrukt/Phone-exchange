#pragma once
#include <Arduino.h>
#include <queue>
#include "drivers/MCPDriver.h"
#include "settings/settings.h"
#include "services/RingGenerator.h"
#include "util/UIConsole.h"

// InterruptManager:   Central collection point for all MCP interrupts
// Hardware -> MCPDriver -> InterruptManager (queue) -> Components (poll)
class InterruptManager {
public:
  InterruptManager(MCPDriver& mcpDriver, Settings& settings);

  // Collect interrupts from MCPDriver and enqueue them
  void collectInterrupts();

  // Poll the next event for a specific MCP address and pin (returns empty IntResult if none exists)
  IntResult pollEvent(uint8_t i2c_addr, uint8_t pin);

  // Poll the next event for a specific MCP address (returns empty IntResult if none exists)
  IntResult pollEventByAddress(uint8_t i2c_addr);

  // Poll all events (used when components want to handle all event types)
  IntResult pollAnyEvent();

  // Clear all queued events
  void clearQueue();

  // Return the number of pending events
  size_t queueSize() const { return eventQueue_.size(); }

private:
  MCPDriver& mcpDriver_;
  Settings& settings_;
  std::queue<IntResult> eventQueue_;
  
  static constexpr size_t MAX_QUEUE_SIZE = 100;
};

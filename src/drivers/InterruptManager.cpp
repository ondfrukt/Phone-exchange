#include "drivers/InterruptManager.h"

InterruptManager::InterruptManager(MCPDriver& mcpDriver, Settings& settings)
  : mcpDriver_(mcpDriver), settings_(settings) {}

// Samla in alla väntande interrupts från MCPDriver och lägg i kön
void InterruptManager::collectInterrupts() {
  uint8_t now = millis();
  // Samla in alla MCP_MAIN interrupts
  while (true) {
    IntResult r = mcpDriver_.handleMainInterrupt();
    if (!r.hasEvent) break;

    if (eventQueue_.size() >= MAX_QUEUE_SIZE) {
      if (settings_.debugIMLevel >= 1) {
        Serial.println(F("InterruptManager: WARNING - Queue full, dropping MCP_MAIN event"));
        util::UIConsole::log("WARNING - Queue full, dropping MCP_MAIN event", "InterruptManager");
      }
      break;
    }
    
    eventQueue_.push(r);
    
    if (settings_.debugIMLevel >= 2) {
      Serial.print(F("InterruptManager: Queued MCP_MAIN event - addr=0x"));
      Serial.print(r.i2c_addr, HEX);
      Serial.print(F(" pin="));
      Serial.print(r.pin);
      Serial.print(F(" level="));
      Serial.println(r.level ? F("HIGH") : F("LOW"));
      util::UIConsole::log("Queued MCP_MAIN 0x" + String(r.i2c_addr, HEX) + 
                          " pin=" + String(r.pin) + 
                          " level=" + String(r.level ? "HIGH" : "LOW"),
                          "InterruptManager");
    }
  }

  // Samla in alla MCP_SLIC1 interrupts
  while (true) {
    IntResult r = mcpDriver_.handleSlic1Interrupt();
    if (!r.hasEvent) break;
    


    if (eventQueue_.size() >= MAX_QUEUE_SIZE) {
      if (settings_.debugIMLevel >= 1) {
        Serial.println(F("InterruptManager: WARNING - Queue full, dropping MCP_SLIC1 event"));
        util::UIConsole::log("WARNING - Queue full, dropping MCP_SLIC1 event", "InterruptManager");
      }
      break;
    }
    
    eventQueue_.push(r);
    
    if (settings_.debugIMLevel >= 2) {
      Serial.print(F("InterruptManager: Queued MCP_SLIC1 event - addr=0x"));
      Serial.print(r.i2c_addr, HEX);
      Serial.print(F(" pin="));
      Serial.print(r.pin);
      Serial.print(F(" line="));
      Serial.print(r.line);
      Serial.print(F(" level="));
      Serial.println(r.level ? F("HIGH") : F("LOW"));
      util::UIConsole::log("Queued MCP_SLIC1 0x" + String(r.i2c_addr, HEX) + 
                          " pin=" + String(r.pin) + 
                          " line=" + String(r.line) +
                          " level=" + String(r.level ? "HIGH" : "LOW"),
                          "InterruptManager");
    }
  }

  // Samla in alla MCP_SLIC2 interrupts
  while (true) {
    IntResult r = mcpDriver_.handleSlic2Interrupt();
    if (!r.hasEvent) break;
    
    if (eventQueue_.size() >= MAX_QUEUE_SIZE) {
      if (settings_.debugIMLevel >= 1) {
        Serial.println(F("InterruptManager: WARNING - Queue full, dropping MCP_SLIC2 event"));
        util::UIConsole::log("WARNING - Queue full, dropping MCP_SLIC2 event", "InterruptManager");
      }
      break;
    }
    
    eventQueue_.push(r);
    
    if (settings_.debugIMLevel >= 2) {
      Serial.print(F("InterruptManager: Queued MCP_SLIC2 event - addr=0x"));
      Serial.print(r.i2c_addr, HEX);
      Serial.print(F(" pin="));
      Serial.print(r.pin);
      Serial.print(F(" line="));
      Serial.print(r.line);
      Serial.print(F(" level="));
      Serial.println(r.level ? F("HIGH") : F("LOW"));
      util::UIConsole::log("Queued MCP_SLIC2 0x" + String(r.i2c_addr, HEX) + 
                          " pin=" + String(r.pin) + 
                          " line=" + String(r.line) +
                          " level=" + String(r.level ? "HIGH" : "LOW"),
                          "InterruptManager");
    }
  }

  // Samla in alla MCP_MT8816 interrupts
  while (true) {
    IntResult r = mcpDriver_.handleMT8816Interrupt();
    if (!r.hasEvent) break;
    
    if (eventQueue_.size() >= MAX_QUEUE_SIZE) {
      if (settings_.debugIMLevel >= 1) {
        Serial.println(F("InterruptManager: WARNING - Queue full, dropping MCP_MT8816 event"));
        util::UIConsole::log("WARNING - Queue full, dropping MCP_MT8816 event", "InterruptManager");
      }
      break;
    }
    
    eventQueue_.push(r);
    
    if (settings_.debugIMLevel >= 1) {
      Serial.print(F("InterruptManager: Queued MCP_MT8816 event - addr=0x"));
      Serial.print(r.i2c_addr, HEX);
      Serial.print(F(" pin="));
      Serial.print(r.pin);
      Serial.print(F(" level="));
      Serial.println(r.level ? F("HIGH") : F("LOW"));
      util::UIConsole::log("Queued MCP_MT8816 0x" + String(r.i2c_addr, HEX) + 
                          " pin=" + String(r.pin) + 
                          " level=" + String(r.level ? "HIGH" : "LOW"),
                          "InterruptManager");
    }
  }
}

// Polla nästa händelse för en specifik MCP-adress och pin
// Note: This implementation has O(n) complexity but is acceptable for small queue sizes
// and the expected low interrupt rate (typically < 100 events/sec).
// Events are requeued if they don't match, which may alter ordering slightly.
IntResult InterruptManager::pollEvent(uint8_t i2c_addr, uint8_t pin) {
  if (eventQueue_.empty()) {
    return IntResult{}; // Tom händelse
  }

  // Använd en temporär kö för att söka efter matchande händelse
  std::queue<IntResult> tempQueue;
  IntResult foundEvent;
  bool found = false;

  while (!eventQueue_.empty()) {
    IntResult current = eventQueue_.front();
    eventQueue_.pop();

    if (!found && current.i2c_addr == i2c_addr && current.pin == pin) {
      foundEvent = current;
      found = true;
      
      if (settings_.debugIMLevel >= 2) {
        Serial.print(F("InterruptManager: Polled event - addr=0x"));
        Serial.print(i2c_addr, HEX);
        Serial.print(F(" pin="));
        Serial.print(pin);
        Serial.print(F(" level="));
        Serial.println(foundEvent.level ? F("HIGH") : F("LOW"));
        util::UIConsole::log("Polled event 0x" + String(i2c_addr, HEX) + 
                            " pin=" + String(pin) + 
                            " level=" + String(foundEvent.level ? "HIGH" : "LOW"),
                            "InterruptManager");
      }
    } else {
      // Spara händelser som inte matchade för att lägga tillbaka dem
      tempQueue.push(current);
    }
  }

  // Lägg tillbaka alla händelser som inte matchade
  while (!tempQueue.empty()) {
    eventQueue_.push(tempQueue.front());
    tempQueue.pop();
  }

  return foundEvent;
}

// Polla nästa händelse för en specifik MCP-adress (oavsett pin)
IntResult InterruptManager::pollEventByAddress(uint8_t i2c_addr) {
  if (eventQueue_.empty()) {
    return IntResult{}; // Tom händelse
  }

  // Använd en temporär kö för att söka efter matchande händelse
  std::queue<IntResult> tempQueue;
  IntResult foundEvent;
  bool found = false;

  while (!eventQueue_.empty()) {
    IntResult current = eventQueue_.front();
    eventQueue_.pop();

    if (!found && current.i2c_addr == i2c_addr) {
      foundEvent = current;
      found = true;
      
      if (settings_.debugIMLevel >= 2) {
        Serial.print(F("InterruptManager: Polled event by address - addr=0x"));
        Serial.print(i2c_addr, HEX);
        Serial.print(F(" pin="));
        Serial.print(foundEvent.pin);
        Serial.print(F(" level="));
        Serial.println(foundEvent.level ? F("HIGH") : F("LOW"));
        util::UIConsole::log("Polled event by address 0x" + String(i2c_addr, HEX) + 
                            " pin=" + String(foundEvent.pin) + 
                            " level=" + String(foundEvent.level ? "HIGH" : "LOW"),
                            "InterruptManager");
      }
    } else {
      // Spara händelser som inte matchade för att lägga tillbaka dem
      tempQueue.push(current);
    }
  }

  // Lägg tillbaka alla händelser som inte matchade
  while (!tempQueue.empty()) {
    eventQueue_.push(tempQueue.front());
    tempQueue.pop();
  }

  return foundEvent;
}

// Polla alla händelser (FIFO-ordning)
IntResult InterruptManager::pollAnyEvent() {
  if (eventQueue_.empty()) {
    return IntResult{}; // Tom händelse
  }

  IntResult event = eventQueue_.front();
  eventQueue_.pop();

  if (settings_.debugIMLevel >= 2) {
    Serial.print(F("InterruptManager: Polled any event - addr=0x"));
    Serial.print(event.i2c_addr, HEX);
    Serial.print(F(" pin="));
    Serial.print(event.pin);
    Serial.print(F(" level="));
    Serial.println(event.level ? F("HIGH") : F("LOW"));
    util::UIConsole::log("Polled any event 0x" + String(event.i2c_addr, HEX) + 
                        " pin=" + String(event.pin) + 
                        " level=" + String(event.level ? "HIGH" : "LOW"),
                        "InterruptManager");
  }

  return event;
}

// Töm alla händelser i kön
void InterruptManager::clearQueue() {
  size_t clearedCount = eventQueue_.size();
  
  while (!eventQueue_.empty()) {
    eventQueue_.pop();
  }

  if (settings_.debugIMLevel >= 1 && clearedCount > 0) {
    Serial.print(F("InterruptManager: Cleared "));
    Serial.print(clearedCount);
    Serial.println(F(" events from queue"));
    util::UIConsole::log("Cleared " + String(clearedCount) + " events from queue", 
                        "InterruptManager");
  }
}

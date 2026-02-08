#include "services/LineAudioConnections.h"


void LineAudioConnections::connectLines(uint8_t lineA, uint8_t lineB) {

  // Check if already connected
  if (isConnected(lineA, lineB)) {
    return;
  }

  mt8816Driver_.setConnection(lineA, lineB, true);
  mt8816Driver_.setConnection(lineB, lineA, true);
  addConnection(lineA, lineB);

  if (settings.debugLAC <= 1) {
    Serial.print("LineAudioConnections: ");
    Serial.print("Connected line ");
    Serial.print(lineA);
    Serial.print(" to line ");
    Serial.println(lineB);
  }
  util::UIConsole::log("Connected line " + String(lineA) + " to line " + String(lineB), "LineAudioConnections");
}
void LineAudioConnections::disconnectLines(uint8_t lineA, uint8_t lineB) {

  mt8816Driver_.setConnection(lineA, lineB, false);
  mt8816Driver_.setConnection(lineB, lineA, false);
  removeConnection(lineA, lineB);

  if (settings.debugLAC <= 1) {
    Serial.print("LineAudioConnections: ");
    Serial.print("Connected line ");
    Serial.print(lineA);
    Serial.print(" to line ");
    Serial.println(lineB);
  }
  util::UIConsole::log("Connected line " + String(lineA) + " to line " + String(lineB), "LineAudioConnections");
}

void LineAudioConnections::connectAudioToLine(uint8_t line, uint8_t audioSource) {
  connectLines(audioSource, line);
}
void LineAudioConnections::disconnectAudioToLine(uint8_t line, uint8_t audioSource) {
  disconnectLines(audioSource, line);
}



// Add a connected pair
void LineAudioConnections::addConnection(uint8_t lineA, uint8_t lineB) {
  activeConnections.push_back(std::make_pair(lineA, lineB));
}

// Remove a connected pair
void LineAudioConnections::removeConnection(uint8_t lineA, uint8_t lineB) {
  for (auto it = activeConnections.begin(); it != activeConnections.end(); ++it) {
    if ((it->first == lineA && it->second == lineB) ||
      (it->first == lineB && it->second == lineA)) { 
      activeConnections.erase(it);
      break;
    }
  }
}

// Conntrolls connections
bool LineAudioConnections::isConnected(uint8_t lineA, uint8_t lineB) const {
    for (const auto& conn : activeConnections) {
        if ((conn.first == lineA && conn.second == lineB) ||
            (conn.first == lineB && conn.second == lineA))
            return true;
    }
    return false;
}

// Print active connections in a compact format
void LineAudioConnections::printConnections() const {
    for (const auto& conn : activeConnections) {
        Serial.print("Anslutning: ");
        Serial.print(conn.first);
        Serial.print(" <--> ");
        Serial.println(conn.second);
    }
}
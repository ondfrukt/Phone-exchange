#include "services/ConnectionHandler.h"

void ConnectionHandler::connectLines(uint8_t lineA, uint8_t lineB) {

  // Check if already connected
  if (isConnected(lineA, lineB)) {
    return;
  }

  mt8816Driver_.setConnection(lineA, lineB, true);
  mt8816Driver_.setConnection(lineB, lineA, true);
  addConnection(lineA, lineB);

  if (settings.debugLAC <= 1) {
    Serial.print("ConnectionHandler: ");
    Serial.print("Connected line ");
    Serial.print(lineA);
    Serial.print(" to line ");
    Serial.println(lineB);
  }
  util::UIConsole::log("Connected line " + String(lineA) + " to line " + String(lineB), "ConnectionHandler");
}
void ConnectionHandler::disconnectLines(uint8_t lineA, uint8_t lineB) {

  mt8816Driver_.setConnection(lineA, lineB, false);
  mt8816Driver_.setConnection(lineB, lineA, false);
  removeConnection(lineA, lineB);

  if (settings.debugLAC <= 1) {
    Serial.print("ConnectionHandler: ");
    Serial.print("Disconnected line ");
    Serial.print(lineA);
    Serial.print(" from line ");
    Serial.println(lineB);
  }
  util::UIConsole::log("Disconnected line " + String(lineA) + " from line " + String(lineB), "ConnectionHandler");
}

void ConnectionHandler::connectAudioToLine(uint8_t line, uint8_t audioSource) {
  mt8816Driver_.setConnection(audioSource, line, true);
  if (settings.debugLAC <= 1) {
    Serial.print("ConnectionHandler: ");
    Serial.print("Connected audio source ");
    Serial.print(audioSource);
    Serial.print(" to line ");
    Serial.println(line);
  }
}
void ConnectionHandler::disconnectAudioToLine(uint8_t line, uint8_t audioSource) {
  mt8816Driver_.setConnection(audioSource, line, false);
  if (settings.debugLAC <= 1) {
    Serial.print("ConnectionHandler: ");
    Serial.print("Disconnected audio source ");
    Serial.print(audioSource);
    Serial.print(" from line ");
    Serial.println(line);
  }
}

// Add a connected pair
void ConnectionHandler::addConnection(uint8_t lineA, uint8_t lineB) {
  activeConnections.push_back(std::make_pair(lineA, lineB));
}

// Remove a connected pair
void ConnectionHandler::removeConnection(uint8_t lineA, uint8_t lineB) {
  for (auto it = activeConnections.begin(); it != activeConnections.end(); ++it) {
    if ((it->first == lineA && it->second == lineB) ||
      (it->first == lineB && it->second == lineA)) { 
      activeConnections.erase(it);
      break;
    }
  }
}

// Conntrolls connections
bool ConnectionHandler::isConnected(uint8_t lineA, uint8_t lineB) const {
    for (const auto& conn : activeConnections) {
        if ((conn.first == lineA && conn.second == lineB) ||
            (conn.first == lineB && conn.second == lineA))
            return true;
    }
    return false;
}

// Print active connections in a compact format
void ConnectionHandler::printConnections() const {
    for (const auto& conn : activeConnections) {
        Serial.print("Connection: ");
        Serial.print(conn.first);
        Serial.print(" <--> ");
        Serial.println(conn.second);
    }
}
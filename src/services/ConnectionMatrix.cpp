#include "ConnectionMatrix.h"

ConnectionMatrix::ConnectionMatrix() {
  clearAll();
}

bool ConnectionMatrix::setConnection(int lineA, int lineB, State state) {
  if (! isValidPair_(lineA, lineB)) {
    return false;
  }

  // Set both directions (symmetric)
  connections_[lineA][lineB] = state;
  connections_[lineB][lineA] = state;

  return true;
}

ConnectionMatrix::State ConnectionMatrix::getConnectionState(int lineA, int lineB) const {
  if (!isValidPair_(lineA, lineB)) {
    return State::None;
  }

  return connections_[lineA][lineB];
}

bool ConnectionMatrix::areConnected(int lineA, int lineB) const {
  if (!isValidPair_(lineA, lineB)) {
    return false;
  }

  return connections_[lineA][lineB] != State::None;
}

int ConnectionMatrix::getConnectedLine(int line, State minState) const {
  if (!isValidLine_(line)) {
    return -1;
  }

  for (int i = 0; i < 8; i++) {
    if (i != line && connections_[line][i] >= minState) {
      return i;
    }
  }

  return -1;
}

std::vector<int> ConnectionMatrix::getAllConnectedLines(int line, State minState) const {
  std::vector<int> result;
  
  if (!isValidLine_(line)) {
    return result;
  }

  for (int i = 0; i < 8; i++) {
    if (i != line && connections_[line][i] >= minState) {
      result.push_back(i);
    }
  }

  return result;
}

bool ConnectionMatrix::disconnect(int lineA, int lineB) {
  return setConnection(lineA, lineB, State::None);
}

void ConnectionMatrix::disconnectLine(int line) {
  if (!isValidLine_(line)) {
    return;
  }

  for (int i = 0; i < 8; i++) {
    connections_[line][i] = State::None;
    connections_[i][line] = State::None;
  }
}

void ConnectionMatrix::clearAll() {
  memset(connections_, 0, sizeof(connections_));
}

bool ConnectionMatrix::hasAnyConnection(int line, State minState) const {
  return getConnectedLine(line, minState) != -1;
}

void ConnectionMatrix::printMatrix() const {
  Serial.println(F("\n╔════════════════════════════════════════╗"));
  Serial.println(F("║      Connection Matrix (8x8)          ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println(F("      0  1  2  3  4  5  6  7"));
  Serial.println(F("    ┌──┬──┬──┬──┬──┬──┬──┬──┐"));

  for (int i = 0; i < 8; i++) {
    Serial.printf("  %d │", i);
    
    for (int j = 0; j < 8; j++) {
      if (i == j) {
        Serial.print(" -");
      } else {
        char symbol = stateToSymbol(connections_[i][j]);
        Serial.printf(" %c", symbol);
      }
      Serial.print("│");
    }
    
    Serial.println();
    
    if (i < 7) {
      Serial.println(F("    ├──┼──┼──┼──┼──┼──┼──┼──┤"));
    }
  }
  
  Serial.println(F("    └──┴──┴──┴──┴──┴──┴──┴──┘"));
  Serial.println();
  Serial.println(F("Legend:"));
  Serial.println(F("  -  Self (no connection to self)"));
  Serial.println(F("     None (no connection)"));
  Serial.println(F("  ?   Attempting"));
  Serial.println(F("  ~  Ringing"));
  Serial.println(F("  ■  Established"));
  Serial.println();

  printConnections();
}

void ConnectionMatrix::printConnections() const {
  Serial.println(F("Active connections:"));
  
  bool hasAny = false;
  
  for (int i = 0; i < 8; i++) {
    for (int j = i + 1; j < 8; j++) {  // Only check upper triangle to avoid duplicates
      if (connections_[i][j] != State::None) {
        Serial.printf("  Line %d <-> Line %d :  %s\n", 
                     i, j, stateToString(connections_[i][j]));
        hasAny = true;
      }
    }
  }
  
  if (!hasAny) {
    Serial.println(F("  (none)"));
  }
  
  Serial.println();
}

const char* ConnectionMatrix::stateToString(State state) {
  switch (state) {
    case State::None:        return "None";
    case State::Attempting:  return "Attempting";
    case State:: Ringing:     return "Ringing";
    case State:: Established:  return "Established";
    default:                 return "Unknown";
  }
}

char ConnectionMatrix::stateToSymbol(State state) {
  switch (state) {
    case State::None:        return ' ';
    case State:: Attempting:  return '?';
    case State::Ringing:      return '~';
    case State::Established: return '*';  // Using * instead of ■ for better compatibility
    default:                 return '?';
  }
}

bool ConnectionMatrix::isValidLine_(int line) const {
  return line >= 0 && line < 8;
}

bool ConnectionMatrix::isValidPair_(int lineA, int lineB) const {
  return isValidLine_(lineA) && isValidLine_(lineB) && lineA != lineB;
}
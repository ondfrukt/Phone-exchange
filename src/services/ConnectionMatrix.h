#pragma once
#include <Arduino.h>
#include <vector>

/**
 * ConnectionMatrix - Manages connection state between phone lines
 * 
 * This class provides a centralized way to track which lines are connected
 * to each other, replacing the previous outgoingTo/incomingFrom approach.
 * 
 * The matrix is symmetric:  if Line A is connected to Line B, then
 * connections[A][B] == connections[B][A]
 */

class ConnectionMatrix {
public:
  /**
   * State of a connection between two lines
   */
  enum class State :  uint8_t {
    None = 0,        // No connection
    Attempting,      // Trying to establish connection
    Ringing,         // Ringing the target line
    Established      // Active call in progress
  };

  /**
   * Constructor - initializes empty matrix
   */
  ConnectionMatrix();

  /**
   * Set connection state between two lines (symmetric operation)
   * @param lineA First line (0-7)
   * @param lineB Second line (0-7)
   * @param state New connection state
   * @return true if successful, false if invalid line numbers
   */
  bool setConnection(int lineA, int lineB, State state);

  /**
   * Get connection state between two lines
   * @param lineA First line (0-7)
   * @param lineB Second line (0-7)
   * @return Connection state, or State::None if invalid line numbers
   */
  State getConnectionState(int lineA, int lineB) const;

  /**
   * Check if two lines are connected (any state except None)
   * @param lineA First line (0-7)
   * @param lineB Second line (0-7)
   * @return true if connected in any way
   */
  bool areConnected(int lineA, int lineB) const;

  /**
   * Find which line this line is connected to
   * @param line Line to check (0-7)
   * @param minState Minimum state to consider (default:  Attempting)
   * @return Connected line number, or -1 if not connected
   */
  int getConnectedLine(int line, State minState = State::Attempting) const;

  /**
   * Get all lines that this line is connected to
   * (For future multi-party support)
   * @param line Line to check (0-7)
   * @param minState Minimum state to consider (default: Attempting)
   * @return Vector of connected line numbers
   */
  std:: vector<int> getAllConnectedLines(int line, State minState = State::Attempting) const;

  /**
   * Disconnect a specific line from another line
   * @param lineA First line (0-7)
   * @param lineB Second line (0-7)
   * @return true if successful
   */
  bool disconnect(int lineA, int lineB);

  /**
   * Disconnect a line from all other lines
   * @param line Line to disconnect (0-7)
   */
  void disconnectLine(int line);

  /**
   * Clear all connections (reset matrix)
   */
  void clearAll();

  /**
   * Check if a line has any connections at all
   * @param line Line to check (0-7)
   * @param minState Minimum state to consider (default: Attempting)
   * @return true if line has at least one connection
   */
  bool hasAnyConnection(int line, State minState = State::Attempting) const;

  /**
   * Print matrix to Serial (for debugging)
   */
  void printMatrix() const;

  /**
   * Print matrix in a compact format (single line per connection)
   */
  void printConnections() const;

  /**
   * Convert State enum to human-readable string
   */
  static const char* stateToString(State state);

  /**
   * Convert State enum to single-character symbol for visualization
   */
  static char stateToSymbol(State state);

private:
  State connections_[8][8];  // 8x8 matrix for 8 phone lines

  /**
   * Validate line number
   * @param line Line number to validate
   * @return true if line is in valid range (0-7)
   */
  bool isValidLine_(int line) const;

  /**
   * Validate line pair
   * @param lineA First line
   * @param lineB Second line
   * @return true if both lines are valid and different
   */
  bool isValidPair_(int lineA, int lineB) const;
};
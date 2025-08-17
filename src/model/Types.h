#pragma once
#include <cstdint>

namespace model {

  // Enum representing all possible statuses of a line
  enum class LineStatus : uint8_t {
    Idle,           // Line is not in use
    Ready,          // Dial tone is playing, waiting for input 
    PulseDialing,   // Old-style rotary dialing in progress
    ToneDialing,    // Modern touch-tone dialing in progress
    Busy,           // Receiving busy signal
    Fail,           // Line failed to connect
    Ringing,        // Line is ringing (outgoing call)
    Connected,      // Call is active
    Disconnected,   // Call has ended, but line not yet idle
    Timeout,        // Line timed out
    Abandoned,      // Line was abandoned
    Incoming,       // Incoming call
    Operator,       // Connected to operator
    SystemConfig    // Line is in configuration mode
  };

  // Enum representing all possible statuses of the hook
  enum class HookStatus : uint8_t {
    Off,            // Hook is off (off-hook)
    On,             // Hook is on (on-hook)
    Disconnected    // Hook is disconnected
  };

  // Enum representing the sequence of tones played
  enum class ToneSequence : uint8_t {
    None,   // No tone played
    Busy,   // Busy tone played
    Ready,  // Ready tone played
    Fail    // Fail tone played
  };

  // Function to convert line status enum to string
  inline const char* toString(LineStatus st) {
    switch (st) {
      case LineStatus::Idle:          return "line_idle";
      case LineStatus::Ready:         return "line_ready";
      case LineStatus::PulseDialing:  return "line_pulse_dialing";
      case LineStatus::ToneDialing:   return "line_tone_dialing";
      case LineStatus::Busy:          return "line_busy";
      case LineStatus::Fail:          return "line_fail";
      case LineStatus::Ringing:       return "line_ringing";
      case LineStatus::Connected:     return "line_connected";
      case LineStatus::Disconnected:  return "line_disconnected";
      case LineStatus::Timeout:       return "line_timeout";
      case LineStatus::Abandoned:     return "line_abandoned";
      case LineStatus::Incoming:      return "line_incoming";
      case LineStatus::Operator:      return "line_operator";
      case LineStatus::SystemConfig:  return "system_config";
      default:                        return "unknown";
      }
  }
}
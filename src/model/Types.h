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
    On,            // Hook is off (off-hook)
    Off,             // Hook is on (on-hook)
    Disconnected    // Hook is disconnected
  };

  // Enum representing the sequence of tones played
  enum class ToneId : uint8_t {
    Ready,  // Ready tone played
    Ring,   // Ring tone played
    Busy,   // Busy tone played
    Fail    // Fail tone played
  };

  // Ringing state machine
  enum class RingState {
    RingIdle,
    RingToggling,  // Generating ring signal (FR toggling)
    RingPause      // Pause between rings
  };

  // Function to convert line status enum to string
  inline const char* toString(LineStatus st) {
    switch (st) {
      case LineStatus::Idle:          return "Idle";
      case LineStatus::Ready:         return "Ready";
      case LineStatus::PulseDialing:  return "PulseDialing";
      case LineStatus::ToneDialing:   return "ToneDialing";
      case LineStatus::Busy:          return "Busy";
      case LineStatus::Fail:          return "Fail";
      case LineStatus::Ringing:       return "Ringing";
      case LineStatus::Connected:     return "Connected";
      case LineStatus::Disconnected:  return "Disconnected";
      case LineStatus::Timeout:       return "Timeout";
      case LineStatus::Abandoned:     return "Abandoned";
      case LineStatus::Incoming:      return "Incoming";
      case LineStatus::Operator:      return "Operator";
      case LineStatus::SystemConfig:  return "Config";
      default:                        return "unknown";
      }
  }
}
#pragma once

#include <Arduino.h>

// Debounced scanner for a single 4x4 membrane keypad.
class Keypad4x4 {
 public:
  // Rows are driven LOW one at a time, columns are read with the internal
  // pull-ups, so no external resistors are needed.
  void begin();

  // Returns the key released since the last call, or 0 when idle.
  // A key is reported once per press: there is no auto-repeat, which is what
  // you want for a PIN pad. Keep calling this from loop().
  char read();

 private:
  char scan() const;

  char stableKey_ = 0;    // key the debounce timer is currently running for
  char delivered_ = 0;    // last key already handed out, 0 = none this press
  uint32_t stableSinceMs_ = 0;
};

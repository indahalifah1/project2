#pragma once

#include <Arduino.h>

// Drives the electric lock relay and owns the re-locking policy.
class Door {
 public:
  void begin();

  // Release the lock. Restarts the unlock timer.
  void unlock();

  // Re-lock immediately. This is the state the board boots into and the state
  // it falls back to on reset, so a crash never leaves the door open.
  void lock();

  bool isUnlocked() const { return unlocked_; }

  // Seconds left before the lock closes by itself, rounded up. 0 while locked.
  uint16_t secondsRemaining() const;

  // Call from loop().
  void tick();

 private:
  bool doorIsClosed() const;

  enum class Phase : uint8_t {
    Locked,
    Unlocked,      // counting down DOOR_UNLOCK_MS
    WaitingClosed, // door sensor: door is open, wait for it to shut
    Settling,      // door sensor: door closed, small delay then lock
  };

  Phase phase_ = Phase::Locked;
  bool unlocked_ = false;
  uint32_t phaseUntilMs_ = 0;
  bool doorSeenOpen_ = false;
};

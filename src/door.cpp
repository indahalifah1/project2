#include "door.h"

#include "config.h"

// Write the pin before switching it to OUTPUT: that keeps the driven level
// correct through the reset state of the port register, so the relay is never
// briefly energised at boot.
static inline void writeRelay(bool energized) {
  digitalWrite(PIN_RELAY, (RELAY_ACTIVE_LOW != energized) ? HIGH : LOW);
}

void Door::begin() {
  writeRelay(false);
  pinMode(PIN_RELAY, OUTPUT);
  writeRelay(false);  // fail-secure: locked on boot and on every reset

#if USE_DOOR_SENSOR
  pinMode(PIN_DOOR_SENSOR, INPUT_PULLUP);
#endif

  phase_ = Phase::Locked;
  unlocked_ = false;
  doorSeenOpen_ = false;
}

void Door::unlock() {
  writeRelay(true);
  unlocked_ = true;
  doorSeenOpen_ = USE_DOOR_SENSOR;
  phaseUntilMs_ = millis() + DOOR_UNLOCK_MS;
  phase_ = Phase::Unlocked;
}

void Door::lock() {
  writeRelay(false);
  unlocked_ = false;
  doorSeenOpen_ = false;
  phase_ = Phase::Locked;
}

uint16_t Door::secondsRemaining() const {
  if (!unlocked_) return 0;
  if (phase_ != Phase::Unlocked) return DOOR_UNLOCK_MS / 1000;  // waiting for the door
  // Rollover-safe "time left": cast to signed so a wrapped millis() compares
  // correctly.
  const int32_t leftMs = static_cast<int32_t>(phaseUntilMs_ - millis());
  if (leftMs <= 0) return 0;
  return static_cast<uint16_t>((leftMs + 999) / 1000);
}

void Door::tick() {
  if (!unlocked_) return;
  if (static_cast<int32_t>(millis() - phaseUntilMs_) < 0) return;  // not due yet

#if USE_DOOR_SENSOR
  if (phase_ == Phase::Unlocked) {
    // The door has not been opened yet, so the wait has not started.
    phase_ = Phase::WaitingClosed;
    return;
  }

  if (phase_ == Phase::WaitingClosed) {
    if (!doorIsClosed()) {
      doorSeenOpen_ = true;  // still standing open: hold the lock and keep waiting
      return;
    }
    if (doorSeenOpen_) {
      phase_ = Phase::Settling;
      phaseUntilMs_ = millis() + DOOR_SETTLE_MS;
    }
    return;
  }
#endif

  // The timer expired with no door switch fitted, or the door has been shut
  // and given a moment to settle. Re-lock.
  lock();
}

bool Door::doorIsClosed() const {
  const bool levelHigh = digitalRead(PIN_DOOR_SENSOR) == HIGH;
  return DOOR_CONTACT_TO_GND ? !levelHigh : levelHigh;
}

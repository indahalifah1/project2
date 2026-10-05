// ============================================================================
//  SMART DOOR LOCK
//
//  Inputs   : RC522 RFID reader (SPI) + 4x4 membrane keypad
//  Outputs  : relay driving the electric lock + 16x2 I2C LCD
//
//  Behaviour
//    * A card on the authorised list, or the master PIN, releases the lock.
//    * The lock re-locks automatically after DOOR_UNLOCK_MS, and immediately
//      once the door swings shut if a position switch is fitted
//      (USE_DOOR_SENSOR in include/config.h).
//    * Press '#' at any time to submit a PIN, '*' to clear it, and while the
//      lock is open '#' re-locks it early.
//    * MAX_PIN_ATTEMPTS denials inside ATTEMPT_WINDOW_MS start a LOCKOUT_MS
//      blackout during which both readers are ignored. Unknown cards and
//      wrong PINs share the same counter.
//
//  The lock is driven first in setup() and defaults to locked, so a reset or
//  a crash can never leave the door open.
//
//  See include/config.h for the pin map and include/credentials.h to enrol
//  cards and change the master PIN.
// ============================================================================

#include <Arduino.h>

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "credentials.h"
#include "display.h"
#include "door.h"
#include "keypad.h"
#include "rfid.h"

namespace {

constexpr uint8_t LINE0 = 0;
constexpr uint8_t LINE1 = 1;

// How long the "press # to lock" hint stays up after the lock is released.
constexpr uint16_t GRANT_HINT_MS = 2000;
// How long a rejection is shown before the display falls back to the idle
// screen, so a stale DENIED is never left up after the door re-locks.
constexpr uint16_t DENIED_HINT_MS = 3000;

Keypad4x4 keypad;
RfidReader rfid;
Door door;
Display display;

enum class Screen : uint8_t {
  Idle,      // waiting for a card or a key
  EnterPin,  // collecting digits
  Denied,    // a failed attempt is being reported
};

Screen screen = Screen::Idle;
char pinBuffer[PIN_LENGTH + 1] = {0};
uint8_t pinLength = 0;
uint32_t grantedAtMs = 0;
uint32_t deniedAtMs = 0;

uint8_t failedAttempts = 0;
uint32_t lastFailureMs = 0;
uint32_t lockedOutUntilMs = 0;

char line[LCD_COLS + 1];

// --- timing helpers --------------------------------------------------------

// Signed compare so a millis() rollover does not stall a timer.
bool reached(uint32_t deadline) {
  return static_cast<int32_t>(millis() - deadline) >= 0;
}

bool inLockout() { return !reached(lockedOutUntilMs); }

uint16_t secondsLeft(uint32_t deadline) {
  const int32_t remainingMs = static_cast<int32_t>(deadline - millis());
  if (remainingMs <= 0) return 0;
  return static_cast<uint16_t>((remainingMs + 999) / 1000);
}

// --- access control --------------------------------------------------------

void clearPin() {
  pinLength = 0;
  pinBuffer[0] = '\0';
}

void grantAccess(const __FlashStringHelper* source) {
  Serial.print(F("OPEN via "));
  Serial.println(source);
  door.unlock();
  grantedAtMs = millis();
  failedAttempts = 0;
  clearPin();
}

void denyAccess(const __FlashStringHelper* source) {
  Serial.print(F("CLOSE via "));
  Serial.println(source);

  const uint32_t now = millis();
  // Denials older than the rolling window stop counting against the user.
  if (failedAttempts > 0 && static_cast<int32_t>(now - lastFailureMs) > ATTEMPT_WINDOW_MS) {
    failedAttempts = 0;
  }
  if (failedAttempts < 0xFF) failedAttempts++;
  lastFailureMs = now;

  if (failedAttempts >= MAX_PIN_ATTEMPTS) {
    lockedOutUntilMs = now + LOCKOUT_MS;
    failedAttempts = 0;
    Serial.println(F("LOCKOUT engaged."));
  }

  clearPin();
  screen = Screen::Denied;
  deniedAtMs = millis();
}

// '*' clears, '#' submits, 0-9 append. A-D are unused.
void handlePinKey(char key) {
  if (key >= '0' && key <= '9') {
    if (pinLength < PIN_LENGTH) {
      if (pinLength == 0) Serial.print(F("PIN: "));
      Serial.print('*');
      pinBuffer[pinLength++] = key;
    }
    return;  // extra digits are ignored, '#' submits what we have
  }

  if (key == '*') {
    if (pinLength > 0) Serial.println();
    clearPin();
    return;
  }
  if (key != '#') return;

  Serial.println();
  if (pinLength == PIN_LENGTH && strncmp(pinBuffer, MASTER_PIN, PIN_LENGTH) == 0) {
    grantAccess(F("keypad PIN"));
  } else {
    if (pinLength != PIN_LENGTH) {
      Serial.println(F("CLOSE: PIN must be 4 digits."));
    } else {
      Serial.println(F("CLOSE: wrong PIN."));
    }
    denyAccess(F("keypad PIN"));
  }
}

// --- screens ---------------------------------------------------------------

// Title on the left, state right-aligned on the right, exactly 16 columns.
void setTitle(const char* state) {
  snprintf(line, sizeof(line), "SMART DOOR %5s", state);
  display.setLine(LINE0, line);
}

void drawIdle() {
  setTitle("LOCKED");
  display.setLine(LINE1, F("Scan card or PIN"));
}

void drawEnterPin() {
  setTitle("LOCKED");

  strcpy(line, "PIN:");
  for (uint8_t i = 0; i < PIN_LENGTH; ++i) {
    line[4 + i] = (i < pinLength) ? '*' : '_';
  }
  line[4 + PIN_LENGTH] = '\0';
  display.setLine(LINE1, line);
}

void drawGranted() {
  setTitle("OPEN");

  if (static_cast<uint32_t>(millis() - grantedAtMs) < GRANT_HINT_MS) {
    display.setLine(LINE1, F("Press # to lock"));
  } else {
    snprintf(line, sizeof(line), "Closes in %us  #",
             static_cast<unsigned>(door.secondsRemaining()));
    display.setLine(LINE1, line);
  }
}

void drawDenied() {
  setTitle("LOCKED");
  display.setLine(LINE1, F("CLOSED - Try PIN"));
}

void drawLockout() {
  setTitle("WAIT");
  snprintf(line, sizeof(line), "LOCKOUT %3us",
           static_cast<unsigned>(secondsLeft(lockedOutUntilMs)));
  display.setLine(LINE1, line);
}

void refreshScreen() {
  if (inLockout()) {
    // A blackout overrides an open lock, so the door cannot be held open.
    door.lock();
    clearPin();
    screen = Screen::Idle;
    drawLockout();
  } else if (door.isUnlocked()) {
    drawGranted();
  } else if (screen == Screen::EnterPin) {
    drawEnterPin();
  } else if (screen == Screen::Denied &&
             static_cast<uint32_t>(millis() - deniedAtMs) < DENIED_HINT_MS) {
    drawDenied();
  } else {
    screen = Screen::Idle;
    drawIdle();
  }
}

}  // namespace

void setup() {
  Serial.begin(9600);
  Serial.println(F("\nSmart door starting..."));

  door.begin();  // first: the lock is closed before anything else runs
  display.begin();
  keypad.begin();
  rfid.begin();

  drawIdle();
}

void loop() {
  door.tick();
  CardResult card = rfid.poll();

  if (inLockout()) {
    // Drain the pad so a held key cannot leak in the moment lockout ends, and
    // drop a card that is sitting on the reader.
    keypad.read();
    card = CardResult::NONE;
  } else if (door.isUnlocked()) {
    // A press while the lock is open re-locks it early: the door can be shut
    // on the way out and the timer restarted.
    if (keypad.read() == '#') {
      Serial.println(F("Locked manually."));
      door.lock();
    }
  } else {
    if (card == CardResult::GRANTED) {
      grantAccess(F("RFID"));
    } else if (card == CardResult::DENIED) {
      denyAccess(F("RFID"));
    }

    if (!door.isUnlocked()) {
      const char key = keypad.read();
      if (key != 0) {
        if (screen != Screen::EnterPin) {
          screen = Screen::EnterPin;  // a fresh keypress restarts the entry
        }
        handlePinKey(key);
      }
    }
  }

  refreshScreen();
  delay(5);
}

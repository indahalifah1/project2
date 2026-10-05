#include "keypad.h"

#include "config.h"

void Keypad4x4::begin() {
  for (uint8_t c = 0; c < KEYPAD_COLS; ++c) {
    pinMode(PIN_KEYPAD_COL[c], INPUT_PULLUP);
  }
  for (uint8_t r = 0; r < KEYPAD_ROWS; ++r) {
    pinMode(PIN_KEYPAD_ROW[r], INPUT);
    digitalWrite(PIN_KEYPAD_ROW[r], HIGH);
  }
}

char Keypad4x4::read() {
  const char key = scan();
  const uint32_t now = millis();

  if (key != stableKey_) {
    // A new state appeared. Restart the debounce window; a release clears the
    // "already delivered" flag so the next press can be reported again.
    stableKey_ = key;
    stableSinceMs_ = now;
    if (key == 0) {
      delivered_ = 0;
    }
    return 0;
  }

  if (key == 0) return 0;                       // nothing held
  if (delivered_) return 0;                     // already reported, wait for release
  if (now - stableSinceMs_ < KEY_DEBOUNCE_MS) return 0;  // still bouncing

  delivered_ = 1;
  return key;
}

char Keypad4x4::scan() const {
  for (uint8_t r = 0; r < KEYPAD_ROWS; ++r) {
    pinMode(PIN_KEYPAD_ROW[r], OUTPUT);
    digitalWrite(PIN_KEYPAD_ROW[r], LOW);

    for (uint8_t c = 0; c < KEYPAD_COLS; ++c) {
      if (digitalRead(PIN_KEYPAD_COL[c]) == LOW) {
        // Release the row before returning so the matrix is left idle.
        pinMode(PIN_KEYPAD_ROW[r], INPUT);
        return KEYPAD_MAP[r][c];
      }
    }

    digitalWrite(PIN_KEYPAD_ROW[r], HIGH);
    pinMode(PIN_KEYPAD_ROW[r], INPUT);
  }
  return 0;
}

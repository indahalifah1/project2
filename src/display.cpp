#include "display.h"

#include <string.h>

#include <avr/pgmspace.h>

bool Display::begin() {
  const bool found = lcd_.begin(LCD_COLS, LCD_ROWS);
  if (!found) {
    Serial.println(F("ERROR: no LCD answered on I2C. Check the wiring and LCD_I2C_ADDR in config.h."));
    return false;  // ready_ stays false, so every setLine below is a no-op
  }

  lcd_.backlight();
  lcd_.clear();
  memset(shadow_, ' ', sizeof(shadow_));  // force a full first write
  ready_ = true;
  return true;
}

void Display::setLine(uint8_t row, const __FlashStringHelper* text) {
  if (row >= LCD_ROWS) return;
  char buffer[LCD_COLS + 1];
  strncpy_P(buffer, reinterpret_cast<const char*>(text), LCD_COLS);
  buffer[LCD_COLS] = '\0';
  write(row, buffer);
}

void Display::setLine(uint8_t row, const char* text) {
  if (row >= LCD_ROWS) return;
  write(row, text);
}

void Display::write(uint8_t row, const char* text) {
  if (!ready_ || row >= LCD_ROWS) return;

  // Pad to the full width so leftovers from a longer previous line are wiped
  // without needing clear().
  char padded[LCD_COLS + 1];
  strncpy(padded, text, LCD_COLS);
  padded[LCD_COLS] = '\0';
  for (uint8_t i = 0; i < LCD_COLS; ++i) {
    if (padded[i] == '\0') padded[i] = ' ';
  }

  if (strncmp(shadow_[row], padded, LCD_COLS) == 0) return;  // nothing changed

  lcd_.setCursor(0, row);
  for (uint8_t i = 0; i < LCD_COLS; ++i) {
    lcd_.write(static_cast<uint8_t>(padded[i]));
  }

  memcpy(shadow_[row], padded, LCD_COLS);
  shadow_[row][LCD_COLS] = '\0';
}

#pragma once

#include <Arduino.h>

#include <LiquidCrystal_I2C.h>

#include "config.h"

// Only the two addresses seen on PCF8574 backpacks are supported, because the
// library takes the address as an enum: PCF8574 sits at 0x27 and the rarer
// PCF8574A at 0x3F. See LCD_I2C_ADDR in config.h.
static_assert(LCD_I2C_ADDR == 0x27 || LCD_I2C_ADDR == 0x3F,
              "LCD_I2C_ADDR must be 0x27 (PCF8574 backpack) or 0x3F (PCF8574A).");

constexpr pcf8574Address LCD_ADDRESS =
    LCD_I2C_ADDR == 0x3F ? PCF8574A_ADDR_A21_A11_A01 : PCF8574_ADDR_A21_A11_A01;

// Thin wrapper over the I2C LCD that keeps a shadow copy of both lines and only
// writes to the controller when the text actually changes.
//
// On a 16x2 HD44780 a full clear() plus a redraw is slow and visibly flickers.
// Comparing against the shadow and padding with spaces costs almost nothing and
// keeps the display rock steady.
class Display {
 public:
  // Returns false if no LCD answered on the I2C bus.
  bool begin();

  void setLine(uint8_t row, const __FlashStringHelper* text);
  void setLine(uint8_t row, const char* text);

 private:
  void write(uint8_t row, const char* text);

  LiquidCrystal_I2C lcd_{LCD_ADDRESS};
  bool ready_ = false;
  char shadow_[LCD_ROWS][LCD_COLS + 1] = {{0}};
};

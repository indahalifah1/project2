#pragma once

#include <Arduino.h>

// ============================================================================
//  SMART DOOR - hardware pin map
//
//  Arduino UNO / ATmega328P - only 20 I/O pins, so the whole board is used.
//
//   D0 / D1  Serial monitor (reserved)
//   D2 - D5  keypad 4x4 rows      (1)
//   D6 - D9  keypad 4x4 columns   (2)
//   D10      RC522 SS / SDA
//   D11      RC522 MOSI
//   D12      RC522 MISO
//   D13      RC522 SCK
//   A0       RELAY control
//   A1       RC522 RST
//   A2       door position switch (optional, see USE_DOOR_SENSOR)
//   A3       free / spare
//   A4       LCD SDA   (I2C)
//   A5       LCD SCL   (I2C)
//
//  The keypad deliberately uses digital pins only, so it never competes with
//  the LCD for the two I2C pins. The RC522 is a 3.3V part: power it from the
//  3V3 pin, and add an LED + 220R in series with its RST line if you see
//  short reads.
// ============================================================================

// NOTE ON STYLE: anything switched with #if below is a #define, NOT a
// constexpr bool. The C preprocessor runs before the C++ front end, so it
// cannot see the value of a constexpr variable - it would silently expand to
// 0 and compile out your code.

// --- Keypad 4x4 -------------------------------------------------------------
constexpr uint8_t KEYPAD_ROWS = 4;
constexpr uint8_t KEYPAD_COLS = 4;

const uint8_t PIN_KEYPAD_ROW[KEYPAD_ROWS] = {2, 3, 4, 5};
const uint8_t PIN_KEYPAD_COL[KEYPAD_COLS] = {6, 7, 8, 9};

// Physical layout of a standard 4x4 membrane keypad, row by row.
// If your keypad prints a different legend, only edit this table.
//        C1  C2  C3  C4
//   R1    1   2   3   A
//   R2    4   5   6   B
//   R3    7   8   9   C
//   R4    *   0   #   D
const char KEYPAD_MAP[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'},
};

constexpr uint8_t KEY_DEBOUNCE_MS = 40;

// --- RFID RC522 (SPI) -------------------------------------------------------
const uint8_t PIN_RFID_SS = 10;
const uint8_t PIN_RFID_MOSI = 11;
const uint8_t PIN_RFID_MISO = 12;
const uint8_t PIN_RFID_SCK = 13;
const uint8_t PIN_RFID_RST = A1;

// A card left on the reader is ignored for this long, so nobody can hold the
// door open by parking a card on it.
constexpr uint16_t RFID_REARM_MS = 3000;

// --- I2C LCD ----------------------------------------------------------------
// 0x27 is the usual PCF8574 backpack; some clones use a PCF8574A at 0x3F.
// The firmware prints an error on Serial at boot if nothing answers, and
// LiquidCrystal_I2C auto-detects the odd pin order these backpacks use.
constexpr uint8_t LCD_I2C_ADDR = 0x27;
constexpr uint8_t LCD_COLS = 16;
constexpr uint8_t LCD_ROWS = 2;

// --- Relay ------------------------------------------------------------------
const uint8_t PIN_RELAY = A0;

// Almost every hobby relay module pulls its input to GND (active LOW). Set
// this to 0 if you drove the coil from the HIGH side through a transistor.
#define RELAY_ACTIVE_LOW 1

// How long the lock stays released when no door switch is fitted.
constexpr uint16_t DOOR_UNLOCK_MS = 5000;

// --- Optional door position switch (reed switch on the door frame) ---------
// Set USE_DOOR_SENSOR to 1 after wiring a switch between A2 and GND. The lock
// then re-locks as soon as the door swings shut, instead of running the timer.
#define USE_DOOR_SENSOR 0

#define PIN_DOOR_SENSOR A2

// Switch to GND and closed when the door is closed (uses the internal
// pull-up) = 1. Switch to +5V and closed when the door is closed = 0.
#define DOOR_CONTACT_TO_GND 1

constexpr uint16_t DOOR_SETTLE_MS = 1000;  // pause after the door closes

// --- Access control ---------------------------------------------------------
constexpr uint8_t PIN_LENGTH = 4;
constexpr uint8_t MAX_PIN_ATTEMPTS = 3;
constexpr uint16_t LOCKOUT_MS = 15000;
// A denied attempt stops counting as "recent" after this long.
constexpr uint16_t ATTEMPT_WINDOW_MS = 60000;

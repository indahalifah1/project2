#pragma once

#include <Arduino.h>

#include "config.h"

// ============================================================================
//  ENROLMENT FILE - edit this, then re-upload the firmware.
//
//  To find a card's UID: run the firmware, scan an unknown card and read the
//  line the Serial monitor (9600 baud) prints:
//      UNKNOWN CARD  size=5  12 34 56 78 90
//  Copy those bytes into the table below.
// ============================================================================

// Match this size and the UID bytes to the card reported by the Serial monitor.
constexpr uint8_t UID_SIZE = 4;

// The keypad master PIN. CHANGE THIS before uploading.
const char MASTER_PIN[PIN_LENGTH + 1] = "5869";

// --- Authorised cards -------------------------------------------------------
// UIDs live in program flash, not RAM - on an UNO each card costs 0 bytes of
// the 2 KB you have, so you can add a fair number of them.
const uint8_t AUTHORIZED_UIDS[][UID_SIZE] PROGMEM = {
    {0xDB, 0x8F, 0x25, 0x02},
};

constexpr uint8_t AUTHORIZED_COUNT =
    sizeof(AUTHORIZED_UIDS) / sizeof(AUTHORIZED_UIDS[0]);

#pragma once

#include <Arduino.h>

#include <MFRC522.h>

#include "config.h"

enum class CardResult : uint8_t {
  NONE,     // no card on the reader
  GRANTED,  // card is on the authorised list
  DENIED,   // card is not authorised (or the card could not be read)
};

class RfidReader {
 public:
  void begin();

  // Call from loop(). Returns GRANTED at most once every RFID_REARM_MS, so a
  // card left sitting on the reader cannot re-trigger the lock.
  CardResult poll();

  // UID bytes of the card behind the last GRANTED/DENIED result, for logging.
  const uint8_t* lastUid() const { return lastUid_; }
  uint8_t lastUidSize() const { return lastUidSize_; }

 private:
  bool isAuthorized() const;
  void warnAboutUidSize();

  MFRC522 reader_{PIN_RFID_SS, PIN_RFID_RST};
  uint8_t lastUid_[10] = {0};
  uint8_t lastUidSize_ = 0;
  uint32_t rearmAtMs_ = 0;
  bool warnedAboutUidSize_ = false;
};

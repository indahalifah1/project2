#include "rfid.h"

#include <avr/pgmspace.h>

#include "config.h"
#include "credentials.h"

void RfidReader::begin() {
  SPI.begin();
  reader_.PCD_Init();
  reader_.PCD_SetAntennaGain(reader_.RxGain_max);  // longer read range
  Serial.println(F("RFID RC522 ready."));
}

void RfidReader::warnAboutUidSize() {
  if (warnedAboutUidSize_) return;
  warnedAboutUidSize_ = true;
  Serial.print(F("WARNING: the card UID is "));
  Serial.print(reader_.uid.size);
  Serial.print(F(" bytes but credentials.h has UID_SIZE "));
  Serial.print(UID_SIZE);
  Serial.println(F(". No card can match until you fix that number."));
}

CardResult RfidReader::poll() {
  const uint32_t now = millis();
  // Signed compare so a millis() rollover cannot break the cool-down.
  if (static_cast<int32_t>(now - rearmAtMs_) < 0) {
    return CardResult::NONE;  // still cooling down after the last card
  }

  // Non-blocking: no card present means one short SPI transaction per poll.
  if (!reader_.PICC_IsNewCardPresent() || !reader_.PICC_ReadCardSerial()) {
    return CardResult::NONE;
  }

  // uid.size is only meaningful once a card has actually been read.
  if (reader_.uid.size != UID_SIZE) warnAboutUidSize();

  const CardResult result = isAuthorized() ? CardResult::GRANTED : CardResult::DENIED;

  lastUidSize_ = reader_.uid.size < sizeof(lastUid_) ? reader_.uid.size : sizeof(lastUid_);
  memcpy(lastUid_, reader_.uid.uidByte, lastUidSize_);

  // Ignore this card until it is removed, or until the timeout expires if the
  // holder never takes it off, so one card cannot keep the door open.
  rearmAtMs_ = now + RFID_REARM_MS;

  Serial.print(result == CardResult::GRANTED ? F("OPEN card  ") : F("UNKNOWN CARD  "));
  Serial.print(F("size="));
  Serial.print(lastUidSize_);
  Serial.print(F("  "));
  for (uint8_t i = 0; i < lastUidSize_; ++i) {
    if (lastUid_[i] < 0x10) Serial.print('0');
    Serial.print(lastUid_[i], HEX);
    Serial.print(' ');
  }
  Serial.println();

  return result;
}

bool RfidReader::isAuthorized() const {
  if (reader_.uid.size != UID_SIZE) return false;

  for (uint8_t i = 0; i < AUTHORIZED_COUNT; ++i) {
    // Compare the scanned RAM UID against the flash copy of the authorised UID.
    if (memcmp_P(reader_.uid.uidByte, &AUTHORIZED_UIDS[i][0], UID_SIZE) == 0) {
      return true;
    }
  }
  return false;
}

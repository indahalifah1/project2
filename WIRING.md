# Wiring — Smart Door Lock

Arduino UNO / ATmega328P. Every one of the 20 I/O pins is spoken for, so
**change the pin map in [`include/config.h`](include/config.h)** if you rewire.

---

## 1. Bill of materials

| Qty | Part | Notes |
|-----|------|-------|
| 1 | Arduino UNO (ATmega328P) | |
| 1 | RC522 RFID module + 1 MIFARE tag | 3.3 V only — see §3 |
| 1 | 4x4 membrane keypad | 16 keys, no external resistors |
| 1 | 16x2 LCD + PCF8574 I2C backpack | 0x27 or 0x3F |
| 1 | Relay module, 1 channel, active-LOW | must switch your lock's current |
| 1 | Electric maglock / fail-secure lock | **separate 12 V supply** |
| 1 | Flyback diode 1N4007 (or 1N4148) | across the lock coil |
| 1 | Reed switch + magnet (optional) | door position, `USE_DOOR_SENSOR` |
| 1 | 220 Ω resistor + LED | in series with RC522 RST (optional) |
| 4 | 100 nF + 1x 10 µF decoupling caps | at each module's supply pins |

---

## 2. Pin map at a glance

```
                       ┌──────────────┐
        D0 / D1  ◄─────┤ Serial (USB) ├────►  9600 baud monitor
        D2 .. D5  ◄─────┤              │
        D6 .. D9  ◄─────┤   4x4 KEYPAD ├────►  R1..R4 / C1..C4
       D10 .. D13  ◄─────┤   RC522 SPI  ├────►  SS / MOSI / MISO / SCK
        A0        ◄─────┤              │
        A1        ◄─────┤              ├────►  RC522 RST
        A2        ◄─────┤    RELAY     ├────►  optional reed switch
        A3             │  (spare)     │
        A4 / A5  ◄─────┤  LCD I2C     ├────►  SDA / SCL
                       └──────────────┘
```

| UNO pin | Goes to | Direction |
|---------|---------|-----------|
| D0, D1 | USB / Serial monitor | reserved |
| D2, D3, D4, D5 | keypad R1, R2, R3, R4 | out |
| D6, D7, D8, D9 | keypad C1, C2, C3, C4 | in (pull-up) |
| D10 | RC522 SDA / SS | out |
| D11 | RC522 MOSI | out |
| D12 | RC522 MISO | in |
| D13 | RC522 SCK | out |
| A0 | relay IN | out |
| A1 | RC522 RST | out |
| A2 | reed switch | in (pull-up) |
| A3 | *spare* | — |
| A4 | LCD SDA | I2C |
| A5 | LCD SCL | I2C |

`A4`/`A5` are the hardware I2C pins — nothing else may use them.
`D0`/`D1` are left for the Serial monitor so you can see the boot log.

---

## 3. RC522 RFID reader

| RC522 pin | UNO | Note |
|-----------|-----|------|
| `3.3V` | **3V3** | ⚠ not 5V |
| `GND` | GND | |
| `RST` | **A1** | + LED + 220 Ω in series (below) |
| `SDA` / `SS` | D10 | pin may be labelled `SS` or `NSS` |
| `MOSI` | D11 | |
| `MISO` | D12 | |
| `SCK` | D13 | |
| `IRQ` | *unconnected* | not used by this project |

**Power.** The RC522 is a 3.3 V part. Power it from the UNO's `3V3` pin. Its
SPI inputs are 5 V tolerant, so the 5 V logic lines from the UNO are accepted,
and the MFRC522 library documents this setup. If you see unreliable or
intermittent reads, add a level shifter (74AHCT125) on MOSI/SCK/SS, or check
your supply first.

The UNO's `3V3` pin can only give ~50 mA. If the antenna draws more or reads
are weak, use a separate 3.3 V supply and tie the grounds together.

**RST LED.** The RC522 datasheet limits the RST pin current. Put an LED and
220 Ω in series between `A1` and `RST`:

```
A1 ──[220Ω]──|>|── RST
```

If you skip this and get flaky reads, add it. You may also leave RST tied to
3.3 V, but then the library cannot software-reset the reader, so this project
keeps it on a GPIO.

---

## 4. 4x4 keypad

A 4x4 membrane keypad has no common pin — each of the 8 pads is a switch.

| Keypad | UNO | Keypad | UNO |
|--------|-----|--------|-----|
| R1 | D2 | C1 | D6 |
| R2 | D3 | C2 | D7 |
| R3 | D4 | C3 | D8 |
| R4 | D5 | C4 | D9 |

Rows are driven one at a time and columns are read with the internal
pull-ups, so **no external resistors are needed**. The firmware drives one row
LOW, reads the columns, and releases it again.

Legend the firmware expects (see `KEYPAD_MAP` in `config.h`):

```
    C1  C2  C3  C4
R1  1   2   3   A
R2  4   5   6   B
R3  7   8   9   C
R4  *   0   #   D
```

Keypad legends vary between manufacturers. If yours differs, **swap rows/columns
in `config.h` or just edit `KEYPAD_MAP`** — that one table is the only place the
legend is described.

---

## 5. I2C LCD

| Backpack | UNO |
|----------|-----|
| `VCC` | 5V |
| `GND` | GND |
| `SDA` | A4 |
| `SCL` | A5 |

The PCF8574 backpack works at 5 V. The pull-ups on SDA/SCL are on the backpack.

**Address.** `LCD_I2C_ADDR` in `config.h` is `0x27` by default. Set it to `0x3F`
if your backpack uses the PCF8574A. The library also auto-detects the
non-standard back-to-front pin order these modules often have, so you usually do
not need `PCF8574_*_P0..P7` tweaks.

The firmware prints `ERROR: no LCD answered on I2C` on Serial at boot if the
backpack is silent — the display simply stays blank while everything else keeps
working.

---

## 6. Relay and lock — read this before wiring

### Never power a maglock from the UNO.

The UNO's 5 V regulator and the USB port cannot supply the 200–500 mA an
electric lock draws. Use a **separate 12 V supply** for the lock, sized for the
lock plus the relay coil.

### Flyback diode

Put a diode **across the lock coil, cathode to +supply** (the banded end goes
to the positive rail). Without it the inductive kick destroys the relay
contacts and can brown out the UNO. Most off-the-shelf relay modules already
include one for the relay coil, but not for your external lock.

### Relay contacts

```
   +12V ──────┬───────── COM      relay
              │         │
   diode      │         NO ────── lock +
   1N4007     │         │
   (+12V ─┤├─ GND)     NC ────── unused
              │
  lock − ─────┴───────────────── (to supply GND)
```

- **NO (normally open)** → the lock is powered *only when the relay is
  energised*. Losing power releases the door. This is **fail-safe** and is the
  usual choice for an electric maglock, because it satisfies fire-code egress.
- **NC** → choose this for a fail-secure lock (power cut = stays locked).

The firmware is fail-secure in software too: `door.begin()` drives the relay
closed before anything else runs, so a reset, a crash or a brown-out leaves the
lock closed rather than open.

### Trigger polarity

`RELAY_ACTIVE_LOW` is `1` in `config.h`, which matches the standard single
channel relay module with the `VCC / JD-VCC` jumper fitted — the module pulls
`IN` to GND to energise. If your module energises on a HIGH input, set it to `0`.

> **Gotcha:** with the `JD-VCC` jumper fitted, the module's input is pulled up to
> the **module's own VCC rail**, not the Arduino's. If you power the module from
> 12 V, remove the jumper and drive `IN` from the UNO's 5 V instead.
>
> The `IN` pin drives the module's optocoupler LED: keep it to ~1 kΩ if you wire
> it directly to a transistor, or just use the module's input as intended.

Check the relay can actually switch your load. The small signal relays inside
"1 channel relay modules" are usually fine for a maglock, but a 24 V maglock
drawing >1 A should go through a proper relay or a MOSFET.

### `A0` and the relay

A0 is the only output. It is also an analogue-capable pin, but the firmware only
ever uses it digitally.

---

## 7. Optional: door position reed switch

Skips the 5 s wait and re-locks as soon as the door shuts.

1. Set `USE_DOOR_SENSOR` to `1` in `config.h`.
2. Mount the reed switch on the **door frame** and the magnet on the **door**, so
   they touch when the door is *closed*.
3. Wire:

```
  A2 ──┬── reed switch ── GND
       │
      (internal pull-up, no external resistor)
```

`A2` idles HIGH and reads LOW while the switch is closed, so with
`DOOR_CONTACT_TO_GND = 1` (the default) the firmware treats LOW as "door is
closed". If you wire the switch to +5 V instead, set `DOOR_CONTACT_TO_GND` to
`0` — then HIGH means closed.

With this fitted, the sequence is: card → lock releases → door opens → you pull
it shut → 1 s (`DOOR_SETTLE_MS`) → lock re-engages. A tap-out button can sit
across the same reed switch if you also want manual exit.

---

## 8. Config knobs that must match your wiring

All in [`include/config.h`](include/config.h):

| Setting | Default | Change if… |
|---------|---------|-----------|
| `LCD_I2C_ADDR` | `0x27` | backpack is `0x3F` |
| `RELAY_ACTIVE_LOW` | `1` | module energises on HIGH |
| `USE_DOOR_SENSOR` | `0` | you fitted a reed switch on A2 |
| `DOOR_CONTACT_TO_GND` | `1` | reed switch wired to +5 V, not GND |
| `KEYPAD_MAP` | 1-9, A-D, \*, 0, # | your keypad legend differs |
| `KEY_DEBOUNCE_MS` | `40` | keys double-register → raise to 60 |
| `PIN_*` | §2 | you moved a wire |
| `DOOR_UNLOCK_MS` | `5000` | you want a different auto-lock delay |
| `MAX_PIN_ATTEMPTS` / `LOCKOUT_MS` | `3` / 15 s | different lockout policy |

And in [`include/credentials.h`](include/credentials.h): the master PIN
(`MASTER_PIN`) and the enrolled card UIDs. Change the PIN before uploading.

---

## 9. Before first power-up

- [ ] 3.3 V decoupling cap (100 nF + 10 µF) across the RC522's `3V3`/`GND`.
- [ ] Decoupling cap on the relay module's supply.
- [ ] Flyback diode across the lock coil, **cathode to +**.
- [ ] Lock powered from its own 12 V supply, **not** the UNO.
- [ ] Relay contacts: `COM` + `NO` for a fail-safe maglock.
- [ ] `A4`/`A5` go to the LCD and nothing else.
- [ ] `MASTER_PIN` changed from `1234` in `credentials.h`.
- [ ] Real card UID pasted into `AUTHORIZED_UIDS` (see §10).

---

## 10. First run

1. Upload, then open the Serial monitor at **9600 baud** (it is not 115200 —
   `monitor_speed` in `platformio.ini` is 9600; 115200 is only the *upload* rate).
2. The boot log should read:

   ```
   Smart door starting...
   RFID RC522 ready.
   ```

   If you see `ERROR: no LCD answered on I2C`, fix the address or the wiring.
   If you see the UID-size warning, see below.
3. Scan your card. An unenrolled card prints its UID:

   ```
   UNKNOWN CARD  size=5  12 34 56 78 90
   ```

   Copy those bytes into `AUTHORIZED_UIDS` in `credentials.h` and re-upload.
4. Enrol at least one card before the keypad-only path is useful.
5. Enter the master PIN: digits, then `#` to submit (`*` clears, `#` re-locks
   early while the lock is open).

### If nothing is ever granted

The library prints a UID-size warning when the card's UID length does not match
`UID_SIZE` in `credentials.h`. The common white MIFARE Classic 1K tags report
**5** bytes; genuine MIFARE Ultralight tags report **7**, and 4-byte tags exist
too. Set `UID_SIZE` to the `size=` value your card actually reports.

---

## 11. Troubleshooting

| Symptom | Likely cause |
|---------|--------------|
| LCD blank, no error on Serial | `LCD_I2C_ADDR` wrong → try `0x3F`; or SDA/SCL swapped |
| `ERROR: no LCD answered on I2C` | backpack unpowered, or address is neither 0x27 nor 0x3F |
| No RFID reads at all | RC522 fed 5 V instead of 3.3 V; check the antenna; try the 220 Ω + LED on RST |
| Reads work but only inches from the tag | weak 3.3 V supply; add 10 µF at the module |
| `size=4` / `size=7` in the log | set `UID_SIZE` in `credentials.h` to match |
| Every card is `UNKNOWN` | UID bytes not pasted correctly, or the tag was not enrolled |
| Keypad presses register twice | raise `KEY_DEBOUNCE_MS` to 60 |
| Some keys do nothing | keypad row/column order differs → edit `KEYPAD_MAP` |
| Relay clicks but lock does not move | wrong COM/NO/NC contact, or no separate lock supply |
| Lock is on when it should be off | set `RELAY_ACTIVE_LOW` to `0` |
| Lock never releases | you are in lockout — 3 bad attempts start a 15 s blackout |
| UNO resets when the relay switches | lock supply too small, or missing flyback diode |

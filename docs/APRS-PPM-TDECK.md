# APRS-PPM compressed frames on the T-Deck tracker (experimental)

This is an **experimental branch** of a fork of [moricef/LoRa_APRS_Tracker](https://github.com/moricef/LoRa_APRS_Tracker).
It is not a pull request and not part of that project. It adds an optional frame format that carries the same
APRS text in fewer bytes. Plain LoRa APRS frames are unchanged, and firmware that does not know the new frame
type ignores it.

The format, the reasons for it, and the measurements are described in the
[iGate branch page](https://github.com/radiohound/LoRa_APRS_iGate/blob/feature/order3-compression/tools/aprs_compress/EXPERIMENT.md)
and the [proposal repository](https://github.com/radiohound/aprs-compression-proposal). This page covers what is
specific to the T-Deck tracker.

## What it does here

* **Receive is always on.** A compressed frame (`3C FF 81` full, `3C FF C1` primed) is expanded back to the usual
  text as it is read, so the map, messaging and station list work as before. The log shows
  `Compressed frame: N B on air, M B expanded`.
* **Transmit is off by default.** Setting `loraConfig.compressTx` to `true` makes every packet the tracker sends
  (beacons, messages) go out as the smallest of a primed frame, a full compressed frame or the plain frame.
  `loraConfig.compressFullEvery` (default 2) sets how often a full or plain frame is sent.
* There is no switch for these on the settings web page yet. Edit `tracker_conf.json` (see below).

## What has been tested

| | Result |
|---|---|
| Hardware | T-Deck Plus, 433 MHz (`ttgo_t_deck_plus_433`), SX1262 |
| Receive, 24 October 2026 | 16 of 16 packets from an ESP32 sender at 433.775 MHz, SF12, 125 kHz: 8 compressed frames (4 full at 29 B, 4 primed at 9 to 10 B, against 42 to 43 B plain) all expanded to exactly the text sent; none dropped |
| Transmit, 24 October 2026 | The start-up beacon and 5 beacon-button presses, all sent with `compressTx` on. An ESP32 receiver decoded 6 of 6 exactly (full frames 23 to 24 B and primed frames 13 to 15 B against 40 B plain for a position beacon; 31 B against 75 B for the start-up beacon, which carries a comment). A Raspberry Pi 2 miles away, still on the old Direwolf, heard all 6 (-95 to -98 dBm) but could only log them as unreadable. After it was upgraded to the Direwolf branch, one further beacon (23 B) was expanded and passed to APRS-IS as an ordinary packet within a second |
| Build size | 71.8 % of the 3 MB application partition, 69.3 % of RAM |
| Other boards (T-Beam, CrowPanel, DIY) | **Not built, not tested** |

## Before you turn transmit on

Only receivers that run the compression code can read a compressed frame. Today that is this branch, a fork of the
ESP32 iGate firmware, and a Direwolf branch. **Other people's iGates will not decode your beacons**, so unless a
compression-aware receiver hears you, your station will not appear on maps that depend on them. The tracker also sends
a beacon every time it starts. Use spare equipment and your own callsign, and leave it off unless you are testing with
a receiver you control.

**A bridge is possible.** In the test above, an iGate running the Direwolf branch expanded a compressed beacon and
gated it to APRS-IS as a normal packet (`K6ATV-4>...,qAO,<igate>:...`) one second after it was sent (seen once; not
yet watched over a long period). Only the
tracker and that one iGate needed the new code; everything downstream saw ordinary APRS. So the safe way to use
compressed transmit is within range of such an iGate.

## Building it

Moricef's repository has a build problem on a fresh clone: `lib/NeoGPS` is a pointer to a commit that is not
available, and the file that says where to get it (`.gitmodules`) is missing. Until that is fixed upstream:

    git clone https://github.com/radiohound/LoRa_APRS_Tracker-moricef.git
    cd LoRa_APRS_Tracker-moricef
    git checkout ppm-compression
    rm -rf lib/NeoGPS
    git clone --depth 1 https://github.com/SlashDevin/NeoGPS lib/NeoGPS
    cp lib/gps_math/GPSfix_cfg.h lib/gps_math/NMEAGPS_cfg.h lib/gps_math/NeoGPS_cfg.h lib/NeoGPS/src/
    pio run -e ttgo_t_deck_plus_433

The last three config files are the project's own GPS settings (they switch on HDOP, VDOP and PDOP); without them the
build stops with `'class gps_fix' has no member named 'hdop'`.

Flash with `pio run -e ttgo_t_deck_plus_433 -t upload`. **Back up the flash first** if the device holds anything you
want to keep: `esptool.py --port <port> read_flash 0 0x1000000 backup.bin` (16 MB, about 30 minutes over USB).

## Turning transmit on (testing only)

Put this in `tracker_conf.json` and upload it to the device's SPIFFS (for example with `pio run -t uploadfs`):

    "loraConfig": { "sendInfo": true, "compressTx": true, "compressFullEvery": 2 }

## Files changed

`src/lora_utils.cpp` (frame reader and writer), `src/configuration.cpp` and `include/configuration.h` (the two
settings), and a new `lib/AprsCompress` (the codec and the model table; GPL-3.0 like the rest of the project).
Questions and results: please open an issue on this fork.

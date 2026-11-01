# LACF (LoRa APRS Compact Frame) on the T-Deck tracker (experimental, local branch `hybrid`)

Status: **tested on the air in both directions, 1 November 2026.** Built, unit-tested on the host, flashed to a T-Deck Plus 433. A LILYGO LoRa32 v2.1 (K6ATV-14) and the T-Deck (K6ATV-4) sent each other LACF frames at 433.775 MHz, SF12, and the hill Pi (K6ATV-10) decoded the T-Deck's frames and gated them to APRS-IS. The T-Deck transmit test used a build that forces LACF; saving the format on the settings page has not been confirmed yet. The settings page offers two choices, Text (standard) and Hybrid (LACF).

LACF, the LoRa APRS Compact Frame (called the hybrid frame in the code and on the settings page), is the native frame `3C FF 02` whose content field is PPM coded (information kinds 2 and 3). It is specified in
`lora-aprs-ideas/spec/11-native-ppm-frame-byte-plan.md` (draft); this branch carries the C reference of that spec, unchanged, in `lib/HybridFrame`.

## What the tracker does

* **Receive (always on):** a frame `3C FF 02` with information kind 2 or 3 is decoded by `HYBRID_FRAME::decode`. The spec's receiver rules apply (version known, CRC-16, canonical payload,
  RXT block checked, impossible fields dropped), and an accepted frame becomes `3C FF 01` + TNC2 + the usual RXT v2 trailer, so every other consumer (display, digipeater, iGate, KISS) sees what a text frame would have given.
  Kinds 0 and 1 (the old native frame) are still handled by the native decoder.
* **Transmit (opt-in):** `loraConfig.txFormat = 3` (web page: Tx frame format, "Hybrid"). The text packet and its RXT trailer are turned into a hybrid frame; a packet that cannot be encoded goes as text.
  Default stays 0 (text). The settings page offers only Text (standard) and Hybrid; any other stored value reads as 0.
* **Digipeating:** the tracker works on the text packet, as before. The digipeated packet is encoded again; for the same packet and tuples the content bytes are identical (tested).

## Cost

* Flash: about 181 KB (`libHybridFrame.a`: two model tables of 82 and 84 KB, the frame code 12 KB). The T-Deck Plus 433 build uses 78.2% of the 3 MB application partition (2,459,181 bytes).
* RAM: 0.9 KB static. The frame code keeps working buffers on the stack (about 10 KB at the deepest, measured on an ESP32), more than the 8 KB loop task has, so on the board it runs in a
  task of its own (16 KB stack, created at the first call, one call at a time). A tracker that never receives a hybrid frame and keeps `txFormat` at 0 never creates it.
* Time (LoRa32, 240 MHz, from the spec's benchmark): encode 4.0 ms median / 22 ms 95th percentile; receive and verify 1.9 / 11 ms; about 9% of the frame's own SF7 airtime at worst.

## Tests

`pio test -e test_hybrid_frame` (host): 40 spec vectors (encode and decode), 35 frames with RXT tuples (tuple characters and re-encoding), the 28 rejection vectors, and the kind check.
`pio run -e ttgo_t_deck_plus_433` builds the firmware. The vectors are generated from `lora-aprs-ideas/test/vectors_hb_*.txt`.

## Not done

* No on-air test of the tracker's transmit side, and no hill station that speaks the format (the Direwolf and iGate ports are separate work).
* The relay in the spec (`hb_relay`, copying the content without decoding) is not used by the tracker; it re-encodes.
* The destination list is set 1 only (the spec's table-set version 1).

## Web-Conf Mode and the WiFi access point (a fix in this branch)

In the base code, Web-Conf Mode started its access point with `WiFi.disconnect(true)`, `WiFi.mode(WIFI_MODE_NULL)` and then `WiFi.mode(WIFI_AP)`. On the T-Deck Plus (ESP32-S3) the network `LoRa-Tracker-AP` then appeared in the list and answered probe requests, but no phone or Mac could ever join it: no join event was logged, and the phones reported a wrong password.

It was found by elimination on 1 November 2026, all with the same password and the same Arduino core (2.0.9):

| Test | Result |
|---|---|
| Bare access point sketch on a LoRa32, newer and older Arduino core | joined at once |
| The same sketch on the T-Deck itself | joined at once |
| Tracker with the CPU at 240 MHz, then also with 17 KB of memory given back | still no join |
| Tracker with a plain `WiFi.mode(WIFI_AP)` and Bluetooth left alone | joined at once, repeatedly |
| Tracker with a plain `WiFi.mode(WIFI_AP)` and the original Bluetooth shutdown (`btStop()`) | joined at once, repeatedly |

So the cause is the off-and-on mode sequence, and the fix is the plain call (in `startAPModeNonBlocking()`). Not shown to be needed: the 240 MHz CPU, which is kept while Web-Conf Mode is open. Also done in this branch: the 16 KB worker task of the LACF code is released when Web-Conf Mode opens (free internal memory was down to 25 KB; with two phones on the web page it fell to 9.5 KB at its lowest, and the largest free block shrank from 32 KB to 9.7 KB after use, which is why a second visit in the same session may need a reboot first).

The setting is on the web page under **Digipeater**, last row, **Tx frame format**. It does not turn on digipeating.

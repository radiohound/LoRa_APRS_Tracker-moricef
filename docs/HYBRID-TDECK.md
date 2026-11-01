# Hybrid frames on the T-Deck tracker (experimental, local branch `hybrid`)

Status: **receive tested on the air, transmit not yet.** Built, unit-tested on the host, and flashed to a T-Deck Plus 433. On 1 November 2026 a LILYGO LoRa32 v2.1 (K6ATV-14, `lora-aprs-ideas/tools/hb/esp32-radio`) sent 5 hybrid frames at 433.775 MHz, SF12, 10 dBm; the T-Deck decoded all 5 and expanded them to the full TNC2 text, with the RXT tuple of one frame arriving as the RXT v2 trailer. Transmit from the T-Deck (`txFormat = 3`) has not been tried.

The hybrid frame is the native frame `3C FF 02` whose content field is PPM coded (information kinds 2 and 3). It is specified in
`lora-aprs-ideas/spec/11-native-ppm-frame-byte-plan.md` (draft); this branch carries the C reference of that spec, unchanged, in `lib/HybridFrame`.

## What the tracker does

* **Receive (always on):** a frame `3C FF 02` with information kind 2 or 3 is decoded by `HYBRID_FRAME::decode`. The spec's receiver rules apply (version known, CRC-16, canonical payload,
  RXT block checked, impossible fields dropped), and an accepted frame becomes `3C FF 01` + TNC2 + the usual RXT v2 trailer, so every other consumer (display, digipeater, iGate, KISS) sees what a text frame would have given.
  Kinds 0 and 1 (the old native frame) are still handled by the native decoder.
* **Transmit (opt-in):** `loraConfig.txFormat = 3` (web page: Tx frame format, "Hybrid"). The text packet and its RXT trailer are turned into a hybrid frame; a packet that cannot be encoded goes as text.
  Default stays 0 (text). Modes 0, 1 and 2 are unchanged.
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

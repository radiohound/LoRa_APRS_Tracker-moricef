# LACF (LoRa APRS Compact Frame) on the T-Deck tracker (experimental, local branch `hybrid`)

Status: **tested on the air in both directions, 1 November 2026, and the relay with RXT on the air, 2 November 2026.** Built, unit-tested on the host, flashed to a T-Deck Plus 433. A LILYGO LoRa32 v2.1 (K6ATV-14) and the T-Deck (K6ATV-4) sent each other LACF frames at 433.775 MHz, SF12, and the hill Pi (K6ATV-10) decoded the T-Deck's frames and gated them to APRS-IS. The T-Deck transmit test used a build that forces LACF; saving the format on the settings page has not been confirmed yet. The settings page offers two choices, Text (standard) and Hybrid (LACF).

LACF, the LoRa APRS Compact Frame (called the hybrid frame in the code and on the settings page), is the native frame `3C FF 02` whose content field is PPM coded (information kinds 2 and 3). It is specified in
`lora-aprs-ideas/spec/11-native-ppm-frame-byte-plan.md` (draft); this branch carries the C reference of that spec, unchanged, in `lib/HybridFrame`.

## What the tracker does

* **Receive (always on):** a frame `3C FF 02` with information kind 2 or 3 is decoded by `HYBRID_FRAME::decode`. The spec's receiver rules apply (version known, CRC-16, canonical payload,
  RXT block checked, impossible fields dropped), and an accepted frame becomes `3C FF 01` + TNC2 + the usual RXT v2 trailer, so every other consumer (display, digipeater, iGate, KISS) sees what a text frame would have given.
  Kinds 0 and 1 (the old native frame) are still handled by the native decoder.
* **Transmit (opt-in):** `loraConfig.txFormat = 3` (web page: Tx frame format, "Hybrid"). The text packet and its RXT trailer are turned into a hybrid frame; a packet that cannot be encoded goes as text.
  Default stays 0 (text). The settings page offers only Text (standard) and Hybrid; any other stored value reads as 0.
* **Digipeating (relay):** when the Repeater setting is on (Settings, Repeater) and the Tx frame format is Hybrid, the tracker relays an LACF frame as spec 11, "Relay", says. The alias rule that rewrites the path is the tracker's own, as before. The frame it sends is then built from the received bytes (`hb_relay_to` of the reference): source, destination, position block and content are copied as received, the tuples of the earlier hops are kept, and the tracker's own tuple is added with the RSSI, SNR and frequency offset of the reception (F4MLV's scales, `rxt_scale.h`) and the time to hop, filled in just before it transmits (the random delay, the PTT pre-delay and the airtime are included). A relay adds 9 bytes to a frame relayed at `WIDE1-1` (4 for the longer path, 5 for the first tuple), 4 for each later tuple. A frame that would be longer than 255 bytes with the tuple goes out without it, and one that is too long even then is not relayed as LACF. When the frame cannot be relayed that way, or the Tx format is Text, the packet is sent as before. The serial log says `Hybrid Tx N bytes (text frame M bytes), relayed, RXT tuple added, time to hop T ms`. Run on the air on 2 November 2026: the T-Deck relayed a LoRa32 beacon of 29 bytes as 38 bytes and the LoRa32 decoded the tuple.

## Cost

* Flash: about 181 KB (`libHybridFrame.a`: two model tables of 82 and 84 KB, the frame code 12 KB). The T-Deck Plus 433 build uses 74.0% of the 3 MB application partition (2,327,385 bytes) since the earlier whole-text codec was removed, and the relay adds about 6 KB.
* RAM: 214,176 bytes of fixed RAM in all, 960 bytes more than the unmodified base (213,216 bytes). The LACF code itself needs about 3 KB of it. The frame code keeps working buffers on the stack (about 10 KB at the deepest, measured on an ESP32), more than the 8 KB loop task has, so on the board it runs in a
  task of its own (16 KB stack, created at the first call, one call at a time). A tracker that never receives a hybrid frame and keeps `txFormat` at 0 never creates it.
* Time (LoRa32, 240 MHz, from the spec's benchmark): encode 4.0 ms median / 22 ms 95th percentile; receive and verify 1.9 / 11 ms; about 9% of the frame's own SF7 airtime at worst.

## Tests

`pio test -e test_hybrid_frame` (host): 40 spec vectors (encode and decode), 35 frames with RXT tuples (tuple characters and re-encoding), the 28 rejection vectors, the kind check, a sample of the spec relay vectors (first hop, later hops, edge cases) run through the relay, and the RXT scales (the spec's example tuple `32 14 2D 0A`: 11 test cases).
`pio run -e ttgo_t_deck_plus_433` builds the firmware. The vectors are generated from `lora-aprs-ideas/test/vectors_hb_*.txt`.

## Not done

* A frame through two relays on the air (one hop has been run in each direction, T-Deck and LoRa32), and a packet near the 255-byte limit relayed on the air (host tests only).
* That the hill station received the T-Deck's relayed frames (not checked in the collector).
* The frequency offset of the tuple sits at the end of its scale for these boards (see `lora-aprs-ideas/docs/rxt-frequency-offset-scale.md`).
* The destination list is set 1 only (the spec's table-set version 1).

## Web-Conf Mode and the WiFi access point (a fix in this branch)

In this branch, Web-Conf Mode started its access point the way the base code does: `WiFi.disconnect(true)`, `WiFi.mode(WIFI_MODE_NULL)` and then `WiFi.mode(WIFI_AP)`. On the T-Deck Plus (ESP32-S3) the network `LoRa-Tracker-AP` then appeared in the list and answered probe requests, but no phone or Mac could join it: no join event was logged, and the phones reported a wrong password.

**This is not a bug in the base code.** The unmodified base firmware (`moricef/LoRa_APRS_Tracker`, branch `devel`, commit `83dfb4a`), which uses the same three-call sequence, was built and flashed to the same T-Deck Plus on 2 November 2026, and a phone joined its access point at once. So the failure belongs to this branch, and nothing should be reported to the base project.

The elimination tests, all with the same password and the same Arduino core (2.0.9):

| Test | Result |
|---|---|
| Unmodified base firmware (`devel` 83dfb4a), three-call sequence, same T-Deck Plus (2 November) | joined at once |
| Bare access point sketch on a LoRa32, newer and older Arduino core | joined at once |
| The same sketch on the T-Deck itself | joined at once |
| This branch with the CPU at 240 MHz, then also with 17 KB of memory given back | still no join |
| This branch with a plain `WiFi.mode(WIFI_AP)` and Bluetooth left alone | joined at once, repeatedly |
| This branch with a plain `WiFi.mode(WIFI_AP)` and the original Bluetooth shutdown (`btStop()`) | joined at once, repeatedly |

The fix is the plain call, in both places where the access point is started (`startAPModeNonBlocking()` and `startBlockingWebConfig()`). Not shown to be needed: the 240 MHz CPU, which is kept while Web-Conf Mode is open.

**Why only this branch is not known.** The likely reason is memory. This branch had about 15 KB more fixed RAM than the base when this was found (PlatformIO reported 228,848 bytes against 213,216; since 2 November the earlier whole-text codec is gone and it is 214,176), from the compression and LACF code, and in Web-Conf Mode free internal memory was down to 25 KB; with two phones on the web page it fell to 9.5 KB at its lowest, and the largest free block shrank from 32 KB to 9.7 KB after use (so a second visit in the same session may need a reboot first). Switching the radio off and on needs fresh memory, and so does a phone's join handshake. But giving 17 KB back did not fix the three-call sequence, so the total alone does not explain it. The shape of the free memory, or something else this branch adds, may. It is not proved.

**Outcome (2 November 2026):** with the plain call and the earlier whole-text codec gone (fixed RAM down to 214,176 bytes, within 1 KB of the base), the author joined the T-Deck's access point many times in Web-Conf Mode, with no failure. The cause of the original failure is still not proved (memory is the likely one); the failure no longer happens on the current build, and this is treated as closed.

Also done in this branch: the 16 KB worker task of the LACF code is released when Web-Conf Mode opens, and the access point now logs what it sees: the free internal memory and the largest free block when it starts and at each join, every station that joins or leaves, and a warning when stations have been probing for 20 seconds without any join. A phone that says "wrong password" can be reporting a join that did not complete; the access point logged no event at all for those failed joins, so the device cannot tell them apart from a wrong password.

The setting is on the web page under **Digipeater**, last row, **Tx frame format**. It does not turn on digipeating.

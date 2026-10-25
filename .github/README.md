# Experimental branch: compressed LoRa APRS frames on the T-Deck tracker

This is a branch of a **fork** of [moricef/LoRa_APRS_Tracker](https://github.com/moricef/LoRa_APRS_Tracker)
(the original project and its own README are there). It is an experiment, **not a pull request** and not part of that
project. It adds an optional frame format that carries the same APRS text in fewer bytes.

It is shown as an idea that works, not as a request: **no pull request will be opened against the original project
unless its maintainer says a pull request would be welcome.**

**Start here: [what it does on the T-Deck, what has been tested, how to build it, and the warnings](https://github.com/radiohound/LoRa_APRS_Tracker-moricef/blob/ppm-compression/docs/APRS-PPM-TDECK.md)**

## In short

* Plain LoRa APRS frames are unchanged. A compressed frame is a different frame type that firmware which does not
  know it ignores.
* **Receive is always on. Transmit is off by default.**
* Tested so far (24 October 2026, T-Deck Plus 433 MHz, SF12): the T-Deck received 16 of 16 packets from an ESP32
  sender, including 8 compressed frames (9 to 29 B on air against 42 to 43 B plain), all decoded exactly.
  Sending compressed frames from the T-Deck was tested the same day: an ESP32 decoded 6 of 6 beacons exactly
  (position beacons 13 to 15 B primed or 23 to 24 B full, against 40 B plain), and, after the hill iGate was upgraded to our Direwolf
  branch, one further beacon was expanded and gated to APRS-IS as a normal packet one second later.
* Not tested: other boards, other spreading factors, weak signals, and how other firmware reacts to a compressed frame.
  Please use spare equipment.
* A fresh clone of the original repository does not build because of a missing NeoGPS submodule; the page above has the
  workaround.

## More

* [The same idea in the ESP32 iGate firmware](https://github.com/radiohound/LoRa_APRS_iGate/tree/feature/order3-compression), with the format description
* [Specifications, tests and results](https://github.com/radiohound/aprs-compression-proposal) (the proposal repository)
* Questions, results and problems: please open an [issue on this fork](https://github.com/radiohound/LoRa_APRS_Tracker-moricef/issues).

Licence: GPL-3.0, as in the original project.

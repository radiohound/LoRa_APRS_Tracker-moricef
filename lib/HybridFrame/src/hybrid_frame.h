// APRS hybrid B frame (spec 11, lora-aprs-ideas/spec/11-native-ppm-frame-byte-plan.md): the native frame 3C FF 02 with a PPM-coded content field.
// Kinds 2 and 3 of the native header; the frame code is the C reference of that spec (hb.c), unchanged, in lib/HybridFrame/src.
#ifndef HYBRID_FRAME_H_
#define HYBRID_FRAME_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "native_aprs.h"

namespace HYBRID_FRAME {

    // True for 3C FF 02 (the tested 2.0.0 frame) and for 3C FF 03 (the experimental 3.0.0-dev.1 frame, spec 12; a second decoder, local experiment).
    bool isHybrid(const uint8_t* data, size_t size);

    // TNC2 packet (and its RXT tuples, one per path element, characters '!'..'z') to a hybrid frame.
    // False when the packet cannot be encoded: the caller then sends it as text.
    bool encode(const std::string& tnc2, const std::vector<NATIVE_APRS::RxtTuple>& rxt, std::vector<uint8_t>& out);

    // The same for a 3C FF 03 frame (3.0.0-dev.1, no capability bytes). Tuple characters are '!'..'z' only for now.
    // cap, if given, points to the four capability bytes (V bit 7 is then set and they follow V).
    bool encode3(const std::string& tnc2, const std::vector<NATIVE_APRS::RxtTuple>& rxt, std::vector<uint8_t>& out, const uint8_t* cap = nullptr);

    // The front of a 3C FF 03 frame whatever its model: the structure and the CRC (over the bytes as sent) are checked, the content is not decoded. frontText is
    // "SRC>DST,PATH:" (DST is "?" when its code is not assigned), so a digipeater can apply its own path rule to a frame it cannot decode; relay() then forwards it
    // without the model. key, if given, gets the frame's CRC-16, to recognise a repeat. False for any other frame.
    bool front(const uint8_t* data, size_t size, std::string& frontText, unsigned* key = nullptr);

    // The outgoing frame of a relay (spec 11, "Relay"): the received frame rxFrame with the new path newTnc2 (the digipeater's rule has already been applied to it, so it holds
    // ownCall + "*" as the element it used; the packet text without any RXT trailer), the tuples of the earlier hops kept, and the relay's own 4-byte tuple (ownTuple, normally with a
    // placeholder time to hop) added at its path element. The source, destination, position block and content are copied as received. ownTupleOffset gets the offset of the relay's
    // tuple in out, or -1 when it has none: a frame that would be longer than 255 bytes with it goes out without it, and a relay at the eighth path element or later has none.
    // False when the frame must not be relayed as LACF (it does not decode, it carries a tuple on the relay's own element or a later one, the own element is not in the new path,
    // only the path may differ from the received packet, or it is too long even without the tuple): the caller then sends the packet the usual way.
    bool relay(const uint8_t* rxFrame, size_t rxLen, const std::string& newTnc2, const std::string& ownCall, const uint8_t ownTuple[4],
               std::vector<uint8_t>& out, int* ownTupleOffset);

    // Gives back the memory of the worker task (16 KB stack) until the next call needs it again. Used when memory is needed elsewhere, e.g. Web-Conf Mode.
    void shutdown();

    // Hybrid frame to TNC2 and RXT tuples. False when the receiver rules of the spec drop the frame.
    // capOut (four bytes) and hasCap, if given, get the capability bytes of a 3C FF 03 frame that carries them.
    bool decode(const uint8_t* data, size_t size, std::string& tnc2, std::vector<NATIVE_APRS::RxtTuple>& rxt, uint8_t* capOut = nullptr, bool* hasCap = nullptr);

}

#endif

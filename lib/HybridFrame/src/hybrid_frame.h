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

    // True for 3C FF 02 with information kind 2 or 3 (the native decoder only knows 0 and 1).
    bool isHybrid(const uint8_t* data, size_t size);

    // TNC2 packet (and its RXT tuples, one per path element, characters '!'..'z') to a hybrid frame.
    // False when the packet cannot be encoded: the caller then sends it as text.
    bool encode(const std::string& tnc2, const std::vector<NATIVE_APRS::RxtTuple>& rxt, std::vector<uint8_t>& out);

    // Hybrid frame to TNC2 and RXT tuples. False when the receiver rules of the spec drop the frame.
    bool decode(const uint8_t* data, size_t size, std::string& tnc2, std::vector<NATIVE_APRS::RxtTuple>& rxt);

}

#endif

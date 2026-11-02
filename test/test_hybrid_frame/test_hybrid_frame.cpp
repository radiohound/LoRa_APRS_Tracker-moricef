#include "hybrid_frame.h"
#include "vectors.h"
#include "relay_vectors.h"
#include "rxt_scale.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <cstdio>

static std::vector<uint8_t> hexBytes(const char* h) {
    std::vector<uint8_t> b;
    for (size_t i = 0; h[i] && h[i + 1]; i += 2) b.push_back((uint8_t)std::stoi(std::string(h + i, 2), nullptr, 16));
    return b;
}
static std::string hexText(const char* h) {
    auto b = hexBytes(h);
    return std::string(b.begin(), b.end());
}

TEST(HybridFrame, EncodesAndDecodesTheSpecVectors) {
    for (const HbVec& v : HB_VECTORS) {
        std::string tnc2 = hexText(v.tnc2);
        std::vector<uint8_t> frame;
        ASSERT_TRUE(HYBRID_FRAME::encode(tnc2, {}, frame)) << v.frame;
        EXPECT_EQ(frame, hexBytes(v.frame));
        std::string back;
        std::vector<NATIVE_APRS::RxtTuple> rxt;
        ASSERT_TRUE(HYBRID_FRAME::decode(frame.data(), frame.size(), back, rxt));
        EXPECT_EQ(back, tnc2);
        EXPECT_TRUE(rxt.empty());
    }
}

TEST(HybridFrame, RxtTuplesBecomeRxtCharactersAndBack) {
    for (const HbVec& v : HB_RELAYED) {
        std::vector<uint8_t> frame = hexBytes(v.frame);
        std::string tnc2;
        std::vector<NATIVE_APRS::RxtTuple> rxt;
        ASSERT_TRUE(HYBRID_FRAME::decode(frame.data(), frame.size(), tnc2, rxt)) << v.frame;
        EXPECT_EQ(tnc2, hexText(v.tnc2));
        ASSERT_GE(rxt.size(), 1u);
        for (const auto& t : rxt)
            for (int q = 0; q < 4; q++) { EXPECT_GE(t.tuple[q], '!'); EXPECT_LE(t.tuple[q], 'z'); }
        // the tracker re-encodes after a digipeat: same packet and tuples give the same bytes (the content is deterministic)
        std::vector<uint8_t> again;
        ASSERT_TRUE(HYBRID_FRAME::encode(tnc2, rxt, again));
        EXPECT_EQ(again, frame) << v.frame;
    }
}

TEST(HybridFrame, FirstRelayTupleIsRssi50AsCharacterS) {
    // worked example of spec 11, after the first relay: tuple 50 20 45 10 on path element 0
    std::vector<uint8_t> frame = hexBytes("3cff022b895994a62c00a159a392e8120001b5f81655eb2f3e011bd55e93af550a8e18e207495b32142d0a81");
    std::string tnc2;
    std::vector<NATIVE_APRS::RxtTuple> rxt;
    ASSERT_TRUE(HYBRID_FRAME::decode(frame.data(), frame.size(), tnc2, rxt));
    EXPECT_EQ(tnc2, "N0CALL-9>APLRG1,N0RLY-1*,WIDE2-2:!3722.40N/12158.93W>LoRa APRS iGate on the hill");
    ASSERT_EQ(rxt.size(), 1u);
    EXPECT_EQ(rxt[0].index, 0);
    EXPECT_EQ(rxt[0].tuple[0], (char)(50 + 33));
    EXPECT_EQ(rxt[0].tuple[1], (char)(20 + 33));
    EXPECT_EQ(rxt[0].tuple[2], (char)(45 + 33));
    EXPECT_EQ(rxt[0].tuple[3], (char)(10 + 33));
}

TEST(HybridFrame, IsHybridOnlyForKinds2And3) {
    uint8_t k2[6] = {0x3C, 0xFF, 0x02, 0x02, 0, 0};
    uint8_t k3[6] = {0x3C, 0xFF, 0x02, 0x03, 0, 0};
    uint8_t k1[6] = {0x3C, 0xFF, 0x02, 0x01, 0, 0};
    uint8_t text[6] = {0x3C, 0xFF, 0x01, 0x02, 0, 0};
    EXPECT_TRUE(HYBRID_FRAME::isHybrid(k2, 6));
    EXPECT_TRUE(HYBRID_FRAME::isHybrid(k3, 6));
    EXPECT_FALSE(HYBRID_FRAME::isHybrid(k1, 6));
    EXPECT_FALSE(HYBRID_FRAME::isHybrid(text, 6));
    EXPECT_FALSE(HYBRID_FRAME::isHybrid(k2, 3));
}

TEST(HybridFrame, DoesNotEncodeWhatTheSpecSaysIsNotEncodable) {
    for (const char* h : HB_NOENC) {
        std::vector<uint8_t> frame;
        EXPECT_FALSE(HYBRID_FRAME::encode(hexText(h), {}, frame)) << h;
    }
}

TEST(HybridFrame, DropsTheRejectionVectors) {
    for (const char* h : HB_REJECT) {
        std::vector<uint8_t> frame = hexBytes(h);
        std::string tnc2;
        std::vector<NATIVE_APRS::RxtTuple> rxt;
        EXPECT_FALSE(HYBRID_FRAME::decode(frame.data(), frame.size(), tnc2, rxt)) << h;
    }
}

TEST(HybridFrame, AnUnencodablePacketIsRefused) {
    std::vector<uint8_t> frame;
    EXPECT_FALSE(HYBRID_FRAME::encode("no colon here", {}, frame));
    EXPECT_FALSE(HYBRID_FRAME::encode("N0CALL>APLRG1,,WIDE1-1:>x", {}, frame));
}

TEST(HybridFrame, RxtOutOfRangeIsLeftOutNotSent) {
    NATIVE_APRS::RxtTuple bad;
    bad.index = 0; bad.tuple[0] = '~'; bad.tuple[1] = 'A'; bad.tuple[2] = 'A'; bad.tuple[3] = 'A';   // '~' is above 'z'
    std::vector<uint8_t> frame;
    ASSERT_TRUE(HYBRID_FRAME::encode("N0CALL>APLRG1,WIDE1-1:>x", {bad}, frame));
    std::string tnc2;
    std::vector<NATIVE_APRS::RxtTuple> rxt;
    ASSERT_TRUE(HYBRID_FRAME::decode(frame.data(), frame.size(), tnc2, rxt));
    EXPECT_TRUE(rxt.empty());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// The relay (spec 11, "Relay"): the received frame with the new path, the earlier hops' tuples kept, the relay's own tuple added.
TEST(HybridFrame, RelayMatchesTheReferenceRelayVectors) {
    for (const RelayVec& v : RELAY_VECTORS) {
        std::vector<uint8_t> rx = hexBytes(v.rx), want = hexBytes(v.relayed), got;
        std::string back;
        std::vector<NATIVE_APRS::RxtTuple> old;
        ASSERT_TRUE(HYBRID_FRAME::decode(rx.data(), rx.size(), back, old)) << v.rx;
        int k = (int)old.size();
        char call[16];
        std::snprintf(call, sizeof call, "N0RLY-%d", k + 1);
        const uint8_t tuple[4] = { (uint8_t)(50 + k), (uint8_t)(20 + k), (uint8_t)(45 + k), (uint8_t)(10 + k) };
        int off = -2;
        ASSERT_TRUE(HYBRID_FRAME::relay(rx.data(), rx.size(), hexText(v.tnc2), call, tuple, got, &off)) << v.rx;
        EXPECT_EQ(got, want);
        if (off >= 0) {
            ASSERT_EQ(off + 5, (int)got.size());
            EXPECT_TRUE(std::equal(tuple, tuple + 4, got.begin() + off));
        } else {
            EXPECT_FALSE(got.size() > 4 && (got[3] & 0x08) && std::equal(tuple, tuple + 4, got.end() - 5));
        }
        std::string text;
        std::vector<NATIVE_APRS::RxtTuple> now;
        ASSERT_TRUE(HYBRID_FRAME::decode(got.data(), got.size(), text, now));
        EXPECT_EQ(text, hexText(v.tnc2));
        EXPECT_GE(now.size(), old.size());
    }
}

TEST(HybridFrame, RelayRefusesWhatItMustNotForward) {
    std::vector<uint8_t> rx = hexBytes(RELAY_VECTORS[0].rx), out;
    const uint8_t tuple[4] = { 50, 20, 45, 10 };
    std::string tnc2 = hexText(RELAY_VECTORS[0].tnc2);
    EXPECT_FALSE(HYBRID_FRAME::relay(rx.data(), rx.size(), tnc2, "N0OTHR-9", tuple, out, nullptr));      // the new path has no element of this relay
    std::string other = tnc2;
    other[other.size() - 1] ^= 1;                                                                           // only the path may differ
    EXPECT_FALSE(HYBRID_FRAME::relay(rx.data(), rx.size(), other, "N0RLY-1", tuple, out, nullptr));
}

TEST(HybridFrame, RxtScalesMatchTheSpecExampleTuple) {
    // 32 14 2D 0A: RSSI -80 dBm, SNR -14 dB, frequency offset 0 Hz, ten steps of time to hop (SF12, 125 kHz)
    EXPECT_EQ(RXT_Scale::rssi(-80), 0x32);
    EXPECT_EQ(RXT_Scale::snr(-14.0f), 0x14);
    EXPECT_EQ(RXT_Scale::frequencyOffset(0), 0x2D);
    EXPECT_EQ(RXT_Scale::timeToHop(380, RXT_Scale::tthScale(12, 125000)), 0x0A);
    EXPECT_EQ(RXT_Scale::rssi(-200), 0);
    EXPECT_EQ(RXT_Scale::rssi(0), 89);
    EXPECT_EQ(RXT_Scale::snr(40.0f), 88);
    EXPECT_EQ(RXT_Scale::frequencyOffset(-9999), 0);
    EXPECT_EQ(RXT_Scale::frequencyOffset(9999), 89);
    EXPECT_EQ(RXT_Scale::timeToHop(100000000UL, 327.68f), 89);
}

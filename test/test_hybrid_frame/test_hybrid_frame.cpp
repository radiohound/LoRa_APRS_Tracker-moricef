#include "hybrid_frame.h"
#include "vectors.h"
#include <gtest/gtest.h>

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

#include "telemetry_parser.hpp"
#include <gtest/gtest.h>

namespace {
void buildPacket(int16_t ax, int16_t ay, int16_t az, uint8_t* out_frame) {
    out_frame[0] = TelemetryParser::SYNC_BYTE_1;
    out_frame[1] = TelemetryParser::SYNC_BYTE_2;
    out_frame[2] = static_cast<uint8_t>((ax >> 8) & 0xFF);
    out_frame[3] = static_cast<uint8_t>(ax & 0xFF);
    out_frame[4] = static_cast<uint8_t>((ay >> 8) & 0xFF);
    out_frame[5] = static_cast<uint8_t>(ay & 0xFF);
    out_frame[6] = static_cast<uint8_t>((az >> 8) & 0xFF);
    out_frame[7] = static_cast<uint8_t>(az & 0xFF);
    out_frame[8] = TelemetryParser::computeXorChecksum(&out_frame[2]);
}
}

TEST(TelemetryParserTest, LevelFlightZeroPitchAndBank) {
    uint8_t frame[9];
    buildPacket(0, 0, 16384, frame); // +1g on Z axis

    HudState state;
    EXPECT_TRUE(TelemetryParser::parseFrame(frame, 9, state));
    EXPECT_TRUE(state.valid);
    EXPECT_NEAR(state.pitch_deg, 0.0, 0.01);
    EXPECT_NEAR(state.bank_deg,  0.0, 0.01);
}

TEST(TelemetryParserTest, RightBank45Degrees) {
    uint8_t frame[9];
    buildPacket(0, 11585, 11585, frame); // Equal Y and Z gravity components

    HudState state;
    EXPECT_TRUE(TelemetryParser::parseFrame(frame, 9, state));
    EXPECT_NEAR(state.pitch_deg, 0.0,  0.1);
    EXPECT_NEAR(state.bank_deg,  45.0, 0.1);
}

TEST(TelemetryParserTest, NoseUpPitch45Degrees) {
    uint8_t frame[9];
    buildPacket(-11585, 0, 11585, frame); // Negative X + Positive Z

    HudState state;
    EXPECT_TRUE(TelemetryParser::parseFrame(frame, 9, state));
    EXPECT_NEAR(state.pitch_deg, 45.0, 0.1);
    EXPECT_NEAR(state.bank_deg,  0.0,  0.1);
}

TEST(TelemetryParserTest, RejectsInvalidSyncHeader) {
    uint8_t frame[9];
    buildPacket(0, 0, 16384, frame);
    frame[0] = 0x00; // Corrupt sync header

    HudState state;
    EXPECT_FALSE(TelemetryParser::parseFrame(frame, 9, state));
    EXPECT_FALSE(state.valid);
}

TEST(TelemetryParserTest, RejectsCorruptedXorChecksum) {
    uint8_t frame[9];
    buildPacket(1000, 2000, 15000, frame);
    frame[4] ^= 0x01; // Flip 1 bit in payload without updating checksum

    HudState state;
    EXPECT_FALSE(TelemetryParser::parseFrame(frame, 9, state));
    EXPECT_FALSE(state.valid);
}
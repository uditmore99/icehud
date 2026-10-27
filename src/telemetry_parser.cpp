#include "telemetry_parser.hpp"
#include <cmath>

namespace {
constexpr double RAD_TO_DEG = 180.0 / 3.14159265358979323846;
}

uint8_t TelemetryParser::computeXorChecksum(const uint8_t* payload6) {
    uint8_t crc = 0;
    for (int i = 0; i < 6; ++i) {
        crc ^= payload6[i];
    }
    return crc;
}

void TelemetryParser::computeAttitudeFromRaw(int16_t raw_ax, int16_t raw_ay, int16_t raw_az,
                                             double& pitch_deg, double& bank_deg) {
    const double ax = static_cast<double>(raw_ax);
    const double ay = static_cast<double>(raw_ay);
    const double az = static_cast<double>(raw_az);

    const double norm = std::sqrt(ax * ax + ay * ay + az * az);
    if (norm < 1e-3) {
        pitch_deg = 0.0;
        bank_deg  = 0.0;
        return;
    }

    pitch_deg = std::atan2(-ax, std::sqrt(ay * ay + az * az)) * RAD_TO_DEG;
    bank_deg  = std::atan2(ay, az) * RAD_TO_DEG;
}

bool TelemetryParser::parseFrame(const uint8_t* frame, std::size_t len, HudState& out_state) {
    if (!frame || len < FRAME_SIZE) {
        out_state.valid = false;
        return false;
    }

    if (frame[0] != SYNC_BYTE_1 || frame[1] != SYNC_BYTE_2) {
        out_state.valid = false;
        return false;
    }

    const uint8_t expected_crc = computeXorChecksum(&frame[2]);
    if (frame[8] != expected_crc) {
        out_state.valid = false;
        return false;
    }

    const int16_t raw_ax = static_cast<int16_t>((static_cast<uint16_t>(frame[2]) << 8) | frame[3]);
    const int16_t raw_ay = static_cast<int16_t>((static_cast<uint16_t>(frame[4]) << 8) | frame[5]);
    const int16_t raw_az = static_cast<int16_t>((static_cast<uint16_t>(frame[6]) << 8) | frame[7]);

    if (raw_ax == 0 && raw_ay == 0 && raw_az == 0) {
        out_state.valid = false;
        return false;
    }

    computeAttitudeFromRaw(raw_ax, raw_ay, raw_az, out_state.pitch_deg, out_state.bank_deg);
    out_state.valid = true;
    return true;
}
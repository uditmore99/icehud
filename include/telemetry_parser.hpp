#ifndef TELEMETRY_PARSER_HPP
#define TELEMETRY_PARSER_HPP

#include <cstdint>
#include <cstddef>

struct HudState {
    double pitch_deg       = 0.0;
    double bank_deg        = 0.0;
    double sim_heading_deg = 285.0;
    double sim_altitude_ft = 12500.0;
    bool   valid           = false;
};

class TelemetryParser {
public:
    static constexpr std::size_t FRAME_SIZE = 9;
    static constexpr uint8_t SYNC_BYTE_1    = 0xAA;
    static constexpr uint8_t SYNC_BYTE_2    = 0x55;

    static uint8_t computeXorChecksum(const uint8_t* payload6);
    static bool parseFrame(const uint8_t* frame, std::size_t len, HudState& out_state);
    static void computeAttitudeFromRaw(int16_t raw_ax, int16_t raw_ay, int16_t raw_az,
                                       double& pitch_deg, double& bank_deg);
};

#endif // TELEMETRY_PARSER_HPP
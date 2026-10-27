#ifndef ATTITUDE_SMOOTHER_HPP
#define ATTITUDE_SMOOTHER_HPP

#include "telemetry_parser.hpp"

class AttitudeSmoother {
public:
    void apply(HudState& state);

private:
    static constexpr double kAlpha = 0.18;
    bool initialized_ = false;
    double pitch_deg_ = 0.0;
    double bank_deg_ = 0.0;
};

#endif // ATTITUDE_SMOOTHER_HPP

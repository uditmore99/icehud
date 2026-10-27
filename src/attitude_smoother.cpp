#include "attitude_smoother.hpp"

#include <cmath>

void AttitudeSmoother::apply(HudState& state) {
    if (!state.valid) return;

    if (!initialized_) {
        pitch_deg_ = state.pitch_deg;
        bank_deg_ = state.bank_deg;
        initialized_ = true;
    } else {
        pitch_deg_ += kAlpha * (state.pitch_deg - pitch_deg_);

        // Follow the shortest path when roll crosses the -180/180 degree boundary.
        const double bank_delta = std::remainder(state.bank_deg - bank_deg_, 360.0);
        bank_deg_ = std::remainder(bank_deg_ + kAlpha * bank_delta, 360.0);
    }

    state.pitch_deg = pitch_deg_;
    state.bank_deg = bank_deg_;
}

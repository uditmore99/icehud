#include "attitude_smoother.hpp"

#include <gtest/gtest.h>

TEST(AttitudeSmootherTest, FirstValidSamplePassesThroughUnchanged) {
    AttitudeSmoother smoother;
    HudState state;
    state.pitch_deg = 12.5;
    state.bank_deg = -34.0;
    state.valid = true;

    smoother.apply(state);

    EXPECT_DOUBLE_EQ(state.pitch_deg, 12.5);
    EXPECT_DOUBLE_EQ(state.bank_deg, -34.0);
}

TEST(AttitudeSmootherTest, AppliesExponentialLowPassToPitchAndBank) {
    AttitudeSmoother smoother;
    HudState first;
    first.valid = true;
    smoother.apply(first);

    HudState next;
    next.pitch_deg = 10.0;
    next.bank_deg = 20.0;
    next.valid = true;
    smoother.apply(next);

    EXPECT_NEAR(next.pitch_deg, 1.8, 1e-12);
    EXPECT_NEAR(next.bank_deg, 3.6, 1e-12);
}

TEST(AttitudeSmootherTest, FollowsShortestPathAcrossRollWrap) {
    AttitudeSmoother smoother;
    HudState first;
    first.bank_deg = 179.0;
    first.valid = true;
    smoother.apply(first);

    HudState next;
    next.bank_deg = -179.0;
    next.valid = true;
    smoother.apply(next);

    EXPECT_NEAR(next.bank_deg, 179.36, 1e-12);
}

TEST(AttitudeSmootherTest, InvalidSampleDoesNotAdvanceFilterState) {
    AttitudeSmoother smoother;
    HudState first;
    first.pitch_deg = 10.0;
    first.bank_deg = 20.0;
    first.valid = true;
    smoother.apply(first);

    HudState invalid;
    invalid.pitch_deg = 100.0;
    invalid.bank_deg = -100.0;
    invalid.valid = false;
    smoother.apply(invalid);

    HudState next;
    next.pitch_deg = 20.0;
    next.bank_deg = 30.0;
    next.valid = true;
    smoother.apply(next);

    EXPECT_NEAR(next.pitch_deg, 11.8, 1e-12);
    EXPECT_NEAR(next.bank_deg, 21.8, 1e-12);
}

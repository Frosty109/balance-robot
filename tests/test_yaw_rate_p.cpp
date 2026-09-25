#include <gtest/gtest.h>
#include "../src/pid/pid_control.hpp"

TEST(YawRatePTest, ZeroTargetAndMeasurementReturnsZero)
{
    YawRateP ctrl(1.0f, 30.0f, 100);
    EXPECT_EQ(ctrl.compute(0.0f, 0.0f), 0);
}

TEST(YawRatePTest, PositiveTargetProducesPositiveEffort)
{
    YawRateP ctrl(2.0f, 30.0f, 100);
    EXPECT_EQ(ctrl.compute(10.0f, 0.0f), 20);
}

TEST(YawRatePTest, PositiveMeasuredRateAtZeroTargetProducesNegativeEffort)
{
    YawRateP ctrl(2.0f, 30.0f, 100);
    EXPECT_EQ(ctrl.compute(0.0f, 10.0f), -20);
}

TEST(YawRatePTest, NegativeInputsAreSymmetric)
{
    YawRateP ctrl(2.0f, 30.0f, 100);
    EXPECT_EQ(ctrl.compute(-10.0f, 0.0f), -20);
    EXPECT_EQ(ctrl.compute(0.0f, -10.0f), 20);
}

TEST(YawRatePTest, TargetIsClampedSymmetrically)
{
    YawRateP ctrl(2.0f, 30.0f, 1000);   // effort cap out of reach: isolates the target clamp
    EXPECT_EQ(ctrl.compute(30.0f, 0.0f), 60);
    EXPECT_EQ(ctrl.compute(50.0f, 0.0f), 60);
    EXPECT_EQ(ctrl.compute(-50.0f, 0.0f), -60);
}

TEST(YawRatePTest, OutputIsClampedSymmetrically)
{
    YawRateP ctrl(10.0f, 30.0f, 100);
    EXPECT_EQ(ctrl.compute(10.0f, 0.0f), 100);
    EXPECT_EQ(ctrl.compute(30.0f, 0.0f), 100);
    EXPECT_EQ(ctrl.compute(-30.0f, 0.0f), -100);
    EXPECT_EQ(ctrl.compute(0.0f, 500.0f), -100);   // measured rate is never clamped
}

TEST(YawRatePTest, FractionalResultTruncatesTowardZero)
{
    YawRateP ctrl(0.5f, 30.0f, 100);
    EXPECT_EQ(ctrl.compute(1.9f, 0.0f), 0);    // |error| < 1/kp: inside the deadband
    EXPECT_EQ(ctrl.compute(-1.9f, 0.0f), 0);   // toward zero, not floor (floor gives -1)
    EXPECT_EQ(ctrl.compute(2.0f, 0.0f), 1);
}

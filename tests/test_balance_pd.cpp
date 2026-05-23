#include <gtest/gtest.h>
#include "../src/pid/pid_control.hpp"

TEST(BalancePDTest, ZeroInputReturnsZero)
{
    BalancePD pid(100.0f, 100.0f, 0.0f);
    EXPECT_EQ(pid.compute(0.0f, 0.0f), 0);
}

TEST(BalancePDTest, PositiveAngleProducesPositiveOutput)
{
    BalancePD pid(100.0f, 0.0f, 0.0f);
    EXPECT_EQ(pid.compute(10.0f, 0.0f), 10);
}

TEST(BalancePDTest, PositiveGyroProducesPositiveOutput)
{
    BalancePD pid(0.0f, 100.0f, 0.0f);
    EXPECT_EQ(pid.compute(0.0f, 5.0f), 5);
}

TEST(BalancePDTest, MidAngleShiftsEquilibrium)
{
    BalancePD pid(100.0f, 0.0f, 5.0f);
    EXPECT_EQ(pid.compute(5.0f, 0.0f), 0);
    EXPECT_EQ(pid.compute(10.0f, 0.0f), 5);
}

TEST(BalancePDTest, NegativeInputsAreSymmetric)
{
    BalancePD pid(100.0f, 100.0f, 0.0f);
    EXPECT_EQ(pid.compute(-10.0f, 0.0f), -10);
    EXPECT_EQ(pid.compute(0.0f, -5.0f), -5);
}

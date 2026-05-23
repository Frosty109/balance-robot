#include <gtest/gtest.h>
#include "../src/pid/pid_control.hpp"

TEST(TurnPDTest, ZeroInputReturnsZero)
{
    TurnPD pid(100.0f, 100.0f);
    EXPECT_EQ(pid.compute(0), 0);
}

TEST(TurnPDTest, GyroProducesScaledOutput)
{
    TurnPD pid(100.0f, 100.0f);
    EXPECT_EQ(pid.compute(5.0f, 0.0f), 5);
}

TEST(TurnPDTest, MoveZShiftsOutput)
{
    TurnPD pid(100.0f, 100.0f);
    EXPECT_EQ(pid.compute(0.0f, 10.0f), 10);
}
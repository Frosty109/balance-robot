#include <gtest/gtest.h>
#include "../src/pid/pid_control.hpp"

TEST(VelocityPITest, ZeroInputReturnsZero)
{
    VelocityPI pid(100.0f, 100.0f, 1000.0f);
    EXPECT_EQ(pid.compute(0, 0, 0.0f), 0);
}

TEST(VelocityPITest, FirstCallProducesExpectedOutput)
{
    VelocityPI pid(100.0f, 100.0f, 1000.0f);
    EXPECT_EQ(pid.compute(100, 100, 0.0f), 64);
}

TEST(VelocityPITest, IntegralAccumulatesOverMultipleCalls)
{
    VelocityPI pid(0.0f, 100.0f, 100000.0f);
    int first  = pid.compute(100, 100, 0.0f);
    int second = pid.compute(100, 100, 0.0f);
    int third  = pid.compute(100, 100, 0.0f);
    EXPECT_GT(second, first);
    EXPECT_GT(third, second);
}
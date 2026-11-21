// Encoded frames must match the CRC-8/SMBUS standard and drive.py, which drives the car today.
#include "balance_car_core/protocol.hpp"

#include <gtest/gtest.h>

using namespace balance_car::core;

TEST(ProtocolTest, Crc8MatchesTheStandardCheckValue)
{
    EXPECT_EQ(crc8("123456789"), 0xF4);
}

TEST(ProtocolTest, MatchesDrivePyFrame)
{
    EXPECT_EQ(encode_motion(17, 120, -150), "$M,17,120,-150*4D\n");
}

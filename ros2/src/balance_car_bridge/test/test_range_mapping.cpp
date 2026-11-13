#include "balance_car_bridge/range_mapping.hpp"

#include <cstdint>
#include <utility>

#include <gtest/gtest.h>

using balance_car::bridge::to_msg;
using balance_car::core::RangeReading;
using balance_car::core::RangeStatus;
using Msg = balance_car_interfaces::msg::RangeReading;

TEST(RangeMapping, CopiesFieldsStampAndFrame)
{
    const RangeReading reading {.sequence = 1539, .completed_ms = 123456, .distance_mm = 500,
                                .status = RangeStatus::valid, .reported_age_ms = 4};
    builtin_interfaces::msg::Time stamp;
    stamp.sec = 10;
    stamp.nanosec = 20;

    const Msg msg = to_msg(reading, stamp, "ultrasonic_link");

    EXPECT_EQ(msg.header.stamp, stamp);
    EXPECT_EQ(msg.header.frame_id, "ultrasonic_link");
    EXPECT_EQ(msg.sequence, 1539u);
    EXPECT_EQ(msg.completed_ms, 123456u);
    EXPECT_EQ(msg.distance_mm, 500u);
    EXPECT_EQ(msg.reported_age_ms, 4u);
}

// Failed readings must keep their own status; none may become STATUS_VALID.
TEST(RangeMapping, MapsEveryStatusExplicitly)
{
    const std::pair<RangeStatus, std::uint8_t> cases[] {
        {RangeStatus::starting, Msg::STATUS_STARTING},
        {RangeStatus::valid, Msg::STATUS_VALID},
        {RangeStatus::no_echo, Msg::STATUS_NO_ECHO},
        {RangeStatus::timeout, Msg::STATUS_TIMEOUT},
        {RangeStatus::out_of_range, Msg::STATUS_OUT_OF_RANGE},
        {RangeStatus::capture_error, Msg::STATUS_CAPTURE_ERROR},
    };

    for (const auto& [status, expected] : cases)
    {
        RangeReading reading;
        reading.status = status;
        EXPECT_EQ(to_msg(reading, builtin_interfaces::msg::Time {}, "").status, expected)
            << "core status " << static_cast<int>(status);
    }
}

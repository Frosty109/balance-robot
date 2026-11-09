// Converts parsed RANGE readings into the ROS RangeReading message.
#pragma once

#include <string>

#include <balance_car_core/telemetry.hpp>
#include <balance_car_interfaces/msg/range_reading.hpp>
#include <builtin_interfaces/msg/time.hpp>

namespace balance_car::bridge
{

// receipt_stamp is when the host read the line, not when the sensor measured.
[[nodiscard]] balance_car_interfaces::msg::RangeReading to_msg(
    const core::RangeReading& reading,
    const builtin_interfaces::msg::Time& receipt_stamp,
    const std::string& frame_id);

}  // namespace balance_car::bridge

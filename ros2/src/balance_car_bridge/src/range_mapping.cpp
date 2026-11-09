#include "balance_car_bridge/range_mapping.hpp"

#include <cstdint>
#include <stdexcept>

namespace balance_car::bridge
{
namespace
{

using Msg = balance_car_interfaces::msg::RangeReading;

std::uint8_t to_msg_status(core::RangeStatus status)
{
    switch (status)
    {
    case core::RangeStatus::starting: return Msg::STATUS_STARTING;
    case core::RangeStatus::valid: return Msg::STATUS_VALID;
    case core::RangeStatus::no_echo: return Msg::STATUS_NO_ECHO;
    case core::RangeStatus::timeout: return Msg::STATUS_TIMEOUT;
    case core::RangeStatus::out_of_range: return Msg::STATUS_OUT_OF_RANGE;
    case core::RangeStatus::capture_error: return Msg::STATUS_CAPTURE_ERROR;
    }
    // Only reachable with a corrupted enum value; refuse rather than publish a guess.
    throw std::invalid_argument("unmapped RangeStatus");
}

}  // namespace

Msg to_msg(const core::RangeReading& reading,
           const builtin_interfaces::msg::Time& receipt_stamp,
           const std::string& frame_id)
{
    Msg msg;
    msg.header.stamp = receipt_stamp;
    msg.header.frame_id = frame_id;
    msg.status = to_msg_status(reading.status);
    msg.sequence = reading.sequence;
    msg.completed_ms = reading.completed_ms;
    msg.distance_mm = reading.distance_mm;
    msg.reported_age_ms = reading.reported_age_ms;
    return msg;
}

}  // namespace balance_car::bridge

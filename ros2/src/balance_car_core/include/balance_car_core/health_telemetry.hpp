#pragma once

#include "balance_car_core/telemetry.hpp"

#include <cstdint>
#include <expected>
#include <string_view>
#include <variant>

namespace balance_car::core
{

struct DisarmedHealth
{
    std::int32_t battery_cV {0};
    std::uint32_t max_fresh_interval_ms {0};
};

// Identifies the wire layout only; an explicit ARMED event confirms arming.
struct ArmedHealth
{
    std::int32_t balance_effort {0};
    std::int32_t velocity_integral {0};
    std::int32_t turn_effort {0};
    std::int32_t target_velocity_mm_s {0};
    std::int32_t target_yaw_rate_ddeg_s {0};
    std::int32_t left_effort {0};
    std::int32_t right_effort {0};
    std::int32_t left_encoder_counts {0};
    std::int32_t right_encoder_counts {0};
};

struct HealthTelemetry
{
    std::int32_t angle_cdeg {0};
    std::int32_t yaw_rate_ddeg_s {0};
    std::uint32_t uptime_ms {0};
    std::uint32_t max_poll_ms {0};
    std::variant<DisarmedHealth, ArmedHealth> details {};
};

// Takes one line without CR/LF. The result owns its values; no input is borrowed.
[[nodiscard]] std::expected<HealthTelemetry, ParseError> parse_health(std::string_view line);

}  // namespace balance_car::core
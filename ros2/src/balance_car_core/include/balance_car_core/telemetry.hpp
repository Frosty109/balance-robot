// Strict parsers for the STM32 serial telemetry lines (plan §4.1).
#pragma once

#include <cstdint>
#include <expected>
#include <string_view>

namespace balance_car::core
{

// Mirrors the firmware's st= names; the wire text is the contract, not a shared header.
enum class RangeStatus : std::uint8_t
{
    starting,
    valid,
    no_echo,
    timeout,
    out_of_range,
    capture_error,
};

struct RangeReading
{
    std::uint32_t sequence {0};
    std::uint32_t completed_ms {0};
    std::uint32_t distance_mm {0};
    RangeStatus status {RangeStatus::starting};
    std::uint32_t reported_age_ms {0};
};

enum class ParseError : std::uint8_t
{
    wrong_line_type,
    unsupported_version,
    malformed_field,
    duplicate_field,
    missing_field,
    bad_number,
    unknown_status,
};

// Parses one RANGE line without its terminator; unknown extra keys are ignored.
[[nodiscard]] std::expected<RangeReading, ParseError> parse_range(std::string_view line);

}  // namespace balance_car::core

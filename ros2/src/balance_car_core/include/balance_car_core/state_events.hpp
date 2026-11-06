#pragma once

#include "balance_car_core/telemetry.hpp"

#include <cstdint>
#include <expected>
#include <optional>
#include <string_view>

namespace balance_car::core
{

enum class StateEventKind
{
    boot,
    armed,
    disarmed,
    arm_rejected,
    angle_fault,
    recovered,
    stale,
};

enum class StateReason
{
    boot,
    operator_request,
    angle,
    stale,
    recovering,
    unknown,
};

struct StateEvent
{
    StateEventKind kind {StateEventKind::boot};
    std::optional<std::int32_t> angle_cdeg {};
    std::optional<StateReason> reason {};
    std::optional<std::uint32_t> last_fresh_ms {};
    std::optional<std::uint32_t> now_ms {};
};

// Takes one line without CR/LF. The result owns its values; no input is borrowed.
[[nodiscard]] std::expected<StateEvent, ParseError> parse_state_event(std::string_view line);

}  // namespace balance_car::core
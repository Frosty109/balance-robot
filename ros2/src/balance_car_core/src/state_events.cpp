#include "balance_car_core/state_events.hpp"

#include <array>
#include <charconv>
#include <ranges>

namespace balance_car::core
{
namespace
{

template <typename T>
std::expected<T, ParseError> parse_number(std::string_view text)
{
    if (text.empty())
    {
        return std::unexpected(ParseError::bad_number);
    }

    T value {};
    const char* const end = text.data() + text.size();
    const auto [last, ec] = std::from_chars(text.data(), end, value);
    if (ec != std::errc {} || last != end)
    {
        return std::unexpected(ParseError::bad_number);
    }
    return value;
}

std::expected<StateReason, ParseError> parse_reason(std::string_view text)
{
    if (text == "boot")       return StateReason::boot;
    if (text == "operator")   return StateReason::operator_request;
    if (text == "angle")      return StateReason::angle;
    if (text == "stale")      return StateReason::stale;
    if (text == "recovering") return StateReason::recovering;
    return StateReason::unknown;
}

template <typename T>
std::expected<void, ParseError> store_once(
    std::optional<T>& slot, const std::expected<T, ParseError>& parsed)
{
    if (slot)
    {
        return std::unexpected(ParseError::duplicate_field);
    }
    if (!parsed)
    {
        return std::unexpected(parsed.error());
    }
    slot = *parsed;
    return {};
}

struct Prefix
{
    std::string_view text;
    StateEventKind kind;
};

constexpr std::array prefixes {
    Prefix {"ARMED ", StateEventKind::armed},
    Prefix {"DISARMED ", StateEventKind::disarmed},
    Prefix {"ARM REJECTED ", StateEventKind::arm_rejected},
    Prefix {"FAULT ", StateEventKind::angle_fault},
    Prefix {"RECOVERED ", StateEventKind::recovered},
    Prefix {"STALE ", StateEventKind::stale},
};

}  // namespace

std::expected<StateEvent, ParseError> parse_state_event(std::string_view line)
{
    StateEvent event;
    if (line == "BOOT")
    {
        return event;
    }

    bool matched {false};
    for (const auto& prefix : prefixes)
    {
        if (line.starts_with(prefix.text))
        {
            event.kind = prefix.kind;
            line.remove_prefix(prefix.text.size());
            matched = true;
            break;
        }
    }
    if (!matched)
    {
        return std::unexpected(ParseError::wrong_line_type);
    }

    for (const auto part : std::views::split(line, ' '))
    {
        const std::string_view token {part.begin(), part.end()};
        const auto eq = token.find('=');
        if (eq == std::string_view::npos || eq == 0 || eq + 1 == token.size()
            || token.find('=', eq + 1) != std::string_view::npos)
        {
            return std::unexpected(ParseError::malformed_field);
        }

        // Check unknown fields too: a trailing CR or control byte is never repaired here.
        for (const char ch : token)
        {
            const auto byte = static_cast<unsigned char>(ch);
            if (byte < 0x21u || byte > 0x7eu)
            {
                return std::unexpected(ParseError::malformed_field);
            }
        }

        const auto key = token.substr(0, eq);
        const auto value = token.substr(eq + 1);
        std::expected<void, ParseError> stored {};

        if (key == "angle")
            stored = store_once(event.angle_cdeg, parse_number<std::int32_t>(value));
        else if (key == "reason")
            stored = store_once(event.reason, parse_reason(value));
        else if (key == "last")
            stored = store_once(event.last_fresh_ms, parse_number<std::uint32_t>(value));
        else if (key == "now")
            stored = store_once(event.now_ms, parse_number<std::uint32_t>(value));

        if (!stored)
        {
            return std::unexpected(stored.error());
        }
    }

    switch (event.kind)
    {
        case StateEventKind::armed:
        case StateEventKind::angle_fault:
        case StateEventKind::recovered:
            if (!event.angle_cdeg)
                return std::unexpected(ParseError::missing_field);
            break;

        case StateEventKind::disarmed:
        case StateEventKind::arm_rejected:
            if (!event.reason)
                return std::unexpected(ParseError::missing_field);
            break;

        case StateEventKind::stale:
            if (!event.last_fresh_ms || !event.now_ms)
                return std::unexpected(ParseError::missing_field);
            break;

        case StateEventKind::boot:
            break;
    }

    return event;
}

}  // namespace balance_car::core
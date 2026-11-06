// Strict parsers for the STM32 serial telemetry lines.
#include "balance_car_core/telemetry.hpp"

#include <charconv>
#include <optional>
#include <ranges>

namespace balance_car::core
{
namespace
{

std::expected<std::uint32_t, ParseError> parse_u32(std::string_view text)
{
    std::uint32_t value {0};
    const char* const end = text.data() + text.size();
    const auto [last, ec] = std::from_chars(text.data(), end, value);

    // Partial consumption ("12a") is as wrong as no digits at all.
    if (ec != std::errc {} || last != end)
    {
        return std::unexpected(ParseError::bad_number);
    }
    return value;
}

std::expected<RangeStatus, ParseError> parse_status(std::string_view text)
{
    if (text == "starting")      return RangeStatus::starting;
    if (text == "valid")         return RangeStatus::valid;
    if (text == "no_echo")       return RangeStatus::no_echo;
    if (text == "timeout")       return RangeStatus::timeout;
    if (text == "out_of_range")  return RangeStatus::out_of_range;
    if (text == "capture_error") return RangeStatus::capture_error;
    return std::unexpected(ParseError::unknown_status);
}

// A repeated key means a corrupted line, so it fails rather than last-wins.
template <typename T>
std::expected<void, ParseError> store_once(std::optional<T>& slot,
                                           const std::expected<T, ParseError>& parsed)
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

}  // namespace

std::expected<RangeReading, ParseError> parse_range(std::string_view line)
{
    constexpr std::string_view prefix {"RANGE "};
    if (!line.starts_with(prefix))
    {
        return std::unexpected(ParseError::wrong_line_type);
    }
    line.remove_prefix(prefix.size());

    std::optional<std::uint32_t> version;
    std::optional<std::uint32_t> sequence;
    std::optional<std::uint32_t> completed_ms;
    std::optional<std::uint32_t> distance_mm;
    std::optional<std::uint32_t> age_ms;
    std::optional<RangeStatus> status;

    for (const auto part : std::views::split(line, ' '))
    {
        const std::string_view token {part.begin(), part.end()};

        // Catches double or trailing spaces (empty token) as well as a token with no key.
        const auto eq = token.find('=');
        if (eq == std::string_view::npos || eq == 0)
        {
            return std::unexpected(ParseError::malformed_field);
        }
        const std::string_view key = token.substr(0, eq);
        const std::string_view value = token.substr(eq + 1);

        std::expected<void, ParseError> stored {};
        if (key == "v")        stored = store_once(version, parse_u32(value));
        else if (key == "seq") stored = store_once(sequence, parse_u32(value));
        else if (key == "t")   stored = store_once(completed_ms, parse_u32(value));
        else if (key == "mm")  stored = store_once(distance_mm, parse_u32(value));
        else if (key == "age") stored = store_once(age_ms, parse_u32(value));
        else if (key == "st")  stored = store_once(status, parse_status(value));

        if (!stored)
        {
            return std::unexpected(stored.error());
        }
    }

    // Version first, so a future v=2 layout reports as unsupported rather than incomplete.
    if (!version)
    {
        return std::unexpected(ParseError::missing_field);
    }
    if (*version != 1)
    {
        return std::unexpected(ParseError::unsupported_version);
    }
    if (!sequence || !completed_ms || !distance_mm || !status || !age_ms)
    {
        return std::unexpected(ParseError::missing_field);
    }

    return RangeReading {
        .sequence = *sequence,
        .completed_ms = *completed_ms,
        .distance_mm = *distance_mm,
        .status = *status,
        .reported_age_ms = *age_ms,
    };
}

}  // namespace balance_car::core

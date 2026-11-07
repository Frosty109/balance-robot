#include "balance_car_core/health_telemetry.hpp"

#include <charconv>
#include <optional>
#include <ranges>

namespace balance_car::core
{
namespace
{

template <typename T>
std::expected<void, ParseError> read_into(std::optional<T>& slot, std::string_view text)
{
    if (slot)
        return std::unexpected(ParseError::duplicate_field);

    T value {};
    const char* const end = text.data() + text.size();
    const auto [last, ec] = std::from_chars(text.data(), end, value);
    if (ec != std::errc {} || last != end)
        return std::unexpected(ParseError::bad_number);
    slot = value;
    return {};
}

}  // namespace

std::expected<HealthTelemetry, ParseError> parse_health(std::string_view line)
{
    // Named messages such as "FAULT angle=..." belong to other parsers.
    const auto first_equals = line.find('=');
    if (first_equals == std::string_view::npos || first_equals > line.find(' '))
        return std::unexpected(ParseError::wrong_line_type);

    bool known_field {false};
    bool disarmed {false};
    std::optional<std::int32_t> angle, gz, battery, bal, integral, tr, vx, wz;
    std::optional<std::int32_t> left, right, enc_l, enc_r;
    std::optional<std::uint32_t> uptime, poll, dtmax;

    for (const auto part : std::views::split(line, ' '))
    {
        const std::string_view token {part.begin(), part.end()};
        const auto eq = token.find('=');
        if (eq == std::string_view::npos || eq == 0 || eq + 1 == token.size()
            || token.find('=', eq + 1) != std::string_view::npos)
        {
            return std::unexpected(ParseError::malformed_field);
        }

        for (const char ch : token)
        {
            const auto byte = static_cast<unsigned char>(ch);
            if (byte < 0x21u || byte > 0x7eu)
                return std::unexpected(ParseError::malformed_field);
        }

        const auto key = token.substr(0, eq);
        const auto value = token.substr(eq + 1);
        std::expected<void, ParseError> stored {};

        if (key == "state")
        {
            if (disarmed)
                return std::unexpected(ParseError::duplicate_field);
            if (value != "DISARMED")
                return std::unexpected(ParseError::unknown_status);
            disarmed = true;
        }
        else if (key == "angle")   stored = read_into(angle, value);
        else if (key == "gz")      stored = read_into(gz, value);
        else if (key == "battery") stored = read_into(battery, value);
        else if (key == "bal")     stored = read_into(bal, value);
        else if (key == "i")       stored = read_into(integral, value);
        else if (key == "tr")      stored = read_into(tr, value);
        else if (key == "vx")      stored = read_into(vx, value);
        else if (key == "wz")      stored = read_into(wz, value);
        else if (key == "L")       stored = read_into(left, value);
        else if (key == "R")       stored = read_into(right, value);
        else if (key == "enc_l")   stored = read_into(enc_l, value);
        else if (key == "enc_r")   stored = read_into(enc_r, value);
        else if (key == "t")       stored = read_into(uptime, value);
        else if (key == "poll")    stored = read_into(poll, value);
        else if (key == "dtmax")   stored = read_into(dtmax, value);
        else continue;

        known_field = true;
        if (!stored)
            return std::unexpected(stored.error());
    }

    if (!known_field)
        return std::unexpected(ParseError::wrong_line_type);
    if (!angle || !gz || !uptime || !poll)
        return std::unexpected(ParseError::missing_field);

    HealthTelemetry result {
        .angle_cdeg = *angle,
        .yaw_rate_ddeg_s = *gz,
        .uptime_ms = *uptime,
        .max_poll_ms = *poll,
    };

    if (disarmed)
    {
        if (bal || integral || tr || vx || wz || left || right || enc_l || enc_r)
            return std::unexpected(ParseError::malformed_field);
        if (!battery || !dtmax)
            return std::unexpected(ParseError::missing_field);

        result.details = DisarmedHealth {
            .battery_cV = *battery,
            .max_fresh_interval_ms = *dtmax,
        };
    }
    else
    {
        if (battery || dtmax)
            return std::unexpected(ParseError::malformed_field);
        if (!bal || !integral || !tr || !vx || !wz || !left || !right || !enc_l || !enc_r)
            return std::unexpected(ParseError::missing_field);

        result.details = ArmedHealth {
            .balance_effort = *bal,
            .velocity_integral = *integral,
            .turn_effort = *tr,
            .target_velocity_mm_s = *vx,
            .target_yaw_rate_ddeg_s = *wz,
            .left_effort = *left,
            .right_effort = *right,
            .left_encoder_counts = *enc_l,
            .right_encoder_counts = *enc_r,
        };
    }
    return result;
}

}  // namespace balance_car::core
#include "balance_car_core/health_telemetry.hpp"

#include <charconv>
#include <optional>
#include <ranges>

namespace balance_car::core
{
namespace
{

// Visible ASCII; spaces, control bytes, DEL and non-ASCII bytes make a token malformed.
constexpr unsigned char first_visible_byte {0x21};  // '!'
constexpr unsigned char last_visible_byte {0x7e};   // '~'

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
    const auto first_space = line.find(' ');
    const bool has_no_field = first_equals == std::string_view::npos;
    const bool starts_with_name = first_equals > first_space;
    if (has_no_field || starts_with_name)
        return std::unexpected(ParseError::wrong_line_type);

    bool known_field {false};
    bool disarmed {false};

    // Both layouts.
    std::optional<std::int32_t> angle;
    std::optional<std::int32_t> gz;
    std::optional<std::uint32_t> uptime;
    std::optional<std::uint32_t> poll;

    // Disarmed layout only.
    std::optional<std::int32_t> battery;
    std::optional<std::uint32_t> dtmax;

    // Armed layout only.
    std::optional<std::int32_t> bal;
    std::optional<std::int32_t> integral;
    std::optional<std::int32_t> tr;
    std::optional<std::int32_t> vx;
    std::optional<std::int32_t> wz;
    std::optional<std::int32_t> left;
    std::optional<std::int32_t> right;
    std::optional<std::int32_t> enc_l;
    std::optional<std::int32_t> enc_r;

    for (const auto part : std::views::split(line, ' '))
    {
        const std::string_view token {part.begin(), part.end()};
        const auto eq = token.find('=');
        const bool no_equals = eq == std::string_view::npos;
        const bool empty_key = eq == 0;
        const bool empty_value = eq + 1 == token.size();
        const bool second_equals = token.find('=', eq + 1) != std::string_view::npos;
        if (no_equals || empty_key || empty_value || second_equals)
            return std::unexpected(ParseError::malformed_field);

        for (const char ch : token)
        {
            const auto byte = static_cast<unsigned char>(ch);
            if (byte < first_visible_byte || byte > last_visible_byte)
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
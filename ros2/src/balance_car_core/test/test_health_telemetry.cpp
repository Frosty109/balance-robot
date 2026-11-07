#include "balance_car_core/health_telemetry.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

using namespace balance_car::core;

namespace
{

constexpr std::string_view disarmed_line {
    "state=DISARMED angle=-123 gz=25 battery=750 t=123456 poll=4 dtmax=11"};
constexpr std::string_view armed_line {
    "angle=-234 bal=120 i=-1000 tr=30 vx=-50 wz=-300 gz=125 "
    "L=-1500 R=1600 enc_l=-12 enc_r=13 t=123556 poll=4"};

void expect_error(std::string_view line, ParseError error)
{
    SCOPED_TRACE(std::string {line});
    const auto row = parse_health(line);
    ASSERT_FALSE(row.has_value());
    EXPECT_EQ(row.error(), error);
}

// Test inputs contain the requested key exactly once; nullopt removes the field.
std::string change_field(std::string_view line, std::string_view key,
                         std::optional<std::string_view> value)
{
    std::string result {line};
    const auto token = std::string {key} + '=';
    const auto begin = result.starts_with(token) ? 0u : result.find(' ' + token) + 1;
    const auto end = result.find(' ', begin);
    const auto length = (end == std::string::npos ? result.size() : end) - begin;
    if (value)
        result.replace(begin, length, token + std::string {*value});
    else if (end != std::string::npos)
        result.erase(begin, length + 1);
    else
        result.erase(begin == 0 ? 0 : begin - 1);
    return result;
}

}  // namespace

TEST(HealthTelemetryTest, ParsesDisarmedRow)
{
    const auto row = parse_health(disarmed_line);
    ASSERT_TRUE(row.has_value());
    EXPECT_EQ(row->angle_cdeg, -123);
    EXPECT_EQ(row->yaw_rate_ddeg_s, 25);
    EXPECT_EQ(row->uptime_ms, 123456u);
    EXPECT_EQ(row->max_poll_ms, 4u);
    const auto* details = std::get_if<DisarmedHealth>(&row->details);
    ASSERT_NE(details, nullptr);
    EXPECT_EQ(details->battery_cV, 750);
    EXPECT_EQ(details->max_fresh_interval_ms, 11u);
}

TEST(HealthTelemetryTest, ParsesArmedRow)
{
    const auto row = parse_health(armed_line);
    ASSERT_TRUE(row.has_value());
    EXPECT_EQ(row->angle_cdeg, -234);
    EXPECT_EQ(row->yaw_rate_ddeg_s, 125);
    EXPECT_EQ(row->uptime_ms, 123556u);
    EXPECT_EQ(row->max_poll_ms, 4u);
    const auto* details = std::get_if<ArmedHealth>(&row->details);
    ASSERT_NE(details, nullptr);
    EXPECT_EQ(details->balance_effort, 120);
    EXPECT_EQ(details->velocity_integral, -1000);
    EXPECT_EQ(details->turn_effort, 30);
    EXPECT_EQ(details->target_velocity_mm_s, -50);
    EXPECT_EQ(details->target_yaw_rate_ddeg_s, -300);
    EXPECT_EQ(details->left_effort, -1500);
    EXPECT_EQ(details->right_effort, 1600);
    EXPECT_EQ(details->left_encoder_counts, -12);
    EXPECT_EQ(details->right_encoder_counts, 13);
}

TEST(HealthTelemetryTest, AcceptsFieldOrderExtraKeysAndZeroValues)
{
    for (const char* line : {
        "extra=x poll=4 t=123456 battery=750 gz=25 angle=-123 dtmax=11 state=DISARMED extra=y",
        "extra=x poll=4 t=123556 enc_r=13 enc_l=-12 R=1600 L=-1500 "
        "gz=125 wz=-300 vx=-50 tr=30 i=-1000 bal=120 angle=-234",
        "state=DISARMED angle=0 gz=0 battery=0 t=0 poll=0 dtmax=0",
        "angle=0 bal=0 i=0 tr=0 vx=0 wz=0 gz=0 L=0 R=0 enc_l=0 enc_r=0 t=0 poll=0",
    })
        EXPECT_TRUE(parse_health(line).has_value()) << line;
}

TEST(HealthTelemetryTest, RequiresEachFieldExactlyOnce)
{
    for (const auto line : {disarmed_line, armed_line})
    {
        for (const auto part : std::views::split(line, ' '))
        {
            const std::string_view token {part.begin(), part.end()};
            const auto key = token.substr(0, token.find('='));
            expect_error(std::string {line} + ' ' + std::string {token},
                         ParseError::duplicate_field);
            expect_error(change_field(line, key, std::nullopt),
                         key == "state" ? ParseError::malformed_field : ParseError::missing_field);
        }
    }
}

TEST(HealthTelemetryTest, RejectsOtherMessagesAndInvalidLayouts)
{
    const std::pair<std::string, ParseError> cases[] {
        {"", ParseError::wrong_line_type},
        {"BOOT", ParseError::wrong_line_type},
        {"ARMED angle=0", ParseError::wrong_line_type},
        {"DISARMED reason=operator", ParseError::wrong_line_type},
        {"FAULT angle=4015", ParseError::wrong_line_type},
        {"RANGE v=1 seq=1 t=100 mm=500 st=valid age=0", ParseError::wrong_line_type},
        {"extra=x future=y", ParseError::wrong_line_type},
        {change_field(disarmed_line, "state", "ARMED"), ParseError::unknown_status},
        {change_field(disarmed_line, "state", "FUTURE"), ParseError::unknown_status},
        {std::string {disarmed_line} + " bal=0", ParseError::malformed_field},
        {std::string {armed_line} + " battery=750", ParseError::malformed_field},
        {std::string {armed_line} + " dtmax=11", ParseError::malformed_field},
        {std::string {armed_line} + " state=DISARMED", ParseError::malformed_field},
    };
    for (const auto& [line, error] : cases)
        expect_error(line, error);
}

TEST(HealthTelemetryTest, ChecksNumericRepresentation)
{
    const auto row = parse_health(
        "state=DISARMED angle=-2147483648 gz=2147483647 battery=-2147483648 "
        "t=4294967295 poll=4294967295 dtmax=4294967295");
    ASSERT_TRUE(row.has_value());
    EXPECT_EQ(row->angle_cdeg, std::numeric_limits<std::int32_t>::min());
    EXPECT_EQ(row->yaw_rate_ddeg_s, std::numeric_limits<std::int32_t>::max());
    EXPECT_EQ(row->uptime_ms, std::numeric_limits<std::uint32_t>::max());
    EXPECT_EQ(row->max_poll_ms, std::numeric_limits<std::uint32_t>::max());
    const auto* details = std::get_if<DisarmedHealth>(&row->details);
    ASSERT_NE(details, nullptr);
    EXPECT_EQ(details->battery_cV, std::numeric_limits<std::int32_t>::min());
    EXPECT_EQ(details->max_fresh_interval_ms, std::numeric_limits<std::uint32_t>::max());

    for (const char* value : {"+1", "-", "1.0", "12a", "0x10", "2147483648", "-2147483649"})
        expect_error(change_field(armed_line, "angle", value), ParseError::bad_number);
    for (const char* value : {"-1", "+1", "1.0", "12a", "0x10", "4294967296"})
        expect_error(change_field(disarmed_line, "t", value), ParseError::bad_number);
}

TEST(HealthTelemetryTest, RejectsMalformedFieldsAndControlBytes)
{
    for (const char* suffix : {" ", "  extra=x", " junk", " =x", " extra=", " extra=x=y"})
        expect_error(std::string {disarmed_line} + suffix, ParseError::malformed_field);
    for (const char* key : {"state", "angle", "battery", "t"})
        expect_error(change_field(disarmed_line, key, ""), ParseError::malformed_field);
    for (const char byte : {'\r', '\n', '\t', '\0', static_cast<char>(0x7f), static_cast<char>(0xff)})
    {
        expect_error(std::string {disarmed_line} + byte, ParseError::malformed_field);
        expect_error(std::string {disarmed_line} + " extra=x" + byte, ParseError::malformed_field);
    }
    expect_error(" " + std::string {disarmed_line}, ParseError::wrong_line_type);
    expect_error("=x " + std::string {disarmed_line}, ParseError::malformed_field);
}

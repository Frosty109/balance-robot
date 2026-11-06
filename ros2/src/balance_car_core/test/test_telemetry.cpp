// Tests for the RANGE line parser against captured and malformed input.
#include "balance_car_core/telemetry.hpp"

#include <gtest/gtest.h>

#include <optional>

using namespace balance_car::core;

namespace
{

// Calling .error() on a success is undefined, so tests compare through an optional.
std::optional<ParseError> error_of(std::string_view line)
{
    const auto result = parse_range(line);
    if (result)
    {
        return std::nullopt;
    }
    return result.error();
}

}  // namespace

TEST(ParseRangeTest, ParsesCapturedValidLine)
{
    const auto reading = parse_range("RANGE v=1 seq=685 t=70334 mm=1540 st=valid age=0");
    ASSERT_TRUE(reading.has_value());
    EXPECT_EQ(reading->sequence, 685u);
    EXPECT_EQ(reading->completed_ms, 70334u);
    EXPECT_EQ(reading->distance_mm, 1540u);
    EXPECT_EQ(reading->status, RangeStatus::valid);
    EXPECT_EQ(reading->reported_age_ms, 0u);
}

TEST(ParseRangeTest, ParsesCapturedFailedAttempt)
{
    const auto reading = parse_range("RANGE v=1 seq=1 t=1793 mm=0 st=capture_error age=1");
    ASSERT_TRUE(reading.has_value());
    EXPECT_EQ(reading->status, RangeStatus::capture_error);
    EXPECT_EQ(reading->distance_mm, 0u);
}

TEST(ParseRangeTest, ParsesEveryFirmwareStatus)
{
    const std::pair<const char*, RangeStatus> cases[] = {
        {"starting", RangeStatus::starting},
        {"valid", RangeStatus::valid},
        {"no_echo", RangeStatus::no_echo},
        {"timeout", RangeStatus::timeout},
        {"out_of_range", RangeStatus::out_of_range},
        {"capture_error", RangeStatus::capture_error},
    };
    for (const auto& [name, expected] : cases)
    {
        const std::string line = std::string {"RANGE v=1 seq=1 t=1 mm=0 st="} + name + " age=0";
        const auto reading = parse_range(line);
        ASSERT_TRUE(reading.has_value()) << line;
        EXPECT_EQ(reading->status, expected) << line;
    }
}

TEST(ParseRangeTest, AcceptsAnyFieldOrderAndUnknownFields)
{
    const auto reading = parse_range("RANGE age=2 st=timeout mm=0 t=10 seq=3 v=1 extra=x");
    ASSERT_TRUE(reading.has_value());
    EXPECT_EQ(reading->sequence, 3u);
    EXPECT_EQ(reading->status, RangeStatus::timeout);
}

TEST(ParseRangeTest, RejectsOtherLineTypes)
{
    EXPECT_EQ(error_of("state=DISARMED angle=12 gz=0"), ParseError::wrong_line_type);
    EXPECT_EQ(error_of("RANGEv=1 seq=1"), ParseError::wrong_line_type);
    EXPECT_EQ(error_of("range v=1 seq=1 t=1 mm=1 st=valid age=0"), ParseError::wrong_line_type);
    EXPECT_EQ(error_of(""), ParseError::wrong_line_type);
}

TEST(ParseRangeTest, RejectsUnsupportedVersion)
{
    EXPECT_EQ(error_of("RANGE v=2 seq=1 t=1 mm=1 st=valid age=0"),
              ParseError::unsupported_version);
}

TEST(ParseRangeTest, RejectsEachMissingField)
{
    for (const char* line : {
             "RANGE seq=1 t=1 mm=1 st=valid age=0",
             "RANGE v=1 t=1 mm=1 st=valid age=0",
             "RANGE v=1 seq=1 mm=1 st=valid age=0",
             "RANGE v=1 seq=1 t=1 st=valid age=0",
             "RANGE v=1 seq=1 t=1 mm=1 age=0",
             "RANGE v=1 seq=1 t=1 mm=1 st=valid",
             "RANGE ",
         })
    {
        EXPECT_EQ(error_of(line), ParseError::missing_field) << line;
    }
}

TEST(ParseRangeTest, RejectsDuplicateField)
{
    EXPECT_EQ(error_of("RANGE v=1 seq=1 seq=2 t=1 mm=1 st=valid age=0"),
              ParseError::duplicate_field);
}

TEST(ParseRangeTest, RejectsBadNumbers)
{
    for (const char* mm : {"-1", "+1", "12a", "", "4294967296", "0x10"})
    {
        const std::string line = std::string {"RANGE v=1 seq=1 t=1 mm="} + mm + " st=valid age=0";
        EXPECT_EQ(error_of(line), ParseError::bad_number) << line;
    }
}

TEST(ParseRangeTest, RejectsUnknownStatus)
{
    EXPECT_EQ(error_of("RANGE v=1 seq=1 t=1 mm=1 st=VALID age=0"), ParseError::unknown_status);
    EXPECT_EQ(error_of("RANGE v=1 seq=1 t=1 mm=1 st= age=0"), ParseError::unknown_status);
}

TEST(ParseRangeTest, RejectsMalformedSpacingAndTokens)
{
    EXPECT_EQ(error_of("RANGE v=1  seq=1 t=1 mm=1 st=valid age=0"), ParseError::malformed_field);
    EXPECT_EQ(error_of("RANGE v=1 seq=1 t=1 mm=1 st=valid age=0 "), ParseError::malformed_field);
    EXPECT_EQ(error_of("RANGE v=1 seq=1 t=1 mm=1 st=valid age=0 junk"), ParseError::malformed_field);
    EXPECT_EQ(error_of("RANGE v=1 =5 seq=1 t=1 mm=1 st=valid age=0"), ParseError::malformed_field);
}

TEST(ParseRangeTest, RejectsCarriageReturnLeftByCaller)
{
    // Stripping CRLF is the line buffer's job; the parser must not quietly accept it.
    EXPECT_EQ(error_of("RANGE v=1 seq=1 t=1 mm=1 st=valid age=0\r"), ParseError::bad_number);
}

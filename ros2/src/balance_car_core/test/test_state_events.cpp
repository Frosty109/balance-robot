#include "balance_car_core/state_events.hpp"
#include "balance_car_core/line_buffer.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace balance_car::core;

namespace
{

std::optional<ParseError> error_of(std::string_view line)
{
    const auto result = parse_state_event(line);
    if (result)
    {
        return std::nullopt;
    }
    return result.error();
}

}  // namespace

TEST(StateEventTest, ParsesBoot)
{
    const auto event = parse_state_event("BOOT");
    ASSERT_TRUE(event.has_value());
    EXPECT_EQ(event->kind, StateEventKind::boot);
    EXPECT_FALSE(event->angle_cdeg.has_value());
    EXPECT_FALSE(event->reason.has_value());
    EXPECT_FALSE(event->last_fresh_ms.has_value());
    EXPECT_FALSE(event->now_ms.has_value());
}

TEST(StateEventTest, ParsesArmAcknowledgement)
{
    const auto event = parse_state_event("ARMED angle=-123");
    ASSERT_TRUE(event.has_value());
    EXPECT_EQ(event->kind, StateEventKind::armed);
    EXPECT_EQ(event->angle_cdeg, -123);
    EXPECT_FALSE(event->reason.has_value());
}

TEST(StateEventTest, ParsesDisarmReasons)
{
    const std::pair<const char*, StateReason> cases[] {
        {"boot", StateReason::boot},
        {"operator", StateReason::operator_request},
        {"angle", StateReason::angle},
        {"stale", StateReason::stale},
        {"recovering", StateReason::recovering},
        {"future_reason", StateReason::unknown},
    };
    for (const auto& [text, reason] : cases)
    {
        const auto event = parse_state_event(std::string {"DISARMED reason="} + text);
        ASSERT_TRUE(event.has_value()) << text;
        EXPECT_EQ(event->kind, StateEventKind::disarmed);
        EXPECT_EQ(event->reason, reason);
    }
}

TEST(StateEventTest, ParsesArmRejectionWithoutAngle)
{
    const auto event = parse_state_event("ARM REJECTED reason=recovering");
    ASSERT_TRUE(event.has_value());
    EXPECT_EQ(event->kind, StateEventKind::arm_rejected);
    EXPECT_EQ(event->reason, StateReason::recovering);
    EXPECT_FALSE(event->angle_cdeg.has_value());
}

TEST(StateEventTest, ParsesArmRejectionWithAngle)
{
    const auto event = parse_state_event("ARM REJECTED reason=angle angle=1234");
    ASSERT_TRUE(event.has_value());
    EXPECT_EQ(event->kind, StateEventKind::arm_rejected);
    EXPECT_EQ(event->reason, StateReason::angle);
    EXPECT_EQ(event->angle_cdeg, 1234);
}

TEST(StateEventTest, ParsesAngleFault)
{
    const auto event = parse_state_event("FAULT angle=-4015");
    ASSERT_TRUE(event.has_value());
    EXPECT_EQ(event->kind, StateEventKind::angle_fault);
    EXPECT_EQ(event->angle_cdeg, -4015);
}

TEST(StateEventTest, ParsesRecovery)
{
    const auto event = parse_state_event("RECOVERED angle=995");
    ASSERT_TRUE(event.has_value());
    EXPECT_EQ(event->kind, StateEventKind::recovered);
    EXPECT_EQ(event->angle_cdeg, 995);
}

TEST(StateEventTest, ParsesStaleEvent)
{
    const auto event = parse_state_event("STALE last=0 now=4294967295");
    ASSERT_TRUE(event.has_value());
    EXPECT_EQ(event->kind, StateEventKind::stale);
    EXPECT_EQ(event->last_fresh_ms, 0u);
    EXPECT_EQ(event->now_ms, std::numeric_limits<std::uint32_t>::max());
}

TEST(StateEventTest, AcceptsFieldOrderAndUnknownKeys)
{
    const auto event = parse_state_event("STALE now=125 extra=x last=100");
    ASSERT_TRUE(event.has_value());
    EXPECT_EQ(event->kind, StateEventKind::stale);
    EXPECT_EQ(event->last_fresh_ms, 100u);
    EXPECT_EQ(event->now_ms, 125u);
}

TEST(StateEventTest, RejectsOtherLineTypes)
{
    for (const char* line : {
        "", "boot", "BOOT extra=x", "BOOT\r", "ARMEDNESS angle=1",
        "ARM REJECTEDNESS reason=angle", "IMU OK", "MOTOR OK",
        "CONFIG velocity_integral_limit=2000", "LINK UP",
        "RANGE v=1 seq=1 t=1 mm=1 st=valid age=0",
        "state=DISARMED angle=1 gz=0 battery=750 t=1 poll=4 dtmax=11",
        "angle=1 bal=0 i=0 tr=0 vx=0 wz=0 gz=0 L=0 R=0 enc_l=0 enc_r=0 t=1 poll=4",
    })
    {
        EXPECT_EQ(error_of(line), ParseError::wrong_line_type) << line;
    }
}

TEST(StateEventTest, RejectsMissingRequiredFields)
{
    for (const char* line : {
        "ARMED extra=x", "DISARMED extra=x", "ARM REJECTED angle=100",
        "FAULT extra=x", "RECOVERED extra=x", "STALE last=100", "STALE now=125",
    })
    {
        EXPECT_EQ(error_of(line), ParseError::missing_field) << line;
    }
}

TEST(StateEventTest, RejectsDuplicateKnownFields)
{
    for (const char* line : {
        "ARMED angle=1 angle=2",
        "DISARMED reason=operator reason=angle",
        "STALE last=1 last=2 now=3",
        "STALE last=1 now=2 now=3",
    })
    {
        EXPECT_EQ(error_of(line), ParseError::duplicate_field) << line;
    }
}

TEST(StateEventTest, RejectsBadSignedNumbers)
{
    for (const char* value : {"+1", "12a", "0x10", "2147483648", "-2147483649"})
    {
        const auto line = std::string {"ARMED angle="} + value;
        EXPECT_EQ(error_of(line), ParseError::bad_number) << line;
    }
}

TEST(StateEventTest, RejectsBadUnsignedNumbers)
{
    for (const char* value : {"-1", "+1", "12a", "4294967296"})
    {
        const auto line = std::string {"STALE last=0 now="} + value;
        EXPECT_EQ(error_of(line), ParseError::bad_number) << line;
    }
}

TEST(StateEventTest, AcceptsSignedLimits)
{
    const auto low = parse_state_event("ARMED angle=-2147483648");
    const auto high = parse_state_event("ARMED angle=2147483647");
    ASSERT_TRUE(low.has_value());
    ASSERT_TRUE(high.has_value());
    EXPECT_EQ(low->angle_cdeg, std::numeric_limits<std::int32_t>::min());
    EXPECT_EQ(high->angle_cdeg, std::numeric_limits<std::int32_t>::max());
}

TEST(StateEventTest, RejectsMalformedFieldsAndSpacing)
{
    for (const char* line : {
        "ARMED  angle=1", "ARMED angle=1 ", "ARMED angle=1 junk",
        "ARMED =1 angle=1", "ARMED angle=", "DISARMED reason=",
        "ARMED angle=1=2", "ARMED angle=1\r", "ARMED angle=1\n",
        "ARMED angle=1 extra=x\t", "ARMED angle=1 extra=x\r",
    })
    {
        EXPECT_EQ(error_of(line), ParseError::malformed_field) << line;
    }
}

TEST(StateEventTest, ParsesEventsFromFragmentedCrLfInput)
{
    LineBuffer<> buffer;
    std::vector<StateEvent> events;
    const auto feed = [&](std::string_view chunk)
    {
        for (const char byte : chunk)
        {
            const auto framed = buffer.feed(byte);
            ASSERT_NE(framed.kind, LineEventKind::too_long);
            if (framed.kind == LineEventKind::line)
            {
                const auto event = parse_state_event(framed.line);
                ASSERT_TRUE(event.has_value());
                events.push_back(*event);
            }
        }
    };

    feed("BOOT\r\nARM");
    feed("ED angle=-20\r\nDISARMED reason=operator\n");

    ASSERT_EQ(events.size(), 3u);
    EXPECT_EQ(events[0].kind, StateEventKind::boot);
    EXPECT_EQ(events[1].kind, StateEventKind::armed);
    EXPECT_EQ(events[1].angle_cdeg, -20);
    EXPECT_EQ(events[2].kind, StateEventKind::disarmed);
    EXPECT_EQ(events[2].reason, StateReason::operator_request);
}